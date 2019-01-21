#include <algorithm>
#include <iostream>
#include <string>
#include <cstdlib>
#include <unistd.h>
#include <bits/stdc++.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>

#define CMD_SIZE 1000
using namespace std;

string removeSpace(string str)
{
	for(int i=0;i<str.size();i++){
		if(str[i]==' '){
			str.erase(str.begin()+i);
		}
	}
	return str;
}
vector<string> parsePipe(string cmd)
{
	vector<string> piped_cmds;
	string delim = "|";
	while(cmd.length()>0){
		int pos = cmd.find(delim);
		if(pos!=string::npos){
			cerr<<"pushing "<<cmd.substr(0,pos)<<endl;
			piped_cmds.push_back(cmd.substr(0,pos));
			cmd.erase(0,cmd.find(delim)+delim.length());
		}
		else break;
	}
	cerr<<"pushing "<<cmd<<endl;	
	piped_cmds.push_back(cmd);
	return piped_cmds;
}

void execute_ext_cmd(string cmd)
{
	string orig = cmd;
	// vector<char *> args;
	vector<string> args;
	string delim = " ";
	while(cmd.size()>0){
		cerr<<"X"<<cmd<<"X\n";
		int pos = cmd.find(delim);
		cerr<<"got pos = "<<pos<<endl;
		if(pos!=string::npos){
			// char *argument = (char *)(cmd.substr(0,pos)).c_str();
			try{
				// strcpy(argument,(char *)removeSpace(cmd.substr(0,pos)).c_str());
				string argument = removeSpace(cmd.substr(0,pos));
				if(argument.size()!=0){
					cerr<<"A"<<argument<<"A vec size "<<args.size()<<endl;
					args.push_back(argument);
				}
				else cerr<<"Argument is blank"<<endl;
			}
			catch(...){
				cerr<<"Bug here"<<endl;
			}
			cerr<<"Y"<<cmd<<"Y"<<endl;
			try{
				cmd.erase(0,cmd.find(delim)+delim.length());
			}
			catch(...){
				cerr<<"Bug 2 here"<<endl;

			}
			cerr<<"Y"<<cmd<<"Y"<<endl;
		}
		else {
			cerr<<"Breaking"<<endl;
			break;
		}
	}
	cerr<<"XY"<<cmd<<"X\n";	
	if(cmd.size()!=0){
		args.push_back(removeSpace(cmd));
		cerr<<"print here "<<args[args.size()-1]<<"yes"<<endl;
	}
	else cerr<<"End argument is blank"<<endl;
	char** exec_args = new char* [args.size()+1];
	for(int i=0;i<args.size();i++){
		exec_args[i] = new char[args[i].size()+1];
		strcpy(exec_args[i],args[i].c_str());
	}
	exec_args[args.size()] = NULL;
	cerr<<"Exec_arg_0 "<<exec_args[0]<<endl;
	for(int i=0;i<args.size()-1;i++)
	{
		cerr<<i<<' '<<args[i]<<endl;
	}
	execvp(exec_args[0],exec_args);
	perror("Invalid Command");
	// cerr<<orig<<endl;
}

void parseInputOutput(string cmd)
{
	int lessThan = cmd.find("<");
	int greaterThan = cmd.find(">");
	if(lessThan==string::npos){
		if(greaterThan==string::npos){
			cerr<<"Exucuting cmd "<<cmd<<endl;
			execute_ext_cmd(cmd);
		}
		else{
			string exec_cmd = cmd.substr(0,greaterThan);
			string filename = cmd.substr(greaterThan+1);
			// cmd.erase(remove_if(filename.begin(),filename.end(),isspace),cmd.end());
			filename = removeSpace(filename);
			int filefd = open(filename.c_str(),O_WRONLY|O_CREAT|O_TRUNC,S_IRWXU);
			dup2(filefd,STDOUT_FILENO);
			execute_ext_cmd(exec_cmd);
		}
	}
	else{
		if(greaterThan==string::npos){
			string exec_cmd = cmd.substr(0,lessThan);
			string filename = cmd.substr(lessThan+1);
			// cmd.erase(remove_if(filename.begin(),filename.end(),isspace),cmd.end());
			filename = removeSpace(filename);
			int fileid = open(filename.c_str(),O_RDONLY);
			if(fileid<0){
				perror("Unable to open file");
				exit(0);
			}
			dup2(fileid,STDIN_FILENO);
			execute_ext_cmd(exec_cmd);
		}
		else{
			string inputfilename,outputfilename,exec_cmd;
			if(lessThan<greaterThan){
				exec_cmd = cmd.substr(0,lessThan);
				inputfilename = cmd.substr(lessThan+1,greaterThan-lessThan-1);
				// cmd.erase(remove_if(inputfilename.begin(),inputfilename.end(),isspace),cmd.end());
				inputfilename = removeSpace(inputfilename);
				outputfilename = cmd.substr(greaterThan+1);
				cerr<<"input file = "<<inputfilename<<endl;
				cerr<<"output file = "<<outputfilename<<endl;
				cerr<<"exec_cmd = "<<exec_cmd<<endl;
				outputfilename = removeSpace(outputfilename);
			}
			else{
				exec_cmd = cmd.substr(0,greaterThan);
				outputfilename = cmd.substr(greaterThan+1,lessThan-greaterThan-1);
				inputfilename = cmd.substr(lessThan+1);
				inputfilename = removeSpace(inputfilename);
				cerr<<"input file = "<<inputfilename<<endl;
				cerr<<"output file = "<<outputfilename<<endl;
				cerr<<"exec_cmd = "<<exec_cmd<<endl;
				outputfilename = removeSpace(outputfilename);
			}
			int inputfileid = open(inputfilename.c_str(),O_RDONLY);
			int outputfileid = open(outputfilename.c_str(),O_WRONLY|O_CREAT|O_TRUNC,S_IRWXU);
			dup2(inputfileid,STDIN_FILENO);
			dup2(outputfileid,STDOUT_FILENO);
			execute_ext_cmd(exec_cmd);
		}
	}
}
int main()
{
	// string cmd;
	char* command = new char[CMD_SIZE];
	while(true)
	{
		cout<<"$ "<<flush;
		cin.getline(command,CMD_SIZE);
		string cmd(command);
		if(strcmp(command,"quit")==0)
			exit(0);
		// int background = (cmd.find("&")==string::npos)?0:1;
		int background=0;
		if(cmd.find("&")!=string::npos){
			background = 1;
			cmd.erase(cmd.find("&"));
		}
		vector<string> args;
		vector<string> piped_cmds = parsePipe(cmd);
		int num_pipes = piped_cmds.size()-1;
		if(num_pipes==0){
			pid_t pid = fork();
			if(pid==0){
				// if(background){
				// 	int fileid = open("/tmp/input.txt",O_WRONLY|O_CREAT|O_TRUNC,S_IRWXU);
				// 	dup2(fileid,STDIN_FILENO);
				// }
				parseInputOutput(piped_cmds[0]);
			}
			else{
				if(!background)	
					wait(NULL);
			}
				
		}
		else{
			int pipes[num_pipes][2];
			for(int i=0;i<num_pipes;i++){
				if(pipe2(pipes[i],0)<0){
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
						parseInputOutput(piped_cmds[i]);
						exit(0);
					}
					else if(i==num_pipes){
						dup2(pipes[i-1][0],STDIN_FILENO);
						close(pipes[i-1][0]);
						// execute_ext_cmd(piped_cmds[i]);
						parseInputOutput(piped_cmds[i]);
						exit(0);
					}
					else{
						dup2(pipes[i-1][0],STDIN_FILENO);
						dup2(pipes[i][1],STDOUT_FILENO);
						close(pipes[i-1][0]);
						close(pipes[i][1]);
						// execute_ext_cmd(piped_cmds[i]);
						parseInputOutput(piped_cmds[i]);
						exit(0);
					}
				}
				else{
					if(!background)
						wait(NULL);
						if(i!=num_pipes){
							close(pipes[i][1]);
						}
				}
			}
		}
	}
}