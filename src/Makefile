hetero: test.o hetero.o hash.o art.o
	gcc -g -o hetero test.o hetero.o hash.o art.o -lm

test.o: test.c hetero.h
	gcc -g -c test.c -lm
hetero.o: hetero.c hetero.h atomic.h art.h
	gcc -g -c hetero.c -lm
art.o: art.c art.h
	gcc -g -c art.c -lm
hash.o: hash.c hash.h
	gcc -g -c hash.c -lm
.PHONY: clean
clean:
	rm *.o hetero