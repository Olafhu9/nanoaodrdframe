from ROOT import *
import os
import csv
import numpy as np
import sys
import math

path = sys.argv[1]
pname = path.replace("/","")
print(pname)
runs = ["16pre", "16post", "17", "18"]
flist = {}
for run in runs:
    flist[run] = [i for i in sorted(os.listdir(path+"/"+run)) if ".root" in i]
    for j in flist[run]:
        if "DYJetsToLL_M-50_madgraph" in j:
            flist[run].remove(j)

# !! Check Selection Steps !!
steps = 5

# Cross Sections (pb)
Xsec_dict = {"DYJetsToLL_M-10to50" : 18610.00,
        "DYJetsToLL_M-50_amcatnlo" : 6077.22,
        #"DYJetsToLL_M-50_madgraph" : 6077.22,
        "WJetsToLNu_madgraphMLM" : 61526.7, 
        "WJetsToLNu_inclHT100" : 59373.3,
        "WJetsToLNu_HT-0To100" : 59373.3,
        "WJetsToLNu_HT-100To200" : 1345 * 0.993, 
        "WJetsToLNu_HT-200To400" : 359.7 * 1.002, 
        "WJetsToLNu_HT-400To600" : 48.91 * 1.009, 
        "WJetsToLNu_HT-600To800" : 12.05 * 1.120, 
        "WJetsToLNu_HT-800To1200" : 5.501 * 1.202, 
        "WJetsToLNu_HT-1200To2500" : 1.329 * 1.332, 
        "WJetsToLNu_HT-2500ToInf" : 0.03216 * 4.200, 
        "ST_t-channel_top" : 136.02, 
        "ST_t-channel_antitop" : 80.95, 
        "ST_tW_top" : 35.85, 
        "ST_tW_antitop" : 35.85, 
        "TTTo2L2Nu" : 88.29, 
        "TTToHadronic" : 377.96, 
        "TTToSemiLeptonic" : 365.34, 
        "ST_LFV_TCMuTau_Scalar" : 7.40 * 1E-03,
        "ST_LFV_TCMuTau_Vector" : 36.8 * 1E-03,
        "ST_LFV_TCMuTau_Tensor" : 178.4 * 1E-03,
        "ST_LFV_TUMuTau_Scalar" : 83.8 * 1E-03,
        "ST_LFV_TUMuTau_Vector" : 393 * 1E-03,
        "ST_LFV_TUMuTau_Tensor" : 1796 * 1E-03,
        "TT_LFV_TToCMuTau_Scalar" : 2.688 * 1E-03,
        "TT_LFV_TToCMuTau_Vector" : 21.50 * 1E-03,
        "TT_LFV_TToCMuTau_Tensor" : 129.0 * 1E-03,
        "TT_LFV_TToUMuTau_Scalar" : 2.688 * 1E-03,
        "TT_LFV_TToUMuTau_Vector" : 21.50 * 1E-03,
        "TT_LFV_TToUMuTau_Tensor" : 129.0 * 1E-03,
        "WW" : 118.7,
        "WZ" : 47.13,
        "ZZ" : 16.523,
        "TTWJetsToLNu" : 0.2043, 
        "TTWJetsToQQ" : 0.4062, 
        "TTZToLLNuNu" : 0.2529, 
        "TTZToQQ" : 0.5297, 
        "QCD_Pt-15To20" : 1273190000 * 0.003 ,
        "QCD_Pt-20To30" : 558528000 * 0.0053 ,
        "QCD_Pt-30To50" : 139803000 * 0.01182 ,
        "QCD_Pt-50To80" : 19222500 * 0.02276 ,
        "QCD_Pt-80To120" : 2758420 * 0.03844 ,
        "QCD_Pt-120To170" : 469797 * 0.05362 ,
        "QCD_Pt-170To300" : 117989 * 0.07335 ,
        "QCD_Pt-300To470" : 7820.25 * 0.10196 ,
        "QCD_Pt-470To600" : 645.528 * 0.12242 ,
        "QCD_Pt-600To800" : 187.109 * 0.13412 ,
        "QCD_Pt-800To1000" : 32.3486 * 0.14552 ,
        "QCD_Pt-1000" : 10.4305 * 0.15544 ,
        } # pb
lumi=None

def run(run):
    if run=="16pre":
        lumi = 19.5 # fb^(-1)
    if run=="16post":
        lumi = 16.8 # fb^(-1)
    elif run=="17":
        lumi = 41.48 # fb^(-1)
    elif run=="18":
        lumi = 59.83 # fb^(-1)
    print(run)
    list_bkgs = [[0 for _ in range(steps+1)],[0 for _ in range(steps+1)]]
    LFV_STc_s = list_bkgs
    LFV_STc_v = list_bkgs
    LFV_STc_t = list_bkgs
    LFV_STu_s = list_bkgs
    LFV_STu_v = list_bkgs
    LFV_STu_t = list_bkgs
    LFV_TTc_s = list_bkgs
    LFV_TTc_v = list_bkgs
    LFV_TTc_t = list_bkgs
    LFV_TTu_s = list_bkgs
    LFV_TTu_v = list_bkgs
    LFV_TTu_t = list_bkgs
    ttdi = list_bkgs
    ttsemi = list_bkgs
    tthad = list_bkgs
    DY = list_bkgs
    ST = list_bkgs
    TTX = list_bkgs
    VV = list_bkgs
    WJets = list_bkgs
    QCD = list_bkgs
    DATA = list_bkgs
    for rfile in flist[run]:
        nevts_integral = []
        nevts_entry = []
        rname = str(rfile.split('.')[0])
        data = TFile.Open(path+"/"+run+"/"+rfile)
        h_count = data.Get("hcounter_nocut")
        if not h_count:
            print("Not a valid file : %s"%(rfile))
            continue
        if ("WJetsToLNu_inclHT100" in rfile) or ("WJetsToLNu_HT-0To100" in rfile):
            nevts_integral.append(h_count.Integral()*0.96)
            nevts_entry.append(h_count.GetEntries()*0.96)
        else:
            nevts_integral.append(h_count.Integral())
            nevts_entry.append(h_count.GetEntries())
        for j in range(steps):
            if j<4:
                nevts_integral.append(data.Get("hnevents_pglep_cut"+str(0)*(j+1)).Integral())
                nevts_entry.append(data.Get("hnevents_pglep_cut"+str(0)*(j+1)).GetEntries())
            else:
                nevts_integral.append(data.Get("hnevents_cut"+str(0)*(j+1)).Integral())
                nevts_entry.append(data.Get("hnevents_cut"+str(0)*(j+1)).GetEntries())

        # Rescale for conservation of event yields when btagging SF is applied.
        prehist = data.Get("hnevents_pglep_cut0000")
        posthist = data.Get("hnevents_cut0000")
        rescale = prehist.Integral() / posthist.Integral() if posthist.GetEntries() != 0 else 1.0
        #resflist = [1.0, 1.0, 1.0, rescale, rescale, rescale]
        resflist = [1.0, 1.0, 1.0, 1.0, 1.0, rescale]
        data.Close() # rootfile closed

        nevts_squarestaterr=nevts_entry
        if not "Run" in rfile:
            rfilename = rfile.split('.')[0]
            dictname = rfilename.split('_'+run)[0]
            norm_nevts = [ Xsec_dict[dictname] * (10**3) * lumi * i / nevts_integral[0] for i in nevts_integral ]
            norm_squarestaterr = [ math.pow(Xsec_dict[dictname] * (10**3) * lumi / nevts_integral[0], 2) * i for i in nevts_squarestaterr ]
            norm_nevts = np.multiply(norm_nevts, resflist).tolist()
        else:
            norm_nevts = nevts_integral
            norm_squarestaterr = nevts_squarestaterr
        tmp_proc = []
        tmp_proc.append(norm_nevts)
        tmp_proc.append(norm_squarestaterr)
        
        if "ST_LFV_TC" in rname:
            if "Scalar" in rname:
                LFV_STc_s = np.add(LFV_STc_s, tmp_proc).tolist()
            elif "Vector" in rname:
                LFV_STc_v = np.add(LFV_STc_v, tmp_proc).tolist()
            elif "Tensor" in rname:
                LFV_STc_t = np.add(LFV_STc_t, tmp_proc).tolist()
        elif "ST_LFV_TU" in rname:
            if "Scalar" in rname:
                LFV_STu_s = np.add(LFV_STu_s, tmp_proc).tolist()
            elif "Vector" in rname:
                LFV_STu_v = np.add(LFV_STu_v, tmp_proc).tolist()
            elif "Tensor" in rname:
                LFV_STu_t = np.add(LFV_STu_t, tmp_proc).tolist()
        elif "TT_LFV_TToC" in rname:
            if "Scalar" in rname:
                LFV_TTc_s = np.add(LFV_TTc_s, tmp_proc).tolist()
            elif "Vector" in rname:
                LFV_TTc_v = np.add(LFV_TTc_v, tmp_proc).tolist()
            elif "Tensor" in rname:
                LFV_TTc_t = np.add(LFV_TTc_t, tmp_proc).tolist()
        elif "TT_LFV_TToU" in rname:
            if "Scalar" in rname:
                LFV_TTu_s = np.add(LFV_TTu_s, tmp_proc).tolist()
            elif "Vector" in rname:
                LFV_TTu_v = np.add(LFV_TTu_v, tmp_proc).tolist()
            elif "Tensor" in rname:
                LFV_TTu_t = np.add(LFV_TTu_t, tmp_proc).tolist()
        else:
            if ("Run" in rname):
                DATA = np.add(DATA, tmp_proc).tolist()
            elif "TTTo2L2Nu" in rname:
                ttdi = np.add(ttdi, tmp_proc).tolist()
            elif "TTToSemi" in rname:
                ttsemi = np.add(ttsemi, tmp_proc).tolist()
            elif "TTToHad" in rname:
                tthad = np.add(tthad, tmp_proc).tolist()
            elif "DY" in rname:
                DY = np.add(DY, tmp_proc).tolist()
            elif "ST" in rname:
                ST = np.add(ST, tmp_proc).tolist()
            elif ("TTW" in rname) or ("TTZ" in rname):
                TTX = np.add(TTX, tmp_proc).tolist()
            elif ("WW" in rname) or ("WZ" in rname) or ("ZZ" in rname):
                VV = np.add(VV, tmp_proc).tolist()
            elif "WJets" in rname:
                WJets = np.add(WJets, tmp_proc).tolist()
            elif ("QCD" in rname):
                QCD = np.add(QCD, tmp_proc).tolist()

    LFV = [LFV_STc_s,LFV_STc_v,LFV_STc_t,LFV_STu_s,LFV_STu_v,LFV_STu_t,LFV_TTc_s,LFV_TTc_v,LFV_TTc_t,LFV_TTu_s,LFV_TTu_v,LFV_TTu_t]
    totalMC = np.add(np.add(np.add(np.add(np.add(np.add(np.add(np.add(ttdi,ttsemi),tthad),DY),ST),TTX),VV),WJets),QCD).tolist()
    #totalMC = np.add(np.add(np.add(np.add(np.add(np.add(np.add(ttdi,ttsemi),tthad),DY),ST),TTX),VV),WJets)
    dataMC = np.divide(DATA[0],totalMC[0]).tolist()
    dataMC_error = np.divide(np.sqrt(totalMC[1]),DATA[0]).tolist()
    dataMC = np.append(dataMC,dataMC_error).reshape(2,steps+1).tolist()
    snb = [0 for _ in range(len(LFV))]
    acc = [0 for _ in range(len(LFV))]
    tmpCutflow = []
    tmpCutflow = [LFV_STc_s,LFV_STc_v,LFV_STc_t,LFV_STu_s,LFV_STu_v,LFV_STu_t,
            LFV_TTc_s,LFV_TTc_v,LFV_TTc_t,LFV_TTu_s,LFV_TTu_v,LFV_TTu_t,
            #ttdi, ttsemi, tthad, DY, ST, WJets, TTX, VV,
            ttdi, ttsemi, tthad, DY, ST, WJets, TTX, VV, QCD,
            totalMC, DATA, dataMC]
    for i in range(len(LFV)):
        if LFV[i][0][0] == 0: continue
        snb = np.divide(LFV[i][0],np.sqrt(totalMC[0]))
        snb.resize(2,steps+1)
        tmpCutflow.append(snb.tolist())

    for i in range(len(LFV)):
        if LFV[i][0][0] == 0: continue
        acc = np.array([j/LFV[i][0][0] for j in LFV[i][0]])
        acc.resize(2,steps+1)
        tmpCutflow.append(acc.tolist())
    
    nsig = len(LFV)
    nbkg = len(tmpCutflow) - 3*nsig - 2
    return tmpCutflow, nsig, nbkg
# Cutflow list shape : (48, 2, 6)
# LFV       = cutflow[0:12]
# MC        = cutflow[12:21]
# totalMC   = cutflow[21]
# DATA      = cutflow[22]
# dataMC    = cutflow[23]
# Sig       = cutflow[24:36]
# Acc       = cutflow[36:48]

# no qcd version
# LFV       = cutflow[0:12]
# MC        = cutflow[12:20]
# totalMC   = cutflow[20]
# DATA      = cutflow[21]
# dataMC    = cutflow[22]
# Sig       = cutflow[23:35]
# Acc       = cutflow[35:47]

def formatCutflow(cutflow, nsig, nbkg):
    # Format
    # LFV vec   = cutflow[0:3]
    # MC        = cutflow[3:11]
    # totalMC   = cutflow[11]
    # DATA      = cutflow[12]
    # dataMC    = cutflow[13]
    # Sig       = cutflow[14:18]
    # Acc       = cutflow[18:22]
    out=[]
    cut1=[]
    cut2=[]
    cut3=[]
    cut4=[]
    nmc = nsig + nbkg
    for p in cutflow[1:nsig:3]+cutflow[nsig:nmc]:
        l=[]
        for i in range(len(p[0][1:])):
            j = p[0][i+1]
            k = math.sqrt(p[1][i+1])
            if(len(cut1)<nmc):
                l.append( '{:,.3g}'.format(j) if j < 100 else '{:,.0f}'.format(j) )
                l.append( '{:,.3f}'.format(k) if k < 1 else '{:,.3g}'.format(k) if k<100 else '{:,.0f}'.format(k) )
            elif(len(cut1)==nmc):
                l.append( '{:,.2f}'.format(j) if j < 1 else '{:,.3f}'.format(j) )
                l.append( '{:,.2f}'.format(k) if k < 1 else '{:,.3f}'.format(k) )
            else:
                l.append('{:.2f}'.format(j))
                l.append('{:.2f}'.format(k))
        cut1.append(l)

    cut2=[ '{:,.3g}'.format(j) if j < 100 else '{:,.0f}'.format(j) for j in cutflow[nmc][0][1:] ]

    for i in range(len(cutflow[nmc+1][0][1:])):
        j = cutflow[nmc+1][0][i+1]
        k = cutflow[nmc+1][1][i+1]
        cut3.append('{:.2f}'.format(j))
        cut3.append('{:.2f}'.format(k))
    for p in cutflow[nmc+2:nmc+nsig+1:3]+cutflow[nmc+nsig+2:nmc+2*nsig+1:3]:
        l=[ "{:.2f}".format(j) if j>1 else "{:.3f}".format(j) for j in p[0]]
        cut4.append(l[1:])
    out=np.concatenate((np.array(cut1).flatten(),np.array(cut2).flatten(),np.array(cut3).flatten(),np.array(cut4).flatten()))
    return out

run2cutflow = np.zeros((48,2,steps+1))
cutflows = []           # Full Lists (shape (48,2,5))
formattedcutflows = []  # Formatted for Cutflow Table (shape (1,))
nsig = -1
nbkg = -1
for year in runs:
    cutflow, nsig, nbkg = run(year)
    cutflows.append(cutflow)
    formattedcutflows.append(formatCutflow(cutflow,nsig,nbkg))
    run2cutflow = np.add(run2cutflow,cutflow)
run2cutflow=run2cutflow.tolist()
cutflows.append(run2cutflow)
nmc = nsig + nbkg
# Calculation data/mc ratio for Integrated Run II
run2dataMC = np.divide(run2cutflow[nmc][0],run2cutflow[nmc-1][0]).tolist()
run2dataMC_error = np.divide(np.sqrt(run2cutflow[nmc-1][1]),run2cutflow[nmc][0]).tolist()
run2dataMC = np.append(run2dataMC,run2dataMC_error).reshape(2,steps+1).tolist()
run2cutflow[nmc+1] = run2dataMC

for i in range(nsig):
    run2cutflow[i+nmc+2] = np.divide(run2cutflow[i][0],np.sqrt(run2cutflow[nmc-1])).tolist()
for i in range(nsig):
    tmpacc = np.array([j/run2cutflow[i][0][0] for j in run2cutflow[i][0]])
    tmpacc.resize(2,steps+1)
    run2cutflow[i+nmc+nsig+1] = tmpacc.tolist()
formattedcutflows.append(formatCutflow(run2cutflow,nsig,nbkg))

def texScript(run,formattedcutflow):
    h1 = "\\begin{table}[!hp]\n    \\tiny\n    \centering\n"
    h2 = "    \\renewcommand{\\arraystretch}{1.2}\n    \\begin{tabular}{c|r|r|r|r|r} \\hline\\hline \n"
    h3 = "    \\textbf{"+run+" (%s~\\fbinv)} & \\textbf{One muon} & \\textbf{One tau} & \\textbf{$m_{\\mu\\tau}>150\GeV$, OS} & \\textbf{nJets$\geq$3} & \\textbf{One b-tagged Jet} \\\\ \hline \n"%(lumi)
    s = "    LFV ST $tc\\mu\\tau$  " + " & {} $\\pm$ {}"*steps + " \\\\ \n\
    LFV ST $tu\\mu\\tau$  " + " & {} $\\pm$ {}"*steps + " \\\\ \n\
    LFV TT $tc\\mu\\tau$  " + " & {} $\\pm$ {}"*steps + " \\\\ \n\
    LFV TT $tu\\mu\\tau$  " + " & {} $\\pm$ {}"*steps + " \\\\ \hline \n\
    \\ttbar dileptonic   " + " & {} $\\pm$ {}"*steps + " \\\\ \n\
    \\ttbar semileptonic " + " & {} $\\pm$ {}"*steps + " \\\\ \n\
    \\ttbar hadronic     " + " & {} $\\pm$ {}"*steps + " \\\\ \n\
    DY                  " + " & {} $\\pm$ {}"*steps + " \\\\ \n\
    Single Top          " + " & {} $\\pm$ {}"*steps + " \\\\ \n\
    W+Jets              " + " & {} $\\pm$ {}"*steps + " \\\\ \n\
    \\ttbar + X          " + " & {} $\\pm$ {}"*steps + " \\\\ \n\
    VV                  " + " & {} $\\pm$ {}"*steps + " \\\\ \n\
    QCD                 " + " & {} $\\pm$ {}"*steps + " \\\\ \hline \n\
    Total MC            " + " & {} $\\pm$ {}"*steps + " \\\\ \hline \n\
    DATA                " + " & {}"*steps + " \\\\ \hline \n\
    DATA / MC           " + " & {} $\\pm$ {}"*steps + " \\\\ \hline \n\
    S/$\sqrt{{B}}$~(ST $tc\\mu\\tau$)" + " & {}"*steps+" \\\\ \n\
    S/$\sqrt{{B}}$~(ST $tu\\mu\\tau$)" + " & {}"*steps+" \\\\ \n\
    S/$\sqrt{{B}}$~(TT $tc\\mu\\tau$)" + " & {}"*steps+" \\\\ \n\
    S/$\sqrt{{B}}$~(TT $tu\\mu\\tau$)" + " & {}"*steps+" \\\\ \hline \n\
    Acceptance~(ST $tc\\mu\\tau$)" + " & {}"*steps+" \\\\ \n\
    Acceptance~(ST $tu\\mu\\tau$)" + " & {}"*steps+" \\\\ \n\
    Acceptance~(TT $tc\\mu\\tau$)" + " & {}"*steps+" \\\\ \n\
    Acceptance~(TT $tu\\mu\\tau$)" + " & {}"*steps+" \\\\ \hline \hline\n"
    s=s.format(*formattedcutflow)
    #QCD                 " + " & {} $\\pm$ {}"*steps + " \\\\ \n\
    b1 = "    \\end{tabular}\n"
    b2 = "    \\caption{Cutflow Table for %s data and MC samples}\n"%(run)
    b3 = "    \\label{tab:STcutflow%s_%s}\n"%(run,pname)
    b4 = "\\end{table}"

    h=h1+h2+h3
    b=b1+b2+b3+b4
    script = h+s+b
    return script

run2s = [ "Run"+i for i in runs ]
if len(runs) == 4:
    run2s.append("Run2")
#run2s=["Run16pre","Run16post","Run17","Run18","Run2"]
for run, cutflow in zip(run2s, formattedcutflows):
    if run=="Run16pre":
        lumi = 19.5 # fb^(-1)
    elif run=="Run16post":
        lumi = 16.8 # fb^(-1)
    elif run=="Run17":
        lumi = 41.5 # fb^(-1)
    elif run=="Run18":
        lumi = 59.8 # fb^(-1)
    elif run=="Run2":
        lumi = 138 # fb^(-1) 
    x=texScript(run,cutflow)
    print(x)

#datacard_path = "./datacards/"+pname
#if not os.path.isdir(datacard_path):
#    os.makedirs(datacard_path)
#
#def writeDatacard(run,cutflow):
#    LFV = [i[0] for i in cutflow[0:12]]
#    TT = np.add(np.add(cutflow[12][0],cutflow[13][0]),cutflow[14][0]).tolist()
#    DY = cutflow[15][0]
#    ST = cutflow[16][0]
#    WJets = cutflow[17][0]
#    Oth = np.add(cutflow[18][0],cutflow[19][0])
#    #Oth = np.add(np.add(cutflow[18][0],cutflow[19][0]),cutflow[20][0])
#    DATA = cutflow[21][0]
#    N=0
#    for j in ['c','u']:
#        for k in ['s','v','t']:
#            name = j+'_'+k
#            with open(datacard_path+'/datacard_countingLFV'+name+'_'+run+'.txt','w') as f_tmp:
#                script="""
#imax    1 number of bins
#jmax    6 number of processes minus 1
#kmax    * number of nuisance parameters
#----------------------------------------------------------------------------------------------------
#----------------------------------------------------------------------------------------------------
#bin             bin1
#observation     {0:<12}
#----------------------------------------------------------------------------------------------------
#bin             bin1        bin1        bin1        bin1        bin1        bin1        bin1
#process         lfv_st      lfv_tt      TT          DY          ST          Wjet        Oth
#process         0           -1          1           2           3           4           5
#rate            {1:<12.3f}{2:<12.3f}{3:<12.3f}{4:<12.3f}{5:<12.3f}{6:<12.3f}{7:<12.3f}
#----------------------------------------------------------------------------------------------------
#lumieff18 lnN   1.025       1.025       1.025       1.025       1.025       1.025       1.025
#            """.format(DATA[steps], LFV[N][steps], LFV[N+6][steps], TT[steps], DY[steps], ST[steps], WJets[steps], Oth[steps])
#                f_tmp.write(str(script))
#                f_tmp.close()
#            N=N+1
#    return
#
#for run, cutflow in zip(run2s, cutflows):
#    writeDatacard(run,cutflow)
