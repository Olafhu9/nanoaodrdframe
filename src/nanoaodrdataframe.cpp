//============================================================================
// Name        : nanoaodrdataframe.cpp
// Author      : Suyong Choi
// Version     :
// Copyright   : suyong@korea.ac.kr, Korea University, Department of Physics
// Description : Hello World in C, Ansi-style
//============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "NanoAODAnalyzerrdframe.h"
#include "FourtopAnalyzer.h"
#include "SkimEvents.h"
#include "TChain.h"

using namespace std;
using namespace ROOT;

int main(void) {
	//TChain c1("outputTree");
	//c1.Add("processed/2016data/2016b/00/nanoAOD_3_analyzed.root");
	//FourtopAnalyzer nanoaodrdf(&c1, "testout.root", "data/Cert_271036-284044_13TeV_23Sep2016ReReco_Collisions16_JSON.txt");

	TChain c1("Events");
	c1.Add("testinputdata/data/JetHT_2017C_DA05CA1A-3265-EE47-84F9-10CB09D22BDA.root"); //data
	//c1.Add("testinputdata/MC/2017/ttJets-2017MC-A258F579-5EC0-D840-95D7-4327595FC3DE.root"); // MC
	SkimEvents nanoaodrdf(&c1, "testout.root");

	string goodjsonfname = "data/Cert_294927-306462_13TeV_EOY2017ReReco_Collisions17_JSON.txt";
	string pileupfname = "data/LUM/2017_UL/puWeights.json.gz";
	string pileuptag = "Collisions17_UltraLegacy_goldenJSON";
	string btvfname = "data/BTV/2017_UL/btagging.json.gz";
	string btvtype = "deepJet_shape";
	string jercfname = "data/JERC/UL17_jerc.json.gz";
	string jerctag = "Summer19UL17_V5_MC_L1L2L3Res_AK4PFchs";
	string jercunctag = "Summer19UL17_V5_MC_Total_AK4PFchs";

	nanoaodrdf.setupCorrections(goodjsonfname, pileupfname, pileuptag, btvfname, btvtype, jercfname, jerctag, jercunctag);
	nanoaodrdf.setupAnalysis();
	nanoaodrdf.run(false, "outputTree2");

	return EXIT_SUCCESS;
}
