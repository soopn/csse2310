#include <stdio.h>
#include <tinyexpr.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#define TRUE 1
#define FALSE 0

/* LOG */
/* 

 * redundancy in exit(9) state, could make it all 1 big void retun checking function
 * needs duplicate variable check
 																					
 																						*/
typedef struct {
	char* name;
	double value;
} variable;

typedef struct {
	char* name;
	double start;
	double inc;
	double end;
} loopvar;

void printerr9() {
	fprintf(stderr, "Usage: ./uqexpr [--looping string] [--sigfigures 2..9] [--initialise string] [filename]\n");
	exit(9); 
}

int argument_check (int argc, char* input[], char* arguments[]) {

	for (int i = 1 ; i < argc ; i++) {
		for (int j = 0 ; j < 3 ; j++) {
			if (strstr(input[i], arguments[j])) {
					return j;
			}
		}
	}
	return -1; // returning -1 will send it to default case
}

void valid_use (int argc, char* argv[]) { 
	for (int i = 1 ; i < argc - 1 ; i+=2) {  // skip over any strings that would follow the arguments
		if (strstr(argv[i],"--") == FALSE) { // if any file before the arguments exit program
		printerr9();
		}
	}
}

void valid_var (char* string, variable x) { //incomplete
	int equal_count = 0;
	for (int i = 0 ; i < (int)(strlen(string)) ; i++) {

		if (string[i] == "=") {
			equal_count++;
		}
	}
	if (equal_count != 1) {
		fprintf(stderr, "uqexpr: invalid variable(s) were specified");
		exit(6);
	}
}

/*
 
int valid_expr(char* string, te_variable vars[]) {
	int errPos;
	te_expr* expr = te_compile; // incomplete
}
													*/
int main (int argc, char* argv[]) //include some while clause for EOF e.g fgets()
{

    argc--; // argc starts at first argument not at ./uqexpr
	char* arguments[3] = {"--initialise", "--looping", "--sigfigures"};
	int init_count = 0;
	char sigfigs = 0;

	// number of variables to initialise
	for (int i = 1 ; i < argc - 1; i++) {
		if (strstr(argv[i], arguments[1])) {
			init_count++;
		}
	}
	variable *vars = malloc(init_count * sizeof(variable));
	te_variable *te_vars = malloc(init_count * sizeof(te_variable));

//---------------------------------------------------------------------------------------------------------//

	// for loop variables
	loopvar *loops = malloc(sizeof(loopvar));

	// command line argument stuff
    char* expression = malloc(sizeof(char*)); 	
	if (argc > 1 ) { 
		valid_use(argc, argv);
		int value_array = 0;
		int loop_array = 0;
		for (int i = 1 ; i < argc - 1 ; i++) {		// find which arguments to run (-1 to exclude filename)
			
			// needs a valid use check e.g blank argument after --initialise, should exit 9, print to stderr

			switch(argument_check(argc, argv[i], arguments)) { // 0 = initialise ; 1 = looping ; 2 = sigfigures

				case 0: //initialise variable (declare them in the variable struct first then map them to a te_variable later	
				vars[value_array].name = strdup(strtok(argv[i+1],"="));
				vars[value_array].value = strdup(strtok(NULL,"=")); 
				value_array++;
				break;
//-----------------------------------------------------------------------------------------------------------------//
				case 1: //looping values
				if (isalpha(argv[i+1][1] == 0)) {
						fprintf(stderr, "uqexpr: invalid variable(s) were specified\n");
						//free values
						exit(6);
				}
				//store in array [var,start,inc,end]
								
				if (strlen((char*)(loop_var)) != 4) {
					fprintf(stderr, "uqexpr: invalid variable(s) were specified\n");	
					exit(6); 
				}
				loops[loop_array] = {.name = strdup(strtok(argv[i+1],","),
									 .start = strdup(strtok(NULL,",")),
									 .inc = strdup(strtok(NULL,",")),
									 .end = strdup(strtok(NULL,","))
									};
				loop_array++;	

				break;
//-----------------------------------------------------------------------------------------------------------------//
				case 2: //sig figures
				sigfigs = argv[i+1][1];	
				if (sigfigs == 1 || isdigit(argv[i+1]) == 0) {
					fprintf(stderr, "Usage: ./uqexpr [--looping string] [--sigfigures 2..9] [--initialise string] [filename]\n");
					exit(9); 
				}
				break;

				default: break;
			}
		}
		//mapping variable to te_variable
		for (int i = 0 ; i < init_count ; i++) {
			te_variable var = {.name = vars[i].name,
							   .address = &(vars[i].value),
							   .type = TE_VARIABLE,
							   .context = NULL};
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
		while (!feof(stdin)) {
			scanf("%s", expression); //might be better to use fgets()
			if (expression){
				printf("Result: %d\n", te_interp(expression,0));
			}
		}
		free(expression);
		fprintf(stdout, "Thank you for using uqexpr!");
		exit(0);
    }

//te_free()
	fclose(file);
    return 0;
}
