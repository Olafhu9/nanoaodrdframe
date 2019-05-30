/*
 * NanoAODAnalyzerrdframe.cpp
 *
 *  Created on: Sep 30, 2018
 *      Author: suyong
 */

#include "NanoAODAnalyzerrdframe.h"
#include <iostream>
#include <algorithm>
#include <typeinfo>

#include "TCanvas.h"
#include "Math/GenVector/VectorUtil.h"
#include <vector>
#include <fstream>
#include "utility.h"
#include <regex>
#include "ROOT/RDFHelpers.hxx"


using namespace std;

NanoAODAnalyzerrdframe::NanoAODAnalyzerrdframe(TTree *atree, std::string outfilename, std::string jsonfname, std::string globaltag, int nthreads)
:_rd(*atree),_jsonOK(false), _outfilename(outfilename), _jsonfname(jsonfname), _globaltag(globaltag), _inrootfile(0),_outrootfile(0), _rlm(_rd)
	, _btagcalib("CSVv2", "data/CSVv2_94XSF_V2_B_F.csv")
	, _btagcalib2("DeepCSV", "data/DeepCSV_94XSF_V4_B_F.csv")
	, _btagcalibreader(BTagEntry::OP_RESHAPING, "central", {"up_jes", "down_jes"})
    , _btagcalibreader2(BTagEntry::OP_RESHAPING, "central", {"up_jes", "down_jes"})
	, _rnt(&_rlm), currentnode(0), _jetCorrector(0), _jetCorrectionUncertainty(0)
{
	//
	// if genWeight column exists, then it is not real data
	//
	if (atree->GetBranch("genWeight") == nullptr) {
		_isData = true;
		cout << "input file is data" <<endl;
	}
	else
	{
		_isData = false;
		cout << "input file is MC" <<endl;
	}
	TObjArray *allbranches = atree->GetListOfBranches();
	for (int i =0; i<allbranches->GetSize(); i++)
	{
		TBranch *abranch = dynamic_cast<TBranch *>(allbranches->At(i));
		if (abranch!= nullptr){
			cout << abranch->GetName() << endl;
			_originalvars.push_back(abranch->GetName());
		}
	}

	// load the formulae b flavor tagging
	_btagcalibreader.load(_btagcalib, BTagEntry::FLAV_B, "iterativefit");
	_btagcalibreader.load(_btagcalib, BTagEntry::FLAV_C, "iterativefit");
	_btagcalibreader.load(_btagcalib, BTagEntry::FLAV_UDSG, "iterativefit");

	_btagcalibreader2.load(_btagcalib2, BTagEntry::FLAV_B, "iterativefit");
	_btagcalibreader2.load(_btagcalib2, BTagEntry::FLAV_C, "iterativefit");
	_btagcalibreader2.load(_btagcalib2, BTagEntry::FLAV_UDSG, "iterativefit");

	//
	// pu weight setup
	//
	//TFile tfmc("data/pileup_profile_Summer16.root"); // for 2016 data
	TFile tfmc("data/PileupMC2017v4.root");
	_hpumc = dynamic_cast<TH1 *>(tfmc.Get("pu_mc"));
	_hpumc->SetDirectory(0);
	tfmc.Close();

	//TFile tfdata("data/PileupData_GoldenJSON_Full2016.root"); // for 2016 data
	TFile tfdata("data/pileup_Cert_294927-306462_13TeV_PromptReco_Collisions17_withVar.root");
	_hpudata = dynamic_cast<TH1 *>(tfdata.Get("pileup"));
	_hpudata_plus = dynamic_cast<TH1 *>(tfdata.Get("pileup_plus"));
	_hpudata_minus = dynamic_cast<TH1 *>(tfdata.Get("pileup_minus"));

	_hpudata->SetDirectory(0);
	_hpudata_plus->SetDirectory(0);
	_hpudata_minus->SetDirectory(0);
	tfdata.Close();

	_puweightcalc = new WeightCalculatorFromHistogram(_hpumc, _hpudata);
	_puweightcalc_plus = new WeightCalculatorFromHistogram(_hpumc, _hpudata_plus);
	_puweightcalc_minus = new WeightCalculatorFromHistogram(_hpumc, _hpudata_minus);

	setupJetMETCorrection(_globaltag);
	/*
	 *  The following doesn't work yet...
	if (nthreads>1)
	{
		ROOT::EnableImplicitMT(nthreads);
	}
	*/
}

NanoAODAnalyzerrdframe::~NanoAODAnalyzerrdframe() {
	// TODO Auto-generated destructor stub
	// ugly...

	cout << "writing histograms" << endl;

	for (auto afile:_outrootfilenames)
	{
		_outrootfile = new TFile(afile.c_str(), "UPDATE");
		for (auto &h : _th1dhistos)
		{
			if (h.second.GetPtr() != nullptr) h.second->Write();
		}

		_outrootfile->Write(0, TObject::kOverwrite);
		_outrootfile->Close();
		delete _outrootfile;
	}
}

bool NanoAODAnalyzerrdframe::isDefined(string v)
{
	auto result = std::find(_originalvars.begin(), _originalvars.end(), v);
	if (result != _originalvars.end()) return true;
	else return false;
}

void NanoAODAnalyzerrdframe::setTree(TTree *t, std::string outfilename)
{
	_rd = ROOT::RDataFrame(*t);
	_rlm = RNode(_rd);
	_outfilename = outfilename;
	_hist1dinfovector.clear();
	_th1dhistos.clear();
	_varstostore.clear();
	_hist1dinfovector.clear();
	_selections.clear();

	this->setupAnalysis();
}

void NanoAODAnalyzerrdframe::setupAnalysis()
{
	/* Must sequentially define objects.
	 *
	 */

	if (_isData) _jsonOK = readjson();
	// Event weight for data it's always one. For MC, it depends on the sign

	_rlm = _rlm.Define("one", "1.0");
	if (_isData && !isDefined("evWeight"))
	{
		_rlm = _rlm.Define("evWeight", [](){
				return 1.0;
			}, {} );
	}
	else if (!_isData) {
		if (!isDefined("puWeight")) _rlm = _rlm.Define("puWeight",
				[this](float x) {
					return _puweightcalc->getWeight(x);
				}, {"Pileup_nTrueInt"}
			);
		if (!isDefined("puWeight_plus")) _rlm = _rlm.Define("puWeight_plus",
				[this](float x) {
					return _puweightcalc_plus->getWeight(x);
				}, {"Pileup_nTrueInt"}
			);
		if (!isDefined("puWeight_minus")) _rlm = _rlm.Define("puWeight_minus",
				[this](float x) {
					return _puweightcalc_minus->getWeight(x);
				}, {"Pileup_nTrueInt"}
			);
		if (!isDefined("pugenWeight")) _rlm = _rlm.Define("pugenWeight", [this](float x, float y){
					return (x > 0 ? 1.0 : -1.0) *y;
				}, {"genWeight", "puWeight"});
	}

	// Object selection will be defined in sequence.
	// Selected objects will be stored in new vectors.
	selectElectrons();
	selectMuons();
	applyJetMETCorrections();
	selectJets();
	removeOverlaps();
	selectFatJets();
	defineMoreVars();
	defineCuts();
	bookHists();
	setupCuts_and_Hists();
	setupTree();
}

bool NanoAODAnalyzerrdframe::readjson()
{
	auto isgoodjsonevent = [this](unsigned int runnumber, unsigned int lumisection)
		{
			auto key = std::to_string(runnumber);

			bool goodeventflag = false;

			if (jsonroot.isMember(key))
			{
				Json::Value runlumiblocks = jsonroot[key];
				for (unsigned int i=0; i<runlumiblocks.size() && !goodeventflag; i++)
				{
					auto lumirange = runlumiblocks[i];
					if (lumisection >= lumirange[0].asUInt() && lumisection <= lumirange[1].asUInt()) goodeventflag = true;
				}
				return goodeventflag;
			}
			else
			{
				//cout << "Run not in json " << runnumber << endl;
				return false;
			}

		};

	if (_jsonfname != "")
	{
		std::ifstream jsoninfile;
		jsoninfile.open(_jsonfname);

		if (jsoninfile.good())
		{
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
		}
		else
		{
			cout << "Problem reading json file " << _jsonfname << endl;
			return false;
		}
	}
	else
	{
		cout << "no JSON file given" << endl;
		return true;
	}
}

void NanoAODAnalyzerrdframe::setupJetMETCorrection(string globaltag, string jetalgo)
{
	if (globaltag != "")
	{
		cout << "Applying new JetMET corrections. GT: "+globaltag+" on jetAlgo: "+jetalgo << endl;
		string basedirectory = "data/jme/";

		string datamcflag = "";
		if (_isData) datamcflag = "DATA";
		else datamcflag = "MC";

		// set file names that contain the parameters for corrections
		string dbfilenamel1 = basedirectory+globaltag+"_"+datamcflag+"_L1FastJet_"+jetalgo+".txt";
		string dbfilenamel2 = basedirectory+globaltag+"_"+datamcflag+"_L2Relative_"+jetalgo+".txt";
		string dbfilenamel3 = basedirectory+globaltag+"_"+datamcflag+"_L3Absolute_"+jetalgo+".txt";
		string dbfilenamel2l3 = basedirectory+globaltag+"_"+datamcflag+"_L2L3Residual_"+jetalgo+".txt";

		JetCorrectorParameters *L1JetCorrPar = new JetCorrectorParameters(dbfilenamel1);
        if (!L1JetCorrPar->isValid())
        {
            std::cerr << "L1FastJet correction parameters not read" << std::endl;
            exit(1);
        }
		JetCorrectorParameters *L2JetCorrPar = new JetCorrectorParameters(dbfilenamel2);
        if (!L2JetCorrPar->isValid())
        {
            std::cerr << "L2Relative correction parameters not read" << std::endl;
            exit(1);
        }
		JetCorrectorParameters *L3JetCorrPar = new JetCorrectorParameters(dbfilenamel3);
        if (!L3JetCorrPar->isValid())
        {
            std::cerr << "L3Absolute correction parameters not read" << std::endl;
            exit(1);
        }
		JetCorrectorParameters *L2L3JetCorrPar = new JetCorrectorParameters(dbfilenamel2l3);
        if (!L2L3JetCorrPar->isValid())
        {
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
		string dbfilenameunc = basedirectory+globaltag+"_"+datamcflag+"_Uncertainty_"+jetalgo+".txt";
		_jetCorrectionUncertainty = new JetCorrectionUncertainty(dbfilenameunc);
	}
	else
	{
		cout << "Not applying new JetMET corrections. Using NanoAOD as is." << endl;
	}
}

void NanoAODAnalyzerrdframe::selectElectrons()
{
	//cout << "select electrons" << endl;
	_rlm = _rlm.Define("elecuts", "Electron_pt>30.0 && abs(Electron_eta)<2.4 && Electron_pfRelIso03_all<0.15");
	_rlm = _rlm.Define("Sel_elept", "Electron_pt[elecuts]") // define new variables
			.Define("Sel_eleta", "Electron_eta[elecuts]")
			.Define("Sel_elephi", "Electron_phi[elecuts]")
			.Define("Sel_elemass", "Electron_mass[elecuts]")
			.Define("nelepass", "int(Sel_elept.size())");

	_rlm = _rlm.Define("ele4vecs", ::gen4vec, {"Sel_elept", "Sel_eleta", "Sel_elephi", "Sel_elemass"});
}

void NanoAODAnalyzerrdframe::selectMuons()
{
	//cout << "select muons" << endl;
	_rlm = _rlm.Define("muoncuts", "Muon_pt>30.0 && abs(Muon_eta)<2.4 && Muon_pfRelIso03_all<0.15");
	_rlm = _rlm.Define("Sel_muonpt", "Muon_pt[muoncuts]") // define new variables
			.Define("Sel_muoneta", "Muon_eta[muoncuts]")
			.Define("Sel_muonphi", "Muon_phi[muoncuts]")
			.Define("Sel_muonmass", "Muon_mass[muoncuts]")
			.Define("nmuonpass", "int(Sel_muonpt.size())");
	_rlm = _rlm.Define("muon4vecs", ::gen4vec, {"Sel_muonpt", "Sel_muoneta", "Sel_muonphi", "Sel_muonmass"});
}

// Adapted from https://github.com/cms-nanoAOD/nanoAOD-tools/blob/master/python/postprocessing/modules/jme/jetRecalib.py
// and https://github.com/cms-nanoAOD/nanoAOD-tools/blob/master/python/postprocessing/modules/jme/JetRecalibrator.py
void NanoAODAnalyzerrdframe::applyJetMETCorrections()
{
	auto appcorrlambdaf = [this](floats jetpts, floats jetetas, floats jetAreas, floats jetrawf, float rho)->floats
	{
		floats corrfactors;
		corrfactors.reserve(jetpts.size());
		for (auto i =0; i<jetpts.size(); i++)
		{
			float rawjetpt = jetpts[i]*(1.0-jetrawf[i]);
			_jetCorrector->setJetPt(rawjetpt);
			_jetCorrector->setJetEta(jetetas[i]);
			_jetCorrector->setJetA(jetAreas[i]);
			_jetCorrector->setRho(rho);
			float corrfactor = _jetCorrector->getCorrection();
			corrfactors.emplace_back(rawjetpt * corrfactor);

		}
		return corrfactors;
	};

	auto jecuncertaintylambdaf= [this](floats jetpts, floats jetetas, floats jetAreas, floats jetrawf, float rho)->floats
		{
			floats uncertainties;
			uncertainties.reserve(jetpts.size());
			for (auto i =0; i<jetpts.size(); i++)
			{
				float rawjetpt = jetpts[i]*(1.0-jetrawf[i]);
				_jetCorrector->setJetPt(rawjetpt);
				_jetCorrector->setJetEta(jetetas[i]);
				_jetCorrector->setJetA(jetAreas[i]);
				_jetCorrector->setRho(rho);
				float corrfactor = _jetCorrector->getCorrection();
				_jetCorrectionUncertainty->setJetPt(corrfactor*rawjetpt);
				_jetCorrectionUncertainty->setJetEta(jetetas[i]);
				float unc = _jetCorrectionUncertainty->getUncertainty(true);
				uncertainties.emplace_back(unc);

			}
			return uncertainties;
		};

	auto metcorrlambdaf = [](float met, float metphi, floats jetptsbefore, floats jetptsafter, floats jetphis)->float
	{
		auto metx = met * cos(metphi);
		auto mety = met * sin(metphi);
		for (auto i=0; i<jetphis.size(); i++)
		{
			if (jetptsafter[i]>15.0)
			{
				metx -= (jetptsafter[i] - jetptsbefore[i])*cos(jetphis[i]);
				mety -= (jetptsafter[i] - jetptsbefore[i])*sin(jetphis[i]);
			}
		}
		return float(sqrt(metx*metx + mety*mety));
	};

	auto metphicorrlambdaf = [](float met, float metphi, floats jetptsbefore, floats jetptsafter, floats jetphis)->float
	{
		auto metx = met * cos(metphi);
		auto mety = met * sin(metphi);
		for (auto i=0; i<jetphis.size(); i++)
		{
			if (jetptsafter[i]>15.0)
			{
				metx -= (jetptsafter[i] - jetptsbefore[i])*cos(jetphis[i]);
				mety -= (jetptsafter[i] - jetptsbefore[i])*sin(jetphis[i]);
			}
		}
		return float(atan2(mety, metx));
	};

	if (_jetCorrector != 0)
	{
		_rlm = _rlm.Define("Jet_pt_corr", appcorrlambdaf, {"Jet_pt", "Jet_eta", "Jet_area", "Jet_rawFactor", "fixedGridRhoFastjetAll"});
		_rlm = _rlm.Define("Jet_pt_relerror", jecuncertaintylambdaf, {"Jet_pt", "Jet_eta", "Jet_area", "Jet_rawFactor", "fixedGridRhoFastjetAll"});
		_rlm = _rlm.Define("Jet_pt_corr_up", "Jet_pt_corr*(1.0f + Jet_pt_relerror)");
		_rlm = _rlm.Define("Jet_pt_corr_down", "Jet_pt_corr*(1.0f - Jet_pt_relerror)");
		_rlm = _rlm.Define("MET_pt_corr", metcorrlambdaf, {"MET_pt", "MET_phi", "Jet_pt", "Jet_pt_corr", "Jet_phi"});
		_rlm = _rlm.Define("MET_phi_corr", metphicorrlambdaf, {"MET_pt", "MET_phi", "Jet_pt", "Jet_pt_corr", "Jet_phi"});
		_rlm = _rlm.Define("MET_pt_corr_up", metcorrlambdaf, {"MET_pt", "MET_phi", "Jet_pt", "Jet_pt_corr_up", "Jet_phi"});
		_rlm = _rlm.Define("MET_phi_corr_up", metphicorrlambdaf, {"MET_pt", "MET_phi", "Jet_pt", "Jet_pt_corr_up", "Jet_phi"});
		_rlm = _rlm.Define("MET_pt_corr_down", metcorrlambdaf, {"MET_pt", "MET_phi", "Jet_pt", "Jet_pt_corr_down", "Jet_phi"});
		_rlm = _rlm.Define("MET_phi_corr_down", metphicorrlambdaf, {"MET_pt", "MET_phi", "Jet_pt", "Jet_pt_corr_down", "Jet_phi"});
	}
}

void NanoAODAnalyzerrdframe::selectJets()
{
	// apparently size() returns long int, which ROOT doesn't recognized for branch types
	// , so it must be cast into int if you want to save them later into a TTree
	_rlm = _rlm.Define("jetcuts", "Jet_pt>30.0 && abs(Jet_eta)<2.4 && Jet_jetId>0")
			.Define("Sel_jetpt", "Jet_pt[jetcuts]")
			.Define("Sel_jeteta", "Jet_eta[jetcuts]")
			.Define("Sel_jetphi", "Jet_phi[jetcuts]")
			.Define("Sel_jetmass", "Jet_mass[jetcuts]")
			.Define("Sel_jetbtag", "Jet_btagCSVV2[jetcuts]")
			.Define("njetspass", "int(Sel_jetpt.size())")
			.Define("jet4vecs", ::gen4vec, {"Sel_jetpt", "Sel_jeteta", "Sel_jetphi", "Sel_jetmass"});


	_rlm = _rlm.Define("btagcuts", "Sel_jetbtag>0.8")
			.Define("Sel_bjetpt", "Sel_jetpt[btagcuts]")
			.Define("Sel_bjeteta", "Sel_jeteta[btagcuts]")
			.Define("Sel_bjetphi", "Sel_jetphi[btagcuts]")
			.Define("Sel_bjetm", "Sel_jetmass[btagcuts]")
			.Define("bnjetspass", "int(Sel_bjetpt.size())")
			.Define("bjet4vecs", ::gen4vec, {"Sel_bjetpt", "Sel_bjeteta", "Sel_bjetphi", "Sel_bjetm"});
			;

			// calculate event weight for MC only
	if (!_isData && !isDefined("evWeight"))
	{
		_rlm = _rlm.Define("jetcutsforsf", "Jet_pt>25.0 && abs(Jet_eta)<2.4 ")
				.Define("Sel_jetforsfpt", "Jet_pt[jetcutsforsf]")
				.Define("Sel_jetforsfeta", "Jet_eta[jetcutsforsf]")
				.Define("Sel_jetforsfhad", "Jet_hadronFlavour[jetcutsforsf]")
				.Define("Sel_jetcsvv2", "Jet_btagCSVV2[jetcutsforsf]")
				.Define("Sel_jetdeepb", "Jet_btagDeepB[jetcutsforsf]");

		// function to calculate event weight for MC events based on CSV algorithm
		auto btagweightgenerator= [this](floats &pts, floats &etas, ints &hadflav, floats &btags)->float
		{
			double bweight=1.0;
			BTagEntry::JetFlavor hadfconv;
			for (auto i=0; i<pts.size(); i++)
			{
				if (hadflav[i]==5) hadfconv=BTagEntry::FLAV_B;
				else if (hadflav[i]==4) hadfconv=BTagEntry::FLAV_C;
				else hadfconv=BTagEntry::FLAV_UDSG;

				double w = _btagcalibreader.eval_auto_bounds("central", hadfconv, fabs(etas[i]), pts[i], btags[i]);
				bweight *= w;
			}
			return bweight;
		};

		_rlm = _rlm.Define("btagWeight_CSVV2recalc", btagweightgenerator, {"Sel_jetforsfpt", "Sel_jetforsfeta", "Sel_jetforsfhad", "Sel_jetcsvv2"});
		_rlm = _rlm.Define("evWeight_CSVV2", "pugenWeight * btagWeight_CSVV2recalc");

		// function to calculate event weight for MC events based on DeepCSV algorithm
		auto btagweightgenerator2= [this](floats &pts, floats &etas, ints &hadflav, floats &btags)->float
		{
			double bweight=1.0;
			BTagEntry::JetFlavor hadfconv;
			for (auto i=0; i<pts.size(); i++)
			{
				if (hadflav[i]==5) hadfconv=BTagEntry::FLAV_B;
				else if (hadflav[i]==4) hadfconv=BTagEntry::FLAV_C;
				else hadfconv=BTagEntry::FLAV_UDSG;

				double w = _btagcalibreader2.eval_auto_bounds("central", hadfconv, fabs(etas[i]), pts[i], btags[i]);
				bweight *= w;
			}
			return bweight;
		};
		_rlm = _rlm.Define("btagWeight_DeepCSVBrecalc", btagweightgenerator2, {"Sel_jetforsfpt", "Sel_jetforsfeta", "Sel_jetforsfhad", "Sel_jetdeepb"});
		_rlm = _rlm.Define("evWeight_DeepCSVB", "pugenWeight * btagWeight_DeepCSVBrecalc");
		_rlm = _rlm.Define("evWeight", "evWeight_DeepCSVB");
	}

}

void NanoAODAnalyzerrdframe::removeOverlaps()
{
	// lambda function
	// for checking overlapped jets with electrons
	auto checkoverlap = [](FourVectorVec &seljets, FourVectorVec &selele)
		{
			doubles mindrlepton;
			//cout << "selected jets size" << seljets.size() << endl;
			//cout << "selected electrons size" << selele.size() << endl;

			for (auto ajet: seljets)
			{
				std::vector<double> drelejet(selele.size());
				for (auto alepton: selele)
				{
					auto dr = ROOT::Math::VectorUtil::DeltaR(ajet, alepton);
					drelejet.emplace_back(dr);
				}
				auto mindr = selele.size()==0 ? 6.0 : *std::min_element(drelejet.begin(), drelejet.end());
				mindrlepton.emplace_back(mindr);
			}

			return mindrlepton;
		};
	//cout << "overlap removal" << endl;
	_rlm = _rlm.Define("mindrlepton", checkoverlap, {"jet4vecs","ele4vecs"});

	//cout << "redefine cleaned jets" << endl;
	_rlm = _rlm.Define("overlapcheck", "mindrlepton>0.4");

	_rlm =	_rlm.Define("Sel2_jetpt", "Sel_jetpt[overlapcheck]")
		.Define("Sel2_jeteta", "Sel_jeteta[overlapcheck]")
		.Define("Sel2_jetphi", "Sel_jetphi[overlapcheck]")
		.Define("Sel2_jetmass", "Sel_jetmass[overlapcheck]")
		.Define("Sel2_jetbtag", "Sel_jetbtag[overlapcheck]")
		.Define("ncleanjetspass", "int(Sel2_jetpt.size())")
		.Define("cleanjet4vecs", ::gen4vec, {"Sel2_jetpt", "Sel2_jeteta", "Sel2_jetphi", "Sel2_jetmass"})
		.Define("Sel2_jetHT", "Sum(Sel2_jetpt)")
		.Define("Sel2_jetweight", "std::vector<double>(ncleanjetspass, evWeight)"); //


	_rlm = _rlm.Define("btagcuts2", "Sel2_jetbtag>0.8")
			.Define("Sel2_bjetpt", "Sel2_jetpt[btagcuts]")
			.Define("Sel2_bjeteta", "Sel2_jeteta[btagcuts]")
			.Define("Sel2_bjetphi", "Sel2_jetphi[btagcuts]")
			.Define("Sel2_bjetmass", "Sel2_jetmass[btagcuts]")
			.Define("ncleanbjetspass", "int(Sel_bjetpt.size())")
			.Define("Sel2_bjetHT", "Sum(Sel2_bjetpt)")
			.Define("cleanbjet4vecs", ::gen4vec, {"Sel2_bjetpt", "Sel2_bjeteta", "Sel2_bjetphi", "Sel2_bjetmass"});

}

void NanoAODAnalyzerrdframe::selectFatJets()
{
	_rlm = _rlm.Define("fatjetcuts", "FatJet_pt>400.0 && abs(FatJet_eta)<2.4 && FatJet_tau1>0.0 && FatJet_tau2>0.0 && FatJet_tau3>0.0 && FatJet_tau3/FatJet_tau2<0.5")
				.Define("Sel_fatjetpt", "FatJet_pt[fatjetcuts]")
				.Define("Sel_fatjeteta", "FatJet_eta[fatjetcuts]")
				.Define("Sel_fatjetphi", "FatJet_phi[fatjetcuts]")
				.Define("Sel_fatjetmass", "FatJet_mass[fatjetcuts]")
				.Define("nfatjetspass", "int(Sel_fatjetpt.size())")
				.Define("Sel_fatjetweight", "std::vector<double>(nfatjetspass, evWeight)")
				.Define("Sel_fatjet4vecs", ::gen4vec, {"Sel_fatjetpt", "Sel_fatjeteta", "Sel_fatjetphi", "Sel_fatjetmass"});
}

/*
bool NanoAODAnalyzerrdframe::helper_1DHistCreator(std::string hname, std::string title, const int nbins, const double xlow, const double xhi, std::string rdfvar, std::string evWeight)
{
	RDF1DHist histojets = _rlm.Histo1D({hname.c_str(), title.c_str(), nbins, xlow, xhi}, rdfvar, evWeight); // Fill with weight given by evWeight
	_th1dhistos[hname] = histojets;
}
*/

bool NanoAODAnalyzerrdframe::helper_1DHistCreator(std::string hname, std::string title, const int nbins, const double xlow, const double xhi, std::string rdfvar, std::string evWeight, RNode *anode)
{
	RDF1DHist histojets = anode->Histo1D({hname.c_str(), title.c_str(), nbins, xlow, xhi}, rdfvar, evWeight); // Fill with weight given by evWeight
	_th1dhistos[hname] = histojets;
}


// Automatically loop to create
void NanoAODAnalyzerrdframe::setupCuts_and_Hists()
{
	cout << "setting up definitions, cuts, and histograms" <<endl;

	for ( auto &c : _varinfovector)
	{
		if (c.mincutstep.length()==0) _rlm = _rlm.Define(c.varname, c.vardefinition);
	}

	for (auto &x : _hist1dinfovector)
	{
		std::string hpost = "_nocut";

		if (x.mincutstep.length()==0)
		{
			helper_1DHistCreator(std::string(x.hmodel.fName)+hpost,  std::string(x.hmodel.fTitle)+hpost, x.hmodel.fNbinsX, x.hmodel.fXLow, x.hmodel.fXUp, x.varname, x.weightname, &_rlm);
		}
	}

	_rnt.setRNode(&_rlm);

	for (auto acut : _cutinfovector)
	{
		std::string cutname = "cut"+ acut.idx;
		std::string hpost = "_"+cutname;
		RNode *r = _rnt.getParent(acut.idx)->getRNode();
		auto rnext = new RNode(r->Define(cutname, acut.cutdefinition));
		*rnext = rnext->Filter(cutname);

		for ( auto &c : _varinfovector)
		{
			if (acut.idx.compare(c.mincutstep)==0) *rnext = rnext->Define(c.varname, c.vardefinition);
		}
		for (auto &x : _hist1dinfovector)
		{
			if (acut.idx.compare(0, x.mincutstep.length(), x.mincutstep)==0)
			{
				helper_1DHistCreator(std::string(x.hmodel.fName)+hpost,  std::string(x.hmodel.fTitle)+hpost, x.hmodel.fNbinsX, x.hmodel.fXLow, x.hmodel.fXUp, x.varname, x.weightname, rnext);
			}
		}
		_rnt.addDaughter(rnext, acut.idx);

		/*
		_rlm = _rlm.Define(cutname, acut.cutdefinition);
		_rlm = _rlm.Filter(cutname);

		for ( auto &c : _varinfovector)
		{
			if (acut.idx.compare(c.mincutstep)==0) _rlm = _rlm.Define(c.varname, c.vardefinition);
		}
		for (auto &x : _hist1dinfovector)
		{
			if (acut.idx.compare(0, x.mincutstep.length(), x.mincutstep)==0)
			{
				helper_1DHistCreator(std::string(x.hmodel.fName)+hpost,  std::string(x.hmodel.fTitle)+hpost, x.hmodel.fNbinsX, x.hmodel.fXLow, x.hmodel.fXUp, x.varname, x.weightname);
			}
		}
		_rnt.addDaughter(&_rlm, acut.idx);
		*/
	}
}

void NanoAODAnalyzerrdframe::add1DHist(TH1DModel histdef, std::string variable, std::string weight,string mincutstep)
{
	_hist1dinfovector.push_back({histdef, variable, weight, mincutstep});
}


void NanoAODAnalyzerrdframe::drawHists(RNode t)
{
	cout << "processing" <<endl;
	t.Count();
}

void NanoAODAnalyzerrdframe::addVar(varinfo v)
{
	_varinfovector.push_back(v);
}

void NanoAODAnalyzerrdframe::addVartoStore(string varname)
{
	// varname is assumed to be a regular expression.
	// e.g. if varname is "Muon_eta" then "Muon_eta" will be stored
	// if varname=="Muon_.*", then any branch name that starts with "Muon_" string will
	// be saved
	_varstostore.push_back(varname);
	/*
	std::regex b(varname);
	bool foundmatch = false;
	for (auto a: _rlm.GetColumnNames())
	{
		if (std::regex_match(a, b)) {
			_varstostore.push_back(a);
			foundmatch = true;
		}
	}
	*/

}

void NanoAODAnalyzerrdframe::setupTree()
{
	vector<RNodeTree *> rntends;
	_rnt.getRNodeLeafs(rntends);
	for (auto arnt: rntends)
	{
		RNode *arnode = arnt->getRNode();
		string nodename = arnt->getIndex();
		vector<string> varforthistree;
		std::map<string, int> varused;

		for (auto varname: _varstostore)
		{
			bool foundmatch = false;
			std::regex b(varname);
			for (auto a: arnode->GetColumnNames())
			{
				if (std::regex_match(a, b) && varused[a]==0)
				{
					varforthistree.push_back(a);
					varused[a]++;
					foundmatch = true;
				}
			}
			if (!foundmatch)
			{
				cout << varname << " not found at "<< nodename << endl;
			}

		}
		_varstostorepertree[nodename]  = varforthistree;
	}

}

void NanoAODAnalyzerrdframe::addCuts(string cut, string idx)
{
	_cutinfovector.push_back({cut, idx});
}


void NanoAODAnalyzerrdframe::run(bool saveAll, string outtreename)
{
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

	for (auto arnt: rntends)
	{
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
		}
		else {
			// use the following if you want to store only a few variables
			//arnode->Snapshot(outtreename, outname, _varstostore);
			cout << " writing branches" << endl;
			for (auto bname: _varstostorepertree[nodename])
			{
				cout << bname << endl;
			}
			arnode->Snapshot(outtreename, outname, _varstostorepertree[nodename]);
		}
	}



}
