/*
 * SkimEvents.h
 *
 *  Created on: Dec 9, 2018
 *      Author: suyong
 */

#ifndef SKIMEVENTS_H_
#define SKIMEVENTS_H_

#include "NanoAODAnalyzerrdframe.h"
#include <string>

class SkimEvents: public NanoAODAnalyzerrdframe
{
	public:
		SkimEvents(TTree *t, std::string outfilename);
		void defineCuts();
		void defineMoreVars(); // define higher-level variables from
		void bookHists();
};



#endif /* SKIMEVENTS_H_ */
