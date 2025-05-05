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

bool endrt = false;
// something
typedef struct optindex {
	int indabort;
	int indpipe;
	int inddry;
	int indfile;
	int indjob;
	int indpertask;
} opt_index;

typedef struct COMMAND {
	char** array;
	int length;
} COMMAND;

/* MAXJOBS
  
 * have an if mflg thing that is a while loop instead of a for loop
 * and have a check to spawn children only while the number is less than the max*
 * function pointer???
 */
void sigfunc(int s);
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
int min(int x, int y);
char** append_to_array (char** array1 , char** array2);
void spawn_child_exec (char** cmd);
void free2darray (char** argv, int argc);
COMMAND parse_cmd (int argc, char** argv, int optind);
COMMAND parse_options (int argc, char** argv);
char*** parse_cmd_file(FILE *file, int jobcount);
char** remove_arguments (int argc, char* argv[]); 
void dryfile(FILE* file, int pflg, int cmdflg, int argc, char** argv, int optind);
void drypt (int argc, char** argv, char** pt_args, int pt_arg_count, int optind, int pflg);
void drynoarg (int argc, char** argv, int optind);
void argsfile(FILE *file, int pflg, int jobcount); 	
void no_args(void); 
int spawn_maxjobs(int totaljobs, int maxjobs, char** cmd);
void pipeline(char*** command_vector, int cmdcount);

// SA_NOCLDSTOP signal so only checks if child dies not if child stops
// char** split_space_not_quote(char *input, int *numtokens);
// SIGINT sent to all processes in group
// KILL is only send to one so it can leave orphans
// EMPTY COMMAND LINES SHOULD NOT BE EXECUTED
// EMPTY STRING INPUT WITH FIXED ARGS IS IN THE FORMAT ./uqparallel "" [fixed-args...]
// TEMPORARY MAGIC NUM = 50
// pipeline needs to wait on final exit status or maybe it returns an int which is the final exit status instead

// NEED TO EXIT ON LAST CHILD EXIT STATUS NOT 0
// DRYRUN NEEDS TO WORK WITH CMDS
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
	int cmdflg = 0;
	int errflg = 0;
	int argument_count = 0;
	int maxjobs = -1;
	int pertask_args_count = 0;
	char** pertask_args = NULL;
	char* filename = NULL;
	opt_index option_index = {.indabort = 0, .indpipe = 0, .indpertask = 0,
							  .inddry = 0, .indfile = 0, .indjob = 0};
	int exit_status = 50;
	struct sigaction sa;			// could probably put sighandler after option parsing so if abort-on-error then include SA_NOCLDSTOP
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = sigfunc;
	sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
	sigaction(SIGTERM | SIGINT, &sa, 0); // i think

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


	if (argc > 1 && cmd_check(argv[1],options)) {
		
		if (strcmp(argv[1], "") == 0) {
			cmd_err();
		}
		else {
			//no_args();	
		}
	}

	COMMAND option_array = parse_options(argc, argv); // to only parse -- long options so -a and --abort-on-error dont get mixed up

	// prob should else this whole while statement
	while ((c = getopt_long(option_array.length, option_array.array, "dpa:m:f:", options, &optind)) != -1) { //reference	https://www.man7.org/linux/man-pages/man3/getopt.3.html 
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

	for (int i = 0 ; i < argc ; i++) {
		if (strcmp(argv[i], ":::") == 0) {
			if (fflg != 0) {
				cmd_err();
			}
			else {
			ptflg++;
			errflg++;

			option_index.indpertask = i; // index the location of :::
			pertask_args_count = argc - i - 1;	
			pertask_args = malloc(pertask_args_count * sizeof(char*)); // might need to +1 for null terminator 
			// POPULATE ARRAY OF PERTASK ARGUMENTS
			for (int j = 0 ; j < pertask_args_count ; j++) {
				pertask_args[j] = strdup(argv[i + 1 + j]); 
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
	if (argv[optind] != NULL) {
		if (cmd_check(argv[optind], options)) {
			cmdflg++;
		}
	}
		
														/* Actual Executions */
//----------------------------------------------------------------------------------------------------------------------------------------//
	// dryrun
	if (dflg) {
		if (fflg) {
			dryfile(inputFile, pflg, cmdflg, argc, argv, optind);
		}
		else if (ptflg) {
			drypt(argc, argv, pertask_args, pertask_args_count, optind, pflg);
		}
		else {
			drynoarg(argc, argv, optind);
		}

	}	

	// run on file
	else if (fflg) {
		int jobcount = count_jobs(inputFile);
		rewind(inputFile);

		if (cmdflg) { // cmd present with file
			COMMAND cmd = parse_cmd(argc, argv, optind);
			int status;
			pid_t* pids = malloc(sizeof(pid_t) * jobcount);
			char*** file_cmd_array = parse_cmd_file(inputFile, jobcount);
			char*** exec_array = malloc(jobcount * sizeof(char**));

			for (int i = 0 ; i < jobcount ; i++) {
				exec_array[i] = append_to_array(cmd.array, file_cmd_array[i]);
			}

			for (int i = 0 ; i < jobcount ; i++) {
				spawn_child_exec(exec_array[i]);
			}

			for (int i = 0 ; i < jobcount ; i++ ) {
				waitpid(pids[i], &status, 0);
			}	

			// FREE'ing
			for (int i = 0 ; i < jobcount ; i++) {
				free(exec_array[i]);
			}
			free(file_cmd_array);
			free(exec_array);
			free(pids);
			exit(status);

		}
		else {
			argsfile(inputFile, pflg, jobcount);
		}
	}
	else if (ptflg) { // run per task arugments
		if (cmdflg) {
			COMMAND cmd = parse_cmd(argc, argv, optind);
			char*** exec_array = pertask_append(pertask_args, cmd.array, cmd.length, pertask_args_count);
			
			for (int i = 0 ; i < pertask_args_count ; i++) {
				spawn_child_exec(exec_array[i]);
			}

			// WAITING
			wait(0);

			//FREE'ing
			for (int i = 0 ; i < pertask_args_count ; i++) {
				free(exec_array[i]);
			}
			free(exec_array);

		}
		else { // NO CMD GIVEN
			char*** exec_array = malloc(pertask_args_count * sizeof(char**));

			for (int i = 0 ; i < pertask_args_count ; i++) {
				exec_array[i][0] = strdup(pertask_args[i]);
			}

			for (int i = 0 ; i < pertask_args_count ; i++) {
				spawn_child_exec(exec_array[i]);
			}
			
			// WAITING
			wait(0);

			// FREE'ing
			for (int i = 0 ; i < pertask_args_count ; i++) {
				free(exec_array[i][0]);
			}
			free(exec_array);

		}
	}

	else {
		no_args();		
	}
	


	// FREE MEMORY
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

void sigfunc (int s) {
	endrt = true;
}

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

int min(int x, int y) {
	int result;
	result = (x < y) ? x : y;
	return result;
}

char* strcombine(int count, char* strs[]) {
	char* buffer = strdup(strs[0]);
	if (count == 1) {
		return buffer;
	}

	for (int i = 1 ; i < count ; i++) {
		strcat(buffer, " ");
		strcat(buffer, strs[i]);
	}
	return buffer;
	/*
	free(buffer);
	*/
}

bool cmd_check (char* cmd, struct option options[]) { // returns true if is a command, false if is an option 
	for (int i = 0 ; i < 5 ; i++) { //number of possible options
		if (!strstr(cmd,options[i].name)) {
			if (i == 4) {
				return true;
			}
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
		if (i == size - 1) {
			fprintf(stdout, "%s", newstr);
		}
		else {
			fprintf(stdout, "%s ", newstr);
		}
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

	while (cmdlines[i] != NULL) {
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

COMMAND parse_options (int argc, char** argv) {
	COMMAND options;
	options.length = 1;
	options.array = malloc(argc*sizeof(char*));
	options.array[0] = strdup(argv[0]);

	for (int i = 0 ; i < argc ; i++) { // getopt_long() starts at argv[1]
		if (strstr(argv[i],"--")) {
			options.array[options.length] = strdup(argv[i]);
			options.length++;
		}
		if (strstr(argv[i],"args-file") || strstr(argv[i],"maxjobs")) {
			if (argv[i+1] != NULL && !strstr(argv[i+1],"--")) {
				options.array[options.length] = strdup(argv[i+1]);
				options.length++;
			}
		}
	}
	options.array = realloc(options.array,(sizeof(char*) * options.length) + 1);
	options.array[options.length] = NULL;
	return options;
	/*
	 * TO FREE
	for (int i = 0 i < options.length ; i++) {
		free(options.array[i]);
	}
	free(options.array);
	*/
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

// appends array2 to array 1
char** append_to_array (char** array1 , char** array2) { 
	int len1 = count_cmd(array1);
	int len2 = count_cmd(array2);
	char** buffer = malloc( (len1+len2) * sizeof(char*) );

	for (int i = 0 ; i < len1 ; i++) {
		buffer[i] = strdup(array1[i]);
	}
	for (int j = 0 ; j < len2 ; j++) {
		buffer[len1+j] = strdup(array2[j]);
	}

	buffer[len1+len2] = NULL;

	return buffer;
/*
 	for (int i = 0 ; i < len1 + len2 ; i++){
		free(array1[i]);
	}
 															*/
}
			

void spawn_child_exec (char** cmd) {
	if (!fork()) {
		if (!strcmp(cmd[0],"") || cmd[0] == NULL){
			exit(78);
		}
		execvp(cmd[0], cmd);
		fprintf(stderr, "uqparallel: \"%s\" not able to be executed\n", cmd[0]);
		fflush(stderr);
		fflush(stdout);
		exit(78); //unsure how this exit code shd work
	}
}

// parses comands from argv from the optind which points at the last option argument
COMMAND parse_cmd (int argc, char** argv, int optind) {
	char** buffer = malloc( sizeof(char*) * (argc-1)); 
	int index = 0;
	COMMAND command;
	int numtokens;

	for (int i = optind ; i < argc ; i++) {
		buffer[index] = strdup(argv[i]);	
		index++;
	}

	char* buffer2 = strcombine(argc-optind, buffer);

	// FREE buffer
	for (int i = 0 ; i < argc-1 ; i++) {
		free(buffer[i]);
	}
	free(buffer);

	command.array = split_space_not_quote(buffer2, &numtokens);
	command.length = numtokens;

	return command;
}

// returns array of commands from file
char*** parse_cmd_file(FILE *file, int jobcount) {
	char buffer[50];
	int index = 0;
	int numtokens;

	char** cmd_array = malloc(jobcount * sizeof(char*)); // stores cmds in array rather than char**
	char*** exec_array = malloc(jobcount * sizeof(char**)); // stores pointers to cmd_array in array

	// GET STRING FROM FILE AND STORE IN ARRAY
	while (fgets(buffer, sizeof(buffer), file)) {
		char* strbuffer = remove_NL(buffer);
		char** cmds = split_space_not_quote(strbuffer, &numtokens);

		if (cmds == NULL) {
			continue;
		}

		for (int x = 0 ; x < numtokens+1; x++) {	
			cmd_array[x] = cmds[x];
		}

		exec_array[index] = malloc((numtokens+1) * sizeof(char*) + sizeof(int)); //unsure about this one but meant to include null terminator
		
		for (int i = 0 ; i < numtokens ; i++) {
			exec_array[index][i] = strdup(cmd_array[i]); 
		}
		exec_array[index][numtokens+1] = NULL;
		index++;
	}
	free(cmd_array);
	return exec_array;
}

void free2darray (char** argv, int argc) {
	for (int i = 0 ; i < argc ; i++) {
		free(argv[i]);
	}
	free(argv);
}

													/* Working Functions */
//------------------------------------------------------------------------------------------------------------------------------//
void dryfile(FILE* file, int pflg, int cmdflg, int argc, char** argv, int optind) {
	// redo this thing with split_space_not_quote
	int jobnum = 1;
	char buffer[50];	
	rewind(file);
	if (cmdflg) {
		COMMAND cmd = parse_cmd(argc, argv, optind);
		while (fgets(buffer, sizeof(buffer), file) != NULL) {
			int numtokens;
			char** buffer2 = split_space_not_quote(buffer, &numtokens);
			
			fprintf(stdout, "%d: %s", jobnum, cmd.array[0]);
					 
			for (int i = 1 ; i < cmd.length; i++) {
				fprintf(stdout, " %s", cmd.array[i]);
			}

			for (int i = 0 ; i < numtokens; i++) {
				fprintf(stdout, " %s", buffer2[i]);
			}

			if (pflg) {
				fprintf(stdout, " |\n");
			}
			else {
				fprintf(stdout, "\n");
			}
		jobnum++;
		free(buffer2);
		}
	free(cmd.array);
	}
	else {
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
	return;
}
void drypt (int argc, char** argv, char** pt_args, int pt_arg_count, int optind, int pflg) {
	int jobnum = 1;
	int cmd_count = argc - optind;

	char*** exec_array = malloc(pt_arg_count * sizeof(char**));
	char** cmd_array = malloc(cmd_count * sizeof(char*)); 

	// POPULATE ARRAY OF COMMANDS
	for (int i = 0 ; i < cmd_count ; i++) {
		cmd_array[i] = strdup(argv[optind+i]);
	}

	// NEW ARRAY OF APPENDED COMMANDS
	exec_array = pertask_append(pt_args, cmd_array, cmd_count, pt_arg_count); 

	while (jobnum <= pt_arg_count) {
		fprintf(stdout, "%d: ", jobnum);
		print_array(cmd_count + 1, exec_array[jobnum-1]); // smells like a segfault
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

void drynoarg (int argc, char** argv, int optind) {
	int jobnum = 1;
	char buffer[50];
	char** cmd_array = malloc((argc-optind) * sizeof(char*)); //might be unecessary
	
	for (int i = 0 ; i < (argc-optind) ; i++) {
		cmd_array[i] = strdup(argv[optind+i]);
	}
	while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
		int numtokens;
		char** outstr = split_space_not_quote(buffer, &numtokens);
		fprintf(stdout, "%d: ", jobnum);
		print_array(argc-optind, cmd_array);
		print_array(numtokens, outstr); // something missing here but should definitely split_space_not_quote
		fprintf(stdout, "\n");
		jobnum++;
	}
	free(cmd_array);
	return;
}

void argsfile(FILE *file, int pflg, int jobcount) {
	char*** exec_array = parse_cmd_file(file,jobcount);

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
		if (exec_array[i][0] == NULL) {
			continue;
		}
		if (!(pids[i] = fork())) {
			execvp(exec_array[i][0], exec_array[i]); 
			fprintf(stderr, "uqparallel: \"%s\" not able to be executed\n", exec_array[i][0]);
			fflush(stdout);
			exit(78); // UNSURE ABOUT THIS EXIT STATUS
		}
	}

	// WAIT FOR DEATH
	for (int i = 0 ; i < jobcount ; i++ ) {
		waitpid(pids[i], &status, 0);
	}	

	// FREE MEMORY ARRAY
	for (int i = 0 ; i < jobcount ; i++) {
		free(exec_array[i]);
	}
	//might need to free one more line not sure
	
	// FREEING MEMORY 
	free(pids);
	exit(status);

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

int spawn_maxjobs(int totaljobs, int maxjobs, char** cmd) {
	int numChildren = 0;
	int jobnum = 0;
	int status;
	
	while (jobnum <= totaljobs) {
		if (numChildren < maxjobs) {
			jobnum++;
			spawn_child_exec(cmd);
		}
		if (endrt) {
			pid_t pid;
			while (pid = waitpid(-1, &status, WNOHANG)) {
				numChildren--;
			}
			endrt = false;
		}
	}
	return status;
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


