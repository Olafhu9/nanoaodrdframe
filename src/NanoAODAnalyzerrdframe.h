/*
 * NanoAODAnalyzerrdframe.h
 *
 *  Created on: Sep 30, 2018
 *      Author: suyong
 */

#ifndef NANOAODANALYZERRDFRAME_H_
#define NANOAODANALYZERRDFRAME_H_

#include "TTree.h"
#include "TFile.h"

#include "ROOT/RDataFrame.hxx"
#include "ROOT/RVec.hxx"

//#include "Math/Vector4D.h"

#include "correction.h"

#include <string>
#include <vector>
#include <map>
//#include "rapidjson/document.h"
#include "nlohmann/json.hpp"

#include "utility.h" // floats, etc are defined here
#include "RNodeTree.h"


using namespace ROOT::RDF;
using namespace std;

class TH1D;

using json = nlohmann::json;

class NanoAODAnalyzerrdframe {
	using RDF1DHist = RResultPtr<TH1D>;
public:
	NanoAODAnalyzerrdframe(string infilename, string intreename, string outfilename);
	NanoAODAnalyzerrdframe(TTree *t, string outfilename);
	virtual ~NanoAODAnalyzerrdframe();

	void setupCorrections(string goodjsonfname, string pufname, string putag, string btvfname, string btvtype, string jercfname, string jerctag, string jercunctag);
	void setupObjects();
	void setupAnalysis();

	// object selectors
	// RNode is in namespace ROOT::RDF
	bool readgoodjson(string goodjsonfname); // get ready for applying golden JSON
	void selectElectrons();
	void selectMuons();
	void applyJetMETCorrections();
	void selectJets();
	void selectFatJets();
	void removeOverlaps();
	virtual void defineMoreVars() = 0; // define higher-level variables from basic ones, you must implement this in your subclassed analysis code

	void addVar(varinfo v);

	// define variables
	template <typename T, typename std::enable_if<!std::is_convertible<T, string>::value, int>::type = 0>
	void defineVar(string varname, T function,  const RDFDetail::ColumnNames_t &columns = {})
	{
		_rlm = _rlm.Define(varname, function, columns);
	};

	void addVartoStore(string varname);
	void addCuts(string cut, string idx);
	virtual void defineCuts() = 0; // define a series of cuts from defined variables only. you must implement this in your subclassed analysis code
	void add1DHist(TH1DModel histdef, string variable, string weight, string mincutstep="");
	virtual void bookHists() = 0; // book histograms, you must implement this in your subclassed analysis code

	void setupCuts_and_Hists();
	void drawHists(RNode t);
	void run(bool saveAll=true, string outtreename="outputTree");
	void setTree(TTree *t, string outfilename);
	void setupTree();

private:
	ROOT::RDataFrame _rd;

	bool _isData;
	bool _jsonOK;
	string _outfilename;
	string _jsonfname;
	string _jerctag;
	string _jercunctag;
	string _putag;
	string _btvtype;

	TFile *_outrootfile;
	vector<string> _outrootfilenames;
	RNode _rlm;
	map<string, RDF1DHist> _th1dhistos;
	//bool helper_1DHistCreator(std::string hname, std::string title, const int nbins, const double xlow, const double xhi, std::string rdfvar, std::string evWeight);
	bool helper_1DHistCreator(string hname, string title, const int nbins, const double xlow, const double xhi, string rdfvar, string evWeight, RNode *anode);
	vector<string> _originalvars;
	vector<string> _selections;

	vector<hist1dinfo> _hist1dinfovector;
	vector<varinfo> _varinfovector;
	vector<cutinfo> _cutinfovector;

	vector<string> _varstostore;
	map<string, vector<std::string>> _varstostorepertree;

	// using rapidjson
	//for applying golden json to data
	//rapidjson::Document jsonroot;

	json jsonroot;

	// pile up weights
	std::unique_ptr<correction::CorrectionSet> _correction_pu;

	// JERC scale factors
	std::unique_ptr<correction::CorrectionSet> _correction_jerc; // json containing all forms of corrections and uncertainties
	std::shared_ptr<const correction::CompoundCorrection> _jetCorrector; // just the combined L1L2L3 correction
	std::shared_ptr<const correction::Correction> _jetCorrectionUnc; // for uncertainty corresponding to the jet corrector

	// btag correction
	std::unique_ptr<correction::CorrectionSet> _correction_btag1;


	RNodeTree _rnt;

	bool isDefined(string v);

	// Jet MET corrections
	void setupJetMETCorrection(string fname, string jettag);

};

#endif /* NANOAODANALYZERRDFRAME_H_ */
