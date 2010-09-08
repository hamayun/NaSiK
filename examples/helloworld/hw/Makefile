NAME=  arch.x 

CC=     g++
OPT=    -O3 
DEBUG=  -g
WARN=   -Wall -Wno-deprecated
CFLAGS= $(OPT) $(DEBUG) $(WARN)

SRCS=   $(wildcard ./sources/*.cpp) 
OBJS=   $(SRCS:.cpp=.o)

INCDIR = -I$(SYSTEMC)/include
INCDIR += -I$(LIBTA_HOME)/include
INCDIR += -I$(LIBTA_HOME)/include/eu
INCDIR += -I$(LIBTA_HOME)/include/linker
INCDIR += -I$(LIBTA_HOME)/include/annotation

LD = g++
LIBDIR = -L$(SYSTEMC)/lib-linux -L$(LIBTA_HOME)/lib -Wl,--export-dynamic
LIBS = -ldl -lta -lsystemc -lsc_config
LDFLAGS = 

## Variable that points to SystemC installation path

.SUFFIXES: .cpp .o .x

default: bin

bin:    $(OBJS)
	echo 'Creating binary: $(NAME)'
	$(LD) $(CFLAGS) $(INCDIR) $(LIBDIR) $(LDFLAGS) -o $(NAME) $(OBJS) $(LIBS) 2>&1 | c++filt
	echo 'done...'

.cpp.o:
	echo 'Compiling object file: $<'
	$(CC) $(CFLAGS) $(INCDIR) -o $@ -c $<


clean:
	echo 'Cleaning ...'
	rm -f $(NAME)
	rm -f $(OBJS)


