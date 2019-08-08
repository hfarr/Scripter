/**
 *
 * scripter.c
 *
 * This program launches another program (called the process) and
 * monitors its output (from standard out). Each line of output triggers 
 * a script which takes that line of output as one of its arguments.
 * That script (called the handler) processes the output, doing 
 * any number of things. The handler can optionally print its own output
 * which is then sent back to the process.
 *
 * Usage:
 * ./scripter <arguments to launch process> <arguments to launch handler>
 *
 * It is recommended to use quotation marks, particularly if either
 * the "process" or "handler" have arguments (the handler almost certainly
 * will, as that is how information is fed into it currently).
 *
 * An additional recommendation is to use shell scripts that will in turn
 * call other processes, so the shell scripts can be modified with impunity
 * leaving launch arguments for scripter simplified (just call it with
 * two scripts).
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define STDIN 0
#define STDOUT 1
#define STDERR 2

// make this an environment variable?
#define BUF_SIZE 2048

// Method primitives... is that what these are called? fail vocab check
int readline(FILE *stream, char* line);
void handle(char* input, char* result, char *args[], int pid);

// Read/write to the process child
int process_write = -1;
int process_read = -1;

// Read/write back to parent, as process child
int parent_write = -1;
int parent_read = -1;

// Write to the handler from parent
int handler_write = -1;

// Read from the parent as handler
int hp_read = -1;

int main(int argc, char **argv) {

    if (argc != 3) {

        printf("Usage: ./scripter <process> <handler>\n");
        exit(1);
    }

    // gee should I TODO make this a struct
    // Holds file descriptors for the communication pipes
    int fdpout[2];  // Communication from <process> (FileDescriptorProcessOUT)
    int fdpin[2];   // Communication to <process>

    int fdhin[2];   // Communication to handler

    // Create pipes that communicate to/from the process
    pipe(fdpout);
    pipe(fdpin);
    pipe(fdhin);

    // Rename pipe variables because confusing
    parent_read     = fdpin[0]; // File descriptor for reading from the parent
    process_write   = fdpin[1]; // File descriptor for everytime parent writes to child

    process_read    = fdpout[0];    // File descriptor for everytime parent reads from child
    parent_write    = fdpout[1];    // File descriptor for writing back to parent

    hp_read         = fdhin[0]; // File descriptor for reading from parent
    handler_write   = fdhin[1]; // File descriptor for writing to handler

    int process_pid = fork();
    if (process_pid == 0) { // the child, which becomes the "process" scripted for
        printf("\n-------------------\
                \nProcess starting...\
                \n-------------------\n");

        dup2(parent_write, STDOUT);    // send stdout to the parent
        dup2(parent_write, STDERR);    // send stdout to the parent
        dup2(parent_read, STDIN);      // read everything from parent as if it came from stdin

        close(process_read);    // child will not read/write to itself
        close(process_write);
        
        // Execute the 'process' specified in argv[1]
        char *argpist[] = {argv[1], argv[1], (char *) NULL};
        execv(argpist[0], argpist);
        perror("Process errored unexpectedly\n");
        exit(-1);
    }
    printf("Process PID: %d\n", process_pid);

    int handler_pid = fork();
    if (handler_pid == 0) {
        printf("\n-------------------\
                \nHandler starting...\
                \n-------------------\n");

        dup2(process_write, STDOUT);    // Output from handler goes to process
        dup2(hp_read, STDIN);      // Input to handler comes from parent

        char *arglist[] = {argv[2], argv[2], (char *) NULL};
        execv(arglist[0], arglist);
        perror("Handler errored unexpectedly\n");
        exit(-1);
    }
    printf("Child PID: %d\n", handler_pid);

    dprintf(process_write, "Whatup\n");

    // the main scripter process
    dup2(process_read, STDIN);

    char buffer[BUF_SIZE];
    while (readline(stdin, buffer) != EOF) {
        printf("%s\n", buffer);
        dprintf(handler_write, "%s\n", buffer);
    }

}

// TODO should readline return the number of characters read?

// precondiction: *line is a pointer to some buffer with enough space for 
// the next line
//
// read a line from stream by:
// * reading until newline character or EOF, placing characters into line
// * not including the newline character
//
// returns the last character read, either newline or EOF
int readline(FILE *stream, char *line) {
    // when you get an EOF it should still put stuff into the line, right?
    
    int count = 0;
    int character;

    do {
        character = fgetc(stream);
        line[count++] = character;
    } while (character != '\n' && character != EOF);
    line[count-1] = 0;

    return character;
}
