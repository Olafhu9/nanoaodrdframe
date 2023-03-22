from correctionlib import _core

#Download the correct JSON files 
evaluator = _core.CorrectionSet.from_file('electron.json.gz')

#Reconstruction (pT< 20 GeV) Run-2 scale factor
valsf= evaluator["UL-Electron-ID-SF"].evaluate("2018","sf","Tight",1.1, 30.0)
print("sf is:"+str(valsf))

#Reconstruction (pT> 20 GeV) Run-2 scale factor
valsf= evaluator["UL-Electron-ID-SF"].evaluate("2018","sfup","Tight",1.1, 30.0)
print("sf is:"+str(valsf))

valsf= evaluator["UL-Electron-ID-SF"].evaluate("2018","sfdown","Tight",1.1, 30.0)
print("sf is:"+str(valsf))
