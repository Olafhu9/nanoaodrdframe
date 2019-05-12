/*
 * FourtopAnalyzer.cpp
 *
 *  Created on: Dec 4, 2018
 *      Author: suyong
 */

#include "FourtopAnalyzer.h"
#include "utility.h"

FourtopAnalyzer::FourtopAnalyzer(TTree *t, std::string outfilename, std::string jsonfname, string globaltag, int nthreads)
:NanoAODAnalyzerrdframe(t, outfilename, jsonfname, globaltag, nthreads)
{

}

// Define your cuts here
void FourtopAnalyzer::defineCuts()
{
	// Cuts to be applied in order
	// These will be passed to Filter method of RDF
	// check for good json event is defined earlier
	addCuts("(HLT_PFHT450_SixJet40_BTagCSV_p056 || HLT_PFHT400_SixJet30_DoubleBTagCSV_p056 || HLT_PFHT900) && nelepass==0 && nmuonpass==0", "0");
	addCuts("ncleanjetspass>=6  && ncleanbjetspass>=2 && Sel2_jetHT>700.0", "00");
	addCuts("ncleanjetspass>=7 && ncleanbjetspass>=2 && Sel2_jetHT>700.0", "01");
	addCuts("ncleanjetspass>=6 && ncleanbjetspass>=3 && Sel2_jetHT>700.0", "02");
	addCuts("ncleanjetspass>=7 && ncleanbjetspass>=3 && Sel2_jetHT>700.0", "000");
	addCuts("ncleanjetspass>=8 && ncleanbjetspass>=3 && Sel2_jetHT>700.0", "0000");
	addCuts("ncleanjetspass>=9 && ncleanbjetspass>=3 && Sel2_jetHT>800.0", "00000");

}
/*
template <typename T>
void FourtopAnalyzer::defineVar(std::string varname, T function,  const RDFDetail::ColumnNames_t &columns)
{
	NanoAODAnalyzerrdframe::defineVar(varname, function, columns);
}
*/

void FourtopAnalyzer::defineMoreVars()
{
	// define your higher level variables here
	addVar({"Sel2_jet6pt", "Sel2_jetpt[5]", "000"});
	defineVar("sphericityQ", ::sphericity , {"cleanjet4vecs"});
	addVar({"sphericityQ3", "sphericityQ[2]", "00"});
	addVar({"Sel2_jet8pt", "Sel2_jetpt[7]", "00000"});
	defineVar("legendreP0", [](FourVectorVec &p){return ::foxwolframmoment(0, p, 6);}, {"cleanjet4vecs"});
	defineVar("legendreP1", [](FourVectorVec &p){return ::foxwolframmoment(1, p, 6);}, {"cleanjet4vecs"});
	defineVar("legendreP2", [](FourVectorVec &p){return ::foxwolframmoment(2, p, 6);}, {"cleanjet4vecs"});
	defineVar("legendreP3", [](FourVectorVec &p){return ::foxwolframmoment(3, p, 6);}, {"cleanjet4vecs"});
	defineVar("legendreP4", [](FourVectorVec &p){return ::foxwolframmoment(4, p, 6);}, {"cleanjet4vecs"});
	defineVar("legendreP5", [](FourVectorVec &p){return ::foxwolframmoment(5, p, 6);}, {"cleanjet4vecs"});
	defineVar("legendreP6", [](FourVectorVec &p){return ::foxwolframmoment(6, p, 6);}, {"cleanjet4vecs"});
	defineVar("legendreP7", [](FourVectorVec &p){return ::foxwolframmoment(7, p, 6);}, {"cleanjet4vecs"});
	defineVar("legendreP10", [](FourVectorVec &p){return ::foxwolframmoment(10, p, 6);}, {"cleanjet4vecs"});
	defineVar("legendreP20", [](FourVectorVec &p){return ::foxwolframmoment(20, p, 6);}, {"cleanjet4vecs"});
	defineVar("legendreP30", [](FourVectorVec &p){return ::foxwolframmoment(30, p, 6);}, {"cleanjet4vecs"});
	defineVar("legendreP1b", [](FourVectorVec &p){return ::foxwolframmoment(1, p);}, {"cleanbjet4vecs"});
	defineVar("legendreP2b", [](FourVectorVec &p){return ::foxwolframmoment(2, p);}, {"cleanbjet4vecs"});
	defineVar("legendreP3b", [](FourVectorVec &p){return ::foxwolframmoment(3, p);}, {"cleanbjet4vecs"});
	defineVar("legendreP4b", [](FourVectorVec &p){return ::foxwolframmoment(4, p);}, {"cleanbjet4vecs"});
	defineVar("legendreP5b", [](FourVectorVec &p){return ::foxwolframmoment(5, p);}, {"cleanbjet4vecs"});
	addVar({"sphericity", "1.5*(sphericityQ[0]+sphericityQ[1])", "00"});


	// define variables that you want to store
	addVartoStore("run");
	addVartoStore("luminosityBlock");
	addVartoStore("event");
	addVartoStore("evWeight");
	addVartoStore("Sel_bjetweight");
	addVartoStore("sphericity");
	addVartoStore("sphericityQ3");
	addVartoStore("ncleanjetspass");
	//addVartoStore("cleanjet4vecs");
	addVartoStore("ncleanbjetspass");
	//addVartoStore("cleanbjet4vecs");
	addVartoStore("Sel2_jetHT");
	addVartoStore("Sel2_bjetHT");
	addVartoStore("Sel2_jet8pt");
	addVartoStore("legendre.*");

}

void FourtopAnalyzer::bookHists()
{
	// _hist1dinfovector contains the information of histogram definitions (as TH1DModel)
	// the variable to be used for filling
	// and the minimum cutstep for which the histogram should be filled
	//
	// The braces are used to initalize the struct
	// TH1D
	//add1DHist( {"hcounter", "Event counter", 2, -0.5, 1.5}, "one", "evWeight", 0);
	add1DHist( {"hnvtx_raw", "Number of Primary Vertex", 200, 0.0, 200.0}, "PV_npvsGood", "one", "");
	add1DHist( {"hnvtx", "Number of Primary Vertex", 200, 0.0, 200.0}, "PV_npvsGood", "evWeight", "");

	add1DHist( {"hnjetpass", "Passing jetcuts", 20, 0.0, 20.0}, "njetspass", "evWeight", "");
	add1DHist( {"hncleanjetpass", "Passing jetcuts", 20, 0.0, 20.0}, "ncleanjetspass", "evWeight", "");
	add1DHist( {"hjetpt", "Passing jet pt", 50, 0.0, 200.0}, "Sel2_jetpt", "Sel2_jetweight", "");
	add1DHist( {"hjetHT", "HT of Passing jet pt" , 50, 0.0, 2000.0}, "Sel2_jetHT", "evWeight", "");
	add1DHist( {"hnbjetpass", "Passing bjetcuts", 20, 0.0, 20.0}, "bnjetspass", "evWeight", "");
	add1DHist( {"hbjetHT", "HT of Passing bjet pt" , 50, 0.0, 2000.0}, "Sel2_bjetHT", "evWeight", "000");
	add1DHist( {"hnfatjets", "Number of fatjets", 10, 0.0, 10.0}, "nfatjetspass", "evWeight", "");
	add1DHist( {"hfatjetmass", "FatJet mass", 50, 0.0, 400}, "Sel_fatjetmass", "Sel_fatjetweight",  "");
	add1DHist( {"hq3", "Sphericity Q3", 50, 0.0, 1.0}, "sphericityQ3", "evWeight", "00");
	add1DHist( {"hsphericity", "Sphericity", 50, 0.0, 1.0}, "sphericity", "evWeight", "00");
	add1DHist( {"hlegendreP0", "P0", 100, 0.0, 1.01}, "legendreP0", "evWeight", "0000");
	add1DHist( {"hlegendreP1", "P1", 100, 0.0, 1.01}, "legendreP1", "evWeight", "0000");
	add1DHist( {"hlegendreP2", "P2", 100, 0.0, 1.01}, "legendreP2", "evWeight", "0000");
	add1DHist( {"hlegendreP3", "P3", 100, 0.0, 1.01}, "legendreP3", "evWeight", "0000");
	add1DHist( {"hlegendreP4", "P4", 100, 0.0, 1.01}, "legendreP4", "evWeight", "0000");
	add1DHist( {"hlegendreP5", "P5", 100, 0.0, 1.01}, "legendreP5", "evWeight", "0000");
	add1DHist( {"hlegendreP6", "P6", 100, 0.0, 1.01}, "legendreP6", "evWeight", "0000");
	add1DHist( {"hlegendreP7", "P7", 100, 0.0, 1.01}, "legendreP7", "evWeight", "0000");
	add1DHist( {"hlegendreP10", "P10", 100, 0.0, 1.01}, "legendreP10", "evWeight", "0000");
	add1DHist( {"hlegendreP20", "P20", 100, 0.0, 1.01}, "legendreP20", "evWeight", "0000");
	add1DHist( {"hlegendreP30", "P30", 100, 0.0, 1.01}, "legendreP30", "evWeight", "0000");
	add1DHist( {"hlegendreP1b", "P1b", 100, 0.0, 1.01}, "legendreP1b", "evWeight", "0000");
	add1DHist( {"hlegendreP2b", "P2b", 100, 0.0, 1.01}, "legendreP2b", "evWeight", "0000");
	add1DHist( {"hlegendreP3b", "P3b", 100, 0.0, 1.01}, "legendreP3b", "evWeight", "0000");
	add1DHist( {"hlegendreP4b", "P4b", 100, 0.0, 1.01}, "legendreP4b", "evWeight", "0000");
	add1DHist( {"hlegendreP5b", "P5b", 100, 0.0, 1.01}, "legendreP5b", "evWeight", "0000");

	// 6th jet should be filled after some cut
	//add1DHist({ {"hjet6pt", "Passing jet peading pt", 50, 0.0, 200.0}, "Sel2_jet6pt", "evWeight", 3} );
}
