#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <ctype.h>

typedef enum {SINGLE, SEQUENTIAL, PARALLEL} execMode;

const int maxInput = 512; 		// max number of characters you can input
const int maxCommands = 256;	// max number of commands that can be entered

int promptForInput();
void useBatchFile(int infp);
void parseInput(char* buffer);
void executeCommand(char* command, int background, int piping, int redirect, int fd, char* pipeStr);
void printError();

int main(int argc, char** argv) 
{
	// handle if normal shell should be run
	if (argc == 1) {
		int stop = 0;
		while(stop != 1) {
			stop = promptForInput();
		}
	} else if (argc == 2) {
		// handle batch mode
		int inFile = open(argv[1], O_RDONLY);
		if (inFile < 0) {
			// file does not exist
			printError(2);
		}
		useBatchFile(inFile);
		close(inFile);
	} else {
		// handle invalid input
		printError(2);
	}


	return 0;
}

void useBatchFile(int infp) 
{
	char* buffer;
	
	int fileSize = lseek(infp, 0, SEEK_END);
	buffer = malloc((fileSize + 1) * sizeof(char));
	lseek(infp, 0, SEEK_SET);
	read(infp, buffer, fileSize + 1);
	
	char* commands[fileSize / 2];
	int lineCount = 0;
	char* element = strtok(buffer, "\n");
	while(element != NULL) {
		commands[lineCount] = element;
		lineCount++;
		element = strtok(NULL, "\n");
	}
	commands[lineCount] = NULL;

	if(lineCount == 0 && buffer[0] == '\n') {
		write(1, "\n", 1);
	}
	
	int i;
	for(i = 0; i < lineCount; i++) {
		//printf("%s\n", commands[i]);
		
	}

	for(i = 0; i < lineCount; i++) {
		int strSize = strlen(commands[i]);
		write(1, commands[i], strSize);
		write(1, "\n", 1);

		int length = (int)strlen(commands[i]);
		if(length > maxInput) {
			printError(0);
		} else {
			parseInput(commands[i]);
		}
	}
	free(buffer);
	
}

int promptForInput()
{
	// I know sizeof(char) is 1, but for sanity...
	char* buffer = malloc(maxInput * sizeof(char));
	if (buffer == NULL) {
		// malloc failed!
		printError(1);
		return 1;
	}

	printf("357sh> ");
	char* getsCheck = fgets(buffer, maxInput, stdin);
	if (getsCheck == NULL) {
		// fgets had an error! Abort!!
		printError(0);
	}

	// need to check for ctrl+D or ctrl+C?
	
	if (strchr(buffer, '\n') == NULL) {
		// '/n' is not in the buffer - input was more than 512 characters
		printError(0);
	}

	// if it's all valid - let's get split the commands up
	parseInput(buffer);
	free(buffer);

	return 0;
}

// Needs to deal with ; and + and > and |
void parseInput(char* buffer)
{
     // replace newline character with null terminating character
     if (buffer[strlen(buffer) - 1] == '\n') {
    		buffer[strlen(buffer) - 1] = '\0';
     }

     char* commands[maxCommands];
     char* element;
     int numCommands = 0;
     execMode mode;
     int redirect = 0;
     int piping = 0;
     int outfp = -1;
     char* pipeStr;

     if (strchr(buffer, ';') != NULL && strchr(buffer, '+') != NULL) {
     	// string contains BOTH ; and + - not allowed
     	printError(0);
     	return;
     }

     if (strchr(buffer, '>') != NULL && strchr(buffer, '|') != NULL) {
     	// string contains BOTH > and | - not allowed
     	printError(0);
     	return;
     }

     if(strchr(buffer, ';') != NULL) {
     	// sequential
     	mode = SEQUENTIAL;
     } else if (strchr(buffer, '+') != NULL) {
     	// parallel
     	mode = PARALLEL;
     } else {
     	// single command
     	mode = SINGLE;
     }

     if(strchr(buffer, '>') != NULL) {
     	//redirect
     	redirect = 1;
     	char* tmp = strtok(buffer, ">");
     	char* outFile = strtok(NULL, ">");
     	if(strchr(outFile, '>') != NULL) {
     		// only one > should appear
     		printError(0);
     		return;
     	}
     	char* tokOutFile = strtok(outFile, " \t");
     	//printf("accessing \"%s\"\n", tokOutFile);
     	outfp = open(tokOutFile, O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU);
     	if(outfp < 0) {
     		printError(0);
     		return;
     	}
     	buffer = tmp;
     }

     if(strchr(buffer, '|') != NULL) {
     	//redirect
     	piping = 1;
     	char* tmp = strtok(buffer, "|");
     	pipeStr = strtok(NULL, "|");
     	if(strchr(pipeStr, '|') != NULL){
     		// only one | should appear
     		printError(0);
     		return;
     	}
     	pipeStr = strtok(pipeStr, " \t");
     	buffer = tmp;
     }
 
     // check for ;
     if(mode == SEQUENTIAL) {
	     element = strtok(buffer, ";");
	     while (element != NULL) {
			commands[numCommands] = element;
			numCommands++;
			
			element = strtok(NULL, ";");
	     }
	     commands[numCommands] = NULL;
	} else if(mode == PARALLEL) {
		element  = strtok(buffer, "+");
		while (element != NULL) {
			commands[numCommands] = element;
			numCommands++;
			element = strtok(NULL, "+");
		}
		commands[numCommands] = NULL;
	} else if(mode == SINGLE) {
		commands[0] = buffer;
		numCommands = 1;
	}

	int i;
	/*for (i = 0; i < numCommands; i++) {
		printf("%s\n", commands[i]);  
	}*/

	//printf("%d commands\n", numCommands);
	for (i = 0; i < numCommands; i++) { 
		executeCommand(commands[i], mode - 1, piping, redirect, outfp, pipeStr); // Single and Sequential will != 1
	}
	for(i = 0; i < numCommands; i++) {
		wait(NULL);
	}
}

void executeCommand(char* command, int background, int piping, int redirect, int fd, char* pipeStr)
{	  	
	int status;
    int numArguments = 0;
    int numPipeArgs = 0;
    char* arguments[maxCommands];
    char* pipeArgs[maxCommands];
    char* element;
    int pipefd[2];
    int pid, pid2;

    // Parse command into arguments 
    element = strtok(command, " \t");
    while (element != NULL) {
		arguments[numArguments] = element;
		numArguments++;
		element = strtok(NULL, " \t");
    }
    arguments[numArguments] = NULL;

    if(numArguments == 0) {
    	// make sure the argument isn't blank
    	printError(0);
    	return;
    }

    if(piping == 1) {
    	// split up 2nd command
    	element = strtok(pipeStr, " \t");
    	while (element != NULL) {
			pipeArgs[numPipeArgs] = element;
			numPipeArgs++;
		element = strtok(NULL, " \t");
    	}
    	pipeArgs[numPipeArgs] = NULL;
    }
	
    if (strcmp(arguments[0], "quit") == 0) {
    	if(background != 1 && redirect != 1) {
			// exit the program
			//printf("Exiting shell...\n");
			exit(0);
		} else {
			printError(0);
			return;
		}
    } else if (strcmp(arguments[0], "pwd") == 0) {
    	if(background != 1 && numArguments == 1) {
			char* cwd = malloc(100 * sizeof(char));
			if(cwd == NULL) {
				// malloc failed
				printError(1);
			}
			getcwd(cwd, 100);
			int strSize = strlen(cwd);
			write(1, cwd, strSize);
			write(1, "\n", 1);
			free(cwd);
			return;
		} else {
			printError(0);
			return;
		}
    } else if( strcmp(arguments[0], "cd") == 0) {
    	if(background != 1 && redirect != 1) {
			if(numArguments == 1) {
				chdir(getenv("HOME"));
			} else if (numArguments == 2) {
				int dirCheck = chdir(arguments[1]);
				if(dirCheck != 0) {
					printError(0);
				}
			} else {
				printError(0);
				return;
			}
			return;
		} else {
			printError(0);
			return;
		}
    }
    if(piping == 1) {
    	pipe(pipefd);
    }
    // Fork to start a new process
    pid = fork();
    if (pid < 0) {
    		// fork error - ABORT
    		printError(1);
	} else if (pid != 0) {
		// if pid != 0 we know it's the parent
		if(background != 1) {
	     	while(wait(&status) != -1) {
	     		break;
	     	}
	     }
    } else { 
	    // if pid == 0 it's the child
	    //printf("I am child %d\n", pid);
	    if(redirect == 1) {
	    	dup2(fd, 1); // outfd is standard output now
	    }
	    if(piping == 1) {
	    	close(pipefd[0]);
	    	dup2(pipefd[1], 1);
	    }
	    execvp(arguments[0], arguments);
	    // execvp only returns if successful
	    printError(0);
    }

    if(piping == 1) {
    	// 2nd child
    	pid2 = fork();
	    if (pid2 < 0) {
	    		// fork error - ABORT
	    		printError(1);
		} else if (pid2 != 0) {
			// parent do nothing
	    } else { 
		    // if pid == 0 it's the child
	    	close(pipefd[1]);
	    	dup2(pipefd[1], 1);
	    	execvp(pipeArgs[0], pipeArgs);
	    	// execvp only returns if successful
	    	printError(0);
	    }
	    close(pipefd[0]);
	    close(pipefd[1]);
    }
    if(redirect == 1) {
		close(fd);
	}

}

void printError(int errorType)
{
	char errorMessage[30] = "An error has occurred\n";
	int writeCheck = write(STDERR_FILENO, errorMessage, strlen(errorMessage));
	if (writeCheck < 1) {
		// write failed! WRITE AN ERROR FOR THAT TOO
		printError(1);
	}

	if (errorType == 0) {
		// non-terminal error - keep processing
	} else if (errorType == 1) {
		// terminal error - exit the program
		exit(0);
	} else if(errorType == 2) {
		//super error
		exit(1);
	}
}
