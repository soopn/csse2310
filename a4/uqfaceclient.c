#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <endian.h>

/* CONSTANTS */
//---------------------------------------------------------------------------//
const char* const usageErrMsg
        = "Usage: ./uqfaceclient portnum [--replacefilename filename] "
          "[--outputfilename filename] [--detectimage filename]\n";
const char* const fileReadErr
        = "uqfaceclient: unable to open the input file \"%s\" for reading\n";
const char* const fileWriteErr
        = "uqfaceclient: cannot open the output file \"%s\" for writing\n";
const char* const portErrMsg
        = "uqfaceclient: unable to connect to the server on port \"%s\"\n";
const char* const runtimeErrMsg
        = "uqfaceclient: received the following error message: \"%s\"\n";
const char* const communicationErr
        = "uqfaceclient: unexpected communication error\n";

const char* const replaceFileArg = "--replacefilename";
const char* const outputFileArg = "--outputfilename";
const char* const outputFileVariation = ">";
const char* const detectImgArg = "--detectimage";
const char* const detectImgVariation = "<"; // TODO THESE ARE REDIR NOT VARIATIONS

const char* const emptyString = "";
const uint32_t imgPrefix = 0x23107231;

//---------------------------------------------------------------------------//

/* STRUCTS */
//---------------------------------------------------------------------------//
typedef enum {
    FACE_DETECT = 0,
    FACE_REPLACE = 1,
    OUTPUT_IMAGE = 2,
    ERROR_MESSAGE = 3
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
    char* replaceFilename;
    FILE* replaceFile;
    char* outputFilename;
    FILE* outputFile;
    char* imgFilename;
    FILE* imgFile;
} Arguments;

typedef struct {
    uint32_t prefix;
    uint8_t operation;
    uint32_t detectImgSize;
    uint8_t* detectImgContent;
    uint32_t replaceImgSize;
    uint8_t* replaceImgContent;
} Message;

typedef struct {
	uint32_t size;
	uint8_t* data;
} Image;

//---------------------------------------------------------------------------//

// TODO: REMOVE THIS
void DEBUG_PRINT_MESSAGE(Message* message);

void usage_error(void);
void empty_input_file(char* filename);
void empty_output_file(char* filename);
void communication_error(void);
void has_empty_string(int argc, char** argv);
int increment_and_check(int iteration, int argc);
bool is_argument(char* string);
void duplicate_argument_check(Arguments* args, const char* const argument);
void has_portnum(char** argv);
Arguments* init_arguments(void);
Arguments* parse_command_line(int argc, char** argv);
Arguments* file_checking(Arguments* args);
int connect_socket(char* port, Arguments* args);
bool input_file_or_stdin(Arguments* args);
char* removeNewline(char* string);
Message* init_message(void);
long get_file_size(FILE* file);
int determine_operation(Arguments* args);
Message* format_message(Arguments* args);
ssize_t send_all(int socketFD, uint8_t* buffer, size_t len); // TODO remove this
void populate_message_buffer(Message* message, uint8_t* buffer);
int write_message(Message* message, int socketFD);
void read_message(int socket, Arguments* args);
void write_img_to_file(int socket, Message* response, Arguments* args);
char* byte_to_string(uint8_t* input, uint32_t length);
void print_error_message(int socket);
ssize_t send_with_header(int socketFD, uint8_t* buffer, size_t length);
void client_runtime(Arguments* args, int socketFD);

/* COMMAND LINE ARGUMENTS
 * USAGE: ./uqfaceclient portnum [--replacefilename filename] [--outputfilename
 * filename] [--detectimage filename] portnum must always be the first argument
 * cannot be empty
 * TODO:
 * unexpected argument checking (might not be necessary?)
 * replace everything with open() with O_CREAT | O_TRUNC, S_IRWXU
 * check what to do when file from stdin can't be opened (still gives an error
 * message from server)
 */

int main(int argc, char** argv)
{
    Arguments* programArgs = parse_command_line(argc, argv);
    int socket = connect_socket(argv[1], programArgs);
    client_runtime(programArgs, socket);
    return 0;
}

/* OTHER FUNCTIONS*/
//-----------------------------------------------------------------------------//

void usage_error(void)
{
    fprintf(stderr, usageErrMsg);
    exit(EXIT_USAGE);
}

void empty_input_file(char* filename)
{
    fprintf(stderr, fileReadErr, filename);
    exit(EXIT_FILE_READ);
}

void empty_output_file(char* filename)
{
    fprintf(stderr, fileWriteErr, filename);
    exit(EXIT_FILE_WRITE);
}

// TODO: include a free function for everything till this point
void communication_error(void)
{
    fprintf(stderr, communicationErr);
    exit(EXIT_COMMUNICATION);
}

void has_empty_string(int argc, char** argv)
{
    argv++; // remove program name
    argc--;
    for (int i = 0; i < argc; i++) {
        if (!strcmp(argv[i], emptyString)) {
            usage_error();
        }
    }
    return;
}

// increment the iteration count and checks if the current iteration exceeds
// total arguments
int increment_and_check(int iteration, int argc)
{
    iteration++;
    if (iteration >= argc) {
        usage_error();
    }
    return iteration;
}

bool is_argument(char* string)
{
    if (!strcmp(string, replaceFileArg)) {
        return true;
    }
    if (!strcmp(string, outputFileArg)
            || !strcmp(string, outputFileVariation)) {
        return true;
    }
    if (!strcmp(string, detectImgArg) || !strcmp(string, detectImgVariation)) {
        return true;
    }
    return false;
}

// check if given argument has already been initialised in args
void duplicate_argument_check(Arguments* args, const char* const argument)
{
    if (argument == replaceFileArg) {
        if (args->replaceFilename != NULL) {
            usage_error();
        }
    }
    if (argument == outputFileArg) {
        if (args->outputFilename != NULL) {
            usage_error();
        }
    }
    if (argument == detectImgArg) {
        if (args->imgFilename != NULL) {
            usage_error();
        }
    }
}

void has_portnum(char** argv)
{
    if (is_argument(argv[1])) {
        usage_error();
    }
    return;
}

Arguments* init_arguments(void)
{ // sets all pointers to NULL
    Arguments* args = (Arguments*)malloc(sizeof(Arguments));
    args->replaceFilename = NULL;
    args->replaceFile = NULL;
    args->outputFilename = NULL;
    args->outputFile = stdout;
    args->imgFilename = NULL;
    args->imgFile = NULL;
    return args;
}

Message* init_message(void)
{
    Message* message = (Message*)malloc(sizeof(Message));
    message->prefix = 0;
    message->operation = ERROR_MESSAGE; // 0 is a valid operation 3 for error
    message->detectImgSize = 0;
    message->detectImgContent = NULL;
    message->replaceImgSize = 0;
    message->replaceImgContent = NULL;
    return message;
}

Arguments* parse_command_line(int argc, char** argv)
{
    if (argc == 1) {
        usage_error();
    }
    has_portnum(argv);
    has_empty_string(argc, argv); // check for empty string
    argv += 2; // remove program name and portnum
    argc -= 2;
    Arguments* args = init_arguments(); // malloc'd

    for (int i = 0; i < argc; i++) {
        if (!strcmp(argv[i], replaceFileArg)) {
            i = increment_and_check(i, argc);
            if (!is_argument(argv[i])) {
                duplicate_argument_check(args, replaceFileArg);
                args->replaceFilename = strdup(argv[i]);
                continue;
            }
        }
        if (!strcmp(argv[i], outputFileArg)
                || !strcmp(argv[i], outputFileVariation)) {
            i = increment_and_check(i, argc);
            if (!is_argument(argv[i])) {
                duplicate_argument_check(args, outputFileArg);
                args->outputFilename = strdup(argv[i]);
                continue;
            }
        }
        if (!strcmp(argv[i], detectImgArg)
                || !strcmp(argv[i], detectImgVariation)) {
            i = increment_and_check(i, argc);
            if (!is_argument(argv[i])) {
                duplicate_argument_check(args, detectImgArg);
                args->imgFilename = strdup(argv[i]);
                continue;
            }
        } else {
            usage_error();
        }
    }
    args = file_checking(args);
    return args;
}

// true if file present, false otherwise
bool input_file_or_stdin(Arguments* args)
{
    if (args->imgFilename) {
        return true;
	}
    return false;
}

// TODO: change to open()
// 		 write file with "rw" access
// 		 add closing of other opened files if they passed
Arguments* file_checking(Arguments* args)
{
    if (args->imgFilename != NULL) {
        args->imgFile = fopen(args->imgFilename, "r");
        if (!args->imgFile) {
            empty_input_file(args->imgFilename);
        }
    }
    if (args->replaceFilename != NULL) {
        args->replaceFile = fopen(args->replaceFilename, "r");
        if (!args->replaceFile) {
            empty_input_file(args->replaceFilename);
        }
    }
    if (args->outputFilename != NULL) {
        args->outputFile = fopen(args->outputFilename,
                "w"); // should be open (truncate if exist, write if not)
        if (!args->outputFile) {
            empty_output_file(args->outputFilename);
        }
    }
    return args;
}

// TODO: remove debug msg
int connect_socket(char* port, Arguments* args)
{
    struct addrinfo* ai = 0;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP
	int err;
    if ((err=getaddrinfo(NULL, port, &hints, &ai))) {
        freeaddrinfo(ai);
        fprintf(stderr, "%s\n", gai_strerror(err));
        return 1;   // could not work out the address
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

    return fd;
}

char* removeNewline(char* string)
{
    char* buffer = strdup(string);
    for (int i = 0; i < (int)strlen(string); i++) {
        if (buffer[i] == '\n') {
            buffer[i] = '\0';
            buffer = (char*)realloc(buffer, (i + 1) * sizeof(char));
            break;
        }
    }
    return buffer;
}

long get_file_size(FILE* file)
{ // REF: fseek man page
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);
    return size;
}

/* WELL BEHAVED CLIENT ONLY SENDS:
 * 0 for face detection request
 * 1 for face replacement request
 * set 3 else I guess
 */
int determine_operation(Arguments* args)
{
    if (args->imgFilename != NULL) {
        if (args->replaceFilename != NULL) {
            return FACE_REPLACE; // face replace
        } else {
            return FACE_DETECT; // face detect
        }
    }
    return ERROR_MESSAGE;
}

Message* format_message(Arguments* args)
{
    Message* message = init_message();

    message->prefix = imgPrefix;
    // printf("Prefix: %d\n", message->prefix); // debug
    message->operation = determine_operation(args);
    // printf("Operation: %d\n", message->operation); // debug
    message->detectImgSize = get_file_size(args->imgFile);
    if (!message->detectImgSize) { // error in image size
        message->operation = ERROR_MESSAGE;
    }
    message->detectImgContent = (uint8_t*)malloc(message->detectImgSize);

    // copy file contents
    fread(message->detectImgContent, sizeof(uint8_t), message->detectImgSize,
            args->imgFile); // might have to read till not EOF

    if (message->operation == FACE_REPLACE) {
        message->replaceImgSize = get_file_size(args->replaceFile);
        if (!message->replaceImgSize) { // error in image size
            message->operation = ERROR_MESSAGE; // TODO just send file as is with 0 bytes
        }
        message->replaceImgContent = (uint8_t*)malloc(message->detectImgSize);

        // copy file contents
        fread(message->replaceImgContent, sizeof(uint8_t),
                message->detectImgSize, args->replaceFile);
    }
    return message;
}

ssize_t send_with_header(int socketFD, uint8_t* buffer, size_t length)
{ // TODO: remove the const and make the buffer const instead
    size_t total = 0;
    const uint8_t* pointer = buffer;

    while (total < length) {
        ssize_t written = write(socketFD, pointer + total, length - total);
        if (written < 0) {
            perror("SOMETHING WENT WRONG IN WRITING"); // debug
            return -1;
        }
        if (written == 0) {
            perror("UNEXPECTED EOF OR SIGPIPE"); // debug
            break;
        }
        total += written;
    }
    return total;
}

// TODO: cast from void*
void populate_message_buffer(Message* message, uint8_t* buffer)
{
    size_t offset = 0;
    memcpy(buffer + offset, &(message->prefix), sizeof(uint32_t));
    offset += sizeof(uint32_t);
    memcpy(buffer + offset, &(message->operation), sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy(buffer + offset, &(message->detectImgSize), sizeof(uint32_t));
    offset += sizeof(uint32_t);
    memcpy(buffer + offset, message->detectImgContent, message->detectImgSize);
    offset += message->detectImgSize;
    if (message->replaceImgContent) {
		memcpy(buffer + offset, &(message->replaceImgSize), sizeof(uint32_t));
		offset += sizeof(uint32_t);
        memcpy(buffer + offset, message->replaceImgContent, message->replaceImgSize);
    }
    free((uint8_t*)message->detectImgContent);
    free((uint8_t*)message->replaceImgContent);
}

int write_message(Message* message, int socketFD)
{
    uint8_t* buffer = NULL;
    size_t messageSize = 0;
    if (message->operation == 0) { // no replace image
        messageSize
                = sizeof(Message) + message->detectImgSize - sizeof(uint32_t);
        buffer = (uint8_t*)malloc(messageSize);
    } else {
        messageSize = sizeof(Message) + message->detectImgSize
                + message->replaceImgSize;
        buffer = (uint8_t*)malloc(messageSize);
    }
    populate_message_buffer(message, buffer);
    int result = send_with_header(socketFD, buffer, messageSize);
    return result;
}

// TODO: maybe check for valid size
void write_img_to_file(int socket, Message* response, Arguments* args)
{
    // get file size
    FILE* stream = args->outputFile;
    uint32_t* size = (uint32_t*)malloc(sizeof(uint32_t));
    read(socket, size, sizeof(uint32_t));

    // write to output stream
    uint8_t* imageData = (uint8_t*)malloc(*size);
    read(socket, imageData, *size);

    size_t total = 0;
    const uint8_t* pointer = imageData;
    while (total < *size) {
        ssize_t written = fwrite(
                pointer + total, sizeof(uint8_t), (size_t)*size, stream);
        if (written < 0) {
            perror("WRITE TO FILE ERROR\n"); // debug
        }
        if (written == 0) {
            perror("UNEXPECTED EOF OR SIGPIPE"); // debug
            break;
        }
        total += written;
    }
    free((uint32_t*)size);
}

char* byte_to_string(uint8_t* input, uint32_t length)
{
    char* string = (char*)malloc(length + 1);
    memcpy(string, input, length);
    string[length] = '\0';
    return string;
}

void print_error_message(int socket)
{
    // error msg size
    uint32_t* size = (uint32_t*)malloc(sizeof(uint32_t));
    read(socket, size, sizeof(uint32_t));

    // get message as byte stream
    uint8_t* inputStream = (uint8_t*)malloc(sizeof(uint8_t));
    read(socket, inputStream, sizeof(uint8_t));

    // print to stderr
    char* errMsg = byte_to_string(inputStream, *size);
    fprintf(stderr, runtimeErrMsg, errMsg);
    free((char*)errMsg);
    free((uint32_t*)size);
    free((uint8_t*)inputStream);
}

// TODO: remove this
void DEBUG_PRINT_MESSAGE(Message* message)
{
    printf("Prefix: %X\n", message->prefix);
    printf("Operation: %d\n", message->operation);
    printf("Image Size: %d\n", message->detectImgSize);
    if (message->detectImgContent) {
        printf("Image Exists\n");
    } else {
        printf("Image doesn't exist\n");
    }
    if (message->replaceImgSize) {
        printf("Image2 Size: %d\n", message->replaceImgSize);
    } else {
        printf("Image2 doesn't exist\n");
    }
    if (message->replaceImgContent) {
        printf("Image Exists\n");
    } else {
        printf("Image2 doesn't exist\n");
    }
}

/* read from socket
 *
 * CORRECT FORMAT ==> operation == 2; prefix correct; img size, img content
 * 				  ==> operation == 3; prefix correct; err size,
 * err content WRONG FORMAT ==> prefix wrong then print stderr communicationErr;
 * exit 1
 * TODO: free memory
 */
void read_message(int socket, Arguments* args)
{
    Message* serverMessage = init_message();

    // read prefix first
    uint32_t* prefixBuffer = (uint32_t*)malloc(sizeof(uint32_t));
    read(socket, prefixBuffer, sizeof(uint32_t));
    if (*prefixBuffer != imgPrefix) {
        free((uint32_t*)prefixBuffer);
        communication_error();
    }
    free((uint32_t*)prefixBuffer);

    // read operation
    uint8_t* opBuffer = (uint8_t*)malloc(sizeof(uint8_t));
    read(socket, opBuffer, sizeof(uint8_t));
    if (*opBuffer == OUTPUT_IMAGE) {
        write_img_to_file(socket, serverMessage, args);
    }
    if (*opBuffer == ERROR_MESSAGE) {
        print_error_message(socket);
    }
    free((uint8_t*)opBuffer);
}

Image read_from_stdin(void) {
	bool start = true;
	int offset = 0;
	Image img;
	img.size = 0;
	img.data = (uint8_t*)malloc(sizeof(uint8_t));
	uint8_t* buffer = (uint8_t*)malloc(sizeof(uint8_t));

	while ((read(STDIN_FILENO, buffer, sizeof(uint8_t)) > 0)) {
		img.size++;
		img.data = (uint8_t*)realloc(img.data, img.size * sizeof(uint8_t));
		memcpy(img.data + offset, buffer, sizeof(uint8_t));
		offset += sizeof(uint8_t);
	}
	if (img.size == 0) { // stdin is 0 bytes or EOF
		free((uint8_t*)img.data);
		img.data = NULL;
		return img;
	}
	free((uint8_t*)buffer);
	return img;
}

/* ORDER OF BYTESTREAM
 * 4 byte: imgPrefix		(uint32_t)
 * 1 byte: OperationType 	(uint8_t)
 * 4 byte: imgsize1			(uint32_t)
 * M byte: img1				(???)
 * 4 byte: imgsize2			(uint32_t)
 * N byte: img2				(???)
 *
 * SPEC:
 * TODO: get byte stream from stdin
 *
 * send send image with header
 * await response
 * write to output file or stdout
 */
void client_runtime(Arguments* args, int socketFD)
{ 
	if (!input_file_or_stdin(args)) { // read from stdin
		Image detectImage = read_from_stdin();
		Message* message = init_message();
		message->prefix = imgPrefix; 
		message->operation = FACE_DETECT;
		message->detectImgSize = detectImage.size; //free
		message->detectImgContent = detectImage.data; //free
		if (args->replaceFile) { // formats message
			message->operation = FACE_REPLACE;
			message->replaceImgSize = get_file_size(args->replaceFile);
			message->replaceImgContent = (uint8_t*)malloc(message->replaceImgSize);
			fread(message->replaceImgContent, sizeof(uint8_t), message->replaceImgSize, args->replaceFile);
		}
		//DEBUG_PRINT_MESSAGE(message);
		int result = write_message(message, socketFD); // TODO free after this
	}
	else { // read from file
		Message* message = format_message(args);
		DEBUG_PRINT_MESSAGE(message);
		int result = write_message(message, socketFD);
	}
	read_message(socketFD, args);
}
