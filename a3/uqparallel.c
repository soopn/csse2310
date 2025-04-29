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
char*** pertask_append (char** argument_array, char** argv,int cmdcount, int argcount); 
void dryrun(FILE *file, int fflg, int pflg, int optind, int argc, char* argv[], int ptlfg,char** pt_args, int pt_arg_count); 
char* remove_NL (char* string); 
int count_jobs (FILE *file); 
int count_cmd (char** cmdlines);
int adjust_argc (int argc, char* argv[]);
char** remove_arguments (int argc, char* argv[]); 
void argsfile(FILE *file, int pflg); 
void no_args(void); 
void pipeline(char*** command_vector, int cmdcount);

// SA_NOCLDSTOP signal so only checks if child dies not if child stops
// char** split_space_not_quote(char *input, int *numtokens);
// SIGINT sent to all processes in group
// KILL is only send to one so it can leave orphans
// EMPTY COMMAND LINES SHOULD NOT BE EXECUTED
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
	int pertask_args_count = 0;
	char** pertask_args = NULL;
	char* filename = NULL;
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


	if (argc != 1 && cmd_check(argv[1],options)) {
		int numtokens;
		char* bufferstr = strcombine(argc,argv); 
		char** inputcmd = split_space_not_quote(bufferstr, &numtokens); 
		// returns array of chars like {"./uqparallel" , "--dryrun"}
		// PROBLEM, ARRAY HAS MAX SIZE TO MALLOC 
	}
	// prob should else this whole while statement
	while ((c = getopt_long_only(argc, argv, "dpa:m:f:", options, &optind)) != -1) { //reference	https://www.man7.org/linux/man-pages/man3/getopt.3.html 
		switch (c) {
			case 'a':	
				abflg++;
				printf("abort-on-error\n");
				option_index.indabort = optind;
				break;
			case 'f':
				fflg++;
				inputFile = fopen(optarg, "r"); //might have to replace with open() later on	
				filename = strdup(optarg);
				option_index.indfile = optind;
				break;

			case 'd':	//dryrun
				dflg++;
				option_index.inddry = optind;
				break;

			case 'p':	//pipe
				pflg++;
				option_index.indpipe = optind;
				break;

			case 'm': 	//maxjobs
				mflg++;

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

/*
	for (int i = 0 ; i < argc ; i++) {
		printf("%s ",argv[i]);
	}
	printf("\n");
	printf("\n");
*/

//////////////////////////////////////////////


	for (int i = 0 ; i < argc ; i++) {
		if (strcmp(argv[i], ":::") == 0) {
			if (fflg != 0) {
				cmd_err();
			}
			else {
			printf(":::\n"); // run ::: per-task-args on next line
			ptflg++;
			errflg++;

			option_index.indpertask = i; // index the location of :::
			pertask_args_count = argc - i - 1;	
			pertask_args = malloc(pertask_args_count * sizeof(char*)); // might need to +1 for null terminator 
			// POPULATE ARRAY OF PERTASK ARGUMENTS
			for (int j = 0 ; j < pertask_args_count ; j++) {
				pertask_args[j] = strdup(argv[i + 1 + j]); // VERY PROBLEMATIC
			}
			continue;
			}
		}
	}

	argv = remove_arguments(argc, argv); // PROBABLE MEMORY LEAK 
	argc = adjust_argc(argc,argv);

	int flg_array[] = {abflg, pflg, dflg, fflg, mflg};
	arg_dup_check(flg_array);
	optind = -1;
	optind = option_index_calc(optind, abflg, pflg, dflg, fflg, mflg); // indexes the last option in the cmdline

	// more argument checking
	if (pflg && !(!errflg ^ !fflg)) {
		cmd_err();
	}
	if (fflg && !inputFile) {
		fprintf(stderr, "uqparallel: Cannot open file \"%s\" for reading\n", filename);
		exit(2);
	}

/* DEBUGGING */	
///////////////////////////////////////////////
/*
	for (int i = 0 ; i < argc ; i++) {
		printf("%s ",argv[i]);
	}
	printf("\n");
	printf("\n");
*/
//////////////////////////////////////////////

														/* Actual Executions */
//----------------------------------------------------------------------------------------------------------------------------------------//
	// dryrun
	if (dflg) {
		dryrun(inputFile, fflg, pflg, optind, argc, argv, ptflg, pertask_args, pertask_args_count);
	}	

	// run on file
	else if (fflg) {
		argsfile(inputFile, pflg);
	}

	else {
		no_args();		
	}
	
	for (int i = 0 ; i < pertask_args_count ; i++) {
		free(pertask_args[i]);
	}
	free(pertask_args);
	free(filename);
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

	while (cmdlines[i][0] != 0) {
		tally++;
		i++;
	}
	return tally;
}

char** remove_arguments (int argc, char* argv[]) {

	for (int i = 1 ; i < argc ; i++) {
		if (strstr(argv[i], ":::")) {
			while (i != argc) {
				argv[i] = NULL;
				i++;
			}
		}
	}
	return argv;
}

int adjust_argc (int argc, char* argv[]) {
	int i;
	for (i = 0 ; i < argc ; i++) {
		if (argv[i] == NULL) {
			break;
		}
	}
	return i;
}

// returns pointer new array of strings with pertask args appended to them
char*** pertask_append (char** argument_array, char** cmds, int cmdcount, int argcount) { //make it append the current argv[] with the pertask arguments
	char*** new_array = malloc(argcount * sizeof(char**));	 //include null terminator
	
	// ALLOCATE MEMORY 
	for (int i = 0 ; i < argcount ; i++) {
		new_array[i] = malloc((cmdcount+2) * sizeof(char*)); // include size for commands and argument and null terminator
	}
	
	for (int i = 0 ; i < argcount ; i++) {
		for (int j = 0 ; j < cmdcount ; j++ ) {
			new_array[i][j] = cmds[j];
		}
		new_array[i][cmdcount] = argument_array[i];
		new_array[i][cmdcount+1] = NULL;
	}
	return new_array;
	
	//THINGS THAT NEED TO BE FREED
/*
	for (int i = 0 ; i < argcount ; i++) {
		for (int j = 0 ; j < cmdcount ; j++) {
			free(new_array[i][j]);
		}
	}
	free(new_array);
												*/
}

void spawn_child_exec (char** cmd) {
	if (!fork()) {
		execvp(cmd[0], cmd);
		fprintf(stderr, "uqparallel: \"%s\" not able to be executed\n", cmd[0]);
		fflush(stderr);
		fflush(stdout);
		exit(78);
	}
}
													/* Working Functions */
//------------------------------------------------------------------------------------------------------------------------------//
void dryrun(FILE *file, int fflg, int pflg, int optind, int argc, char* argv[], int ptflg, char** pt_args, int pt_arg_count) {
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
	else if (ptflg) {
		int cmd_count = argc - optind;

		char*** exec_array = malloc(pt_arg_count * sizeof(char**));
		char** cmd_array = malloc(cmd_count * sizeof(char*)); 

		// POPULATE ARRAY OF COMMANDS
		for (int i = 0 ; i < cmd_count ; i++) {
			cmd_array[i] = strdup(argv[optind+i]);
		}

		// NEW ARRAY OF APPENDED COMMANDS
		exec_array = pertask_append(pt_args, cmd_array, cmd_count, pt_arg_count); 
	
		while (jobnum != pt_arg_count) {
			print_array(cmd_count + pt_arg_count, exec_array[jobnum-1]); // smells like a segfault
			if (pflg) {
				fprintf(stdout, " |\n");
			}
			else {
				fprintf(stdout, "\n");
			}
			jobnum++;
		}

		for (int i = 0 ; i < cmd_count ; i++) {
			free(cmd_array[i]);
		}
		free(cmd_array);
		free(exec_array);
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

void argsfile(FILE *file, int pflg) {
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
	if (pflg) {
		// RUN PIPELINE
		pipeline(exec_array, jobcount);

		// FREE MEMORY ARRAY
		for (int i = 0 ; i < jobcount ; i++) {
			free(exec_array[i]);
		}
		return;
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

void no_args(void) {
	char buffer[50];
	int index = 0;
	int numtokens;
	int jobcount = 1;

	char*** cmd_array = malloc(jobcount * sizeof(char*)); // stores cmds in array 
	int* numtoken_array = calloc(jobcount, sizeof(int)); // store number of args in each command

	// GET STRING FROM FILE AND STORE IN ARRAY
	while (fgets(buffer, sizeof(buffer), stdin)) {
		char* strbuffer = remove_NL(buffer);
		char** cmds = split_space_not_quote(strbuffer, &numtokens);
		
		if (jobcount > 1) {
			cmd_array = (char***)realloc(cmd_array, jobcount * sizeof(char*));
			numtoken_array = (int*)realloc(numtoken_array, jobcount * sizeof(int));
		}
			
		numtoken_array[index] = numtokens;
		cmd_array[index] = cmds;
		spawn_child_exec(cmd_array[index]);
		index++;
		jobcount++;
		wait(0);
		
	}
	
	// FREEING MEMORY 
	//free(pids);
	free(cmd_array);
	free(numtoken_array);
	exit(0);
}

// from **cmds[] cmd1 --> cmd2 --> cmd3 --> ... --> stdout
void pipeline(char*** command_vector, int cmdcount) {
	int** fds = malloc(cmdcount * sizeof(int*));  

	for (int i = 0 ; i < cmdcount ; i++) {
		fds[i] = malloc(2 * sizeof(int));
	}

	// POPULATE PIPES
	for (int i = 0 ; i < cmdcount ; i++) {
		if(pipe(fds[i]) < 0) {
			perror("Creating pipe");
			exit(1);
		}
	}

	for (int i = 0 ; i < cmdcount ; i++) {
		// PARENT
		if (fork()){
			// first command gets ignored
			if (i != 0) {
				dup2(fds[i-1][0], STDIN_FILENO);
			}

			// pipe to next command if not last command
			if (i != cmdcount - 1) {
				dup2(fds[i][1], STDOUT_FILENO); 
			}

			for (int j = 0 ; j < cmdcount - 1;  j++) {
				close(fds[j][0]);
				close(fds[j][1]);
			}

			execvp(command_vector[i][0], command_vector[i]);
			perror("ERROR IN PIPELINE");
			exit(99);
		}
	}
	
	// CLOSE ALL PIPES
	for (int i = 0 ; i < cmdcount - 1 ; i++) {
		for (int j = 0 ; j < 2 ; j++) {
			close(fds[i][j]);
		}
	}

	// WAIT FOR CHILDREN
	for (int i = 0 ; i < cmdcount ; i++) {
		wait(0);
	}

	// FREE MEMORY
	for (int i = 0 ; i < cmdcount ; i++) {
		free(fds[i]);
	}
	free(fds);
	return;
}


