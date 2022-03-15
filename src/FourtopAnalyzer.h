/*
 * FourtopAnalyzer.h
 *
 *  Created on: Dec 4, 2018
 *      Author: suyong
 */

#ifndef FOURTOPANALYZER_H_
#define FOURTOPANALYZER_H_

#include "NanoAODAnalyzerrdframe.h"

class FourtopAnalyzer: public NanoAODAnalyzerrdframe
{
	public:
		FourtopAnalyzer(TTree *t, std::string outfilename);
		void defineCuts();
		void defineMoreVars(); // define higher-level variables from
		void bookHists();

};



#endif /* FOURTOPANALYZER_H_ */
