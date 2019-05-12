/*
 * SkimEvents.cpp
 *
 *  Created on: Dec 9, 2018
 *      Author: suyong
 */

#include "SkimEvents.h"
#include "utility.h"

SkimEvents::SkimEvents(TTree *t, std::string outfilename, std::string jsonfname, string globaltag, int nthreads)
:NanoAODAnalyzerrdframe(t, outfilename, jsonfname, globaltag, nthreads)
{

}

// Define your cuts here
void SkimEvents::defineCuts()
{
	// Cuts to be applied in order
	// These will be passed to Filter method of RDF
	// check for good json event is defined earlier
	//addCuts("HLT_PFHT450_SixJet40_BTagCSV_p056 || HLT_PFHT400_SixJet30_DoubleBTagCSV_p056 || HLT_PFHT900", "0"); // 2016 triggers
	addCuts("HLT_PFHT380_SixPFJet32_DoublePFBTagCSV_2p2 ||HLT_PFHT430_SixPFJet40_PFBTagCSV_1p5", "0");
	addCuts("njetspass>=6", "00");
	addCuts("bnjetspass>=3", "000");
	addCuts("Sel_jetHT>700.0", "0000");
}

void SkimEvents::defineMoreVars()
{
	addVar({"Sel_jetHT", "Sum(Sel_jetpt)"});

	// define variables that you want to store
	addVartoStore("run");
	addVartoStore("luminosityBlock");
	addVartoStore("event");
	addVartoStore("gen.*");
	addVartoStore("Pileup.*");
	addVartoStore("btagWeight.*");

	addVartoStore("nJet");
	addVartoStore("Jet_.*");
	addVartoStore("nFatJet");
	addVartoStore("FatJet_.*");
	addVartoStore("MET.*");
	addVartoStore("Puppi.*");
	addVartoStore("nElectron");
	addVartoStore("Electron_.*");
	addVartoStore("nMuon");
	addVartoStore("Muon_.*");
	addVartoStore("nSubJet");
	addVartoStore("SubJet.*");
	addVartoStore("PV*");
	addVartoStore("nOtherPV");
	addVartoStore("OtherPV_z");
	addVartoStore("HLT_PFHT.*");
	addVartoStore("Flag.*");
}

void SkimEvents::bookHists()
{
	// _hist1dinfovector contains the information of histogram definitions (as TH1DModel)
	// the variable to be used for filling
	// and the minimum cutstep for which the histogram should be filled
	//
	// The braces are used to initalize the struct
	// TH1D
	add1DHist( {"hcounter", "Event counter", 2, -0.5, 1.5}, "one", "evWeight", "");
}
