#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
Created on Mon Sep 14 11:01:46 2018

@author: Suyong Choi (Department of Physics, Korea University suyong@korea.ac.kr)

This script applies nanoaod processing to all the files in the input
 directory and its subdirectory recursively, copying directory structure to the outputdirectory

"""
import os
import re
import subprocess


from multiprocessing import Process
import cppyy
import ROOT


def function_calling_PostProcessor(outdir, rootfileshere, jobconfmod):
    for afile in rootfileshere:
        rootfname = re.split('\/', afile)[-1]
        withoutext = re.split('\.root', rootfname)[0]
        outfname = outdir + '/' + withoutext + '_analyzed.root'
        subprocess.run(["./skimonefile.py", afile, outfname, jobconfmod])
    pass

class Nanoaodprocessor:
    def __init__(self, indir, outdir, jobconfmod, procflags, config):
        self.outdir = outdir
        self.indir = indir
        self.jobconfmod = jobconfmod

        self.split = procflags['split']
        self.skipold = procflags['skipold']
        self.recursive = procflags['recursive']
        self.saveallbranches = procflags['saveallbranches']
        
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
            if self.skipold: # if processed file exists, then skip, otherwise make a list of unprocessed files
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
                    p = Process(target=function_calling_PostProcessor, args=(outputdirectory, filesforjob, self.jobconfmod)) # positional arguments go into kwargs
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
                    subprocess.run(["./skimonefile.py", afile, outfname, self.jobconfmod])

        # if there are subdirectories recursively call
        if self.recursive:
            for indir, outdir in zip(subdirs, outsubdirs):
                self._processROOTfiles(indir, outdir)
    
def Nanoaodprocessor_singledir(indir, outputroot, procflags, config):
    """Runs nanoaod analyzer over ROOT files in indir 
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
    if not procflags['recursive']:
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
    intreename = config['intreename']
    outtreename = config['outtreename']
    saveallbranches = procflags['saveallbranches']
    t = ROOT.TChain(intreename)
    for afile in rootfilestoprocess:
        t.Add(afile)
    aproc = ROOT.SkimEvents(t, outputroot)
    aproc.setupCorrections(config['goodjson'], config['pileupfname'], config['pileuptag']\
        , config['btvfname'], config['btvtype'], config['jercfname'], config['jerctag'], config['jercunctag'])
    aproc.setupObjects()
    aproc.setupAnalysis()
    aproc.run(saveallbranches, outtreename)
    pass

if __name__=='__main__':
    from importlib import import_module
    from argparse import ArgumentParser
    # inputDir and lower directories contain input NanoAOD files
    # outputDir is where the outputs will be created
    parser = ArgumentParser(usage="%prog inputDir outputDir jobconfmod")
    parser.add_argument("indir")
    parser.add_argument("outdir")
    parser.add_argument("jobconfmod")
    args = parser.parse_args()

    indir = args.indir
    outdir = args.outdir
    jobconfmod = args.jobconfmod

    # check whether input directory exists
    if not os.path.exists(indir):
        print ('Path '+indir+' does not exist. Stopping')
        exit(1)

    # load compiled C++ library into ROOT/python
    cppyy.load_reflection_info("libnanoadrdframe.so")
    cppyy.load_reflection_info("libcorrectionlib.so")

    # read in configurations from job configuration python module
    mod = import_module(jobconfmod)
    procflags = getattr(mod, 'procflags')
    config = getattr(mod, 'config')

    if not procflags['allinone']:
        n=Nanoaodprocessor(indir, outdir, jobconfmod, procflags, config)
        n.process()
    else:
        Nanoaodprocessor_singledir (indir, outdir, procflags, config) # although it says outdir, it should really be a output ROOT file name
