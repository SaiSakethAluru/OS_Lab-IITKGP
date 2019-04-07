#ifndef FAT_H
#define FAT_H
#include <bits/stdc++.h>
#include <string>
using namespace std;

int init();
int my_open(string filename,int type);
void my_close(int fd);
int my_read(int fd, char* buffer, int length);
int my_write(int fd, char* buffer, int length);
void my_copy(string filename,int direction);
void my_cat(string filename);

#endif





