#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <endian.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <opencv2/imgcodecs/imgcodecs_c.h>
#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/objdetect/objdetect_c.h>

/* CONSTANTS */
//---------------------------------------------------------------------------//
const char* const usageErrMsg
        = "Usage: ./uqfacedetect maxclients maxsize [portnum]\n";
const char* const imageWriteErrMsg
        = "uqfacedetect: unable to open the image file for writing\n";
const char* const cascadeErrMsg
        = "uqfacedetect: unable to load a cascade classifier\n";
const char* const portErrMsg
        = "uqfacedetect: cannot listen on given port \"%s\"\n";

const char* const emptyString = "";
const char* const tempFileDir = "/tmp/imagefile.jpg";
const char* const faceCascadeFileDir = "/local/courses/csse2310/resources/a4/"
                                       "haarcascade_frontalface_alt2.xml";
const char* const eyeCascadeFileDir = "/local/courses/csse2310/resources/a4/"
                                      "haarcascade_eye_tree_eyeglasses.xml";
const char* const responseFileDir
        = "/local/courses/csse2310/resources/a4/responsefile";

const uint32_t serverMaxClients = 10000;
const int serverNumStatistics = 5;
const uint32_t msgPrefix = 0x23107231;
const uint32_t varMsgPrefix = 0x31721023;
const uint32_t serverMaxSize = (1UL << 32) - 1;
const uint32_t serverHeaderSize
        = sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint32_t);
const size_t decimalBase = 10;
const uint32_t timeout = 30;

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

/*
 * Enum to discriminate exit codes
 */
typedef enum {
    EXIT_USAGE = 6,
    EXIT_IMAGE_WRITE = 20,
    EXIT_CASCADE = 11,
    EXIT_PORT = 1
} ExitStatus;

/**
 * Enum to discriminate operation codes 
 */
typedef enum {
    FACE_DETECT = 0,
    FACE_REPLACE = 1,
    OUTPUT_IMAGE = 2,
    ERROR_MESSAGE = 3,
    INVALID_OP = 4
} OperationType;

/**
 * Enum to discriminate message codes 
 */
typedef enum {
    INVALID_MESSAGE = 0,
    INVALID_OPERATION = 1,
    IMAGE_ZERO_BYTES = 2,
    IMAGE_TOO_LARGE = 3,
    IMAGE_LOAD_ERROR = 4,
    IMAGE_NO_FACE = 5,
    RESPONSE_FILE = 6,
    SUCCESS = 7
} ErrorMessageCodes;

/**
 * Enum to distinguish what happened to a thread 
 */
typedef enum {
    CONNECTION = 0,
    COMPLETED = 1,
    DETECTION = 2,
    REPLACE = 3,
    MALFORMED = 4
} Stats;

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
    OperationType operation;
    ErrorMessageCodes error;
} Instructions;

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

typedef struct {
    uint32_t connections;
    uint32_t completed;
    uint32_t detectionRequests;
    uint32_t replaceRequests;
    uint32_t malformed;
    pthread_mutex_t* statLock;
} Statistics;

typedef struct {
    int socket;
    CascadeStruct* cascade; // malloc'd
    OpenCVStruct* openCVParameters; // malloc'd
    uint32_t imgMaxSize;
    sem_t* lock;
    Statistics* stats;
} ClientStruct;

const char* const errorMessageList[] = {"invalid message",
        "invalid operation type", "image is 0 bytes", "image too large",
        "invalid image", "no faces detected in image"};

const char* const statisticsList[] = {"Connected clients: ",
        "Clients completed: ", "Face detection requests: ",
        "Face replace requests: ", "Malformed requests: "};

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
void load_temp_file(uint8_t* fileBuf, uint32_t fileSize); // needs a semaphone
CascadeStruct* init_cascade_struct(Arguments* args);
int open_listen(Arguments* args);
uint32_t get_file_size(FILE* file);
ssize_t write_from_memory(int socket, const uint8_t* memory, size_t length);
void send_response_file(int socket);
void send_error_message(int socket, ErrorMessageCodes errorCode);
Message* format_message(
        OperationType operation, uint32_t length, uint8_t* content);
OpenCVStruct* init_open_cv_struct();
OpenCVStruct* load_detect_image(
        OpenCVStruct* image, CascadeStruct* cascadeParam);
OpenCVStruct* load_replace_image(OpenCVStruct* image);
void cv_detect_faces(CascadeStruct* cascadeParam, OpenCVStruct* image);
void cv_detect_and_replace_faces(OpenCVStruct* image);
ErrorMessageCodes read_check_prefix(int socket);
OperationType read_operation(int socket);
void* client_handler(void* c);
void print_statistics(Statistics* stats);
ClientStruct* init_client_parameters(int fd, CascadeStruct* cascade,
        sem_t* lock, Arguments* programArgs, Statistics* stats);
void server_runtime(int fdServer, Arguments* serverArgs,
        CascadeStruct* cascadeParam, sem_t* sem, Statistics* stats);
int read_to_buf(int socket, void* dest, uint32_t len);
ErrorMessageCodes read_data(int socket, uint32_t imgMaxSize);
void close_connection(ClientStruct* c);
Statistics* init_stats(void);
void spawn_signal_handler(Statistics* stats);
void increment_stats(Statistics* stats, Stats identifier);
void* signal_handler(void* arg);
void sigpipe_handler(int sig);
void ignore_sigpipe(void);
void increment_stat_and_close(ClientStruct* info, Stats identifier);
bool check_and_do_operation(
        Instructions ins, OpenCVStruct* detectImage, ClientStruct* clientInfo);
Instructions read_message(int socket, uint32_t imgMaxSize, sem_t* sem);

//---------------------------------------------------------------------------//
int main(int argc, char** argv)
{
    ignore_sigpipe();
    Arguments* args = argument_check(argc, argv);
    tmp_file_check(args);
    CascadeStruct* cascadeParameters = init_cascade_struct(args);
    int serverFD = open_listen(args);
    sem_t lock;
    sem_init(&lock, 0, 1);
    Statistics* stats = init_stats();
    spawn_signal_handler(stats);
    server_runtime(serverFD, args, cascadeParameters, &lock, stats);

    clean(args);
    sem_destroy(&lock);

    return 0;
}

void exit_usage_error(void)
{
    fprintf(stderr, usageErrMsg);
    exit(EXIT_USAGE);
}

void exit_image_error(void)
{
    fprintf(stderr, imageWriteErrMsg);
    exit(EXIT_IMAGE_WRITE);
}

void exit_cascade_error(void)
{
    fprintf(stderr, cascadeErrMsg);
    exit(EXIT_CASCADE);
}

void exit_port(char* port)
{ // might not be int
    fprintf(stderr, portErrMsg, port);
    exit(EXIT_PORT);
}

/**
 * Checks if a given string is a number  
 */
bool is_num(char* inputString)
{
    for (int i = 0; i < (int)strlen(inputString); i++) {
        if (!isdigit(inputString[i])) {
            return false;
        }
    }
    return true;
}

bool valid_max_clients(char* input)
{
    if (!is_num(input)) {
        return false;
    }
    long buffer = strtol(input, NULL, decimalBase);
    if (buffer > serverMaxClients || buffer < 0) {
        return false;
    }
    return true;
}

// 	 use strtoul REF: found from atol
bool valid_max_size(char* input)
{
    long buffer = strtol(input, NULL, decimalBase);
    if (buffer > serverMaxSize || buffer < 0) {
        return false;
    }
    return true;
}

void has_empty_string(int argc, char** argv)
{
    argv++; // remove program name
    argc--;
    for (int i = 0; i < argc; i++) {
        if (!strcmp(argv[i], emptyString)) {
            exit_usage_error();
        }
    }
    return;
}

Message* init_message(void)
{
    Message* message = (Message*)malloc(sizeof(Message));
    message->prefix = msgPrefix;
    message->operation = ERROR_MESSAGE; // 0 is a valid operation 3 for error
    message->detectImgSize = 0;
    message->detectImgContent = NULL;
    message->replaceImgSize = 0;
    message->replaceImgContent = NULL;
    return message;
}

Arguments* init_arguments(void)
{
    Arguments* args = (Arguments*)malloc(sizeof(Arguments));
    args->maxClients = 0;
    args->maxSize = 0;
    args->port = "0"; // default ephemeral
    return args;
}

/**
 * checks if the program arguments are valid
 *
 * @returns Arguments* struct on success 
 * @exits 6 if unsuccessful
 */
Arguments* argument_check(int argc, char** argv)
{
    has_empty_string(argc, argv);
    argc--; // remove program name
    argv++; // starts at max size
    Arguments* args = init_arguments(); // malloc'd

    if (argc < 2) {
        free((Arguments*)args);
        exit_usage_error();
    }
    if (!valid_max_clients(argv[0])) {
        free((Arguments*)args);
        exit_usage_error();
    } else {
        args->maxClients = atoi(argv[0]);
    }
    if (!valid_max_size(argv[1])) {
        free((Arguments*)args);
        exit_usage_error();
    }
    args->maxSize = strtoul(argv[1], NULL, decimalBase);
    if (argv[2] != NULL) {
        args->port = strdup(argv[2]); // malloc'd
    }
    return args;
}

void clean(Arguments* args)
{
    free((char*)args->port);
    free((Arguments*)args);
}

/**
 * checks if the temporary file directory can be opened
 *
 * @exits 20 if unable
 */
void tmp_file_check(Arguments* args)
{
    FILE* tmp = fopen(tempFileDir, "wrb");
    if (!tmp) {
        clean(args);
        exit(EXIT_IMAGE_WRITE);
    }
    fclose(tmp); // check if can be closed?
}

/**
 * initialises the data required for an operation of OpenCV 
 *
 * @args - arguments of the program
 */
CascadeStruct* init_cascade_struct(Arguments* args)
{ 
    CascadeStruct* param = (CascadeStruct*)calloc(1, sizeof(CascadeStruct));
    param->outputFile = NULL;
    param->faceCascade = (CvHaarClassifierCascade*)cvLoad(
            faceCascadeFileDir, NULL, NULL, NULL);
    param->eyeCascade = (CvHaarClassifierCascade*)cvLoad(
            eyeCascadeFileDir, NULL, NULL, NULL);
    if (!param->faceCascade || !param->eyeCascade) {
        clean(args);
        exit_cascade_error();
    }
    return param;
}

/**
 * Opens and starts listning on a specified port, or ephemeral port if unspecified 
 *
 * @args - arguments of the program
 * @returns file descriptor of the open port
 */
int open_listen(Arguments* args)
{
    struct addrinfo* ai = 0;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // listen on all IP addresses

    int err; // debugging
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

    // Bind socket to address
    if (bind(listenFD, (struct sockaddr*)ai->ai_addr, sizeof(struct sockaddr))
            < 0) { // not sure about the casting
        close(listenFD);
        freeaddrinfo(ai);
        fprintf(stderr, portErrMsg, args->port);
        clean(args);
        exit(EXIT_PORT); // Could not determine address
    }

    // Get portnum
    struct sockaddr_in ad;
    memset(&ad, 0, sizeof(struct sockaddr_in));
    socklen_t len = sizeof(struct sockaddr_in);
    getsockname(listenFD, (struct sockaddr*)&ad, &len);
    fprintf(stderr, "%u\n", ntohs(ad.sin_port));

    listen(listenFD, 0);
    freeaddrinfo(ai);

    fflush(stderr);
    fflush(stdout);
    return listenFD;
}

// assumes file is already opened
uint32_t get_file_size(FILE* file)
{ // REF: fseek man page
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, 0);
    return (uint32_t)size;
}

/**
 * Writes to the socket from a specified buffer in memory
 * 
 * @socket - file descriptor to be written total
 * @memory - buffer where data is store
 * @length - length of the data to be written
 *
 * @returns total number of bytes written to the socket
 */ 
ssize_t write_from_memory(int socket, const uint8_t* memory, size_t length)
{
    size_t total = 0;

    while (total < length) {
        ssize_t written = write(socket, memory + total, length - total);
        if (written < 0) {
            break;
        }
        if (written == 0) {
            break;
        }
        total += written;
    }
    return total;
}

/**
 * writes contents from a buffer into the temp file 
 *
 * @fileBuf - buffer containing the data to be written
 * @fileSize - size of the data 
 */
void load_temp_file(uint8_t* fileBuf, uint32_t fileSize)
{
    FILE* tempFile = fopen(tempFileDir, "wb");
    fwrite(fileBuf, 1, fileSize, tempFile);

    fclose(tempFile);
}

/**
 * Formats a message to be sent to the client 
 *
 * @operation - type of operation specified by enum OperationType
 * @length - length of the message
 * @content - buffer containing the byte stream to be sent
 * @returns pointer to a Message struct containing the data 
 */
Message* format_message(
        OperationType operation, uint32_t length, uint8_t* content)
{
    Message* message = init_message();
    message->prefix = msgPrefix;
    message->operation = operation;
    message->detectImgSize = length;
    message->detectImgContent = (uint8_t*)malloc(length);
    memcpy(message->detectImgContent, content, length);
    return message;
}

/**
 * Writes a message to the designated file descriptor
 *
 * @socket - file descriptor to be written to
 * @message - desired message
 */
void write_message(int socket, Message* message)
{
    uint32_t messageSize
            = serverHeaderSize + sizeof(uint8_t) * message->detectImgSize;
    size_t offset = 0;
    uint8_t* messageBuffer = (uint8_t*)malloc(messageSize);

    // populating message buffer
    memcpy(messageBuffer, &(message->prefix), sizeof(uint32_t));
    offset += sizeof(uint32_t);
    memcpy(messageBuffer + offset, &(message->operation), sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy(messageBuffer + offset, &(message->detectImgSize), sizeof(uint32_t));
    offset += sizeof(uint32_t);
    memcpy(messageBuffer + offset, message->detectImgContent,
            message->detectImgSize);

    write(socket, messageBuffer, messageSize);
}

/**
 * Sends a hardcoded response file over the socket 
 */
void send_response_file(int socket)
{
    // read and store response file
    FILE* responseFile = fopen(responseFileDir, "r");
    uint32_t responseFileSize = get_file_size(responseFile);
    uint8_t* responseFileContents = (uint8_t*)malloc(responseFileSize);
    fread(responseFileContents, sizeof(uint8_t), responseFileSize,
            responseFile);

    // sends file contents without following communication protocol
    write_from_memory(socket, responseFileContents, responseFileSize);
    fclose(responseFile);
    free((uint8_t*)responseFileContents);
}

/**
 * Sends a specified error message
 * 
 * @socket - file descriptor to be written to
 * @errorCode - type of error occured specified by enum ErrorMessageCodes
 */
void send_error_message(int socket, ErrorMessageCodes errorCode)
{
    size_t errorMessageLength = strlen(errorMessageList[errorCode]);
    uint8_t* buffer = (uint8_t*)malloc(errorMessageLength);
    memcpy(buffer, errorMessageList[errorCode], errorMessageLength);

    Message* errorMessage
            = format_message(ERROR_MESSAGE, errorMessageLength, buffer);
    write_message(socket, errorMessage);
    free((uint8_t*)buffer);
}

/**
 * Reads a byte stream over the socket to a designated buffer
 * 
 * @socket - file descriptor to be read from
 * @dest - buffer to store byte stream
 * @len - length of the data 
 */
int read_to_buf(int socket, void* dest, uint32_t len)
{
    uint8_t* buffer = (uint8_t*)malloc(sizeof(uint8_t));
    ssize_t result;
    uint32_t tally = 0;
    for (uint32_t i = 0; i < len; i++) {
        result = read(socket, buffer, sizeof(uint8_t));
        if (result < 0) {
            for (uint32_t j = 0; j < timeout; j++) {
                if (read(socket, buffer, sizeof(uint8_t)) > 0) {
                    break;
                }
            }
        }
        memcpy((uint8_t*)dest + i, buffer, 1);
        tally += result;
    }
    free((uint8_t*)buffer);
    if (tally < len) {
        return -1;
    }
    if (tally == 0) {
        return 1;
    }
    return 0;
}

OpenCVStruct* init_open_cv_struct()
{
    OpenCVStruct* temp = (OpenCVStruct*)malloc(sizeof(OpenCVStruct));
    temp->frame = NULL;
    temp->frameGray = NULL;
    temp->replace = NULL;
    temp->storage = NULL;
    temp->faces = NULL;
    temp->error = false;
    return temp;
}

ClientStruct* init_client_parameters(int fd, CascadeStruct* cascade,
        sem_t* lock, Arguments* programArgs, Statistics* stats)
{
    ClientStruct* info = (ClientStruct*)malloc(sizeof(ClientStruct));
    info->socket = fd;
    info->cascade = cascade;
    info->openCVParameters = NULL;
    info->imgMaxSize = programArgs->maxSize;
    info->lock = lock;
    info->stats = stats;
    return info;
}

/**
 * Loads an image with OpenCV for face detection 
 *
 * @image - initialised OpenCV parameters
 * @cascadeParam - initialised cascade parameters for OpenCV
 */
OpenCVStruct* load_detect_image(
        OpenCVStruct* image, CascadeStruct* cascadeParam)
{
    image->frame = cvLoadImage(tempFileDir, CV_LOAD_IMAGE_COLOR);
    if (!image->frame) { // error loading; send error message and close
                         // connection
        image->error = true;
        return image;
    }
    // Grayscale and equalise image
    image->frameGray = cvCreateImage(cvGetSize(image->frame), IPL_DEPTH_8U, 1);
    cvCvtColor(image->frame, image->frameGray, CV_BGR2GRAY);
    cvEqualizeHist(image->frameGray, image->frameGray);
    image->storage = cvCreateMemStorage(0);
    cvClearMemStorage(image->storage);
    // Detect Faces
    image->faces = cvHaarDetectObjects(image->frameGray,
            cascadeParam->faceCascade, image->storage, haarScaleFactor,
            haarMinNeighbours, haarFlags, cvSize(haarMinSize, haarMinSize),
            cvSize(haarMaxSize, haarMaxSize));
    if (image->faces->total == 0) {
        cvReleaseMemStorage(&(image->storage));
        cvReleaseImage(&(image->frame));
        cvReleaseImage(&(image->frameGray));
    }
    return image;
}

/**
 * Loads an image with OpenCV for face replacement 
 *
 * @image - initialised OpenCV parameters
 */
OpenCVStruct* load_replace_image(OpenCVStruct* image)
{
    image->replace = cvLoadImage(tempFileDir, CV_LOAD_IMAGE_UNCHANGED);
    if (!image->replace) {
        cvReleaseImage(&(image->frame));
        image->error = true;
    }
    return image;
}

/**
 * Detects faces with OpenCV 
 */
void cv_detect_faces(CascadeStruct* cascadeParam, OpenCVStruct* image)
{
    const CvScalar magenta = cvScalar(255, 0, 255, 0);
    const CvScalar blue = cvScalar(255, 0, 0, 0);
    // iterate through each face and draw ellipses
    for (int i = 0; i < image->faces->total; i++) {
        CvRect* face = (CvRect*)cvGetSeqElem(image->faces, i);
        CvPoint center
                = {face->x + face->width / 2, face->y + face->height / 2};
        cvEllipse(image->frame, center,
                cvSize(face->width / 2, face->height / 2), 0, ellipseStartAngle,
                ellipseEndAngle, magenta, lineThickness, lineType, shift);
        IplImage* faceROI
                = cvCreateImage(cvGetSize(image->frameGray), IPL_DEPTH_8U, 1);
        cvCopy(image->frameGray, faceROI, NULL);
        cvSetImageROI(faceROI, *face);
        // Create memory for calculations, allocate and clear it
        CvMemStorage* eyeStorage = 0;
        eyeStorage = cvCreateMemStorage(0);
        cvClearMemStorage(eyeStorage);
        // Detect eyes within each face
        CvSeq* eyes = cvHaarDetectObjects(faceROI, cascadeParam->eyeCascade,
                eyeStorage, haarScaleFactor, haarMinNeighbours, haarFlags,
                cvSize(haarMinSize, haarMinSize),
                cvSize(haarMaxSize, haarMaxSize));
        if (eyes->total == 2) {
            // Draw a circle around each eye
            for (int j = 0; j < eyes->total; j++) {
                CvRect* eye = (CvRect*)cvGetSeqElem(eyes, j);
                CvPoint eyeCenter = {face->x + eye->x + eye->width / 2,
                        face->y + eye->y + eye->height / 2};
                int radius = cvRound((eye->width / 2 + eye->height / 2) / 2);
                cvCircle(image->frame, eyeCenter, radius, blue, lineThickness,
                        lineType, shift);
            }
        }
        // Free memory
        cvReleaseImage(&faceROI);
        cvReleaseMemStorage(&eyeStorage);
    }
    cvSaveImage(tempFileDir, image->frame, 0); // free memory
    cvReleaseImage(&(image->frame));
    cvReleaseImage(&(image->frameGray));
    cvReleaseMemStorage(&(image->storage));
}

/**
 * Replaces the faces of a given image
 */
void cv_detect_and_replace_faces(OpenCVStruct* image)
{
    // Iterate through each detected face and replace it with an image
    for (int i = 0; i < image->faces->total; i++) {
        CvRect* face = (CvRect*)cvGetSeqElem(image->faces, i);
        IplImage* resized = cvCreateImage(cvSize(face->width, face->height),
                IPL_DEPTH_8U, image->replace->nChannels);
        // Resize the replacement image->to be the size of the face
        cvResize(image->replace, resized, CV_INTER_AREA);
        char* frameData = image->frame->imageData;
        char* faceData = resized->imageData;
        // Iterate through each pixel in the image->to replace
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
                int frameIndex = (image->frame->widthStep * (face->y + y))
                        + ((face->x + x) * image->frame->nChannels);
                frameData[frameIndex + 0] = faceData[faceIndex + 0];
                frameData[frameIndex + 1] = faceData[faceIndex + 1];
                frameData[frameIndex + 2] = faceData[faceIndex + 2];
            }
        }
        // Free memory
        cvReleaseImage(&resized);
    }
    // Save the processed image
    cvSaveImage(tempFileDir, image->frame, 0);
    // Free memory
    cvReleaseImage(&(image->frame));
    cvReleaseImage(&(image->replace));
    cvReleaseImage(&(image->frameGray));
    cvReleaseMemStorage(&(image->storage));
}

/**
 * Writes to the socket from the designated temp file
 */
void write_from_temp_file(int socket)
{
    FILE* tempFile = fopen(tempFileDir, "r");
    long imageSize = get_file_size(tempFile);
    uint8_t* imageData = (uint8_t*)malloc(imageSize);
    fread(imageData, sizeof(uint8_t), imageSize, tempFile);

    Message* response = format_message(OUTPUT_IMAGE, imageSize, imageData);
    write_message(socket, response);
    free((uint8_t*)imageData);
    free((Message*)response);
    fclose(tempFile);
}

/**
 * Reads the prefix of the message to see if it is properly formatted 
 *
 * @socket - file descriptor to be read from
 * @returns integer specified by ErrorMessageCodes
 */
ErrorMessageCodes read_check_prefix(int socket)
{
    uint32_t* prefBuffer = (uint32_t*)malloc(sizeof(uint32_t));

    if (read_to_buf(socket, prefBuffer, sizeof(uint32_t)) < 0) {
        free((uint32_t*)prefBuffer);
        return INVALID_MESSAGE;
    }
    if (*prefBuffer != msgPrefix && *prefBuffer != varMsgPrefix) {
        free((uint32_t*)prefBuffer);
        return RESPONSE_FILE;
    }

    free((uint32_t*)prefBuffer);
    return SUCCESS;
}

/**
 * Reads the operation code from the socket 
 */
OperationType read_operation(int socket)
{
    uint8_t* opBuffer = (uint8_t*)malloc(sizeof(uint8_t));
    if (read_to_buf(socket, opBuffer, sizeof(uint8_t)) < 0) {
        free((uint8_t*)opBuffer);
        return ERROR_MESSAGE;
    }

    OperationType operation = *opBuffer;
    free((uint8_t*)opBuffer);

    if (operation != FACE_DETECT && operation != FACE_REPLACE) {
        return INVALID_OP;
    }
    return operation;
}

/**
 * Reads the message from the socket
 *
 * @socket - file descriptor to be read from
 * @imgMaxSize - max image size allowable by the server 
 * @sem - semaphore to lock other threads from mutating the temp file 
 * @returns Instructions on how to move forward with the program including
 * 	if there is an error
 */
Instructions read_message(int socket, uint32_t imgMaxSize, sem_t* sem)
{
    Instructions inst = {.operation = 0, .error = SUCCESS};
    ErrorMessageCodes err;
    err = read_check_prefix(socket);
    if (err == INVALID_MESSAGE) {
        inst.error = err;
        send_error_message(socket, INVALID_MESSAGE);
        return inst;
    }
    if (err == RESPONSE_FILE) {
        send_response_file(socket);
        inst.error = err;
        return inst;
    }

    OperationType op = read_operation(socket);
    if (op != FACE_DETECT && op != FACE_REPLACE) {
        if (op == INVALID_OP) {
            send_error_message(socket, INVALID_OPERATION);
            inst.error = INVALID_OPERATION;
            return inst;
        }
        if (op == ERROR_MESSAGE) {
            send_error_message(socket, INVALID_MESSAGE);
            inst.error = INVALID_MESSAGE;
            return inst;
        }
    }
    inst.operation = op;

    sem_wait(sem);
    err = read_data(socket, imgMaxSize);
    if (err != SUCCESS) {
        inst.error = err;
        return inst;
    }
    return inst;
}


/**
 * reads image size and checks if is valid
 * reads image data ad stores into temp file if valid
 */
ErrorMessageCodes read_data(int socket, uint32_t imgMaxSize)
{
    uint32_t max = 0;
    if (imgMaxSize > 0) {
        max = imgMaxSize;
    } else {
        max = serverMaxSize;
    }
    uint32_t* imageSize = (uint32_t*)calloc(1, sizeof(uint32_t));
    int result = read_to_buf(socket, imageSize, sizeof(uint32_t));
    if (result < 0) {
        free((uint32_t*)imageSize);
        send_error_message(socket, IMAGE_ZERO_BYTES);
        return IMAGE_ZERO_BYTES;
    }
    if (*imageSize > max) {
        free((uint32_t*)imageSize);
        send_error_message(socket, IMAGE_TOO_LARGE);
        return IMAGE_TOO_LARGE;
    }
    if (*imageSize == 0) {
        free((uint32_t*)imageSize);
        send_error_message(socket, IMAGE_ZERO_BYTES);
        return IMAGE_ZERO_BYTES;
    }

    uint8_t* imageData = (uint8_t*)malloc(*imageSize);
    if (read_to_buf(socket, imageData, *imageSize) < 0) {
        free((uint32_t*)imageSize);
        free((uint8_t*)imageData);
        send_error_message(socket, IMAGE_ZERO_BYTES);
        return IMAGE_ZERO_BYTES;
    }
    load_temp_file(imageData, *imageSize);
    free((uint32_t*)imageSize);
    free((uint8_t*)imageData);
    return SUCCESS;
}

void close_connection(ClientStruct* c)
{
    sem_post(c->lock);
    close(c->socket);
    pthread_exit(NULL);
}

/**
 * used to wait for a SIGHUP signal before printing the server statistics 
 */
void* signal_handler(void* arg)
{
    Statistics* data = (Statistics*)arg;
    sigset_t sigset;
    int sig;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGHUP);

    while (1) {
        if (sigwait(&sigset, &sig) == 0) {
            print_statistics(data);
        }
    }
}

void sigpipe_handler(int sig)
{
    sig++;
}

/**
 * signal handler to ignore SIGPIPE signal
 */
void ignore_sigpipe(void)
{
    struct sigaction sa;
    sa.sa_handler = sigpipe_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGPIPE, &sa, NULL);
}

/**
 * spawns a signal handler thread to watch for SIGHUP
 */
void spawn_signal_handler(Statistics* stats)
{ // REF: man pthread_sigmask
    pthread_t signalThread;
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGHUP);
    pthread_sigmask(SIG_BLOCK, &set, NULL);
    pthread_create(&signalThread, NULL, signal_handler, stats);
    pthread_detach(signalThread);
}

void increment_stats(Statistics* stat, Stats identifier)
{
    pthread_mutex_lock(stat->statLock);
    switch (identifier) {
    case CONNECTION:
        stat->connections++;
        break;
    case COMPLETED:
        stat->connections--;
        stat->completed++;
        break;
    case DETECTION:
        stat->detectionRequests++;
        break;
    case REPLACE:
        stat->replaceRequests++;
        break;
    case MALFORMED:
        stat->connections--;
        stat->malformed++;
        break;
    default:
        break;
    }
    pthread_mutex_unlock(stat->statLock);
}

/**
 * increments the statistics before closing the connection with the client
 */
void increment_stat_and_close(ClientStruct* info, Stats identifier)
{
    increment_stats(info->stats, identifier);
    close_connection(info);
}

/**
 * Checks the instructions for the operation type before doing the designated
 * task
 *
 * @inst -  Instructions struct detailing what the client wants
 * @detectImage - OpenCVStruct containing the specified image to be detected
 * 		- and image to be replaced with if specified
 * @clientInfo - ClientInfo struct containing the statistics of the server
 */
bool check_and_do_operation(
        Instructions inst, OpenCVStruct* detectImage, ClientStruct* clientInfo)
{
    bool endRuntime = false;
    if (inst.operation == FACE_REPLACE && !detectImage->error) {
        ErrorMessageCodes err
                = read_data(clientInfo->socket, clientInfo->imgMaxSize);
        if (err != SUCCESS) {
            endRuntime = true;
            free((OpenCVStruct*)detectImage);
            increment_stat_and_close(clientInfo, MALFORMED);
        }
        OpenCVStruct* replaceImage = load_replace_image(detectImage);
        if (!replaceImage->replace) {
            endRuntime = true;
            send_error_message(clientInfo->socket, IMAGE_LOAD_ERROR);
            free((OpenCVStruct*)detectImage);
            free((OpenCVStruct*)replaceImage);
            increment_stat_and_close(clientInfo, MALFORMED);
        } else {
            cv_detect_and_replace_faces(replaceImage);
            increment_stats(clientInfo->stats, REPLACE);
        }
    } else {
        cv_detect_faces(clientInfo->cascade, detectImage);
        increment_stats(clientInfo->stats, DETECTION);
    }
    return endRuntime;
}

/**
 * Thread function to handle each client connection 
 * @c - to be casted to a ClientInfo struct that details all the information
 *	 required to handle a client
 */
void* client_handler(void* c)
{
    ClientStruct* clientInfo = (ClientStruct*)c;
    bool endRuntime = false;

    while (!endRuntime) {
        // take lock
        increment_stats(clientInfo->stats, CONNECTION);
        Instructions inst = read_message(
                clientInfo->socket, clientInfo->imgMaxSize, clientInfo->lock);
        if (inst.error != SUCCESS) {
            endRuntime = true;
            increment_stat_and_close(clientInfo, MALFORMED);
        }
        OpenCVStruct* image = init_open_cv_struct();
        OpenCVStruct* detectImage
                = load_detect_image(image, clientInfo->cascade);
        if (detectImage->error == true) {
            endRuntime = true;
            send_error_message(clientInfo->socket, IMAGE_LOAD_ERROR);
            increment_stat_and_close(clientInfo, MALFORMED);
        } else if (!detectImage->faces->total) {
            endRuntime = true;
            send_error_message(clientInfo->socket, IMAGE_NO_FACE);
            increment_stat_and_close(clientInfo, MALFORMED);
        }

        endRuntime = check_and_do_operation(inst, detectImage, clientInfo);

        write_from_temp_file(clientInfo->socket);
        free((OpenCVStruct*)image);
        increment_stats(clientInfo->stats, COMPLETED);
        sem_post(clientInfo->lock);
    }
    free((ClientStruct*)c);
    return NULL;
}

/**
 * spawn thread for each connection
 * ensure mutex of shared data structures, (CvHaarClassifierCascade)
 * if read() error or EOF from client, client handler must close connection,
 * clean and terminate
 */
void server_runtime(int fdServer, Arguments* serverArgs,
        CascadeStruct* cascadeParam, sem_t* sem, Statistics* stats)
{
    int fd;
    struct sockaddr_in fromAddr;
    socklen_t fromAddrSize;
    uint32_t conditional = serverArgs->maxClients;

    while (!conditional || stats->connections < conditional) {

        fromAddrSize = sizeof(struct sockaddr_in);
        // Block, waiting for a new connection. (fromAddr will be populated
        // with address of client)

        // accept connection
        fd = accept(fdServer, (struct sockaddr*)&fromAddr, &fromAddrSize);

        ClientStruct* clientParam = init_client_parameters(
                fd, cascadeParam, sem, serverArgs, stats);

        // threading
        pthread_t threadID;
        pthread_create(&threadID, NULL, client_handler, clientParam);
        pthread_detach(threadID);
    }
    cvReleaseHaarClassifierCascade(&(cascadeParam->faceCascade));
    cvReleaseHaarClassifierCascade(&(cascadeParam->eyeCascade));
    sem_destroy(sem);
    free((pthread_mutex_t*)stats->statLock);
    pthread_mutex_destroy(stats->statLock);
}

Statistics* init_stats(void)
{
    Statistics* data = (Statistics*)malloc(sizeof(Statistics));
    data->connections = 0;
    data->completed = 0;
    data->detectionRequests = 0;
    data->malformed = 0;
    data->statLock = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(data->statLock, NULL);
    return data;
}

void print_statistics(Statistics* stats)
{
    int i = 0;
    fprintf(stderr, "%s", statisticsList[i]);
    i++;
    fprintf(stderr, "%d\n", stats->connections);
    fprintf(stderr, "%s", statisticsList[i]);
    i++;
    fprintf(stderr, "%d\n", stats->completed);
    fprintf(stderr, "%s", statisticsList[i]);
    i++;
    fprintf(stderr, "%d\n", stats->detectionRequests);
    fprintf(stderr, "%s", statisticsList[i]);
    i++;
    fprintf(stderr, "%d\n", stats->replaceRequests);
    fprintf(stderr, "%s", statisticsList[i]);
    i++;
    fprintf(stderr, "%d\n", stats->malformed);
    fflush(stderr);
}
