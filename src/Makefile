CC := gcc# compiler to use
CFLAGS := -Wall -Wno-unused -g# compiling options
LDFLAGS := -lm
EXEC := hetero

all: $(EXEC)

$(EXEC): test.o hetero.o hash.o art.o data_sketch.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

test.o: test.c hetero.h
	$(CC) $(CFLAGS) -o $@ -c $<
hetero.o: hetero.c hetero.h atomic.h art.h data_sketch.h
	$(CC) $(CFLAGS) -o $@ -c $<
art.o: art.c art.h
	$(CC) $(CFLAGS) -o $@ -c $<
hash.o: hash.c hash.h
	$(CC) $(CFLAGS) -o $@ -c $<
data_sketch.o: data_sketch.c data_sketch.h hash.h
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY: clean
clean:
	rm *.o $(EXEC)