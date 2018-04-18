# Directories
OBJ=obj
SRC=src
INC=inc
BIN=bin


# Set ROOT variables
ROOTC = $(shell root-config --cflags)
ROOTLIB := $(shell root-config --libs)

# Set compiler flags
GCC = g++ -Wall -Wformat=0 -std=c++11
COPT = $(ROOTC) -I$(INC)


# Set linker flags
# Ubuntu's new default linker setting (--as-needed) exposes that many 
# libraries are linked incorrectly in ROOT. To fix this, we use the flag
# --no-as-needed for Ubuntu. Currently, this is not an issue for other 
# Linux distributions.
LD = g++
UNAME_OS := $(shell lsb_release -si)
ifeq ($(UNAME_OS),Ubuntu)
	LDFLAGS	= "-Wl,--no-as-needed" $(ROOTLIB) -L$(OBJ) # Ubuntu
else 
	LDFLAGS	= $(ROOTLIB) -L$(OBJ) # Other OS
endif


# List of sources and objects for main program
cxxFiles       = $(wildcard $(SRC)/*.cxx)
cxxObjects     = $(cxxFiles:$(SRC)/%.cxx=$(OBJ)/%.cxx.o)
cppFiles       = $(wildcard $(SRC)/*.cpp)
cppObjects     = $(cppFiles:$(SRC)/%.cpp=$(OBJ)/%.cpp.o)
cppExecutables = $(cppFiles:$(SRC)/%.cpp=$(BIN)/%)
headerFiles    = $(filter-out $(cxxFiles:$(SRC)/%.cxx=$(INC)/%.h), $(wildcard $(INC)/*.h))


# Set default target
all: $(cppExecutables)

.PHONY: $(all) $(clean)


# Generic rule for cppObjects
$(OBJ)/%.cpp.o: $(SRC)/%.cpp $(cxxObjects) $(headerFiles)
	@echo " "
	@echo "------>>>>>> Compiling $<"
	$(GCC) $(COPT) -c $< -o $@


# Generic rule for cxxObjects
$(OBJ)/%.cxx.o: $(SRC)/%.cxx $(INC)/%.h $(headerFiles)
	@echo " "
	@echo "------>>>>>> Compiling $<"
	$(GCC) $(COPT) -c $< -o $@

# Link objects
$(BIN)/%: $(OBJ)/%.cpp.o $(cxxObjects) 
	@echo " "
	@echo "------>>>>>> Linking $<"
	$(LD) $(LDFLAGS) $^ -o $@

# Clean
clean:
	@echo " "
	@echo "------>>>>>> Removing object files and executable"
	rm -rf $(BIN)/* $(OBJ)/*.o 
