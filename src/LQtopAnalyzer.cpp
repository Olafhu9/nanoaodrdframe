/*
 * LQtopAnalyzer.cpp
 *
 *  Created on: April 9, 2020
 *      Author: Tae Jeong Kim
 */

#include "LQtopAnalyzer.h"
#include "utility.h"

LQtopAnalyzer::LQtopAnalyzer(TTree *t, std::string outfilename, std::string year, std::string syst, std::string jsonfname, string globaltag, int nthreads)
:NanoAODAnalyzerrdframe(t, outfilename, year, syst, jsonfname, globaltag, nthreads)
{

}

// Define your cuts here
void LQtopAnalyzer::defineCuts()
{
	// Cuts to be applied in order
	// These will be passed to Filter method of RDF
	// check for good json event is defined earlier
        addCuts("nmuonpass == 1 && nvetoelepass == 0 && nvetomuons == 0","0");
        addCuts("ncleantaupass == 1", "00");
        addCuts("mutau_mass > 150 && mutau_charge < 0", "000");
        addCuts("ncleanjetspass >= 3", "0000");
        // Signal Region
        addCuts("ncleanbjetspass == 1", "00000");
        // Control Region
        //addCuts("ncleanbjetspass > 1", "00000");
}
/*
template <typename T>
void LQtopAnalyzer::defineVar(std::string varname, T function,  const RDFDetail::ColumnNames_t &columns)
{
	NanoAODAnalyzerrdframe::defineVar(varname, function, columns);
}
*/

void LQtopAnalyzer::defineMoreVars()
{
	// define your higher level variables here
        defineVar("muonvec",::select_leadingvec,{"muon4vecs"});
        defineVar("tauvec",::select_leadingvec,{"cleantau4vecs"});
        defineVar("mutau_mass",::calculate_invMass,{"muonvec","tauvec"});
        defineVar("mutau_dEta",::calculate_deltaEta,{"muonvec","tauvec"});
        defineVar("mutau_dPhi",::calculate_deltaPhi,{"muonvec","tauvec"});
        defineVar("mutau_dR",::calculate_deltaR,{"muonvec","tauvec"});
        defineVar("muMET_mt",::calculate_MT,{"muon4vecs","Sys_METpt","Sys_METphi"});

        addVar({"Sel_muon1pt", "Sel_muonpt[0]", ""});
        addVar({"Sel_muon1eta", "Sel_muoneta[0]", ""});
        addVar({"Sel_muon1mass", "Sel_muonmass[0]", ""});
        addVar({"Sel_muon1charge", "Sel_muoncharge[0]", ""});
	
        addVar({"Sel_tau1pt", "Sel_taupt[0]", ""});
        addVar({"Sel_tau1eta", "Sel_taueta[0]", ""});
        addVar({"Sel_tau1mass", "Sel_taumass[0]", ""});
        addVar({"Sel_tau1charge", "Sel_taucharge[0]", ""});

        addVar({"mutau_charge", "Sel_muon1charge * Sel_tau1charge", ""});

        addVar({"Sel2_jet1pt", "(Sel2_jetpt.size()>0) ? Sel2_jetpt[0] : -1", ""});
        addVar({"Sel2_jet1eta", "Sel2_jeteta[0]", ""});
        addVar({"Sel2_jet1mass", "Sel2_jetmass[0]", ""});

        addVar({"Sel2_jet2pt", "Sel2_jetpt[1]", ""});
        addVar({"Sel2_jet2eta", "Sel2_jeteta[1]", ""});
        addVar({"Sel2_jet2mass", "Sel2_jetmass[1]", ""});

        addVar({"Sel2_jet3pt", "Sel2_jetpt[2]", ""});
        addVar({"Sel2_jet3eta", "Sel2_jeteta[2]", ""});
        addVar({"Sel2_jet3mass", "Sel2_jetmass[2]", ""});

	//For the TT analysis
        addVar({"Sel2_jet4pt", "Sel2_jetpt[3]", ""});
        addVar({"Sel2_jet4eta", "Sel2_jeteta[3]", ""});
        addVar({"Sel2_jet4mass", "Sel2_jetmass[3]", ""});


        addVar({"Sel2_jet1btag","Sel2_jetbtag[0]",""});
        addVar({"Sel2_jet2btag","Sel2_jetbtag[1]",""});
        addVar({"Sel2_jet3btag","Sel2_jetbtag[2]",""});
	addVar({"Sel2_jet4btag","Sel2_jetbtag[3]",""}); //For the TT analysis

        addVar({"Sel2_bjet1pt", "Sel2_bjetpt[0]", ""});
        addVar({"Sel2_bjet1eta", "Sel2_bjeteta[0]", ""});
        addVar({"Sel2_bjet1mass", "Sel2_bjetmass[0]", ""});

        defineVar("top_reco_whad", ::top_reconstruction_STLFV, {"cleanjet4vecs","cleanbjet4vecs","muon4vecs","cleantau4vecs"});
        addVar({"chi2","top_reco_whad[0]",""});
        addVar({"chi2_SMW_mass","top_reco_whad[1]",""});
        addVar({"chi2_SMTop_mass","top_reco_whad[2]",""});
        addVar({"chi2_wjet1_idx","top_reco_whad[3]",""});
        addVar({"chi2_wjet2_idx","top_reco_whad[4]",""});
        addVar({"chi2_SMW","top_reco_whad[5]",""});
        addVar({"chi2_SMTop","top_reco_whad[6]",""});

        defineVar("top_reco_prod", ::top_reco_products_STLFV, {"cleanjet4vecs","muon4vecs","cleantau4vecs","top_reco_whad"});
        addVar({"chi2_wqq_dEta","top_reco_prod[0]",""});
        addVar({"chi2_wqq_dPhi","top_reco_prod[1]",""});
        addVar({"chi2_wqq_dR","top_reco_prod[2]",""});

        //addVar({"Jet_rawpt","Jet_pt*(1-Jet_rawFactor)"});

        addVar({"evWeight_pglep","re_pugenWeight * evWeight_leptonSF"});
        addVar({"evWeight", "re_pugenWeight * btagWeight_DeepFlavBrecalc * evWeight_leptonSF"});

        // define variables that you want to store
	addVartoStore("run");
	addVartoStore("luminosityBlock");
	addVartoStore("event");
	addVartoStore("evWeight.*");
        addVartoStore("re_.*");
        addVartoStore("nmuonpass");
	addVartoStore("ncleanjetspass");
	addVartoStore("ncleanbjetspass");
        addVartoStore("ncleantaupass");
        addVartoStore("Sys.*");
        addVartoStore("Sel_muon1.*");
        addVartoStore("Sel2_jet1.*");
        addVartoStore("Sel2_jet2.*");
        addVartoStore("Sel2_jet3.*");
        addVartoStore("Sel2_bjet1.*");
        addVartoStore("Sel_tau1.*");
        addVartoStore("mutau.*");
        addVartoStore("chi2.*");
        addVartoStore("btagWeight_DeepFlavBrecalc.*");
}

void LQtopAnalyzer::bookHists()
{
	// _hist1dinfovector contains the information of histogram definitions (as TH1DModel)
	// the variable to be used for filling
	// and the minimum cutstep for which the histogram should be filled
	//
	// The braces are used to initalize the struct
        // Pre-set binnings for histograms
	// TH1D
        add1DHist( {"hnevents", "Number of Events", 2, -0.5, 1.5}, "one", "evWeight", "");
        add1DHist( {"hnevents_pglep", "Number of Events", 2, -0.5, 1.5}, "one", "evWeight_pglep", "");
	//add1DHist( {"hnvtx_raw", "Number of Primary Vertex", 200, 0.0, 200.0}, "PV_npvsGood", "one", "");
	//add1DHist( {"hnvtx", "Number of Primary Vertex", 200, 0.0, 200.0}, "PV_npvsGood", "evWeight", "");

        //=================== PUGEN * Lepton SF ===================
	add1DHist( {"h1metpt", "MET pt", 20, 0, 400}, "Sys_METpt", "evWeight_pglep", "0");
	add1DHist( {"h1sumet", "Sum ET", 50, 0.0, 5000.0}, "MET_sumEt", "evWeight_pglep", "0");
	add1DHist( {"h1metphi", "MET phi", 20, -4.0, 4.0}, "Sys_METphi", "evWeight_pglep", "0");

//        add1DHist( {"h1npvdof", "Number of PV of DoF", 50, 0.0, 50.0}, "PV_ndof", "evWeight_pglep", "0");
//        add1DHist( {"h1npvs", "Number of PVs", 100, 0.0, 100.0}, "PV_npvs", "evWeight_pglep", "0");
//        add1DHist( {"h1npvsgood", "Number of good PVs", 100, 0.0, 100.0}, "PV_npvsGood", "evWeight_pglep", "0");

	add1DHist( {"h1nmuonpass", "Passing muoncuts", 5, 0.0, 5.0}, "nmuonpass", "evWeight_pglep", "0");
	add1DHist( {"h1ncleantaupass", "Passing taucuts", 5, 0.0, 5.0}, "ncleantaupass", "evWeight_pglep", "0");
	add1DHist( {"h1ncleanjetspass", "Passing jetcuts", 10, 0.0, 10.0}, "ncleanjetspass", "evWeight_pglep", "0");
        add1DHist( {"h1ncleanbjetspass", "Passing bjetcuts", 5, 0.0, 5.0}, "ncleanbjetspass", "evWeight_pglep", "0");

        add1DHist( {"h1muon1pt", "Muon pt", 20, 0, 400}, "Sel_muon1pt", "evWeight_pglep", "0");
        add1DHist( {"h1muon1eta", "Muon eta", 18, -2.7, 2.7}, "Sel_muon1eta", "evWeight_pglep", "0");
        add1DHist( {"h1muon1mass", "Muon mass", 20, 0, 100}, "Sel_muon1mass", "evWeight_pglep", "0");
        add1DHist( {"h1muMETmt", "Muon met mt", 20, 0, 200}, "muMET_mt", "evWeight_pglep", "0");
    
        add1DHist( {"h1tau1pt", "Tau pt", 20, 0, 400}, "Sel_tau1pt", "evWeight_pglep", "00");
        add1DHist( {"h1tau1eta", "Tau eta", 18, -2.7, 2.7}, "Sel_tau1eta", "evWeight_pglep", "00");
        add1DHist( {"h1tau1mass", "Tau mass", 20, 0, 100}, "Sel_tau1mass", "evWeight_pglep", "00");

        add1DHist( {"h1mutau_dEta", "dEta of muon and tau", 25, -5, 5}, "mutau_dEta", "evWeight_pglep","00");
        add1DHist( {"h1mutau_dPhi", "dPhi of muon and tau", 20, -4, 4}, "mutau_dPhi", "evWeight_pglep","00");
        add1DHist( {"h1mutau_dR", "dR of muon and tau", 20, 0, 4.0}, "mutau_dR", "evWeight_pglep","00");
        add1DHist( {"h1mutau_mass", "Mass of muon and tau", 30, 0, 600}, "mutau_mass", "evWeight_pglep","00");

        add1DHist( {"h1jet1pt", "leading jet pt", 20, 0, 400}, "Sel2_jet1pt", "evWeight_pglep", "0000");
        add1DHist( {"h1jet2pt", "sub-leading jet pt", 20, 0, 400}, "Sel2_jet2pt", "evWeight_pglep", "0000");
        add1DHist( {"h1jet3pt", "third jet pt", 20, 0, 400}, "Sel2_jet3pt", "evWeight_pglep", "0000");

        add1DHist( {"h1jet1eta", "leading jet eta", 18, -2.7, 2.7}, "Sel2_jet1eta", "evWeight_pglep", "0000");
        add1DHist( {"h1jet2eta", "sub-leading jet eta", 18, -2.7, 2.7}, "Sel2_jet2eta", "evWeight_pglep", "0000");
        add1DHist( {"h1jet3eta", "third jet eta", 18, -2.7, 2.7}, "Sel2_jet3eta", "evWeight_pglep", "0000");

        add1DHist( {"h1jet1mass", "leading jet mass", 20, 0, 100}, "Sel2_jet1mass", "evWeight_pglep", "0000");
        add1DHist( {"h1jet2mass", "sub-leading jet mass", 20, 0, 100}, "Sel2_jet2mass", "evWeight_pglep", "0000");
        add1DHist( {"h1jet3mass", "third jet mass", 20, 0, 100}, "Sel2_jet3mass", "evWeight_pglep", "0000");

        add1DHist( {"h1bjet1pt", "b jet pt", 20, 0, 400}, "Sel2_bjet1pt", "evWeight_pglep", "0000");
        add1DHist( {"h1bjet1eta", "b jet eta", 18, -2.7, 2.7}, "Sel2_bjet1eta", "evWeight_pglep", "0000");
        add1DHist( {"h1bjet1mass", "b jet mass", 20, 0, 100}, "Sel2_bjet1mass", "evWeight_pglep", "0000");

        add1DHist( {"h1jet1btag","btag discr of leading jet", 20, 0, 1.0}, "Sel2_jet1btag", "evWeight_pglep", "0000");
        add1DHist( {"h1jet2btag","btag discr of sub-leading jet", 20, 0, 1.0}, "Sel2_jet2btag", "evWeight_pglep", "0000");
        add1DHist( {"h1jet3btag","btag discr of third jet", 20, 0, 1.0}, "Sel2_jet3btag", "evWeight_pglep", "0000");

        //=================== Fully weighted ===================
	add1DHist( {"hmetpt", "MET pt", 20, 0, 400}, "Sys_METpt", "evWeight", "00000");
	add1DHist( {"hsumet", "Sum ET", 50, 0.0, 5000.0}, "MET_sumEt", "evWeight", "00000");
	add1DHist( {"hmetphi", "MET phi", 20, -4.0, 4.0}, "Sys_METphi", "evWeight", "00000");

	add1DHist( {"hnmuonpass", "Passing muoncuts", 5, 0.0, 5.0}, "nmuonpass", "evWeight", "00000");
	add1DHist( {"hncleantaupass", "Passing taucuts", 5, 0.0, 5.0}, "ncleantaupass", "evWeight", "00000");
	add1DHist( {"hncleanjetspass", "Passing jetcuts", 10, 0.0, 10.0}, "ncleanjetspass", "evWeight", "00000");
        add1DHist( {"hncleanbjetspass", "Passing bjetcuts", 5, 0.0, 5.0}, "ncleanbjetspass", "evWeight", "0000");

        add1DHist( {"hmuon1pt", "Muon pt", 20, 0, 400}, "Sel_muon1pt", "evWeight", "00000");
        add1DHist( {"hmuon1eta", "Muon eta", 18, -2.7, 2.7}, "Sel_muon1eta", "evWeight", "00000");
        add1DHist( {"hmuon1mass", "Muon mass", 20, 0, 100}, "Sel_muon1mass", "evWeight", "00000");
        add1DHist( {"hmuMETmt", "Muon met mt", 20, 0, 200}, "muMET_mt", "evWeight", "00000");
    
        add1DHist( {"htau1pt", "Tau pt", 20, 0, 400}, "Sel_tau1pt", "evWeight", "00000");
        add1DHist( {"htau1eta", "Tau eta", 18, -2.7, 2.7}, "Sel_tau1eta", "evWeight", "00000");
        add1DHist( {"htau1mass", "Tau mass", 20, 0, 100}, "Sel_tau1mass", "evWeight", "00000");

        add1DHist( {"hmutau_dEta", "dEta of muon and tau", 25, -5, 5}, "mutau_dEta", "evWeight","00000");
        add1DHist( {"hmutau_dPhi", "dPhi of muon and tau", 20, -4, 4}, "mutau_dPhi", "evWeight","00000");
        add1DHist( {"hmutau_dR", "dR of muon and tau", 20, 0, 4.0}, "mutau_dR", "evWeight","00000");
        add1DHist( {"hmutau_mass", "Mass of muon and tau", 30, 0, 600}, "mutau_mass", "evWeight","00000");

        add1DHist( {"hjet1pt", "leading jet pt", 20, 0, 400}, "Sel2_jet1pt", "evWeight", "00000");
        add1DHist( {"hjet2pt", "sub-leading jet pt", 20, 0, 400}, "Sel2_jet2pt", "evWeight", "00000");
        add1DHist( {"hjet3pt", "third jet pt", 20, 0, 400}, "Sel2_jet3pt", "evWeight", "00000");

        add1DHist( {"hjet1eta", "leading jet eta", 18, -2.7, 2.7}, "Sel2_jet1eta", "evWeight", "00000");
        add1DHist( {"hjet2eta", "sub-leading jet eta", 18, -2.7, 2.7}, "Sel2_jet2eta", "evWeight", "00000");
        add1DHist( {"hjet3eta", "third jet eta", 18, -2.7, 2.7}, "Sel2_jet3eta", "evWeight", "00000");

        add1DHist( {"hjet1mass", "leading jet mass", 20, 0, 100}, "Sel2_jet1mass", "evWeight", "00000");
        add1DHist( {"hjet2mass", "sub-leading jet mass", 20, 0, 100}, "Sel2_jet2mass", "evWeight", "00000");
        add1DHist( {"hjet3mass", "third jet mass", 20, 0, 100}, "Sel2_jet3mass", "evWeight", "00000");

        add1DHist( {"hbjet1pt", "b jet pt", 20, 0, 400}, "Sel2_bjet1pt", "evWeight", "00000");
        add1DHist( {"hbjet1eta", "b jet eta", 18, -2.7, 2.7}, "Sel2_bjet1eta", "evWeight", "00000");
        add1DHist( {"hbjet1mass", "b jet mass", 20, 0, 100}, "Sel2_bjet1mass", "evWeight", "00000");

        add1DHist( {"hjet1btag","btag discr of leading jet", 20, 0, 1.0}, "Sel2_jet1btag", "evWeight", "00000");
        add1DHist( {"hjet2btag","btag discr of sub-leading jet", 20, 0, 1.0}, "Sel2_jet2btag", "evWeight", "00000");
        add1DHist( {"hjet3btag","btag discr of third jet", 20, 0, 1.0}, "Sel2_jet3btag", "evWeight", "00000");
        
        add1DHist( {"hchi2", "Minimum chi2 for hadronic W", 20, 0, 1000}, "chi2", "evWeight","00000");
        add1DHist( {"hchi2_SMTop_mass", "chi2 SM Top mass", 20, 0, 400}, "chi2_SMTop_mass", "evWeight","00000");
        add1DHist( {"hchi2_SMW_mass", "chi2 SM W mass", 20, 0, 400}, "chi2_SMW_mass", "evWeight","00000");

        add1DHist( {"hchi2_wqq_dEta", "dEta of jets from W", 25, -5, 5}, "chi2_wqq_dEta", "evWeight","00000");
        add1DHist( {"hchi2_wqq_dPhi", "dPhi of jets from W", 20, -4, 4}, "chi2_wqq_dPhi", "evWeight","00000");
        add1DHist( {"hchi2_wqq_dR", "dR of jets from W", 20, 0, 4.0}, "chi2_wqq_dR", "evWeight","00000");
}
