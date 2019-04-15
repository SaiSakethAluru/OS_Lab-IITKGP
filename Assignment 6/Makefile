all: myalter2 myfat
	
myalter2: libalter2.a myprog.cpp
	g++ -g myprog.cpp -L. -lalter2 -o myalter2

myfat: fat.o libfat.a myprog_fat.cpp
	g++ -g myprog_fat.cpp -L. -lfat -o myfat

libfat.a: fat.o
	ar -rcs libfat.a fat.o

inode_fs.o: inode_fs.cpp inode_fs.hpp
	g++ -g -c inode_fs.cpp

libalter2.a: inode_fs.o
	ar -rcs libalter2.a inode_fs.o

fat.o: fat.cpp myfat.hpp
	g++ -g -c fat.cpp

clean_fat:
	rm fat.o libfat.a myfat

clean_alter2:
	rm libalter2.a myalter2 inode_fs.o 

clean: clean_fat clean_alter2