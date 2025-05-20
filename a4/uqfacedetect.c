#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <opencv2/imgcodecs/imgcodecs_c.h>
#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/objdetect/objdetect_c.h>

								/* CONSTANTS */
//---------------------------------------------------------------------------//
const char* const usageErrMsg = "Usage: ./uqfacedetect maxclients maxsize [portNumber]\n";
const char* const imageWriteErrMsg = "uqfacedetect: unable to open the image file for writing\n";
const char* const cascadeErrMsg = "uqfacedetect: unable to load a cascade classifier\n";
const char* const portErrMsg = "uqfacedetect: cannot listen on given port \"%s\"\n";

const char* const emptyString = "";
const char* const tmpFileDir = "/tmp/imagefile.jpg";
const char* const faceCascadeFilename = "/local/courses/csse2310/resources/a4/haarcascade_frontalface_alt2.xml";
const char* const eyeCascadeFilename = "/local/courses/csse2310/resources/a4/haarcascade_eye_tree_eyeglasses.xml";

const int CONST_MAX_CLIENTS = 10000;
//TODO: const uint32_t maxSize = ??
//---------------------------------------------------------------------------//

								/* STRUCTS */
//---------------------------------------------------------------------------//
typedef enum {
	EXIT_USAGE = 6,
	EXIT_IMAGE_WRITE = 20,
	EXIT_CASCADE = 11,
	EXIT_PORT = 1
} ExitStatus;

typedef struct {
	int maxClients;
	uint32_t maxSize;
	char* port;
} Arguments;

typedef struct {
	FILE* outputFile;
	CvHaarClassifierCascade* faceCascade;
	CvHaarClassifierCascade* eyeCascade;
} OpenCVstruct;
//---------------------------------------------------------------------------//

void exit_usage_error ();
void exit_image_error ();
void exit_cascade_error ();
void exit_port (char* port);
bool is_num (char* inputString);
bool valid_max_clients (char* input);
bool valid_max_size (char* input);
bool valid_port (char* input);
void has_empty_string (int argc, char** argv);
Arguments* init_arguments ();
Arguments* argument_check (int argc, char** argv);
void clean (Arguments* args);
FILE* tmp_file_check (Arguments* args);
OpenCVstruct* init_cascade_struct (Arguments* args);
int open_listen_connection (Arguments* args);

//---------------------------------------------------------------------------//
int main (int argc, char** argv) {
	Arguments* args = argument_check(argc, argv);
	FILE* tmpFile = tmp_file_check(args);
	OpenCVstruct* OpenCVparameters = init_cascade_struct(args);
	int serverFD = open_listen_connection(args);
	return 0;
}

								/* OTHER FUNCTIONS*/
//-----------------------------------------------------------------------------//
void exit_usage_error () {
	fprintf(stderr, usageErrMsg);
	exit(EXIT_USAGE);
}

void exit_image_error () {
	fprintf(stderr, imageWriteErrMsg);
	exit(EXIT_IMAGE_WRITE);
}

void exit_cascade_error () {
	fprintf(stderr, cascadeErrMsg);
	exit(EXIT_CASCADE);
}

void exit_port (char* port) { //might not be int
	fprintf(stderr, portErrMsg, port);
	exit(EXIT_PORT);
}

bool is_num (char* inputString) {
	for (int i = 0 ; i < (int)strlen(inputString) ; i++) {
		if (!isdigit(inputString[i])) {
			return false;
		}
	}
	return true;
}

bool valid_max_clients (char* input) { 
	if (!is_num(input)) return false;
	int buffer = atoi(input);
	if (buffer > CONST_MAX_CLIENTS || buffer < 0) {
		return false;
	}
	return true;
}

// TODO: this part i guess
// 		 use strtoul REF: found from atol
bool valid_max_size (char* input) {
	return true;	
}

bool valid_port (char* input) {
	return true;	
}

void has_empty_string (int argc, char** argv) {
	argv++; // remove program name
	argc--;
	for (int i = 0; i < argc ; i++) {
		if (!strcmp(argv[i], emptyString)) {
			exit_usage_error();
		}
	}
	return;
}

Arguments* init_arguments () {
	Arguments* args = (Arguments*)malloc(sizeof(Arguments));
	args->maxClients = 0;
	args->maxSize = 0;
	args->port = NULL;
	return args;
}

// TODO: maxsize checking
Arguments* argument_check (int argc, char** argv) {
	has_empty_string(argc, argv);
	argc--; // remove program name
	argv++; // starts at max size
	Arguments* args = init_arguments(); //malloc'd

	if (argc < 2) exit_usage_error();
	if (!valid_max_clients(argv[0])) {
		free((Arguments*)args);
		exit_usage_error();
	}
	else {
		args->maxClients = atoi(argv[0]);
	}
	if (!valid_max_size(argv[1])) {
		free((Arguments*)args);
		exit_usage_error();
	}
	else {
		args->maxSize = strtoul(argv[1], NULL, 32); // 32 bit unsigned long?
	}
	if (argv[2] != NULL) {
		args->port = strdup(argv[2]); // malloc'd
	}
	return args;
}
void clean (Arguments* args) {
	free((char*)args->port);
	free((Arguments*)args);
}

FILE* tmp_file_check (Arguments* args) {
	FILE* tmp = fopen(tmpFileDir,"w");
	if (!tmp) {
		clean(args);
		exit(EXIT_IMAGE_WRITE);
	}
	return tmp;
}
	
OpenCVstruct* init_cascade_struct (Arguments* args) {
	OpenCVstruct* param = (OpenCVstruct*)calloc(1, sizeof(OpenCVstruct));
	param->outputFile = NULL;
	param->faceCascade = (CvHaarClassifierCascade*)cvLoad(faceCascadeFilename, NULL, NULL, NULL);
	param->eyeCascade = (CvHaarClassifierCascade*)cvLoad(eyeCascadeFilename, NULL, NULL, NULL);
	if (!param->faceCascade || !param->eyeCascade) {
		clean(args);
		exit_cascade_error();
	}
	return param;
}

// TODO: remove debugging
// 		 add mutex to limit maximum clients
int open_listen_connection (Arguments* args) {
    struct addrinfo* ai = 0;
    struct addrinfo hints;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // listen on all IP addresses

    int err; //debugging
    if ((err = getaddrinfo(NULL, args->port, &hints, &ai))) {
        freeaddrinfo(ai);
        fprintf(stderr, "%s\n", gai_strerror(err));
        exit(99); // Could not determine address
    }

    // Create a socket
    int listenfd = socket(AF_INET, SOCK_STREAM, 0); // 0=default protocol (TCP)
    if (listenfd < 0) { // error in listening
		clean(args);
		exit_port(args->port);
    }

	/* NOT SURE IF NECESSARY
    // Allow address (port number) to be reused immediately
    int optVal = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &optVal, sizeof(int))
            < 0) {
        perror("Error setting socket option");
        exit(1);
    }
	*/

    // Bind socket to address
    if (bind(listenfd, ai->ai_addr, sizeof(struct sockaddr)) < 0) {
        perror("Binding");
        exit(3);
    }

	return listenfd;
}
