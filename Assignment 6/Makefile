myalter2: libalter2.a myprog.cpp
	g++ -g myprog.cpp -L. -lalter2 -o myprog

myfat: fat.o libfat.a myprog.cpp
	g++ -g myprog.cpp -L. -lfat -o myprog

libfat.a: fat.o
	ar -rcs libfat.a fat.o

alter2_trial.o: alter2_trial.cpp alter2_trial.hpp
	g++ -g -c alter2_trial.cpp

libalter2.a: alter2_trial.o
	ar -rcs libalter2.a alter2_trial.o

fat.o: fat.cpp myfat.hpp
	g++ -g -c fat.cpp

clean_fat:
	rm fat.o libfat.a myprog

clean_alter2:
	rm libalter2.a myprog alter2_trial.o 