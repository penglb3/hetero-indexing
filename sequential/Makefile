CC := gcc# compiler to use
CXX := g++
CFLAGS := -Wall -std=c11 -Wno-unused -g -mcx16 -O2 -D_GNU_SOURCE# compiling options
CXXFLAGS= -O2 -g -std=c++11 $(shell pkg-config --cflags gflags)
LDFLAGS := -lm -lpthread $(shell pkg-config --libs gflags)
DEPFLAGS = -MMD -MP -MT $@ -MF $(DEPDIR)/$*.d

# Executables
TEST := hetero
VALIDATE := hetero_val
CONTROL := test_dram_unordered_map # use std::unordered_map as control group.
UTEST := util_test

# Obj files
OBJS := hetero.o murmur3.o art.o data_sketch.o hash.o queue.o test_dram_hetero.o test.o
OBJS_V := $(OBJS:test_dram_hetero.o=)
OBJS_B := $(OBJS:test.o=)

# Dependency directory and files
DEPDIR := .deps
DEPFILES := $(OBJS:%.o=$(DEPDIR)/%.d)

ROOT=..
INDEXES=$(ROOT)/indexes

.PHONY: clean all

# Default: Generate main project executable
all: $(TEST)

# Compile the executable from all needed .o files.
$(TEST): $(OBJS_B)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(VALIDATE): $(OBJS_V)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compile .cpp files into .o files
%.o : %.cpp
%.o : %.cpp $(DEPDIR)/%.d | $(DEPDIR)
	$(CXX) $(DEPFLAGS) $(CXXFLAGS) -c $<

# Compile .c files into .o files
%.o : %.c
%.o : %.c $(DEPDIR)/%.d | $(DEPDIR)
	$(CC) $(DEPFLAGS) $(CFLAGS) -c $<

# Control group program, doesn't affect the main program
$(CONTROL) : test_dram_unordered_map.cpp hashBenchmark.hpp
	$(CXX) $(CXXFLAGS) -o $@ $^ -I$(ROOT) $(LDFLAGS)

# Utility testing program, doesn't affect the main program
$(UTEST): util_test.o data_sketch.o murmur3.o queue.o
	$(CC) $(CFLAGS) -o $@ $^ 

$(DEPDIR): ; @mkdir -p $@

$(DEPFILES):
include $(wildcard $(DEPFILES))

clean:
	rm -rf *.o $(TEST) $(VALIDATE) $(CONTROL) $(TEST_SK) .deps
