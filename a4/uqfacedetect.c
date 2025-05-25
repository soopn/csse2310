#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/stat.h>
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
const char* const usageErrMsg = "Usage: ./uqfacedetect maxclients maxsize [portnum]\n";
const char* const imageWriteErrMsg = "uqfacedetect: unable to open the image file for writing\n";
const char* const cascadeErrMsg = "uqfacedetect: unable to load a cascade classifier\n";
const char* const portErrMsg = "uqfacedetect: cannot listen on given port \"%s\"\n";

const char* const emptyString = "";
const char* const tempFileDir = "/tmp/imagefile.jpg";
const char* const faceCascadeFileDir = "/local/courses/csse2310/resources/a4/haarcascade_frontalface_alt2.xml";
const char* const eyeCascadeFileDir = "/local/courses/csse2310/resources/a4/haarcascade_eye_tree_eyeglasses.xml";
const char* const responseFileDir = "/local/courses/csse2310/resources/a4/responsefile";

const int MAX_CLIENTS = 10000; // might be uint32_t
const int NUM_STATISTICS = 5; 
const uint32_t MSG_PREFIX = 0x23107231;
const uint32_t MAX_SIZE = (1UL << 32) - 1; 
const uint32_t SERVER_HEADER_SIZE = sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint32_t);

// OPEN CV PARAMETERS
const float haarScaleFactor = 1.1;
const int haarMinNeighbours = 4;
const int haarFlags = 0;
const int haarMinSize = 0;
const int haarMaxSize = 1000;
const int ellipseStartAngle = 0;
const int ellipseEndAngle = 360;
const int lineThickness = 4;
const int lineType = 8;
const int shift = 0;
const int bgraChannels = 4;
const int alphaIndex = 3;

//---------------------------------------------------------------------------//

							/* STRUCTS & ENUMS */
//---------------------------------------------------------------------------//
typedef enum {
	EXIT_USAGE = 6,
	EXIT_IMAGE_WRITE = 20,
	EXIT_CASCADE = 11,
	EXIT_PORT = 1
} ExitStatus;

typedef enum {
	FACE_DETECT = 0,
	FACE_REPLACE = 1,
	OUTPUT_IMAGE = 2,
	ERROR_MESSAGE = 3
} OperationType;

typedef enum {
	INVALID_MESSAGE = 0,
	INVALID_OPERATION = 1,
	IMAGE_ZERO_BYTES = 2,
	IMAGE_TOO_LARGE = 3,
	IMAGE_LOAD_ERROR = 4,
	IMAGE_NO_FACE = 5
} ErrorMessageCodes;

typedef struct {
	uint32_t prefix;
	uint8_t operation;
	uint32_t detectImgSize;
	uint8_t* detectImgContent;
	uint32_t replaceImgSize;
	uint8_t* replaceImgContent;
} Message;

typedef struct {
	int maxClients;
	uint32_t maxSize;
	char* port;
} Arguments;

typedef struct {
	FILE* outputFile;
	CvHaarClassifierCascade* faceCascade;
	CvHaarClassifierCascade* eyeCascade;
} CascadeStruct;

typedef struct {
	IplImage* frame;
	IplImage* frameGray;
	IplImage* replace;
	CvMemStorage* storage;
	CvSeq* faces;	
	bool error;
} OpenCVStruct;

typedef struct { // TODO: might not be all there is
	int socket;
	CascadeStruct* cascade;
	OpenCVStruct openCVParameters; 
	sem_t* lock;
} ClientStruct;

typedef struct {
	uint32_t connections;
	uint32_t completed;
	uint32_t detectionRequests;
	uint32_t replaceRequests;
	uint32_t malformed;
} Statistics;

const char* const errorMessageList[] = {
	"invalid message",
	"invalid operation type",
	"image is 0 bytes",
	"image too large",
	"invalid image",
	"no faces detected in image" 
};

const char* const statisticsList[] = {
	"Connected clients: ",
	"Clients completed: ",
	"Face detection requests: ",
	"Face replace requests: ",
	"Malformed requests: "
};

//---------------------------------------------------------------------------//

void exit_usage_error(void);
void exit_image_error(void);
void exit_cascade_error(void);
void exit_port(char* port);
bool is_num(char* inputString);
bool valid_max_clients(char* input);
bool valid_max_size(char* input);
bool valid_port(char* input);
void has_empty_string(int argc, char** argv);
Message* init_message(void);
Arguments* init_arguments(void);
Arguments* argument_check(int argc, char** argv);
void clean(Arguments* args);
void tmp_file_check(Arguments* args);
void load_temp_file (uint8_t* fileBuf, long fileSize); // needs a semaphone
CascadeStruct* init_cascade_struct(Arguments* args);
int open_listen_connection(Arguments* args);
uint32_t get_file_size(FILE* file);
ssize_t write_from_memory(int socket, const uint8_t* memory, size_t length);
void send_response_file (int socket);
void send_error_message(int socket, ErrorMessageCodes errorCode);
Message* read_message(int socket);
Message* format_message(OperationType operation, uint32_t length, uint8_t* content);
OpenCVStruct init_OpenCV_struct();
OpenCVStruct load_detect_image(CascadeStruct* cascadeParam);
OpenCVStruct load_replace_image(CascadeStruct* cascadeParam, OpenCVStruct image);
void cv_detect_faces(CascadeStruct* cascadeParam, OpenCVStruct image);
void cv_detect_and_replace_faces(CascadeStruct* cascadeParam, OpenCVStruct image);
void* client_handler(void* c);
void server_runtime (int fdServer, Arguments* serverArgs);

//---------------------------------------------------------------------------//
// TODO: redirect all stdout and stderr (EXCEPT LISTENING PORT NUM AND ERROR MSGS) to /dev/null
// 		 rename enums
// 		 malloc with sizeof instead of just raw length for portability
// 		 protect important things with semaphone:
// 		 		* tempfile
// 		 		* statistics struct
int main(int argc, char** argv) {
	Arguments* args = argument_check(argc, argv);
	tmp_file_check(args);
	CascadeStruct* cascadeParameters = init_cascade_struct(args);
	int serverFD = open_listen_connection(args);
	sem_t lock;
	return 0;
}

								/* OTHER FUNCTIONS*/
//-----------------------------------------------------------------------------//
void exit_usage_error(void) {
	fprintf(stderr, usageErrMsg);
	exit(EXIT_USAGE);
}

void exit_image_error(void) {
	fprintf(stderr, imageWriteErrMsg);
	exit(EXIT_IMAGE_WRITE);
}

void exit_cascade_error(void) {
	fprintf(stderr, cascadeErrMsg);
	exit(EXIT_CASCADE);
}

void exit_port(char* port) { //might not be int
	fprintf(stderr, portErrMsg, port);
	exit(EXIT_PORT);
}

bool is_num(char* inputString) {
	for (int i = 0 ; i < (int)strlen(inputString) ; i++) {
		if (!isdigit(inputString[i])) {
			return false;
		}
	}
	return true;
}

bool valid_max_clients(char* input) { 
	if (!is_num(input)) return false;
	int buffer = atoi(input);
	if (buffer > MAX_CLIENTS || buffer < 0) {
		return false;
	}
	return true;
}

// TODO: this part i guess
// 		 use strtoul REF: found from atol
// 		 magic number
bool valid_max_size(char* input) {
	long buffer = strtol(input, NULL, 32); //magic number
	if (buffer > MAX_SIZE || buffer < 0) return false;
	return true;	
}

void has_empty_string(int argc, char** argv) {
	argv++; // remove program name
	argc--;
	for (int i = 0; i < argc ; i++) {
		if (!strcmp(argv[i], emptyString)) {
			exit_usage_error();
		}
	}
	return;
}

Message* init_message(void) {
	Message* message = (Message*)malloc(sizeof(Message));
	message->prefix = MSG_PREFIX;
	message->operation = 3; // 0 is a valid operation 3 for error
	message->detectImgSize = 0;
	message->detectImgContent = NULL;
	message->replaceImgSize = 0;
	message->replaceImgContent = NULL;
	return message;
}
 
Arguments* init_arguments(void) {
	Arguments* args = (Arguments*)malloc(sizeof(Arguments));
	args->maxClients = 0;
	args->maxSize = 0;
	args->port = "0"; // default ephemeral
	return args;
}

// TODO: maxsize checking
Arguments* argument_check(int argc, char** argv) {
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
	args->maxSize = strtoul(argv[1], NULL, 32); // 32 bit unsigned long?
	if (argv[2] != NULL) {
		args->port = strdup(argv[2]); // malloc'd
	}
	return args;
}
void clean(Arguments* args) {
	free((char*)args->port);
	free((Arguments*)args);
}

// just to check, each thread will open the file themselves later to adhere to mutex
void tmp_file_check(Arguments* args) {
	FILE* tmp = fopen(tempFileDir,"w");
	if (!tmp) {
		clean(args);
		exit(EXIT_IMAGE_WRITE);
	}
	fclose(tmp); // check if can be closed?
}
	
CascadeStruct* init_cascade_struct(Arguments* args) {
	CascadeStruct* param = (CascadeStruct*)calloc(1, sizeof(CascadeStruct));
	param->outputFile = NULL;
	param->faceCascade = (CvHaarClassifierCascade*)cvLoad(faceCascadeFileDir, NULL, NULL, NULL);
	param->eyeCascade = (CvHaarClassifierCascade*)cvLoad(eyeCascadeFileDir, NULL, NULL, NULL);
	if (!param->faceCascade || !param->eyeCascade) {
		clean(args);
		exit_cascade_error();
	}
	return param;
}

// TODO: remove debugging
// 		 add mutex to limit maximum clients
int open_listen_connection(Arguments* args) {
    struct addrinfo* ai = 0;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // listen on all IP addresses

    int err; //debugging
    if ((err = getaddrinfo(NULL, args->port, &hints, &ai))) {
        freeaddrinfo(ai);
		fprintf(stderr, portErrMsg, args->port);
		clean(args);
        exit(EXIT_PORT); // Could not determine address
    }

    // Create a socket
    int listenFD = socket(AF_INET, SOCK_STREAM, 0); // 0=default protocol (TCP)
    if (listenFD < 0) { // error in listening
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
    if (bind(listenFD, (struct sockaddr*)ai->ai_addr, sizeof(struct sockaddr)) < 0) { // not sure about the casting
		close(listenFD);
        freeaddrinfo(ai);
		fprintf(stderr, portErrMsg, args->port);
		clean(args);
        exit(EXIT_PORT); // Could not determine address
    }
	
	// Get portnum
	struct sockaddr_in ad;
	socklen_t len = sizeof(struct sockaddr_in);
	memset(&ad, 0, sizeof(struct sockaddr_in));
	if (getsockname(listenFD, (struct sockaddr*)&ad, &len)) {
		perror("sockname"); // debug
		exit(4);
	}
	fprintf(stderr, "%d\n", ntohs(ad.sin_port));
	fflush(stderr);
	fflush(stdout);
	return listenFD;
}

// assumes file is already opened
uint32_t get_file_size(FILE* file) { // REF: fseek man page
	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	rewind(file);
	return (uint32_t)size;
}

ssize_t write_from_memory(int socket, const uint8_t* memory, size_t length) {
	size_t total = 0;

	while (total < length) {
		ssize_t written = write(socket, memory + total, length - total);
		if (written < 0) {
			printf("WRITE ERROR\n");
			break;
		}
		if (written == 0) {
			printf("UNEXPECTED EOF OR SIGPIPE\n");
			break;
		}
		total += written;
	}
	return total;
}

/* writes temp file contents (from buffer) into tempfile
 * TODO: protect with semaphone
 */
void load_temp_file(uint8_t* fileBuf, uint32_t fileSize) {
	// semaphone thingy here
	
	int tempFile = open(tempFileDir, O_WRONLY | O_TRUNC);
	write_from_memory(tempFile, fileBuf, fileSize);

	free((uint8_t*) fileBuf);

	// release semaphone
	close(tempFile);
}

/* After this go back to sending error messages 
 * Server can only send 1 content
 * */
Message* format_message(OperationType operation, uint32_t length, uint8_t* content) {
	Message* message = init_message(); 
	message->prefix = htonl(MSG_PREFIX);
	message->operation = operation;
	message->detectImgSize = length;
	message->detectImgContent = (uint8_t*)malloc(length * sizeof(uint8_t));
	memcpy(message->detectImgContent, content, length);
	return message;	
}

void write_message(int socket, Message* message) {
	uint32_t messageSize = SERVER_HEADER_SIZE + sizeof(uint8_t) * message->detectImgSize;
	size_t offset = 0;
	uint8_t* messageBuffer = (uint8_t*)malloc(messageSize);
	
	// populating message buffer
	memcpy(messageBuffer, &(message->prefix), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(messageBuffer + offset, &(message->operation), sizeof(uint8_t));
	offset += sizeof(uint8_t);
	memcpy(messageBuffer + offset, &(message->detectImgSize), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(messageBuffer + offset, message->detectImgContent, message->detectImgSize);

	write(socket, messageBuffer, messageSize);
}
 
// IF FIRST 4 BYTES DONT WORK, SEND responsefile over socket as is, no 
void send_response_file (int socket) {
	// read and store response file
	FILE* responseFile = fopen(responseFileDir, "r");
	long responseFileSize = get_file_size(responseFile);
	uint8_t* responseFileContents = (uint8_t*)malloc(responseFileSize);
	fread(responseFileContents, sizeof(uint8_t), responseFileSize, responseFile);

	// sends file contents without following communication protocol
	write_from_memory(socket, responseFileContents, responseFileSize);
	fclose(responseFile);
	free((uint8_t*)responseFileContents);
}

// TODO: format error message to follow protocol:
// 		 prefix; operation type; err msg size; err msg
void send_error_message(int socket, ErrorMessageCodes errorCode) {
	size_t errorMessageLength = strlen(errorMessageList[errorCode]);
	
	Message* errorMessage = format_message(ERROR_MESSAGE, errorMessageLength, (uint8_t*)errorMessageList[errorCode]);
	write_message(socket, errorMessage);
}

bool message_check(int socket, ssize_t numRead, uint32_t length) {
	if (length > MAX_SIZE) {
		send_error_message(socket, IMAGE_TOO_LARGE);
		return true;
	}
	if (numRead < length || numRead < 0) {
		send_error_message(socket, INVALID_MESSAGE); // unexpected EOF, size was wrong
		return true;
	}
	if (numRead == 0) {
		send_error_message(socket, IMAGE_ZERO_BYTES);
		return true;
	}
	return false;
}

bool prefix_check(int socket) {
	uint32_t* prefixBuffer = (uint32_t*)malloc(sizeof(uint32_t));
	read(socket, prefixBuffer, sizeof(uint32_t));
	if (*prefixBuffer != MSG_PREFIX) { // incorrect prefix
		free((uint32_t*)prefixBuffer);
		return false;
	}
	free((uint32_t*)prefixBuffer);
	return true;
}

/* Reads message from socket
 * clients can only send 0, or 1 operation types
 * stores as message struct and returns the struct to be processed later
 * passes to OpenCV
 * 		if unexpected EOF or 
 *
 * writes input contents into tempfile
 * loads image into OpenCV
 * writes replace image into tempfile
 * loads replace image into OpenCV
 * writes ouput into tempfile
 * writes tempfile to 
 */
Message* read_message(int socket, CascadeStruct* cascadeParam) {
	Message* serverMessage = init_message();

	// read prefix first
 	if (!prefix_check(socket)) {
		send_response_file(socket);
		serverMessage->prefix = 0; //TODO:
		return serverMessage;
	}

	// read operation
	uint8_t* opBuffer = (uint8_t*)malloc(sizeof(uint8_t));
	read(socket, opBuffer, sizeof(uint8_t));
	serverMessage->operation = *opBuffer;

	// reads detect img data
	read(socket, &(serverMessage->detectImgSize), sizeof(uint32_t));
	uint8_t* imageData = (uint8_t*)malloc(serverMessage->detectImgSize);
	size_t numRead = read(socket, imageData, serverMessage->detectImgSize);
	if (message_check(socket, numRead, serverMessage->detectImgSize)) {
		free((uint8_t*)opBuffer);
		free((uint8_t*)imageData);
		return serverMessage; // TODO: figure out what to do here supposed to 
							//		 clean and close client connection
	}
	
	// load detect image to temp file 
	load_temp_file(imageData, serverMessage->detectImgSize);
	OpenCVStruct detectImage = load_detect_image(cascadeParam);
	
	// replace face functionality
	if (*opBuffer == FACE_REPLACE) {
		read(socket, &(serverMessage->replaceImgSize), sizeof(uint32_t));
		uint8_t* replaceImageData = (uint8_t*)malloc(sizeof(uint8_t) * serverMessage->detectImgSize);
		size_t numRead = read(socket, replaceImageData, serverMessage->replaceImgSize);
		if (message_check(socket, numRead, serverMessage->detectImgSize)) {
			free((uint8_t*)opBuffer);
			free((uint8_t*)replaceImageData);
			return serverMessage; // TODO: figure out what to do here supposed to 
								//		 clean and close client connection
		}
		load_temp_file(replaceImageData, serverMessage->replaceImgSize);
		
	}
	else if (*opBuffer != FACE_DETECT) { // incorrect protocol
		send_error_message(socket, INVALID_OPERATION);
	}
	return serverMessage;
}

OpenCVStruct init_OpenCV_struct() {
	OpenCVStruct temp;
	temp.frame = NULL;
	temp.frameGray = NULL;
	temp.replace = NULL;
	temp.storage = NULL;
	temp.faces = NULL;
	temp.error = false;
	return temp;
}

OpenCVStruct load_detect_image(CascadeStruct* cascadeParam) {
	OpenCVStruct image = init_OpenCV_struct();
	image.frame = cvLoadImage(tempFileDir, CV_LOAD_IMAGE_COLOR);
	if (!image.frame) { // error loading; send error message and close connection
		//send_error_message(socket, IMAGE_LOAD_ERROR); // TODO: sending error message
		cvReleaseHaarClassifierCascade(&(cascadeParam->faceCascade)); // TODO: confirm this
		cvReleaseHaarClassifierCascade(&(cascadeParam->eyeCascade));
		image.error = true;
		return image;
	}
	// Grayscale and equalise image
	image.frameGray = cvCreateImage(cvGetSize(image.frame), IPL_DEPTH_8U, 1);
	cvCvtColor(image.frame, image.frameGray, CV_BGR2GRAY);
	cvEqualizeHist(image.frameGray, image.frameGray);
	image.storage = cvCreateMemStorage(0);
	cvClearMemStorage(image.storage);	
	// Detect Faces
	image.faces = cvHaarDetectObjects(image.frameGray, cascadeParam->faceCascade, image.storage,
									haarScaleFactor, haarMinNeighbours, haarFlags,
									cvSize(haarMinSize, haarMinSize), cvSize(haarMaxSize, haarMaxSize));
	return image;
}

OpenCVStruct load_replace_image(CascadeStruct* cascadeParam, OpenCVStruct image) {
	image.replace = cvLoadImage(tempFileDir, CV_LOAD_IMAGE_UNCHANGED);
	cvReleaseHaarClassifierCascade(&(cascadeParam->eyeCascade));
	if (!image.replace) {
		cvReleaseImage(&(image.frame));
		cvReleaseHaarClassifierCascade(&(cascadeParam->faceCascade)); // TODO: could release everything later together
		image.error = true;
		return image;
	}
}

// TODO: protect with mutex
// 		 can probably be shortened with some initialising function
// 		 check for no initialised faces
void cv_detect_faces(CascadeStruct* cascadeParam, OpenCVStruct image) {
	// iterate through each face and draw ellipses
	for (int i = 0; i < image.faces->total; i++) {
		CvRect* face = (CvRect*)cvGetSeqElem(image.faces, i);
		CvPoint center = {face->x + face->width / 2, face->y + face->height / 2};
		const CvScalar magenta = cvScalar(255, 0, 255, 0);
		const CvScalar blue = cvScalar(255, 0, 0, 0);
		cvEllipse(image.frame, center, cvSize(face->width / 2, face->height / 2), 0,
			ellipseStartAngle, ellipseEndAngle, magenta, lineThickness,
			lineType, shift);
		IplImage* faceROI = cvCreateImage(cvGetSize(image.frameGray), IPL_DEPTH_8U, 1);
		cvCopy(image.frameGray, faceROI, NULL);
		cvSetImageROI(faceROI, *face);
		// Create memory for calculations, allocate and clear it
		CvMemStorage* eyeStorage = 0;
		eyeStorage = cvCreateMemStorage(0);
		cvClearMemStorage(eyeStorage);
		// Detect eyes within each face
		CvSeq* eyes = cvHaarDetectObjects(faceROI, cascadeParam->eyeCascade, eyeStorage,
			haarScaleFactor, haarMinNeighbours, haarFlags,
			cvSize(haarMinSize, haarMinSize),
			cvSize(haarMaxSize, haarMaxSize));
		if (eyes->total == 2) {
			// Draw a circle around each eye
			for (int j = 0; j < eyes->total; j++) {
				CvRect* eye = (CvRect*)cvGetSeqElem(eyes, j);
				CvPoint eyeCenter = {face->x + eye->x + eye->width / 2,
					face->y + eye->y + eye->height / 2};
				int radius = cvRound((eye->width / 2 + eye->height / 2) / 2);
				cvCircle(image.frame, eyeCenter, radius, blue, lineThickness, lineType,
					shift);
				}
		}
		// Free memory
		cvReleaseImage(&faceROI);
		cvReleaseMemStorage(&eyeStorage);
	}
	cvSaveImage(tempFileDir, image.frame, 0); // free memory
	cvReleaseImage(&(image.frame));
	cvReleaseImage(&(image.frameGray));
	cvReleaseHaarClassifierCascade(&(cascadeParam->faceCascade));
	cvReleaseHaarClassifierCascade(&(cascadeParam->eyeCascade));
	cvReleaseMemStorage(&(image.storage));
}

void cv_detect_and_replace_faces(CascadeStruct* cascadeParam, OpenCVStruct image) {
	// Iterate through each detected face and replace it with an image
	for (int i = 0; i < image.faces->total; i++) {
		CvRect* face = (CvRect*)cvGetSeqElem(image.faces, i);
		IplImage* resized = cvCreateImage(cvSize(face->width, face->height),
				IPL_DEPTH_8U, image.replace->nChannels);
		// Resize the replacement image to be the size of the face
		cvResize(image.replace, resized, CV_INTER_AREA);
		char* frameData = image.frame->imageData;
		char* faceData = resized->imageData;
		// Iterate through each pixel in the image to replace
		// and draw over the original image
		for (int y = 0; y < face->height; y++) {
			for (int x = 0; x < face->width; x++) {
				int faceIndex
					= (resized->widthStep * y) + (x * resized->nChannels);
				// if BGRA, then look at the alpha channel
				if ((resized->nChannels == bgraChannels)
					&& (faceData[faceIndex + alphaIndex] == 0)) {
				// If alpha is 0 then skip this pixel
				continue;
				}
				// Frame is BGR which is 3 channels
				int frameIndex = (image.frame->widthStep * (face->y + y))
					+ ((face->x + x) * image.frame->nChannels);
				frameData[frameIndex + 0] = faceData[faceIndex + 0];
				frameData[frameIndex + 1] = faceData[faceIndex + 1];
				frameData[frameIndex + 2] = faceData[faceIndex + 2];
			}
		}
		 // Free memory
		cvReleaseImage(&resized);
	}
	// Save the processed image
	cvSaveImage(tempFileDir, image.frame, 0);
	// Free memory
	cvReleaseImage(&(image.frame));
	cvReleaseImage(&(image.replace));
	cvReleaseImage(&(image.frameGray));
	cvReleaseHaarClassifierCascade(&(cascadeParam->faceCascade));
	cvReleaseMemStorage(&(image.storage));
}

void* client_handler(void* c) {
	ClientStruct* info = (ClientStruct*)c;
}

/* BEHAVIOUR:
 *
 * spawn thread for each connection
 * ensure mutex of shared data structures, (CvHaarClassifierCascade)
 * if read() error or EOF from client, client handler must close connection, clean
 * 		and terminate
 * on SIGHUP print statistics to stderr
 */
void server_runtime (int fdServer, Arguments* serverArgs) {
	int connections = 0;
	int fd;
	struct sockaddr_in fromAddr;
	socklen_t fromAddrSize;
	int conditional = serverArgs->maxClients;

	while(conditional ? 1 : (connections < conditional)) {
        fromAddrSize = sizeof(struct sockaddr_in);
        // Block, waiting for a new connection. (fromAddr will be populated
        // with address of client)
        fd = accept(fdServer, (struct sockaddr*)&fromAddr, &fromAddrSize);
		connections++;
		// accepted connection
		int* fdData = (int*)malloc(sizeof(int));
		*fdData = fd;
		pthread_t threadID;
	}
}


