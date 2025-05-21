#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

							/* CONSTANTS */
//---------------------------------------------------------------------------//
const char* const usageErrMsg
        = "Usage: ./uqfaceclient portnum [--replacefilename filename] "
          "[--outputfilename filename] [--detectimage filename]\n";
const char* const fileReadErr = "uqfaceclient: unable to open the input file \"%s\" for reading\n";
const char* const fileWriteErr = "uqfaceclient: cannot open the output file \"%s\" for writing\n";
const char* const portErrMsg = "uqfaceclient: unable to connect to the server on port \"%d\"\n";
const char* const runtimeErrMsg = "uqfaceclient: received the following error message: \"%s\"\n";
const char* const communicationErr = "uqfaceclient: unexpected communication error\n";

const char* const replaceFileArg = "--replacefilename";
const char* const outputFileArg = "--outputfilename";
const char* const outputFileVariation = ">";
const char* const detectImgArg = "--detectimage";
const char* const detectImgVariation = "<";

const char* const emptyString = "";
const uint32_t imgPrefix = 0x23107231;
//---------------------------------------------------------------------------//

							/* STRUCTS */
//---------------------------------------------------------------------------//
typedef enum {
	faceDetect = 0;
	faceReplace = 1;
	outputImage = 2;
	errorMessage = 3;
} OperationType;
 
typedef enum {
	EXIT_USAGE = 8,
	EXIT_FILE_READ = 19,
	EXIT_FILE_WRITE = 15,
	EXIT_PORT = 2,
	EXIT_RUNTIME = 20,
	EXIT_COMMUNICATION = 1
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
Arguments* file_checking (Arguments* args);
int connect_socket (char* port, Arguments* args);
bool input_file_or_stdin (Arguments* args);
char* removeNewline (char* string);


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

void usage_error() {
	fprintf(stderr, usageErrMsg);
	exit(EXIT_USAGE);
}

void empty_input_file (char* filename) {
	fprintf(stderr, fileReadErr, filename);
	exit(EXIT_FILE_READ);
}

void empty_output_file (char* filename) {
	fprintf(stderr, fileWriteErr, filename);
	exit(EXIT_FILE_WRITE);
}

void has_empty_string (int argc, char** argv) {
	argv++; // remove program name
	argc--;
	for (int i = 0; i < argc ; i++) {
		if (!strcmp(argv[i], emptyString)) {
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
	args = file_checking(args);
	return args;
}

// true if file present, false otherwise
bool input_file_or_stdin (Arguments* args) {
	if (args->imgFileName || args->replaceFileName) {
		return true;
	}
	else return false;
}

// TODO: change to open()
// 		 write file with "rw" access
// 		 add closing of other opened files if they passed
Arguments* file_checking (Arguments* args) {
	if (args->imgFileName != NULL) {
		args->imgFile = fopen(args->imgFileName, "r");
		if (!args->imgFile) {
			empty_input_file(args->imgFileName);
		}
	}
	if (args->replaceFileName != NULL) {
		args->replaceFile = fopen(args->replaceFileName, "r");
		if (!args->replaceFile) {
			empty_input_file(args->replaceFileName);
		}
	}
	if (args->outputFileName != NULL) { 
		args->outputFile = fopen(args->outputFileName, "w");// should be open (truncate if exist, write if not)
		if (!args->outputFile) {
			empty_output_file(args->replaceFileName);
		}
	}
	return args;
}

// TODO: remove debug msg
int connect_socket (char* port, Arguments* args) {
	struct addrinfo* ai = 0;
	struct addrinfo hints;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET; 		 // IPv4
	hints.ai_socktype = SOCK_STREAM; // TCP
	int err;
	if (err = getaddrinfo("localhost", port, &hints, &ai) != 0) {
		fprintf(stderr, "SOMETHING WRONG HAPPENED WITH ADDRESS");
		freeaddrinfo(ai);
		exit(99); //debug 
	}

	// TODO: get portnum to print err msg
	int fd = socket(AF_INET, SOCK_STREAM, 0); // default protocol
	if (connect(fd, ai->ai_addr, sizeof(struct sockaddr)) == -1) {
		fprintf(stderr, portErrMsg, port);
		freeaddrinfo(ai);
		free((Arguments*)args);
		exit(EXIT_PORT);
	}
	// connected
	  

	// random tasks to be done, return back with fds to do stuff with later on i guess?
	int fd2 = dup(fd);
	FILE* output = fdopen(fd, "w");
	FILE* input = fdopen(fd2, "r");

	fprintf(output, "CONNECTED!\n");
	fflush(output);
	fclose(output);
	return fd;
}

char* removeNewline (char* string) {
	char* buffer = strdup(string);
	for (int i = 0 ; i < (int)strlen(string) ; i++) {
		if (buffer[i] = '\n') {
			buffer[i] = '\0';
			buffer = (char*)realloc(buffer, (i+1) * sizeof(char));
			break;
		}
	}
	return buffer;
}

/* ORDER OF BYTESTREAM
 * 4 byte: imgPrefix		(uint32_t)
 * 1 byte: OperationType 	(uint8_t) 
 * 4 byte: imgsize1			(uint32_t) 
 * M byte: img1				(???)
 * 4 byte: imgsize2			(uint32_t)
 * N byte: img2				(???)
 */
void client_runtime (Arguments* args, int socketFD) {
	if (!input_file_or_stdin(args)) { // read from stdin
		char* buffer = NULL;
		size_t len = 0;
		ssize_t numLines;
		numLines = getline(&buffer, &len, stdin); // get file from stdin
		FILE* inputFile = fopen(removeNewline(buffer), "r");


		// doing something to send stuff over
		

		return 0;
	}

}



}
