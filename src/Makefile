CC := gcc# compiler to use
CFLAGS := -Wall -std=c11 -Wno-unused -g -mcx16 -O2 -D_GNU_SOURCE# compiling options
LDFLAGS := -lm -lpthread

EXEC := hetero
UTEST := util_test
OBJS := test.o hetero.o murmur3.o art.o data_sketch.o hash.o queue.o ebr.o gc.o
DEPDIR := .deps
DEPFLAGS = -MMD -MP -MT $@ -MF $(DEPDIR)/$*.d

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o : %.c
%.o : %.c $(DEPDIR)/%.d | $(DEPDIR)
	$(CC) $(DEPFLAGS) $(CFLAGS) -c $<

$(DEPDIR): ; @mkdir -p $@

DEPFILES := $(OBJS:%.o=$(DEPDIR)/%.d)

# Utility testing program, doesn't affect the main program
$(UTEST): util_test.o data_sketch.o murmur3.o queue.o
	$(CC) $(CFLAGS) -o $@ $^ 

$(DEPFILES):
include $(wildcard $(DEPFILES))

.PHONY: clean all
clean:
	rm -rf *.o $(EXEC) $(TEST_SK) .deps