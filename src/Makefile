hetero: test.o hetero.o hash.o
	gcc -g -o hetero test.o hetero.o hash.o -lm

test.o: test.c hetero.h
	gcc -g -c test.c -lm
hetero.o: hetero.c hetero.h atomic.h
	gcc -g -c hetero.c -lm
hash.o: hash.c hash.h
	gcc -g -c hash.c -lm
.PHONY: clean
clean:
	rm *.o hetero