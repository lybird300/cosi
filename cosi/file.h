/* $Id: file.h,v 1.3 2011/05/04 15:46:19 sfs Exp $ */
#ifndef __INCLUDE_COSI_FILE_H
#define __INCLUDE_COSI_FILE_H
#include <cstdio>
#include <string>
#include <boost/filesystem.hpp>
#include <cosi/defs.h>
#include <cosi/decls.h>
#include <cosi/geneconversion.h>
#include <cosi/generalmath.h>

namespace cosi {

//
// Class: ParamFileReader
//
// Parses cosi <parameter files>, calls on other objects (notably <Demography> and <HistEvents>)
// as needed to construct a programmatic representation of the demographic model
// to be simulated, and stores some parameters inside itself for later retrieval.
//
class ParamFileReader {
public:
	 ParamFileReader( DemographyP demography_ );

	 

	 // MethodP: file_read
	 // Parse the specified parameter file.
	 void file_read( boost::filesystem::path filename, FILE *segfp );
	 
	 GenMapP getGenMap() const { return genMap; }
	 unsigned long getRandomSeed() const { return rseed; }
	 len_bp_int_t getLength() const { return length; }
	 prob_per_bp_per_gen_t getMu() const { return mu; }
	 factor_t getGeneConv2RecombRateRatio() const { return geneConv2RecombRateRatio; }
	 len_bp_int_t getGeneConversionMeanTractLength() const { return geneConversionMeanTractLength; }
	 len_bp_int_t getGeneConversionMinTractLength() const { return geneConversionMinTractLength; }
	 GeneConversion::GCModel getGeneConversionModel() const { return geneConversionModel; }
	 HistEventsP getHistEvents() const { return histEvents; }
	 bool_t getInfSites() const { return infSites; }

	 void setPrintSeed( bool_t printSeed_ ) { printSeed = printSeed_; }
	 void set_genMapShift( ploc_bp_diff_t genMapShift_ ) { this->genMapShift = genMapShift_; }
	 void set_recombfileFN( filename_t recombfileFN_ ) { this->recombfileFN = recombfileFN_; }

	 unsigned long getRandSeed() const { return rseed; }
	 bool_t isSeeded() const { return seeded; }

	 popid getIgnoreRecombsInPop() const { return this->ignoreRecombsInPop; }

	 typedef math::Function< genid, popsize_float_t, math::Piecewise< math::Const<> > > popSizeTraj_t;
	 typedef std::map< popid, popSizeTraj_t > pop2sizeTraj_t;

	 pop2sizeTraj_t get_pop2sizeTraj() const { return pop2sizeTraj; }
	 
	 
private:
	 // Field: demography
	 // The current state of the simulation.  As the parameter file is parsed,
	 // this object is initialized to the initial state of the simulation.
	 DemographyP demography;

	 // Field: seeded
	 // Whether a random seed has been specified in the parameter file.
	 bool_t seeded;

	 
	 int popsize;
	 int sampsize;
	 len_bp_int_t length;
	 prob_per_bp_per_gen_t mu;
	 double recombrate;
	 factor_t geneConv2RecombRateRatio;
	 len_bp_int_t geneConversionMeanTractLength;
	 len_bp_int_t geneConversionMinTractLength;
	 GeneConversion::GCModel geneConversionModel;
	 // Private field: rseed
	 // The random seed, if specified in the parameter file. 
	 unsigned long rseed;
	 GenMapP genMap;
	 HistEventsP histEvents;
	 bool_t infSites;
	 bool_t printSeed;
	 popid ignoreRecombsInPop;
	 boost::filesystem::path paramFileName;

	 // Field: genMapShift
	 // A shift applied to the genetic map: added to all physical positions in the genetic map file.
	 ploc_bp_diff_t genMapShift;

	 // Field: recombfileFN
	 // If non-empty, overrides the recomb file specified in the paramfile
	 filename_t recombfileFN;

	 // Field: pop2sizeTraj
	 // Map from pop to trajectory specification for that pop.
	 pop2sizeTraj_t pop2sizeTraj;

	 void init();
	 int file_get_data (FILE * fileptr, FILE *);
	 int file_proc_buff(char * var, char* buffer, FILE*);
	 int file_killwhitespace(FILE * fileptr);
	 void file_proc_recombfile (const char*  filename);
	 void file_exit(const char* , const char*);
	 void file_error_nonfatal(const char* , const char*);
};

typedef boost::shared_ptr<ParamFileReader> ParamFileReaderP;

}  // namespace cosi

#endif
