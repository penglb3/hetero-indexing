CC := gcc# compiler to use
CXX := g++
CFLAGS := -Wall -std=c11 -Wno-unused -g -mcx16 -O2 -D_GNU_SOURCE# compiling options
LDFLAGS := -lm -lpthread 
DEPFLAGS = -MMD -MP -MT $@ -MF $(DEPDIR)/$*.d

# Executables
TEST := hetero
UTEST := util_test

# Obj files
OBJS := hetero.o murmur3.o art.o data_sketch.o hash.o queue.o test.o

# Dependency directory and files
DEPDIR := .deps
DEPFILES := $(OBJS:%.o=$(DEPDIR)/%.d)

.PHONY: clean all

# Default: Generate main project executable
all: $(TEST)

# Compile the executable from all needed .o files.
$(TEST): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compile .c files into .o files
%.o : %.c
%.o : %.c $(DEPDIR)/%.d | $(DEPDIR)
	$(CC) $(DEPFLAGS) $(CFLAGS) -c $<
 
# Utility testing program, doesn't affect the main program
$(UTEST): util_test.o data_sketch.o murmur3.o queue.o
	$(CC) $(CFLAGS) -o $@ $^ 

$(DEPDIR): ; @mkdir -p $@

$(DEPFILES):
include $(wildcard $(DEPFILES))

clean:
	rm -rf *.o $(TEST) $(CONTROL) $(UTEST) .deps
