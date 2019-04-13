// Standard includes
#include <iostream>
#include <bits/stdc++.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
using namespace std;
// Include for api header
#include "myfat.hpp"
// Macros for better readability of code
#define vec_dir vector<directory_node>*
#define vec_fat vector<int>*
#define free_block_vec (*(super_block *)main_memory[0]).free_blocks
#define MB_TO_KB 1024
// Local function prototypes
int read_multiple_blocks(int fd, char* buffer, int length);
int write_multiple_blocks(int fd, char* buffer, int length);
int allocate_new_block(int curr_block);
void copy_from_linux(string filename,int fd);
void copy_to_linux(string filename,int fd);

// Global variables
int block_size = 1, mem_size = 64;
int num_blocks = mem_size*MB_TO_KB/block_size;
void** main_memory;

// Class description for super block in main memory
class super_block{
public:
	int num_blocks;
	int directory_index;		// index of directory
	vector<bool> free_blocks;	// bit map for free block list
	super_block(int num, int index)		// constructor to initialise data
	{
		num_blocks = num;
		directory_index = index;
		free_blocks.resize(num_blocks,false);
		free_blocks[0] = true;
		free_blocks[1] = true;
		free_blocks[2] = true;
	}
};
// structure of directory
struct directory_node{
	string filename;
	int first_block;
	int index;
	int size;
	bool open;
	int type_of_open;
};

// Allocate and initialise various data structures
int init(int bsize, int msize)
{
	block_size = bsize;
	mem_size = msize;
	main_memory = new void* [num_blocks];
	for(int i=3; i<num_blocks;i++){
		main_memory[i] = new char[MB_TO_KB*block_size];
	}

	main_memory[0] = new super_block(num_blocks,2);
	main_memory[1] = new vector<int>(num_blocks,-1);
	main_memory[2] = new vector<directory_node>;
}
// Function to open a file given the filename
// type -> 1 write only, type -> 0 read only
int my_open(string filename,int type)
{
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
		// If file not found in write mode, create a new file
		if(fd == -1){
			struct directory_node new_file_node;
			new_file_node.filename = filename;
			typeof(free_block_vec.begin()) it = find(free_block_vec.begin(),free_block_vec.end(),0);
			new_file_node.first_block = it - free_block_vec.begin();
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
// Function to close the file
void my_close(int fd)
{
	(*(vec_dir)main_memory[2])[fd].open = false;
	(*(vec_dir)main_memory[2])[fd].index = 0;

}
// Function to read from the file blocks
int my_read(int fd, char* buffer, int length)
{
	// If file is not open or is in write only mode, 
	if((*(vec_dir)main_memory[2])[fd].open==false || (*(vec_dir)main_memory[2])[fd].type_of_open == 1){
		return -1;
	}
	else{
		return read_multiple_blocks(fd,buffer,length);			
	}
}
// Helper function to read from blocks in continuous way of size min(length filesize)
int read_multiple_blocks(int fd, char* buffer, int length)
{
	int chars_read = 0;
	// Jump to correct block based on index read till now
	int curr_block_jumps = (*(vec_dir)main_memory[2])[fd].index / (block_size*MB_TO_KB);
	int curr_block = (*(vec_dir)main_memory[2])[fd].first_block;
	for(int i=0;i<curr_block_jumps;i++){
		curr_block = (*(vec_fat)main_memory[1])[curr_block];
	}	
	// Calculate index from where we start reading
	int read_index = (*(vec_dir)main_memory[2])[fd].index % (block_size*MB_TO_KB);
	int buf_index = 0;
	// While it is not the last block of the file,
	while((*(vec_fat)main_memory[1])[curr_block] >= 0){
		// Determine the size to read
		int size_to_read = min(block_size*MB_TO_KB - read_index,length);
		memcpy(buffer+buf_index,(char *)(main_memory[curr_block])+read_index,size_to_read);
		// Update file index and characters read count
		chars_read += size_to_read;
		buf_index += size_to_read;
		(*(vec_dir)main_memory[2])[fd].index += size_to_read;
		// Switch to next block
		curr_block = (*(vec_fat)main_memory[1])[curr_block];
		read_index = 0;
		// Update length left in buffer
		length -= size_to_read;
		if(length <= 0){
			break;
		}
	}
	read_index = (*(vec_dir)main_memory[2])[fd].index % (block_size*MB_TO_KB);
	// If there is still space in buffer,
	if(length > 0){
		// Determine data left to read in last block
		int size_to_read = min(block_size*MB_TO_KB - read_index,length);
		size_to_read = min(size_to_read,(*(vec_dir)main_memory[2])[fd].size-(*(vec_dir)main_memory[2])[fd].index);
		memcpy(buffer+buf_index,(char *)main_memory[curr_block]+read_index,size_to_read);
		// Update characters read and file index
		chars_read += size_to_read;
		(*(vec_dir)main_memory[2])[fd].index += size_to_read;
		read_index = 0;
		length -= size_to_read;
	}
	// Return characters read
	return chars_read;
}


// The API call to use my_wite function which is the API function for \
write
int my_write(int fd, char* buffer, int length)
{
	if((*(vec_dir)main_memory[2])[fd].open==false || (*(vec_dir)main_memory[2])[fd].type_of_open == 0){
		return -1;
	}
	int bytes_written = write_multiple_blocks(fd,buffer,length);
	(*(vec_dir)main_memory[2])[fd].size += bytes_written;
	return bytes_written;
}

// The functoin called by my_write to perform the write operation
int write_multiple_blocks(int fd, char* buffer, int length)
{
	// Navigate to the latest write index
	int chars_written = 0;
	int curr_block_jumps = (*(vec_dir)main_memory[2])[fd].index / (block_size*MB_TO_KB);
	int curr_block = (*(vec_dir)main_memory[2])[fd].first_block;
	for(int i=0;i<curr_block_jumps;i++){
		curr_block = (*(vec_fat)main_memory[1])[curr_block];
	}	
	// If the current index points to a new entry, the allocate a new block to write
	if(curr_block == -1){
		curr_block = allocate_new_block(curr_block);
	}

	// Get the write index of the block which indicates the position to start writing
	int write_index = (*(vec_dir)main_memory[2])[fd].index % (block_size*MB_TO_KB);
	// Get the index in the biffer from where to start writing
	int buf_index = 0;

	// The write while loop
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

// The function to allocate a new block\
This essentially makes changes in the directory block and the FAT block
int allocate_new_block(int curr_block)
{
	typeof(free_block_vec.begin()) it = find(free_block_vec.begin(),free_block_vec.end(),0);
	int new_block = it -free_block_vec.begin();
	(*(vec_fat)main_memory[1])[curr_block] =new_block;
	free_block_vec[new_block] = 1;
	return new_block;
}

// The my_copy function to exchange files between linux file system and my file system\
If direction is 0 then the file is copied from linux fs to my file system\
and iff direction is 1 then file is copied the other way
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

// The copy from linux function that uses read to read file from linux\
 and my_write to write file to mfs
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

// The copy to linux function that uses write to write files to linux file \
and my_read to read file from my file system
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

// The cat function repeatedly uses my_read to print the data \
to the standard output of linux using printf
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