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
 * needs to print out variable as it gets written from stdin
 * @print
 * @loop
 * recheck all possible exit states
 * change printf to fprintf(stdout,...)
 * make it so it doesnt display as float if it doesnt have to
 																					
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

int isint(double x) {
	int num;
	char chr;
	char out[50];
	snprintf(out,50,"%f",x);
	
	if (sscanf(out,"%d%c", &num, &chr) != 2 || chr != '\n') {
		return 0;
	}
	else {
		return 1;
	}
	return 0;
}


int loopingcheck(double start, double inc, double end) {
	if (inc == 0) {
		return 0;
	}
	if (start > end) {
		if (fmod(start-end,inc) !=0 ) {
			return 0;
		}
		return 1;
	}
	if (start < end) {
		if (fmod(end - start, inc) != 0) {
			return 0;
		}
		return 1;
	}
	return 0;
}
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

int var_count(te_variable *X, char* string, int total) {
	int tally = 0;
	for (int i = 0; i < total ; i++) {
		if(strstr(string,X[i].name)) {
				tally++;
		}
	}
	return tally;
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

int argument_check (int argc, char* input, char* arguments[]) {

	for (int j = 0 ; j < 3 ; j++) {
		if (strstr(input, arguments[j])) {
				return j;
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
/*
void clrstr(char* string) {
	for (int i = 0 ; i < (int)strlen(string) ; i++) {
		string[i] = "NULL";
	}
}
														*/
/*
void funcprint (variable *vars, loopvar *loop, int varnum, int loopnum) {
	if (vars[0].name != NULL) {
		for (int i = 0 ; i < varnum; i++) {
			fprintf(stdout,"%s = %s\n",vars[i].name,vars[i].value);
		}
	}
	else {
		fprintf(stdout,"No variables were identified.");
	}

	if (loop[0].name != NULL) {
		for (int i = 0 ; i < loopnum ; i++) { // incomplete
			fprintf(stdout, "%s = %s (%s,%s,%s)",loop[i].name,)
		}
	}
}
															*/
/*
//@loop x "expression"
//if ("@loop @31[a-zA-Z] = %64[^\n] ", Var, eqn) == 2
void funcloop (char* Var, char* eqn, te_variable *vars, loopvar *loop, int varnum, int loopnum, int sigfigs) {
	for (int i = 0 ; i < loopnum ; i++) {
		do {
			te_expr *expr = te_compile(eqn, vars, var_count(vars,eqn, varnum),0 );
			fprintf(stdout, "%s = %s when %s = %s", Var, sigfigprint(te_eval(expr),sigfigs), Var, sigfigprint(loop[i].start,sigfigs));
			te_free(expr);
			loop[i].start += loop[i].inc;
		} 
		while(loop[i].start != loop[i].end); 
	}
}
*/
int main (int argc, char* argv[]) //include some while clause for EOF e.g fgets()
{

	argc--;
	int init_count = 0;
	int loop_count = 0;
	char sigfigs = 0;
	int value_array = 0;
	int loop_array = 0; // still unused
	char* arguments[3] = {"--initialise", "--looping", "--sigfigures"};

	// number of variables to initialise
	for (int i = 1 ; i < argc ; i++) {
		if (strcmp(argv[i], arguments[0]) == 0) {
			init_count++;
		}
		if (strcmp(argv[i], arguments[1]) == 0) {
			loop_count++;
		}
	}
	variable *vars = malloc(init_count * sizeof(variable)); //unsure if i need to cast it to variable* 
	te_variable *te_vars = calloc(init_count ,sizeof(te_variable));
	
	// for loop variables
	loopvar *loops = calloc(loop_count , sizeof(loopvar)); // NO free()

//---------------------------------------------------------------------------------------------------------//

	// command line argument stuff
    char* expression = malloc(sizeof(char*)); //NO free()	
	if (argc > 1 ) { 
		valid_use(argc, argv);
		for (int i = 1 ; i < argc ; i+=2) {		// find which arguments to run (-1 to exclude filename)

			char varname[sizeof(char*)];
			double val, f1, f2, f3;

			// valid use check e.g blank argument after --initialise, should exit 9, print to stderr
			if (argv[i+1] == NULL || strstr(argv[i+1],"--")) {
				printerr(9);
			}

			switch(argument_check(argc, argv[i], arguments)) { // 0 = initialise ; 1 = looping ; 2 = sigfigures

				case 0: //initialise variable (declare them in the variable struct first then map them to a te_variable later	
				if (sscanf(argv[i+1], " %31[a-zA-Z] = %lf",varname,&val) == 2) {
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
				
				if (sscanf(argv[i+1],"%31[a-zA-Z],%lf,%lf,%lf",varname,&f1,&f2,&f3)) {
					if (loopingcheck(f1,f2,f3) == 1) {
						loops[loop_array].name  = strdup(varname);
						loops[loop_array].start = f1;
						loops[loop_array].inc = f2;
						loops[loop_array].end = f3; 
						loop_array++;	

						break;
					}
				}
				else {
					printerr(6); 
					break;
				}
				break;
//-----------------------------------------------------------------------------------------------------------------//
				case 2: //sig figures 
				if (strlen(argv[i+1]) == 1) {
					sigfigs = argv[i+1][1];	
					if (sigfigs == 1 || isdigit(argv[i+1]) == 0 || strlen(argv[i+1]) != 1 ) {
						printerr(9);
					}
					break;
				}	

				default: break;
			}
//---------------------------------------------------------------------------------------------------------------//
			if (i == argc - 1) { //reaches filename
				FILE *inputfile = fopen(argv[i], "r");
				//RUN STUFF ON FILE
			}
		}
// check for duplicate variables 
		for (int i = 0 ; i < init_count || i < loop_count ; i++) {
			for (int j = i+1 ; j < init_count || j < loop_count ; j++) {
				if (vars[i].name == loops[j].name || vars[i].name == vars[j].name || loops[i].name == loops[j].name) {
					fprintf(stderr,"uqexpr: duplicate variables were detected\n");
					exit(2);
				}
			}
		}

		// mapping variable to te_variable 
		for (int i = 0 ; i < init_count ; i++) {
			te_variable var = {.name = vars[i].name,
							   .address = &(vars[i].value),
							   .type = TE_VARIABLE,
							   .context = NULL};
			te_vars[i] = var;
		}
		for (int j = init_count ; j < loop_count+init_count ; j++) {
			te_variable var = {.name = loops[j].name,
							   .address = &(loops[j].start),
							   .type = TE_VARIABLE,
							   .context = "LOOP"};
			te_vars[j] = var;

		} 
				// Print out initialised variables 
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
		
		// Print out loop variables 
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
		while(!feof(stdin)) {
			char *input = (char*)calloc(32,sizeof(char));
			char *ditto = (char*)calloc(32,sizeof(char));
			char *varname = (char*)calloc(32,sizeof(char)); 
			double val,f1,f2,f3;
			scanf(" %[^\n]s", input);
			if (sscanf(input, " %31[a-zA-Z] = %lf",varname,&val) == 2 ) {
			//check if variable already exists
				int dupe = 0;
				for (int i = 0 ; i < value_array ; i++) {
					if (strcmp(vars[i].name,varname) == 1) { //something wrong here
						vars[i].value = val;
						dupe = 1;
						printf("%s = %s", varname, sigfigprint(val,sigfigs));
						break;
					}
					continue;
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
									   .address = &(vars[value_array].value),
									   .type = TE_VARIABLE,
									   .context = NULL};
					te_vars[value_array] = var;

					printf("%s = %s",vars[value_array].name,sigfigprint(vars[value_array].value,sigfigs));
					free(input);
					free(ditto);
					continue;
				}
	
								}
			else if (sscanf(input, "@range %31[a-zA-Z],%lf,%lf,%lf",varname,&f1,&f2,&f3) == 4) {
				if (loopingcheck(f1,f2,f3)) {
					loops[loop_array+value_array].name = strdup(varname);
					loops[loop_array+value_array].start = f1;
					loops[loop_array+value_array].inc = f2;
					loops[loop_array+value_array].end = f3;
					printf("%s = %lf (%lf,%lf,%lf)\n",varname,f1,f1,f2,f3);
					free(input);
					free(ditto);
					continue;
				}
			}
			else if (sscanf(input, " %31[a-zA-Z] = %31[^\n]",varname,ditto) == 2) {
				if (strstr(ditto,varname)) {
					for (int i = 0 ; i < value_array ; i++) {
						if (strcmp(varname, vars[i].name) == 0) {
							te_expr *expr = te_compile(ditto,te_vars,var_count(te_vars,ditto,value_array),0);
							if (expr) {
								vars[i].value = te_eval(expr);
								te_variable var = {.name = strdup(varname),
												   .address = &(vars[i].value),
												   .type = TE_VARIABLE,
												   .context = NULL};
								te_vars[i] = var;
								free(input);
								free(ditto);
								printf("%s = %lf\n",varname,vars[i].value);
								break;	
							}
							else {
							fprintf(stderr,"Invalid command, expression or assignment operation detected\n");	
							te_free(expr);
							free(input);
							free(ditto);
							break;
							}
						}
					}
					continue;
				}
				else {
					te_expr *expr = te_compile(ditto,te_vars,var_count(te_vars,ditto,value_array),0);
					if (expr) {
						vars[value_array].name = strdup(varname);
						vars[value_array].value = te_eval(expr); 
						te_variable var = {.name = varname,
										   .address = &(vars[value_array].value),
										   .type = TE_VARIABLE,
										   .context = NULL};
						te_vars[value_array] = var;
						printf("%s = %lf\n",vars[value_array].name,vars[value_array].value);
						value_array++;
					}
					else {
						fprintf(stderr,"Invalid command, expression or assignment operation detected\n");	
						te_free(expr);
						free(input);
						free(ditto);
						break;
					}
				}
			}
						
			else if (feof(stdin)) {
					fprintf(stdout, "Thank you for using uqexpr!\n");
					free(input);
					free(ditto);
					continue;
			}
			else {
				int err;
				te_expr *expr = te_compile(input, te_vars, var_count(te_vars,input,value_array), &err);
				if (!expr) {
					fprintf(stderr, "Invalid command, expression or assignment operation detected\n");	
					free(input);
					free(ditto);
					continue;
				}
				else {
					fprintf(stdout, "Result = %f\n",te_eval(expr));
					te_free(expr);
					free(input);
					free(ditto);
					continue;
				}
			}

		fprintf(stdout, "Thank you for using uqexpr!\n");
		free(varname);
		//free(expression);
		exit(0);
		} 
	}
	
//---------------------------------------------------------------------------------------------//	
	// Reading filename from arguments 
	else if (argc == 1) {
		FILE *file = NULL;
		if (strstr(argv[1],"./")) { //ignore "./" characters
			char* inputfile = strtok(argv[1],"./");
			file = fopen(inputfile, "r");
		}
		else {
			char* inputfile = argv[1];
			file = fopen(inputfile, "r");
		}

		char* string = NULL;
		while (fgetc(file) != EOF) {
			char* varname;
			double val;

			fgets(string, sizeof(string), file);
			//check if string is variable declaration
			if (sscanf(string, "%8[^=]=%lf", varname, &val) == 2) {
			//check if variable already exists
				int dupe = 0;
				for (int i = 0 ; i < value_array ; i++) {
					if (strcmp(vars[i].name,varname) == 1) { //something wrong here
						vars[i].value = val;
						dupe = 1;
						printf("%s = %s", varname, sigfigprint(val,sigfigs));
						break;
					}
					continue;
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
									   .address = &(vars[value_array].value),
									   .type = TE_VARIABLE,
									   .context = NULL};
					te_vars[value_array] = var;

					printf("%s = %s",vars[value_array].name,sigfigprint(vars[value_array].value,sigfigs));
					continue;
				}
			}
			else {
				int err;
				te_expr *expr = te_compile(string, te_vars, value_array, &err);

				if (!expr) {
					fprintf(stderr, "Invalid command, expression or assignment operation detected\n");	
					continue;
				}
				else {
					double result = te_eval(expr);
					printf("Result: %s\n",sigfigprint(result,sigfigs));
					continue;
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
	if (argc == 0) {			// if no arguments run normal calc 
		printf("No loop variables were found\n");
		while(!feof(stdin)) {
			char *input = (char*)calloc(32,sizeof(char));
			char *ditto = (char*)calloc(32,sizeof(char));
			char *varname = (char*)calloc(32,sizeof(char)); 
			double val,f1,f2,f3;
			scanf(" %[^\n]s", input);
			if (sscanf(input, " %31[a-zA-Z] = %lf",varname,&val) == 2 ) {
					vars[value_array].name = strdup(varname);
					vars[value_array].value = val; 
					te_variable var = {.name = varname,
									   .address = &(vars[value_array].value),
									   .type = TE_VARIABLE,
									   .context = NULL};
					te_vars[value_array] = var;
					printf("%s = %lf\n",vars[value_array].name,vars[value_array].value);
					value_array++;
					free(input);
					free(ditto);
					continue;
			}
			else if (sscanf(input, "@range %31[a-zA-Z],%lf,%lf,%lf",varname,&f1,&f2,&f3) == 4) {
				if (loopingcheck(f1,f2,f3)) {
					loops[loop_array].name = strdup(varname);
					loops[loop_array].start = f1;
					loops[loop_array].inc = f2;
					loops[loop_array].end = f3;
					printf("%s = %lf (%lf,%lf,%lf)\n",varname,f1,f1,f2,f3);
					free(input);
					free(ditto);
					continue;
				}
			}
			else if (sscanf(input, " %31[a-zA-Z] = %31[^\n]",varname,ditto) == 2) {
				if (strstr(ditto,varname)) {
					for (int i = 0 ; i < value_array ; i++) {
						if (strcmp(varname, vars[i].name) == 0) {
							te_expr *expr = te_compile(ditto,te_vars,var_count(te_vars,ditto,value_array),0);
							if (expr) {
								vars[i].value = te_eval(expr);
								te_variable var = {.name = strdup(varname),
												   .address = &(vars[i].value),
												   .type = TE_VARIABLE,
												   .context = NULL};
								te_vars[i] = var;
								free(input);
								free(ditto);
								printf("%s = %lf\n",varname,vars[i].value);
								break;	
							}
							else {
							fprintf(stderr,"Invalid command, expression or assignment operation detected\n");	
							te_free(expr);
							free(input);
							free(ditto);
							break;
							}
						}
					}
					continue;
				}
				else {
					te_expr *expr = te_compile(ditto,te_vars,var_count(te_vars,ditto,value_array),0);
					if (expr) {
						vars[value_array].name = strdup(varname);
						vars[value_array].value = te_eval(expr); 
						te_variable var = {.name = varname,
										   .address = &(vars[value_array].value),
										   .type = TE_VARIABLE,
										   .context = NULL};
						te_vars[value_array] = var;
						printf("%s = %lf\n",vars[value_array].name,vars[value_array].value);
						value_array++;
					}
					else {
						fprintf(stderr,"Invalid command, expression or assignment operation detected\n");	
						te_free(expr);
						free(input);
						free(ditto);
						break;
					}
				}
			}
						
			else if (feof(stdin)) {
					fprintf(stdout, "Thank you for using uqexpr!\n");
					free(input);
					free(ditto);
					continue;
			}
			else {
				int err;
				te_expr *expr = te_compile(input, te_vars, var_count(te_vars,input,value_array), &err);
				if (!expr) {
					fprintf(stderr, "Invalid command, expression or assignment operation detected\n");	
					free(input);
					free(ditto);
					continue;
				}
				else {
					fprintf(stdout, "Result = %f\n",te_eval(expr));
					te_free(expr);
					free(input);
					free(ditto);
					continue;
				}
			}

		fprintf(stdout, "Thank you for using uqexpr!\n");
		free(varname);
		//free(expression);
		exit(0);
		} 
	}

//te_free()
	//fclose(file);
    return 0;
}
