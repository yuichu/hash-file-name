#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

using namespace std;

int parentToChildPipe[2];	// The pipe for parent-to-child communications
int childToParentPipe[2];	// The pipe for the child-to-parent communication

#define READ_END 0		// The read end of the pipe
#define WRITE_END 1		// The write end of the pipe

#define HASH_PROG_ARRAY_SIZE 6		// The maximum size of the array of hash programs
#define HASH_VALUE_LENGTH 1000		// The maximum length of the hash value
#define MAX_FILE_NAME_LENGTH 1000	// The maximum length of the file name

// The array of names of hash programs
const string hashProgs[] = {"md5sum", "sha1sum", "sha224sum", "sha256sum", "sha384sum", "sha512sum"};

void computeHash(const string& hashProgName);


int main(int argc, char** argv)
{
	// Check for errors
	if(argc < 2)
	{
		fprintf(stderr, "USAGE: %s <FILE NAME>\n", argv[0]);
		exit(-1);
	}

	// Save the name of the file
	string fileName(argv[1]);
	// Convert fileName type
	int n = fileName.length();
	char fileName_array[n + 1];
	strcpy(fileName_array, fileName.c_str());

	// Process id
	pid_t pid;

	// Run a program for each type of hashing algorithm hash algorithm
	for(int hashAlgNum = 0; hashAlgNum < HASH_PROG_ARRAY_SIZE; ++hashAlgNum)
	{

		// Create two pipes
		if(pipe(parentToChildPipe) < 0)
		{
			perror("pipe");
			exit(-1);
		}
		if(pipe(childToParentPipe) < 0)
		{
			perror("pipe");
			exit(-1);
		}

		// Fork a child process and save the id
		if((pid = fork()) < 0)
		{
			perror("fork");
			exit(-1);
		}
		// Begin child process
		else if(pid == 0)
		{
			// Closing unused ends of both pipes
			// Note: within computeHash(), program will:
			// parentToChildPipe[READ_END] & childToParentPipe[WRITE_END]
			if(close(parentToChildPipe[WRITE_END]) < 0)
			{
				perror("close");
				exit(-1);
			}
			if(close(childToParentPipe[READ_END]) < 0)
			{
				perror("close");
				exit(-1);
			}

			// Compute the hash
			computeHash(hashProgs[hashAlgNum]);
		}

		// Begin parent process

		// Closing unused ends of both pipes
		// Note: from here to the end, the program will:
		// parentToChildPipe[WRITE_END] & childToParentPipe[READ_END]
		if(close(childToParentPipe[WRITE_END]) < 0)
		{
			perror("close");
			exit(-1);
		}
		if(close(parentToChildPipe[READ_END]) < 0)
		{
			perror("close");
			exit(-1);
		}

		// Buffer to hold the hash received from the child
		char hashValue[HASH_VALUE_LENGTH];

		// Reset the hash buffer
		memset(hashValue, (char)NULL, HASH_VALUE_LENGTH);


		// Send the file name to child
		if(write(parentToChildPipe[WRITE_END], fileName_array, sizeof(fileName_array)) < 0)
		{
			perror("write");
			exit(-1);
		}

		// Read the hash sent by the child
		if(read(childToParentPipe[READ_END], hashValue, sizeof(hashValue)) < 0)
		{
			perror("read");
			exit(-1);
		}

		// Print the hash value
		fprintf(stdout, "%s HASH VALUE: %s\n", hashProgs[hashAlgNum].c_str(), hashValue);
		fflush(stdout);


		// Wait for the program to terminate
		if(wait(NULL) < 0)
		{
			perror("wait");
			exit(-1);
		}
	}

	return 0;
}

// ----------------------------------------------------------------------------
// Function: computeHash()
// called by the child to compute hash value of file name
// @param hashProgName - the name of the hash program
// ----------------------------------------------------------------------------
void computeHash(const string& hashProgName)
{


	char hashValue[HASH_VALUE_LENGTH];			// Hash value buffer
	char fileNameRecv[MAX_FILE_NAME_LENGTH];	// The received file name

	// Fill the buffer with 0's
	memset(fileNameRecv, (char)NULL, MAX_FILE_NAME_LENGTH);

	// Read file name from the parent
	if(read(parentToChildPipe[READ_END], fileNameRecv, sizeof(fileNameRecv)) < 0)
	{
		perror("read");
		exit(-1);
	}

	// Concat has program with file name
 	// i.e. sha512sum fileName
	string cmdLine(hashProgName);
	cmdLine += " ";
	cmdLine += fileNameRecv;
	// Convert cmdLine type
	int n = cmdLine.length();
	char cmdLine_array[n + 1];
	strcpy(cmdLine_array, cmdLine.c_str());

    // Open the pipe to the program (specified in cmdLine)
	FILE* progOutput = popen(cmdLine_array, "r");

	// Reset the value buffer
	memset(hashValue, (char)NULL, HASH_VALUE_LENGTH);

	// Read the program output into the buffer
	if(fread(hashValue, sizeof(char), sizeof(char) * HASH_VALUE_LENGTH, progOutput) < 0)
	{
		perror("fread");
		exit(-1);
	}

	// Send hash to the parent
	if(write(childToParentPipe[WRITE_END], hashValue, sizeof(hashValue)) < 0)
	{
		perror("write");
		exit(-1);
	}

	// Child terminates
	exit(0);
}
