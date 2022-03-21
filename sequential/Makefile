CC := gcc# compiler to use
CFLAGS := -Wall -std=c11 -Wno-unused -g -mcx16 -O2 -D_GNU_SOURCE# compiling options
LDFLAGS := -lm -lpthread
DEPFLAGS = -MMD -MP -MT $@ -MF $(DEPDIR)/$*.d

EXEC := hetero
UTEST := util_test
OBJS := test.o hetero.o murmur3.o art.o data_sketch.o hash.o queue.o
DEPDIR := .deps
DEPFILES := $(OBJS:%.o=$(DEPDIR)/%.d)

.PHONY: clean all

# Default: Generate main project executable
all: $(EXEC)

# Compile the executable from all .o files.
$(EXEC): $(OBJS)
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
	rm -rf *.o $(EXEC) $(TEST_SK) .deps