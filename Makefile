ifeq ($(CXXSTD),)
	CXXSTD := c++17
endif

override CXXFLAGS += -Wall -Wextra -std=$(CXXSTD)
override LDFLAGS += `sdl2-config --libs --cflags`
override LDFLAGS += -lcudart -lcublas -lSDL2_ttf -L/opt/cuda/targets/x86_64-linux/lib/

RM=rm -f
# $(wildcard *.cpp /xxx/xxx/*.cpp): get all .cpp files from the current directory and dir "/xxx/xxx/"
SRCS := $(wildcard */*.cpp *.cpp)
CSRCS := $(wildcard *.cu)
# $(patsubst %.cpp,%.o,$(SRCS)): substitute all ".cpp" file name strings to ".o" file name strings
OBJS := $(patsubst %.cpp,%.o,$(SRCS))
OBJS += $(patsubst %.cu,%.o,$(CSRCS))

# Allows one to enable verbose builds with VERBOSE=1
V := @
ifeq ($(VERBOSE),1)
	V :=
endif

all: release

release: CXXFLAGS += -O3
release: LDFLAGS += -s -flto
release: NVFLAGS += -O3
release: build

profile: CXXFLAGS += -O2 -g3
profile: NVFLAGS += -O2 -g -G
profile: build

debug: CXXFLAGS += -g3 -DDEBUG
debug: NVFLAGS += -g -G
debug: build

pgo: merge_profraw pgouse

ifeq ($(findstring clang++,$(CXX)),clang++)
clean_pgodata: clean
	$(V) rm -f default_*.profraw default.profdata
else
clean_pgodata: clean
	$(V) rm -f *.gcda objects/*.gcda stdlib/*/*.gcda
endif

pgobuild: CXXFLAGS+=-fprofile-generate -march=native
pgobuild: LDFLAGS+=-fprofile-generate -flto
ifeq ($(findstring clang++,$(CXX)),clang++)
pgobuild: CXXFLAGS+=-mllvm -vp-counters-per-site=5
endif
pgobuild: | clean_pgodata cgoto

pgorun: | pgobuild benchmark tests

ifeq ($(findstring clang++,$(CXX)),clang++)
merge_profraw: pgorun
	$(V) llvm-profdata merge --output=default.profdata default_*.profraw
else
merge_profraw: pgorun
endif

pgouse: merge_profraw
	$(V) $(MAKE) clean
	$(V) $(MAKE) cgoto CXXFLAGS=-fprofile-use CXXFLAGS+=-march=native LDFLAGS+=-fprofile-use LDFLAGS+=-flto
	$(V) $(MAKE) all

build: $(OBJS)
	$(V) $(CXX) $(OBJS) $(LDFLAGS) -o renderer

depend: .depend

.depend: $(SRCS)
	$(RM) ./.depend
	$(CXX) $(CXXFLAGS) -MM $^>>./.depend;

clean:
	$(RM) $(OBJS)

distclean: clean
	$(RM) *~ .depend

include .depend

kernel.o: kernel.cu
	nvcc $(NVFLAGS) -c -o kernel.o kernel.cu

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@
