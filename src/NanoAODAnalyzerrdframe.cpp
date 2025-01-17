/*
 * NanoAODAnalyzerrdframe.cpp
 *
 *  Created on: Sep 30, 2018
 *      Author: suyong
 *      Refactored for LFV analysis: jiwon park
 */

#include "NanoAODAnalyzerrdframe.h"
#include <iostream>
#include <algorithm>
#include <typeinfo>
#include <random>

#include "TCanvas.h"
#include "Math/GenVector/VectorUtil.h"
#include <vector>
#include <fstream>
#include "utility.h"
#include <regex>
#include "ROOT/RDFHelpers.hxx"
#include "correction.h"

using namespace std;

NanoAODAnalyzerrdframe::NanoAODAnalyzerrdframe(TTree *atree, std::string outfilename, std::string year, std::string syst, std::string jsonfname, std::string globaltag, int nthreads)
:_rd(*atree), _isData(false), _jsonOK(false), _outfilename(outfilename), _year(year), _syst(syst), _jsonfname(jsonfname), _globaltag(globaltag), _inrootfile(0), _outrootfile(0), _rlm(_rd), _rnt(&_rlm), currentnode(0), PDFWeights(103, 0.0) {

    // Skim switch
    if (_isSkim == true) {
        cout << "<< Start Skim NanoAOD >>" << endl;
    } else {
        cout << "<< Start Process NanoAOD >>" << endl;
    }

    // Year switch
    if (_year.find("2016pre") != std::string::npos) {
        _isRun16pre = true;
        cout << "Year : Run 2016 pre (16 APV)" << endl;
    } else if (_year.find("2016post") != std::string::npos) {
        _isRun16post = true;
        cout << "Year : Run 2016 post" << endl;
    } else if (_year.find("2017") != std::string::npos) {
        _isRun17 = true;
        cout << "Year : Run 2017" << endl;
    } else if (_year.find("2018") != std::string::npos) {
        _isRun18 = true;
        cout << "Year : Run 2018" << endl;
    }
    _isRun16 = _isRun16pre || _isRun16post;

    // Data/mc switch
    if (atree->GetBranch("genWeight") == nullptr) {
        _isData = true;
        cout << "Input file is data" <<endl;
    } else {
        _isData = false;
        cout << "Input file is MC" <<endl;
    }

    // Systematic switch
    if (_syst != "") {
        cout << "Systematics Information" << endl;
        cout << "Activated option --syst " << _syst << endl;
    } else {
        cout << "Nominal process without systematics" << endl;
    }

    TObjArray *allbranches = atree->GetListOfBranches();
    for (int i =0; i<allbranches->GetSize(); i++) {
        TBranch *abranch = dynamic_cast<TBranch *>(allbranches->At(i));
        if (abranch!= nullptr) {
            std::string brname = abranch->GetName();
            if (brname.find("HLT_") == std::string::npos and brname.find("L1_") == std::string::npos)
                cout << brname << ", ";
            _originalvars.push_back(abranch->GetName());
        }
    }
    cout << endl;
}

NanoAODAnalyzerrdframe::~NanoAODAnalyzerrdframe() {

    // TODO Auto-generated destructor stub
    // ugly...
    std::cout<<">>  Job Done  <<"<<std::endl;
}

bool NanoAODAnalyzerrdframe::isDefined(string v) {

	auto result = std::find(_originalvars.begin(), _originalvars.end(), v);
	if (result != _originalvars.end()) return true;
	else return false;
}

void NanoAODAnalyzerrdframe::setTree(TTree *t, std::string outfilename) {

	_rd = ROOT::RDataFrame(*t);
	_rlm = RNode(_rd);
	_outfilename = outfilename;
	_hist1dinfovector.clear();
	_th1dhistos.clear();
	_varstostore.clear();
	_selections.clear();

	this->setupAnalysis();
}

void NanoAODAnalyzerrdframe::setupAnalysis() {

    if (_isData) _jsonOK = readjson();

    //_rlm = _rlm.Filter("event < 12534199");

    _rlm = _rlm.Define("one", "1.0");
    // Event weight for data it's always one. For MC, it depends on the sign
    if(_isSkim){
        _rlm = _rlm.Define("unitGenWeight", "one");

        if(!_isData){

            // Store PDF sum of weights
            auto storeWeights = [this](floats weights)->floats {

                for (unsigned int i=0; i<weights.size(); i++)
                    PDFWeights[i] += weights[i];

                return PDFWeights;
            };
            try {
                _rlm.Foreach(storeWeights, {"LHEPdfWeight"});
            } catch (exception& e) {
                cout << e.what() << endl;
                cout << "No PDF weight in this root file!" << endl;
            }

            // pu weight setup
            cout<<"Loading Pileup profiles"<<endl;
            // MC 2016pre = MC 2016post (same file)
            TFile tfmc(("data/Pileup/PileupMC_UL" + _year + ".root").c_str());
            TH1D* _hpumc = dynamic_cast<TH1D *>(tfmc.Get("pu_mc"));
            _hpumc->SetDirectory(0);
            tfmc.Close();

            TFile tfdata(("data/Pileup/PileupDATA_UL" + _year + ".root").c_str());
            TH1D* _hpudata = dynamic_cast<TH1D *>(tfdata.Get("pileup"));
            TH1D* _hpudata_plus = dynamic_cast<TH1D *>(tfdata.Get("pileup_plus"));
            TH1D* _hpudata_minus = dynamic_cast<TH1D *>(tfdata.Get("pileup_minus"));

            _hpudata->SetDirectory(0);
            _hpudata_plus->SetDirectory(0);
            _hpudata_minus->SetDirectory(0);
            tfdata.Close();

            WeightCalculatorFromHistogram* _puweightcalc = new WeightCalculatorFromHistogram(_hpumc, _hpudata);
            WeightCalculatorFromHistogram* _puweightcalc_plus = new WeightCalculatorFromHistogram(_hpumc, _hpudata_plus);
            WeightCalculatorFromHistogram* _puweightcalc_minus = new WeightCalculatorFromHistogram(_hpumc, _hpudata_minus);

            _rlm = _rlm.Redefine("unitGenWeight","genWeight != 0 ? genWeight/abs(genWeight) : 0")
                       .Define("puWeight", [this, _puweightcalc, _puweightcalc_plus, _puweightcalc_minus](float x) ->floats
                              {return {_puweightcalc->getWeight(x), _puweightcalc_plus->getWeight(x), _puweightcalc_minus->getWeight(x)};}, {"Pileup_nTrueInt"});
        }
    }

    std::vector<std::string> jes_var;
    if (_isRun16)
        jes_var = jes_var_2016;
    else if (_isRun17)
        jes_var = jes_var_2017;
    else if (_isRun18)
        jes_var = jes_var_2018;
    else
        cout << "WARNING !!!!!! : No JES - BTAG SF found" << endl;

    // Object selection will be defined in sequence.
    // Selected objects will be stored in new vectors.
    if (_isSkim) {
        selectMuons();
        selectElectrons();
		selectLeptons();
        setupJetMETCorrection(_globaltag, jes_var, "AK4PFchs", _isData);
        skimJets();
        if (!_isData){
            calculateEvWeight();
            applyBSFs(jes_var);
        }
    } else {
        //selectTaus();
        selectJets(jes_var);
        if (!_isData){
            topPtReweight();
        }
    }
    defineMoreVars();
    defineCuts();
    bookHists();
    setupCuts_and_Hists();
    setupTree();
}

bool NanoAODAnalyzerrdframe::readjson() {

    auto isgoodjsonevent = [this](unsigned int runnumber, unsigned int lumisection) {

        auto key = std::to_string(runnumber);
        bool goodeventflag = false;

        if (jsonroot.isMember(key)) {
            Json::Value runlumiblocks = jsonroot[key];
            for (unsigned int i=0; i<runlumiblocks.size() && !goodeventflag; i++) {
                auto lumirange = runlumiblocks[i];
                if (lumisection >= lumirange[0].asUInt() && lumisection <= lumirange[1].asUInt()) goodeventflag = true;
            }
            return goodeventflag;
        } else {
            //cout << "Run not in json " << runnumber << endl;
            return false;
        }

    };

    if (_jsonfname != "") {
        std::ifstream jsoninfile;
        jsoninfile.open(_jsonfname);

        if (jsoninfile.good()) {
            jsoninfile >> jsonroot;
            /*
            auto runlumiblocks =  jsonroot["276775"];
            for (auto i=0; i<runlumiblocks.size(); i++)
            {
            auto lumirange = runlumiblocks[i];
            cout << "lumi range " << lumirange[0] << " " << lumirange[1] << endl;
            }
            */
            _rlm = _rlm.Define("goodjsonevent", isgoodjsonevent, {"run", "luminosityBlock"}).Filter("goodjsonevent");
            _jsonOK = true;
            return true;
        } else {
          cout << "Problem reading json file " << _jsonfname << endl;
          return false;
          }
        } else {
            cout << "no JSON file given" << endl;
        return true;
    }
}

void NanoAODAnalyzerrdframe::selectElectrons()
{
    //cout << "select electrons" << endl;
    // Run II recommendation: https://twiki.cern.ch/twiki/bin/viewauth/CMS/EgammaRunIIRecommendations
    // Run II recomendation - cutbased: https://twiki.cern.ch/twiki/bin/view/CMS/CutBasedElectronIdentificationRun2
    // Temporary elecuts (Not to be used!)
	
	// different year different cuts
	auto selectEleCuts = [this, _year]() {
		
		std::string pteta = "( ( Electron_pt > 34.0 && abs(Electron_eta) < 2.5 ) || ( Electron_pt > 30.0 && abs(Electron_eta) < 2.1 ) )";
		if (_year.find("2016")) { pteta = "( Electron_pt > 29.0 && abs(Electron_eta) < 2.5 )"; }

		std::string etasc = "( ( abs(Electron_eta + Electron_deltaEtaSC) < 1.4442 ) || abs(Electron_eta + Electron_deltaEtaSC) > 1.566 )"; 
		std::string id = "Electron_cutBased == 4";
		std::string ip = "( ( (abs(Electron_eta + Electron_deltaEtaSC) <= 1.479) && abs(Electron_dxy) < 0.05 && abs(Electron_dz) < 0.10 ) || ( (abs(Electron_eta + Electron_deltaEtaSC)>1.479) && abs(Electron_dxy) < 0.10 && abs(Electron_dz) < 0.20) )";

		std::string elecuts = pteta + " && " + etasc + " && " + id + " && " + ip
		return elecuts;
	}

    //_rlm = _rlm.Define("elecuts", "( ( Electron_pt > 34.0 && abs(Electron_eta) < 2.4 ) || ( Electron_pt > 30.0 && abs(Electron_eta) < 2.1 ) ) && ( ( abs(Electron_eta + Electron_deltaEtaSC) < 1.4442 ) || abs(Electron_eta + Electron_deltaEtaSC) > 1.566 ) && Electron_cutBased == 4 && ( ( (abs(Electron_eta + Electron_deltaEtaSC) <= 1.479) && abs(Electron_dxy) < 0.05 && abs(Electron_dz) < 0.10 ) || ( (abs(Electron_eta + Electron_deltaEtaSC)>1.479) && abs(Electron_dxy) < 0.10 && abs(Electron_dz) < 0.20) )");
	_rlm = _rlm.Define("elecuts", selectEleCuts) 
               .Define("vetoelecuts", "Electron_pt > 15.0 && abs(Electron_eta) < 2.5 && Electron_cutBased == 1"); 
               .Define("nvetoeles","Sum(vetoelecuts)");
               .Redefine("Electron_pt", "Electron_pt[elecuts]") // define new variables
               .Redefine("Electron_eta", "Electron_eta[elecuts]")
               .Redefine("Electron_phi", "Electron_phi[elecuts]")
               .Redefine("Electron_mass", "Electron_mass[elecuts]")
               .Define("Sel_eleidx", ::good_idx, {"elecuts"})
               .Define("nelepass", "int(Electron_pt.size())")
        	   .Define("ele4vecs", ::gen4vec, {"Electron_pt", "Electron_eta", "Electron_phi", "Electron_mass"});

	// Electron SF
	cout << "Loading Electron SF" << endl;
	std::string eleYear = _year + "_UL";
	std::string eleid_ = "Tight";
	std::string elereco_ = "RecoAbove20";

	
	cout << "Electron ID WP" << endl;
	cout << eleid_ << endl;
	cout << elereco_ << endl;

	auto eleFile = correction::CorrectionSet::from_file("/home/juhee5819/cheer/nanoaodrdframe/data/ElectronSF/"+eleYear+"electron.json.gz");
	auto _eleidSF = eleFile->at("UL-Electron-ID-SF");
	//auto sf = Map->evaluate({eleYear, "sf", eleID})

	auto eleSFId = [this, _eleidSF, _year, eleid_](floats &pt, floats &eta)->floats {

		floats wVec;
		wVec.reserve(3); //cent, up, down

		if (pt.size() == 1) {
			for (unsigned int i=0; i<pt.size(); i++) {
				wVec.emplace_back(_eleidSF->evaluate({_year, "sf", eleid_, eta[i], pt[i]}));
				wVec.emplace_back(_eleidSF->evaluate({_year, "sfup", eleid_, eta[i], pt[i]}));
				wVec.emplace_back(_eleidSF->evaluate({_year, "sfdown", eleid_, eta[i], pt[i]}));
			}
		}
		else wVec = {1.0, 1.0, 1.0};
		return wVec;
	};

	auto eleSFReco = [this, _eleidSF, _year, elereco_](floats &pt, floats &eta)->floats {

		floats wVec;
		wVec.reserve(3); //cent, up, down

		if (pt.size() == 1) {
			for (unsigned int i=0; i<pt.size(); i++) {
				wVec.emplace_back(_eleidSF->evaluate({_year, "sf", elereco_, eta[i], pt[i]}));
				wVec.emplace_back(_eleidSF->evaluate({_year, "sfup", elereco_, eta[i], pt[i]}));
				wVec.emplace_back(_eleidSF->evaluate({_year, "sfdown", elereco_, eta[i], pt[i]}));
			}
		}
		else wVec = {1.0, 1.0, 1.0};
		return wVec;
	};


    _rlm = _rlm.Define("electronWeightId", eleSFId, {"Electron_pt","Electron_eta"})
               .Define("electronWeightIso", eleSFReco, {"Electron_pt","Electron_eta"})
               //.Define("electronWeightTrg", eleSFTrg, {"Electron_pt","Electron_eta"});

}

void NanoAODAnalyzerrdframe::selectMuons() {

	auto selectMuonCuts = [this, _year]() {
		
		std::string pt = "Muon_pt>26";
		if (_year.find("2017")) { pt = "Muon_pt>29"; }

		std::string common = "abs(Muon_eta)<2.4 && Muon_tightId && Muon_pfRelIso04_all<0.15"

		std::string muoncuts = pt + " && " + common;
		return muoncuts;
	}

    //_rlm = _rlm.Define("muoncuts", "Muon_pt>50.0 && abs(Muon_eta)<2.4 && Muon_tightId && Muon_pfRelIso04_all<0.15")
	_rlm = _rlm.Define("muoncuts", selectMuonCuts)
               .Define("vetomuoncuts", "!muoncuts && Muon_pt>15.0 && abs(Muon_eta)<2.4 && Muon_looseId && Muon_pfRelIso04_all<0.25")
               .Define("nvetomuons","Sum(vetomuoncuts)")
               .Redefine("Muon_pt", "Muon_pt[muoncuts]")
               .Redefine("Muon_eta", "Muon_eta[muoncuts]")
               .Redefine("Muon_phi", "Muon_phi[muoncuts]")
               .Redefine("Muon_mass", "Muon_mass[muoncuts]")
               //.Redefine("Muon_charge", "Muon_charge[muoncuts]")
               //.Redefine("Muon_looseId", "Muon_looseId[muoncuts]")
               //.Redefine("Muon_pfRelIso04_all", "Muon_pfRelIso04_all[muoncuts]")
               .Define("Sel_muonidx", ::good_idx, {"muoncuts"})
               .Define("nmuonpass", "int(Muon_pt.size())")
               .Define("muon4vecs", ::gen4vec, {"Muon_pt", "Muon_eta", "Muon_phi", "Muon_mass"});


    // Muon SF
    cout<<"Loading Muon SF"<<endl;
    std::string muonFile = _year + "_UL";
    std::string muonTrgHist = "";
    TFile *muontrg;
    TFile *muonid;
    TFile *muoniso;

    if (_isRun16pre) {
        muonFile = "2016_UL_HIPM";
        muonTrgHist = "NUM_IsoMu24_or_IsoTkMu24_DEN_CutBasedIdTight_and_PFIsoTight_abseta_pt";
    } else if (_isRun16post) {
        muonFile = "2016_UL";
        muonTrgHist = "NUM_IsoMu24_or_IsoTkMu24_DEN_CutBasedIdTight_and_PFIsoTight_abseta_pt";
    } else if (_isRun17) {
        muonTrgHist = "NUM_IsoMu27_DEN_CutBasedIdTight_and_PFIsoTight_abseta_pt";
    } else if (_isRun18) {
        muonTrgHist = "NUM_IsoMu24_DEN_CutBasedIdTight_and_PFIsoTight_abseta_pt";
    }
    muonid = TFile::Open(("data/MuonSF/Efficiencies_muon_generalTracks_Z_Run" + muonFile + "_ID.root").c_str());
    TH2F* _hmuonid = dynamic_cast<TH2F *>(muonid->Get("NUM_TightID_DEN_TrackerMuons_abseta_pt"));
    _hmuonid->SetDirectory(0);
    muonid->Close();
    WeightCalculatorFromHistogram* _muonid = new WeightCalculatorFromHistogram(_hmuonid);

    muoniso = TFile::Open(("data/MuonSF/Efficiencies_muon_generalTracks_Z_Run" + muonFile + "_ISO.root").c_str());
    TH2F* _hmuoniso = dynamic_cast<TH2F *>(muoniso->Get("NUM_TightRelIso_DEN_TightIDandIPCut_abseta_pt"));
    _hmuoniso->SetDirectory(0);
    muoniso->Close();
    WeightCalculatorFromHistogram* _muoniso = new WeightCalculatorFromHistogram(_hmuoniso);

    muontrg = TFile::Open(("data/MuonSF/Efficiencies_muon_generalTracks_Z_Run" + muonFile + "_SingleMuonTriggers.root").c_str());
    TH2F* _hmuontrg = dynamic_cast<TH2F *>(muontrg->Get(muonTrgHist.c_str()));
    _hmuontrg->SetDirectory(0);
    muontrg->Close();
    WeightCalculatorFromHistogram* _muontrg = new WeightCalculatorFromHistogram(_hmuontrg);

    // We have only one muon!
    auto muonSFId = [this, _muonid](floats &pt, floats &eta)->floats {

        floats wVec;
        wVec.reserve(3); //cent, up, down

        if (pt.size() == 1) {
            for (unsigned int i=0; i<pt.size(); i++) {
                float sf = _muonid->getWeight(std::abs(eta[i]),pt[i]);
                float err = _muonid->getWeightErr(std::abs(eta[i]),pt[i]);
                wVec.emplace_back(sf);
                wVec.emplace_back(sf + err);
                wVec.emplace_back(sf - err);
            }
        }
        else wVec = {1.0, 1.0, 1.0};
        return wVec;
    };

    auto muonSFIso = [this, _muoniso](floats &pt, floats &eta)->floats {

        floats wVec;
        wVec.reserve(3); //cent, up, down

        if (pt.size() == 1) {
            for (unsigned int i=0; i<pt.size(); i++) {
                float sf = _muoniso->getWeight(std::abs(eta[i]),pt[i]);
                float err = _muoniso->getWeightErr(std::abs(eta[i]),pt[i]);
                wVec.emplace_back(sf);
                wVec.emplace_back(sf + err);
                wVec.emplace_back(sf - err);
            }
        }
        else wVec = {1.0, 1.0, 1.0};
        return wVec;
    };

    auto muonSFTrg = [this, _muontrg](floats &pt, floats &eta)->floats {

        floats wVec;
        wVec.reserve(3); //cent, up, down

        if (pt.size() == 1) {
            for (unsigned int i=0; i<pt.size(); i++) {
                float sf = _muontrg->getWeight(std::abs(eta[i]),pt[i]);
                float err = _muontrg->getWeightErr(std::abs(eta[i]),pt[i]);
                wVec.emplace_back(sf);
                wVec.emplace_back(sf + err);
                wVec.emplace_back(sf - err);
            }
        }
        else wVec = {1.0, 1.0, 1.0};
        return wVec;
    };

    _rlm = _rlm.Define("muonWeightId", muonSFId, {"Muon_pt","Muon_eta"})
               .Define("muonWeightIso", muonSFIso, {"Muon_pt","Muon_eta"})
               .Define("muonWeightTrg", muonSFTrg, {"Muon_pt","Muon_eta"});
}

void NanoAODAnalyzerrdframe::selectLeptons()
{
        
        auto selLeptons = [](FourVectorVec &muon4vecs, FourVectorVec &ele4vecs, int nmuonpass, int nelepass, int nvetoeles, int nvetomuons){ 
                FourVectorVec outs;
                if( nmuonpass == 1 && nvetoeles == 0 && nvetomuons == 0 ) outs.emplace_back(muon4vecs[0]);
                if( nelepass == 1 && nvetoeles == 0 && nvetomuons == 0 ) outs.emplace_back(ele4vecs[0]);
                return outs;
        };

        auto channel = [](FourVectorVec &muon4vecs, FourVectorVec &ele4vecs, int nmuonpass, int nelepass, int nvetoeles, int nvetomuons){ 
                int ch=100;
                if( nmuonpass == 1 && nelepass == 0 && nvetoeles == 0 && nvetomuons == 0 ) ch = 0;
                if( nelepass == 1 && nmuonpass == 0 && nvetoeles == 0 && nvetomuons == 0 ) ch = 1;
                return ch;
        };
        
        _rlm = _rlm.Define("selLepton4vecs", selLeptons, {"muon4vecs", "ele4vecs", "nmuonpass", "nelepass", "nvetoeles", "nvetomuons" }); 
        _rlm = _rlm.Define("channel", channel, {"muon4vecs", "ele4vecs", "nmuonpass", "nelepass", "nvetoeles", "nvetomuons" }); 
        _rlm = _rlm.Define("lepton_pt", "float(selLepton4vecs[0].Pt())");
        _rlm = _rlm.Define("lepton_eta", "float(selLepton4vecs[0].Eta())");
        _rlm = _rlm.Define("lepton_phi", "float(selLepton4vecs[0].Phi())");
        _rlm = _rlm.Define("lepton_m", "float(selLepton4vecs[0].M())");

}



void NanoAODAnalyzerrdframe::setupJetMETCorrection(string globaltag, std::vector<std::string> jes_var, std::string jetalgo, bool dataMc) {

    std::vector<JetCorrectionUncertainty*> regroupedUnc;
    FactorizedJetCorrector* _jetCorrector;

    if (_globaltag != "") {
        cout << "Applying new JetMET corrections. GT: " + _globaltag + " on jetAlgo: AK4PFchs" << endl;
        string basedirectory = "data/jes/";

        string datamcflag = "";
        if (dataMc) datamcflag = "DATA";
        else datamcflag = "MC";

        // set file names that contain the parameters for corrections
        string dbfilenamel1 = basedirectory + _globaltag + "_" + datamcflag + "_L1FastJet_" + jetalgo + ".txt";
        string dbfilenamel2 = basedirectory + _globaltag + "_" + datamcflag + "_L2Relative_" + jetalgo + ".txt";
        string dbfilenamel3 = basedirectory + _globaltag + "_" + datamcflag + "_L3Absolute_" + jetalgo + ".txt";
        string dbfilenamel2l3 = basedirectory + _globaltag + "_" + datamcflag + "_L2L3Residual_" + jetalgo + ".txt";

        JetCorrectorParameters *L1JetCorrPar = new JetCorrectorParameters(dbfilenamel1);
        if (!L1JetCorrPar->isValid()) {
          std::cerr << "L1FastJet correction parameters not read" << std::endl;
          exit(1);
        }

        JetCorrectorParameters *L2JetCorrPar = new JetCorrectorParameters(dbfilenamel2);
        if (!L2JetCorrPar->isValid()) {
            std::cerr << "L2Relative correction parameters not read" << std::endl;
            exit(1);
        }

        JetCorrectorParameters *L3JetCorrPar = new JetCorrectorParameters(dbfilenamel3);
        if (!L3JetCorrPar->isValid()) {
            std::cerr << "L3Absolute correction parameters not read" << std::endl;
            exit(1);
        }

        JetCorrectorParameters *L2L3JetCorrPar = new JetCorrectorParameters(dbfilenamel2l3);
        if (!L2L3JetCorrPar->isValid()) {
            std::cerr << "L2L3Residual correction parameters not read" << std::endl;
            exit(1);
        }

        // to apply all the corrections, first collect them into a vector
        std::vector<JetCorrectorParameters> jetc;
        jetc.push_back(*L1JetCorrPar);
        jetc.push_back(*L2JetCorrPar);
        jetc.push_back(*L3JetCorrPar);
        jetc.push_back(*L2L3JetCorrPar);

        // apply the various corrections
        _jetCorrector = new FactorizedJetCorrector(jetc);

        // object to calculate uncertainty
        if (!dataMc) {
            cout<<"Applying JEC Uncertainty"<<endl;
            for (std::string src : jes_var) {
                if (src.find("up") != std::string::npos) {
                    auto uncsource = src.substr(3, src.size()-2-3);
                    cout << "JEC Uncertainty Source : " + uncsource << endl;
                    string dbfilenameunc = basedirectory + "RegroupedV2_" + _globaltag + "_MC_UncertaintySources_AK4PFchs.txt";
                    JetCorrectorParameters* uncCorrPar = new JetCorrectorParameters(dbfilenameunc, uncsource);
                    JetCorrectionUncertainty* _jetCorrectionUncertainty = new JetCorrectionUncertainty(*uncCorrPar);
                    regroupedUnc.emplace_back(_jetCorrectionUncertainty);
                } else {
                    continue; //We only need var name, no up/down
                }
            }
        }
    }

    auto applyJes = [this, _jetCorrector](floats jetpts, floats jetetas, floats jetAreas, floats jetrawf, float rho, floats tocorrect)->floats {

        floats corrfactors;
        corrfactors.reserve(jetpts.size());

        for (unsigned int i=0; i<jetpts.size(); i++) {
            float rawfrac = 1.0-jetrawf[i];
            float rawjetpt = jetpts[i] * (rawfrac);
            _jetCorrector->setJetPt(rawjetpt);
            _jetCorrector->setJetEta(jetetas[i]);
            _jetCorrector->setJetA(jetAreas[i]);
            _jetCorrector->setRho(rho);
            float corrfactor = _jetCorrector->getCorrection();
            corrfactors.emplace_back(tocorrect[i] * rawfrac * corrfactor);
        }
        return corrfactors;
    };

    // structure: jes[jetIdx][varIdx]
    auto jesUnc = [this, regroupedUnc](floats jetpts, floats jetetas, floats jetAreas, floats jetrawf, float rho)->floatsVec {

        floats uncSources;
        uncSources.reserve(2 * regroupedUnc.size());
        floatsVec uncertainties;
        uncertainties.reserve(jetpts.size());

        for (unsigned int i=0; i<jetpts.size(); i++) {
            for (size_t j=0; j<regroupedUnc.size(); j++) {
                auto corrector = regroupedUnc[j];

                corrector->setJetPt(jetpts[i]);
                corrector->setJetEta(jetetas[i]);
                float unc = corrector->getUncertainty(true);
                uncSources.emplace_back(1.0f + unc);
                uncSources.emplace_back(1.0f - unc);
            }
            uncertainties.emplace_back(uncSources);
            uncSources.clear();
        }
        return uncertainties;
    };

    auto metCorr = [](float met, float metphi, floats jetptsbefore, floats jetptsafter, floats jetphis)->float {

        auto metx = met * cos(metphi);
        auto mety = met * sin(metphi);

        for (unsigned int i=0; i<jetphis.size(); i++) {
            if (jetptsafter[i] > 15.0) {
                metx -= (jetptsafter[i] - jetptsbefore[i])*cos(jetphis[i]);
                mety -= (jetptsafter[i] - jetptsbefore[i])*sin(jetphis[i]);
            }
        }
        return (float(sqrt(metx*metx + mety*mety)));
    };

    auto metUnc = [](float met, float metphi, floats jetptsbefore, floatsVec jetptscorr, floats jetphis)->floats {

        floats corrfactors;
        corrfactors.reserve(static_cast<int>(jetptscorr.size()));

        for (size_t j=0; j<jetptscorr[0].size(); j++) {
            auto metx = met * cos(metphi);
            auto mety = met * sin(metphi);

            for (unsigned int i=0; i<jetphis.size(); i++) {
                if (jetptsbefore[i] * jetptscorr[i][j] > 15.0) {
                    metx -= (jetptscorr[i][j] - 1.0) * jetptsbefore[i] * cos(jetphis[i]);
                    mety -= (jetptscorr[i][j] - 1.0) * jetptsbefore[i] * sin(jetphis[i]);
                }
            }
            corrfactors.emplace_back(float(sqrt(metx*metx + mety*mety)));
        }
        return corrfactors;
    };

    auto metPhiCorr = [](float met, float metphi, floats jetptsbefore, floats jetptsafter, floats jetphis)->float {

        auto metx = met * cos(metphi);
        auto mety = met * sin(metphi);

        for (unsigned int i=0; i<jetphis.size(); i++) {
            if (jetptsafter[i] > 15.0) {
                metx -= (jetptsafter[i] - jetptsbefore[i])*cos(jetphis[i]);
                mety -= (jetptsafter[i] - jetptsbefore[i])*sin(jetphis[i]);
            }
        }
        return float(atan2(mety, metx));
    };

    auto metPhiUnc = [](float met, float metphi, floats jetptsbefore, floatsVec jetptscorr, floats jetphis)->floats {

        floats corrfactors;
        corrfactors.reserve(static_cast<int>(jetptscorr.size()));

        for (size_t j=0; j<jetptscorr[0].size(); j++) {
            auto metx = met * cos(metphi);
            auto mety = met * sin(metphi);

            for (unsigned int i=0; i<jetphis.size(); i++) {
                if (jetptsbefore[i] * jetptscorr[i][j] > 15.0) {
                    metx -= (jetptscorr[i][j] - 1.0) * jetptsbefore[i] * cos(jetphis[i]);
                    mety -= (jetptscorr[i][j] - 1.0) * jetptsbefore[i] * sin(jetphis[i]);
                }
            }
            corrfactors.emplace_back(float(atan2(mety, metx)));
        }
        return corrfactors;
    };

    //FIXME: should correct jet mass. but can we do it at once?
    if (_jetCorrector != 0) {
        _rlm = _rlm.Define("Jet_pt_uncorr", "Jet_pt");
        _rlm = _rlm.Define("Jet_pt_corr", applyJes, {"Jet_pt", "Jet_eta", "Jet_area", "Jet_rawFactor", "fixedGridRhoFastjetAll", "Jet_pt"})
                   .Redefine("Jet_mass", applyJes, {"Jet_pt", "Jet_eta", "Jet_area", "Jet_rawFactor", "fixedGridRhoFastjetAll", "Jet_mass"})
                   .Redefine("MET_pt", metCorr, {"MET_pt", "MET_phi", "Jet_pt", "Jet_pt_corr", "Jet_phi"})
                   .Redefine("MET_phi", metPhiCorr, {"MET_pt", "MET_phi", "Jet_pt", "Jet_pt_corr", "Jet_phi"});
        if (!dataMc) {
            _rlm = _rlm.Define("Jet_pt_unc", jesUnc, {"Jet_pt", "Jet_eta", "Jet_area", "Jet_rawFactor", "fixedGridRhoFastjetAll"})
                       .Define("MET_pt_unc", metUnc, {"MET_pt", "MET_phi", "Jet_pt", "Jet_pt_unc", "Jet_phi"})
                       .Define("MET_phi_unc", metPhiUnc, {"MET_pt", "MET_phi", "Jet_pt", "Jet_pt_unc", "Jet_phi"});
        }
        _rlm = _rlm.Redefine("Jet_pt", "Jet_pt_corr");
    }


    // JER
    std::string jetResFilePath_ = "data/jer/";
    std::string jetResSFFilePath_ = "data/jer/";
    if (_isRun16pre) {
        jetResFilePath_ += "Summer20UL16APV_JRV3_MC_PtResolution_AK4PFchs.txt";
        jetResSFFilePath_ += "Summer20UL16APV_JRV3_MC_SF_AK4PFchs.txt";
    } else if (_isRun16post) {
        jetResFilePath_ += "Summer20UL16_JRV3_MC_PtResolution_AK4PFchs.txt";
        jetResSFFilePath_ += "Summer20UL16_JRV3_MC_SF_AK4PFchs.txt";
    } else if (_isRun17) {
        jetResFilePath_ += "Summer19UL17_JRV2_MC_PtResolution_AK4PFchs.txt";
        jetResSFFilePath_ += "Summer19UL17_JRV2_MC_SF_AK4PFchs.txt";
    } else if (_isRun18) {
        jetResFilePath_ += "Summer19UL18_JRV2_MC_PtResolution_AK4PFchs.txt";
        jetResSFFilePath_ += "Summer19UL18_JRV2_MC_SF_AK4PFchs.txt";
    }

    JME::JetResolution jetResObj;
    JME::JetResolutionScaleFactor jetResSFObj;
    if (!_isData) {
        jetResObj = JME::JetResolution(jetResFilePath_);
        jetResSFObj = JME::JetResolutionScaleFactor(jetResSFFilePath_);
    }

    // Compute the JER and Unc ( v[pt][unc], unc = nom, up, down)
    // cattool + PhysicsTools/PatUtils/interface/SmearedJetProducerT.h
    auto applyJer = [this, jetResObj, jetResSFObj](floats jetpts, floats jetetas, floats jetphis, floats jetms,
                    floats genjetpts, floats genjetetas, floats genjetphis, floats genjetms, ints genidx, float rho, unsigned long long event)
                    ->floatsVec {

        floatsVec out;
        out.reserve(jetpts.size());
        floats var;
        var.reserve(3);

        if (jetpts.size() > 0) {
            for (size_t i=0; i<jetpts.size(); i++) {

                JME::JetParameters jetPars = {{JME::Binning::JetPt, jetpts[i]},
                                              {JME::Binning::JetEta, jetetas[i]},
                                              {JME::Binning::Rho, rho}};
                const float jetRes = static_cast<float>(jetResObj.getResolution(jetPars)); // Note: this is relative resolution.
                const float cJER   = static_cast<float>(jetResSFObj.getScaleFactor(jetPars));
                const float cJERUp = static_cast<float>(jetResSFObj.getScaleFactor(jetPars, Variation::UP));
                const float cJERDn = static_cast<float>(jetResSFObj.getScaleFactor(jetPars, Variation::DOWN));

                bool _isGenMatch = false;
                ROOT::Math::PtEtaPhiMVector jetv(jetpts[i], jetetas[i], jetphis[i], jetms[i]);
                ROOT::Math::PtEtaPhiMVector genv(genjetpts[i], genjetetas[i], genjetphis[i], genjetms[i]);
                if (genidx[i] >= 0 && ROOT::Math::VectorUtil::DeltaR(jetv, genv) < 0.2 &&
                    std::abs(genjetpts[i] - jetpts[i]) < jetRes * 3 * jetpts[i])
                    _isGenMatch = true;


                // JER - apply scaling method if matched genJet is found,
                //       apply gaussian smearing method if unmatched
                float jetpt = jetpts[i];
                float genjetpt = genjetpts[i];
                if (_isGenMatch) {
                    float dPt = jetpt - genjetpt;
                    float jersf = 1 + (dPt * (cJER - 1))/jetpt;
                    float jersfup = 1 + (dPt * (cJERUp - 1))/jetpt;
                    float jersfdn = 1 + (dPt * (cJERDn - 1))/jetpt;

                    floats tmpjers = {jersf, jersfup, jersfdn};
                    for (size_t j=0; j<tmpjers.size(); j++) {
                        float tmpjer = tmpjers[j];
                        if (std::isnan(tmpjer) or std::isinf(tmpjer) or tmpjer<0 ) var.emplace_back(1.0f);
                        else var.emplace_back(std::max(0.0f, tmpjer));
                    }

                } else if (cJER > 1){
                    std::uint32_t seed = uint32_t(jetetas[0] * 100) + static_cast<unsigned int>(event);
                    std::mt19937 m_random_generator = std::mt19937(seed);

                    float sigma = jetRes * std::sqrt(cJER * cJER - 1);
                    float sigmaUp = jetRes * std::sqrt(cJERUp * cJERUp - 1);
                    float sigmaDn = jetRes * std::sqrt(cJERDn * cJERDn - 1);
                    std::normal_distribution<float> d(0, sigma);
                    std::normal_distribution<float> dup(0, sigmaUp);
                    std::normal_distribution<float> ddn(0, sigmaDn);

                    floats tmpjers = {1.0f + d(m_random_generator), 1.0f + dup(m_random_generator), 1.0f + ddn(m_random_generator)};
                    for (size_t j=0; j<tmpjers.size(); j++) {
                        float tmpjer = tmpjers[j];
                        if (std::isnan(tmpjer) or std::isinf(tmpjer) or tmpjer<0 ) var.emplace_back(1.0f);
                        else var.emplace_back(std::max(0.0f, tmpjer));
                    }
                } else {
                    var = {1.0f, 1.0f, 1.0f};
                }
                out.emplace_back(var);
                var.clear();
            }
        }
        return out;
    };

    if (!dataMc) {
        _rlm = _rlm.Define("Jet_jer", applyJer, {"Jet_pt", "Jet_eta", "Jet_phi", "Jet_mass", "GenJet_pt", "GenJet_eta", "GenJet_phi", "GenJet_mass", "Jet_genJetIdx", "fixedGridRhoFastjetAll", "event"});
    }

}

void NanoAODAnalyzerrdframe::skimJets() {

    // input vector: vec[pt][vars]
    // Note: do not skim with exact value of pt!
    auto skimCol = [this](floatsVec toSkim, ints cut)->floatsVec {

        floatsVec out;
        for (size_t i=0; i<toSkim.size(); i++) {
            if (cut[i] > 0) out.emplace_back(toSkim[i]);
        }
        return out;
    };

    // skim jet collection
    _rlm = _rlm.Define("jetcuts", "Jet_pt>20.0 && abs(Jet_eta)<2.4 && Jet_jetId == 6")
               .Redefine("Jet_pt", "Jet_pt[jetcuts]")
               .Redefine("Jet_eta", "Jet_eta[jetcuts]")
               .Redefine("Jet_phi", "Jet_phi[jetcuts]")
               .Redefine("Jet_mass", "Jet_mass[jetcuts]")
               .Redefine("Jet_btag", "Jet_btagDeepFlavB[jetcuts]")
               .Redefine("Jet_cvsb", "Jet_btagDeepFlavC[jetcuts]/(Jet_btagDeepFlavC[jetcuts]+Jet_btagDeepFlavB[jetcuts])")
               .Redefine("Jet_cvsl", "Jet_btagDeepFlavC[jetcuts]/(1- Jet_btagDeepFlavB[jetcuts])")
               .Redefine("Jet_jetId", "Jet_jetId[jetcuts]")
               .Redefine("Jet_area", "Jet_area[jetcuts]")
               .Redefine("Jet_pt_uncorr", "Jet_pt_uncorr[jetcuts]")
               .Redefine("Jet_rawFactor", "Jet_rawFactor[jetcuts]")
               .Redefine("Jet_btagDeepFlavB", "Jet_btagDeepFlavB[jetcuts]")
               .Redefine("nJet", "int(Jet_pt.size())");
    if (!_isData) {
        _rlm = _rlm.Redefine("Jet_pt_unc", skimCol, {"Jet_pt_unc", "jetcuts"})
                   .Redefine("Jet_jer", skimCol, {"Jet_jer", "jetcuts"})
                   .Redefine("Jet_hadronFlavour","Jet_hadronFlavour[jetcuts]")
                   .Redefine("Jet_genJetIdx","Jet_genJetIdx[jetcuts]");
    }

}

void NanoAODAnalyzerrdframe::applyBSFs(std::vector<string> jes_var) {

    cout << "Loading Btag SF" << endl;
    string btagpath = "data/btagSF/";

    BTagCalibration _btagcalib = {"DeepJet", btagpath + "skimmed_btag_" + _year + ".csv"};
    cout << "    Loaded file : " << btagpath + "skimmed_btag_" + _year + ".csv" << endl;
    BTagCalibration _btagcalibJes = {"DeepJet", btagpath + "skimmed_jes_" + _year + ".csv"};
    cout << "    Loaded file : " << btagpath + "skimmed_jes_" + _year + ".csv" << endl;

    _rlm = _rlm.Define("btag_var", [](){return strings(btag_var);})
               .Define("btag_jes_var", [jes_var](){return strings(jes_var);});


    BTagCalibrationReader _btagcalibreader = {BTagEntry::OP_RESHAPING, "central", btag_var};
    BTagCalibrationReader _btagcalibreaderJes = {BTagEntry::OP_RESHAPING, "central", jes_var};

    // load the formulae b flavor tagging
    _btagcalibreader.load(_btagcalib, BTagEntry::FLAV_B, "iterativefit");
    _btagcalibreader.load(_btagcalib, BTagEntry::FLAV_C, "iterativefit");
    _btagcalibreader.load(_btagcalib, BTagEntry::FLAV_UDSG, "iterativefit");
    _btagcalibreaderJes.load(_btagcalibJes, BTagEntry::FLAV_B, "iterativefit");
    _btagcalibreaderJes.load(_btagcalibJes, BTagEntry::FLAV_C, "iterativefit");
    _btagcalibreaderJes.load(_btagcalibJes, BTagEntry::FLAV_UDSG, "iterativefit");

    // function to calculate event weight for MC events based on DeepJet algorithm
    auto btagweightgenerator = [this, _btagcalibreader](floats &pts, floats &etas, ints &hadflav, floats &btags, strings &var, floatsVec &jer)->doublesVec {

        doubles bSFs;
        bSFs.reserve(var.size());
        doublesVec out;
        out.reserve(pts.size());

        for (unsigned int j=0; j<pts.size(); j++) {
            for (size_t i=0; i<var.size(); i++) {
                double bweight = 1.0;
                auto newpt = pts[j]*jer[j][0];
                if (newpt > 40) {
                    std::string unc = var[i];
                    if (unc.find("cferr") != std::string::npos and hadflav[j] != 4) unc = "central";
                    if (unc.find("cferr") == std::string::npos and hadflav[j] == 4) unc = "central";

                    BTagEntry::JetFlavor hadfconv;
                    if      (hadflav[j]==5) hadfconv=BTagEntry::FLAV_B;
                    else if (hadflav[j]==4) hadfconv=BTagEntry::FLAV_C;
                    else                    hadfconv=BTagEntry::FLAV_UDSG;
                    bweight = _btagcalibreader.eval_auto_bounds(unc, hadfconv, fabs(etas[j]), newpt, btags[j]);
                }
                bSFs.emplace_back(bweight);
            }
            out.emplace_back(bSFs);
            bSFs.clear();
        }
        return out;
    };

    auto btagweightgeneratorJes = [this, _btagcalibreaderJes](floats &pts, floats &etas, ints &hadflav,
                                  floats &btags, floatsVec jes, strings &var, floatsVec &jer)->doublesVec {

        doubles bSFs;
        bSFs.reserve(var.size());
        doublesVec out;
        out.reserve(pts.size());

        for (unsigned int j=0; j<pts.size(); j++) {
            for (size_t i=0; i<var.size(); i++) {
                double bweight = 1.0;
                auto newpt = pts[j] * jes[j][i] * jer[j][0];
                if (newpt > 40) {
                    std::string unc = var[i];
                    if (hadflav[j] == 4) unc = "central";

                    BTagEntry::JetFlavor hadfconv;
                    if      (hadflav[j]==5) hadfconv=BTagEntry::FLAV_B;
                    else if (hadflav[j]==4) hadfconv=BTagEntry::FLAV_C;
                    else                    hadfconv=BTagEntry::FLAV_UDSG;
                    bweight = _btagcalibreaderJes.eval_auto_bounds(unc, hadfconv, fabs(etas[j]), newpt, btags[j]);
                }
                bSFs.emplace_back(bweight);
            }
            out.emplace_back(bSFs);
            bSFs.clear();
        }
        return out;
    };

    cout << "Generate b-tagging weight" << endl;
    _rlm = _rlm.Define("btagWeight_DeepFlavB_perJet", btagweightgenerator, {"Jet_pt", "Jet_eta", "Jet_hadronFlavour", "Jet_btagDeepFlavB", "btag_var", "Jet_jer"})
               .Define("btagWeight_DeepFlavB_jes_perJet", btagweightgeneratorJes, {"Jet_pt", "Jet_eta", "Jet_hadronFlavour", "Jet_btagDeepFlavB", "Jet_pt_unc", "btag_jes_var", "Jet_jer"});
}

void NanoAODAnalyzerrdframe::selectJets(std::vector<std::string> jes_var) {


    // input vector: vec[pt][vars], for bSF
    auto skimCol = [this](doublesVec toSkim, ints cut)->doublesVec {

        doublesVec out;
        for (size_t i=0; i<toSkim.size(); i++) {
            if (cut[i] > 0) out.emplace_back(toSkim[i]);
        }
        return out;
    };

    // input vector: vec[pt][vars]
    auto calcBSF = [this](doublesVec perJetSF, int nvar)->doubles {

        doubles out;
        out.reserve(nvar);
        for (size_t i=0; i<nvar; i++) {
            double bSF = 1.0;
            for (size_t j=0; j<perJetSF.size(); j++) {
                if (perJetSF[j].empty()) continue;
                bSF *= perJetSF[j][i];
            }
            out.emplace_back(bSF);
        }
        return out;
    };

    if (!_isData) {

        auto syst_unc = _syst;

        auto selectJer = [syst_unc](floatsVec unc)->floats {

            int idx = 0;
            if (syst_unc == "jerup") idx = 1;
            else if (syst_unc == "jerdown") idx = 2;
            floats selected;
            selected.reserve(unc.size());

            for (size_t i=0; i<unc.size(); i++) {
                selected.emplace_back(unc[i][idx]);
            }
            return selected;
        };

        if (_syst.find("jes") != std::string::npos) {

            auto selectJes = [syst_unc, jes_var](floatsVec unc)->floats {

                floats selected;
                selected.reserve(unc.size());
                unsigned int jesidx = -1;
                for (size_t i=0; i<jes_var.size(); i++) {
                    if (jes_var[i] == syst_unc) jesidx = i;
                }
                if (int(jesidx) == -1) cerr << "Found No JES Unc Name!!" << endl;

                for (size_t i=0; i<unc.size(); i++) {
                    selected.emplace_back(unc[i][jesidx]);
                }
                return selected;

            };

            _rlm = _rlm.Define("Jet_pt_unc_toapply", selectJes, {"Jet_pt_unc"})
                       .Define("Jet_jer_toapply", selectJer, {"Jet_jer"})
                       .Redefine("Jet_pt", "Jet_pt * Jet_jer_toapply * Jet_pt_unc_toapply")
                       .Redefine("Jet_mass", "Jet_mass * Jet_jer_toapply * Jet_pt_unc_toapply");

        } else {
            _rlm = _rlm.Define("Jet_jer_toapply", selectJer, {"Jet_jer"})
                       .Redefine("Jet_pt", "Jet_pt * Jet_jer_toapply")
                       .Redefine("Jet_mass", "Jet_mass * Jet_jer_toapply");
        }
    }

    _rlm = _rlm.Define("jetcuts", "Jet_pt>20.0 && abs(Jet_eta)<2.4 && Jet_jetId == 6");
    _rlm = _rlm.Redefine("Jet_pt", "Jet_pt[jetcuts]")
               .Redefine("Jet_eta", "Jet_eta[jetcuts]")
               .Redefine("Jet_phi", "Jet_phi[jetcuts]")
               .Redefine("Jet_mass", "Jet_mass[jetcuts]")
               .Redefine("Jet_btagDeepFlavB", "Jet_btagDeepFlavB[jetcuts]")
               .Define("jet4vecs", ::gen4vec, {"Jet_pt", "Jet_eta", "Jet_phi", "Jet_mass"});

    if (!_isData) {
        _rlm = _rlm.Redefine("btagWeight_DeepFlavB_perJet", skimCol, {"btagWeight_DeepFlavB_perJet", "jetcuts"})
                   .Redefine("btagWeight_DeepFlavB_jes_perJet", skimCol, {"btagWeight_DeepFlavB_jes_perJet", "jetcuts"});
    }

    // for checking overlapped jets with leptons
    auto checkoverlap = [](FourVectorVec &seljets, FourVectorVec &sellep) {

        ints mindrlepton;
        for (auto ajet: seljets) {
            auto mindr = 6.0;
            for ( auto alepton : sellep ) {
                auto dr = ROOT::Math::VectorUtil::DeltaR(ajet, alepton);
                if (dr < mindr) mindr = dr;
            }
            int out = mindr >= 0.4 ? 1 : 0;
            mindrlepton.emplace_back(out);
        }
        return mindrlepton;
    };

    // Overlap removal with muon (used for btagging SF)
    _rlm = _rlm.Define("muonjetoverlap", checkoverlap, {"jet4vecs","muon4vecs"})
    		   .Define("elejetoverlap", checkoverlap, {"jet4vecs","ele4vecs"})
               //.Define("taujetoverlap", checkoverlap, {"jet4vecs","cleantau4vecs"})
               //.Define("jetoverlap","muonjetoverlap && taujetoverlap");
               .Define("jetoverlap","muonjetoverlap && elejetoverlap");

    _rlm = _rlm.Redefine("Jet_pt", "Jet_pt[jetoverlap]")
               .Redefine("Jet_eta", "Jet_eta[jetoverlap]")
               .Redefine("Jet_phi", "Jet_phi[jetoverlap]")
               .Redefine("Jet_mass", "Jet_mass[jetoverlap]")
               .Redefine("Jet_btagDeepFlavB", "Jet_btagDeepFlavB[jetoverlap]")
               .Define("ncleanjetspass", "int(Jet_pt.size())")
               .Define("cleanjet4vecs", ::gen4vec, {"Jet_pt", "Jet_eta", "Jet_phi", "Jet_mass"})
               .Define("Jet_HT", "Sum(Jet_pt)");

    if (!_isData) {
        int nbsf_var = btag_var.size();
        int njes_var = jes_var.size();
        _rlm = _rlm.Redefine("btagWeight_DeepFlavB_perJet", skimCol, {"btagWeight_DeepFlavB_perJet", "jetoverlap"})
                   .Redefine("btagWeight_DeepFlavB_jes_perJet", skimCol, {"btagWeight_DeepFlavB_jes_perJet", "jetoverlap"})
                   .Define("nbsf_var", [nbsf_var](){return int(nbsf_var);})
                   .Define("njes_var", [njes_var](){return int(njes_var);})
                   .Define("btagWeight_DeepFlavB", calcBSF, {"btagWeight_DeepFlavB_perJet", "nbsf_var"})
                   .Define("btagWeight_DeepFlavB_jes", calcBSF, {"btagWeight_DeepFlavB_jes_perJet", "njes_var"});
    }


    // b-tagging
    if (_isRun16pre) {
        //https://twiki.cern.ch/twiki/bin/view/CMS/BtagRecommendation106XUL16preVFP
        _rlm = _rlm.Define("btagcuts", "Jet_btagDeepFlavB>0.2598"); //l: 0.0508, m: 0.2598, t: 0.6502
    } else if (_isRun16post) {
        //https://twiki.cern.ch/twiki/bin/view/CMS/BtagRecommendation106XUL16postVFP#AK4_b_tagging
        _rlm = _rlm.Define("btagcuts", "Jet_btagDeepFlavB>0.2489"); //l: 0.0480, m: 0.2489, t: 0.6377
    } else if (_isRun17) {
        //https://twiki.cern.ch/twiki/bin/viewauth/CMS/BtagRecommendation106XUL17
        _rlm = _rlm.Define("btagcuts", "Jet_btagDeepFlavB>0.3040"); //l: 0.0532, m: 0.3040, t: 0.7476
    } else if (_isRun18) {
        //https://twiki.cern.ch/twiki/bin/viewauth/CMS/BtagRecommendation106XUL18
        _rlm = _rlm.Define("btagcuts", "Jet_btagDeepFlavB>0.2783"); //l: 0.0490, m: 0.2783, t: 0.7100
    }

    _rlm = _rlm.Define("bJet_pt", "Jet_pt[btagcuts]")
               .Define("bJet_eta", "Jet_eta[btagcuts]")
               .Define("bJet_phi", "Jet_phi[btagcuts]")
               .Define("bJet_mass", "Jet_mass[btagcuts]")
               .Define("bJet_btagDeepFlavB", "Jet_btagDeepFlavB[btagcuts]")
               .Define("ncleanbjetspass", "int(bJet_pt.size())")
               .Define("bJet_HT", "Sum(bJet_pt)")
               .Define("cleanbjet4vecs", ::gen4vec, {"bJet_pt", "bJet_eta", "bJet_phi", "bJet_mass"});
}
/*
void NanoAODAnalyzerrdframe::selectTaus() {

    auto syst_unc = _syst;

    //TES var.
    if (!_isData) {

        auto selectTES = [syst_unc](floatsVec unc)->floats {

            int idx = -1;
            if (syst_unc.find("tesup") != std::string::npos) idx = 0;
            else if (syst_unc.find("tesdown") != std::string::npos) idx = 1;
            floats selected;
            selected.reserve(unc.size());

            for (size_t i=0; i<unc.size(); i++) {
                if (idx < 0) selected.emplace_back(1.0f);
                selected.emplace_back(unc[i][idx]);
                std::cout << idx << " " << unc[i][idx]  << endl;
            }
            return selected;
        };

        if (_syst.find("tes") != std::string::npos) {
          _rlm = _rlm.Define("Tau_pt_unc_toapply", selectTES, {"Tau_pt_unc"})
                     .Redefine("Tau_pt", "Tau_pt * Tau_pt_unc_toapply")
                     .Redefine("Tau_mass", "Tau_mass * Tau_pt_unc_toapply");
        }
    }

    auto overlap_removal_mutau = [](FourVectorVec &muon4vecs, FourVectorVec &tau4vecs) {
        ints out;
        for (auto tau: tau4vecs) {
            int check = 0;
            for (auto mu: muon4vecs) {
                auto dR = ROOT::Math::VectorUtil::DeltaR(mu, tau);
                if( dR >= 0.4 ) check = 1;
            }
            out.emplace_back(check);
        }
        return out;
    };

    _rlm = _rlm.Define("tau4vecs", ::gen4vec, {"Tau_pt", "Tau_eta", "Tau_phi", "Tau_mass"})
               .Define("mutauoverlap", overlap_removal_mutau, {"muon4vecs","tau4vecs"});

    // input vector: vec[pt][vars]
    auto skimCol = [this](floatsVec toSkim, ints cut)->floatsVec {

        floatsVec out;
        for (size_t i=0; i<toSkim.size(); i++) {
            if (cut[i] > 0) out.emplace_back(toSkim[i]);
        }
        if (out.size() > 0) return out;
        else return {{1.0f, 1.0f, 1.0f}};
    };

    // Hadronic Tau Object Selections
    _rlm = _rlm.Define("taucuts", "Tau_pt>40.0 && abs(Tau_eta)<2.3 && Tau_idDecayModeNewDMs  && (Tau_decayMode == 0 || Tau_decayMode == 1 || Tau_decayMode == 2 || Tau_decayMode == 10 || Tau_decayMode == 11)")
               .Define("deeptauidcuts","Tau_idDeepTau2017v2p1VSmu & 8 && Tau_idDeepTau2017v2p1VSe & 4 && Tau_idDeepTau2017v2p1VSjet & 64");

    // Hadronic Tau Selection
    _rlm = _rlm.Define("seltaucuts","taucuts && deeptauidcuts && mutauoverlap")
               .Redefine("Tau_pt", "Tau_pt[seltaucuts]")
               .Redefine("Tau_eta", "Tau_eta[seltaucuts]")
               .Redefine("Tau_phi", "Tau_phi[seltaucuts]")
               .Redefine("Tau_mass", "Tau_mass[seltaucuts]")
               .Redefine("Tau_charge", "Tau_charge[seltaucuts]")
               .Redefine("Tau_jetIdx", "Tau_jetIdx[seltaucuts]")
               .Redefine("Tau_decayMode", "Tau_decayMode[seltaucuts]")
               .Define("ncleantaupass", "int(Tau_pt.size())")
               .Define("cleantau4vecs", ::gen4vec, {"Tau_pt", "Tau_eta", "Tau_phi", "Tau_mass"});

    if (!_isData) {
        _rlm = _rlm.Redefine("Tau_genPartFlav","Tau_genPartFlav[seltaucuts]")
                   .Redefine("tauWeightIdVsJet", skimCol, {"tauWeightIdVsJet", "seltaucuts"})
                   .Redefine("tauWeightIdVsEl", skimCol, {"tauWeightIdVsEl", "seltaucuts"})
                   .Redefine("tauWeightIdVsMu", skimCol, {"tauWeightIdVsMu", "seltaucuts"});
    }

}
*/
void NanoAODAnalyzerrdframe::matchGenReco() {

    _rlm = _rlm.Define("FinalGenPart_idx", ::FinalGenPart_idx, {"GenPart_pdgId", "GenPart_genPartIdxMother"})
               .Define("GenPart_LFVup_idx", "FinalGenPart_idx[0]")
               .Define("GenPart_LFVmuon_idx", "FinalGenPart_idx[1]")
               .Define("GenPart_LFVtau_idx", "FinalGenPart_idx[2]")
               .Define("GenPart_SMb_idx", "FinalGenPart_idx[3]")
               .Define("GenPart_SMW1_idx", "FinalGenPart_idx[4]")
               .Define("GenPart_SMW2_idx", "FinalGenPart_idx[5]")
               .Define("GenPart_LFVtop_idx", "FinalGenPart_idx[6]")
               .Define("GenPart_SMtop_idx", "FinalGenPart_idx[7]");

    _rlm = _rlm.Define("drmax1", "float(0.15)")
               .Define("drmax2", "float(0.4)")
               .Define("Muon_matched", ::dRmatching_binary,{"GenPart_LFVmuon_idx","drmax1","GenPart_pt","GenPart_eta","GenPart_phi","GenPart_mass","Muon_pt","Muon_eta","Muon_phi","Muon_mass"})
               .Define("Tau_matched",::dRmatching_binary,{"GenPart_LFVtau_idx","drmax2", "GenPart_pt","GenPart_eta","GenPart_phi","GenPart_mass","Tau_pt","Tau_eta","Tau_phi","Tau_mass"})
               .Define("Jet_LFVup_matched",::dRmatching_binary,{"GenPart_LFVup_idx","drmax2","GenPart_pt","GenPart_eta","GenPart_phi","GenPart_mass","Jet_pt","Jet_eta","Jet_phi","Jet_mass"})
               .Define("Jet_SMb_matched",::dRmatching_binary,{"GenPart_SMb_idx","drmax2","GenPart_pt","GenPart_eta","GenPart_phi","GenPart_mass","Jet_pt","Jet_eta","Jet_phi","Jet_mass"})
               .Define("Jet_SMW1_matched",::dRmatching_binary,{"GenPart_SMW1_idx","drmax2","GenPart_pt","GenPart_eta","GenPart_phi","GenPart_mass","Jet_pt","Jet_eta","Jet_phi","Jet_mass"})
               .Define("Jet_SMW2_matched",::dRmatching_binary,{"GenPart_SMW2_idx","drmax2", "GenPart_pt","GenPart_eta","GenPart_phi","GenPart_mass","Jet_pt","Jet_eta","Jet_phi","Jet_mass"})
               .Define("Sel_muon_matched","Muon_matched[muoncuts]")
               .Define("Sel_tau_matched","Tau_matched[seltaucuts]")
               .Define("Sel2_LFVupjet_matched","Jet_LFVup_matched[jetcuts][muonjetoverlap][taujetoverlap]")
               .Define("Sel2_SMbjet_matched","Jet_SMb_matched[jetcuts][muonjetoverlap][taujetoverlap]")
               .Define("Sel2_SMW1jet_matched","Jet_SMW1_matched[jetcuts][muonjetoverlap][taujetoverlap]")
               .Define("Sel2_SMW2jet_matched","Jet_SMW2_matched[jetcuts][muonjetoverlap][taujetoverlap]")
               .Define("Sel_muonmatched_idx","ArgMax(Sel_muon_matched)")
               .Define("Sel_taumatched_idx","ArgMax(Sel_tau_matched)")
               .Define("Sel2_LFVupjet_matched_idx","ArgMax(Sel2_LFVupjet_matched)")
               .Define("Sel2_SMbjet_matched_idx","ArgMax(Sel2_SMbjet_matched)")
               .Define("Sel2_SMW1jet_matched_idx","ArgMax(Sel2_SMW1jet_matched)")
               .Define("Sel2_SMW2jet_matched_idx","ArgMax(Sel2_SMW2jet_matched)")
               .Define("nmuonmatched", "Sum(Muon_matched)")
               .Define("ntaumatched", "Sum(Tau_matched)")
               .Define("nJet_LFVup_matched", "Sum(Jet_LFVup_matched)")
               .Define("nJet_SMb_matched", "Sum(Jet_SMb_matched)")
               .Define("nJet_SMW1_matched", "Sum(Jet_SMW1_matched)")
               .Define("nJet_SMW2_matched", "Sum(Jet_SMW2_matched)");
}
/*
void NanoAODAnalyzerrdframe::selectFatJets() {

    _rlm = _rlm.Define("fatjetcuts", "FatJet_pt>400.0 && abs(FatJet_eta)<2.4 && FatJet_tau1>0.0 && FatJet_tau2>0.0 && FatJet_tau3>0.0 && FatJet_tau3/FatJet_tau2<0.5")
               .Define("Sel_fatjetpt", "FatJet_pt[fatjetcuts]")
               .Define("Sel_fatjeteta", "FatJet_eta[fatjetcuts]")
               .Define("Sel_fatjetphi", "FatJet_phi[fatjetcuts]")
               .Define("Sel_fatjetmass", "FatJet_mass[fatjetcuts]")
               .Define("nfatjetspass", "int(Sel_fatjetpt.size())")
               //.Define("Sel_fatjetweight", "std::vector<double>(nfatjetspass, evWeight)")
               .Define("Sel_fatjet4vecs", ::gen4vec, {"Sel_fatjetpt", "Sel_fatjeteta", "Sel_fatjetphi", "Sel_fatjetmass"});
}
*/
void NanoAODAnalyzerrdframe::topPtReweight() {

    _rlm = _rlm.Define("gentopcut", "abs(GenPart_pdgId) == 6 && GenPart_statusFlags & (1 << 13)")
               .Define("GenPart_top_pt", "GenPart_pt[gentopcut]");

    // To be updated for nanoaod n9
    auto topPtLOtoNLO = [](floats toppt)->float {

        float out = 1.0;
        std::vector<float> xbins = {0,50,100,150,200,250,300,350,400,450,500,550,600,800,1000,2000};
        std::vector<float> sfs = {1.1536, 1.0578, 0.993, 0.9373, 0.8881, 0.8456, 0.8087, 0.7809, 0.7559, 0.7388, 0.7247, 0.7245, 0.7124, 0.7284, 0.7317};

        if (toppt.size() != 2) out = 1.0;
        else {
            float pt1 = toppt[0];
            float pt2 = toppt[1];
            int xbin1 = (std::upper_bound(xbins.begin(), xbins.end(), pt1)-1) - xbins.begin();
            int xbin2 = (std::upper_bound(xbins.begin(), xbins.end(), pt2)-1) - xbins.begin();
            out = std::sqrt( sfs.at(xbin1) * sfs.at(xbin2));
        }
        return out;
    };

    auto topPtNLOtoNNLO = [](floats toppt)->float {

        float out = 1.0;
        if (toppt.size() != 2) out = 1.0;
        else {
            float pt1 = toppt[0];
            float pt2 = toppt[1];
            out = std::sqrt( (0.103 * std::exp(-0.0118 * pt1) - 0.000134 * pt1 + 0.973)\
                           * (0.103 * std::exp(-0.0118 * pt2) - 0.000134 * pt2 + 0.973));
        }
        return out;
    };

    // NLO ttbar: NLO to theory weight
    // https://twiki.cern.ch/twiki/bin/view/CMS/TopPtReweighting#TOP_PAG_corrections_based_on_the
    if (_outfilename.find("TTTo") != std::string::npos) {
        _rlm = _rlm.Define("TopPtWeight", topPtNLOtoNNLO, {"GenPart_top_pt"});
    } else if (_outfilename.find("TT_LFV") != std::string::npos) {
        _rlm = _rlm.Define("TopPtWeight_LO", topPtLOtoNLO, {"GenPart_top_pt"})
                   .Define("TopPtWeight_NLO", topPtNLOtoNNLO, {"GenPart_top_pt"})
                   .Define("TopPtWeight", "TopPtWeight_LO * TopPtWeight_NLO");
    } else {
        _rlm = _rlm.Define("TopPtWeight", "one");
    }

}

void NanoAODAnalyzerrdframe::calculateEvWeight() {

    // Put SFs that need to be calculated in skim:
    // e.g. tau selection is done at processing but we want to have SFs already

	// Electron SF
	cout << "Loading Electron SF" << endl;
	std::string eleid_ = 
	std::string eleid_ =
	std::string eleid_ = 

	cout << "Electron ID WP" << endl;


	auto eleFile = correction::CorrectionSet::from_file("/home/juhee5819/cheer/nanoaodrdframe/data/ElectronSF/"+eleYear+"electron.json.gz");
	auto Map = eleFile->at("UL-Electron-ID-SF");
	auto sf = Map->evaluate({eleYear, "sf", eleID})
    // Tau SF
    cout << "Loading Tau SF" << endl;
    std::string tauid_vse = "VLoose";
    std::string tauid_vsmu = "Tight";
    std::string tauid_vsjet = "VTight";

    cout << "Tau ID WP vsJet : " << tauid_vsjet << endl;
    cout << "Tau ID WP vsMuon : " << tauid_vsmu << endl;
    cout << "Tau ID WP vsElectron : " << tauid_vse << endl;

    TauIDSFTool* _tauidSFjet;
    TauIDSFTool* _tauidSFele;
    TauIDSFTool* _tauidSFmu;
    TauESTool* _testool;
    TauFESTool* _festool;
    std::string tauYear = "";

    if (_isRun16pre) {
        tauYear = "UL2016_preVFP";
    } else if (_isRun16post) {
        tauYear = "UL2016_postVFP";
    } else {
        tauYear = "UL" + _year;
    }
    _tauidSFjet = new TauIDSFTool(tauYear, "DeepTau2017v2p1VSjet", tauid_vsjet);
    _tauidSFele = new TauIDSFTool(tauYear, "DeepTau2017v2p1VSe", tauid_vse);
    _tauidSFmu = new TauIDSFTool(tauYear, "DeepTau2017v2p1VSmu", tauid_vsmu);
    _testool = new TauESTool(tauYear, "DeepTau2017v2p1VSjet");
    _festool = new TauFESTool(tauYear);

    auto tauSFIdVsJet = [this, _tauidSFjet](floats &pt, floats &eta, uchars &genid)->floatsVec {

        floats uncSources;
        uncSources.reserve(3);
        floatsVec wVec;
        wVec.reserve(pt.size());

        if (pt.size() > 0) {
            for (unsigned int i=0; i<pt.size(); i++) {
                uncSources.emplace_back(_tauidSFjet->getSFvsPT(pt[i], int(genid[i])));
                uncSources.emplace_back(_tauidSFjet->getSFvsPT(pt[i], int(genid[i]), "Up"));
                uncSources.emplace_back(_tauidSFjet->getSFvsPT(pt[i], int(genid[i]), "Down"));
                wVec.emplace_back(uncSources);
                uncSources.clear();
            }
        }
        return wVec;
    };

    auto tauSFIdVsEl = [this, _tauidSFele](floats &pt, floats &eta, uchars &genid)->floatsVec {

        floats uncSources;
        uncSources.reserve(3);
        floatsVec wVec;
        wVec.reserve(pt.size());

        if (pt.size() > 0) {
            for (unsigned int i=0; i<pt.size(); i++) {
                uncSources.emplace_back(_tauidSFele->getSFvsEta(abs(eta[i]), int(genid[i])));
                uncSources.emplace_back(_tauidSFele->getSFvsEta(abs(eta[i]), int(genid[i]), "Up"));
                uncSources.emplace_back(_tauidSFele->getSFvsEta(abs(eta[i]), int(genid[i]), "Down"));
                wVec.emplace_back(uncSources);
                uncSources.clear();
            }
        }
        return wVec;
    };

    auto tauSFIdVsMu = [this, _tauidSFmu](floats &pt, floats &eta, uchars &genid)->floatsVec {

        floats uncSources;
        uncSources.reserve(3);
        floatsVec wVec;
        wVec.reserve(pt.size());

        if (pt.size() > 0) {
            for (unsigned int i=0; i<pt.size(); i++) {
                uncSources.emplace_back(_tauidSFmu->getSFvsEta(abs(eta[i]), int(genid[i])));
                uncSources.emplace_back(_tauidSFmu->getSFvsEta(abs(eta[i]), int(genid[i]), "Up"));
                uncSources.emplace_back(_tauidSFmu->getSFvsEta(abs(eta[i]), int(genid[i]), "Down"));
                wVec.emplace_back(uncSources);
                uncSources.clear();
            }
        }
        return wVec;
    };

    _rlm = _rlm.Define("tauWeightIdVsJet", tauSFIdVsJet, {"Tau_pt","Tau_eta","Tau_genPartFlav"})
               .Define("tauWeightIdVsEl", tauSFIdVsEl, {"Tau_pt","Tau_eta","Tau_genPartFlav"})
               .Define("tauWeightIdVsMu", tauSFIdVsMu, {"Tau_pt","Tau_eta","Tau_genPartFlav"});

    // Tau ES
    cout<<"Applying TauES on Genuine taus"<<endl;
    auto tauES = [this, _testool, _festool](floats &pt, floats &eta, ints &dm, uchars &genid, floats &x)->floats {

        floats xout;

        for (unsigned int i=0; i<pt.size(); i++) {
            float es = 1.0;
            if (genid[i]==5) {
                es = _testool->getTES(pt[i], dm[i], genid[i]);
            } else if (genid[i]==1 || genid[i]==3){
                es = _festool->getFES(eta[i], dm[i], genid[i]);
            }
            xout.emplace_back(x[i]*es);
        }
        return xout;
    };

    // return SF for pT and mass
    auto tauESUnc = [this, _testool, _festool](floats &pt, floats &eta, ints &dm, uchars &genid, floats &x)->floatsVec {

        floats uncSources;
        uncSources.reserve(2);
        floatsVec xout;
        xout.reserve(pt.size());

        for (unsigned int i=0; i<pt.size(); i++) {
            if (genid[i]==5) {
                uncSources.emplace_back(_testool->getTES(pt[i], dm[i], genid[i], "Up"));
                uncSources.emplace_back(_testool->getTES(pt[i], dm[i], genid[i], "Down"));
            } else if (genid[i]==1 || genid[i]==3){
                uncSources.emplace_back(_festool->getFES(eta[i], dm[i], genid[i], "Up"));
                uncSources.emplace_back(_festool->getFES(eta[i], dm[i], genid[i], "Down"));
            }
            else uncSources = {1.0f, 1.0f};
            xout.emplace_back(uncSources);
            uncSources.clear();
        }
        return xout;
    };

    _rlm = _rlm.Define("Tau_pt_uncor", "Tau_pt")
               .Redefine("Tau_pt", tauES, {"Tau_pt_uncor", "Tau_eta", "Tau_decayMode", "Tau_genPartFlav", "Tau_pt_uncor"})
               .Redefine("Tau_mass", tauES, {"Tau_pt_uncor", "Tau_eta", "Tau_decayMode", "Tau_genPartFlav", "Tau_mass"})
               .Define("Tau_pt_unc", tauESUnc, {"Tau_pt_uncor", "Tau_eta", "Tau_decayMode", "Tau_genPartFlav", "Tau_pt_uncor"});
}

/*
bool NanoAODAnalyzerrdframe::helper_1DHistCreator(std::string hname, std::string title, const int nbins, const double xlow, const double xhi, std::string rdfvar, std::string evWeight)
{
	RDF1DHist histojets = _rlm.Histo1D({hname.c_str(), title.c_str(), nbins, xlow, xhi}, rdfvar, evWeight); // Fill with weight given by evWeight
	_th1dhistos[hname] = histojets;
}
*/

void NanoAODAnalyzerrdframe::helper_1DHistCreator(std::string hname, std::string title, const int nbins, const double xlow, const double xhi, std::string rdfvar, std::string evWeight, RNode *anode) {

	RDF1DHist histojets = anode->Histo1D({hname.c_str(), title.c_str(), nbins, xlow, xhi}, rdfvar, evWeight); // Fill with weight given by evWeight
	_th1dhistos[hname] = histojets;
}

// Automatically loop to create
void NanoAODAnalyzerrdframe::setupCuts_and_Hists() {

    cout << "setting up definitions, cuts, and histograms" <<endl;

    for ( auto &c : _varinfovector) {
        if (c.mincutstep.length()==0) _rlm = _rlm.Define(c.varname, c.vardefinition);
    }

    for (auto &x : _hist1dinfovector) {
        std::string hpost = "";

        if (x.mincutstep.length()==0) {
            helper_1DHistCreator(std::string(x.hmodel.fName)+hpost+x.systname,  std::string(x.hmodel.fTitle), x.hmodel.fNbinsX, x.hmodel.fXLow, x.hmodel.fXUp, x.varname, x.weightname+x.systname, &_rlm);
        }
    }

    _rnt.setRNode(&_rlm);

    for (auto acut : _cutinfovector) {
        //std::string cutname = "S" + to_string(acut.idx.length()-1);
        std::string cutname = "S" + to_string(acut.idx.length());
        std::string hpost = "_"+cutname;
        RNode *r = _rnt.getParent(acut.idx)->getRNode();
        auto rnext = new RNode(r->Define(cutname, acut.cutdefinition));
        *rnext = rnext->Filter(cutname);

        for ( auto &c : _varinfovector) {
            if (acut.idx.compare(c.mincutstep)==0) *rnext = rnext->Define(c.varname, c.vardefinition);
        }
        for (auto &x : _hist1dinfovector) {
            if (acut.idx.compare(0, x.mincutstep.length(), x.mincutstep)==0) {
                bool reachedMax = false;
                if (x.maxcutstep.length() > 0 and acut.idx.compare(0, x.maxcutstep.length(), x.maxcutstep)>=0) reachedMax = true;
                if (!reachedMax)
                    helper_1DHistCreator(std::string(x.hmodel.fName)+hpost+x.systname,  std::string(x.hmodel.fTitle), x.hmodel.fNbinsX, x.hmodel.fXLow, x.hmodel.fXUp, x.varname, x.weightname+x.systname, rnext);
            }
        }
        _rnt.addDaughter(rnext, acut.idx);

        /*
        _rlm = _rlm.Define(cutname, acut.cutdefinition);
        _rlm = _rlm.Filter(cutname);

        for ( auto &c : _varinfovector) {
            if (acut.idx.compare(c.mincutstep)==0) _rlm = _rlm.Define(c.varname, c.vardefinition);
        }
        for (auto &x : _hist1dinfovector) {
            if (acut.idx.compare(0, x.mincutstep.length(), x.mincutstep)==0) {
                helper_1DHistCreator(std::string(x.hmodel.fName)+hpost,  std::string(x.hmodel.fTitle)+hpost, x.hmodel.fNbinsX, x.hmodel.fXLow, x.hmodel.fXUp, x.varname, x.weightname);
            }
        }
        _rnt.addDaughter(&_rlm, acut.idx);
        */
    }
}

void NanoAODAnalyzerrdframe::add1DHist(TH1DModel histdef, std::string variable, std::string weight, string syst, string mincutstep, string maxcutstep) {

	_hist1dinfovector.push_back({histdef, variable, weight, syst, mincutstep, maxcutstep});
}


void NanoAODAnalyzerrdframe::drawHists(RNode t) {

	cout << "processing" <<endl;
	t.Count();
}

void NanoAODAnalyzerrdframe::addVar(varinfo v) {

	_varinfovector.push_back(v);
}

void NanoAODAnalyzerrdframe::addVartoStore(string varname) {

    // varname is assumed to be a regular expression.
    // e.g. if varname is "Muon_eta" then "Muon_eta" will be stored
    // if varname=="Muon_.*", then any branch name that starts with "Muon_" string will
    // be saved
    _varstostore.push_back(varname);
    /*
    std::regex b(varname);
    bool foundmatch = false;
    for (auto a: _rlm.GetColumnNames()) {
        if (std::regex_match(a, b)) {
            _varstostore.push_back(a);
            foundmatch = true;
        }
    }
    */

}

void NanoAODAnalyzerrdframe::setupTree() {

    vector<RNodeTree *> rntends;
    _rnt.getRNodeLeafs(rntends);
    for (auto arnt: rntends) {
        RNode *arnode = arnt->getRNode();
        string nodename = arnt->getIndex();
        vector<string> varforthistree;
        std::map<string, int> varused;

        for (auto varname: _varstostore) {
            bool foundmatch = false;
            std::regex b(varname);
            for (auto a: arnode->GetColumnNames()) {
                if (std::regex_match(a, b) && varused[a]==0) {
                    varforthistree.push_back(a);
                    varused[a]++;
                    foundmatch = true;
                }
            }
            if (!foundmatch) {
                cout << varname << " not found at "<< nodename << endl;
            }
        }
        _varstostorepertree[nodename]  = varforthistree;
    }
}

void NanoAODAnalyzerrdframe::addCuts(string cut, string idx) {

	_cutinfovector.push_back({cut, idx});
}


void NanoAODAnalyzerrdframe::run(bool saveAll, string outtreename) {

    /*
    if (saveAll) {
        _rlm.Snapshot(outtreename, _outfilename);
    }
    else {
        // use the following if you want to store only a few variables
        _rlm.Snapshot(outtreename, _outfilename, _varstostore);
    }
    */

    vector<RNodeTree *> rntends;
    _rnt.getRNodeLeafs(rntends);
    _rnt.Print();
    cout << rntends.size() << endl;
    // on master, regex_replace doesn't work somehow
    //std::regex rootextension("\\.root");

    for (auto arnt: rntends) {
        string nodename = arnt->getIndex();
        //string outname = std::regex_replace(_outfilename, rootextension, "_"+nodename+".root");
        string outname = _outfilename;

        // if producing many root files due to branched selection criteria,  each root file will get a different name
        if (rntends.size()>1) outname.replace(outname.find(".root"), 5, "_"+nodename+".root");
        _outrootfilenames.push_back(outname);
        RNode *arnode = arnt->getRNode();
        cout << arnt->getIndex();
        //cout << ROOT::RDF::SaveGraph(_rlm) << endl;

        if (saveAll) {
            arnode->Snapshot(outtreename, outname);
        } else {
            // use the following if you want to store only a few variables
            //arnode->Snapshot(outtreename, outname, _varstostore);
            cout << " writing branches" << endl;
            for (auto bname: _varstostorepertree[nodename]) {
                cout << bname << ", ";
            }
            cout<<endl;
            arnode->Snapshot(outtreename, outname, _varstostorepertree[nodename]);
        }
        _outrootfile = new TFile(outname.c_str(),"UPDATE");
        for (auto &h : _th1dhistos) {
            if (h.second.GetPtr() != nullptr) {
                h.second.GetPtr()->Print();
                h.second.GetPtr()->Write();
                //std::cout<<h.second->GetName()<<std::endl;
                //h.second->Write();
                //std::cout<<"Histogram is written"<<std::endl;
            }
        }

        TH1F* hPDFWeights = new TH1F("LHEPdfWeightSum", "LHEPdfWeightSum", 103, 0, 103);
        for (size_t i=0; i<PDFWeights.size(); i++)
            hPDFWeights->SetBinContent(i+1, PDFWeights[i]);

        _outrootfile->Write(0, TObject::kOverwrite);
        _outrootfile->Close();
    }
}
