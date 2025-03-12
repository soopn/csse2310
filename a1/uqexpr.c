#include <stdio.h>
#include <tinyexpr.h>
#include <stdlib.h>
#include <string.h>

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

int argument_check (int argc, char* input) {

	char* arguments[3] = {"initialise" , "looping" , "sigfigures"};

	for (int i = 1 ; i < argc ; i++) {
		for (int j = 0 ; j < 3 ; j++) {
			if (strstr(input[i], arguments[j])) {
					return j;
			}
		}
	}
	return 0; //unsure about this return value maybe greater than 3?
}

bool valid_use(int argc, char* argv[]) {
	for (int i = 1 ; i < argc - 1 ; i++) {
		if (strstr(argv[i],"--") == FALSE) {
				return FALSE;
		}
	}
	return TRUE;
}

int main (int argc, char* argv[]) //include some while clause for EOF e.g fgets()
{
	char* arguments[3] = {"initialise", "sigfigures", "looping"};

    argc--; // argc starts at first argument not at ./uqexpr
    char* expression = malloc(sizeof(char**));
	
	if (argc > 1 && valid_use(argc, argv)) { 
		fprintf(stderr, "Usage: ./uqexpr [--looping string] [--sigfigures 2..9] [--initialise string] [filename]\n");
		return 9;
		}
    if (argc < 1) {			// if no arguments run normal calc 
        printf("No loop variables were found\n");
        scanf("%s", expression);
    }
	for (int i = 1 ; i < argc ; i++) {		// find which arguments to run
		switch(argument_check(argc, argv[i])) {
				case 0:
				initialise();
				break;

				case 1:
				looping();
				break;

				case 2:
				sigfigures();
				break;

				default: break;
		}
	
	}


    return 0;
}
