/* search.c by Jackson Reed
*  for CS537 - Spring 2014 with Prof. Lu
*
*  This program takes in arguments in the format search 
*  <input-number> <key-word> <input-list> <output> 
*  and searches through the indicated files for the key-word,
*  returning the results either in an ouput file or the console. 
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct
{
	const char* fileName;
	int instances;
	FILE * file;
	char * buffer;
} fileInfo;

int compare(const void *a, const void * b) 
{
	// return 0 if same, positive if a > b, negative if i1 < i2

	const fileInfo *f1 = a;
	const fileInfo *f2 = b;

	if(f1->instances > f2->instances) {
		return -1;
	} else if(f1->instances < f2->instances) {
		return 1;
	} else {
		return 0;
	}
}

int main(int argc, const char* argv[]) 
{
	// argv[0] is file of code
	// argv[1] is the number of input files
	// argv[2] is keyword to search for
	// argv[3] and on are the filenames 

	int numFiles;
	int outSpecified = 0;
	int i;

	// Check if the # of files argument is valid
	char* end;
	numFiles = strtol(argv[1], &end, 10);
	if(*end) {
		fprintf(stderr, "Usage: search <input-number> <key-word> <input-list> <output>\n");
		exit(1);
	}
	
	// Create structs needed
	fileInfo files[numFiles];
	fileInfo outFile; 

	if(numFiles < (argc - 3)) { outSpecified = 1; } // Boolean to track if an output file was specified

	// confirm the correct number of arguments was entered (also handles negatives and too few)
	if(argc < 4 || (argc != (numFiles + 3) && argc != (numFiles + 4))) {
		fprintf(stderr, "Usage: search <input-number> <key-word> <input-list> <output>\n");
		exit(1);
	}

	// try to open all files - input and output
	for(i = 0; i < numFiles; i++) {
		files[i].file = fopen(argv[i + 3], "r");
		if(files[i].file == NULL) {
			fprintf(stderr, "Error: Cannot open file '%s'\n", argv[i + 3]);
			exit(1);
		}
	}

	// Set all filenames
	for(i = 0; i < numFiles; i++) {
		files[i].fileName = argv[i + 3];
	}

	// Deal with opening output file & setting data
	if(outSpecified > 0) {
		outFile.file = fopen(argv[argc - 1], "w");
		if(outFile.file == NULL) {
			fprintf(stderr, "Error: Cannot open file '%s'\n", argv[argc - 1]);
			exit(1);
		}

		outFile.fileName = argv[argc - 1];

		for(i = 0; i < numFiles; i++) {
			if(strcmp(realpath(files[i].fileName, NULL), realpath(outFile.fileName, NULL)) == 0) {
				fprintf(stderr, "Input and output file must differ\n");
				exit(1);
			}
		}
	}

	// Deal with all input files: Read, count, and sort
	for(i = 0; i < numFiles; i++) {
		long fileSize = 0;
		size_t result = 0;

		// determine size of file to read it in
		fseek(files[i].file, 0L, SEEK_END); 	// Go to memory pos at end of file
		fileSize = ftell(files[i].file);    	// Get that memory position (size)
		rewind(files[i].file);			// and back to the beginning of the file

		// Allocate memory and read in data from file
		files[i].buffer = (char*)malloc(sizeof(char) * fileSize + 1);
		if(files[i].buffer == NULL) {
			fprintf(stderr, "Malloc failed");
			exit(1);
		}

		result = fread(files[i].buffer, sizeof(char), fileSize, files[i].file);
		if(result != fileSize) {
			fprintf(stderr, "Read failed");
			exit(1);
		}
		files[i].buffer[fileSize] = '\0';

		files[i].instances = 0;
		char* foundString = strstr(files[i].buffer, argv[2]);
		while(foundString != NULL) {
			files[i].instances += 1;
			// Incrementing by length of the keyword to avoid 'aa' in 'aaa' situations
			foundString = strstr(foundString + strlen(argv[2]), argv[2]);
		}

		// make sure to close the file and free the memory
		if(fclose(files[i].file) != 0) {
			fprintf(stderr, "Error: Could not close file %s\n", argv[i + 3]);
			exit(1);
		}
		free(files[i].buffer);
	}

	// sort the frequencies
	qsort(files, numFiles, sizeof(fileInfo), compare);

	// Deal with writing to ouput file
	if(outSpecified > 0) {
		for(i = 0; i < numFiles; i++) {
			fprintf(outFile.file, "%d %s\n", files[i].instances, files[i].fileName);
		}

		if(fclose(outFile.file) != 0) {
			fprintf(stderr, "ERROR: Could not close file '%s'\n", argv[argc - 1]);
			exit(1);
		}

	} else { // print to stdout if an output file is not specified
		for(i = 0; i < numFiles; i++) {
			fprintf(stdout, "%d %s\n", files[i].instances, files[i].fileName);
		}
	}

	exit(0);
	return 0;
}
