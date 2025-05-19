#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

							/* CONSTANTS */
//---------------------------------------------------------------------------//
// CONST STRINGS
const char* const usageErrMsg
        = "Usage: ./uqfaceclient portnum [--replacefilename filename] "
          "[--outputfilename filename] [--detectimage filename]\n";
const char* const fileReadErr = "uqfaceclient: unable to open the input file \"%s\" for reading\n";
const char* const fileWriteErr = "uqfaceclient: cannot open the output file \"%s\" for writing\n";
const char* const portErrMsg = "uqfaceclient: unable to connect to the server on port \"%d\"\n";

// CONST CMD LINE ARGUMENTS
const char* const replaceFileArg = "--replacefilename";
const char* const outputFileArg = "--outputfilename";
const char* const outputFileVariation = ">";
const char* const detectImgArg = "--detectimage";
const char* const detectImgVariation = "<";
//---------------------------------------------------------------------------//

							/* STRUCTS */
//---------------------------------------------------------------------------//
typedef enum {
	EXIT_USAGE = 8,
	EXIT_FILE_READ = 19,
	EXIT_FILE_WRITE = 18,
	EXIT_PORT = 2 
} ExitStatus;

typedef struct {
	char* replaceFileName;
	FILE* replaceFile;
	char* outputFileName;
	FILE* outputFile;
	char* imgFileName;
	FILE* imgFile;
} Arguments;
//---------------------------------------------------------------------------//

void usage_error();
bool is_argument (char* string);
void duplicate_argument_check (Arguments* args, const char* const argument);
void has_portnum (char** argv);
Arguments* init_arguments ();
Arguments* parse_command_line (int argc, char** argv);


/* COMMAND LINE ARGUMENTS
 * USAGE: ./uqfaceclient portnum [--replacefilename filename] [--outputfilename filename] [--detectimage filename]
 * portnum must always be the first argument cannot be empty
 * TODO:
 * unexpected argument checking
 */

int main(int argc, char** argv)
{
	Arguments* programArgs = parse_command_line(argc, argv);
    return 0;
}

								/* OTHER FUNCTIONS*/
//-----------------------------------------------------------------------------//

void has_empty_string (int argc, char** argv) {
	argv++; // remove program name
	argc--;
	for (int i = 0; i < argc ; i++) {
		if (!strcmp(argv[i], "")) {
			usage_error();
		}
	}
	return;
}

// increment the iteration count and checks if the current iteration exceeds total arguments
int increment_and_check (int iteration, int argc) {
	iteration++;
	if (iteration >= argc) {
		usage_error();
	}
	return iteration;
}

bool is_argument (char* string) {
	if (!strcmp(string, replaceFileArg)) {
		return true;
	}
	if (!strcmp(string, outputFileArg) || !strcmp(string, outputFileVariation)) {
		return true;
	}
	if (!strcmp(string, detectImgArg) || !strcmp(string, detectImgVariation)) {
		return true;
	}
	return false;
}
 
void usage_error() {
	fprintf(stderr, usageErrMsg);
	exit(EXIT_USAGE);
}

// check if given argument has already been initialised in args
void duplicate_argument_check (Arguments* args, const char* const argument) {
	if (argument == replaceFileArg) {
		if (args->replaceFileName != NULL) {
			usage_error();
		}
	}
	if (argument == outputFileArg) {
		if (args->outputFileName != NULL) {
			usage_error();
		}
	}
	if (argument == detectImgArg) {
		if (args->imgFileName != NULL) {
			usage_error();
		}
	}
}

void has_portnum (char** argv) {
	if (is_argument(argv[1])) usage_error();
	return;
}

Arguments* init_arguments () { // sets all pointers to NULL
	Arguments* args = (Arguments*)malloc(sizeof(Arguments));
	args->replaceFileName = NULL;
	args->replaceFile = NULL;
	args->outputFileName = NULL;
	args->outputFile = NULL;
	args->imgFileName = NULL;
	args->imgFile = NULL;
	return args;
}

Arguments* parse_command_line (int argc, char** argv) {
	if (argc == 1) usage_error(); 
	has_portnum(argv);
	has_empty_string(argc, argv); // check for empty string
	argv += 2; // remove program name and portnum
	argc -= 2;
	Arguments* args = init_arguments(); // malloc'd

	for (int i = 0 ; i < argc; i++) {
		if (!strcmp(argv[i], replaceFileArg)) {
			i = increment_and_check(i, argc);
			if (!is_argument(argv[i])) {
				duplicate_argument_check(args, replaceFileArg);
				args->replaceFileName = strdup(argv[i]);
				continue;
			}
		}
		if (!strcmp(argv[i], outputFileArg) || !strcmp(argv[i], outputFileVariation)) {
			i = increment_and_check(i, argc);
			if (!is_argument(argv[i])) {
				duplicate_argument_check(args, outputFileArg);
				args->outputFileName = strdup(argv[i]);
				continue;
			}
		}
		if (!strcmp(argv[i], detectImgArg) || !strcmp(argv[i], detectImgVariation)) {
			i = increment_and_check(i, argc);
			if (!is_argument(argv[i])) {
				duplicate_argument_check(args, detectImgArg);
				args->imgFileName = strdup(argv[i]);
				continue;
			}
		}
		else usage_error();
	}
	return args;
}
