#include <stdio.h>
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
//char** cmd_array (char* argv[], int argc, struct option options[]); 


int main(int argc, char* argv[]) {

	//cmd parsing
	int c;
	int optind = -1;
	opterr = 0;
	int fflg=0; int abflg=0; int pflg=0; int mflg=0; int dflg=0;
	int errflg = 0;
	int argument_count = 0;

	static struct option options[] = {
		{"abort-on-error", 	no_argument, 		0,	'a'},
		{"args-file", 		required_argument, 	0,	'f'},
		{"dryrun", 			no_argument, 		0,	'd'},
		{"pipe", 			no_argument, 		0,	'p'},
		{"maxjobs", 		required_argument, 	0,	'm'},
		{0, 				0, 					0, 	0}
	};

	while ((c = getopt_long(argc, argv, "dpa:m:f:", options, NULL)) != -1) { //reference	https://www.man7.org/linux/man-pages/man3/getopt.3.html 
		switch (c) {
			case 'a':	
				abflg++;
				printf("abort-on-error\n");
				break;
			case 'f':
				fflg++;
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
				if (isnum(optarg) < 1 || isnum(optarg) > 130) { // 1 < n <= 130
					cmd_err();
				}
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

	optind = option_index_calc(optind, abflg, pflg, dflg, fflg, mflg);

	while (argv[optind] != NULL) {
		printf("%s\n", argv[optind++]);
	}

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

	//if (argc == 1) for ./uqparallel case
	return 0;
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
/*
void dryrun(argv) {
	for (int i = 0 ; i < argc ; i++) {
		
	}
}
*/

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





	
