# Flags and Compilers
CC := gcc
CXX := g++
CFLAGS := -Wall -std=c11 -Wno-unused -g -mcx16 -O2 -D_GNU_SOURCE# compiling options
CXXFLAGS= -O2 -g -std=c++11 $(shell pkg-config --cflags gflags)
LDFLAGS := -lm -lpthread $(shell pkg-config --libs gflags)

# Executables
HETERO_EXEC := hetero.test
LEVEL_EXEC := level_hash.test
CONTROL := unordered_map.test # use std::unordered_map as control group.
ART := art+inb.test
ART_RAW := art.test
# Directories
INDEXES=indexes
HETERO_DIR = ../sequential
LEVEL_DIR := $(INDEXES)/Level-Hashing/level_hashing

# Obj files
HETERO_OBJS := hetero.o murmur3.o art.o data_sketch.o hash.o queue.o
LEVEL_OBJS := hash.o level_hashing.o

.PHONY: clean all

############### General rules ##############
all: $(HETERO_EXEC) $(CONTROL) $(LEVEL_EXEC) $(ART) $(ART_RAW)

%.o: %.cpp hashBenchmark.hpp
	$(CXX) $(CXXFLAGS) -o $@ -c $< -I$(INDEXES) -I$(HETERO_DIR)

################# HETERO ###################
# Compile the executable from all needed .o files.
$(HETERO_EXEC): test_dram_hetero.o $(HETERO_OBJS:%.o=$(HETERO_DIR)/%.o)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Compile .cpp files into .o files
$(HETERO_DIR)/%.o : $(HETERO_DIR)/%.c $(HETERO_DIR)/%.h $(HETERO_DIR)/common.h
	cd $(HETERO_DIR) && make

################ CONTROL ###################
# Control group programs, doesn't affect the main program
$(CONTROL) : test_dram_unordered_map.cpp hashBenchmark.hpp
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(ART) : test_dram_art.o $(HETERO_DIR)/art.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(ART_RAW) : test_dram_art_raw.o $(HETERO_DIR)/raw_art.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

test_dram_art_raw.o: test_dram_art.cpp hashBenchmark.hpp $(HETERO_DIR)/art.h $(HETERO_DIR)/common.h
	$(CXX) $(CXXFLAGS) -o $@ -c $< -I$(HETERO_DIR) -DRAW_ART

$(HETERO_DIR)/raw_art.o: $(HETERO_DIR)/art.c
	$(CC) $(CFLAGS) -o $@ -c $< $(LDFLAGS) -DRAW_ART

################ LEVEL #####################
$(LEVEL_EXEC) : test_dram_level_hash.o $(LEVEL_OBJS:%.o=$(LEVEL_DIR)/%.o)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

############### QD ########################
qd.test: experimental/qd_hash.o test_dram_quasi.o 
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

experimental/qd_hash.o: experimental/quasi-dynamic.cpp experimental/quasi-dynamic.h
	$(CXX) $(CXXFLAGS) -o $@ -c $<

clean:
	rm -rf *.o $(HETERO_EXEC) $(CONTROL) $(LEVEL_EXEC) $(ART)
