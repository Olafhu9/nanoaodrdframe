#!/bin/python
# -*- coding: utf-8 -*-

import os
from jobconfig import *

# loop over the directories and submit background jobs
for indir,outdir,outfile in nanoaod_inputdir_outputdir_pairs:
    os.system('./processnanoaod.py '+ indir + ' ' + outdir + ' > ' + outfile + ' 2>&1 &') # submit background jobs
