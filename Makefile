rootlibs:=$(shell root-config --libs)
rootflags:=$(shell root-config --cflags)

SOFLAGS       = -shared

LD = g++ -g -Wall

CXXFLAGS = -O2 -g -Wall -fmessage-length=0 $(rootflags) -fpermissive -fPIC -pthread -DSTANDALONE -I.

OBJDIR=src
SRCDIR=src
SRCS := $(wildcard $(SRCDIR)/*.cpp)
OBJS := $(patsubst %.cpp,%.o,$(SRCS)) $(OBJDIR)/rootdict.o $(OBJDIR)/JetMETObjects_dict.o

LIBS = $(rootlibs) -lMathMore -lGenVector -ljsoncpp

TARGET =	nanoaodrdataframe

all:	$(TARGET) libnanoadrdframe.so 

clean:
	rm -f $(OBJS) $(TARGET) libnanoadrdframe.so libnanoaodrdframe.so $(SRCDIR)/JetMETObjects_dict.cpp $(SRCDIR)/rootdict.cpp JetMETObjects_dict_rdict.pcm $(SRCDIR)/JetMETObjects_dict_rdict.pcm  rootdict_rdict.pcm $(SRCDIR)/rootdict_rdict.pcm

$(SRCDIR)/rootdict.cpp: $(SRCDIR)/NanoAODAnalyzerrdframe.h $(SRCDIR)/FourtopAnalyzer.h $(SRCDIR)/SkimEvents.h $(SRCDIR)/Linkdef.h
	@rm -f $@
	rootcint -v $@ -c $(rootflags) $^
	ln -s rootdict_rdict.pcm ..

	
libnanoadrdframe.so: $(OBJS)
	$(LD) $(SOFLAGS) $(LIBS) -o $@ $^ 


$(SRCDIR)/JetMETObjects_dict.cpp: $(SRCDIR)/JetCorrectorParameters.h $(SRCDIR)/SimpleJetCorrector.h $(SRCDIR)/FactorizedJetCorrector.h $(SRCDIR)/LinkdefJetmet.h
	rm -f $(SRCDIR)/JetMETObjects_dict.h
	rm -f $(SRCDIR)/JetMETObjects_dict.cc
	$(ROOTSYS)/bin/rootcint -f $@ -c $(rootflags) $^ 
	ln -s JetMETObjects_dict_rdict.pcm ..

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) -c -o $@ $(CXXFLAGS) $<
	
$(TARGET):	$(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIBS)
    
