/*
 * ttbbLepJetsAnalyzer.h
 *
 *  Created on: April 9, 2020
 *      Author: Tae Jeong Kim 
 */

#ifndef LQTOPANALYZER_H_
#define LQTOPANALYZER_H_

#include "NanoAODAnalyzerrdframe.h"

class ttbbLepJetsAnalyzer: public NanoAODAnalyzerrdframe
{
	public:
		ttbbLepJetsAnalyzer(TTree *t, std::string outfilename, std::string cat ="", std::string year="", std::string jsonfname="", string globaltag="", int nthreads=1);
		void defineCuts();
		void defineMoreVars(); // define higher-level variables from
		void bookHists();
        private:
                std::string cat;
                std::string year;

};



#endif /* LQTOPANALYZER_H_ */
