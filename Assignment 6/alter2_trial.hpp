#ifndef ALTER_2
#define ALTER_2
#include <bits/stdc++.h>
using namespace std;

int init(int bsize, int memsize);
int my_open(string filename, int type_of_open);
int my_close(int inode_num);
int my_mkdir(string dirname);
int my_chdir(string dirname);
int my_read(int inode_number, char* buffer, int len);
int my_write(int inode_number, char* buffer, int len);
int my_rmdir(string dirname);
void my_copy(string filename,int direction);
void my_cat(string filename);
void my_ls();

#endif