rootlibs:=$(shell root-config --libs)
rootflags:=$(shell root-config --cflags)

correctionlib_inc = /home/suyong/work/src/correctionlib/include
correctionlib_lib = /home/suyong/work/src/correctionlib/lib

CORRECTION_INCPATH:=$(shell correction config --incdir)
CORRECTION_LIBPATH:=$(shell correction config --libdir)

SOFLAGS       = -shared

LD = g++ -m64 -g -Wall

CXXFLAGS = -O0 -g -Wall -fmessage-length=0 $(rootflags) -fpermissive -fPIC -pthread -DSTANDALONE -I. -I$(correctionlib_inc)

OBJDIR=src
SRCDIR=src
SRCS := $(wildcard $(SRCDIR)/*.cpp)
OBJS := $(patsubst %.cpp,%.o,$(SRCS)) $(SRCDIR)/rootdict.o

LIBS_EXE = $(rootlibs) -lMathMore -lGenVector -lcorrectionlib -L$(correctionlib_lib) 
LIBS = $(rootlibs) 

TARGET =	nanoaodrdataframe

all:	$(TARGET) libnanoadrdframe.so 

clean:
	rm -f $(OBJS) $(TARGET) libnanoaodrdframe.so $(SRCDIR)/rootdict.C rootdict_rdict.pcm

$(SRCDIR)/rootdict.C: $(SRCDIR)/NanoAODAnalyzerrdframe.h $(SRCDIR)/FourtopAnalyzer.h $(SRCDIR)/SkimEvents.h $(SRCDIR)/Linkdef.h 
	rm -f $@
	rootcint $@ -I$(correctionlib_inc) -I$(SRCDIR) $^
	rm -f rootdict_rdict.pcm
	ln -s $(SRCDIR)/rootdict_rdict.pcm .

	
libnanoadrdframe.so: $(OBJS)
	$(LD) $(SOFLAGS) $(LIBS) -o $@ $^ 


$(SRCDIR)/rootdict.o: $(SRCDIR)/rootdict.C
	$(CXX) -c -o $@ $(CXXFLAGS) $<

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) -c -o $@ $(CXXFLAGS) $<
	
$(TARGET):	$(OBJS)
	$(CXX) -o $(TARGET)  $(OBJS) $(LIBS_EXE)
    
