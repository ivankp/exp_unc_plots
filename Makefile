CXX := g++
STD := -std=c++14
DF := $(STD) -Iinclude
CF := $(STD) -Wall -O3 -flto -Iinclude -fmax-errors=3
# CF := $(STD) -Wall -g -Iinclude -fmax-errors=3
LF := $(STD) -flto

ROOT_CFLAGS := $(shell root-config --cflags)
ROOT_LIBS   := $(shell root-config --libs)

# RPATH
rpath_script := ldd `root-config --libdir`/libTreePlayer.so \
  | sed -n 's/.*=> \(.*\)\/.\+\.so[^ ]* (.*/\1/p' \
  | sort | uniq \
  | sed '/^\/lib/d;/^\/usr\/lib/d' \
  | sed 's/^/-Wl,-rpath=/'
ROOT_LIBS += $(shell $(rpath_script))

C_plot := $(ROOT_CFLAGS)
L_plot := $(ROOT_LIBS) -lboost_regex

SRC := src
BIN := bin
BLD := .build

SRCS := $(shell find $(SRC) -type f -name '*.cc')
DEPS := $(patsubst $(SRC)%.cc,$(BLD)%.d,$(SRCS))

GREP_EXES := grep -rl '^ *int \+main *(' $(SRC)
EXES := $(patsubst $(SRC)%.cc,$(BIN)%,$(shell $(GREP_EXES)))

NODEPS := clean
.PHONY: all clean

all: $(EXES)

$(BIN)/plot $(BIN)/edit $(BIN)/convert_hepdata: \
  $(BLD)/program_options.o $(BLD)/string_view.o $(BLD)/reader.o

#Don't create dependencies when we're cleaning, for instance
ifeq (0, $(words $(findstring $(MAKECMDGOALS), $(NODEPS))))
-include $(DEPS)
endif

.SECONDEXPANSION:

$(DEPS): $(BLD)/%.d: $(SRC)/%.cc | $(BLD)/$$(dir %)
	$(CXX) $(DF) -MM -MT '$(@:.d=.o)' $< -MF $@

$(BLD)/%.o: | $(BLD)
	$(CXX) $(CF) $(C_$*) -c $(filter %.cc,$^) -o $@

$(BIN)/%: $(BLD)/%.o | $(BIN)
	$(CXX) $(LF) $(filter %.o,$^) -o $@ $(L_$*)

$(BIN) $(BLD)/%/:
	mkdir -p $@

clean:
	@rm -rfv $(BLD) $(BIN)
