NAME= libsc_config.so

CC=     gcc
CXX=	g++
YACC=   yacc
FLEX=   flex

CXXFLAGS= -O3 -Wall -Wno-deprecated 
CFLAGS= -O3 -Wall 
YACC_FLAGS = -d 
FLEX_FLAGS =

INCDIR = -I$(SYSTEMC)/include \
         -I./include

LDFLAGS = -shared
LIBDIRS = -L$(SYSTEMC)/lib-linux 
LIBS = -lsystemc

YACC_SRCS = $(wildcard ./src/*.y) 
FLEX_SRCS = $(wildcard ./src/*.l) 

LEXYACCOBJS = $(YACC_SRCS:.y=.cc) $(FLEX_SRCS:.l=.c) 
OBJS = $(YACC_SRCS:.y=.o) $(FLEX_SRCS:.l=.o)

.SUFFIXES: .l .y .cc .c .o .x 

default: bin

verif:
	@echo '[BUILD.....] $(NAME)'
ifndef SYSTEMC 
	$(error SYSTEMC environment variable not set !!!)
endif
	@if [ ! -d $(SYSTEMC) ] ; then echo $(SYSTEMC) not found !!!; exit 1; fi;

.y.cc:
	@echo '    | Compiling $<'
	@$(YACC) $(YACC_FLAGS) $< -o $@

.l.c:
	@echo '    | Compiling $<'
	@$(FLEX) $(FLEX_FLAGS) -o $@ $<

.cc.o:
	@echo '    | Compiling $<'
	@$(CXX) $(CXXFLAGS) $(INCDIR) -o $@ -c $<

.c.o:
	@echo '    | Compiling $<'
	@$(CC) $(CFLAGS) $(INCDIR) -o $@ -c $<

bin: verif  $(LEXYACCOBJS) $(OBJS)
	@echo '    | linking $(NAME)'
	@$(CXX) $(LDFLAGS) $(LIBDIRS) $(LIBS) $(OBJS) -o $(NAME)
	@echo 'done...'

clean:
	@echo '[CLEANING ...]'
	@rm -f $(LEXYACCOBJS) $(OBJS) ./src/*.hh $(NAME)

install: 
	@echo '[INSTALL...]'
ifndef NATIVE_HOME 
	$(error NATIVE_HOME environment variable not set !!!);
endif
	install -d $(NATIVE_HOME)
	install -d $(NATIVE_HOME)/lib
	install -d $(NATIVE_HOME)/include
	install $(NAME) $(NATIVE_HOME)/lib
	install -m 644 include/sc_config.h $(NATIVE_HOME)/include
	

