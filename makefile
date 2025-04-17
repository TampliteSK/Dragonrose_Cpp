# Compiler selection
CXX ?= g++
SRCS = $(wildcard src/*.cpp)
EXE ?= Dragonrose_Cpp

# Cross-compilation settings
WINDOWS_TARGET ?= 0
SYSROOT_PATH = /usr/x86_64-w64-mingw32

# Base flags
WARN_FLAGS = -Wall -Werror -Wextra -Wno-error=vla -Wpedantic
STD_FLAGS = -std=c++20
OPT_FLAGS = -Ofast -march=native -funroll-loops
MISC_FLAGS ?=

# Windows-specific setup
ifeq ($(WINDOWS_TARGET),1)
    ifeq ($(CXX),clang++)
        TARGET_FLAGS = --target=x86_64-w64-mingw32
        LINKER_FLAGS = -B$(SYSROOT_PATH)/bin -L$(SYSROOT_PATH)/lib -lstdc++
        SYSROOT_FLAGS = --sysroot=$(SYSROOT_PATH)
        EXE := $(EXE).exe
        
        # Workaround for Windows headers
        CXXFLAGS += -D_WIN32_WINNT=0x0601
    endif
endif

# Final build flags
CXXFLAGS = $(STD_FLAGS) $(WARN_FLAGS) $(OPT_FLAGS) $(MISC_FLAGS) \
           $(TARGET_FLAGS) $(SYSROOT_FLAGS) $(LINKER_FLAGS)

# Build target
all:
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(EXE)

clean:
	rm -f $(EXE)