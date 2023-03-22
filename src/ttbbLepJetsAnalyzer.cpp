/*
 * ttbbLepJetsAnalyzer.cpp
 *
 *  Created on: April 9, 2020
 *      Author: Tae Jeong Kim
 */

#include "ttbbLepJetsAnalyzer.h"
#include "utility.h"

ttbbLepJetsAnalyzer::ttbbLepJetsAnalyzer(TTree *t, std::string outfilename, std::string cat, std::string year, std::string jsonfname, string globaltag, int nthreads)
:NanoAODAnalyzerrdframe(t, outfilename, cat, year, jsonfname, globaltag, nthreads)
{

}

// Define your cuts here
void ttbbLepJetsAnalyzer::defineCuts()
{
	// Cuts to be applied in order
	// These will be passed to Filter method of RDF
	// check for good json event is defined earlier
        addCuts("nmuonpass == 1 && nvetoelepass == 0 && nvetomuons == 0","0");
        addCuts("ncleantaupass >= 1", "00");
        addCuts("ncleanjetspass >= 4", "000"); 
        addCuts("ncleanbjetspass == 1", "0000");
        addCuts("MET_pt<70","00000");
        //addCuts("chi2 <= 10000", "00000");
}
/*
template <typename T>
void ttbbLepJetsAnalyzer::defineVar(std::string varname, T function,  const RDFDetail::ColumnNames_t &columns)
{
	NanoAODAnalyzerrdframe::defineVar(varname, function, columns);
}
*/

void ttbbLepJetsAnalyzer::defineMoreVars()
{
        // define your higher level variables here

        addVar({"Sel_muon1pt", "Sel_muonpt[0]", ""});
        addVar({"Sel_muon1eta", "Sel_muoneta[0]", ""});
        addVar({"Sel_muon1phi", "Sel_muonphi[0]", ""});

        addVar({"Sel2_tau1pt", "Sel2_taupt[0]", ""});
        addVar({"Sel2_tau1eta", "Sel2_taueta[0]", ""});
        addVar({"Sel2_tau1phi", "Sel2_tauphi[0]", ""});

        //addVar({"Sel2_cjet1pt", "Sel2_cjetpt[0]", ""});
        //addVar({"Sel2_cjet1eta", "Sel2_cjeteta[0]", ""});
        //addVar({"Sel2_cjet1phi", "Sel2_cjetphi[0]", ""});

        addVar({"Sel2_jet1pt", "Sel2_jetpt[0]", ""});
        addVar({"Sel2_jet1eta", "Sel2_jeteta[0]", ""});
        addVar({"Sel2_jet1phi", "Sel2_jetphi[0]", ""});
        addVar({"Sel2_jet2pt", "Sel2_jetpt[1]", ""});
        addVar({"Sel2_jet2eta", "Sel2_jeteta[1]", ""});
        addVar({"Sel2_jet2phi", "Sel2_jetphi[1]", ""});
        addVar({"Sel2_jet3pt", "Sel2_jetpt[2]", ""});
        addVar({"Sel2_jet3eta", "Sel2_jeteta[2]", ""});
        addVar({"Sel2_jet3phi", "Sel2_jetphi[2]", ""});
        addVar({"Sel2_jet4pt", "Sel2_jetpt[3]", ""});
        addVar({"Sel2_jet4eta", "Sel2_jeteta[3]", ""});
        addVar({"Sel2_jet4phi", "Sel2_jetphi[3]", ""});

        addVar({"Sel2_bjet1pt", "Sel2_bjetpt[0]", ""});
        addVar({"Sel2_bjet1eta", "Sel2_bjeteta[0]", ""});
        addVar({"Sel2_bjet1phi", "Sel2_bjetphi[0]", ""});	

        defineVar("top_reco_whad", ::top_reconstruction_whad, {"cleanjet4vecs","cleanbjet4vecs","muon4vecs","cleantau4vecs"});
        addVar({"chi2","top_reco_whad[0]",""});
        addVar({"chi2_LQTop_mass","top_reco_whad[1]",""});
        addVar({"chi2_SMW_mass","top_reco_whad[2]",""});
        addVar({"chi2_SMTop_mass","top_reco_whad[3]",""});
        addVar({"chi2_lfvjet_idx","top_reco_whad[4]",""});
        addVar({"chi2_wjet1_idx","top_reco_whad[5]",""});
        addVar({"chi2_wjet2_idx","top_reco_whad[6]",""});
        addVar({"chi2_tau_idx","top_reco_whad[7]",""});

        defineVar("top_reco_prod", ::top_reco_products, {"cleanjet4vecs","muon4vecs","cleantau4vecs","top_reco_whad"});

        addVar({"chi2_wqq_dEta","top_reco_prod[0]",""});
        addVar({"chi2_wqq_absdEta","abs(chi2_wqq_dEta)",""});
        addVar({"chi2_wqq_dPhi","top_reco_prod[1]",""});
        addVar({"chi2_wqq_absdPhi","abs(chi2_wqq_dPhi)",""});
        addVar({"chi2_wqq_dR","top_reco_prod[2]",""});

        addVar({"chi2_lfvjmu_dEta","top_reco_prod[3]",""});
        addVar({"chi2_lfvjmu_absdEta","abs(chi2_lfvjmu_dEta)",""});
        addVar({"chi2_lfvjmu_dPhi","top_reco_prod[4]",""});
        addVar({"chi2_lfvjmu_absdPhi","abs(chi2_lfvjmu_dPhi)",""});
        addVar({"chi2_lfvjmu_dR","top_reco_prod[5]",""});
        addVar({"chi2_lfvjmu_mass","top_reco_prod[6]",""});
        
        addVar({"chi2_lfvjtau_dEta","top_reco_prod[7]",""});
        addVar({"chi2_lfvjtau_absdEta","abs(chi2_lfvjtau_dEta)",""});
        addVar({"chi2_lfvjtau_dPhi","top_reco_prod[8]",""});
        addVar({"chi2_lfvjtau_absdPhi","abs(chi2_lfvjtau_dPhi)",""});
        addVar({"chi2_lfvjtau_dR","top_reco_prod[9]",""});
        addVar({"chi2_lfvjtau_mass","top_reco_prod[10]",""});

        addVar({"chi2_mutau_dEta","top_reco_prod[11]",""});
        addVar({"chi2_mutau_absdEta","abs(chi2_mutau_dEta)",""});
        addVar({"chi2_mutau_dPhi","top_reco_prod[12]",""});
        addVar({"chi2_mutau_absdPhi","abs(chi2_mutau_dPhi)",""});
        addVar({"chi2_mutau_dR","top_reco_prod[13]",""});
        addVar({"chi2_mutau_mass","top_reco_prod[14]",""});

        addVar({"chi2_lfvjmutau_dEta","top_reco_prod[15]",""});
        addVar({"chi2_lfvjmutau_absdEta","abs(chi2_lfvjmutau_dEta)",""});
        addVar({"chi2_lfvjmutau_dPhi","top_reco_prod[16]",""});
        addVar({"chi2_lfvjmutau_absdPhi","abs(chi2_lfvjmutau_dPhi)",""});
        addVar({"chi2_lfvjmutau_dR","top_reco_prod[17]",""});
        addVar({"chi2_lfvjmutau_mass","top_reco_prod[18]",""});

        // define variables that you want to store
        addVartoStore("run");
        addVartoStore("luminosityBlock");
        addVartoStore("event");
        addVartoStore("evWeight");
        addVartoStore("nmuonpass");
        addVartoStore("ncleanjetspass");
        addVartoStore("ncleanbjetspass");
        addVartoStore("ncleantaupass");
        addVartoStore("MET_.*");
        addVartoStore("Sel_muon1.*");
        addVartoStore("Sel2_jet1.*");
        addVartoStore("Sel2_jet2.*");
        addVartoStore("Sel2_jet3.*");
        addVartoStore("Sel2_jet4.*");
        addVartoStore("Sel2_bjet1.*");
        addVartoStore("chi2_.*");

}

void ttbbLepJetsAnalyzer::bookHists()
{
        // _hist1dinfovector contains the information of histogram definitions (as TH1DModel)
        // the variable to be used for filling
        // and the minimum cutstep for which the histogram should be filled
        //
        // The braces are used to initalize the struct
        // TH1D
        add1DHist( {"hnevents", "Number of Events", 2, -0.5, 1.5}, "one", "evWeight", "");
        //add1DHist( {"hnvtx_raw", "Number of Primary Vertex", 200, 0.0, 200.0}, "PV_npvsGood", "one", "");
        //add1DHist( {"hnvtx", "Number of Primary Vertex", 200, 0.0, 200.0}, "PV_npvsGood", "evWeight", "");
        
        add1DHist( {"hmetpt", "MET pt", 30, 0.0, 300.0}, "MET_pt", "evWeight", "0");
        add1DHist( {"hsumet", "Sum ET", 50, 0.0, 5000.0}, "MET_sumEt", "evWeight", "0");
        add1DHist( {"hmetphi", "MET phi", 40, -4.0, 4.0}, "MET_phi", "evWeight", "0");

//        add1DHist( {"hnpvdof", "Number of PV of DoF", 50, 0.0, 50.0}, "PV_ndof", "evWeight", "0");
//        add1DHist( {"hnpvs", "Number of PVs", 100, 0.0, 100.0}, "PV_npvs", "evWeight", "0");
//        add1DHist( {"hnpvsgood", "Number of good PVs", 100, 0.0, 100.0}, "PV_npvsGood", "evWeight", "0");

        add1DHist( {"hnmuonpass", "Passing muoncuts", 5, 0.0, 5.0}, "nmuonpass", "evWeight", "0");
        add1DHist( {"hncleantaupass", "Passing taucuts", 5, 0.0, 5.0}, "ncleantaupass", "evWeight", "0");
        add1DHist( {"hncleanjetspass", "Passing jetcuts", 10, 0.0, 10.0}, "ncleanjetspass", "evWeight", "0");
        add1DHist( {"hncleanbjetspass", "Passing bjetcuts", 5, 0.0, 5.0}, "ncleanbjetspass", "evWeight", "0");
        
        add1DHist( {"hmuon1pt", "Passing leading muon pt", 30, 0.0, 300.0}, "Sel_muon1pt", "evWeight", "0"); 
        add1DHist( {"hmuon1eta", "Passing leading muon eta", 25, 0.0, 2.5}, "Sel_muon1eta", "evWeight", "0");
        add1DHist( {"htau1pt", "Passing leading tau pt", 30, 0.0, 300.0}, "Sel2_tau1pt", "evWeight", "00");
        add1DHist( {"htau1eta", "Passing leading tau eta", 25, 0.0, 2.5}, "Sel2_tau1eta", "evWeight", "00");
        
        add1DHist( {"hjet1pt", "Passing leading jet pt", 30, 0.0, 300.0}, "Sel2_jet1pt", "evWeight", "000");
        add1DHist( {"hjet1eta", "Passing leading jet eta", 25, 0.0, 2.5}, "Sel2_jet1eta", "evWeight", "000");
        add1DHist( {"hjet2pt", "Passing sub-leading jet pt", 30, 0.0, 300.0}, "Sel2_jet2pt", "evWeight", "000");
        add1DHist( {"hjet2eta", "Passing sub-leading jet eta", 25, 0.0, 2.5}, "Sel2_jet2eta", "evWeight", "000");
        add1DHist( {"hbjet1pt", "Passing b jet pt", 30, 0.0, 300.0}, "Sel2_bjet1pt", "evWeight", "0000");
        add1DHist( {"hbjet1eta", "Passing b jet eta", 25, 0.0, 2.5}, "Sel2_bjet1eta", "evWeight", "0000");

        add1DHist( {"hchi2", "Minimum chi2 for hadronic W", 30, 0, 30000}, "chi2", "evWeight","0000");
        add1DHist( {"hchi2_SMTop_mass", "chi2 SM Top mass", 25, 0, 500}, "chi2_SMTop_mass", "evWeight","0000");
        add1DHist( {"hchi2_SMW_mass", "chi2 SM W mass", 25, 0, 500}, "chi2_SMW_mass", "evWeight","0000");
        add1DHist( {"hchi2_LQTop_mass", "chi2 LQ Top mass", 25, 0, 500}, "chi2_LQTop_mass", "evWeight","0000");

        add1DHist( {"hchi2_wqq_dEta", "dEta of jets from W", 25, -5, 5}, "chi2_wqq_dEta", "evWeight","0000");
        add1DHist( {"hchi2_wqq_absdEta", "abs dEta of jets from W", 25, 0, 5}, "chi2_wqq_absdEta", "evWeight","0000");
        add1DHist( {"hchi2_wqq_dPhi", "dPhi of jets from W", 20, -4, 4}, "chi2_wqq_dPhi", "evWeight","0000");
        add1DHist( {"hchi2_wqq_absdPhi", "absdPhi of jets from W", 20, 0, 4}, "chi2_wqq_absdPhi", "evWeight","0000");
        add1DHist( {"hchi2_wqq_dR", "dR of jets from W", 25, 0, 5}, "chi2_wqq_dR", "evWeight","0000");
        
        add1DHist( {"hchi2_lfvjmu_dEta", "dEta of lfv jet and muon", 25, -5, 5}, "chi2_lfvjmu_dEta", "evWeight","0000");
        add1DHist( {"hchi2_lfvjmu_absdEta", "abs dEta of lfv jet and muon", 25, 0, 5}, "chi2_lfvjmu_absdEta", "evWeight","0000");
        add1DHist( {"hchi2_lfvjmu_dPhi", "dPhi of lfv jet and muon", 20, -4, 4}, "chi2_lfvjmu_dPhi", "evWeight","0000");
        add1DHist( {"hchi2_lfvjmu_absdPhi", "absdPhi of lfv jet and muon", 20, 0, 4}, "chi2_lfvjmu_absdPhi", "evWeight","0000");
        add1DHist( {"hchi2_lfvjmu_dR", "dR of lfv jet and muon", 25, 0, 5}, "chi2_lfvjmu_dR", "evWeight","0000");
        add1DHist( {"hchi2_lfvjmu_mass", "Mass of lfv jet and muon", 25, 0, 500}, "chi2_lfvjmu_mass", "evWeight","0000");
        
        add1DHist( {"hchi2_lfvjtau_dEta", "dEta of lfv jet and tau", 25, -5, 5}, "chi2_lfvjtau_dEta", "evWeight","0000");
        add1DHist( {"hchi2_lfvjtau_absdEta", "abs dEta of lfv jet and tau", 25, 0, 5}, "chi2_lfvjtau_absdEta", "evWeight","0000");
        add1DHist( {"hchi2_lfvjtau_dPhi", "dPhi of lfv jet and tau", 20, -4, 4}, "chi2_lfvjtau_dPhi", "evWeight","0000");
        add1DHist( {"hchi2_lfvjtau_absdPhi", "absdPhi of lfv jet and tau", 20, 0, 4}, "chi2_lfvjtau_absdPhi", "evWeight","0000");
        add1DHist( {"hchi2_lfvjtau_dR", "dR of lfv jet and tau", 25, 0, 5}, "chi2_lfvjtau_dR", "evWeight","0000");
        add1DHist( {"hchi2_lfvjtau_mass", "Mass of lfv jet and tau", 25, 0, 500}, "chi2_lfvjtau_mass", "evWeight","0000");
        
        add1DHist( {"hchi2_mutau_dEta", "dEta of muon and tau", 25, -5, 5}, "chi2_mutau_dEta", "evWeight","0000");
        add1DHist( {"hchi2_mutau_absdEta", "abs dEta of muon and tau", 25, 0, 5}, "chi2_mutau_absdEta", "evWeight","0000");
        add1DHist( {"hchi2_mutau_dPhi", "dPhi of muon and tau", 20, -4, 4}, "chi2_mutau_dPhi", "evWeight","0000");
        add1DHist( {"hchi2_mutau_absdPhi", "absdPhi of muon and tau", 20, 0, 4}, "chi2_mutau_absdPhi", "evWeight","0000");
        add1DHist( {"hchi2_mutau_dR", "dR of muon and tau", 25, 0, 5}, "chi2_mutau_dR", "evWeight","0000");
        add1DHist( {"hchi2_mutau_mass", "Mass of muon and tau", 25, 0, 500}, "chi2_mutau_mass", "evWeight","0000");
        
        add1DHist( {"hchi2_lfvjmutau_dEta", "dEta of lfv jet and mutau", 25, -5, 5}, "chi2_lfvjmutau_dEta", "evWeight","0000");
        add1DHist( {"hchi2_lfvjmutau_absdEta", "abs dEta of lfv jet and mutau", 25, 0, 5}, "chi2_lfvjmutau_absdEta", "evWeight","0000");
        add1DHist( {"hchi2_lfvjmutau_dPhi", "dPhi of lfv jet and mutau", 20, -4, 4}, "chi2_lfvjmutau_dPhi", "evWeight","0000");
        add1DHist( {"hchi2_lfvjmutau_absdPhi", "absdPhi of lfv jet and mutau", 20, 0, 4}, "chi2_lfvjmutau_absdPhi", "evWeight","0000");
        add1DHist( {"hchi2_lfvjmutau_dR", "dR of lfv jet and mutau", 25, 0, 5}, "chi2_lfvjmutau_dR", "evWeight","0000");
}
