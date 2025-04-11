#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <csse2310a3.h>
#include <stdbool.h>
#include <getopt.h>

void cmd_err();

int main(int argc, char* argv[]) {

	//cmd parsing
	int c;
	int optind = 0;
	opterr = 0;
	int fflg=0; int abflg=0; int pflg=0; int mflg=0; int dflg=0;
	int errflg = 0;

	static struct option options[] = {
		{"abort-on-error", 	no_argument, 		0,	'a'},
		{"args-file", 		required_argument, 	0,	'f'},
		{"dryrun", 			no_argument, 		0,	'd'},
		{"pipe", 			no_argument, 		0,	'p'},
		{"maxjobs", 		required_argument, 	0,	'm'},
		{0, 				0, 					0, 	0}
	};

	while ((c = getopt_long(argc, argv, "dpa:m:f:", options, &optind)) != -1) {
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
				printf("dryrun\n");
				break;

			case 'p':	//pipe
				pflg++;
				printf("pipe\n");
				break;

			case 'm': 	//maxjobs
				mflg++;
				printf("maxjobs\n");
				printf("%s\n",optarg);
				break;
				
			case '?':
				cmd_err();
				break;

			default: 
				break;
		}
	}
	if (optind < 0) {
		for (int i = 0 ; i < argc ; i++) {
			if (strcmp(argv[i],"cmd") == 0) { //run cmd on next line
				printf("cmd");
				errflg++;
				continue;
			}
			if (strcmp(argv[i], ":::") == 0) {
				if (fflg != 0) {
					cmd_err();
				}
				else {
				printf(":::"); // run ::: per-flags on next line
				errflg++;
				continue;
				}
			}
		}
		if (errflg == 0) {
			cmd_err();
		}
	}
	//if (argc == 1) for ./uqparallel case
	return 0;
}

void cmd_err(){
	fprintf(stderr,"Usage: ./uqparallel [--dryrun] [--abort-on-error] [--maxjobs n] [--pipe] [--args-file argument-filename] [cmd [fixed-args ...]] [::: per-task-args ...]\n");
	exit(14);
}
/*
void dryrun(argv) {
	for (int i = 0 ; i < argc ; i++) {

	}
}
*/

