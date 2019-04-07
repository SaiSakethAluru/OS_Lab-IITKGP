#include <iostream>
#include <bits/stdc++.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <signal.h>
using namespace std;

//The global counter counting the number of page accesses
int global_count = 0;

// A paremeter which when non zerp prints lines for debugging
int debug=0;

/*
	This is a data structure that is used to receive from and trasmit
	messages to the processes
	The mtype can take values 1 when from process to mmu and 2 when from mmu to process
	the pid is the pid of the process sending the message and, 
	the page_no is the page number that the process is requesting
*/
typedef struct{
	long mtype;
	int pid;
	int page_no;
}page_req_node;

/*
	page_table_node describes a single entry of a page table data structure
	that stores the fame noumber corresponding to that particular page number
	and also if the particular page number is valid or not
*/
typedef struct{
	int frame_no;
	int valid;
}page_table_node;

/*
	This is a simple table that is used by the MMU
	to find out what is the maximum page number that
	is accessed by any particular processs.
*/
typedef struct{
	int pid;
	int num_pages;
}process_page_map_node;

/*
	This is a data structure that sends the message from 
	the MMU to the process. The mtype value is 2 in this case
	and the frame_no is the response sent to the process
*/
typedef struct{
	long mtype;
	// int pid;
	int frame_no;
}page_response_node;

/*
	This is a data structure that is used to send the Page Fault or
	terminated message to the scheduler
*/
typedef struct {
	long mtype;
	int status;
}status_node;

//We declare tlb as a map with key as a the pid,page_no pair and \
value as the frame_no, counter pair
typedef	map< pair<int,int>,pair<int,int> > tlb;
tlb tlb_table;// tlb variable
// A vector that stores the pids in a table
// This array is usefule to assign an index value to a pid
//The first process to send a request is assigned zero and so on, so that t
// the next time a process requests, we can get back to the request easilu
vector<int> pid_index_map;

//A varible to write teh result to result.txt
fstream outfile;

void HandlePageFault(page_table_node* page_tables,vector<int> &free_frame_list,vector<vector<int> > &counters,int index,int pid,int page_no,int s);
void remove_from_tlb(int pid,int page_no);
void insert_into_tlb(int pid, int page_no, int frame_no, int s);

// A signal_handler which upon receiving of the signal\
from the master will close the result file and terminate the process
void sig_handler(int signo)
{
	if(signo == SIGUSR1)
	{
		outfile.close();
		exit(0);
	}
}

int main(int argc, char* argv[])
{
	//Open the file in a truncate mode
	outfile.open("result.txt", ios::trunc | ios::out);
	signal(SIGUSR1,sig_handler);// Install the signal handler
	//Get the ids form the command line arguments
	/*
		argv[1] = mq2
		argv[2] = mq3
		argv[3] = sm1(page tables)
		argv[4] = sm2(free frame list)
		argv[5] = sm3(pid to max page number mapping)
		argv[6] = k(Number of processes)
		argv[7] = m(Max size of page tables)
		argv[8] = f(Max number of free frames)
		argv[9] = s(Max size of tlb) 
	*/

	int mq2 = atoi(argv[1]);
	int mq3 = atoi(argv[2]);
	int sm1 = atoi(argv[3]);
	int sm2 = atoi(argv[4]);
	int sm3 = atoi(argv[5]);

	// Get the ids for the message queues q1 and mq2
	int mq2_id = msgget(mq2,IPC_CREAT|0666);
	int mq3_id = msgget(mq3,IPC_CREAT|0666);

	int k = atoi(argv[6]);
	int m = atoi(argv[7]);
	int f = atoi(argv[8]);
	int s = atoi(argv[9]);

	// Get the shared memories
	int sm1_id = shmget(sm1,m*k*sizeof(page_table_node),IPC_CREAT|0666);
	if(sm1_id<=0){
		perror("sm1_id");
	}
	int sm2_id = shmget(sm2,f*sizeof(int),IPC_CREAT|0666);
	if(sm2_id<=0){
		perror("sm2_id");
	}
	int sm3_id = shmget(sm3,k*sizeof(process_page_map_node),IPC_CREAT|0666);
	if(sm3_id<=0){
		perror("sm3_id");
	}

	if(debug)
		cout<<"MMU: "<<" mq2_id = "<<mq2_id<<" mq3_id = "<<mq3_id<<endl;
	// init tlb
	if(debug)
		cout<<"MMU: Received k="<<k<<" m="<<m<<" f="<<f<<" s="<<s<<endl;
	
	//initialize the data structures from the shared memories
	
	/*
		Attach shared memory 1 to the page table, which is essentially a 
		2D array of number of rows = number of processes and number of columns
		equal to the number of pages,

		Shared memory 2 to a int pointer which indicates the number of frames

		Shared memory 3 to a process page map_node pointer of the size of the 
		number of processes.
	*/
	page_table_node* page_tables = (page_table_node*)shmat(sm1_id,NULL,0);
	int* free_frame_array = (int *)shmat(sm2_id,NULL,0);
	process_page_map_node* process_page_map = (process_page_map_node*)shmat(sm3_id,NULL,0);
	vector<int> free_frame_list(f,1);
	/*
		Counters is a 2D array that stores the counter for the last updated value
		of each entry into the page table
	*/

	vector<vector<int> > counters(k,vector<int>(m,0));
	
	//A node for the response to be sent to the process
	page_req_node page_req;
	
	while(1){
		//Frame_sent is a flag to check if a reply has already been sent to the process or not
		int frame_sent = 0;

		//Receive a page request
		page_req.mtype = 1;
		msgrcv(mq3_id,&page_req,sizeof(page_req.pid)+sizeof(page_req.page_no),1,0);
	
		if(debug)
			cout<<"MMU: Received page_req with pid "<<page_req.pid<<" and page_no "<<page_req.page_no<<endl;
	
		int index;
		//Check if the process had already sent a request before	
		if(find(pid_index_map.begin(),pid_index_map.end(),page_req.pid) == pid_index_map.end()){
			index = pid_index_map.size();
			pid_index_map.push_back(page_req.pid);//Get a new index value
		}
		else{//If yes then set index to the previous index value
			index = find(pid_index_map.begin(),pid_index_map.end(),page_req.pid) - pid_index_map.begin();
		}

		if(debug)
			cout<<"MMU: Index of the process "<<index<<endl;
		//A node to send the response to the process
		page_response_node page_response;
		page_response.mtype = 2;//Set the mtype to 2 else the message may be treated as a page request 

		if(page_req.page_no==-9){
			//If the process has terminated, then send terminated to the scheduler
			status_node st_node;
			st_node.mtype = 1;
			st_node.status = 0;
			cout<<"MMU: Process sent 9, terminated,status = "<<st_node.status<<endl<<endl;
			msgsnd(mq2_id,&st_node,sizeof(st_node.status),0);
			continue;//continue the while loop
		}
		// If invalid page request is received(all the process_page_map is checked to find this out)
		for(int i=0;i<k;i++){
			if(process_page_map[i].pid == page_req.pid && page_req.page_no >= process_page_map[i].num_pages){
				page_response.frame_no = -2;
				
				if(debug)
					cout<<"MMU: Invalid page no. This shoudn't happen\n";
				//Send the process a response of -2, telling it to terminate
				msgsnd(mq3_id,&page_response,sizeof(page_response.frame_no),0);
				
				frame_sent = 1;
				//Send the terminated status to the scheduler
				status_node st_node;
				st_node.mtype = 1;
				st_node.status = 0;
				
				cout<<"MMU: Process sent invalid page number, terminated,status = "<<st_node.status<<endl<<endl;
				//Send the status
				msgsnd(mq2_id,&st_node,sizeof(st_node.status),0);

				break;// Break the for loop
			}
		}
		//If any of the above two conditions held true
		if(frame_sent)
			continue;

		//If a valid page request has been sent
		// access tlb
		if(tlb_table.find(pair<int,int>(page_req.pid,page_req.page_no)) != tlb_table.end()){
			//If the request can be answered by using the tlb
			if(debug)
				cout<<"MMU: Found in tlb"<<endl;
			
			global_count++;//Increment the global access count
			
			if(debug)
				cout<<"MMU: Global count "<<global_count<<" process id "<<
				page_req.pid<<" and page no "<<page_req.page_no<<endl;
			outfile<<"MMU: Global count "<<global_count<<" process id "<<
				page_req.pid<<" and page no "<<page_req.page_no<<endl;	
			
			if(debug)
				cout<<"MMU: Current value of global_count = "<<global_count<<endl;
			
			//Get the frame number from the tlb, update the counter of that frame number
			page_response.frame_no = tlb_table[pair<int,int>(page_req.pid,page_req.page_no)].first;
			tlb_table[pair<int,int>(page_req.pid,page_req.page_no)].second = global_count;

			//Update the counter in the page table as well
			counters[index][page_req.page_no] = global_count;
		}
		//If the page was not found in the tlb
		else if(page_tables[index*m+page_req.page_no].frame_no == -1){
			//If the page_table entry for that page is onvalid
			if(debug)
				cout<<"MMU: Page fault occured at process "<<page_req.pid<<
					" requesting page no "<<page_req.page_no<<endl;

			//Tell the process that a page fault has occured in which case, it pauses
			page_response.frame_no = -1;

			outfile<<"MMU: Page fault occured at process "<<page_req.pid<<
					" requesting page no "<<page_req.page_no<<endl;
			//Incerment the global count
			global_count++;
			
			if(debug)
				cout<<"MMU: Global count "<<global_count<<" process id "<<
				page_req.pid<<" and page no "<<page_req.page_no<<endl;
			outfile<<"MMU: Global count "<<global_count<<" process id "<<
				page_req.pid<<" and page no "<<page_req.page_no<<endl;

			//Call the handle page fault function
			HandlePageFault(page_tables,free_frame_list,counters,index,page_req.pid,page_req.page_no,s);
			
			// Tell the scheduler that a page fault occured by sneding a message 1 on\
			,essage queue 2
			status_node st_node;
			st_node.mtype = 1;
			st_node.status = 1;

			//Send the message
			msgsnd(mq2_id,&st_node,sizeof(st_node.status),0);
			
			if(debug)
				cout<<"MMU: sending status "<<st_node.status<<endl;
		}
		else{// If valid entry found in the page table
			if(debug)
				cout<<"MMU: Page found in page table"<<endl;
			//Get the frame number from the page table
			page_response.frame_no = page_tables[index*m+page_req.page_no].frame_no;
			
			if(debug)
				cout<<"MMU: Frame number assigned in message"<<endl;

			//Increment the global count
			global_count++;
			
			if(debug)
				cout<<"MMU: Global count "<<global_count<<" process id "<<\
				page_req.pid<<" and page no "<<page_req.page_no<<endl;
			outfile<<"MMU: Global count "<<global_count<<" process id "<<\
				page_req.pid<<" and page no "<<page_req.page_no<<endl;
			if(debug)
				cout<<"MMU: Current value of global_count = "<<global_count<<endl;
			
			//Increment the counter in the page table
			counters[index][page_req.page_no] = global_count;

			// insert into tlb
			if(debug)
				cout<<"MMU: Enter into tlb"<<endl;
			
			//Insert the latest access into the tlb
			insert_into_tlb(page_req.pid,page_req.page_no,page_response.frame_no,s);
			
			if(debug)
				cout<<"MMU: Enter into tlb returned successfully"<<endl;
		}

		//Send the page response to the process
		msgsnd(mq3_id,&page_response,sizeof(page_response.frame_no),0);
		
		if(debug)
			cout<<"MMU: Sent message to process"<<endl;
	}
}
/*
	This function completely handles the page faults


*/
void HandlePageFault(page_table_node* page_tables,vector<int> &free_frame_list,vector<vector<int> > &counters,int index,int pid,int page_no,int s)
{
	if(debug)
		cout<<"MMU: In HandlePageFault"<<endl;
	
	int m = counters[0].size();
	
	//It tries to find the free frame in the freee frame list
	if(find(free_frame_list.begin(),free_frame_list.end(),1) == free_frame_list.end()){
		//If no free frame is available
		if(debug)
			cout<<"MMU: Free frame not available"<<endl;
		int lru_row=0,lru_col=0;
		int lru_time = -1;
		
		for(int i=0;i<counters.size();i++){
			for(int j=0;j<counters[i].size();j++){
				//find the entry in the page table with the oldes counter value which is also a valid entry
				if(page_tables[i*counters[i].size()+j].frame_no != -1 && global_count-counters[i][j]>lru_time){
					lru_row = i;
					lru_col = j;
					lru_time = global_count-counters[i][j];
				}
			}
		}
		
		if(debug)
			cout<<"MMU: lru_row = "<<lru_row<<" lru_col = "<<lru_col<<endl;
		//Get the frame number of the to be replaced page table entry
		int frame_no = page_tables[lru_row*m+lru_col].frame_no;
		
		if(debug)
			cout<<"MMU: frame_no = "<<frame_no<<endl;
		//Assign the frame number to the latest page table access

		page_tables[index*m+page_no].frame_no = frame_no;
		page_tables[index*m+page_no].valid = 1;
		//Set the victim entry to be an invalid entry
		page_tables[lru_row*m+lru_col].frame_no = -1;
		
		if(debug)
			cout<<"MMU: global_count = "<<global_count<<endl;
		//Update the counter of the requested page table entry
		counters[index][page_no] = global_count;
		//Remove the victim entry from the tlb
		remove_from_tlb(pid_index_map[lru_row],lru_col);
		//Insert the requested page table entry into the tlb
		insert_into_tlb(pid,page_no,frame_no,s);

	}
	else{// If free frame is found
		if(debug)
			cout<<"MMU: Free frame available"<<endl;
		//Get the available frame number
		int frame_no = find(free_frame_list.begin(),free_frame_list.end(),1) - free_frame_list.begin();
		free_frame_list[frame_no] = 0;// Mark that frame as occupied
		page_tables[index*m+page_no].frame_no = frame_no;// Update the page table entry
		page_tables[index*m+page_no].valid = 1;
		insert_into_tlb(pid,page_no,frame_no,s);// Insert the request into the tlb
	}
}

/*
	Function that inserts a page requet into the tlb
*/
void insert_into_tlb(int pid, int page_no, int frame_no, int s)
{
	if(debug)
		cout<<"MMU: Inserting into tlb"<<endl;
	//If tlb is not yet full
	if(tlb_table.size()<s){
		// Just insert the new pair
		pair< pair<int,int>, pair<int,int> > to_insert(pair<int,int>(pid,page_no),pair<int,int>(frame_no,global_count));
		tlb_table.insert(to_insert);
	}
	else{
		//If tlb is full

		auto lru_it = tlb_table.begin();
		int lru_time = -1;

		//Find the ldest entry
		for(auto it = tlb_table.begin();it != tlb_table.end();it++){
			if(global_count - it->second.second > lru_time) {
				lru_it = it;
				lru_time = global_count - it->second.second;
			}
		}

		//Erase entry from the table
		tlb_table.erase(lru_it);
		//Make a new table entry and insert them into the  tlb
		pair< pair<int,int>, pair<int,int> > to_insert(pair<int,int>(pid,page_no),pair<int,int>(frame_no,global_count));
		tlb_table.insert(to_insert);
	}
}

//Remove an entry form the tlb
void remove_from_tlb(int pid,int page_no)
{
	tlb_table.erase(pair<int,int>(pid,page_no));
}