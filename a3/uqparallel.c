#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <csse2310a3.h>
#include <stdbool.h>
#include <getopt.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

bool endRT = false;

typedef struct COMMAND {
    char** array;
    int length;
} COMMAND;

typedef enum {
    EXIT_NORMAL = 0,
    EXIT_USAGE = 14,
    EXIT_FILE = 2,
    EXIT_EMPTY = 78,
    EXIT_SIGNAL = 92,
    EXIT_PIPELINE = 78
} ExitStatus;

/////////////////////////////////////////////////////////////////////////////
const char* const empty_cmd_msg = "uqparallel: cannot execute empty command.\n";
const char* const usage_err_msg
        = "Usage: ./uqparallel [--dryrun] [--abort-on-error] [--maxjobs n] "
          "[--pipe] [--args-file argument-filename] [cmd [fixed-args ...]] "
          "[::: per-task-args ...]\n";
const char* const abort_err_msg
        = "uqparallel: aborting because of execution failure.\n";
const char* const cmd_err_msg = "uqparallel: \"%s\" not able to be executed\n";
const char* const file_err_msg
        = "uqparallel: Cannot open file \"%s\" for reading\n";
const char* const fileArg = "args-file";
const char* const maxJobsArg = "maxjobs";
const char* const abortArg = "abort-on-error";
const char* const pipeArg = "pipe";
const char* const dryrunArg = "dryrun";
const char* const ptOption = ":::";
const char* const emptyString = "";
const char* const optionDelim = "--";
///////////////////////////////////////////////////////////////////////////////

void sigfunc();
void cmd_err();
int isnum(char* string);
int option_index_calc(
        int optind, int abflg, int pflg, int dflg, int fflg, int mflg);
void arg_dup_check(int* flags);
char* str_combine(int count, char* strs[]);
bool cmd_check(char* cmd, struct option options[]);
char*** pertask_append(
        char** argumentArray, char** argv, int cmdcount, int argcount);
char* remove_NL(char* string);
int count_jobs(FILE* file);
int count_cmd(char** cmdlines);
COMMAND populate_pt_args(int argc, char** argv, int index);
int adjust_argc(int argc, char* argv[]);
int min(int x, int y);
void print_string_with_quotes(char* string);
bool quote_check(char* string);
void print_array(int size, char* array[]);
char** append_to_array(char** array1, char** array2);
pid_t spawn_child_exec(char** cmd);
pid_t* spawn_child_array(char** cmd, int N);
void free_array2d(char** array, int size);
void free_array3d(char*** array, int size1, int size2);
int wait_children(int numChildren, pid_t* pidArray, char*** execArray);
int status_check(int status, char* cmd);
COMMAND parse_cmd(int argc, char** argv, int optind);
int run_pertask_args_cmd(char** ptArgs, int ptArgsCount, int argc, char** argv,
        int pflg, int mflg, int maxJobs);
COMMAND parse_options(int argc, char** argv);
char*** parse_cmd_file(FILE* file, int jobCount);
char** remove_arguments(int argc, char* argv[]);
void dryfile(
        FILE* file, int pflg, int cmdflg, int argc, char** argv, int optind);
void drypt(int argc, char** argv, char** ptArgs, int ptArgsCount, int optind,
        int pflg);
void drynoarg(int argc, char** argv, int optind);
void argsfile(FILE* file, int pflg, int jobCount, int mflg, int maxJobs);
int no_args(void);
int stdinloop(COMMAND cmd);
pid_t* spawn_maxJobs(int totaljobs, int maxJobs, char*** execArray);
int pipeline(char*** command_vector, int cmdcount);
int run_pertask_args(
        char** ptArgs, int ptArgsCount, int pflg, int mflg, int maxJobs);

int main(int argc, char* argv[])
{
    int exitStatus = 0;

    if (argc == 1) {
        exitStatus = no_args();
    }

    // cmd parsing
    int c;
    int optind = -1;
    opterr = 0;
    int fflg = 0;
    int abflg = 0;
    int pflg = 0;
    int mflg = 0;
    int dflg = 0;
    int ptflg = 0;
    int cmdflg = 0;
    int errflg = 0;
    int maxJobs = -1;
    COMMAND ptArgs = {.array = NULL, .length = 0};
    char* fileName = NULL;
    struct sigaction sa; // could probably put sighandler after option parsing
                         // so if abort-on-error then include SA_NOCLDSTOP
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigfunc;
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa, 0); // i think add SIGTERM SIGUSR1

    // option arguments
    FILE* inputFile = NULL;

    static struct option options[] = {{abortArg, no_argument, 0, 'a'},
            {fileArg, required_argument, 0, 'f'},
            {dryrunArg, no_argument, 0, 'd'}, {pipeArg, no_argument, 0, 'p'},
            {maxJobsArg, required_argument, 0, 'm'}, {0, 0, 0, 0}};

    if (argc > 1 && cmd_check(argv[1], options)) {
        if (strcmp(argv[1], emptyString) == 0) {
            cmd_err();
        }
    }

    COMMAND option_array = parse_options(
            argc, argv); // to only parse -- long options so -a and
                         // --abort-on-error dont get mixed up

    while ((c = getopt_long(option_array.length, option_array.array,
                    "dpa:m:f:", options, &optind))
            != -1) { // REF:
                     // https://www.man7.org/linux/man-pages/man3/getopt.3.html
        switch (c) {
        case 'a':
            abflg++;
            break;
        case 'f':
            fflg++;
            inputFile = fopen(optarg, "r");
            fileName = strdup(optarg);
            break;

        case 'd': // dryrun
            dflg++;
            break;

        case 'p': // pipe
            pflg++;
            break;

        case 'm': // maxJobs
            mflg++;

            int buffer = isnum(optarg);
            if (buffer < 1 || buffer > 130) { // 1 < n <= 130
                cmd_err();
            }
            maxJobs = buffer;

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
    }

    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], ptOption) == 0) {
            if (fflg != 0) {
                cmd_err();
            } else {
                ptflg++;
                errflg++;

                ptArgs = populate_pt_args(argc, argv, i);
                continue;
            }
        }
    }

    argv = remove_arguments(argc, argv); // PROBABLE MEMORY LEAK
    argc = adjust_argc(argc, argv);

    int flg_array[] = {abflg, pflg, dflg, fflg, mflg};
    arg_dup_check(flg_array);
    optind = -1;
    optind = option_index_calc(optind, abflg, pflg, dflg, fflg,
            mflg); // indexes the last option in the cmdline

    // more argument checking
    if (pflg && !(!errflg ^ !fflg)) {
        cmd_err();
    }
    if (fflg && !inputFile) {
        fprintf(stderr, file_err_msg, fileName);
        exit(2);
    }
    if (argv[optind] != NULL) {
        if (cmd_check(argv[optind], options)) {
            cmdflg++;
        }
    }

    /* Actual Executions */
    //----------------------------------------------------------------------------------------------------------------------------------------//
    // dryrun
    if (dflg) {
        if (fflg) {
            dryfile(inputFile, pflg, cmdflg, argc, argv, optind);
        } else if (ptflg) {
            drypt(argc, argv, ptArgs.array, ptArgs.length, optind, pflg);
        } else {
            drynoarg(argc, argv, optind);
        }

    }

    // run on file
    else if (fflg) {
        int jobCount = count_jobs(inputFile);
        rewind(inputFile);

        if (cmdflg) { // cmd present with file
            COMMAND cmd = parse_cmd(argc, argv, optind);
            pid_t* pids = (pid_t*)malloc(jobCount * sizeof(pid_t));
            char*** file_cmdArray = parse_cmd_file(
                    inputFile, jobCount); // screws with cmd above
            char*** execArray = (char***)malloc(jobCount * sizeof(char**));

            for (int i = 0; i < jobCount; i++) {
                execArray[i] = append_to_array(cmd.array, file_cmdArray[i]);
            }
            if (pflg) {
                exitStatus = pipeline(execArray, jobCount);
            } else {
                for (int i = 0; i < jobCount; i++) {
                    pids[i] = spawn_child_exec(execArray[i]);
                }
            }

            exitStatus = wait_children(jobCount, pids, execArray);

            free(file_cmdArray);
            free(execArray);
            // free(pids); // causes error
        } else {
            argsfile(inputFile, pflg, jobCount, mflg, maxJobs);
        }
    } else if (ptflg) { // run per task arugments
        if (!ptArgs.length) {
            exitStatus = EXIT_EMPTY;
        }
        if (cmdflg) {
            exitStatus = run_pertask_args_cmd(ptArgs.array, ptArgs.length, argc,
                    argv, pflg, mflg, maxJobs);
        } else { // NO CMD GIVEN
            exitStatus = run_pertask_args(
                    ptArgs.array, ptArgs.length, pflg, mflg, maxJobs);
        }
    } else if (cmdflg) {
        COMMAND cmd = parse_cmd(argc, argv, optind);
        exitStatus = stdinloop(cmd);
    } else if (argc != 1) {
        exitStatus = no_args();
    }

    // FREE MEMORY
    for (int i = 0; i < ptArgs.length; i++) {
        free(ptArgs.array[i]);
    }
    free(ptArgs.array);
    free(fileName);
    exit(exitStatus);
}

/* Helper Functions */
//-----------------------------------------------------------------------------------------------------------------------------------------//

void sigfunc()
{
    endRT = true;
}

void cmd_err()
{
    fprintf(stderr, usage_err_msg);
    exit(EXIT_USAGE);
}

int isnum(char* string)
{
    int len = strlen(string);

    for (int i = 0; i < len; i++) {
        if (!isdigit(string[i])) {
            return 0;
        }
    }
    return atoi(string);
}

void arg_dup_check(int* flags)
{
    for (int i = 0; i < 5; i++) { // number of flags
        if (flags[i] > 1) {
            cmd_err();
        }
    }
    return;
}

int option_index_calc(
        int optind, int abflg, int pflg, int dflg, int fflg, int mflg)
{
    optind = (optind < 0) ? 1 : optind;
    if (abflg || pflg || dflg) {
        int flgsum = abflg + pflg + dflg;
        optind = optind + flgsum;
    }
    if (fflg || mflg) {
        int flgsum = fflg + mflg;
        optind = optind + 2 * flgsum;
    }
    return optind;
}

int min(int x, int y)
{
    int result;
    result = (x < y) ? x : y;
    return result;
}

char* str_combine(int count, char* strs[])
{
    char* buffer = strdup(strs[0]);
    if (count == 1) {
        return buffer;
    }

    for (int i = 1; i < count; i++) {
        strcat(buffer, " ");
        strcat(buffer, strs[i]);
    }
    return buffer;
    /*
    free(buffer);
    */
}

bool cmd_check(char* cmd, struct option options[])
{ // returns true if is a command, false if is an option
    for (int i = 0; i < 5; i++) { // number of possible options
        if (!strstr(cmd, options[i].name)) {
            if (i == 4) {
                return true;
            }
            continue;
        } else {
            break;
        }
        return true;
    }
    return false;
}

void print_string_with_quotes(char* string)
{
    fprintf(stdout, "\"");
    fprintf(stdout, "%s", string);
    fprintf(stdout, "\"");
}

bool quote_check(char* string)
{
    for (int i = 0; i < (int)strlen(string); i++) {
        if (strstr(string, " ")) {
            return true;
        }
    }
    return false;
}

void print_array(int size, char* array[])
{ // prints array separated by whitespace
    char* buffer;
    char* newstr;
    for (int i = 0; i < size; i++) {
        buffer = strdup(array[i]);
        newstr = remove_NL(buffer);
        if (quote_check(array[i])) {
            if (i == size - 1) {
                print_string_with_quotes(newstr);
            } else {
                print_string_with_quotes(newstr);
                fprintf(stdout, " ");
            }
        } else {
            if (i == size - 1) {
                fprintf(stdout, "%s", newstr);
            } else {
                fprintf(stdout, "%s ", newstr);
            }
            free(buffer);
        }
    }
}

char* remove_NL(char* string)
{
    int len = (int)strlen(string);
    if (string[len - 1] == '\n') {
        string[len - 1] = '\0';
        return string;
    } else {
        return string;
    }
}

int count_jobs(FILE* file)
{ // NEEDS TO rewind() BEFORE NEXT fgets() use
    char* line = NULL;
    size_t bufLen = 0;
    ssize_t nread;
    int tally = 0;
    while ((nread = getline(&line, &bufLen, file)) != -1) {
        tally++;
    }
    free(line);
    return tally;
}

int count_cmd(char** cmdlines)
{
    int i = 0;
    int tally = 0;

    while (cmdlines[i] != NULL) {
        tally++;
        i++;
    }
    return tally;
}

char** remove_arguments(int argc, char* argv[])
{
    for (int i = 1; i < argc; i++) {
        if (strstr(argv[i], ":::")) {
            while (i != argc) {
                argv[i] = NULL;
                i++;
            }
        }
    }
    return argv;
}

int adjust_argc(int argc, char* argv[])
{
    int i;
    for (i = 0; i < argc; i++) {
        if (argv[i] == NULL) {
            break;
        }
    }
    return i;
}

COMMAND parse_options(int argc, char** argv)
{
    COMMAND options;
    options.length = 1;
    options.array = (char**)malloc(argc * sizeof(char*));
    options.array[0] = strdup(argv[0]);

    for (int i = 0; i < argc; i++) { // getopt_long() starts at argv[1]
        if (strstr(argv[i], optionDelim)) {
            options.array[options.length] = strdup(argv[i]);
            options.length++;
        }
        if (strstr(argv[i], fileArg) || strstr(argv[i], maxJobsArg)) {
            if (argv[i + 1] != NULL && !strstr(argv[i + 1], optionDelim)) {
                options.array[options.length] = strdup(argv[i + 1]);
                options.length++;
            }
        }
    }
    options.array = realloc(options.array, (sizeof(char*) * options.length));
    options.array[options.length] = NULL;
    return options;
    /*
     * TO FREE
    for (int i = 0 i < options.length ; i++) {
            free(options.array[i]);
    }
    free(options.array);
    */
}

// returns pointer new array of strings with pertask args appended to them
char*** pertask_append(
        char** argumentArray, char** cmds, int cmdcount, int argcount)
{ // make it append the current argv[] with the pertask arguments
    char*** newArray = (char***)malloc(
            argcount * sizeof(char**)); // include null terminator

    // ALLOCATE MEMORY
    for (int i = 0; i < argcount; i++) {
        newArray[i] = (char**)malloc((cmdcount + 2)
                * sizeof(char*)); // include size for commands and argument and
                                  // null terminator
    }

    for (int i = 0; i < argcount; i++) {
        for (int j = 0; j < cmdcount; j++) {
            newArray[i][j] = cmds[j];
        }
        newArray[i][cmdcount] = argumentArray[i];
        newArray[i][cmdcount + 1] = NULL;
    }
    return newArray;
}

// appends array2 to array 1
char** append_to_array(char** array1, char** array2)
{
    int len1 = count_cmd(array1);
    int len2 = count_cmd(array2);
    char** buffer = (char**)malloc((len1 + len2) * sizeof(char*));

    for (int i = 0; i < len1; i++) {
        buffer[i] = strdup(array1[i]);
    }
    for (int j = 0; j < len2; j++) {
        buffer[len1 + j] = strdup(array2[j]);
    }

    buffer[len1 + len2] = NULL;

    return buffer;
    /*
            for (int i = 0 ; i < len1 + len2 ; i++){
                    free(buffer[i]);
            }
                                                                                                                            */
}

pid_t spawn_child_exec(char** cmd)
{
    if (cmd == NULL) {
        return -1;
    }
    pid_t pid = fork();
    if (pid < 0) {
        return -1;
    } else if (pid == 0) {
        close(STDERR_FILENO);
        if (!strcmp(cmd[0], "") || cmd[0] == NULL) {
            exit(EXIT_EMPTY);
        }
        execvp(cmd[0], cmd);
        raise(SIGUSR1);
        // exit(9999999); // for checking
    } else {
        return pid;
    }
    return pid;
}

pid_t* spawn_child_array(char** cmd, int N)
{ // dont know if this works
    pid_t* pid = (pid_t*)malloc(N * sizeof(pid_t));
    for (int i = 0; i < N; i++) {
        pid[i] = spawn_child_exec(cmd);
    }
    return pid;
}

// parses comands from argv from the optind which points at the last option
// argument
COMMAND parse_cmd(int argc, char** argv, int optind)
{
    char** buffer = (char**)malloc(sizeof(char*) * (argc - 1));
    int index = 0;
    COMMAND command;
    int numtokens;

    for (int i = optind; i < argc; i++) {
        buffer[index] = strdup(argv[i]);
        index++;
    }

    char* buffer2 = str_combine(argc - optind, buffer);

    // FREE buffer
    for (int i = 0; i < argc - 1; i++) {
        free(buffer[i]);
    }
    free(buffer);

    command.array = split_space_not_quote(buffer2, &numtokens);
    command.length = numtokens;

    return command;
}

// returns array of commands from file
char*** parse_cmd_file(FILE* file, int jobCount)
{
    char* buffer = NULL;
    size_t len = 0;
    ssize_t nLines;
    int index = 0;
    int numtokens;

    char*** execArray = (char***)malloc(
            jobCount * sizeof(char**)); // stores pointers to cmdArray in array

    // GET STRING FROM FILE AND STORE IN ARRAY
    while ((nLines = getline(&buffer, &len, file)) != -1) {
        char* strbuffer = remove_NL(buffer);
        if (!strcmp(strbuffer, "")) {
            execArray[index] = NULL;
            continue;
        }
        char** cmds = split_space_not_quote(strbuffer, &numtokens);

        execArray[index] = (char**)malloc((numtokens + 1) * sizeof(char*));
        for (int i = 0; i < numtokens; i++) {
            execArray[index][i] = strdup(cmds[i]);
        }

        execArray[index][numtokens] = NULL;
        index++;
        free(cmds);
    }
    return execArray;
}

void free_array2d(char** array, int size)
{
    for (int i = 0; i < size; i++) {
        free(array[i]);
    }
    free(array);
}

void free_array3d(char*** array, int size1, int size2)
{
    for (int i = 0; i < size1; i++) {
        for (int j = 0; j < size2; j++) {
            free(array[i][j]);
        }
    }
    free(array);
}

int status_check(int status, char* cmd)
{
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        fprintf(stderr, cmd_err_msg, cmd);
        return EXIT_SIGNAL;
    }
    return EXIT_EMPTY;
}

int wait_children(int numChildren, pid_t* pidArray, char*** execArray)
{
    int status;
    int exitStatus = EXIT_EMPTY;
    int lastChildIndex = 0;
    for (int i = 0; i < numChildren; i++) {
        if (waitpid(pidArray[i], &status, 0) > 0) {
            if (i == numChildren - 1) {
                lastChildIndex = i;
            }
            exitStatus = status_check(status, execArray[i][0]);
        }
    }
    if (waitpid(pidArray[lastChildIndex], &status, 0) > 0) {
        exitStatus = status_check(status, execArray[lastChildIndex][0]);
    }
    return exitStatus;
}

/* Working Functions */
//------------------------------------------------------------------------------------------------------------------------------//
void dryfile(
        FILE* file, int pflg, int cmdflg, int argc, char** argv, int optind)
{ // rewrite this with append_to_array
    char* buffer = NULL;
    size_t len = 0;
    ssize_t nLines;
    int jobNum = 1;
    int totaljobs = count_jobs(file);
    rewind(file);
    if (cmdflg) {
        COMMAND cmd = parse_cmd(argc, argv, optind);
        while ((nLines = getline(&buffer, &len, file)) != -1) {
            int numtokens;
            char* buffer1 = remove_NL(buffer);
            char** file_args = split_space_not_quote(buffer1, &numtokens);

            char** output_array = append_to_array(cmd.array, file_args);
            int array_length = cmd.length + numtokens;

            fprintf(stdout, "%d: ", jobNum);
            print_array(array_length, output_array);

            if (pflg && jobNum < totaljobs) {
                fprintf(stdout, " |\n");
            } else {
                fprintf(stdout, "\n");
            }
            jobNum++;
            free(file_args);
        }
        free(cmd.array);
    } else {
        while ((nLines = getline(&buffer, &len, file)) != -1) {
            int numtokens;
            char* temp = remove_NL(buffer);
            char** newstring = split_space_not_quote(temp, &numtokens);
            fprintf(stdout, "%d: ", jobNum);
            print_array(numtokens, newstring);

            if (pflg && jobNum < totaljobs) {
                fprintf(stdout, " |\n");
            } else {
                fprintf(stdout, "\n");
            }
            jobNum++;
        }
    }
    return;
}

void drypt(int argc, char** argv, char** ptArgs, int ptArgsCount, int optind,
        int pflg)
{
    int jobNum = 1;
    int cmdCount = argc - optind;

    char*** execArray = (char***)malloc(ptArgsCount * sizeof(char**));
    char** cmdArray = (char**)malloc(cmdCount * sizeof(char*));

    // POPULATE ARRAY OF COMMANDS
    for (int i = 0; i < cmdCount; i++) {
        cmdArray[i] = strdup(argv[optind + i]);
    }

    // NEW ARRAY OF APPENDED COMMANDS
    execArray = pertask_append(ptArgs, cmdArray, cmdCount, ptArgsCount);

    while (jobNum <= ptArgsCount) {
        fprintf(stdout, "%d: ", jobNum);
        print_array(
                cmdCount + 1, execArray[jobNum - 1]); // smells like a segfault
        if (pflg && jobNum < ptArgsCount) {
            fprintf(stdout, " |\n");
        } else {
            fprintf(stdout, "\n");
        }
        jobNum++;
    }

    for (int i = 0; i < cmdCount; i++) {
        free(cmdArray[i]);
    }
    free(cmdArray);
    free(execArray);
}

void drynoarg(int argc, char** argv, int optind)
{
    char* buffer = NULL;
    size_t len = 0;
    ssize_t nLines;
    int jobNum = 1;
    char** cmdArray = (char**)malloc(
            (argc - optind) * sizeof(char*)); // might be unecessary

    for (int i = 0; i < (argc - optind); i++) {
        cmdArray[i] = strdup(argv[optind + i]);
    }
    while ((nLines = getline(&buffer, &len, stdin)) != -1) {
        int numtokens;
        char** outstr = split_space_not_quote(buffer, &numtokens);
        fprintf(stdout, "%d: ", jobNum);
        char** output_array = append_to_array(cmdArray, outstr);
        free(outstr);
        print_array(argc - optind + numtokens, output_array);
        fprintf(stdout, "\n");
        jobNum++;
    }
    free(cmdArray);
    return;
}

void argsfile(FILE* file, int pflg, int jobCount, int mflg, int maxJobs)
{
    char*** execArray = parse_cmd_file(file, jobCount);
    int status = EXIT_EMPTY;

    if (pflg) {
        // RUN PIPELINE
        status = pipeline(execArray, jobCount);

        // FREE MEMORY ARRAY
        for (int i = 0; i < jobCount; i++) {
            free(execArray[i]);
        }
        return;
    } else if (mflg) {
        pid_t* pids = spawn_maxJobs(jobCount, maxJobs, execArray);
        status = wait_children(jobCount, pids, execArray);
    } else {
        pid_t* pids = (pid_t*)malloc(sizeof(pid_t) * jobCount);

        for (int i = 0; i < jobCount; i++) {
            if (execArray[i] == NULL) {
                continue;
            }
            if (!(pids[i] = fork())) { // look at parent and store pid in array
                close(STDERR_FILENO);
                if (!strcmp(execArray[i][0], "") || execArray[i] == NULL) {
                    exit(EXIT_EMPTY);
                }
                execvp(execArray[i][0], execArray[i]);
                raise(SIGUSR1);
            }
        }

        // WAIT FOR DEATH
        status = wait_children(jobCount, pids, execArray);
        free(pids);
    }

    // FREE MEMORY ARRAY
    for (int i = 0; i < jobCount; i++) {
        free(execArray[i]);
    }
    // might need to free one more line not sure

    // FREEING MEMORY
    free(file);
    exit(status);
}

int no_args(void)
{
    char* buffer = NULL;
    size_t len = 0;
    ssize_t nLines;
    int index = 0;
    int numtokens;
    int jobCount = 1;
    int status = EXIT_EMPTY; // starts as empty but maybe shouldnt be

    char*** cmdArray = (char***)malloc(
            jobCount * sizeof(char**)); // stores cmds in array
    pid_t* pids = (pid_t*)malloc(jobCount * sizeof(pid_t)); // pid array

    // GET STRING FROM FILE AND STORE IN ARRAY
    while ((nLines = getline(&buffer, &len, stdin)) != -1) {
        char* strbuffer = remove_NL(buffer);
        if (!strcmp(strbuffer, "")) {
            status = EXIT_EMPTY;
            continue;
        }
        char** cmds = split_space_not_quote(strbuffer, &numtokens);

        if (jobCount > 1) {
            cmdArray = (char***)realloc(cmdArray, jobCount * sizeof(char**));
        }

        cmdArray[index] = cmds;
        pids[index] = spawn_child_exec(cmdArray[index]);
        index++;
        jobCount++;
        free(cmds);
    }

    // WAIT FOR DEATH
    status = wait_children(jobCount, pids, cmdArray);
    free(pids);
    free(cmdArray);
    return status;
}

pid_t* spawn_maxJobs(int totaljobs, int maxJobs, char*** execArray)
{
    int numChildren = 0;
    int jobNum = 0;
    int status;
    pid_t* pids = (pid_t*)malloc(totaljobs * sizeof(pid_t));

    while (jobNum < totaljobs) { // unsure if <=
        if (numChildren < maxJobs) {
            pids[jobNum] = spawn_child_exec(execArray[jobNum]);
            jobNum++;
            numChildren++;
        }
        if (endRT) {
            pid_t pid;
            while (pid = waitpid(-1, &status, WNOHANG), pid > 0) {
                numChildren--;
            }
            endRT = false;
        }
    }
    return pids;
}

void close_pipes(int cmdNum, int** fds)
{
    for (int i = 0; i < cmdNum - 1; i++) {
        for (int j = 0; j < 2; j++) {
            close(fds[i][j]);
        }
    }
}

// from **cmds[] cmd1 --> cmd2 --> cmd3 --> ... --> stdout
int pipeline(char*** command_vector, int cmdcount)
{ // needs a wait thing and EXIT_PIPELINE if theres something wrong
    int** fds = (int**)malloc(cmdcount * sizeof(int*));

    for (int i = 0; i < cmdcount; i++) {
        fds[i] = (int*)malloc(2 * sizeof(int));
    }

    // POPULATE PIPES
    for (int i = 0; i < cmdcount; i++) {
        if (pipe(fds[i]) < 0) {
            perror("Creating pipe");
            exit(1);
        }
    }

    for (int i = 0; i < cmdcount; i++) {
        // PARENT
        if (fork()) {
            // first command gets ignored
            if (i != 0) {
                dup2(fds[i - 1][0], STDIN_FILENO);
            }

            // pipe to next command if not last command
            if (i != cmdcount - 1) {
                dup2(fds[i][1], STDOUT_FILENO);
            }

            for (int j = 0; j < cmdcount - 1; j++) {
                close(fds[j][0]);
                close(fds[j][1]);
            }

            execvp(command_vector[i][0], command_vector[i]);
            close_pipes(cmdcount, fds);
            return EXIT_PIPELINE;
        }
    }

    // CLOSE ALL PIPES
    close_pipes(cmdcount, fds);
    // WAIT FOR CHILDREN
    for (int i = 0; i < cmdcount; i++) {
        wait(0);
    }

    // FREE MEMORY
    for (int i = 0; i < cmdcount; i++) {
        free(fds[i]);
    }
    free(fds);
    return 0;
}

int stdinloop(COMMAND input)
{
    char* buffer = NULL;
    size_t len = 0;
    ssize_t nLines;
    int numtokens;
    int index = 0;
    int jobCount = 1;
    int status;

    pid_t* pids = (pid_t*)malloc(jobCount * sizeof(pid_t));
    char*** cmdArray = (char***)malloc(jobCount * sizeof(char**));

    while ((nLines = getline(&buffer, &len, stdin)) != -1) {
        char* strbuffer = remove_NL(buffer);
        char** cmds = split_space_not_quote(strbuffer, &numtokens);

        if (jobCount > 1) {
            cmdArray = (char***)realloc(cmdArray, jobCount * sizeof(char*));
            pids = (pid_t*)realloc(pids, jobCount * sizeof(pid_t));
        }

        cmdArray[index] = append_to_array(input.array, cmds);
        pids[index] = spawn_child_exec(cmdArray[index]);
        index++;
        jobCount++;
    }

    // WAIT FOR DEATH
    status = wait_children(jobCount, pids, cmdArray);
    // FREE'ing
    free(cmdArray);
    return status;
}

COMMAND populate_pt_args(int argc, char** argv, int index)
{
    COMMAND ptArgs;
    ptArgs.length = argc - index - 1;
    ptArgs.array = (char**)malloc(ptArgs.length * sizeof(char*));

    for (int i = 0; i < ptArgs.length; i++) {
        ptArgs.array[i] = strdup(argv[index + 1 + i]);
    }
    return ptArgs;
}

int run_pertask_args_cmd(char** ptArgs, int ptArgsCount, int argc, char** argv,
        int pflg, int mflg, int maxJobs)
{
    COMMAND cmd = parse_cmd(argc, argv, optind);
    char*** execArray
            = pertask_append(ptArgs, cmd.array, cmd.length, ptArgsCount);
    int status;

    if (pflg) {
        status = pipeline(execArray, ptArgsCount);
    } else if (mflg) {
        pid_t* pids = spawn_maxJobs(ptArgsCount, maxJobs, execArray);
        status = wait_children(ptArgsCount, pids, execArray);
    } else {
        pid_t* pids = (pid_t*)malloc(ptArgsCount * sizeof(pid_t));
        for (int i = 0; i < ptArgsCount; i++) {
            pids[i] = spawn_child_exec(execArray[i]);
        }

        // WAITING
        status = wait_children(ptArgsCount, pids, execArray);
    }

    // FREE'ing
    for (int i = 0; i < ptArgsCount; i++) {
        free(execArray[i]);
    }
    free(execArray);
    return status;
}

int run_pertask_args(
        char** ptArgs, int ptArgsCount, int pflg, int mflg, int maxJobs)
{
    char*** execArray = calloc(ptArgsCount, sizeof(char**));
    int status;
    for (int i = 0; i < ptArgsCount; i++) {
        execArray[i] = (char**)malloc(sizeof(char*));
    }

    for (int i = 0; i < ptArgsCount; i++) {
        execArray[i][0] = strdup(ptArgs[i]);
    }
    if (pflg) {
        status = pipeline(execArray, ptArgsCount);
    }

    else if (mflg) {
        pid_t* pids = spawn_maxJobs(ptArgsCount, maxJobs, execArray);
        status = wait_children(ptArgsCount, pids, execArray);
    } else {
        pid_t* pids = (pid_t*)malloc(ptArgsCount * sizeof(pid_t));

        for (int i = 0; i < ptArgsCount; i++) {
            pids[i] = spawn_child_exec(execArray[i]);
        }

        // WAITING
        status = wait_children(ptArgsCount, pids, execArray);
    }

    // FREE'ing
    free_array3d(execArray, ptArgsCount, 1);
    return status;
}
