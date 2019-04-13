#include <iostream>
#include <bits/stdc++.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "alter2_trial.hpp"
using namespace std;

#define MB_TO_KB 1024
#define KB_TO_B 1024
#define MAX_INODE_NUM (((super_block*)main_memory[0])->inodes_per_blk)
#define super_b ((super_block*)main_memory[0])
#define inode_vec1 ((vector<inode>*)(main_memory[1]))
#define inode_vec2 ((vector<inode>*)(main_memory[2]))

int block_size = 1, mem_size = 64;
int num_blocks;
void** main_memory;
int dir_per_blk;

class block{
    public:
    int val;
    block* next;
    block(int val): val(val), next(NULL) {}
};

// typedef block*** single_indirect_pointer;
// typedef block**** double_indirect_pointer;

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

class dir_record{
    public:
    int16_t inode_number;
    char name[14];
};

class super_block{
    public:
    int num_blocks;
    int curr_dir;
    block* fbl_start;
    block* fbl_end;
    int inodes_per_blk;
    super_block(int n): num_blocks(n) {}
};


int read_multiple_blocks(int inode_num,char* buffer,int length);
int write_multiple_blocks(int inode_num,char* buffer,int length);
int my_mkdir(string dirname);
inode* get_inode_ptr(int i);

int init(int bsize, int memsize){
    //cout<<"Entered init"<<endl;
    block_size = bsize;
    mem_size = memsize;
    num_blocks = mem_size*MB_TO_KB/block_size;
    main_memory = new void* [num_blocks];

    for(int i=3;i<num_blocks;i++)
    {
        // main_memory[i] = new char[KB_TO_B*block_size];
        main_memory[i] = NULL;
    }
    main_memory[0] = new super_block(num_blocks);
    super_b->fbl_start = new block(3);
    super_b->fbl_end = super_b->fbl_start;
    //cout<<"Allocates super block and the first "<<endl;
    
    block* temp;
    for(int i=4;i<num_blocks;i++)
    {
        temp = new block(i);
        super_b->fbl_end->next = temp;
        super_b->fbl_end = super_b->fbl_end->next;
    }
    
    super_b->inodes_per_blk = block_size*KB_TO_B/sizeof(inode);
    
    main_memory[1] = new vector<inode>;
    main_memory[2] = new vector<inode>;
    //cout<<"Allocated block 1 and block 2 of main memory"<<endl;
    dir_per_blk = block_size*KB_TO_B/sizeof(dir_record);
    super_b->curr_dir = -1;
    //cout<<"Before calling my_mkdir in root"<<endl;
    int root_block = my_mkdir("root");
    //cout<<"after calling my_mkdir in root"<<endl;
    super_b->curr_dir = root_block;
    // inode_list[new_block]->dir->    
}

int get_new_inode_num(){
    if(inode_vec1->size() == MAX_INODE_NUM)
    {
        if(inode_vec2->size() == MAX_INODE_NUM)
        {
            return -1;
        }
        else
        {
            return MAX_INODE_NUM + inode_vec2->size();
        }
    }    
    else
    {
        return inode_vec1->size();
    }
    
}

int get_new_block(){
    if(super_b->fbl_start == super_b->fbl_end)
        return -1;
    
    int val = super_b->fbl_start->val;
    super_b->fbl_start =super_b->fbl_start->next;
    return val; 
}

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
            // return inode_vec2->size();
            return 0;
        }
    }    
    else
    {
        inode_vec1->push_back(new_node);
        // return inode_vec1->size();
        return 0;
    }
}

inode get_inode(int i){
    if(i>MAX_INODE_NUM)
    {
        i = i-MAX_INODE_NUM;
        return (*(inode_vec2))[i];
    }    
    else
    {
        return (*(inode_vec1))[i];
    }
}


void add_inode_to_dir(string name, int inode_num){
    if(super_b->curr_dir == -1)
        return;
    int i = super_b->curr_dir;  
    inode *curr_dir = get_inode_ptr(i);


    int n = curr_dir->num_blks;
    int block_num;
    if(n<=5)
    {
        block_num = curr_dir->dp[n-1];
    }
    else if (n>5 && n<=curr_dir->sip.size()+5)
    {
        block_num = curr_dir->sip[n-5-1];
    }
    else
    {
        //to do later
    }
    dir_record temp;
    strcpy(temp.name, name.c_str());
    temp.inode_number = inode_num;
    (*(vector<dir_record>*)(main_memory[block_num])).push_back(temp);
    
}

int my_mkdir(string dirname){    
    int prev_dir_no=super_b->curr_dir;
    inode new_node(dirname, 1);
    new_node.inode_number = get_new_inode_num();
    int new_blk_no = get_new_block();
    new_node.dp[0] = new_blk_no;
    new_node.num_blks++;

    main_memory[new_blk_no] = new vector<dir_record>;


    dir_record  record = *(new dir_record); 
    strcpy(record.name,".");
    record.inode_number = (int16_t)new_node.inode_number;
    ((vector<dir_record>*)(main_memory[new_blk_no]))->push_back(record);

    dir_record prev_dir = *(new dir_record);
    strcpy(prev_dir.name, "..");
    prev_dir.inode_number = (int16_t)prev_dir_no;
    ((vector<dir_record>*)(main_memory[new_blk_no]))->push_back(prev_dir);
    
    push_inode(new_node);

    add_inode_to_dir(dirname, new_node.inode_number);
    return new_node.inode_number;
}

int my_chdir(string dirname){
    // Write the code for multiple directory changes
    int i = super_b->curr_dir;  
    inode curr_dir = get_inode(i);

    // dir_inode* curr = inode_list[i]->dir;
    int new_dir_inode_num = -1;
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
                super_b->curr_dir = blk_data[j].inode_number;
                return super_b->curr_dir;
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
            if(strcmp(blk_data[j].name, dirname.c_str()) == 0)
            {
                int new_dir_inode_num = -1;
                super_b->curr_dir = blk_data[j].inode_number;
                return super_b->curr_dir;
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
                if(strcmp(blk_data[k].name, dirname.c_str()) == 0)
                {
                    int new_dir_inode_num = -1;
                    super_b->curr_dir = blk_data[k].inode_number;
                    return super_b->curr_dir;
                }
            } 
        }
    }
    // Check if actually a directory
    //cout<<"Directory change failed. Incorrect Name"<<endl;
    return -1;
}


int find_file(string filename){
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
                    return file_inode;
                }
            } 
        }
    }
}

int my_open(string filename, int type_of_open){
    int curr_dir_num = super_b->curr_dir;
    
    int flag = find_file(filename);

    if(type_of_open == 0 && flag == -1)
    {
        //cout<<"File not found"<<endl;
        return -1;
    }

    if(type_of_open == 0 && flag != -1)
    {
        inode *file_inode = get_inode_ptr(flag);

        if(file_inode->type == 1)
        {
            //cout<<"File is a directory"<<endl;
            return -1;
        }

        file_inode->open = 1;
        file_inode->type_of_open = 0;
        return flag;
    }

    if(type_of_open == 1 && flag != -1)
    {
        inode* file_inode = get_inode_ptr(flag);

        if(file_inode->type == 1)
        {
            //cout<<"File is a directory"<<endl;
            return -1;
        }

        file_inode->open = 1;
        file_inode->type_of_open = 1;
        file_inode->index = 0;
        file_inode->size = 0;
        return flag;
    }

    inode new_file_inode = *(new inode(filename,0));
    new_file_inode.size = 0;
    new_file_inode.open = 1;
    new_file_inode.index = 0;
    new_file_inode.type_of_open = 1;
    new_file_inode.type = 0;
    new_file_inode.inode_number = get_new_inode_num();
    push_inode(new_file_inode);

    add_inode_to_dir(filename,new_file_inode.inode_number);
    return new_file_inode.inode_number;
}
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


int my_close(int inode_num){
    inode *curr = get_inode_ptr(inode_num);
    if(curr->type == 1)
    {
        //cout<<"Trying to close a directory"<<endl;
        return -1;
    }
    curr->open = 0;
    curr->index = 0;
    return 0;    
}

int my_read(int inode_number, char* buffer, int len){
    inode file = get_inode(inode_number);
    if(file.open==0 || file.type_of_open == 1){
        //cout<<"my_read returned preemptively when type is "<<file.open<<endl;
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
        // TO do dip
       

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
        // TO do dip
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


        if(curr_block_jumps<5)
        {
            curr_block = file->dp[curr_block_jumps];
        }
        else if(curr_block_jumps>=5 && curr_block_jumps<(file->sip.size()+5))
        {
            curr_block = file->sip[curr_block_jumps - 5];
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
        }

        // Get the write index of the block which indicates the position to start writing
        int write_index = file->index % (block_size*KB_TO_B);
        // Get the index in the biffer from where to start writing

	// The write while loop
		int size_to_write = min(block_size*MB_TO_KB - write_index,length);
		memcpy((char *)main_memory[curr_block]+write_index,buffer+buf_index,size_to_write);
		// //cout<<"written: "<<(char *)main_memory[curr_block]+write_index<<endl;
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
    }
    for(int i=0;i<file->sip.size();i++)
    {
        if(file->sip[i] == -1)
        {
            rm_inode(inode_num);
            return 0;
        } 
        delete[] (char*)main_memory[file->sip[i]];
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
    cout<<"Engtered rmdir for \""<<dirname<<"\""<<endl;
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
    }
    cout<<"deleted dp"<<endl;
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
    }
    cout<<"deleted sip"<<endl;
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
	while(read(fd,buffer,100)>0){
		// //cout<<"writing from linux "<<buffer<<"-----------------------"<<endl;
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
		printf("%s",buffer);
		bzero(buffer,101);
	}
	//cout<<endl;
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