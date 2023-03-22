#!/bin/python
# -*- coding: utf-8 -*-
"""
Created on Mon Sep 14 11:01:46 2018

@author: Suyong Choi (Department of Physics, Korea University suyong@korea.ac.kr)

This script applies nanoaod processing to one file
"""

import sys
import cppyy
import ROOT
from ROOT import *

if __name__=='__main__':
    from optparse import OptionParser
    parser = OptionParser(usage="%prog [options]")
    parser.add_option("-I", "--infile",  dest="infile", type="string", default="", help="Input file name")
    parser.add_option("-O", "--outfile",  dest="outfile", type="string", default="", help="Output file name")
    parser.add_option("-Y", "--year",  dest="year", type="string", default="", help="Select 2016pre/post, 2017, or 2018 for years")
    parser.add_option("-S", "--syst",  dest="syst", type="string", default="", help="Systematic sources")
    parser.add_option("-J", "--json",  dest="json", type="string", default="", help="Select events using this JSON file, meaningful only for data")
    parser.add_option("--saveallbranches", dest="saveallbranches", action="store_true", default=False, help="Save all branches. False by default")
    parser.add_option("--globaltag", dest="globaltag", type="string", default="", help="Global tag to be used in JetMET corrections")
    (options, args) = parser.parse_args()


    #if "Run2016" in options.infile:
    #    options.json = "data/GoldenJSON/Cert_271036-284044_13TeV_Legacy2016_Collisions16_JSON.txt"
    #elif "Run2017" in options.infile:
    #    options.json = "data/GoldenJSON/Cert_294927-306462_13TeV_UL2017_Collisions17_GoldenJSON.txt"
    #elif "Run2018" in options.infile:
    #    options.json = "data/GoldenJSON/Cert_314472-325175_13TeV_Legacy2018_Collisions18_JSON.txt"

    if any(i in options.infile for i in ["Run2016B", "Run2016C", "Run2016D"]):
        options.globaltag = "Summer19UL16APV_RunBCD_V7"
    elif any(i in options.infile for i in ["Run2016E", "Run2016F"]) and options.year == "2016pre":
        options.globaltag = "Summer19UL16APV_RunEF_V7"
    elif any(i in options.infile for i in ["Run2016F", "Run2016G", "Run2016H"]) and options.year == "2016post":
        options.globaltag = "Summer19UL16_RunFGH_V7"
    elif any(i in options.infile for i in ["Run2017", "Run2018"]):
        era = options.infile[options.infile.find("Run" + options.year)+7]
        options.globaltag = "Summer19UL" + options.year[2:] + "_Run" + era + "_V5"
    elif "UL16NanoAODAPVv" in options.infile:
        options.globaltag = "Summer19UL16APV_V7"
    elif "UL16NanoAODv" in options.infile:
        options.globaltag = "Summer19UL16_V7"
    elif "UL17NanoAODv" in options.infile:
        options.globaltag = "Summer19UL17_V5"
    elif "UL18NanoAODv" in options.infile:
        options.globaltag = "Summer19UL18_V5"

    # load compiled C++ library into ROOT/python
    cppyy.load_reflection_info("libnanoadrdframe.so")
    t = ROOT.TChain("Events")
    t.Add(options.infile)
    aproc = ROOT.SkimEvents(t, options.outfile, options.year, options.syst, options.json, options.globaltag)
    aproc.setupAnalysis()
    aproc.run(options.saveallbranches, "Events")

