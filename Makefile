C_SRCS	= 
C_SRCS2	= 

SRCS	= flictl.cpp flicamera.cpp
SRCS2	= simpleimageloop.cpp

# The name of the program to be build
PROGRAM       = flictl
PROGRAM2      = fli_image_loop

PROGRAMS = $(PROGRAM) $(PROGRAM2)

DEF_TARGET = $(PROGRAM)

#---- Compile information -----
CC            = g++
C             = gcc
LD	      = g++

CFLAGS        = -g
			#-v -O2
HDRPATH	      = -I. -I./libfli -I/usr/local/include
C++FLAGS      = -g -Wall
			#-O2 -v -Wno-deprecated
C++HDRPATH    = -I/usr/include $(HDRPATH)

#----- Linking information -----
LDFLAGS       =	
			#-Wl,--verbose 
LDPATHS       = -L./libfli -L/usr/local/lib
STDLIBS       = -lpthread -lboost_program_options
OTHERLIBS     = -lusb-1.0 -lcfitsio
STATICLIBS    = -llibflipro

INSTALL_DIR_BIN	= /usr/local/bin
INSTALL_NAME  = flictl

### MC: static libs need to be listed first - otherwise the linking often fails
LIBS          = $(STATICLIBS) $(STDLIBS) $(OTHERLIBS)

OBJS	      = ${SRCS:.cpp=.o} ${C_SRCS:.c=.o}
OBJS2	      = ${SRCS2:.cpp=.o} ${C_SRCS2:.c=.o}
HDRS	      = ${SRCS:.cpp=.h} ${C_SRCS:.c=.h} 
HDRS2	      = ${SRCS2:.cpp=.h} ${C_SRCS2:.c=.h} 

$(PROGRAM):	$(OBJS)
		@echo "Linking $(PROGRAM) ..."
		@echo $(LD) $(LDFLAGS) $(CFLAGS) $(DEFINES) $(C++HDRPATH) $(LDPATHS) $(OBJS) $(LIBS) -o $(PROGRAM)
		$(LD) $(LDFLAGS) $(CFLAGS) $(DEFINES) $(C++HDRPATH) $(LDPATHS) $(OBJS) $(LIBS) -o $(PROGRAM)
		@echo "$(PROGRAM) done."

$(PROGRAM2):	$(OBJS2)
		@echo "Linking $(PROGRAM2) ..."
		@echo $(LD) $(LDFLAGS) $(CFLAGS) $(DEFINES) $(C++HDRPATH) $(LDPATHS) $(OBJS2) $(LIBS) -o $(PROGRAM2)
		$(LD) $(LDFLAGS) $(CFLAGS) $(DEFINES) $(C++HDRPATH) $(LDPATHS) $(OBJS2) $(LIBS) -o $(PROGRAM2)
		@echo "$(PROGRAM2) done."

clean:;		@rm -f $(OBJS) $(C_OBJS) $(OBJS2) $(C_OBJS2) $(PROGRAMS) core

cleanall:;	@make clean; rm -rf *~

install:;	@sudo cp $(PROGRAM) $(INSTALL_DIR_BIN)

tgz:;		@tar -cvzf `date "+%Y-%m-%d_%H%M"`_flictl.tar.gz $(SRCS) $(C_SRCS) $(HDRS) Makefile

.SUFFIXES: .c .cpp .c++ .cpp~ 

.cpp.o: 
	@echo Compiling ...
	$(CC) -c $(C++FLAGS) $(DEFINES) $(C++HDRPATH)  $<

.c.o:
	@echo Compiling ...
	$(C) -c $(CFLAGS) $(DEFINES) $(CHDRPATH)  $<

.PRECIOUS: $(SRCS) $(C_SRCS)

.c++.cpp:
	@echo Renaming $<  to $*.cpp
	- @mv $< $*.cpp

