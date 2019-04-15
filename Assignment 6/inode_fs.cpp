// Standard includes
#include <iostream>
#include <bits/stdc++.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
// Include our library header
#include "inode_fs.hpp"
using namespace std;
// Conversion constants
#define MB_TO_KB 1024
#define KB_TO_B 1024
// typecast definitions
#define MAX_INODE_NUM (((super_block*)main_memory[0])->inodes_per_blk)
#define super_b ((super_block*)main_memory[0])
#define inode_vec1 ((vector<inode>*)(main_memory[1]))
#define inode_vec2 ((vector<inode>*)(main_memory[2]))

// Global variables for main memory
int block_size = 1, mem_size = 16;
int num_blocks;
void** main_memory;
int dir_per_blk;

// Structure of a block
class block{
    public:
    int val;
    block* next;
    block(int val): val(val), next(NULL) {}
};
// Structure of an inode
class inode{
    public:
    string name;
    int inode_number;
    vector<int> dp;
    vector<int> sip;
    vector<vector<int> > dip;
    int type;//0 for file and 1 for directory
    int num_blks;
    int curr_block;
    int index;
    int size;
    int open;
    int type_of_open;
    inode(string name, int type){ 
        this->name = name; 
        this->type =type; 
        this->num_blks = 0; 
        this->dp.resize(5,-1);
        this->sip.resize(block_size*KB_TO_B/4, -1);
        this->dip.resize ((block_size*KB_TO_B)/4, (vector<int>(block_size*KB_TO_B/4, -1)));
    }
};
// Structure of a directory node
class dir_record{
    public:
    int16_t inode_number;
    char name[14];
};
// Structure of main_memory[0] --> super block
class super_block{
    public:
    int num_blocks;
    int curr_dir;
    block* fbl_start;
    block* fbl_end;
    int inodes_per_blk;
    super_block(int n): num_blocks(n) {}
};

// Forward declarations for local (non-api) functions
int read_multiple_blocks(int inode_num,char* buffer,int length);
int write_multiple_blocks(int inode_num,char* buffer,int length);
int my_mkdir(string dirname);
inode* get_inode_ptr(int i);

// init function -> used to declare and allocate memory in mainmemory.
// Populates some values in super block as well
// Creates root directory
int init(int bsize, int memsize){
    block_size = bsize;
    mem_size = memsize;
    num_blocks = mem_size*MB_TO_KB/block_size;
    // Initialise main memory
    main_memory = new void* [num_blocks];
    // Make all blocks starting from index 3 initially null pointers. These are the data blocks
    for(int i=3;i<num_blocks;i++)
    {
        main_memory[i] = NULL;
    }
    // Allocate memory to super block
    main_memory[0] = new super_block(num_blocks);
    // Pointer for head of free blocks linkedlist
    super_b->fbl_start = new block(3);
    super_b->fbl_end = super_b->fbl_start;
    // Append all the blocks to the linked list's head
    block* temp;
    for(int i=4;i<num_blocks;i++)
    {
        temp = new block(i);
        super_b->fbl_end->next = temp;
        super_b->fbl_end = super_b->fbl_end->next;
    }
    // Constants used in file system
    super_b->inodes_per_blk = block_size*KB_TO_B/sizeof(inode);
    // Initialise vectors in main_memory indices 1 and 2 --> contain the inodes
    main_memory[1] = new vector<inode>;
    main_memory[2] = new vector<inode>;
    dir_per_blk = block_size*KB_TO_B/sizeof(dir_record);
    // Create the root directory, with parent pointing to -1.
    super_b->curr_dir = -1;
    int root_block = my_mkdir("root");
    // Make root as the current directory initially
    super_b->curr_dir = root_block;
}
// Function to get a new inode number. Depending on the current inode sizes, the inode
// is allocated either from block 1 or from block 2. If no space is available, then a -1 is returned
int get_new_inode_num(){
    // If block 1 is full
    if(inode_vec1->size() == MAX_INODE_NUM)
    {
        // If block 2 is also full -> no space, return -1
        if(inode_vec2->size() == MAX_INODE_NUM)
        {
            return -1;
        }
        else
        {
            // Allocate from block 2 if 1 is full and 2 is not full
            return MAX_INODE_NUM + inode_vec2->size();
        }
    }    
    else
    {
        // Allocate from block 1 if it is not full
        return inode_vec1->size();
    }
    
}
// Function to get a new free block from head of linked list maintained in super block
int get_new_block(){
    if(super_b->fbl_start == super_b->fbl_end)
        return -1;
    
    int val = super_b->fbl_start->val;
    super_b->fbl_start =super_b->fbl_start->next;
    return val; 
}
// Push an inode in the inode vectors.
int push_inode(inode new_node)
{
    if(inode_vec1->size() == MAX_INODE_NUM)
    {
        if(inode_vec2->size() == MAX_INODE_NUM)
        {
           return -1;
        }
        else
        {
            inode_vec2->push_back(new_node);
            return 0;
        }
    }    
    else
    {
        inode_vec1->push_back(new_node);
        return 0;
    }
}
// Get the inode given the inode number
inode get_inode(int i){
    // If it is in block 2
    if(i>MAX_INODE_NUM)
    {
        i = i-MAX_INODE_NUM;
        return (*(inode_vec2))[i];
    }    
    // Else if it is block 1
    else
    {
        return (*(inode_vec1))[i];
    }
}

// Add an directory entry to a block in directory's inode
void add_inode_to_dir(string name, int inode_num){
    // Skip in case of root directory creation
    if(super_b->curr_dir == -1)
        return;
    int i = super_b->curr_dir;  
    // Get the inode of the current directory
    inode *curr_dir = get_inode_ptr(i);

    // Check the blocks in current_directory and calculate where to add the new file
    int n = curr_dir->num_blks;
    int block_num;
    // If it can be added to one of direct pointers, add it there
    if(n<=5)
    {
        block_num = curr_dir->dp[n-1];
    }
    // Else check the singly indirect pointer
    else if (n>5 && n<=curr_dir->sip.size()+5)
    {
        block_num = curr_dir->sip[n-5-1];
    }
    // Else go for the double indirect pointer
    else
    {
        n -= (5+block_size);
        int i = n/block_size;
        int j = n%block_size;
        block_num = curr_dir->dip[i][j];
    }
    // Add the directory record to the block calculated
    dir_record temp;
    strcpy(temp.name, name.c_str());
    temp.inode_number = inode_num;
    (*(vector<dir_record>*)(main_memory[block_num])).push_back(temp);
    
}
// API Function to create directory with given name
int my_mkdir(string dirname){    
    // Get the current directory
    int prev_dir_no=super_b->curr_dir;
    // Create a new inode with given name and directory type
    inode new_node(dirname, 1);
    //  Get a new inode number to give to this
    new_node.inode_number = get_new_inode_num();
    // Get a new free block and allocate to the first direct pointer of the directory
    int new_blk_no = get_new_block();
    new_node.dp[0] = new_blk_no;
    new_node.num_blks++;
    // Initialise this free block
    main_memory[new_blk_no] = new vector<dir_record>;
    // Create two directory records for . and ..
    dir_record  record = *(new dir_record); 
    strcpy(record.name,".");
    record.inode_number = (int16_t)new_node.inode_number;
    ((vector<dir_record>*)(main_memory[new_blk_no]))->push_back(record);

    dir_record prev_dir = *(new dir_record);
    strcpy(prev_dir.name, "..");
    prev_dir.inode_number = (int16_t)prev_dir_no;
    ((vector<dir_record>*)(main_memory[new_blk_no]))->push_back(prev_dir);
    // Push the inode of the directory 2 in the block 1 or block 2
    push_inode(new_node);
    // Add the inode for the new directory in the inode for the current directory
    add_inode_to_dir(dirname, new_node.inode_number);
    // Return the inode number
    return new_node.inode_number;
}
// API Funtion to change directory
int my_chdir(string dirname){
    // Get current directory
    int i = super_b->curr_dir;  
    // Get inode for current Directory
    inode curr_dir = get_inode(i);
    int new_dir_inode_num = -1;
    // Look for the directory name in the current directory's inode's direct pointers
    for(int i=0;i<5;i++)
    {
        int block_num = curr_dir.dp[i];
        if(curr_dir.dp[i] == -1)
            return -1;
        vector<dir_record> blk_data = *((vector<dir_record>*)(main_memory[block_num]));
        for(int j=0;j<blk_data.size();j++)
        {
            if(strcmp(blk_data[j].name, dirname.c_str()) == 0)
            {
                int new_dir_inode_num = -1;
                // Change current directory in super block
                super_b->curr_dir = blk_data[j].inode_number;
                return super_b->curr_dir;
            }
        }
    }

    // Look for the directory name in the current directory's inode's single indirect pointers
    for(int i=0;i<curr_dir.sip.size();i++)
    {
        int block_num = curr_dir.sip[i];
        if(block_num == -1)
            return -1;
        
        vector<dir_record> blk_data = *((vector<dir_record>*)(main_memory[block_num]));
        for(int j=0;j<blk_data.size();j++)
        {
            if(strcmp(blk_data[j].name, dirname.c_str()) == 0)
            {
                int new_dir_inode_num = -1;
                // Change current directory in super block
                super_b->curr_dir = blk_data[j].inode_number;
                return super_b->curr_dir;
            }
        }
    }

    // Look for the directory name in the current directory's inode's double indirect pointers
    for(int i=0;i<curr_dir.dip.size();i++)
    {
        for(int j=0;j>curr_dir.dip[i].size();j++)
        {
            int block_num = curr_dir.dip[i][j];
            if(block_num == -1)
                return -1;
            
            vector<dir_record> blk_data = *((vector<dir_record>*)(main_memory[block_num]));
            for(int k=0;k<blk_data.size();k++)
            {
                if(strcmp(blk_data[k].name, dirname.c_str()) == 0)
                {
                    int new_dir_inode_num = -1;
                    // Change current directory in super block
                    super_b->curr_dir = blk_data[k].inode_number;
                    return super_b->curr_dir;
                }
            } 
        }
    }
    // Check if actually a directory. If not, print error message and return -1
    cout<<"Directory change failed. Incorrect Name"<<endl;
    return -1;
}

// Search for inode number given a file name
int find_file(string filename){
    int i = super_b->curr_dir;  
    // Get inode for current directory
    inode curr_dir = get_inode(i);
    // Look for file name in current directory inode's direct pointers
    for(int i=0;i<5;i++)
    {
        int block_num = curr_dir.dp[i];
        if(curr_dir.dp[i] == -1)
            return -1;
        vector<dir_record> blk_data = *((vector<dir_record>*)(main_memory[block_num]));
        for(int j=0;j<blk_data.size();j++)
        {
            if(strcmp(blk_data[j].name, filename.c_str()) == 0)
            {
                int file_inode = -1;
                file_inode = blk_data[j].inode_number;
                return file_inode;
            }
        }
    }

    // Look for file name in current directory inode's single indirect pointers
    for(int i=0;i<curr_dir.sip.size();i++)
    {
        int block_num = curr_dir.sip[i];
        if(block_num == -1)
            return -1;
        
        vector<dir_record> blk_data = *((vector<dir_record>*)(main_memory[block_num]));
        for(int j=0;j<blk_data.size();j++)
        {
            if(strcmp(blk_data[j].name, filename.c_str()) == 0)
            {
                int file_inode = -1;
                file_inode = blk_data[j].inode_number;
                return file_inode;
            }
        }
    }

    // Look for file name in current directory inode's double indirect pointers
    for(int i=0;i<curr_dir.dip.size();i++)
    {
        for(int j=0;j>curr_dir.dip[i].size();j++)
        {
            int block_num = curr_dir.dip[i][j];
            if(block_num == -1)
                return -1;
            
            vector<dir_record> blk_data = *((vector<dir_record>*)(main_memory[block_num]));
            for(int k=0;k<blk_data.size();k++)
            {
                if(strcmp(blk_data[k].name, filename.c_str()) == 0)
                {
                    int file_inode = -1;
                    file_inode = blk_data[k].inode_number;
                    return file_inode;
                }
            } 
        }
    }
}
// API function for my_open
int my_open(string filename, int type_of_open){
    int curr_dir_num = super_b->curr_dir;
    
    int flag = find_file(filename);
    // If opening in read only mode and file is not found, return -1
    if(type_of_open == 0 && flag == -1)
    {
        return -1;
    }
    // If opening in read only mode and file is found
    if(type_of_open == 0 && flag != -1)
    {
        inode *file_inode = get_inode_ptr(flag);
        // If the filename is that of a directory
        if(file_inode->type == 1)
        {
            // cout<<"File is a directory"<<endl;
            return -1;
        }
        // If it is a valid file, mark it as opened in read only and return the inode number as file descriptor
        file_inode->open = 1;
        file_inode->type_of_open = 0;
        return flag;
    }
    // If it is opened in write only mode and file is in the fs
    if(type_of_open == 1 && flag != -1)
    {
        inode* file_inode = get_inode_ptr(flag);
        // Check if it is a directory
        if(file_inode->type == 1)
        {
            //cout<<"File is a directory"<<endl;
            return -1;
        }
        // Mark it as opened and reset index and size
        // Return inode number as file descriptor 
        file_inode->open = 1;
        file_inode->type_of_open = 1;
        file_inode->index = 0;
        file_inode->size = 0;
        return flag;
    }
    // Create a new file if in write mode and file not found
    inode new_file_inode = *(new inode(filename,0));
    new_file_inode.size = 0;
    new_file_inode.open = 1;
    new_file_inode.index = 0;
    new_file_inode.type_of_open = 1;
    new_file_inode.type = 0;
    new_file_inode.inode_number = get_new_inode_num();
    push_inode(new_file_inode);
    // Add new file to current directory and return inode number as fd
    add_inode_to_dir(filename,new_file_inode.inode_number);
    return new_file_inode.inode_number;
}
// Given an inode number, return a pointer to it
inode* get_inode_ptr(int i){
    if(i>MAX_INODE_NUM)
    {
        i = i-MAX_INODE_NUM;
        return &((*(inode_vec2))[i]);
    }    
    else
    {
        return &(*(inode_vec1))[i];
    }
}

// Close the file, given its inode number
int my_close(int inode_num){
    // Get pointer to the inode
    inode *curr = get_inode_ptr(inode_num);
    // Check if it is a directory
    if(curr->type == 1)
    {
        return -1;
    }
    // Mark the file as closed and reset its read/write indexs
    curr->open = 0;
    curr->index = 0;
    return 0;    
}

int my_read(int inode_number, char* buffer, int len){
    inode file = get_inode(inode_number);
    if(file.open==0 || file.type_of_open == 1){
		return -1;
	}
	else{
		return read_multiple_blocks(inode_number,buffer,len);			
	}
}

int read_multiple_blocks(int inode_number,char* buffer,int length){
    int chars_read = 0;
    inode *file = get_inode_ptr(inode_number);
    int buf_index = 0;
    int  read_index = file->index % (block_size*KB_TO_B);
    

    // //cout<<"Before entering the while loop"<<endl;
    while(1){
        //cout<<"After entering the while loop"<<endl;
        int curr_block_jumps = file->index / (block_size*KB_TO_B);
        int curr_block;

        if(curr_block_jumps<5)
        {
            curr_block = file->dp[curr_block_jumps];
        }
        else if(curr_block_jumps>=5 && curr_block_jumps<(file->sip.size()+5))
        {
            curr_block = file->sip[curr_block_jumps - 5];
        }
        else{
            curr_block_jumps -= (5+block_size);
            int i = curr_block_jumps / block_size;
            int j = curr_block_jumps % block_size;
            curr_block = file->dip[i][j];
        }
       

        if(length < block_size*KB_TO_B)
            break;
		// Determine the size to read
		int size_to_read = min(block_size*KB_TO_B - read_index,length);
		memcpy(buffer+buf_index,(char *)(main_memory[curr_block])+read_index,size_to_read);

        // //cout<<"Just wrote\""<<((char *)(main_memory[curr_block])+read_index)<<"\" to the buffer"<<endl;
		// Update file index and characters read count
		chars_read += size_to_read;
		buf_index += size_to_read;
		file->index += size_to_read;
		// Switch to next block
		read_index = 0;
		// Update length left in buffer
		length -= size_to_read;
		if(length <= 0){
			break;
		}
	}
    read_index = file->index % (block_size*MB_TO_KB);
    if(length > 0){
        int curr_block_jumps = file->index / (block_size*KB_TO_B);
        int curr_block;

        if(curr_block_jumps<5)
        {
            curr_block = file->dp[curr_block_jumps];
        }
        else if(curr_block_jumps>=5 && curr_block_jumps<(file->sip.size()+5))
        {
            curr_block = file->sip[curr_block_jumps - 5];
        }
        else{
            curr_block_jumps -= (5+block_size);
            int i = curr_block_jumps / block_size;
            int j = curr_block_jumps % block_size;
            curr_block = file->dip[i][j];
        }
		// Determine data left to read in last block
		int size_to_read = min(block_size*MB_TO_KB - read_index,length);
		size_to_read = min(size_to_read,file->size-file->index);
		memcpy(buffer+buf_index,(char *)main_memory[curr_block]+read_index,size_to_read);

        //cout<<"Just read now\""<<((char *)(main_memory[curr_block])+read_index)<<"\""<<endl<<"size to read "<<size_to_read<<endl;
        
		// Update characters read and file index
		chars_read += size_to_read;
		file->index += size_to_read;
		read_index = 0;
		length -= size_to_read;
	}
    return chars_read;
}


int my_write(int inode_number, char* buffer, int len)
{
    inode *file = get_inode_ptr(inode_number);
    if(file->open==0 || file->type_of_open == 0){
		return -1;
	}
	else{
        int bytes_written = write_multiple_blocks(inode_number,buffer,len);			
        file->size += bytes_written;
		return bytes_written;
	}
}

int write_multiple_blocks(int fd, char* buffer, int length)
{
	// Navigate to the latest write index
    inode* file = get_inode_ptr(fd);

	int chars_written = 0;
    int curr_block;
    int buf_index = 0;
	while(length > 0)
	{
        int curr_block_jumps = file->index / (block_size*KB_TO_B);
        // cout<<"curr_blcok_jumps = "<<curr_block_jumps<<endl;

        if(curr_block_jumps<5)
        {
            curr_block = file->dp[curr_block_jumps];
        }
        else if(curr_block_jumps>=5 && curr_block_jumps<(file->sip.size()+5))
        {
            curr_block = file->sip[curr_block_jumps - 5];
        }
        else{
            curr_block_jumps -= (5+block_size);
            int i = curr_block_jumps / block_size;
            int j = curr_block_jumps % block_size;
            curr_block = file->dip[i][j];
        }

        // If the current index points to a new entry, the allocate a new block to write
        if(curr_block == -1){
            curr_block = get_new_block();
            file->num_blks++;
            main_memory[curr_block] = new char[block_size*KB_TO_B];

            if(curr_block_jumps<5)
            {
                file->dp[curr_block_jumps] = curr_block;
            }
            else if(curr_block_jumps>=5 && curr_block_jumps<(file->sip.size()+5))
            {
                file->sip[curr_block_jumps - 5]  = curr_block;
            }
            else{
                curr_block_jumps -= (5+block_size);
                int i = curr_block_jumps / block_size;
                int j = curr_block_jumps % block_size;
                file->dip[i][j] = curr_block;
            }
        }

        // Get the write index of the block which indicates the position to start writing
        int write_index = file->index % (block_size*KB_TO_B);
        // Get the index in the biffer from where to start writing

	// The write while loop
		int size_to_write = min(block_size*MB_TO_KB - write_index,length);
		memcpy((char *)main_memory[curr_block]+write_index,buffer+buf_index,size_to_write);
		// cout<<"written: "<<(char *)main_memory[curr_block]+write_index<<endl;
		chars_written += size_to_write;
		buf_index += size_to_write;
		file->index += size_to_write;
		length -= size_to_write;
		if(length <=0){
			break;
		}
		write_index = 0;
	}
	return chars_written;
}

int rm_inode(int inode_num){
    if(inode_num > MAX_INODE_NUM)
    {
        inode_vec2->erase(inode_vec2->begin()+inode_num - MAX_INODE_NUM);
    }
    else
    {
        inode_vec1->erase(inode_vec1->begin()+inode_num);
    }
}

void add_to_free_blk(int blk){
    block* new_blk = new block(blk);
    if(super_b->fbl_end == NULL){
        super_b->fbl_end = super_b->fbl_start = new_blk;
    }
    else
    {
        super_b->fbl_end->next = new_blk;
        super_b->fbl_end = super_b->fbl_end->next;
    }
}

int my_rm(int inode_num){
    inode *file = get_inode_ptr(inode_num);

    for(int i=0;i<5;i++)
    {
        if(file->dp[i] == -1)
        {
            rm_inode(inode_num);
            return 0;
        } 
        delete[] (char*)main_memory[file->dp[i]];
        add_to_free_blk(file->dp[i]);
    }
    for(int i=0;i<file->sip.size();i++)
    {
        if(file->sip[i] == -1)
        {
            rm_inode(inode_num);
            return 0;
        } 
        delete[] (char*)main_memory[file->sip[i]];
        add_to_free_blk(file->sip[i]);
    }
    for(int i=0;i<file->dip.size();i++)
    {
        for(int j=0;j<file->dip[i].size();j++)
        {
            if(file->dip[i][j] == -1)
            {
                rm_inode(inode_num);
                return 0;
            } 
            delete[] (char*)main_memory[file->dip[i][j]];
            add_to_free_blk(file->dip[i][j]);
        }
    }
    rm_inode(inode_num);
    return 0;
}

int find_file2(string filename){
    int i = super_b->curr_dir;  
    inode curr_dir = get_inode(i);

    // dir_inode* curr = inode_list[i]->dir;
    for(int i=0;i<5;i++)
    {
        int block_num = curr_dir.dp[i];
        if(curr_dir.dp[i] == -1)
            return -1;
        vector<dir_record> blk_data = *((vector<dir_record>*)(main_memory[block_num]));
        for(int j=0;j<blk_data.size();j++)
        {
            if(strcmp(blk_data[j].name, filename.c_str()) == 0)
            {
                int file_inode = -1;
                file_inode = blk_data[j].inode_number;
                ((vector<dir_record>*)(main_memory[block_num]))->erase(((vector<dir_record>*)(main_memory[block_num]))->begin() + j);
                return file_inode;
            }
        }
    }

    for(int i=0;i<curr_dir.sip.size();i++)
    {
        int block_num = curr_dir.sip[i];
        if(block_num == -1)
            return -1;
        
        vector<dir_record> blk_data = *((vector<dir_record>*)(main_memory[block_num]));
        for(int j=0;j<blk_data.size();j++)
        {
            if(strcmp(blk_data[j].name, filename.c_str()) == 0)
            {
                int file_inode = -1;
                file_inode = blk_data[j].inode_number;
                ((vector<dir_record>*)(main_memory[block_num]))->erase(((vector<dir_record>*)(main_memory[block_num]))->begin() + j);
                return file_inode;
            }
        }
    }

    for(int i=0;i<curr_dir.dip.size();i++)
    {
        for(int j=0;j>curr_dir.dip[i].size();j++)
        {
            int block_num = curr_dir.dip[i][j];
            if(block_num == -1)
                return -1;
            
            vector<dir_record> blk_data = *((vector<dir_record>*)(main_memory[block_num]));
            for(int k=0;k<blk_data.size();k++)
            {
                if(strcmp(blk_data[k].name, filename.c_str()) == 0)
                {
                    int file_inode = -1;
                    file_inode = blk_data[k].inode_number;
                    ((vector<dir_record>*)(main_memory[block_num]))->erase(((vector<dir_record>*)(main_memory[block_num]))->begin() + k);
                    return file_inode;
                }
            } 
        }
    }
}

int my_rmdir(string dirname)
{
    // cout<<"Engtered rmdir for \""<<dirname<<"\""<<endl;
    int dir_inode = find_file2(dirname);
    inode* dir = get_inode_ptr(dir_inode);
    // cout<<dir->name<<endl;
    // cout<<super_b->curr_dir<<endl;
    // cout<<"chdir "<<my_chdir(dir->name)<<endl;
    // cout<<super_b->curr_dir<<endl;
    // my_ls();
    // cout<<"received inode_pointer"<<endl;
    if(dir->type == 0)
    {
        return -1;
    }

    for(int i=0;i<5;i++)
    {
        if(dir->dp[i] == -1)
        {
           rm_inode(dir->inode_number);
           return 0;
        }
        vector<dir_record> temp_rec = *((vector<dir_record>*)main_memory[dir->dp[i]]);
        for(int j= 0; j<temp_rec.size();j++)
        {
            int temp_inode_num = (int)temp_rec[j].inode_number;
            inode temp2 = get_inode(temp_inode_num);
            if(temp2.type == 1)
            {
                if(strcmp(temp_rec[j].name, ".") != 0 && strcmp(temp_rec[j].name, "..") != 0 )
                {
                    cout<<temp_rec[j].name<<endl;
                    my_rmdir(temp_rec[j].name);
                }
            }
            else
            {
                my_rm(temp_inode_num);
            }
            
        }
        delete (vector<dir_record>*)main_memory[dir->dp[i]];
        add_to_free_blk(dir->dp[i]);
    }
    for(int i=0;i<dir->sip.size();i++)
    {
        if(dir->sip[i] == -1)
        {
           rm_inode(dir->inode_number);
           return 0;
        }
        vector<dir_record> temp_rec = *((vector<dir_record>*)main_memory[dir->sip[i]]);
        for(int j= 0; j<temp_rec.size();j++)
        {
            int temp_inode_num = (int)temp_rec[j].inode_number;
            inode temp2 = get_inode(temp_inode_num);
            if(temp2.type == 1)
            {
                if(strcmp(temp_rec[j].name, ".") != 0 && strcmp(temp_rec[j].name, "..") != 0 )
                    my_rmdir(temp2.name);
            }
            else
            {
                my_rm(temp_inode_num);
            }
            
        }
        delete (vector<dir_record>*)main_memory[dir->sip[i]];
        add_to_free_blk(dir->sip[i]);
    }
    for(int i=0;i<dir->dip.size();i++)
    {
        for(int k = 0;k<dir->dip[i].size();k++)
        {
            if(dir->dip[i][k] == -1)
            {
                rm_inode(dir->inode_number);
                return 0;
            }
            vector<dir_record> temp_rec = *((vector<dir_record>*)main_memory[dir->dip[i][k]]);
            for(int j= 0; j<temp_rec.size();j++)
            {
                int temp_inode_num = (int)temp_rec[j].inode_number;
                inode temp2 = get_inode(temp_inode_num);
                if(temp2.type == 1)
                {
                    if(strcmp(temp_rec[j].name, ".") != 0 && strcmp(temp_rec[j].name, "..") != 0 )
                        my_rmdir(temp2.name);
                }
                else
                {
                    my_rm(temp_inode_num);
                }
                
            }
            delete (vector<dir_record>*)main_memory[dir->dip[i][k]];
            add_to_free_blk(dir->dip[i][k]);
        }
    }
    
    return 0;
}



// The copy from linux function that uses read to read file from linux\
 and my_write to write file to mfs
void copy_from_linux(string filename,int fd)
{
	int new_fd = my_open(filename,1);
	char buffer[101];
    int n;
	while((n=read(fd,buffer,100))>0){
		// //cout<<"writing from linux "<<buffer<<"-----------------------"<<endl;
		my_write(new_fd,buffer,n);
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
// The cat function repeatedly uses my_read to print the data \
to the standard output of linux using printf
void my_cat(string filename)
{
	int new_fd = my_open(filename,0);
	char buffer[101];
	bzero(buffer,101);
	int n;
	while((n=my_read(new_fd,buffer,100))>0){
        buffer[n] = '\0';
		printf("%s",buffer);
		bzero(buffer,101);
	}
	cout<<endl;
	my_close(new_fd);
}

void my_ls(){
    int dir_node = super_b->curr_dir;
    inode* dir = get_inode_ptr(dir_node);

    for(int i=0;i<5;i++)
    {
        if(dir->dp[i] == -1)
        {
           return;
        }
        vector<dir_record> temp_rec = *((vector<dir_record>*)main_memory[dir->dp[i]]);
        for(int j= 0; j<temp_rec.size();j++)
        {
            // int temp_inode_num = (int)temp_rec[j].inode_number;
            cout<<temp_rec[j].name<<endl;
        }
    }
    for(int i=0;i<dir->sip.size();i++)
    {
        if(dir->sip[i] == -1)
        {
           return;
        }
        vector<dir_record> temp_rec = *((vector<dir_record>*)main_memory[dir->sip[i]]);
        for(int j= 0; j<temp_rec.size();j++)
        {
            int temp_inode_num = (int)temp_rec[j].inode_number;
            cout<<temp_rec[j].name<<endl;
                       
        }
    }
    for(int i=0;i<dir->dip.size();i++)
    {
        for(int k = 0;k<dir->dip[i].size();k++)
        {
            if(dir->dip[i][k] == -1)
            {
                return;
            }
            vector<dir_record> temp_rec = *((vector<dir_record>*)main_memory[dir->dip[i][k]]);
            for(int j= 0; j<temp_rec.size();j++)
            {
                int temp_inode_num = (int)temp_rec[j].inode_number;
                cout<<temp_rec[j].name<<endl;
            }
        }
    }  
}
