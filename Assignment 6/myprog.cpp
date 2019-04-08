#include <iostream>
#include <bits/stdc++.h>
#include "myfat.hpp"
using namespace std;

int main()
{
	init();
	string filename("file.txt");
	int fd = my_open(filename,1);
	string test1 = "Hello saketh";
	char test[100];
	strcpy(test,test1.c_str());
	int bytes_written = my_write(fd,test,strlen(test));
	cout<<"Written "<<bytes_written<<endl;
	my_close(fd);
	fd = my_open(filename,0);
	char read_test[100];
	int bytes_read = my_read(fd,read_test,100);
	read_test[bytes_read] = '\0';
	cout<<"Read "<<bytes_read<<" bytes, data = "<<read_test<<endl;
	my_cat(filename);
	cout<<endl;
	my_copy(filename,1); 
	strcpy(test,"test.txt");
	my_copy("test.txt",0);
	my_cat("test.txt");
}