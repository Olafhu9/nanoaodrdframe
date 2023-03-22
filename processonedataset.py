#!/bin/python
# -*- coding: utf-8 -*-
"""
Created on Mon Sep 14 11:01:46 2018

@author: Suyong Choi (Department of Physics, Korea University suyong@korea.ac.kr)
@refactored for LFV analysis: Jiwon Park (jiwon.park@cern.ch)
"""
import sys, os, re, argparse
import cppyy
import ROOT

#def Nanoaodprocessor_singledir(outputroot, indir, year, syst, json):
if __name__=='__main__':
    parser = argparse.ArgumentParser(usage="%prog [options]")
    parser.add_argument("-I", "--indir",  dest="indir", type=str, default="", help="Input directory")
    parser.add_argument("-O", "--outputroot", dest="outputroot", type=str, default="", help="Output file in your working directory")
    parser.add_argument("-Y", "--year", dest="year", type=str, default="", help="Select 2016pre, 2016post, 2017, or 2018 runs")
    parser.add_argument("-S", "--syst", dest="syst", type=str, default="theory", help="Systematic: 'data' for Data, 'nosyst' for mc without uncertainties. Default is 'theory'. To run without theory unc for TT samples, put 'all'.")
    parser.add_argument("-D", "--dataset", dest="dataset", action="store", nargs="+", default=[], help="Put dataset folder name (eg. TTTo2L2Nu) to process specific one.")
    parser.add_argument("-F", "--dataOrMC", dest="dataOrMC", type=str, default="", help="data or mc flag, if you want to process data-only or mc-only")
    options = parser.parse_args()

    outputroot = options.outputroot
    indir = options.indir
    year = options.year
    syst = options.syst

    workdir = os.getcwd()

    json = ""
    if 'data' in indir[5:]:
        if syst == "data":
            if "2016" in indir:
                json = os.path.join(workdir, "data/GoldenJSON/Cert_271036-284044_13TeV_Legacy2016_Collisions16_JSON.txt")
            elif "2017" in indir:
                json = os.path.join(workdir, "data/GoldenJSON/Cert_294927-306462_13TeV_UL2017_Collisions17_GoldenJSON.txt")
            elif "2018" in indir:
                json = os.path.join(workdir, "data/GoldenJSON/Cert_314472-325175_13TeV_Legacy2018_Collisions18_JSON.txt")

    print("Input: " + indir + ", " + "Output: " + outputroot + ", Syst: " + syst + ", Json: " + json + "\n")

    if not re.match('.*\.root', outputroot):
        print("Output file should be a root file! Quitting")
        exit(-1)

    fullnamelist =[]
    rootfilestoprocess = []
    print("collecting root files in "+indir)
    flist = os.listdir(indir)
    if len(flist) == 0:
        print("No file found in," + indir + ", ending processing")
        sys.exit()
    for fname in flist:
        fullname = os.path.join(indir, fname)
        fullnamelist.append(fullname)

    for fname in fullnamelist:            
        if re.match('.*\.root', fname) and os.path.isfile(fname): # if it has .root file extension
            rootfilestoprocess.append(fname)
    print("files to process")
    print(rootfilestoprocess)
    cppyy.load_reflection_info("libnanoadrdframe.so")
    t = ROOT.TChain("Events")
    for afile in rootfilestoprocess:
        tmpf = ROOT.TFile.Open(afile)
        tmpt = tmpf.Get("Events")
        if tmpt.GetEntries() > 0:
            t.Add(afile)
    if t.GetEntries() == 0:
        print("There is NO EVENT to process, ending the processing!!")
        sys.exit()
    aproc = None
    aproc = ROOT.TopLFVAnalyzer(t, outputroot, year, syst, json, "", 1)
    aproc.setupAnalysis()
    aproc.run(False, "Events")

    # process input rootfiles to sum up all the counterhistograms
    counterhistogramsum = None
    for arootfile in rootfilestoprocess:
        intf = ROOT.TFile(arootfile)
        counterhistogram = intf.Get("hcounter")
        if counterhistogram != None:
            if counterhistogramsum == None:
                counterhistogramsum = counterhistogram.Clone()
                counterhistogramsum.SetDirectory(0)
            else:
                counterhistogramsum.Add(counterhistogram)
        intf.Close()
    if counterhistogramsum != None:
        print("Updating with counter histogram")
        outf = ROOT.TFile(outputroot, "UPDATE")
        counterhistogramsum.Write()
        outf.Write("", ROOT.TObject.kOverwrite)
        outf.Close()
    else:
        print("counter histogram not found")

    pass
