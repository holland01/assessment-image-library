VERBOSE=1
DEBUG=1

ifdef VERBOSE
        Q =
        E = @true	
else
        Q = @
        E = @echo	
endif

LFORMAT = bc

CFILES := $(shell find src -mindepth 1 -maxdepth 4 -name "*.c")
CXXFILES := $(shell find src -mindepth 1 -maxdepth 4 -name "*.cpp")

INFILES := $(CFILES) $(CXXFILES)

OBJFILES := $(CXXFILES:src/%.cpp=%) $(CFILES:src/%.c=%)
DEPFILES := $(CXXFILES:src/%.cpp=%) $(CFILES:src/%.c=%)
OFILES := $(OBJFILES:%=obj/%.$(LFORMAT))

BINFILE = testbin.html

COMMONFLAGS = -Wall -Wextra -pedantic -Werror\
 -Wno-unused-function -Wno-unused-variable -Wno-missing-field-initializers -Wno-self-assign -Wno-unused-value\
 -Isrc/lib 

LDFLAGS = --emrun 

ifdef DEBUG
        COMMONFLAGS := $(COMMONFLAGS) -g
endif
CFLAGS = $(COMMONFLAGS) -std=c99
CXXFLAGS = $(COMMONFLAGS) -std=c++14
DEPDIR = deps
all: $(BINFILE)
ifeq ($(MAKECMDGOALS),)
-include Makefile.dep
endif
ifneq ($(filter-out clean, $(MAKECMDGOALS)),)
-include Makefile.dep
endif

CC = emcc -v
CXX = em++ -v

OP_LVL=-O2

DEPFLAGS=-s USE_SDL=2 -s ASSERTIONS=1

-include Makefile.local

.PHONY: clean all depend
.SUFFIXES:
obj/%.$(LFORMAT): src/%.c
	$(E)C-compiling $<
	$(Q)if [ ! -d `dirname $@` ]; then mkdir -p `dirname $@`; fi
	$(Q)$(CC) $< $(DEPFLAGS) -o $@

obj/%.$(LFORMAT): src/%.cpp
	$(E)C++-compiling $<
	$(Q)if [ ! -d `dirname $@` ]; then mkdir -p `dirname $@`; fi
	$(Q)$(CXX) $(CXXFLAGS) $(OP_LVL) $< $(DEPFLAGS) -o $@

Makefile.dep: $(CFILES) $(CXXFILES)
	#$(E)Depend
	#$(Q)for i in $(^); do $(CXX) $(CXXFLAGS) -MM "$${i}" -MT obj/`basename $${i%.*}`.$(LFORMAT); done > $@

        
$(BINFILE): $(OFILES)
	$(E)Linking $@
	$(Q)$(CXX) $(LDFLAGS) $(OFILES) ~/.emscripten_cache/ports-builds/sdl2/libsdl2.bc -o $@
clean:
	$(E)Removing files
	$(Q)rm -rf obj/ 
	$(Q)rm -f $(BINFILE) Makefile.dep
	$(Q)mkdir obj


