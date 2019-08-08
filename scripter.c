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

int handler_write = -1;
int handler_read = -1;

int main(int argc, char **argv) {

    if (argc != 3) {

        printf("Usage: ./scripter <process> <handler>\n");
        exit(1);
    }

    // gee should I TODO make this a struct
    // Holds file descriptors for the communication pipes
    int fdpout[2];    // Communications to <process> (FileDescriptorProcessOUT)
    int fdpin[2];

    // Create pipes that communicate to/from the process
    pipe(fdpout);
    pipe(fdpin);

    // Rename pipe variables because confusing
    process_write = fdpin[1];   // File descriptor for everytime parent writes to child
    process_read = fdpout[0];   // File descriptor for everytime parent reads from child

    parent_write = fdpout[1];   // File descriptor for writing back to parent
    parent_read = fdpin[0];     // File descriptor for reading from the parent

    int pid = fork();
    if (pid == 0) { // the child, which becomes the "process" scripted for
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

    }
    else { // the main scripter process

        char buffer[BUF_SIZE]; // Text from the process
        char result[BUF_SIZE]; // Response from the handler
        FILE *inputstream = fdopen(process_read, "r");  // Get a file pointer from child output

        close(parent_write);   //parent won't read/write to itself
        close(parent_read);
        
        while (readline(inputstream, buffer) != EOF) { 
            printf("%s\n", buffer);     // Echo the output of the process

            // Construct the process start arguments
            char *argHandler[] = {argv[2], argv[2], buffer, (char *) NULL}; 

            // Launch the handler to handle the output
            handle(buffer, result, argHandler, pid);

            // Print the result of the handler back to the main process
            if (strnlen(result, BUF_SIZE) > 0) {
                dprintf(process_write, "%s\n", result);
            }

        }
        printf("\n----------------------------\
                \nProcess terminated, good bye\
                \n----------------------------\n");

        fclose(inputstream);
    }
}

/**
 * Launches the handling process
 *
 * Args
 *  input-  The line of input that caused this invocation (unused)
 *  result- Output of the handler process
 *  args-   Launch arguments for the handler
 *  pid-    Process ID for the caller of this function (unused)
 */
void handle(char *input, char *result, char *args[], int pid) {

    int fdhandler[2];
    pipe(fdhandler); // pipe for child process output

    printf("Sending to handler: %s\n", args[2]);
       
    if (fork() == 0) { // Child process execution

                                // direct the handler's stdout to the 
                                // write-in of the pipe
        dup2(fdhandler[1], STDOUT); 
        
        close(fdhandler[0]);    // we don't need to read from the handler
                                // parent will do that

        execv(args[0], args);   // Launch the handler
        exit(0);                // Close this child process, no longer needed
    }

    // Result stream takes input from the handler process
    FILE *resultStream = fdopen(fdhandler[0], "r");

    // We do not write to the child process, so we close that end of the pipe
    // This also forces output from the handler to flush to us immediately
    // (if we kept the write end open, once the handler closes its side the 
    //  pipe assumes it will still get input)
    close(fdhandler[1]);

    // Buffer to hold each line of output from the handler
    char line[BUF_SIZE];

    // Read all of the output from the handler for given input
    readline(resultStream, line);
    /*
    while (readline(resultStream, line) != EOF) { 

        if (strnlen(line, BUF_SIZE) > 0) {
           
            // Send the output from the handler back to the process
            dprintf(process_write, "%s\n", line);
        }
    }*/

    close(fdhandler[0]); // close the read end of the pipe
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
