import stackhists
import CMS_lumi

s = stackhists.Stackhists(35.9)
s.setupStyle(alpha=0.5)
CMS_lumi.extraText = "Work in progress"

s.addChannel('2016data_00000.root', '2016 data', 2, isMC=False)
s.addChannel("tttt_00000.root", "Four top", 0, isMC=True, xsec=9.2, counterhistogramroot="tttt.root")
s.addChannel("tt_00000.root", "tt-bar", 1, isMC=True, xsec=836000.0, counterhistogramroot="tt.root")
s.addChannel("qcd_500_700_00000.root", "qcd", 2, isMC=True, xsec=32060000.0, counterhistogramroot="qcd_500_700.root")
s.addChannel("qcd_700_1000_00000.root", "qcd", 2, isMC=True, xsec=6829000.0, counterhistogramroot="qcd_700_1000.root")
s.addChannel("qcd_1000_1500_00000.root", "qcd", 3, isMC=True, xsec=1207000.0, counterhistogramroot="qcd_1000_1500.root")
s.addChannel("qcd_1500_2000_00000.root", "qcd", 4, isMC=True, xsec=120000.0, counterhistogramroot="qcd_1500_2000.root")
s.addChannel("qcd_2000_Inf_00000.root", "qcd", 5, isMC=True, xsec=25250.0, counterhistogramroot="qcd_2000_Inf.root")

"""
s.addChannel("tttt.root", "Four top", 0, isMC=True, xsec=9.2, scalefactor=1.0/1136652.9)
s.addChannel("tt.root", "tt-bar", 1, isMC=True, xsec=836000.0, scalefactor=1.0/30358784.)
s.addChannel("qcd_500_700.root", "qcd", 2, isMC=True, xsec=32060000.0, scalefactor=1.0/20081874.0)
s.addChannel("qcd_700_1000.root", "qcd", 2, isMC=True, xsec=6829000.0, scalefactor=1.0/16910375.)
s.addChannel("qcd_1000_1500.root", "qcd", 3, isMC=True, xsec=1207000.0, scalefactor=1.0/5314861.2)
s.addChannel("qcd_1500_2000.root", "qcd", 4, isMC=True, xsec=120000.0, scalefactor=1.0/4546002.2)
s.addChannel("qcd_2000_Inf.root", "qcd", 5, isMC=True, xsec=25250.0, scalefactor=1.0/2320643.0)
"""


s.addHistogram("hjetHT_cut00000", "GeV", "Entries", drawmode=stackhists.STACKED, drawoption="hist", isLogy=True, ymin=0.1)
#s.addHistogram("hsphericity_cut4", "Sphericity", "Entries", drawmode=stackhists.STACKED, drawoption="hist", isLogy=True, ymin=0.1)
#s.addHistogram("hncleanjetpass_cut4", "N_\{jets\}", "Entries", drawmode=stackhists.STACKED, drawoption="hist", isLogy=True, ymin=0.1)
s.addHistogram("hlegendreP1_cut00000", "P1", "Entries", drawmode=stackhists.NORMALIZED, drawoption="hist", isLogy=False)
s.addHistogram("hlegendreP2_cut00000", "P2", "Entries", drawmode=stackhists.NORMALIZED, drawoption="hist", isLogy=False)
s.addHistogram("hlegendreP3_cut00000", "P3", "Entries", drawmode=stackhists.NORMALIZED, drawoption="hist", isLogy=False)
s.addHistogram("hlegendreP4_cut00000", "P4", "Entries", drawmode=stackhists.NORMALIZED, drawoption="hist", isLogy=False)
s.addHistogram("hlegendreP5_cut00000", "P5", "Entries", drawmode=stackhists.NORMALIZED, drawoption="hist", isLogy=False)
s.addHistogram("hlegendreP6_cut00000", "P6", "Entries", drawmode=stackhists.NORMALIZED, drawoption="hist", isLogy=False)
s.addHistogram("hlegendreP7_cut00000", "P7", "Entries", drawmode=stackhists.NORMALIZED, drawoption="hist", isLogy=False)
s.addHistogram("hlegendreP10_cut00000", "P10", "Entries", drawmode=stackhists.NORMALIZED, drawoption="hist", isLogy=False)
s.addHistogram("hlegendreP20_cut00000", "P20", "Entries", drawmode=stackhists.NORMALIZED, drawoption="hist", isLogy=False)
s.addHistogram("hlegendreP30_cut00000", "P30", "Entries", drawmode=stackhists.NORMALIZED, drawoption="hist", isLogy=False)
s.addHistogram("hlegendreP1b_cut00000", "P1b", "Entries", drawmode=stackhists.NORMALIZED, drawoption="hist", isLogy=False)
s.addHistogram("hlegendreP2b_cut00000", "P2b", "Entries", drawmode=stackhists.NORMALIZED, drawoption="hist", isLogy=False)
s.addHistogram("hlegendreP3b_cut00000", "P3b", "Entries", drawmode=stackhists.NORMALIZED, drawoption="hist", isLogy=False)
s.addHistogram("hlegendreP4b_cut00000", "P4b", "Entries", drawmode=stackhists.NORMALIZED, drawoption="hist", isLogy=False)
s.addHistogram("hlegendreP5b_cut00000", "P5b", "Entries", drawmode=stackhists.NORMALIZED, drawoption="hist", isLogy=False)
s.draw()

