//
// notes
//
//   - if two begs at same place, the pair gets double-counted.

//   - if initial portion coalesces, have to subtract from number at zero.

//   - could unify the handling with beg0:
//       - at each pt, keep a count of how many beg here
//       - if >=2, count the pairs
//       - then, how many cross this pt?  (tot - endbefore - begin-here-or-later)
//       - if smth ends at or past 1, it doesn't endbefore.

//       - if two begs at same pt coalesce, decrement count of begs there
//
//    - what happens if a portion coalesces?
//
//         - only matters if two ends or two begs match?
//
//                         ==================
//                         =================
//
//               - then, both begs might disappear, and then possibly a new hullbeg be inserted (or not, if no new node gets created).
//               - for purpose of indexing count these as two nodes (or actually keep them as separate).
//
//               - if two ends match, count of ends there goes down.  also, possibly, a new hullend is inserted which was not there before.
//
//    - so, this deletes the old beg and end, correctly
//
//    - node can hold iterator to the beg/end records of its hull
//
//    - need ability to write the weight field, to adjust when inserting.
//    - if range being updated isn't large, simpler to just manually update?
//        - how common are large-range updates?  because if not common, may be more efficient to just update manually.
//        - delayed insertion: just store things till needed for query?
//
//     - we're storing node ids... ok better to just allow duplicates in the skiplist.
//       should never be more than two here.  just make sure it's counted correctly.
//         - when deleting, does it matter which?  it does if they have different inters-counts.
//
//      - adjust the new-level constant as needed.   can rebalance the list if searches get long.
//

//#undef NDEBUG

#include <algorithm>
#include <boost/foreach.hpp>
#include <boost/typeof/typeof.hpp>
#include <boost/utility.hpp>
#include <boost/phoenix.hpp>
#include <boost/range.hpp>
#include <boost/range/adaptors.hpp>
#include <cosi/utils.h>
//#include <cosi/seglist.h>
#include <cosi/hullmgr.h>

namespace cosi {

#ifdef COSI_SUPPORT_COALAPX

static const loc_t INF_LOC( std::numeric_limits<cosi_double>::infinity() );

HullMgr::HullMgr( ): margin( MAX_LOC - MIN_LOC ), ninters( 0 ), isRestrictingCoalescence( false ) {
	//std::cerr.precision(14);

	// begs.setIntrusive( true );
	// ends.setIntrusive( true );

	// begs.insert( HullBeg( INF_LOC, ninters_t( 0 ), (Node *)NULL ) );
	
	// ost_t t;

	// ost_iter_t v1 = t.insert( loc_t( .5 ) ).first;
	// ost_iter_t v2 = t.insert( loc_t( .5 ) ).first;
	// ost_iter_t v3 = t.insert( loc_t( .5 ) ).first;
	// PRINT3( v1.position(), v2.position(), v3.position() );
	// PRINT( t.lower_bound( loc_t( .5 ) ).position() );
	// PRINT( t.upper_bound( loc_t( .5 ) ).position() );
	// t.erase( v2 );
	// PRINT2( v1.position(), v3.position() );
}

HullMgr::~HullMgr() { }

// Function: last_among_equal
// Returns an iterator pointing to the rightmost item in a range of equal items.
// template <typename ForwardIterator>
// ForwardIterator last_among_equal( ForwardIterator i, ForwardIterator end ) {
// 	while ( i =! end ) {
// 		ForwardIterator n = boost::next( i );
// 		if ( n != end && *n == *i )
// 			 i = n;
// 		else
// 			 break;
// 	}
// 	return i;
// }

// Function: first_among_equal
// Returns an iterator pointing to the leftmost item in a range of equal items.
// template <typename BidiIterator>
// BidiIterator first_among_equal( BidiIterator i, BidiIterator begin ) {
// 	while ( i =! begin ) {
// 		BidiIterator p = boost::prior( i );
// 		if ( *p == *i )
// 			 i = p;
// 		else
// 			 break;
// 	}
// 	return i;
// }

//const len_t smallestLen( std::numeric_limits<cosi_double>::min() );

void HullMgr::addHull( loc_t beg, loc_t end, Hull *hullPtr ) {
	assert( beg < end );
	if ( !isRestrictingCoalescence ) return;
	Hull& hull = *hullPtr;
	//PRINT6( "bef_add_hull", beg, end, begs.size(), ends.size(), getNumIntersections() );
	ost_ends_t::iterator end_it = ends.insert( end ).first;
	ost_begs_t::iterator end_lb_in_begs = begs.lower_bound( loc_t( end + margin ) );
	nchroms_t nbegsBefEnd = end_lb_in_begs.position();
	nchroms_t nendsBefBeg = ends.upper_bound( loc_t( beg - margin ) ).position();
	nchromPairs_t ninters_added = nbegsBefEnd - nendsBefBeg;

	ninters += ninters_added;
	hull = begs.insert( HullBeg( beg, end_it, hullPtr ) ).first;

	//assert( nbegsBefEnd >= result.position() );
	hull.addWeight( ninters_added - ( nbegsBefEnd - hull.position() ) );
	
	begs.addWeightFast( boost::next( hull ), end_lb_in_begs, nchromPairs_t( 1 ) );

	chkMap();
	
	//PRINT6( "aft_add_hull", beg, end, begs.size(), ends.size(), getNumIntersections() );
	//return result;
}

void HullMgr::removeHull( Hull *hullPtr ) {
	if ( !isRestrictingCoalescence ) return;
	const Hull& hull = *hullPtr;
	assert( hull->hullPtr == hullPtr );
	loc_t beg = hull->beg, end = *hull->end_it;
	//PRINT6( "bef_rem_hull", beg, end, begs.size(), ends.size(), getNumIntersections() );

	assert( beg < end );

//	nchroms_t hullPos = hull.position();

	ost_begs_t::iterator beg_succ = boost::next( hull );
	
	ends.erase( hull->end_it );
	begs.erase( hull );

	ost_begs_t::iterator end_lb_in_begs = begs.lower_bound( loc_t( end + margin ) );
	nchroms_t nbegsBefEnd = end_lb_in_begs.position();
	nchroms_t nendsBefBeg = ends.upper_bound( loc_t( beg - margin ) ).position();
	
	begs.addWeightFast( beg_succ, end_lb_in_begs, nchromPairs_t( -1 ) );

	ninters -= ( nbegsBefEnd - nendsBefBeg );

	chkMap();
	//PRINT6( "aft_rem_hull", beg, end, begs.size(), ends.size(), getNumIntersections() );
}

//
// Method: chkMap
//
// Check that for each beg, the intersection count at that beg is correct (matches intersections
// counted explicitly).
//
void HullMgr::chkMap() const {
#if !defined(NDEBUG) && defined(COSI_ASSERTS_2)
	if ( !isRestrictingCoalescence ) return;
	
//#error "chego-chego?"
	{
		ost_begs_t::const_iterator b = begs.begin();
		for ( ost_begs_t::const_iterator hull1 = b; hull1 != begs.end(); hull1++ ) {
			namespace ph = boost::phoenix;
			using ph::bind;
			using ph::arg_names::arg1;
			using ph::val;
			
				
			nchromPairs_t ninters_direct = std::count_if( begs.begin(), hull1,
																										bind( &HullBeg::getEnd, arg1 ) >
																										val( hull1->getBeg() - margin ) );
			
			//PRINT2( ninters_direct, hull1->ninters );
			
			assert( ninters_direct == hull1.weightActual() );
			
			
			// ost_begs_t::const_iterator hull2 = boost::prior( hull1 );
			// while ( residue > 1 ) {
			// 	if ( hull2->getEnd() + margin > hull1->getBeg() )
			// 		 residue--;
			// 	if ( residue > 1 ) {
			// 		assert( hull2 != b );
			// 		hull2 = boost::prior( hull2 );
			// 	}
			// }
			
		}
	}
	
	
	//typedef ost_begs_t::iterator ost_begs_iter_t;
	nchromPairs_t totPairs = 0;
	nchroms_t begIdx = 0;
	for( ost_begs_t::const_iterator beg = begs.begin(); beg != begs.end(); beg++ ) {
		//PRINT3( beg.getBeg(), beg.getEnd(), beg.ninters );
		nchroms_t nendsBefBeg = ends.upper_bound( loc_t( beg->beg - margin ) ).position();

		assert( beg.weightActual() == ( begIdx - nendsBefBeg ) );
		
		totPairs += beg.weightActual();
		begIdx++;
	}
	//PRINT2( totPairs, ninters );
	assert( totPairs == ninters );

	static int checkedHere = 0;

	if ( !( checkedHere % 1000 ) ) { PRINT4( checkedHere, begs.size(), ends.size(), ninters ); } 
	
	checkedHere++;
#endif	// #ifndef NDEBUG
}  // HullMgr::chkMap()

// Method: recomb
// Called after each recombination to update the hull information.
// Params:
//    beg1,end1 and beg2,end2 - the two hulls resulting from the recombination.
//    The original hull was [beg1,end2].  If the recomb point fell in the middle of a seg,
//    then end1 == beg2.  If the recomb point fell in a gap, then end1 < beg2.
//
// Complexity: lg n, where n is the current number of hulls.
//
// void HullMgr::recomb( Hull hull, Node *newNode1, Node *newNode2,
// 											loc_t beg1, loc_t end1, loc_t beg2, loc_t end2, Hull *newHull1, Hull *newHull2 ) {
// //	PRINT11( "bef_recomb", getNumIntersections(), nbegs0, begs.size(), ends.size(), beg1, end1, beg2, end2, loc, end1 == beg2 );
// 	removeHull( hull );
// 	*newHull1 = addHull( newNode1, beg1, end1 );
// 	*newHull2 = addHull( newNode2, beg2, end2 );
// //	PRINT11( "aft_recomb", getNumIntersections(), nbegs0, begs.size(), ends.size(), beg1, end1, beg2, end2, loc, end1 == beg2 );	
	
// #if 0	
// 	PRINT11( "bef_recomb", getNumIntersections(), nbegs0, begs.size(), ends.size(), beg1, end1, beg2, end2, loc, end1 == beg2 );
	
// 	assert( MIN_LOC <= beg1 && beg1 < end1 && end1 <= beg2 && beg2 < end2 && end2 <= MAX_LOC );
	
// 	using std::make_pair;
	
// 	//
// 	// What happens here:
// 	//
// 	//    - a new beg is added, beg2.  We must compute the number of intersections starting at beg2,
// 	//    i.e. the number of hulls in the middle of which beg2 falls.
// 	//    Beware that there may already be one hull beginning at beg2, if the seg immediately left of beg2 has
// 	//    fully coalesced; if there is a beg at beg2, its intersection
// 	//    with beg2 must be counted.
// 	//
// 	//    beg2 cannot be MIN_LOC, so numBeg0 is not affected.
// 	// 
// 	//    - a new end is added, end1.  We add it to <ends>.  Note that there may already be one end here,
// 	//    if the seg immediately right of end1 has fully coalesced.
// 	//
// 	//    - any existing begs in [beg2, end1+margin) previously intersected one hull, now they intersect two;
// 	//      so increment the count for them.
// 	//    
// 	//
	
// 	// Step 1: Count how many intersections that begin at beg2 are added.
// 	//
// 	// We subtract from the total:
// 	//     - any hulls that end before or at beg2
// 	//     - any hulls that begin after beg2.
// 	//
// 	// For hulls that begin at beg2, there may be one such existing hull (if seg left of beg2 has
// 	// fully coalesced).  We can say that beg2 landed slightly to the right of it,
// 	// so we do not subtract it from the total.  The alternative would have been to
// 	// say that we land slightly to the left; which can't, because it has coalesced.
// 	//
// 	assert( begs.find( loc ) == begs.end() );
// 	assert( ends.find( loc ) == ends.end() );
	
// 	assert(  // loc either broke an existing seg ...
// 		( end1 == beg2 && beg2 == loc )
// 		// ...or fell in a gap
// 		|| ( end1 < loc && loc < beg2 ) );
	
// 	begs.check();
// 	ends.check();
	
// 	// Determine the number of intersection that will start at beg2.
// 	// (Possibly including the intersection with the new hull [beg1,end1+margin]).
	
// 	// How many hulls begin left of loc?
// 	// How many end left of loc-margin?
	
// 	// (both of these can be answered on the same traversal that inserted loc;
// 	// initially, can just ask these from scratch)
	
// 	loc_t end1margin( end1 + margin );
	
// 	//if ( end1margin < MAX_LOC )
// 	ends.insert( end1 );
	
// 	PRINT3( "cntbef", begs.count( beg2 ), ends.count( end1 ) );
	
// 	PRINT( end1margin );
	
// 	ost_begs_t::const_iterator beg2_it = begs.lower_bound( beg2 );
	
// #ifndef	NDEBUG
// 	ost_begs_t::const_iterator end1margin_it = begs.lower_bound( end1margin );
// 	assert( end1margin_it == begs.end() || end1margin_it->first > end1margin );
// #endif	
	
// 	begs.addWeight( beg2, end1margin, 1 );
// 	begs.addWeight( end1margin, len_t( beg2 - smallestLen ), -1 ) ;
	
// 	if ( equal_eps( beg2, loc_t( 0.63281095310391 ) ) ) {
// 		if ( beg2_it != begs.end() ) {
// 			PRINT3( "nu", beg2_it->first, beg2_it->second );
// 			if ( beg2_it.position() > 0 ) PRINT2( boost::prior( beg2_it )->first, boost::prior( beg2_it )->second );
// 			if ( boost::next( beg2_it ) != begs.end() ) PRINT2( boost::next( beg2_it )->first, boost::next( beg2_it )->second );
// 		}
// 	}
	
// 	nchroms_t n_begs_bef_beg2 = nbegs0 + beg2_it.position();
// 	assert( ends.find( loc_t( beg2 - margin ) ) == ends.end() );
// 	nchroms_t n_ends_bef_beg2 = ends.upper_bound( loc_t( beg2 - margin ) ).position();
	
// 	ninters_t ninters_at_beg2 = n_begs_bef_beg2 - n_ends_bef_beg2;
	
// 	PRINT6( n_begs_bef_beg2, n_ends_bef_beg2, ninters_at_beg2, beg2_it == begs.end(), beg2_it == begs.end() ? 0 : beg2_it->second, beg2_it == begs.end() ? MIN_LOC : beg2_it->first );

// 	static size_t counter = 0;
// 	if ( counter == 2248 ) {
		
// 	}

// 	PRINT( counter++ );
	
	
// 	assert( beg2_it == begs.end() || beg2_it->first > beg2  ||  ( beg2_it->first == beg2 && beg2_it->second == ninters_at_beg2+1 ) );
	
// 	ost_begs_t::const_iterator beg2_new_it = begs.insert( make_pair( beg2, ninters_at_beg2 ) ).first;

// 	assert( beg2_new_it->first == beg2 );
// 	assert( beg2_new_it->second == ninters_at_beg2 );
// 	assert( beg2_it == begs.end() || beg2_it->first > beg2 || boost::next( beg2_it ) == beg2_new_it );
	
// 	//
// 	// 
// 	//


// 	PRINT3( "cntaft", begs.count( beg2 ), ends.count( end1 ) );
		
// 	// debugging / reporting
	
// 	nchroms_t tot_chroms = nbegs0 + begs.size();
// 	PRINT13( "aft_recomb", getNumIntersections(), nbegs0, begs.size(), ends.size(), beg1, end1, beg2, end2, n_begs_bef_beg2, n_ends_bef_beg2, tot_chroms, loc );
// #endif	
// }  // HullMgr::recomb()


// void HullMgr::coalesce( Hull hull1, Hull hull2 ) {
// //	PRINT9( "bef_coal", getNumIntersections(), nbegs0, begs.size(), ends.size(), beg1, end1, beg2, end2 );
// 	removeHull( hull1 );
// 	removeHull( hull2 );
// //	PRINT9( "aft_coal", getNumIntersections(), nbegs0, begs.size(), ends.size(), beg1, end1, beg2, end2 );
// }
	
// //
// // Method: coalesce
// //
// // Called after two nodes coalesce, to update the hull information and intersection count.
// HullMgr::Hull HullMgr::coalesce( Hull hull1, Hull hull2, Node *newNode,
// 																 loc_t newbeg, loc_t newend ) {
// 	// PRINT11( "bef_coal", getNumIntersections(), nbegs0, begs.size(), ends.size(), beg1, end1, beg2, end2,
// 	// 				newbeg, newend );
// 	removeHull( hull1 );
// 	removeHull( hull2 );
// 	return addHull( newNode, newbeg, newend );
// 	// PRINT11( "aft_coal", getNumIntersections(), nbegs0, begs.size(), ends.size(), beg1, end1, beg2, end2,
// 	// 				newbeg, newend );
	
// #if 0	
// 	PRINT9( "bef_coal", getNumIntersections(), nbegs0, begs.size(), ends.size(), beg1, end1, beg2, end2 );

// 	loc_t overlap_beg = std::max( beg1, beg2 );
// 	loc_t overlap_end = std::min( end1, end2 );

// 	loc_t overlap_end_m( overlap_end + margin ); 
// 	assert( overlap_beg < overlap_end_m );

// 	assert( overlap_beg == MIN_LOC || begs.find( overlap_beg ) != begs.end() );
// 	assert( overlap_end_m > MAX_LOC || ends.find( overlap_end ) != ends.end() );
	
// 	if ( overlap_beg == MIN_LOC )
// 		nbegs0--;

// 	//
// 	// overlap_beg is being deleted from begs.
// 	// Let's count how many hull intersections there were starting at overlap_beg; we'll need to subtract them
// 	// from the inters count.  If there are multiple begs at overlap_beg, we say that the one we're deleting
// 	// was the rightmost.
// 	//

// 	PRINT2( ends.count( overlap_end ), begs.count( overlap_beg ) );

// 	ost_ends_iter_t i_end = ends.find( overlap_end );
// 	if ( i_end != ends.end() ) ends.erase( i_end );

// 	ost_begs_iter_t i_beg = begs.find( overlap_beg );
// 	if ( i_beg != begs.end() ) begs.erase( i_beg );

// 	PRINT2( ends.count( overlap_end ), begs.count( overlap_beg ) );
	

// 	begs.addWeight( overlap_beg, overlap_end_m, -1 );

// 	//if ( overlap_end_m < MAX_LOC ) ends.erase( overlap_end );
	
	
// 	PRINT11( "aft_coal", getNumIntersections(), nbegs0, begs.size(), ends.size(), beg1, end1, beg2, end2, overlap_beg, overlap_end  );
// #endif
// }  // HullMgr::coalesce()

std::pair< const HullMgr::Hull *, const HullMgr::Hull * >
HullMgr::chooseRandomIntersection( RandGenP randGen ) const {

	namespace ph = boost::phoenix;
	namespace ran = boost::range;
	namespace ad = boost::adaptors;

	assert( getNumIntersections() > 0 );
	nchromPairs_t inters_idx = randGen->random_idx( getNumIntersections() );

	PRINT2( inters_idx, getNumIntersections() );

	nchromPairs_t residue = 0;

//	static size_t requestId = 0;
	
	ost_begs_t::const_iterator hull1 = begs.upper_bound_for_inclusive_partial_sum( inters_idx, &residue );
	//PRINT4( inters_idx, hull1.position(), hull1->ninters, residue );
	assert( hull1 != begs.end() );
	assert( 1 <= residue && residue <= hull1.weightActual() );
	//nchroms_t hull1_pos = hull1.position();

	ost_begs_t::const_iterator hull2 = hull1;
	ost_begs_t::const_iterator b = begs.begin();
	loc_t begCmp = loc_t( hull1->getBeg() - margin );
	while ( true ) {
		assert( hull2 != b );
		hull2 = boost::prior( hull2 );
		if ( hull2->getEnd() > begCmp )
			 residue--;
		if ( residue == 0 ) break;
	}

	// BOOST_AUTO( hull2,
	// 						boost::begin( std::make_pair( begs.begin(), hull1 )
	// 													| ad::filtered( ph::bind( &HullBeg::getEnd, ph::arg_names::arg1 ) > ph::val( hull1->getBeg() - margin ) )
	// 													| ad::sliced( residue-1, residue ) ) );
							
	// ost_begs_t::const_iterator b = begs.begin();
	// assert( hull1 != b );

	// if ( requestId == 2261 ) {

	// 	nchromPairs_t ninters_direct = std::count_if( begs.begin(), hull1,  );		
		
	// 	PRINT8( begs.size(), ends.size(), ninters,
	// 					inters_idx, hull1.position(), hull1->ninters, ninters_direct, residue );

	// }

	// ost_begs_t::const_iterator hull2 = hull1;
	// while ( residue > 1 ) {
	// 	if ( hull2->getEnd() + margin > hull1->getBeg() )
	// 		 residue--;
	// 	if ( residue > 1 ) {
	// 		assert( hull2 != b );
	// 		hull2 = boost::prior( hull2 );
	// 	}
	// }

	// node::Node *node1 = hull1->node;
	// node::Node *node2 = hull2->node;
	// assert( node1 != node2 );

	// if ( seglist_beg( node1->getSegs() ) - seglist_end( node2->getSegs() ) > margin ||
	// 		 seglist_beg( node2->getSegs() ) - seglist_end( node1->getSegs() ) > margin ) {
	// 	PRINT4( requestId, begs.size(), ends.size(), ninters );
	// }
	
	// requestId++;
	
	// assert (	!( seglist_beg( node1->getSegs() ) - seglist_end( node2->getSegs() ) > margin ||
	// 						 seglist_beg( node2->getSegs() ) - seglist_end( node1->getSegs() ) > margin ) );

	return std::make_pair( static_cast< Hull *>( hull1->hullPtr ),
												 static_cast< Hull *>( hull2->hullPtr ) );
	
	
#if 0
	int nattempts = 0;


	
	while ( true ) {
		nattempts++;

//		if ( nattempts == 6 ) std::cerr << "\n";


		size_t hull1_idx = randGen->random_idx( begs.size() );
		
		Hull hull1 = begs[ hull1_idx ];
		size_t hull1_beg_succ_pos = hull1.position() + 1;
		size_t hull1_end_pos = begs.lower_bound( loc_t( hull1->getEnd() + margin ) ).position();
		ninters_t nbegs_inside_hull1 = hull1_end_pos - hull1_beg_succ_pos;

		assert( nbegs_inside_hull1 >= 0 );

		if ( nbegs_inside_hull1 == 0 ) continue;

		size_t hull2_idx;

		if ( ninters == 1 ) {
			assert( nbegs_inside_hull1 == 1 );
			hull2_idx = 0;
		} else {

			prob_t rejectProb = 1.0 - std::min( ((double)begs.size()), ((double)ninters) / begs.size() ) * ( ((double)nbegs_inside_hull1) / ((double)ninters ) );
			frac_t rejectCoinResult = randGen->random_double();

			if ( nattempts > 5 ) {
			 	 PRINT10( nattempts, nbegs_inside_hull1, ninters, begs.size(), ends.size(), hull1_idx,
			 						hull1_beg_succ_pos, hull1_end_pos, rejectProb, rejectCoinResult );
			}
			if ( rejectCoinResult < rejectProb ) {
				extern unsigned long nretry;
				nretry++;
				continue;
			}

			hull2_idx = randGen->random_idx( nbegs_inside_hull1 );
		}  // if ninters > 1

		Hull hull2 = begs[ hull1_beg_succ_pos + hull2_idx ];

		assert( hull1->node != hull2->node );
		
		return std::make_pair( hull1->node, hull2->node );
	}
#endif // #if 0	
}  // HullMgr::chooseRandomIntersection()


#endif  // #ifdef COSI_SUPPORT_COALAPX

}  // namespace cosi

