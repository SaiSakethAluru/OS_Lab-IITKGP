all: fat.o libfat.a myprog.cpp
	g++ -g myprog.cpp -L. -lfat -o myprog

libfat.a: fat.o
	ar -rcs libfat.a fat.o

fat.o: fat.cpp myfat.hpp
	g++ -g -c fat.cpp

clean:
	rm fat.o libfat.a myprog