hetero: test.o hetero.o hash.o
	gcc -o hetero test.o hetero.o hash.o -lm

test.o: test.c hetero.h
	gcc -c test.c -lm
hetero.o: hetero.c hetero.h
	gcc -c hetero.c -lm
hash.o: hash.c hash.h
	gcc -c hash.c -lm
.PHONY: clean
clean:
	rm *.o hetero