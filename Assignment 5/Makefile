all: master scheduler proc mmu
	
master: master.cpp
	g++ -g -std=c++11 master.cpp -o master

scheduler: scheduler.cpp
	g++ -g -std=c++11 scheduler.cpp -o scheduler

proc: process.cpp
	g++ -g -std=c++11 process.cpp -o proc

mmu: mmu.cpp
	g++ -g -std=c++11 mmu.cpp -o mmu


clean: 
	rm -f master scheduler proc mmu 