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

typedef struct optindex {
	int indabort;
	int indpipe;
	int inddry;
	int indfile;
	int indjob;
} opt_index;

void cmd_err();
int isnum(char* string);
int option_index_calc (int optind, int abflg, int pflg, int dflg, int fflg, int mflg);
void arg_dup_check(int* flags); 
char* strcombine(int count, char* strs[]); 
bool cmd_check (char* cmd, struct option options[]); 
void dryrun(FILE *file, int fflg, int pflg, int optind, int argc, char* argv[]); 
char* remove_NL (char* string); 

// SA_NOCLDSTOP signal so only checks if child dies not if child stops
// char** split_space_not_quote(char *input, int *numtokens);
// SIGINT sent to all processes in group
// KILL is only send to one so it can leave orphans

int main(int argc, char* argv[]) {

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
	int fflg=0; int abflg=0; int pflg=0; int mflg=0; int dflg=0;
	int errflg = 0;
	int argument_count = 0;
	int maxjobs;
	opt_index option_index = {.indabort = 0, .indpipe = 0,
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
				printf("args-file\n");
				printf("%s\n",optarg);
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
				printf("maxjobs\n");
				printf("%s\n",optarg);

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
			continue;
			}
		}
	}

	if (pflg && !(!errflg ^ !fflg)) {
		cmd_err();
	}

	//actual exectutions
	//dryrun
	if (dflg) {
		dryrun(inputFile, fflg, pflg, optind, argc, argv);
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
		char** cmd_array = calloc(argc-optind, sizeof(char*)); //might be unecessary
		
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

