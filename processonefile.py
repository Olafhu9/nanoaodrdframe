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

if __name__=='__main__':

    from optparse import OptionParser
    parser = OptionParser(usage="%prog [options] inputDir outputDir")
    parser.add_option("-I", "--infile",  dest="infile", type="string", default="", help="Input file name")
    parser.add_option("-O", "--outfile",  dest="outfile", type="string", default="", help="Output file name")
    parser.add_option("-Y", "--year",  dest="year", type="string", default="", help="Select 2016, 2017, 2018 year of runs")
    parser.add_option("-S", "--syst",  dest="syst", type="string", default="", help="Systematic sources")
    parser.add_option("-J", "--json",  dest="json", type="string", default="", help="Select events using this JSON file, meaningful only for data")
    parser.add_option("--saveallbranches", dest="saveallbranches", action="store_true", default=False, help="Save all branches. False by default")
    parser.add_option("--globaltag", dest="globaltag", type="string", default="", help="Global tag to be used in JetMET corrections")
    (options, args) = parser.parse_args()

    if "SingleMuon2016" in options.infile:
        options.json = "data/GoldenJSON/Cert_271036-284044_13TeV_Legacy2016_Collisions16_JSON.txt"
    elif "SingleMuon2017" in options.infile:
        options.json = "data/GoldenJSON/Cert_294927-306462_13TeV_UL2017_Collisions17_GoldenJSON.txt"
    elif "SingleMuon2018" in options.infile:
        options.json = "data/GoldenJSON/Cert_314472-325175_13TeV_Legacy2018_Collisions18_JSON.txt"


    # load compiled C++ library into ROOT/python
    cppyy.load_reflection_info("libnanoadrdframe.so")
    t = ROOT.TChain("Events")
    t.Add(options.infile)
    aproc = ROOT.TopLFVAnalyzer(t, options.outfile, options.year, options.syst, options.json, options.globaltag)
    aproc.setupAnalysis()
    aproc.run(options.saveallbranches, "Events")

    # process input rootfiles to sum up all the counterhistograms
    counterhistogramsum = None
    intf = ROOT.TFile(options.infile)
    counterhistogram = intf.Get("hcounter")
    counterhistogramsum = counterhistogram.Clone()
    counterhistogramsum.SetDirectory(0)
    intf.Close()

    if counterhistogramsum != None:
        print("Updating with counter histogram")
        outf = ROOT.TFile(options.outfile, "UPDATE")
        counterhistogramsum.Write()
        outf.Write("", ROOT.TObject.kOverwrite)
        outf.Close()
    else:
        print("counter histogram not found")
