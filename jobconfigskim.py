"""
File contains job options 
"""

# options for Nanoaodrdframe
config = {
        # tree name of input file(s)
        'intreename': "Events", 
        # tree name of output file(s) it cannot be the same as the input tree name or it'll crash
        'outtreename': "outputTree", 
        # good json file
        'goodjson': 'data/Cert_294927-306462_13TeV_EOY2017ReReco_Collisions17_JSON.txt',
        # pileup weight for MC
        #'pileupfname': 'data/LUM/2017_UL/puWeights.json.gz',
        'pileupfname': 'data/LUM/2016postVFP_UL/puWeights.json.gz',
        #'pileuptag': 'Collisions17_UltraLegacy_goldenJSON',
        'pileuptag': 'Collisions16_UltraLegacy_goldenJSON',
        # json filename for BTV correction
        #'btvfname': 'data/BTV/2017_UL/btagging.json.gz',
        'btvfname': 'data/BTV/2016postVFP_UL/btagging.json.gz',
        # BTV correction type
        'btvtype': 'deepJet_shape',
        # json file name for JERC
        #'jercfname': 'data/JERC/UL17_jerc.json.gz',
        'jercfname': 'data/JERC/UL16postVFP_jerc.json.gz',
        # conbined correction type for jets
        #'jerctag': 'Summer19UL17_V5_MC_L1L2L3Res_AK4PFchs', 
        'jerctag': 'Summer19UL16_V7_MC_L1L2L3Res_AK4PFchs', 
        # jet uncertainty 
        #'jercunctag': 'Summer19UL17_V5_MC_Total_AK4PFchs', 
        'jercunctag': 'Summer19UL16_V7_MC_Total_AK4PFchs', 
        }

# processing options
procflags = {
        # how many jobs?
        'split': 2, 
        # if False, one output file per input file, if True then one output file for everything
        'allinone': False, 
        # if True then skip existing analyzed files
        'skipold': False,
        # travel through the subdirectories and their subdirecties when processing.
        # becareful not to mix MC and real DATA in them.
        'recursive': True,
        # if False then only selected branches which is done in the .cpp file will be saved
        'saveallbranches': False,
        }

# input directory where your input root tree resides
# output directory wheere your output should go
# dump of stderr/stdout to file
nanoaod_inputdir_outputdir_pairs = [
    ['testinputdata/MC/2016', 'processed/2016', 'stderrout.out' ],
    #['processed/','analyzed/tt.root', 'stderrout.out' ],
]

