/*
Name: G Rahul Krantikiran - 16CS10018
	  Sai Saketh Aluru 	- 	16CS30030
*/
// Standard includes
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

// Max no. of characters for the command string input
#define CMD_SIZE 1000
using namespace std;

// Function to parse and remove spaces in a string
string removeSpace(string str)
{
	for(int i=0;i<str.size();i++){
		if(str[i]==' '){
			// If a space is found, erase that character in the string
			str.erase(str.begin()+i);
		}
	}
	return str;
}
/* 
Function to break the given string at the delimeter '|'
and return a vector containing individual commands
*/
vector<string> parsePipe(string cmd)
{
	vector<string> piped_cmds;
	string delim = "|";
	while(cmd.length()>0){
		// Find the first occurence of '|'
		int pos = cmd.find(delim);
		// If the return value of find is npos, it is the end of string, i.e. not found
		if(pos!=string::npos){
			// If '|' is found,
			piped_cmds.push_back(cmd.substr(0,pos)); 	// Push the first command in the vector,
			cmd.erase(0,cmd.find(delim)+delim.length()); // And erase the characters upto the '|' for next iteration
		}
		else break; // If on more pipes are present in the string, break the loop
	}
	piped_cmds.push_back(cmd); // Push the command after the last pipe in the vector
	return piped_cmds;
}
/* 
Function to take a string, parse it and call execvp 
to execute the command along with its arguments
*/
void execute_ext_cmd(string cmd)
{
	string orig = cmd;
	vector<string> args;
	string delim = " ";
	while(cmd.size()>0){
		// Find the first occurence of space in cmd
		int pos = cmd.find(delim);
		// If space exists,
		if(pos!=string::npos){
			try{
				// Take substring of the cmd upto the occurence of space and 
				// strip any spaces left in the command in the beginning or the end
				string argument = removeSpace(cmd.substr(0,pos)); 
				if(argument.size()!=0){
					// If is not a blank string, i.e the command didn't have only spaces in it
					args.push_back(argument); // Add the substring to list of arguments
				}
			}
			catch(...){
			}
			try{
				// Erase the substring upto that space
				cmd.erase(0,cmd.find(delim)+delim.length());
			}
			catch(...){

			}
		}
		else {
			break; // If no spaces are present, break the loop
		}
	}
	// If any argument is left over after the last space, add it to the list
	if(cmd.size()!=0){
		args.push_back(removeSpace(cmd));
	}
	// Create a array of C strings for execvp
	char** exec_args = new char* [args.size()+1];
	// Add the arguments to the array
	for(int i=0;i<args.size();i++){
		exec_args[i] = new char[args[i].size()+1];
		strcpy(exec_args[i],args[i].c_str());
	}
	// Append NULL to the array of arguments
	exec_args[args.size()] = NULL;
	// Call execvp with the arguments
	execvp(exec_args[0],exec_args);
	// If execvp returns, it is an invalid command. Exit after printing the error.
	perror("Invalid Command");
	exit(0);
}

/*
Function to parse a command for presence of '>' or '<'
and redirecting input and outputs accordingly
*/
void parseInputOutput(string cmd)
{
	int lessThan = cmd.find("<"); 
	int greaterThan = cmd.find(">");
	// If there is no input redirection
	if(lessThan==string::npos){
		// Also no output redirection
		if(greaterThan==string::npos){
			// Simply execute the command
			execute_ext_cmd(cmd);
		}
		// If there is only output redirection
		else{
			// Separate the command from the input file name
			string exec_cmd = cmd.substr(0,greaterThan);
			string filename = cmd.substr(greaterThan+1);
			// Parse the filename to remove any leading or trailing spaces
			filename = removeSpace(filename);
			// Create/Open the file in write only mode.
			// If file exists, we need to overwrite the data in it.
			int filefd = open(filename.c_str(),O_WRONLY|O_CREAT|O_TRUNC,S_IRWXU);
			// Replace the standard output file descriptor with the output file's descriptor
			dup2(filefd,STDOUT_FILENO);
			// Execute the command
			execute_ext_cmd(exec_cmd);
		}
	}
	// There is input redirection
	else{
		// No output redirection => Only input redirection
		if(greaterThan==string::npos){
			// Extract the command and the input file name
			string exec_cmd = cmd.substr(0,lessThan);
			string filename = cmd.substr(lessThan+1);
			// Parse the filename to remove any extra spaces
			filename = removeSpace(filename);
			// Open the file in read only mode
			int fileid = open(filename.c_str(),O_RDONLY);
			// In case of invalid filename, print the error and exit.
			if(fileid<0){
				perror("Unable to open file");
				exit(0);
			}
			// Replace the standard input file descriptor with the input file's descriptor
			dup2(fileid,STDIN_FILENO);
			// Execute the command
			execute_ext_cmd(exec_cmd);
		}
		else{
			// If there are both input and output redirection
			string inputfilename,outputfilename,exec_cmd;
			// If first input file is mentioned then output,
			if(lessThan<greaterThan){
				// Extract the command, and corresponding file names
				// Parse the filenames to remove spaces
				exec_cmd = cmd.substr(0,lessThan);
				inputfilename = cmd.substr(lessThan+1,greaterThan-lessThan-1);
				inputfilename = removeSpace(inputfilename);
				outputfilename = cmd.substr(greaterThan+1);
				outputfilename = removeSpace(outputfilename);
			}
			// If first output file mentioned then input,
			else{
				// Extract the command, and corresponding file names
				// Parse the filenames to remove spaces
				exec_cmd = cmd.substr(0,greaterThan);
				outputfilename = cmd.substr(greaterThan+1,lessThan-greaterThan-1);
				inputfilename = cmd.substr(lessThan+1);
				inputfilename = removeSpace(inputfilename);
				outputfilename = removeSpace(outputfilename);
			}
			// Open the files, input file in readonly mode 
			// And output file in write only mode, create if not found
			int inputfileid = open(inputfilename.c_str(),O_RDONLY);
			int outputfileid = open(outputfilename.c_str(),O_WRONLY|O_CREAT|O_TRUNC,S_IRWXU);
			// Replace the corresponding file descriptors of standard I/O with file ids
			dup2(inputfileid,STDIN_FILENO);
			dup2(outputfileid,STDOUT_FILENO);
			// Execute the command
			execute_ext_cmd(exec_cmd);
		}
	}
}
int main()
{
	// character array for reading input from user
	char* command = new char[CMD_SIZE];
	while(true)
	{
		// Print shell prompt
		cout<<"$ ";
		// Using getline to take input with spaces at once
		cin.getline(command,CMD_SIZE);
		string cmd(command); // Convert the char array to c++ string for further use
		// If the command is quit exit the shell
		if(strcmp(command,"quit")==0)
			exit(0);
		// Flag for checking background processes
		int background=0;
		// If there is an & in the command change the flag and erase the &
		if(cmd.find("&")!=string::npos){
			background = 1;
			cmd.erase(cmd.find("&"));
		}
		// Parse the given string for '|' and break into separate commands
		vector<string> piped_cmds = parsePipe(cmd);
		// Number of pipes in the command
		int num_pipes = piped_cmds.size()-1;
		// If there are no pipes, simply execute the command
		if(num_pipes==0){
			// Create a child process
			pid_t pid = fork();
			if(pid==0){
				// Parse the command for I/O redirection and then execute it.
				parseInputOutput(piped_cmds[0]);
			}
			else{
				// Make the parent process wait only if it is not a background command
				if(!background)	
					waitpid(pid,NULL,0);
			}
				
		}
		// If there are pipes in the command given
		else{
			// Create pipes equal to no. of commands - 1
			int pipes[num_pipes][2];
			for(int i=0;i<num_pipes;i++){
				if(pipe2(pipes[i],0)<0){
					perror("");
					exit(1);
				}
			}
			// For each of the commands in the parsed list
			for(int i=0;i<=num_pipes;i++){
				// Create a child process, to parse for I/O redirection and execute it
				pid_t pid = fork();	
				if(pid == 0){
					// For first command, only redirect output to next pipe.
					if(i==0){
						dup2(pipes[i][1],STDOUT_FILENO);
						close(pipes[i][1]);
						parseInputOutput(piped_cmds[i]);
					}
					// For last command, only redirect input from the previous pipe
					else if(i==num_pipes){
						dup2(pipes[i-1][0],STDIN_FILENO);
						close(pipes[i-1][0]);
						parseInputOutput(piped_cmds[i]);
					}
					// For intermediate commands, redirect input and output
					// to read from previous pipe and write to its pipe for the next process
					else{
						dup2(pipes[i-1][0],STDIN_FILENO);
						dup2(pipes[i][1],STDOUT_FILENO);
						close(pipes[i-1][0]);
						close(pipes[i][1]);
						parseInputOutput(piped_cmds[i]);
					}
				}
				// Parent process
				else{
					// If it is not a background process,
					if(!background){
						// Wait for termination of current child process
						waitpid(pid,NULL,0);
						// For all commands except last one, close the write end of the pipe
						// as the process writing to it has terminated
						if(i!=num_pipes){
							close(pipes[i][1]);
						}
					}
					// If it is background process, make the wait call as non blocking
					else{
						waitpid(pid,NULL,WNOHANG);
						// Close the write end of the pipe
						if(i!=num_pipes){
							close(pipes[i][1]);
						}
					}
				}
			}
		}
	}
}