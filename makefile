# Compiler
# Run "make CXX=<compiler>" and replace <compiler> with either gcc or clang
# If left unspecified, gcc is used as default
CXX ?= g++
SRCS = $(wildcard src/*.cpp)

# Attaching "EXE=<name>" to the above command allows you to rename the executable to <name>. By default it is "Dragonrose.exe"
EXE ?= Dragonrose


#
# COMPILER FLAGS
#


# Default flags
WARN_FLAGS = -Wall -Werror -Wextra -Wno-error=vla -Wpedantic -Wno-unused-command-line-argument
OPT_FLAGS = -Ofast -march=native -funroll-loops

# Custom additional flags like -g
MISC_FLAGS ?= -std=c++20

# Detect Clang
ifeq ($(CXX), clang)
	OPT_FLAGS = -O3 -ffast-math -flto -march=native -funroll-loops
endif


#
# TARGETS
#


# Default target
all:
	$(CXX) $(SRCS) -o $(EXE) $(OPT_FLAGS) $(MISC_FLAGS)

# Clean target to remove the executable
clean:
	rm -f $(EXE)