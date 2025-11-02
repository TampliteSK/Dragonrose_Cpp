# Compiler selection
CXX ?= g++
SRCS = $(wildcard src/*.cpp) $(wildcard src/chess/*.cpp) $(wildcard src/eval/*.cpp)
EXE ?= Dragonrose_Cpp

# Compiler flags
INC_DIRS = -Isrc -Isrc/chess -Isrc/eval
STD_FLAGS = -std=c++20
WARN_FLAGS = -Wall -Werror -Wextra -Wno-error=vla -Wpedantic

# Optimization flags: use -Ofast for g++, -O3 otherwise
OPT_FLAGS = -O3 -ffast-math -march=native -funroll-loops -flto=auto
ifeq ($(findstring g++,$(CXX)),g++)
    OPT_FLAGS = -Ofast -ffast-math -march=native -funroll-loops -flto=auto
endif

MISC_FLAGS ?=
CXXFLAGS = $(STD_FLAGS) $(WARN_FLAGS) $(OPT_FLAGS) $(INC_DIRS) $(MISC_FLAGS)

# Debug flags
# Usage: make DEBUG=1 (-fsanitize not supported by MinGW)
ifdef DEBUG
	CXXFLAGS += -g -fsanitize=address -fsanitize=undefined
endif

# Add .exe if Windows
ifeq ($(OS),Windows_NT)
    EXE := $(EXE).exe
else
	# WSL / Linux
	# Cross-compile option
	ifdef WINDOWS
		CXX = x86_64-w64-mingw32-g++
		EXE := $(EXE).exe
		LDFLAGS += -static-libgcc -static-libstdc++
	endif
endif

# Build target
all:
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(SRCS) -o $(EXE)

clean:
	rm -f $(EXE)