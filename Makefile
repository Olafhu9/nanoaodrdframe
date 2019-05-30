rootlibs:=$(shell root-config --libs)
rootflags:=$(shell root-config --cflags)

SOFLAGS       = -shared

LD = g++ -g -Wall

CXXFLAGS = -O2 -g -Wall -fmessage-length=0 $(rootflags) -fpermissive -fPIC -pthread -DSTANDALONE -I.

OBJDIR=src
SRCDIR=src
SRCS := $(wildcard $(SRCDIR)/*.cpp)
OBJS := $(patsubst %.cpp,%.o,$(SRCS)) $(SRCDIR)/JetMETObjects_dict.o $(SRCDIR)/rootdict.o 

LIBS = $(rootlibs) -lMathMore -lGenVector -ljsoncpp

TARGET =	nanoaodrdataframe

all:	$(TARGET) libnanoadrdframe.so 

clean:
	rm -f $(OBJS) $(TARGET) libnanoaodrdframe.so $(SRCDIR)/JetMETObjects_dict.C $(SRCDIR)/rootdict.C JetMETObjects_dict_rdict.pcm rootdict_rdict.pcm

$(SRCDIR)/rootdict.C: $(SRCDIR)/NanoAODAnalyzerrdframe.h $(SRCDIR)/FourtopAnalyzer.h $(SRCDIR)/SkimEvents.h $(SRCDIR)/Linkdef.h
	rm -f $@
	rootcint -v $@ -c $(rootflags) $^
	ln -s $(SRCDIR)/rootdict_rdict.pcm .

	
libnanoadrdframe.so: $(OBJS)
	$(LD) $(SOFLAGS) $(LIBS) -o $@ $^ 


$(SRCDIR)/JetMETObjects_dict.C: $(SRCDIR)/JetCorrectorParameters.h $(SRCDIR)/SimpleJetCorrector.h $(SRCDIR)/FactorizedJetCorrector.h $(SRCDIR)/LinkdefJetmet.h
	rm -f $@
	$(ROOTSYS)/bin/rootcint -f $@ -c $(rootflags) $^ 
	ln -s $(SRCDIR)/JetMETObjects_dict_rdict.pcm .

$(SRCDIR)/rootdict.o: $(SRCDIR)/rootdict.C
	$(CXX) -c -o $@ $(CXXFLAGS) $<

$(SRCDIR)/JetMETObjects_dict.o: $(SRCDIR)/JetMETObjects_dict.C
	$(CXX) -c -o $@ $(CXXFLAGS) $<

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) -c -o $@ $(CXXFLAGS) $<
	
$(TARGET):	$(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIBS)
    
