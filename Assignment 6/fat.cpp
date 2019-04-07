#include <iostream>
#include <bits/stdc++.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
using namespace std;
#include "myfat.hpp"

#define vec_dir vector<directory_node>*
#define vec_fat vector<int>*
#define free_block_vec (*(super_block *)main_memory[0]).free_blocks
#define MB_TO_KB 1024

int read_multiple_blocks(int fd, char* buffer, int length);
int write_multiple_blocks(int fd, char* buffer, int length);
int allocate_new_block(int curr_block);
void copy_from_linux(string filename,int fd);
void copy_to_linux(string filename,int fd);


int block_size = 1, mem_size = 64;
int num_blocks = mem_size*MB_TO_KB/block_size;
void** main_memory;

class super_block{
public:
	int num_blocks;
	int directory_index;
	vector<bool> free_blocks;
	super_block(int num, int index)
	{
		num_blocks = num;
		directory_index = index;
		free_blocks.resize(num_blocks,false);
		free_blocks[0] = true;
		free_blocks[1] = true;
		free_blocks[2] = true;
	}
};

struct directory_node{
	string filename;
	int first_block;
	int index;
	int size;
	bool open;
	int type_of_open;
};
int init()
{
	main_memory = new void* [num_blocks];
	for(int i=3; i<num_blocks;i++){
		main_memory[i] = new char[MB_TO_KB*block_size];
	}

	main_memory[0] = new super_block(num_blocks,2);
	main_memory[1] = new vector<int>(num_blocks,-1);
	main_memory[2] = new vector<directory_node>;
}

int my_open(string filename,int type)
{
	// int dir_index = main_memory[2].size();
	// Read only
	int fd = -1;
	if(type == 0){
		for(int i = 0;i < (*(vec_dir)main_memory[2]).size();i++){
			if((*(vec_dir)main_memory[2])[i].filename == filename){
				(*(vec_dir)main_memory[2])[i].open = true;
				(*(vec_dir)main_memory[2])[i].type_of_open = 0;
				fd = i;
				break;
			}
		}
	}
	// Write only
	else if(type==1){
		for(int i = 0;i<(*(vec_dir)main_memory[2]).size();i++){
			if((*(vec_dir)main_memory[2])[i].filename == filename){
				(*(vec_dir)main_memory[2])[i].open = true;
				(*(vec_dir)main_memory[2])[i].type_of_open = 1;
				(*(vec_dir)main_memory[2])[i].size = 0;
				fd = i;
				break;
			}
		}
		if(fd == -1){
			struct directory_node new_file_node;
			new_file_node.filename = filename;
			typeof(free_block_vec.begin()) it = find(free_block_vec.begin(),free_block_vec.end(),0);
			new_file_node.first_block = it - free_block_vec.begin();
			// (*(vec_fat)main_memory[1])[new_file_node.first_block] = 1;
			free_block_vec[new_file_node.first_block] = 1;
			new_file_node.open = true;
			new_file_node.type_of_open = 1;
			new_file_node.index = 0;
			new_file_node.size = 0;
			fd = (*(vec_dir)main_memory[2]).size();
			(*(vec_dir)main_memory[2]).push_back(new_file_node);
		}
	}
	return fd;
}

void my_close(int fd)
{
	(*(vec_dir)main_memory[2])[fd].open = false;
	(*(vec_dir)main_memory[2])[fd].index = 0;

}

int my_read(int fd, char* buffer, int length)
{
	if((*(vec_dir)main_memory[2])[fd].open==false || (*(vec_dir)main_memory[2])[fd].type_of_open == 1){
		return -1;
	}
	else{
		// if(length > block_size*MB_TO_KB){
			return read_multiple_blocks(fd,buffer,length);			
		// }

	}
}

int read_multiple_blocks(int fd, char* buffer, int length)
{
	// cout<<"Index at start "<< (*(vec_dir)main_memory[2])[fd].index<<endl;
	int chars_read = 0;
	int curr_block_jumps = (*(vec_dir)main_memory[2])[fd].index / (block_size*MB_TO_KB);
	int curr_block = (*(vec_dir)main_memory[2])[fd].first_block;
	// cout<<"initial curr_block = "<<curr_block;
	for(int i=0;i<curr_block_jumps;i++){
		curr_block = (*(vec_fat)main_memory[1])[curr_block];
	}	
	// cout<<"Curr after jumps "<<curr_block<<endl;
	int read_index = (*(vec_dir)main_memory[2])[fd].index % (block_size*MB_TO_KB);
	// cout<<"read_index = "<<read_index<<endl;
	int buf_index = 0;
	while((*(vec_fat)main_memory[1])[curr_block] >= 0){
		// cout<<"in while\n";
		int size_to_read = min(block_size*MB_TO_KB - read_index,length);
		memcpy(buffer+buf_index,(char *)(main_memory[curr_block])+read_index,size_to_read);
		chars_read += size_to_read;
		buf_index += size_to_read;
		(*(vec_dir)main_memory[2])[fd].index += size_to_read;
		curr_block = (*(vec_fat)main_memory[1])[curr_block];
		// cout<<"Current block in while "<<curr_block<<endl;
		read_index = 0;
		length -= size_to_read;
		if(length <= 0){
			break;
		}
	}
	read_index = (*(vec_dir)main_memory[2])[fd].index % (block_size*MB_TO_KB);
	// cout<<"read_index = "<<read_index<<endl;
	// cout<<"chars_read = "<<chars_read<<endl;
	if(length > 0){
		// cout<<"Current block = "<<curr_block<<endl;
		int size_to_read = min(block_size*MB_TO_KB - read_index,length);
		size_to_read = min(size_to_read,(*(vec_dir)main_memory[2])[fd].size-(*(vec_dir)main_memory[2])[fd].index);
		memcpy(buffer+buf_index,(char *)main_memory[curr_block]+read_index,size_to_read);
		chars_read += size_to_read;
		(*(vec_dir)main_memory[2])[fd].index += size_to_read;
		read_index = 0;
		length -= size_to_read;
	}
	// cout<<"chars read "<<chars_read<<endl;
	return chars_read;
}

int my_write(int fd, char* buffer, int length)
{
	if((*(vec_dir)main_memory[2])[fd].open==false || (*(vec_dir)main_memory[2])[fd].type_of_open == 0){
		return -1;
	}
	int bytes_written = write_multiple_blocks(fd,buffer,length);
	(*(vec_dir)main_memory[2])[fd].size += bytes_written;
	return bytes_written;
}

int write_multiple_blocks(int fd, char* buffer, int length)
{
	int chars_written = 0;
	int curr_block_jumps = (*(vec_dir)main_memory[2])[fd].index / (block_size*MB_TO_KB);
	int curr_block = (*(vec_dir)main_memory[2])[fd].first_block;
	for(int i=0;i<curr_block_jumps;i++){
		curr_block = (*(vec_fat)main_memory[1])[curr_block];
	}	
	if(curr_block == -1){
		curr_block = allocate_new_block(curr_block);
	}
	int write_index = (*(vec_dir)main_memory[2])[fd].index % (block_size*MB_TO_KB);
	int buf_index = 0;
	while(length > 0)
	{
		int size_to_write = min(block_size*MB_TO_KB - write_index,length);
		memcpy((char *)main_memory[curr_block]+write_index,buffer+buf_index,size_to_write);
		// cout<<"written: "<<(char *)main_memory[curr_block]+write_index<<endl;
		chars_written += size_to_write;
		buf_index += size_to_write;
		(*(vec_dir)main_memory[2])[fd].index += size_to_write;
		length -= size_to_write;
		if(length <=0){
			break;
		}
		curr_block = allocate_new_block(curr_block);
		write_index = 0;
	}
	return chars_written;
}

int allocate_new_block(int curr_block)
{
	typeof(free_block_vec.begin()) it = find(free_block_vec.begin(),free_block_vec.end(),0);
	int new_block = it -free_block_vec.begin();
	(*(vec_fat)main_memory[1])[curr_block] =new_block;
	free_block_vec[new_block] = 1;
	return new_block;
}

void my_copy(string filename,int direction)
{
	int fd;
	// linux ---> mfs
	if(direction==0)
		fd = open(filename.c_str(),O_RDONLY);
	// mfs --> linux
	else	fd = open(filename.c_str(),O_WRONLY|O_CREAT|O_TRUNC,S_IRWXU);
	if(fd<0){
		perror("open error");
		return;
	}
	if(direction==0){
		copy_from_linux(filename,fd);
		close(fd);
	}
	else{
		copy_to_linux(filename,fd);
		close(fd);
	}
}

void copy_from_linux(string filename,int fd)
{
	int new_fd = my_open(filename,1);
	char buffer[101];
	while(read(fd,buffer,100)>0){
		// cout<<"writing from linux "<<buffer<<"-----------------------"<<endl;
		my_write(new_fd,buffer,100);
		bzero(buffer,101);
	}
	my_close(new_fd);
}

void copy_to_linux(string filename,int fd)
{
	char buffer[100];
	bzero(buffer,100);
	int new_fd = my_open(filename,0);
	int n;
	while((n=my_read(new_fd,buffer,100))>0){
		write(fd,buffer,n);
		bzero(buffer,100);
	}
	my_close(new_fd);
}

void my_cat(string filename)
{
	int new_fd = my_open(filename,0);
	char buffer[101];
	bzero(buffer,101);
	int n;
	while((n=my_read(new_fd,buffer,100))>0){
		printf("%s",buffer);
		bzero(buffer,101);
	}
	cout<<endl;
	my_close(new_fd);
}