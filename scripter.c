/**
 *
 * scripter.c
 *
 * This program launches another program (called the process) and acts
 * as a wrapper for it. Input and output from Scripter are mirrored to
 * and from the process. Each line of output from the process are sent
 * to another program that is launched (called the handler).
 * The handler processes the output, doing anything imaginable with
 * the output from the process. Anything printed from the handler (as
 * a response to the process or otherwise) is in turn sent to the
 * process.
 *
 * Usage:
 * ./scripter <arguments to launch process> <arguments to launch handler>
 *
 * It is recommended to use quotation marks, particularly if either
 * the "process" or "handler" have arguments
 *
 * An additional recommendation is to use shell scripts as the handler
 * and the process, so the run arguments for Scripter don't have to
 * change from run to run, and desired changes to the run configurations
 * are saved in the scripts.
 *
 * @author Henry Farr
 * @email hfarr@hey.com
 *
 */

/*
 * !!!!!!!!!!IMPORTANT!!!!!!!!!!!
 * !                            !
 * ! Must compile with -pthread !
 * !          flag              !
 * !                            !
 * !!!!!!!!!!IMPORTANT!!!!!!!!!!!
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#define STDIN 0
#define STDOUT 1
#define STDERR 2

// make this an environment variable?
#define BUF_SIZE 2048

// Method primitives... is that what these are called? fail vocab check
int forkchild(int fileDescriptorOut, int fileDescriptorIn, char* path);
int readline(FILE *stream, char* line, int buf_size);
void handle(char* input, char* result, char *args[], int pid);
void pipeSetup();
void *readInput(void *ignored);

// File descriptors to read/write to the process child
int process_write = -1;
int process_read = -1;

// File descriptors to read/write back to parent, as process child
int parent_write = -1;
int parent_read = -1;

// File descriptors to write to the handler from parent
int handler_write = -1;

// File descriptors to read from the parent as handler
int handler_read = -1;

// File pointer to the output from the process. This is
// associated with the process_read file descriptor
FILE* process_in;

/**
 * Entry point for Scripter
 * Set up the three communication pipes, launch
 * the subprocesses "process" and "handler".
 * Finally, mirror the input/output of "process"
 * by forwarding lines read from stdin to "process"
 * and forward all of the output from "process" to the "handler
 **/
int main(int argc, char **argv) {

    if (argc != 3) {

        printf("Usage: ./scripter <process> <handler>\n");
        exit(1);
    }

    // Set up the file descriptors for all communication
    pipeSetup();

    // Theory: just set up the pipes, then terminate scripter :/
    // if that STILL shuts things down, then I guess we just fork again
    // and. idk. forever loop. :/ bummer

    // Launch the "process"
    int process_pid = forkchild(parent_write, parent_read, argv[1]);
    // int process_pid = forkchild(handler_write, handler_read, argv[1]);
    printf("Process PID: %d\n", process_pid);

    // Launch the "handler"
    int handler_pid = forkchild(process_write, handler_read, argv[2]);
    // int handler_pid = forkchild(process_write, process_read, argv[2]);
    printf("Handler PID: %d\n", handler_pid);

    // Launch the command line reading thread
    pthread_t *thread = malloc(sizeof(pthread_t));
    pthread_create(thread, NULL, &readInput, NULL);

    // Read input from the "process"
    char buffer[BUF_SIZE];
    process_in = fdopen(process_read, "r");
    while (readline(process_in, buffer, BUF_SIZE) != EOF) {

        // Echo output from the process to stdout
        printf("%s\n", buffer); 
     
        // Forward the output to the "handler"
        dprintf(handler_write, "%s\n", buffer); 
    }

}

/*
 * Stop this program. TODO doesn't handle killing children well
 */
//void stop() {
//
//    // TODO use signal handling instead of EOF?
//    printf("Stopping scripter!\n");
//    
//    exit(0);
//}

/**
 * Thread to continuously read input from stdin and
 * print it directly to the "process"
 */
void *readInput(void *ignored) {

    printf("Starting to read forever\n");
    char buffer[BUF_SIZE];
    while (readline(stdin, buffer, BUF_SIZE) != EOF) {
        dprintf(process_write, "%s\n", buffer);
    }
    // stop();
}

/**
 * Set up the three main communication pipes and alias the six endpoints
 * Pipes are described within the function
 */
void pipeSetup() {

    // Holds file descriptors for the communication pipes
    int fdpout[2];  // Communication from process
    int fdpin[2];   // Communication to process
    int fdhin[2];   // Communication to handler
    // No communication FROM handler- that goes to process

    // Create pipes that communicate to/from the process
    pipe(fdpout);
    pipe(fdpin);
    pipe(fdhin);

    // Rename pipe end points
    // PARENT --> PROCESS and HANDLER --> PROCESS
    parent_read   = fdpin[0]; // File descriptor for reading from parent
    process_write = fdpin[1]; // File descriptor for writing to process

    // PROCESS --> PARENT
    process_read = fdpout[0]; // File descriptor for reading from process
    parent_write = fdpout[1]; // File descriptor for writing to parent

    // PARENT --> HANDLER
    // hp_read       = fdhin[0]; // File descriptor for reading from parent
    handler_read  = fdhin[0]; // File descriptor for reading from handler
    handler_write = fdhin[1]; // File descriptor for writing to handler

    /*
     * No HANDLER --> PARENT because handler writes to process
     * using the "to process" pipe, skipping parent
     * Process writes to parent, which then has the job of echoing that
     * to the user and to the handler
     */

}

/* TODO should accept actual runtime arguments
 * Fork a child process that in turn executes another process
 * with a system call. Input and output to the child process are
 * redirected via the given file descriptors.
 *
 * Arguments:
 *  fileDescriptorOut:  File descriptor where stdout is redirected
 *  fileDescriptorIn:   File descriptor where stdin is redirected
 *  path:   Path to the exectuable to be run
 *
 * Return:
 *  Returns the processID of the child back to the parent. Child
 *  process never returns.
 */
int forkchild(int fileDescriptorOut, int fileDescriptorIn, char *path) {

    // TODO Hacky solution to construct argv- we also don't support
    // any actual arguments to the child processes
    char *argv[] = {path, path, (char *) NULL};  

    int pid = fork();
    if (pid == 0) {

        dup2(fileDescriptorIn, STDIN);      // Input to this child
        dup2(fileDescriptorOut, STDOUT);    // Output from this child
        execv(path, argv); // Does not return under normal conditions
        perror("Child process ended unexpectedly\n");
        exit(-1);
    }

    return pid; // Parent, child should never get here
}

// TODO should readline return the number of characters read?

/* precondiction: *line is a pointer to some buffer with enough space for 
 * the next line
 *
 * read a line from stream by:
 * * reading until newline character or EOF, placing characters into line
 * * not including the newline character
 * This function is executed for the side effect of modifying
 * the value pointed to by *line
 *
 * Arguments:
 *      stream: Input stream to read from
 *      line:   Buffer to write the result
 *      buffer_size:    Size of the buffer (including space for null term)
 *
 * returns the last character read, either newline or EOF
 */
int readline(FILE *stream, char *line, int buffer_size) {
    
    int count = 0;
    int character;

    do {
        if (count > buffer_size) {
            fprintf(stderr, 
                    "Attempted to read a size larger than the buffer allows");
            break;
        }
        character = fgetc(stream);
        line[count++] = character;

    } while (character != '\n' && character != EOF);
    line[count-1] = 0;

    return character;
}
