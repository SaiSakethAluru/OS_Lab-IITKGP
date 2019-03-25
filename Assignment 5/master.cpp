#include <iostream>
#include <bits/stdc++.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <string>
using namespace std;
//Flag to toggle the print of couts
int debug = 1;

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
	This a a structure that is used for transferring the next
	pid over the message queue M1. This data strucure can be 
	read by the scheduler also as it is the sol rescepient of the
	messages from MQ1
	The "mtype" attribute is used to recognize the recepinet and
	the pid indicates the pid of the process that has to be signalled
*/
typedef struct {
	long mtype;
	int pid;
}process_node;

//The sig_handler function receives SIGUSR1 signals form the scheduler\
This has been installed only so that SIGUSR1 does not terminate the process
void sig_handler(int signo)
{
	if(signo == SIGUSR1){
		// do nothing
	}
}

int main()
{
	int k=3,m=10,f=10,s=6;

	//Install the signal handler
	signal(SIGUSR1,sig_handler);
	// Init data structures
	// cin>>k>>m>>f>>s;// Read the parameters form the user
	/*
		k = Number of processes
		m = The maximum number of pages a process can request
		f = The maximum number of occupied frames in memory
		s = Size of the TLB
	*/

	cout<<"MASTER: pid of master is "<<getpid()<<endl;
	
	srand(time(NULL));
	/****************************************************
	*********Instantiate the Shared Memories and********
	*********the message queues**************************/
	key_t mq1,mq2,mq3,sm1,sm2,sm3;
	int mq1_id,mq2_id,mq3_id,sm1_id,sm2_id,sm3_id;
	mq1 = ftok("mq1",65+rand()%10);		// message queue 1, from master to scheduler
	mq2 = ftok("mq2",65+rand()%10);		// message queue 2, from mmu to scheduler
	mq3 = ftok("mq3",65+rand()%10);		// message queue 3, from process to mmu
	sm1 = ftok("sm1",65+rand()%10);		// shared memorry 1, for storing the page table
	sm2 = ftok("sm2",65+rand()%10);		// shared memory 2, for storing the frame list
	sm3 = ftok("sm3",65+rand()%10);		//shared memory 3, for string the pid to max number of page numbers mapping

	//Allocate memory to the Shared Memory and the Message Queues
	sm1_id = shmget(sm1,m*k*sizeof(page_table_node),IPC_CREAT|0666);
	if(sm1_id<=0){
		perror("sm1_id");
	}
	sm2_id = shmget(sm2,f*sizeof(int),IPC_CREAT|0666);
	if(sm2_id<=0){
		perror("sm2_id");
	}
	sm3_id = shmget(sm3,k*sizeof(process_page_map_node),IPC_CREAT|0666);
	if(sm3_id<=0){
		perror("sm3_id");
	}
	mq1_id = msgget(mq1,IPC_CREAT|0666);
	if(mq1_id<=0){
		perror("mq1_id");
	}
	mq2_id = msgget(mq2,IPC_CREAT|0666);
	if(mq2_id<=0){
		perror("mq2_id");
	}
	mq3_id = msgget(mq3,IPC_CREAT|0666);
	if(mq3_id<=0){
		perror("mq3_id");
	}
	
	if(debug)
		cout<<"MASTER: mq1_id = "<<mq1_id<<" mq2_id = "<<mq2_id<<" mq3_id = "<<mq3_id<<endl;
	if(debug)
		cout<<"MASTER: Master- Successfully creates shared memory and message queues"<<endl;

	/*
		Attach shared memory 1 to the page table, which is essentially a 
		2D array of number of rows = number of processes and number of columns
		equal to the number of pages,

		Shared memory 2 to a int pointer which indicates the number of frames

		Shared memory 3 to a process page map_node pointer of the size of the 
		number of processes.
	*/
	page_table_node* page_tables = (page_table_node*)shmat(sm1_id,NULL,0);
	int* free_frame_list = (int *)shmat(sm2_id,NULL,0);
	process_page_map_node* process_page_map = (process_page_map_node*)shmat(sm3_id,NULL,0);
	
	if(debug)
		cout<<"MASTER: Master- Allocates the page_tables, free_frame_list"<<\
			" and the process page map"<<endl;
	
	//Set all the page number initially to zero and all the valid bits to \
		show that the page table node is invalid
	for(int i=0;i<k;i++){
		for(int j=0;j<m;j++){
			page_tables[i*m+j].frame_no = -1;
			page_tables[i*m+j].valid = 0;
		}
	}

	//Initialize all frames to free
	for(int i=0;i<f;i++){
		free_frame_list[i] = 1;
	}

	if(debug)
		cout<<"MASTER: Master- Initialized data structures"<<endl;

	// Fork to create the scheduler
	pid_t pid_scheduler = fork();
	if(pid_scheduler==0){
		
		/*
			Here the arguments are:- 
			{"xterm", "xterm", "-hold", "-e", 
			"./scheduler", "mq1", "mq2", "k", NULL}
		*/
		char** args = new char*[8];
		args[0] = new char[10];
		strcpy(args[0],"xterm");
		
		args[1] = new char[5];
		strcpy(args[1],"-hold");
		
		args[2] = new char[5];
		strcpy(args[2],"-e");

		args[3+0] = new char[40];
		strcpy(args[3+0],"./scheduler");

		string mq1_to_str = to_string(mq1);
		args[3+1] = new char[mq1_to_str.size()+1];
		strcpy(args[3+1],mq1_to_str.c_str());

		string mq2_to_str = to_string(mq2);
		args[3+2] = new char[mq2_to_str.size()+1];
		strcpy(args[3+2],mq2_to_str.c_str());

		string k_str = to_string(k);
		args[3+3] = new char[k_str.size()+1];
		strcpy(args[3+3],k_str.c_str());

		args[3+4] = NULL;

		if(debug)
			cout<<"MASTER: Master- About to call scheduler"<<endl;
		if(debug)
			cout<<"MASTER: "<<args[3+0]<<" "<<args[3+1]<<" "<<args[3+2]<<" "<<args[3+3]<<endl;	
		
		execvp(args[0],args);
	}

	//Fork to run the MMU process
	pid_t pid_mmu = fork();
	if(pid_mmu == 0){
		// exec call to mmu

		/*
			Here the arguments are:- 
			{"xterm", "xterm", "-hold", "-e", 
			"./mmu", "mq2", "mq3","sm1", "sm2", "sm3", "k",
			"m", "f", "s",  NULL}
		*/
		char** args = new char* [14];
		
		args[0] = new char[10];
		strcpy(args[0],"xterm");
		
		args[1] = new char[5];
		strcpy(args[1],"-hold");
		
		args[2] = new char[5];
		strcpy(args[2],"-e");

		args[3+0] = new char[30];
		strcpy(args[3+0],"./mmu");

		string mq2_to_str = to_string(mq2);
		args[3+1] = new char[mq2_to_str.size()+1];
		strcpy(args[3+1],mq2_to_str.c_str());

		string mq3_to_str = to_string(mq3);
		args[3+2] = new char[mq3_to_str.size()+1];
		strcpy(args[3+2],mq3_to_str.c_str());

		string sm1_to_str = to_string(sm1);
		args[3+3] = new char[sm1_to_str.size()+1];
		strcpy(args[3+3],sm1_to_str.c_str());

		string sm2_to_str = to_string(sm2);
		args[3+4] = new char[sm2_to_str.size()+1];
		strcpy(args[3+4],sm2_to_str.c_str());

		string sm3_to_str = to_string(sm3);
		args[3+5] = new char[sm3_to_str.size()+1];
		strcpy(args[3+5],sm3_to_str.c_str());

		string k_str = to_string(k);
		args[3+6] = new char[k_str.size()+1];
		strcpy(args[3+6],k_str.c_str());

		string m_str = to_string(m);
		args[3+7] = new char[m_str.size()+1];
		strcpy(args[3+7],m_str.c_str());

		string f_str = to_string(f);
		args[3+8] = new char[f_str.size()];
		strcpy(args[3+8],f_str.c_str());

		string s_str = to_string(s);
		args[3+9] = new char[s_str.size()];
		strcpy(args[3+9],s_str.c_str());

		args[3+10] = NULL;

		if(debug)
			cout<<"MASTER: Master- About to call mmu"<<endl;

		execvp(args[0],args);
	}

	// Many forks to create the k processes
	pid_t processes[k];// Array to store the pids

	for(int i=0;i<k;i++){
		// create pr string
		int mi = rand()%m+1;// The max page number for process mi

		/*
			We find the page_ref_len to be rand%(8*mi+1) which is from 0 to 8*mi +1
			to which we add 2mi so the 2mi<=length<=10mi
		*/
		int page_ref_len = rand()%(8*mi+1) + (2*mi);
		
		string pr_str = "";

		/*
			STL function to generate an exponential distribution.
			with mean mi/2 and a variance of 1, so that locality 
			of reference can be tested.
		*/
		default_random_engine generator;
		normal_distribution<double> distribution(mi/2,1.0);
		

		for(int j=0;j<page_ref_len;){
			double number = distribution(generator);
			// A number from the distribution is added to the \
			page refernce string only if it is a valid page number
			if(number>=0 && number < mi)
			{
				pr_str += to_string((int)number);
				pr_str += "|";
				j++;
			}
		}

		if(debug)
			cout<<"MASTER: The ref string for process "<<i<<" is "<<pr_str<<endl;
		
		processes[i] = fork();// Fork for process[i]
		
		if(debug)
			cout<<"MASTER: The pid of process "<<i<<" is "<<processes[i]<<endl;
		
		if(processes[i] == 0){
			// exec, parameters - pr string, mq1, mq3
			char** args = new char* [5];
			/*
				Here the arguments are
				{"./proc", "page reference string", "mq1", "mq3", NULL}
			*/
			if(debug)
				cout<<"MASTER: Master- Starting process "<<i<<endl;
	
			args[0] = new char[30];
			strcpy(args[0],"./proc");
	
			args[1] = new char[pr_str.size()+1];
			strcpy(args[1],pr_str.c_str());
	
			string mq1_to_str = to_string(mq1);
			args[2] = new char[mq1_to_str.size()+1];
			strcpy(args[2],mq1_to_str.c_str());
	
			string mq3_to_str = to_string(mq3);
			args[3] = new char[mq3_to_str.size()+1];
			strcpy(args[3],mq3_to_str.c_str());
	
			args[4] = NULL;
	
			if(debug)
				cout<<"MASTER: "<<args[0]<<" "<<args[1]<<" "<<args[2]<<" "<<args[3]<<endl;
	
			execvp(args[0],args);
	
			printf("MASTER: Error in proc %d\n",i);
		}
		else{
			//Modify the process_page_map, \
			i.e the shared memory 2
			process_page_map[i].pid = processes[i];
			process_page_map[i].num_pages = mi;
			
			// Create a new process_nde which is the data type for message_queue 1\
			This tell the scheduler that the process[i] is ready to start
			process_node* pr_node = new process_node;
			pr_node->mtype = 1;// The message type is 1
			pr_node->pid = processes[i];
			
			if(debug)
				cout<<"MASTER: Sending process "<<pr_node->pid<<endl;
			
			//Actually send the message to the scheduler
			msgsnd(mq1_id,pr_node,sizeof(pr_node->pid),0);

			//Sleep for 250ms
			usleep(2500000);
		}

	}

	if(debug)
		cout<<"MASTER: Just before pausing"<<endl;

	//	Sleep put here for synchronization in cases whre \
	the execution is finished early and the signal from scheduler may be missed
	// sleep(10);
	pause();

	if(debug)
		cout<<"MASTER: Master- Pause done, time to kill"<<endl;
	
	//Signal Scheduler and MMU to stop
	if(kill(pid_mmu,SIGUSR1)<0)
		perror("MASTER: mmu kill error");
	if(kill(pid_scheduler,SIGUSR2)<0)
		perror("MASTER: scheduler kill error");

	// Kill the process, just as a safety measure
	for(int i=0;i<k;i++){
		kill(processes[i],SIGKILL);
	}
	
	// Free the shared memory
	shmctl(sm1_id, IPC_RMID, NULL);
	shmctl(sm2_id, IPC_RMID, NULL);
	shmctl(sm3_id, IPC_RMID, NULL);
	// Free the message queues
	msgctl(mq1_id, IPC_RMID, NULL);
	msgctl(mq2_id, IPC_RMID, NULL);
	msgctl(mq3_id, IPC_RMID, NULL);
	
	printf("MASTER: All done!\n");

	return 0;
}