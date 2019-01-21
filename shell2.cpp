#include <iostream>
#include <string>
#include <cstdlib>
#include <unistd.h>
#include <bits/stdc++.h>
// #include <sys/type.h>
#include <sys/wait.h>
#define CMD_SIZE 1000
using namespace std;

vector<string> parsePipe(string cmd)
{
	vector<string> piped_cmds;
	string delim = "|";
	while(cmd.length()>0){
		int pos = cmd.find(delim);
		if(pos!=string::npos){
			piped_cmds.push_back(cmd.substr(0,pos));
			cmd.erase(0,cmd.find(delim)+delim.length());
		}
		else break;
	}
	piped_cmds.push_back(cmd);
	return piped_cmds;
}

void execute_ext_cmd(string cmd)
{
	string orig = cmd;
	vector<char *> args;
	string delim = " ";
	while(cmd.length()>0){
		int pos = cmd.find(delim);
		if(pos!=string::npos){
			char *argument = (char *)(cmd.substr(0,pos)).c_str();
			if(strcmp(argument,"")!=0)
				args.push_back(argument);
	cerr<<"print here "<<args[0]<<"yes"<<endl;
			cmd.erase(0,cmd.find(delim)+delim.length());
		}
		else break;
	}
	if(cmd.compare("")!=0)
		args.push_back((char *)cmd.c_str());
	args.push_back(NULL);
	char** exec_args = &args[0];
	execvp(exec_args[0],exec_args);
	// cerr<<"error "<<endl;
	cerr<<exec_args[0]<<endl;
	for(int i=0;i<args.size()-1;i++)
	{
		cerr<<args[i]<<endl;
	}
	perror("Invalid Command");
	// cerr<<orig<<endl;
}

int main()
{
	// string cmd;
	char* command = new char[CMD_SIZE];
	while(true)
	{
		cout<<"$ ";
		cin.getline(command,CMD_SIZE);
		string cmd(command);
		if(strcmp(command,"quit")==0)
			exit(0);
		int background = (cmd.find("&")==string::npos)?0:1;
		vector<string> args;
		vector<string> piped_cmds = parsePipe(cmd);
		int num_pipes = piped_cmds.size()-1;
		if(num_pipes==0){
			pid_t pid = fork();
			if(pid==0)
				execute_ext_cmd(piped_cmds[0]);
			else{
				if(!background)	
					wait(NULL);
			}
				
		}
		else{
			int pipes[num_pipes][2];
			for(int i=0;i<num_pipes;i++){
				if(pipe(pipes[i])<0){
					perror("");
					exit(1);
				}
			}
			for(int i=0;i<=num_pipes;i++){	
				if(fork()==0){
					if(i==0){
						dup2(pipes[i][1],STDOUT_FILENO);
						close(pipes[i][1]);
						execute_ext_cmd(piped_cmds[i]);
						exit(0);
					}
					else if(i==num_pipes){
						dup2(pipes[i-1][0],STDIN_FILENO);
						close(pipes[i-1][0]);
						execute_ext_cmd(piped_cmds[i]);
						exit(0);
					}
					else{
						dup2(pipes[i-1][0],STDIN_FILENO);
						dup2(pipes[i][1],STDOUT_FILENO);
						close(pipes[i-1][0]);
						close(pipes[i][1]);
						execute_ext_cmd(piped_cmds[i]);
						exit(0);
					}
				}
				else{
					if(!background)
						wait(NULL);
				}
			}
		}
	}
}