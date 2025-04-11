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
	int fflag = 0;
	int errflag = 0;

	static struct option options[] = {
		{"abort-on-error", 	no_argument, 		0, 'a'},
		{"args-file", 		required_argument, 	0, 'f'},
		{"dryrun", 			no_argument, 		0, 'd'},
		{"pipe", 			no_argument, 		0, 'p'},
		{"maxjobs", 		required_argument, 	0, 'm'},
		{0, 				0, 					0, 0}
	};

	while ((c = getopt_long(argc, argv, "dpa:", options, &optind)) != -1) {
		switch (c) {
			case 'a':	
				printf("abort-on-error\n");
				break;
			case 'f':
				fflag++;
				printf("args-file\n");
				break;

			case 'd':	//dryrun
				printf("dryrun\n");
				break;

			case 'p':	//pipe
				printf("pipe\n");
				break;

			case 'm': 	//maxjobs
				printf("maxjobs\n");
				break;
				
			case '?':
				cmd_err();
				break;

			default: 
				cmd_err();
		}
	}
	argc--; //allow for just "./uqparallel"
	if (optind < argc) {
		for (int i = 0 ; i < argc ; i++) {
			if (strcmp(argv[i],"cmd") == 0) { //run cmd on next line
				printf("cmd");
				errflag++;
				continue;
			}
			if (strcmp(argv[i], ":::") == 0) {
				if (fflag != 0) {
					cmd_err();
				}
				else {
				printf(":::"); // run ::: per-flags on next line
				errflag++;
				continue;
				}
			}
		}
		if (errflag == 0) {
			cmd_err();
		}
	}
	return 0;
}

void cmd_err(){
	fprintf(stderr,"Usage: ./uqparallel [--dryrun] [--abort-on-error] [--maxjobs n] [--pipe] [--args-file argument-filename] [cmd [fixed-args ...]] [::: per-task-args ...]\n");
	exit(14);
}

