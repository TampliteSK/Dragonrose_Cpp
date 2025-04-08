# Compiler
# Run "make CXX=<compiler>" and replace <compiler> with either g++ or clang++
# If left unspecified, g++ is used as default
CXX ?= g++
SRCS = $(wildcard src/*.cpp)

# Attaching "EXE=<name>" to the above command allows you to rename the executable to <name>. By default it is "Dragonrose.exe"
EXE ?= Dragonrose_Cpp


#
# COMPILER FLAGS
#


# Default flags
WARN_FLAGS = -Wall -Werror -Wextra -Wno-error=vla -Wpedantic -Wno-unused-command-line-argument
OPT_FLAGS = -Ofast -march=native -funroll-loops

# Custom additional flags like -g
MISC_FLAGS ?= 

# Detect Clang
ifeq ($(CXX), clang)
	OPT_FLAGS = -O3 -ffast-math -flto -march=native -funroll-loops
	LIB_FLAGS = -lstdc++
endif


#
# TARGETS
#


# Default target
all:
	$(CXX) $(SRCS) -o $(EXE).exe $(OPT_FLAGS) $(MISC_FLAGS) -std=c++20

# Clean target to remove the executable
clean:
	rm -f $(EXE)