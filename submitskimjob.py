#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
from importlib import import_module

# loop over the directories and submit background jobs
def submit(jobconfmod):

    # get input directory, outputdirectory, stderrout file from the job configuration python module
    mod = import_module(jobconfmod)
    nanoaod_inputdir_outputdir_pairs= getattr(mod, 'nanoaod_inputdir_outputdir_pairs')

    # job configuation module name is to be passed to the skim namoaod main 
    for indir,outdir,outfile in nanoaod_inputdir_outputdir_pairs:
        os.system('./skimnanoaod.py '+ indir + ' ' + outdir + ' ' + jobconfmod  + ' > ' + outfile + ' 2>&1 &') # submit background jobs
    pass


if __name__=="__main__":
    from argparse import ArgumentParser
    parser = ArgumentParser(usage="%prog jobconfigmodule", description='submits skim jobs in background where the job '\
        +' configuration python module name should be passedd')
    parser.add_argument("jobconfmod")
    args = parser.parse_args()

    jobconfmod = args.jobconfmod
    submit(jobconfmod)
    pass