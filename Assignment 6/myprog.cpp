#include <iostream>
#include <bits/stdc++.h>
#include "alter2_trial.hpp"
using namespace std;

int main()
{
	// Read input from user
	int bsize = 1, msize = 1;
	cout<<"Enter block size in kb: ";
	cin>>bsize;
	cout<<"Enter memory size in Mb: ";
	cin>>msize;
	init(bsize,msize);
	// cout<<"My prog came out of init"<<endl;
	// The file in mfs 
	string filename("file.txt");

	// First open the file
	int fd = my_open(filename,1);

	string test1 = "Hello saketh";
	char test[100];
	strcpy(test,test1.c_str());

	// Write the string to the file in mfs using my_write
	int bytes_written = my_write(fd,test,strlen(test));
	cout<<"Written "<<bytes_written<<endl;

	// Close the file
	my_close(fd);

	// Open the same file in read mode
	fd = my_open(filename,0);
	char read_test[100];

	// read from the file
	int bytes_read = my_read(fd,read_test,100);
	read_test[bytes_read] = '\0';
	// cout<<"Read "<<bytes_read<<" bytes, data = "<<read_test<<endl;

	my_close(fd);
	// Print the entire file to stdout using my_cat
	my_cat(filename);
	// cout<<endl;

	// Copy file in mfs to linus fs
	my_copy(filename,1); 
	strcpy(test,"test.txt");

	// copy test file from linux fs to mfs
	my_copy("test.txt",0);
	my_copy("test.txt",1);
	// Print the newly copied file
	my_cat("test.txt");
	my_mkdir("new");
	my_chdir("new");
	// my_chdir("..");
	int new_fd = my_open("file.txt",1);
	char next_str[] = "Hello India";
	my_write(new_fd, next_str, strlen(next_str));
	my_close(new_fd);
	my_copy("file.txt",1);
	my_chdir("..");
	// cout<<"My Cat-------\n\n";
	my_cat(filename);
	// cout<<"My Cat-------\n\n";
	my_copy("file.txt",0);
	my_cat(filename);
	my_ls();
	my_rmdir("new");
	my_ls();
}