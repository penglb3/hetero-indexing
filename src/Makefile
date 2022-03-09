CC := gcc# compiler to use
CFLAGS := -Wall -Wno-unused -g -std=c11 -mcx16 -O3# compiling options
LDFLAGS := -lm
EXEC := hetero
UTEST := util_test

all: $(EXEC)

$(EXEC): test.o hetero.o murmur3.o art.o data_sketch.o hash.o queue.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

test.o: test.c hetero.h hash.h atomic.h art.h data_sketch.h murmur3.h
	$(CC) $(CFLAGS) -o $@ -c $<
hetero.o: hetero.c hetero.h hash.h atomic.h art.h data_sketch.h murmur3.h queue.h
	$(CC) $(CFLAGS) -o $@ -c $<
art.o: art.c art.h common.h
	$(CC) $(CFLAGS) -o $@ -c $<
hash.o: hash.c hash.h murmur3.h common.h atomic.h
	$(CC) $(CFLAGS) -o $@ -c $<
murmur3.o: murmur3.c murmur3.h
	$(CC) $(CFLAGS) -o $@ -c $<
data_sketch.o: data_sketch.c data_sketch.h murmur3.h atomic.h
	$(CC) $(CFLAGS) -o $@ -c $<
queue.o: queue.c queue.h
	$(CC) $(CFLAGS) -o $@ -c $<

# Utility testing program, doesn't affect the main program
$(UTEST): util_test.o data_sketch.o murmur3.o queue.o
	$(CC) $(CFLAGS) -o $@ $^ 
util_test.o: util_test.c data_sketch.h
	$(CC) $(CFLAGS) -o $@ -c $< 

.PHONY: clean
clean:
	rm *.o $(EXEC) $(TEST_SK)