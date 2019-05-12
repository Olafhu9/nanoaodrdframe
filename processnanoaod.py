#!/bin/python
# -*- coding: utf-8 -*-
"""
Created on Mon Sep 14 11:01:46 2018

@author: Suyong Choi (Department of Physics, Korea University suyong@korea.ac.kr)

This script applies nanoaod processing to all the files in the input
 directory and its subdirectory recursively, copying directory structure to the outputdirectory

"""
import sys
import os
import re
import string
import subprocess


from importlib import import_module
from multiprocessing import Process
import cppyy
import ROOT


def function_calling_PostProcessor(outdir, rootfileshere, outtreename, intreeename, json, saveallbranches, globaltag):
    for afile in rootfileshere:
        rootfname = re.split('\/', afile)[-1]
        withoutext = re.split('\.root', rootfname)[0]
        outfname = outdir + '/' + withoutext + '_analyzed.root'
        subprocess.call(["./processonefile.py", "--json=%s"%json, "--savealbranches=%s"%saveallbranches, "--globaltag=%s"%globaltag, afile, outfname, intreeename, outtreename])
    pass


class Nanoaodprocessor:
    def __init__(self, outdir, indir, outtree, intree, json, split, skipold, recursive, saveallbranches, globaltag):
        self.outdir = outdir
        self.indir = indir
        self.outtreename = outtree
        self.intreeename = intree
        self.json = json
        self.split = split
        self.skipold = skipold
        self.recursive = recursive
        self.saveallbranches = saveallbranches
        self.globaltag = globaltag
        
        # check whether input directory exists
        if not os.path.exists(self.indir):
            print ('Path '+indir+' does not exist')
            exit(1)
        pass

    def process(self):
        self._processROOTfiles(self.indir, self.outdir)
        pass


    def _processROOTfiles(self, inputdirectory, outputdirectory):
        # list currect directory
        flist = os.listdir(inputdirectory) 
        print(flist)
        rootfileshere = []
        subdirs = []
        outsubdirs = []
        # create output directory if it doesn't already exist
        if not os.path.exists(outputdirectory):
            os.mkdir(outputdirectory)
        # loop through the list
        # pick root files but not those that match  _analyzed.root 
        for fname in flist:
            fullname = os.path.join(inputdirectory, fname)
            if re.match('.*\.root', fname) and not re.match('.*_analyzed\.root', fname) and os.path.isfile(fullname): # if it has .root file extension
                rootfileshere.append(fullname) 
            elif os.path.isdir(fullname):  # if it's a directory name
                subdirs.append(fullname)
                outsubdirs.append(outputdirectory+'/'+fname)
        
        print("files found in directory "+inputdirectory)
        print(rootfileshere)
        
        # run
        if len(rootfileshere)>0:
            if self.skipold: # if processed file exists, then skip
                oflist = os.listdir(outputdirectory)
                filteredoflist = []
                for fname in oflist :
                    fullname = os.path.join(outputdirectory, fname)
                    if re.match('.*\.root', fname) and os.path.isfile(fullname): # if it has .root file extension:
                        withoutext = re.split("\.root", fname)[0]
                        wihoutskimtext = re.split("\_analyzed", withoutext)[0]
                        filteredoflist.append(wihoutskimtext)
                print('root files in output directory')
                print(filteredoflist)
                filterediflist = []
                for ifname in rootfileshere:
                    rootfname = re.split('\/', ifname)[-1]
                    withoutext = re.split('\.root', rootfname)[0]
                    if withoutext not in filteredoflist:
                        print(withoutext+' not yet in output dir')
                        filterediflist.append(ifname)
                    else:
                        print(withoutext+' in output dir')
                         
                rootfileshere = filterediflist

            if self.split>1: # use multiprocessing
                njobs = self.split
                nfileperjob = len(rootfileshere) *1.0 / njobs

                # if number of files is less than the number of splits desired
                if len(rootfileshere) < self.split:
                    njobs = len(rootfileshere)
                    nfileperjob = 1

                print ("splitting files")
                
                ap = []
                for i in range(njobs):
                    if i<njobs-1:
                        filesforjob = rootfileshere[int(i*nfileperjob):int((i+1)*nfileperjob)]
                    else:
                        filesforjob = rootfileshere[int(i*nfileperjob):]
                    p = Process(target=function_calling_PostProcessor, args=(outputdirectory, filesforjob, self.outtreename, self.intreeename, self.json, self.saveallbranches, self.globaltag)) # positional arguments go into kwargs
                    p.start()
                    ap.append(p)
                for proc in ap:
                    proc.join()
            else: # no multiprocessing
                #function_calling_PostProcessor(outputdirectory, rootfileshere, self.json, self.isdata)
                aproc = None
                for afile in rootfileshere:
                    rootfname = re.split('\/', afile)[-1]
                    withoutext = re.split('\.root', rootfname)[0]
                    outfname = outputdirectory +'/'+ withoutext + '_analyzed.root'
                    subprocess.call(["./processonefile.py", "--json=%s"%self.json, "--savealbranches=%s"%self.saveallbranches, "--globaltag=%s"%self.globaltag, afile, outfname, self.intreeename, self.outtreename])

                    # the following works, but memory usage of this process grows with time.. Don't know how to solve it.
                    """
                    t = ROOT.TChain("Events")
                    t.Add(afile)
                    rootfname = re.split('\/', afile)[-1]
                    withoutext = re.split('\.root', rootfname)[0]
                    outfname = outputdirectory +'/'+ withoutext + '_analyzed.root'
                    if aproc is None:
                        aproc = ROOT.NanoAODAnalyzerrdframe(t, outfname, self.json)
                        aproc.setupAnalysis()
                    else:
                        aproc.setTree(t, outfname)
                    aproc.run()
                    t.Delete();
                    t = None
                    """
        # if there are subdirectories recursively call
        if self.recursive:
            for indir, outdir in zip(subdirs, outsubdirs):
                self._processROOTfiles(indir, outdir)
    
def Nanoaodprocessor_singledir(outputroot, indir, outtree, intree, json, split, recursive, saveallbranches, globaltag):
    """Runs nanoaod analyzer over ROOT files in indir (but doesn't search recursively)
    and run outputs into a signel ROOT file.
    
    Arguments:
        outputroot {string} -- [description]
        indir {string} -- [description]
        json {string} -- [description]
    """

    if not re.match('.*\.root', outputroot):
        print("Output file should be a root file! Quitting")
        exit(-1)

    fullnamelist =[]
    rootfilestoprocess = []
    print("collecting root files in "+indir)
    if not recursive:
        flist = os.listdir(indir)
        for fname in flist:
            fullname = os.path.join(indir, fname)
            fullnamelist.append(fullname)
    else: # os.walk lists directories recursively (here, we will not follow symlink)
        for root, dirs, flist in os.walk(indir):
            for fname in flist:
                fullname = os.path.join(root, fname)
                fullnamelist.append(fullname)

    for  fname in fullnamelist:            
        if re.match('.*\.root', fname) and os.path.isfile(fname): # if it has .root file extension
            rootfilestoprocess.append(fname)
    print("files to process")
    print(rootfilestoprocess)
    t = ROOT.TChain(intree)
    for afile in rootfilestoprocess:
        t.Add(afile)
    aproc = ROOT.FourtopAnalyzer(t, outputroot, json, globaltag, split)
    aproc.setupAnalysis()
    aproc.run(saveallbranches, outtree)

    # process input rootfiles to sum up all the counterhistograms
    counterhistogramsum = None
    for arootfile in rootfilestoprocess:    
        intf = ROOT.TFile(arootfile)
        counterhistogram = intf.Get("hcounter_nocut")
        if counterhistogram != None:
            if counterhistogramsum == None:
                counterhistogramsum = counterhistogram.Clone()
                counterhistogramsum.SetDirectory(0)
            else:
                counterhistogramsum.Add(counterhistogram)
        intf.Close()
    # save the summed up counterhistogram to the outputfile
    if counterhistogramsum != None:
        print("Updating with counter histogram")
        outf = ROOT.TFile(outputroot, "UPDATE")
        counterhistogramsum.Write()
        outf.Write(0, ROOT.TObject.kOverwrite)
        outf.Close()
    else:
        print("counter histogram not found@")

    pass

if __name__=='__main__':
    
    from optparse import OptionParser
    # inputDir and lower directories contain input NanoAOD files
    # outputDir is where the outputs will be created
    parser = OptionParser(usage="%prog [options] inputDir outputDir")
    parser.add_option("-J", "--json",  dest="json", type="string", default="", help="Select events using this JSON file, meaningful only for data")
    parser.add_option("--split", dest="split", type=int, default=1, help="How many jobs to split into")
    parser.add_option("--skipold", dest="skipold", action="store_true", default=False, help="Skip existing root files")
    parser.add_option("--recursive", dest="recursive", action="store_true", default=True, help="Process files in the subdirectories recursively")
    parser.add_option("--allinone", dest="allinone", action="store_true", default=False, help="Process all files and output a single root file. You must make sure MC and Data are not mixed together.")
    parser.add_option("--saveallbranches", dest="saveallbranches", action="store_true", default=False, help="Save all branches. False by default")
    parser.add_option("--globaltag", dest="globaltag", type="string", default="", help="Global tag to be used in JetMET corrections")
    (options, args) = parser.parse_args()

    if len(args) < 1 :
        parser.print_help()
        sys.exit(1)

    indir = args[0]
    outdir = args[1]

    # load compiled C++ library into ROOT/python
    cppyy.load_reflection_info("libnanoadrdframe.so")

    intree = "outputTree"
    outtree = "outputTree2"

    if not options.allinone:
        n=Nanoaodprocessor(outdir, indir, outtree, intree, options.json, options.split, options.skipold, options.recursive, options.saveallbranches, options.globaltag)
        n.process()
    else:
        Nanoaodprocessor_singledir(outdir, indir, outtree, intree, options.json,  options.split, options.recursive, options.saveallbranches, options.globaltag) # although it says outdir, it should really be a output ROOT file name
