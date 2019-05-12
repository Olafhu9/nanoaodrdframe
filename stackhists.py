import ROOT
import os
import sys
import tdrstyle
import CMS_lumi
import tdrstyle

STACKED=0
NORMALIZED=1

class Stackhists:
    def __init__(self, integrlumi):
        self.mcfilelist = []
        self.mclabellist = []
        self.mccolorlist = []
        self.mcpatternlist = []
        self.datafile=None
        self.datafilelist = [] # you can have more than one data file
        self.xseclist = []
        self.sflist = []

        self.mcrootfiles = []
        self.mccounterhistfiles = []
        self.mcfilecounterhistlist = []
        self.datarootfiles = []
        
        self.integrlumi = integrlumi
        # bounding box location for the legends
        self.legend_x1=0.57
        self.legend_y1=0.75
        self.legend_x2=0.87
        self.legend_y2=0.87

        self.histogramlist = []
        self.xtitles = []
        self.ytitles = []
        self.drawmodes = []
        self.drawoptions = []
        self.isLogy = []
        self.ymin = [] # histogram minimum values
        self.ymax = [] # histogram maximum values
        tdrstyle.setTDRStyle()
        pass

    def prepare(self):
        """Open ROOT files and calculate scale factors for histograms
        """

        #open ROOT files where histograms reside in MC and data
        for afile, cfile in zip(self.mcfilelist, self.mcfilecounterhistlist):
            atfile = ROOT.TFile(afile)
            if cfile=="":
                cfile = afile
            ctfile = ROOT.TFile(cfile)
            self.mcrootfiles.append(atfile)
            self.mccounterhistfiles.append(ctfile)

        """
        for afile in self.datafilelist:
            atfile = ROOT.TFile(afile)
            self.datarootfiles.append(atfile)
        """

        # calculate scaling factors for MC
        # look for 
        for afile, cfile, xsec, id in zip(self.mcfilelist, self.mcfilecounterhistlist, self.xseclist, range(len(self.xseclist))):
            atfile = ROOT.TFile(afile)
            if cfile=="":
                cfile = afile
            ctfile = ROOT.TFile(cfile)
            ahist = ctfile.Get("hcounter_nocut") # this should contain all entries before cuts
            if ahist == None:
                print("counter histogram doesn\'t exist, will proceed with histintegaral=1. Be sure to put 1/histintegral in the scalefactor!")
                self.sflist[id] *= xsec * self.integrlumi
            else:
                histintgral = ahist.Integral()
                histentries = ahist.GetEntries()
                # all histograms should be scaled by this factor
                self.sflist[id] *= xsec * self.integrlumi / histintgral 


    def setupStyle(self, colorlist=None, patternlist=None, alpha=1.0):
        self.fillalpha = alpha
        self.colorlist = []
        if colorlist == None:
            self.colorlist = [ROOT.kRed+1, ROOT.kCyan+2, ROOT.kMagenta+2, ROOT.kOrange+1, ROOT.kGreen+1, ROOT.kBlue+2,  ROOT.kSpring+2]
        else:
            self.colorlist = colorlist

        self.patternlist = []
        if patternlist == None:
            for i in range(10):
                self.patternlist.append(1001)
        else:
            self.patternlist = patternlist

        tdrstyle.setTDRStyle()
        pass

    def addChannel(self, rootfile, label, colorindex, patternindex=0, isMC=True, xsec=1.0, scalefactor=1.0, counterhistogramroot=""):
        if os.path.isfile(rootfile):
            
            if isMC:
                self.mcfilelist.append(rootfile)
                self.mcfilecounterhistlist.append(counterhistogramroot)
                self.mclabellist.append(label) # if same label, then the histograms will be added together
                self.mccolorlist.append(colorindex)
                self.mcpatternlist.append(patternindex)
                self.xseclist.append(xsec)
                self.sflist.append(scalefactor)
            else:
                self.datafilelist.append(rootfile)
        else:
            print('Cannot add file %s, it doesn\'t exist'%rootfile)
            print('Please Check')
            sys.exit(-1)
        pass

    def addHistogram(self, histname, xtitle="", ytitle="", drawmode=STACKED, drawoption="", isLogy=False, ymin=-1111, ymax=-1111):
        self.histogramlist.append(histname)
        self.xtitles.append(xtitle)
        self.ytitles.append(ytitle)
        self.drawmodes.append(drawmode)
        self.drawoptions.append(drawoption)
        self.isLogy.append(isLogy)
        self.ymin.append(ymin)
        self.ymax.append(ymax)

    def draw(self):
        self.prepare()
        for histname, xtitle, ytitle, mode, drawoption, isLogy, ymin, ymax in zip(self.histogramlist, self.xtitles, self.ytitles, self.drawmodes, self.drawoptions, self.isLogy, self.ymin, self.ymax):
            self.createStacks(histname, xtitle, ytitle, mode, drawoption, isLogy, ymin, ymax)

    def createStacks(self, histname, xtitle, ytitle, mode, option="", isLogy=False, ymin=-1111, ymax=-1111):
        
        # now stack
        
        hs = ROOT.THStack()
        tl = ROOT.TLegend(self.legend_x1, self.legend_y1, self.legend_x2, self.legend_y2)
        histgroup = dict()
        labellist = []

        mchistsum = None

        for ifile in range(len(self.mcfilelist)):
            #afile = ROOT.TFile(self.mcfilelist[ifile])
            #ahist = afile.Get(histname)
            ahist = self.mcrootfiles[ifile].Get(histname)

            if ahist == None:
                print("histogram %s not found in %s"%(histname, self.mcfilelist[ifile]))
                print("quitting")
                sys.exit(-1)
            else:
                ahist.Scale(self.sflist[ifile])

                if mchistsum == None:
                    mchistsum = ahist.Clone("mchistsum")
                else:
                    mchistsum.Add(ahist)
                ahist.SetFillColorAlpha(self.colorlist[self.mccolorlist[ifile]], self.fillalpha)
                ahist.SetLineColor(self.colorlist[self.mccolorlist[ifile]])
                ahist.SetFillStyle(self.patternlist[self.mcpatternlist[ifile]])
                #ahist.UseCurrentStyle()
                # group by labels
                label = self.mclabellist[ifile]
                if label not in histgroup:
                    histgroup[label] = ahist
                    labellist.append(label) # need to take care of the order
                else:
                    histgroup[label].Add(ahist)
        for label in labellist:
            ahist = histgroup[label]
            print(label+" : %f"%ahist.Integral())
            if mode == NORMALIZED:
                ahistcopy = ahist.Clone()
                normscale = ahistcopy.Integral()
                ahistcopy.Scale(1.0/normscale)
                hs.Add(ahistcopy)
                tl.AddEntry(ahistcopy, label, "F")
            else:
                hs.Add(ahist)
                tl.AddEntry(ahist, label, "F")

        finaldatahist = None
        for ifile in self.datafilelist:
            atfile = ROOT.TFile(ifile)
            ahist = atfile.Get(histname)
            if ahist is None:
                print("histogram %s not found in %s"%(histname, self.datafilelist[ifile]))
                sys.exit(-1)
            else:
                if finaldatahist == None:
                    finaldatahist = ahist.Clone("finaldata")
                else:
                    finaldatahist.Add(ahist)
        print("Data : %f"%finaldatahist.Integral())
        if mode == NORMALIZED:
            normscale = finaldatahist.Integral()
            finaldatahist.Scale(1.0/normscale)

        c1 = ROOT.TCanvas("c1", "c1", 600, 700)

        """
        c1.SetLeftMargin(0.15)
        c1.SetBottomMargin(0.15)
        c1.SetTopMargin(0.06)
        c1.SetRightMargin(0.10)
        """

        c1_top = ROOT.TPad("c1_top", "top", 0.01, 0.33, 0.99, 0.99)
        c1_top.Draw()
        c1_top.cd()
        c1_top.SetTopMargin(0.1)
        c1_top.SetBottomMargin(0.01)
        c1_top.SetRightMargin(0.1)
        
        # log y scale
        if isLogy:
            c1_top.SetLogy(isLogy)

        if mode == STACKED:
            hs.Draw(option)
        else:
            hs.Draw("nostack " + option)
        xaxis = hs.GetXaxis()
        xaxis.SetTitle(xtitle)
        xaxis.SetNdivisions(6,5,0)
        #xaxis.SetMaxDigits(4)

        # Set vertical range
        if ymin != -1111:
            hs.SetMinimum(ymin)
        if ymax != -1111:
            hs.SetMaximum(ymax)
        elif isLogy:
            hs.SetMaximum(hs.GetMaximum()*100.0)
        

        finaldatahist.SetMarkerStyle(ROOT.kFullCircle)
        finaldatahist.Draw("sameerr P")

        yaxis = hs.GetYaxis()
        yaxis.SetTitle(ytitle)
        yaxis.SetNdivisions(6,5,0)
        yaxis.SetMaxDigits(3)
        

        # Ratio plot
        hstackhist = mchistsum
        
        ratiohist = finaldatahist.Clone("ratiohist")
        ratiohist.Divide(hstackhist)
        ratiohist.SetMinimum(0.3)
        ratiohist.SetMaximum(1.7)


        tl.Draw()
        c1_top.Modified()
        CMS_lumi.CMS_lumi(c1_top, 4, 11)
        c1_top.cd()
        c1_top.Update()
        c1_top.RedrawAxis()

        c1.cd()
        c1_bottom = ROOT.TPad("c1_bottom", "bottom", 0.01, 0.01, 0.99, 0.32)
        c1_bottom.Draw()
        c1_bottom.cd()
        c1_bottom.SetTopMargin(0.02)
        c1_bottom.SetBottomMargin(0.3)
        c1_bottom.SetRightMargin(0.1)
        c1_bottom.SetGridx(1)
        c1_bottom.SetGridy(1)
        ratiohist.Draw("err")

        xaxis = ratiohist.GetXaxis()
        xaxis.SetTitle(xtitle)
        xaxis.SetNdivisions(6,5,0)
        xaxis.SetTitleSize(0.12)
        xaxis.SetLabelSize(0.10)

        yaxis = ratiohist.GetYaxis()
        yaxis.SetTitle("Data/Model")
        yaxis.SetNdivisions(6,5,0)
        yaxis.SetTitleSize(0.1)
        yaxis.SetLabelSize(0.08)
        yaxis.SetTitleOffset(0.7)
        yaxis.SetLabelOffset(0.007)

        c1_bottom.Modified()
        c1_bottom.RedrawAxis()

        frame = c1.GetFrame()
        frame.Draw()
        c1.SaveAs(histname+".pdf")

        pass