#include <stdio.h>
#include <tinyexpr.h>
#include <stdlib.h>
#include <string.h>

#define TRUE 1
#define FALSE 0

/*
bool var_check(char* input){
    for (int i=0 ; i < strlen(input) ; i++){
        if (input[i] != '=') {
            return FALSE;
        }
        else {
            return TRUE;
        }
    }
}
*/

/*
bool eqn_check (char* input) {
    for (int i=0 ; i<strlen(input) ; i++) {
       if (input[i]  
			   }
*/
/*
int argument_check (char* input) {
	if (input == "initialise") {
		initialise();	// incomplete
	}
	else if (input == "sigfigures") {
		sigfigures();	// incomplete
	}
	else if (input == "looping") {
		looping();		// incomplete
	}
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
	char* arguments[3] = {"--initialise", "--sigfigures", "--looping"};
	int init_count = 0;

	for (int i = 1 ; i < argc - 1; i++) {
		if (strstr(argv[i], arguments[1])) {
			init_count++;
		}
	}
	te_variable teVars[init_count];	
	double values[init_count];

    argc--; // argc starts at first argument not at ./uqexpr
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
				//looping();
				break;

				case 2:
				//sigfigures();
				break;

				default: break;
			}
		}
	}
	else if (argc == 1) { //put this in the first if() statement?
		// open file and do stuff make that a function?
	}
	else {			// if no arguments run normal calc 
		printf("No loop variables were found\n");
		scanf("%s", expression);
    }

	/*if (argv[argc-1]) {
		char* inputfile = argv[argc-1];
		FILE *file = fopen("%s", "r");

		// do something with the file

	}

	fclose(file);
	*/
    return 0;
}
