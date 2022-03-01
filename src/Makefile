CC := gcc# compiler to use
CFLAGS := -Wall -Wno-unused -g -std=c11 -mcx16# compiling options
LDFLAGS := -lm
EXEC := hetero
TEST_SK := test_sketch

all: $(EXEC)

$(EXEC): test.o hetero.o murmur3.o art.o data_sketch.o hash.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

test.o: test.c hetero.h
	$(CC) $(CFLAGS) -o $@ -c $<
hetero.o: hetero.c hetero.h atomic.h art.h data_sketch.h
	$(CC) $(CFLAGS) -o $@ -c $<
art.o: art.c art.h common.h
	$(CC) $(CFLAGS) -o $@ -c $<
hash.o: hash.c hash.h murmur3.h common.h
	$(CC) $(CFLAGS) -o $@ -c $<
murmur3.o: murmur3.c murmur3.h
	$(CC) $(CFLAGS) -o $@ -c $<
data_sketch.o: data_sketch.c data_sketch.h murmur3.h
	$(CC) $(CFLAGS) -o $@ -c $<

# Data sketching scheme testing program, has nothing to do with the main program
$(TEST_SK): test_sketch.o data_sketch.o murmur3.o
	$(CC) $(CFLAGS) -o $@ $^ 
test_sketch.o: test_sketch.c data_sketch.h
	$(CC) $(CFLAGS) -o $@ -c $< 

.PHONY: clean
clean:
	rm *.o $(EXEC) $(TEST_SK)