#include <stdio.h>
#include <tinyexpr.h>
#include <stdlib.h>
#include <string.h>

#define TRUE 1
#define FALSE 0

/* LOG */
/* 

 * redundancy in exit(9) state, could make it all 1 big void retun checking function
	
																						*/

int argument_check (int argc, char** input, char* arguments) {

	for (int i = 1 ; i < argc ; i++) {
		for (int j = 0 ; j < 3 ; j++) {
			if (strstr(input[i], arguments[j])) {
					return j;
			}
		}
	}
	return -1; // returning -1 will send it to default case
}

void valid_use(int argc, char* argv[]) { 
	for (int i = 1 ; i < argc - 1 ; i+=2) {  // skip over any strings that would follow the arguments
		if (strstr(argv[i],"--") == FALSE) { // if any file before the arguments exit program
		fprintf(stderr, "Usage: ./uqexpr [--looping string] [--sigfigures 2..9] [--initialise string] [filename]\n");
		exit(9); 
		}
	}
}

int main (int argc, char* argv[]) //include some while clause for EOF e.g fgets()
{

    argc--; // argc starts at first argument not at ./uqexpr
	char* arguments[3] = {"--initialise", "--sigfigures", "--looping"};
	int init_count = 0;
	int sigfigs = 0;

	// for initialising variables
	for (int i = 1 ; i < argc - 1; i++) {
		if (strstr(argv[i], arguments[1])) {
			init_count++;
		}
	}
	te_variable teVars[init_count];	
	double values[init_count];

	// for loop variables
	char* loop_var[]; //smth wrong here

	// command line argument stuff
    char* expression = malloc(sizeof(char**)); 
	if (argc > 1 ) { 
		valid_use(argc, argv);
		int value_array = 0;
		for (int i = 1 ; i < argc - 1 ; i++) {		// find which arguments to run (-1 to exclude filename)
			switch(argument_check(argc, argv[i], arguments)) {

				case 0: //initialise variable
				values[value_array] = value_array;
				te_variable var = {.name = argv[i+1], 
								.address = &(values[value_array]), 
								.type = TE_VARIABLE, 
								.context = NULL};
				teVars[value_array] = var;
				value_array++;
				break;

				case 1:
				if (isalpha(argv[i+1][1] == 0)) {
						fprintf(stderr, "uqexpr: invalid variable(s) were specified\n");
						//free values
						exit(6);
						}
				//store in array [var,start,inc,end]
				char* temp = strtok(argv[i+1],",");
				for (int j = 0 ; j < strlen(argv[i+1]) ; j++) {
					strcat(loop_var,temp[j]);	//need to be stored as double at some point
					temp = strtok(NULL,",");

				}
				if (strlen(loop_var) != 4) {
					exit(); // find exit code
				}
				break;

				case 2:
				sigfigs = argv[i+1];	
				if (sigfigs == 1) {
					fprintf(stderr, "Usage: ./uqexpr [--looping string] [--sigfigures 2..9] [--initialise string] [filename]\n");
					exit(9); 
				}
				break;

				default: break;
			}
		}
	} 
	else if (argc == 1) { //put this in the first if() statement?
		if (strstr(argv[1],"./")) {
			char* inputfile = strtok(argv[1],"./");
			FILE *file = fopen(inputfile,"r");
			
			//invalid file
			if (file == NULL) {
				fprintf(stderr,"uqexpr: can't open file %s for reading",argv[1]);
				//free expressions
				exit(9);
			}
			// run through main?
		}
	}
	else {			// if no arguments run normal calc 
		printf("No loop variables were found\n");
		scanf("%s", expression);
		if (expression){
			printf("Result: %d\n", te_interp(expression),0);
		}
    }


	fclose(file);
    return 0;
}
