#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <csse2310a3.h>
#include <stdbool.h>
#include <getopt.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

typedef struct optindex {
	int indabort;
	int indpipe;
	int inddry;
	int indfile;
	int indjob;
	int indpertask;
} opt_index;

void cmd_err();
int isnum(char* string);
int option_index_calc (int optind, int abflg, int pflg, int dflg, int fflg, int mflg);
void arg_dup_check(int* flags); 
char* strcombine(int count, char* strs[]); 
bool cmd_check (char* cmd, struct option options[]); 
void dryrun(FILE *file, int fflg, int pflg, int optind, int argc, char* argv[]); 
char* remove_NL (char* string); 
int count_jobs (FILE *file); 
int count_cmd (char** cmdlines);
void argsfile(FILE *file); 
//void no_args(void); 
char*** per_task (char** argument_array, char** argv,int cmdcount, int argcount); 

// SA_NOCLDSTOP signal so only checks if child dies not if child stops
// char** split_space_not_quote(char *input, int *numtokens);
// SIGINT sent to all processes in group
// KILL is only send to one so it can leave orphans
// TEMPORARY MAGIC NUM = 50

int main(int argc, char* argv[]) {

	if (argc == 1) {
		//no_args();
	}

	/* Possible implementation

	 * parse with split_space_not_quote()
	 * use that new vector with getopt_long()
	 * if the first argument besides ./uqparallel not an argument then the rest is just fixed argument of the cmd
	 * except for :::

	 * need to account for SIGINT signal
	 * REMEMBER TO FREE strdup()
	*/

	//cmd parsing
	int c;
	int optind = -1;
	opterr = 0;
	int fflg=0; int abflg=0; int pflg=0; int mflg=0; int dflg=0; int ptflg=0;
	int errflg = 0;
	int argument_count = 0;
	int maxjobs = -1;
	char** pertask_args = NULL;
	int pertask_args_count = 0;
	opt_index option_index = {.indabort = 0, .indpipe = 0, .indpertask = 0,
							  .inddry = 0, .indfile = 0, .indjob = 0};

	//option arguments
	FILE *inputFile = NULL;

	static struct option options[] = {
		{"abort-on-error", 	no_argument, 		0,	'a'},
		{"args-file", 		required_argument, 	0,	'f'},
		{"dryrun", 			no_argument, 		0,	'd'},
		{"pipe", 			no_argument, 		0,	'p'},
		{"maxjobs", 		required_argument, 	0,	'm'},
		{0, 				0, 					0, 	0}
	};


	if (cmd_check(argv[1],options)) {
		int numtokens;
		char* bufferstr = strcombine(argc,argv); 
		char** inputcmd = split_space_not_quote(bufferstr, &numtokens); 
		// returns array of chars like {"./uqparallel" , "--dryrun"}
		// PROBLEM, ARRAY HAS MAX SIZE TO MALLOC 
	}
	// prob should else this whole while statement
	while ((c = getopt_long(argc, argv, "dpa:m:f:", options, &optind)) != -1) { //reference	https://www.man7.org/linux/man-pages/man3/getopt.3.html 
		switch (c) {
			case 'a':	
				abflg++;
				printf("abort-on-error\n");
				option_index.indabort = optind;
				break;
			case 'f':
				fflg++;
				inputFile = fopen(optarg, "r"); //might have to replace with open() later on	
				if (!inputFile) {
					fprintf(stderr, "uqparallel: Cannot open file \"%s\" for reading\n", optarg);
					exit(2);
				}
				option_index.indfile = optind;
				break;

			case 'd':	//dryrun
				dflg++;
				option_index.inddry = optind;

				break;

			case 'p':	//pipe
				pflg++;
				printf("pipe\n");
				option_index.indpipe = optind;
				break;

			case 'm': 	//maxjobs
				mflg++;
				printf("MAX JOBS: %s\n",optarg);

				int buffer = isnum(optarg);
				if (buffer < 1 || buffer > 130) { // 1 < n <= 130
					cmd_err();
				}
				maxjobs = buffer;
				option_index.indjob = optind;

				break;
				
			case '?':
				cmd_err();
				break;
			
			case ':':
			  cmd_err();
			  break;

			default: 
				break;
		}
		/*
		//possible implementation, store all seen arguments in a vector, iterate over that vector
		//use those as the cmd arguments
		//do that last and do the :::, finish off with iterating over all the seen arguments and comparing if they came up
		//store all the new commands in a new array so you can do that with dryrun
		*/
			
	}

/* DEBUGGING */	
///////////////////////////////////////////////
	for (int i = 0 ; i < argc ; i++) {
		printf("%s ",argv[i]);
	}
	printf("\n");
	printf("\n");
//////////////////////////////////////////////

	int flg_array[] = {abflg, pflg, dflg, fflg, mflg};
	arg_dup_check(flg_array);
	optind = -1;
	optind = option_index_calc(optind, abflg, pflg, dflg, fflg, mflg);

	for (int i = 0 ; i < argc ; i++) {
		if (strcmp(argv[i], ":::") == 0) {
			if (fflg != 0) {
				cmd_err();
			}
			else {
			printf(":::\n"); // run ::: per-task-args on next line
			errflg++;

			option_index.indpertask = i; // index the location of :::
			int pertast_args_count = argc - i;	
			pertask_args = malloc(pertask_args_count * sizeof(char*)); // might need to +1 for null terminator 
			// POPULATE ARRAY OF PERTASK ARGUMENTS
			for (int j = 0 ; j < argc - pertask_args_count ; j++) {
				pertask_args[j] = argv[pertask_args_count + 1 - j];
			}
			continue;
			}
		}
	}

	if (pflg && !(!errflg ^ !fflg)) {
		cmd_err();
	}

														/* Actual Executions */
//----------------------------------------------------------------------------------------------------------------------------------------//
	// dryrun
	if (dflg) {
		dryrun(inputFile, fflg, pflg, optind, argc, argv);
	}	

	// run on file
	else if (fflg) {
		argsfile(inputFile);
	}

	else if (pflg) {
	}
	
	else {
		
	}

	//if (argc == 1) for ./uqparallel case
	return 0;
}

														/* Helper Functions */
//-----------------------------------------------------------------------------------------------------------------------------------------//

void cmd_err(){
	fprintf(stderr,"Usage: ./uqparallel [--dryrun] [--abort-on-error] [--maxjobs n] [--pipe] [--args-file argument-filename] [cmd [fixed-args ...]] [::: per-task-args ...]\n");
	exit(14);
}
int isnum(char* string) {
	int len = strlen(string);
	
	for (int i = 0 ; i < len ; i++) {
		if (!isdigit(string[i])) {
			return 0;
		}
	}
	return atoi(string);
}
void arg_dup_check(int* flags) {
	for (int i = 0 ; i < 5 ; i++) { // number of flags
		if (flags[i] > 1) {
			cmd_err();
		}
	}
	return;
}

int option_index_calc (int optind, int abflg, int pflg, int dflg, int fflg, int mflg) {	
	optind = (optind < 0) ? 1 : optind;
	if (abflg || pflg || dflg) {
		int flgsum = abflg + pflg + dflg;
		optind = optind + flgsum;
	}
	if (fflg || mflg) {
		int flgsum = fflg + mflg;
		optind = optind + 2*flgsum ;
	}
	return optind;
}

char* strcombine(int count, char* strs[]) {
	char* buffer = strdup(strs[0]);
	for (int i = 1 ; i < count ; i++) {
		strcat(buffer, " ");
		strcat(buffer, strs[i]);
	}
	return buffer;
}

bool cmd_check (char* cmd, struct option options[]) { // returns true if is a command, false if is an option 
	for (int i = 0 ; i < 5 ; i++) { //number of possible options
		if (!strstr(cmd,options[i].name)) {
			continue;
		}
		else {
			break;
		}
		return true;
	}
	return false;
}

void print_array(int size, char* array[]) { // prints array separated by whitespace
	char* buffer;
	char* newstr;
	for (int i = 0 ; i < size ; i++) {
		buffer = strdup(array[i]);
		newstr = remove_NL(buffer);	
		fprintf(stdout, "%s ", newstr);
		free(buffer);
	}
}

char* remove_NL (char* string) {
	int len = (int)strlen(string);
	if (string[len-1] == '\n') {
		string[len-1] = '\0';
		return string;
	}
	else {
		return string;
	}
}

int count_jobs (FILE *file) { // NEEDS TO rewind() BEFORE NEXT fgets() use
	char buffer[50];
	int lines = 0;
	while(fgets(buffer, sizeof(buffer), file) != NULL) {
		lines++;
	}
	return lines;
}

int count_cmd (char** cmdlines) {
	int i = 0;
	int tally = 0;

	while (cmdlines[i][0] != NULL) {
		tally++;
		i++;
	}
	return tally;
}


													/* Working Functions */
//------------------------------------------------------------------------------------------------------------------------------//
void dryrun(FILE *file, int fflg, int pflg, int optind, int argc, char* argv[]) {
	int jobnum = 1;
	char buffer[50];
	
	if (fflg){
		while (fgets(buffer, sizeof(buffer), file) != NULL) {
			char* string = remove_NL(buffer);
			fprintf(stdout, "%d: %s", jobnum, string);

			if (pflg) {
				fprintf(stdout, " |\n");
			}
			else {
				fprintf(stdout, "\n");
			}
			jobnum++;
		}
	}
	else {
		char** cmd_array = malloc((argc-optind) * sizeof(char*)); //might be unecessary
		
		for (int i = 0 ; i < (argc-optind) ; i++) {
			cmd_array[i] = strdup(argv[optind+i]);
			printf("%s\n", cmd_array[i]);
		}
		while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
			fprintf(stdout, "%d: ", jobnum);
			print_array(argc-optind, cmd_array);
			fprintf(stdout,"%s", buffer);
			jobnum++;
		}
		free(cmd_array);
	}
	return; 

}

void argsfile(FILE *file) {
	char buffer[50];
	int index = 0;
	int numtokens;
	int jobcount = count_jobs(file);
	rewind(file); // because jobcount calls fgets(), go back to start of file found from fseek() man page from lectures

	char** cmd_array = malloc(jobcount * sizeof(char*)); // stores cmds in array rather than char**
	char** exec_array[jobcount+1]; // stores pointers to cmd_array in array
	int* numtoken_array = calloc(jobcount,sizeof(int)); // store number of args in each command

	// GET STRING FROM FILE AND STORE IN ARRAY
	while (fgets(buffer, sizeof(buffer), file)) {
		char* strbuffer = remove_NL(buffer);
		char** cmds = split_space_not_quote(strbuffer, &numtokens);

		for (int x = 0 ; x < numtokens+1; x++) {	
			cmd_array[x] = cmds[x];
		}
		numtoken_array[index] = numtokens;

		exec_array[index] = malloc((numtokens+1) * sizeof(char*)+ sizeof(int)); //unsure about this one but meant to include null terminator
		
		for (int i = 0 ; i < numtokens ; i++) {
			exec_array[index][i] = strdup(cmd_array[i]); 
		}
		exec_array[index][numtokens+1] = NULL;
		index++;
		
		// FREE MEMORY (LEADS TO ERROR???)
		//free(cmds);
		//free(strbuffer);
	

	}

	// SPAWNING CHILDREN	
	int status;
	pid_t* pids = malloc(sizeof(pid_t) * jobcount);
	
	for (int i = 0 ; i < jobcount ; i++) {
		if (!(pids[i] = fork())) {
			execvp(exec_array[i][0], exec_array[i]); 
			fflush(stdout);
			exit(78); // UNSURE ABOUT THIS EXIT STATUS
		}
	}

	// WAIT FOR DEATH
	for (int i = 0 ; i < jobcount ; i++ ) {
		waitpid(pids[i], &status, 0);
		if (WIFEXITED(status)) {
			printf("EXITED WITH STATUS %d\n", WEXITSTATUS(status));
		}
		if (WIFSIGNALED(status)) {
			printf("SIGNALLED %d\n", WTERMSIG(status));
		}
	}	

	// FREE MEMORY ARRAY
	for (int i = 0 ; i < jobcount ; i++) {
		free(exec_array[i]);
	}
	//might need to free one more line not sure
	
	// FREEING MEMORY 
	free(pids);
	free(cmd_array);
	free(numtoken_array);

}

// MEANT TO RUN EACH OF THE TASKS GIVEN BY STDIN, IN PARALLEL
/* POSSIBLE IMPLEMENTATION 
 * 
 * store all commands in array like argsfile
 * fork and exec
 * create function to append a char**[] with a char*[]
 * 
*/

/*
void no_args(void) {
	char buffer[50];
	int index = 0;
	int numtokens;
	int jobcount = 1;

	char** cmd_array = malloc(jobcount * sizeof(char*)); // stores cmds in array rather than char**
	int* numtoken_array = calloc(jobcount,sizeof(int)); // store number of args in each command

	// GET STRING FROM FILE AND STORE IN ARRAY
	while (fgets(buffer, sizeof(buffer), stdin)) {
		char* strbuffer = remove_NL(buffer);
		char** cmds = split_space_not_quote(strbuffer, &numtokens);
		
		if (jobcount > 1) {
			cmd_array = realloc(cmd_array, jobcount * sizeof(char*));
		}
			

		for (int x = 0 ; x < numtokens+1; x++) {	
			cmd_array[x] = cmds[x];
		}
		numtoken_array[index] = numtokens;

		cmd_array[index] = malloc((numtokens+1) * sizeof(char*)+ sizeof(int)); //unsure about this one but meant to include null terminator
		
		for (int i = 0 ; i < numtokens ; i++) {
			exec_array[index][i] = strdup(cmd_array[i]); 
		}
		cmd_array[index][numtokens+1] = NULL;
		index++;
		
		// FREE MEMORY (LEADS TO ERROR???)
		//free(cmds);
		//free(strbuffer);
	

	}

	// SPAWNING CHILDREN	
	int status;
	pid_t* pids = malloc(sizeof(pid_t) * jobcount);
	
	for (int i = 0 ; i < jobcount ; i++) {
		if (!(pids[i] = fork())) {
			execvp(exec_array[i][0], exec_array[i]); 
			fflush(stdout);
			exit(78); // UNSURE ABOUT THIS EXIT STATUS
		}
	}

	// WAIT FOR DEATH
	for (int i = 0 ; i < jobcount ; i++ ) {
		waitpid(pids[i], &status, 0);
		if (WIFEXITED(status)) {
			printf("EXITED WITH STATUS %d\n", WEXITSTATUS(status));
		}
		if (WIFSIGNALED(status)) {
			printf("SIGNALLED %d\n", WTERMSIG(status));
		}
	}	

	// FREE MEMORY ARRAY
	for (int i = 0 ; i < jobcount ; i++) {
		free(exec_array[i]);
	}
	//might need to free one more line not sure
	
	// FREEING MEMORY 
	free(pids);
	free(cmd_array);
	free(numtoken_array);
	exit(0);
}
*/

// returns pointer new array of strings with pertask args appended to them
char*** per_task (char** argument_array, char** argv,int cmdcount, int argcount) { //make it append the current argv[] with the pertask arguments
	char*** new_array = malloc(sizeof(char**) * (cmdcount + 2) );	 //include null terminator

	for (int i = 0 ; i < cmdcount+2 ; i++) {
		new_array[i] = malloc(cmdcount * sizeof(char*));
	}
	
	for (int i = 0 ; i < argcount ; i++) {
		for (int j = 0 ; j < cmdcount ; j++ ) {
			new_array[i][j] = strdup(argv[j]);
		}
		new_array[i][cmdcount] = argument_array[i];
		new_array[cmdcount+1] = NULL;
	}
	return new_array;
}



