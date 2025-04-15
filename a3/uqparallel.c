#include <stdio.h>
#include <fcntl.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <csse2310a3.h>
#include <stdbool.h>
#include <getopt.h>
#include <ctype.h>

void cmd_err();
int isnum(char* string);
int option_index_calc (int optind, int abflg, int pflg, int dflg, int fflg, int mflg);
void arg_dup_check(int* flags); 
char* strcombine(int count, char* strs[]); 
bool cmd_check (char* cmd, struct option options[]); 

//char** split_space_not_quote(char *input, int *numtokens);

int main(int argc, char* argv[]) {
	


	/* Possible implementation

	 * parse with split_space_not_quote()
	 * use that new vector with getopt_long()
	 * if the first argument besides ./uqparallel not an argument then the rest is just fixed argument of the cmd
	 * except for :::

	 * need to account for SIGINT signal
	*/

	//cmd parsing
	int c;
	int optind = -1;
	opterr = 0;
	int fflg=0; int abflg=0; int pflg=0; int mflg=0; int dflg=0;
	int errflg = 0;
	int argument_count = 0;
	int maxjobs;

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
	
	/*
	if (!cmd_check(argv[1],options)) {
		char* inputstr = 
		*/ //do this later


	while ((c = getopt_long(argc, argv, "dpa:m:f:", options, NULL)) != -1) { //reference	https://www.man7.org/linux/man-pages/man3/getopt.3.html 
		switch (c) {
			case 'a':	
				abflg++;
				printf("abort-on-error\n");
				break;
			case 'f':
				fflg++;
				inputFile = fopen(optarg, "r"); //might have to replace with open() later on	
				if (!inputFile) {
					fprintf(stderr, "uqparallel: Cannot open file \"%s\" for reading\n", optarg);
					exit(2);
				}
				printf("args-file\n");
				printf("%s\n",optarg);
				break;

			case 'd':	//dryrun
				dflg++;

				break;

			case 'p':	//pipe
				pflg++;
				printf("pipe\n");
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
	int flg_array[] = {abflg, pflg, dflg, fflg, mflg};
	arg_dup_check(flg_array);

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
	/*
	if (dflg) {
		int print_count = 1;

		if (fflg) {
			char buffer[50];
			while (fgets(buffer, sizeof(buffer), inputFile) != NULL) {
				fprintf(stdout,"%d: %s\n",print_count,buffer);
				print_count++;
			}
		}
	}
	*/
	

	//if (argc == 1) for ./uqparallel case
	return 0;
}

//-------------------------------------------------------------------------------------------------------------------------//

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

bool cmd_check (char* cmd, struct option options[]) {
	for (int i = 0 ; i < 5 ; i++) { //number of possible options
		if (strstr(cmd,options[i].name)) {
			return true;
		}
	}
	return false;
}

void dryrun(FILE file, int fflg, int pflg) {
	int jobnum = 1;
	char buffer[50];
	
	if (fflg){
		while (fgets(buffer, sizeof(buffer), file) != NULL) {
			buffer = remove_NL(buffer);
			fprintf(stdout, "%d: %s", print_count, buffer);

			if (pflg) {
				fprintf(stdout, " |\n");
			}
			else {
				fprintf(stdout, "\n");
			}
		}
	}
	else {
		//need a way to get the arguments after --dryrun
		//maybe do a max of the optind? and put back &optind in the getopt_long()?
		//then
		//while (!feof(stdin))
		//fgets(buffer, sizeof(buffer), stdin) 
	}

}

char* remove_NL (char* string) {
	int len = (int)strlen(string);
	if (string[len-1] == "\n") {
		string[len-1] == NULL;
		return string;
	}
	else {
		return string;
	}
}
