all: myalter2 myfat
	
myalter2: libalter2.a myprog.cpp
	g++ -g myprog.cpp -L. -lalter2 -o myalter2

myfat: fat.o libfat.a myprog_fat.cpp
	g++ -g myprog_fat.cpp -L. -lfat -o myfat

libfat.a: fat.o
	ar -rcs libfat.a fat.o

alter2_trial.o: alter2_trial.cpp alter2_trial.hpp
	g++ -g -c alter2_trial.cpp

libalter2.a: alter2_trial.o
	ar -rcs libalter2.a alter2_trial.o

fat.o: fat.cpp myfat.hpp
	g++ -g -c fat.cpp

clean_fat:
	rm fat.o libfat.a myfat

clean_alter2:
	rm libalter2.a myalter2 alter2_trial.o 

clean: clean_fat clean_alter2