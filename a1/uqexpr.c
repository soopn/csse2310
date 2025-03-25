#include <stdio.h>
#include <tinyexpr.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>

#define TRUE 1
#define FALSE 0

/* LOG */
/* 

 * include stuff in while loops that end in continues so it starts back at the first if()
 * still doesn't read file in normal case with arguments from ./uqexpr
 * should check if --looping has x,%d,%d,%d format
 * need to redo regular calculation bit
 * needs to print out variable as it gets written from stdin
 * @print
 * @loop
 * recheck all possible exit states
 																					
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

char* sigfigprint(double result, int sigfigs) {
	char* outputstr = (char*)(malloc(16*sizeof(char)));
    int exp = (int)floor(log10(fabs(result)));

    // Compute scaling factor
    double scale = pow(10, sigfigs - 1 - exp);
    double rounded = round(result * scale) / scale;

    // Print scientific notation
    if (exp < sigfigs) {
        snprintf(outputstr, 16, "%.*f", sigfigs - 1 - exp, rounded);
    } else {
        snprintf(outputstr, 16, "%.*e", sigfigs - 1, rounded);
    }
	return outputstr;
}

void printerr(int x) {
	switch (x) {
		case 2:
			fprintf(stderr,"uqexpr: duplicate variables were detected");
			exit(2);
			break;
		
		case 6:
			fprintf(stderr,"uqexpr: invalid variable(s) were specified");
			exit(6);
			break;

		case 9:
			fprintf(stderr,"Usage: ./uqexpr [--looping string] [--sigfigures 2..9] [--initialise string] [filename]");
			exit(9);
			break;
		default: break;
	}
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
		printerr(9);
		}
	}
}


int main (int argc, char* argv[]) //include some while clause for EOF e.g fgets()
{

    argc--; // argc starts at first argument not at ./uqexpr
	char* arguments[3] = {"--initialise", "--looping", "--sigfigures"};
	int init_count = 0;
	int loop_count = 0;
	char sigfigs = 0;
	int value_array = 0;
	int loop_array = 0; // still unused

	// number of variables to initialise
	for (int i = 1 ; i < argc - 1; i++) {
		if (strstr(argv[i], arguments[0])) {
			init_count++;
		}
		if (strstr(argv[i], arguments[1])) {
			loop_count++;
		}
	}
	variable *vars = malloc(init_count * sizeof(variable)); //unsure if i need to cast it to variable* 
	te_variable *te_vars = malloc(init_count * sizeof(te_variable));
	
	// for loop variables
	loopvar *loops = malloc(loop_count * sizeof(loopvar)); // NO free()

//---------------------------------------------------------------------------------------------------------//

	// command line argument stuff
    char* expression = malloc(sizeof(char*)); //NO free()	
	if (argc > 1 ) { 
		valid_use(argc, argv);
		for (int i = 1 ; i < argc - 1 ; i++) {		// find which arguments to run (-1 to exclude filename)

			char varname[sizeof(char*)];
			float val, f1, f2, f3;
			// needs a valid use check e.g blank argument after --initialise, should exit 9, print to stderr

			switch(argument_check(argc, argv[i], arguments)) { // 0 = initialise ; 1 = looping ; 2 = sigfigures

				case 0: //initialise variable (declare them in the variable struct first then map them to a te_variable later	
				if (sscanf(argv[i+1], "%8[^=]=%f",varname,&val) == 2) {
					vars[value_array].name = strdup(varname);
					vars[value_array].value = val; 
					value_array++;
				}
				else {
					printerr(6);
				}
				break;
//-----------------------------------------------------------------------------------------------------------------//
				case 1: //looping values

				if (isalpha(argv[i+1][1] == 0)) {
					printerr(6);
					//free values
				}
				//store in array [var,start,inc,end]
				
				if (sscanf(argv[i+1],"%8[^,]%f,%f,%f",varname,&f1,&f2,&f3)) {
					printerr(6);
				}
				loops[loop_array].name  = strdup(varname);
				loops[loop_array].start = f1;
				loops[loop_array].inc = f2;
				loops[loop_array].end = f3; 
				loop_array++;	
				break;
//-----------------------------------------------------------------------------------------------------------------//
				case 2: /*sig figures */
				if (strlen(argv[i+1]) == 1) {
					sigfigs = argv[i+1][1];	
					if (sigfigs == 1 || isdigit(argv[i+1]) == 0 || strlen(argv[i+1] != 1 )) {
						printerr(9);
					}
					break;
				}	

				default: break;
			}
		}
//---------------------------------------------------------------------------------------------------------------//
		/* mapping variable to te_variable */
		for (int i = 0 ; i < init_count ; i++) {
			te_variable var = {.name = vars[i].name,
							   .address = &(vars[i].value),
							   .type = TE_VARIABLE,
							   .context = NULL};
			te_vars[i] = var;
		} 
		/* check for duplicate variables */
		for (int i = 0 ; i < init_count || i < loop_count ; i++) {
			for (int j = 0 ; j < init_count || j < loop_count ; j++) {
				if (vars[i].name == loops[j].name) {
					fprintf(stderr,"uqexpr: duplicate variables were detected");
					exit(2);
				}
			}
		}
		/* Print out initialised variables */
		if (vars[0].name != NULL) {
			printf("Variables:\n");
			for (int i = 0 ; i < value_array+1 ; i++) { //UNSURE ABOUT THE +1
				if (vars[i].name != NULL) {	
					printf("%s = %s\n",vars[i].name, sigfigprint(vars[i].value, sigfigs));	
				}
				else {break;}
			}
		}	
		else {
			printf("No variables were identified.\n");
		}
		
		/* Print out loop variables */
		if (loops[0].name != NULL) {
			printf("Loop variables:\n");
			for (int i = 0 ; i < loop_array+1 ; i++) { //UNSURE ABOUT THE +1
				if(loops[i].name != NULL) {	
					printf("%s = %s (%s,%s,%s)\n",loops[i].name,
												  sigfigprint(loops[i].start,sigfigs),
												  sigfigprint(loops[i].start,sigfigs),
												  sigfigprint(loops[i].inc,sigfigs),
												  sigfigprint(loops[i].end,sigfigs));
				}
				else {break;}
			}
		}
		else {
			printf("No loop variables were found.\n");
		}
	}
//---------------------------------------------------------------------------------------------//	
	/* Reading filename from arguments */
	else if (argc == 1) {
		if (strstr(argv[1],"./")) { //ignore "./" characters
			char* inputfile = strtok(argv[1],"./");
			FILE *file = fopen(inputfile, "r");
		}
		else {
			char* inputfile = argv[1];
			FILE *file = fopen(inputfile, "r");
		}

		char* string;
		while (!feof(file)) {
			char* varname;
			float val;

			fgets(string, sizeof(string), file);
			//check if string is variable declaration
			if (sscanf(string, "%8[^=]=%f", varname, &val) == 2) {
			//check if variable already exists
				int dupe = 0;
				for (int i = 0 ; i < value_array ; i++) {
					if (strcmp(vars[i],varname) == 1) { //something wrong here
						vars[i].value = val;
						dupe = 1;
						break;
						//NEEDS TO PRINT OUT VARIABLE
					}
				}

			if (dupe == 0) {
				//realloc space for new variable
					value_array++;
					vars = realloc(vars,(value_array+1) * sizeof(variable)); // might be redundant 
					te_vars = realloc(te_vars, (value_array+1) * sizeof(te_variable));

					vars[value_array].name = strdup(varname);
					vars[value_array].value = val;
				//add new variable to array
					te_variable var = {.name = strdup(varname),
									   .address = &val,
									   .type = TE_VARIABLE,
									   .context = NULL};
					te_vars[value_array] = var;
					//NEEDS TO PRINT OUT VARIABLE

				}
			}
			else {
				int err;
				te_expr *expr = te_compile(string, te_vars, value_array, &err);

				if (!expr) {
					fprintf(stderr, "Invalid command, expression or assignment operation detected\n");	
				}
				else {
					double result = te_eval(expr);
					printf("Result: %s\n",sigfigprint(result,sigfigs));
				}
			}	
			
			//invalid file
			if (file == NULL) {
				fprintf(stderr,"uqexpr: can't open file \"%s\" for reading\n",argv[1]);
				//free expressions
				exit(10);
			}
			// run through main?
		}
	}
	else {			// if no arguments run normal calc 
		printf("No loop variables were found\n");
		while (!feof(stdin)) {
			scanf("%s", expression); //might be better to use fgets()
			if (expression){
				printf("Result: %f\n", te_interp(expression,0));
			}
		}
		free(expression);
		fprintf(stdout, "Thank you for using uqexpr!\n");
		exit(0);
    }

//te_free()
	//fclose(file);
    return 0;
}
