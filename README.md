# nanoaodrdframe
Analysis of CMS experiment Nano AOD format using ROOT RDataFrame

The package shows how to analyze using the data frame concept.
In a data frame concept, the operations on data should be defined
in terms of functions, which means that result of applying
a function should produce outputs and no internal states are allowed.
In ROOT, RDataFrame allows us to borrow the concept for analysis.

This package code implements a hadronic channel analysis using NanoAOD format.
However, it can be adapted for any flat ROOT trees.

I. Introduction

- The advantage of using RDataFrame is that you don't have to
worry about looping, filling of histograms at the right place,
writing out trees. In some sense you can concentrate more on
thinking about data analysis.

- Draw back is that you have to think of a new way of doing old things.
This could take some getting used to. Also complicated algorithms may not
be so easy to implement with RDataFrame alone.

- Purely data frame concept is not ideal for HEP data since
we have to treat real data and simulated data differently.
We must apply different operations depending on the input data,
   such as scale factors and corrections.
Therefore, some form of global state should be stored. 
In this package, the data frame concept is used together
with object oriented concept for this purpose.


II. Code

- The code consists of a main class NanoAODAnalyzerrdframe. 
There is one .h header file and one .cpp source file for it.
The class has several methods:
    - OBject corrections, event weights
    - Object selection definitions (selectElectrons, selectMuons, ...)
    - Additional derived variables definition (defineMoreVars)
    - Histogram definitions (bookHists)
    - Selections (defineCuts)
    - Plus some utility methods/functions (gen4vec, readjson, removeOverlaps, helper_1DHistCreator, createHists)

- Objects where corrections are applied, the leaf names are changed. It cannot be the same as the input tree leaf name, otherwise, it will crash.
- Users should modify: object selection definitions, define additional variables, histogram definitions, selections.
- To use the class, you need to pass a pointer to TChain object, output root file name and a JSON file.
  If there is "genWeight" branch in the ROOT Tree then the input is assumed to be MC, otherwise data.
  Look at nanoaoddataframe.cpp to find how to use within C++ 

- SkimEvents.cpp subclasses from NanoAODAnalyzerrdframe and shows how skimming could be done.
- FourtopAnalyzer.cpp  subclasses from NanoAODAnalyzerrdframe shows how analysis could be done on skimmed files.  

- Compiling
  - if using standalone: 
    - ROOT >= 6.24
    - correctionlib >=2.0.0 (https://github.com/cms-nanoAOD/correctionlib)
    - rapidjson (https://github.com/Tencent/rapidjson)
    - nlohman/json (https://github.com/nlohmann/json)
  - if CMSSW is available: the package doesn't depend on any CMSSW libraries, but some of the dependent libraries are available, so this
    makes it easier to compile.
    - CMSSW >= 12.2.x
    - ROOT >= 6.24 : CMSSW 12.2 has ROOT 6.22 but there are run time errors when using this. So you must setup ROOT 6.24.

  > make

  this will compile and create a libnanoaodrdframe.so shared library that can be loaded in a ROOT session or macro:
  gSystem->Load("libnanoadrdframe.so");
  or within pyROOT (look in processnanoaod.py).

III. Running

> submitskimjob.py jobconfigskim.py

All the options are to be set in the jobconfigskim.py. It contains 3 python directories,
one for configurations for various POG corrections, golden JSON, input and output root tree names, etc.
The second dictionary contains processing options, to split processing into
multiple jobs, produce one output file per input file, skip files already processed, 
whether to dive into subdirectories recursively and process the files there.
The third dictionary contains input directory where your input files are, output directory, and text output file 
for any print out or error messages.

As is, one output file will be produced per input file with the name "analyzed" added to
the input filename. So, we know which files are processed. The output directory 
structure will be created to match the input directory structure.
The skim job is where the corrections are applied.

> submitanalysisjob.py jobconfiganalysis.py job

Analyzes the skimmed files for analysis. Since the corrections were already applied
from previous step, no additional corrections are applied. However, if the user chooses to simply
select on the triggers for the skimming stage, one would have to apply the corrections
here. User should modify processnanoaod.py if one wishes to do so (search for skipcorrections in that file). 