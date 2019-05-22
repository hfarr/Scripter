#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define STDIN 0
#define STDOUT 1
#define STDERR 2

#define BUF_SIZE 2048

//int startProcess(char *args, int myIn, int myOut);
//
int readline(FILE *stream, char* line);

void handle(char* input, char *result, char *args[], int pid);

int main(int argc, char **argv) {

    if (argc != 3) {

        printf("Usage: ./scripter <process> <handler>\n");
        exit(1);
    }

    int fdpout[2];    // Communications to <process>
    int fdpin[2];

    // Create communication pipes
    pipe(fdpout);
    pipe(fdpin);

    FILE *outStream;

    int pid = fork();
    if (pid == 0) { // the "process"
        dup2(fdpout[1], STDOUT);    // send stdout to the pipe write
        dup2(fdpout[1], STDERR);    // send stdout to the pipe write
        dup2(fdpin[0], STDIN);      // pipe read end now means child stdin

        close(fdpout[0]);   // child will not read from the output
        close(fdpin[1]);    // child will not write to the input pipe
        
        // Execute the 'process' specified in argv[1]
        char *argpist[] = {argv[1], argv[1], (char *) NULL};
        execv(argpist[0], argpist);

    }
    else { // the main scripter process

        char buffer[BUF_SIZE]; // Text from the process
        char result[BUF_SIZE]; // Response from the handler
        FILE *inputstream = fdopen(fdpout[0], "r");

        close(fdpout[1]);   // parent won't write to the output pipe
        close(fdpin[0]);    // parent won't read from the input pipe
        
        while (readline(inputstream, buffer) != EOF) { 
            printf("%s\n", buffer);     // Echo the output of the process

            // Construct the process start arguments
            char *argHandler[] = {argv[2], argv[2], buffer, (char *) NULL}; 
            handle(buffer, result, argHandler, pid);

            // fprintf(stdout, "RESULT: %s\n", result);
            dprintf(fdpin[1], "%s\n", result);

        }
        printf("\n----------------------------\
                \nProcess terminated, good bye\
                \n----------------------------\n");

        fclose(inputstream);
    }
}

// Spawn the handling process
void handle(char *input, char *result, char *args[], int pid) {

    int fdhandler[2];
    pipe(fdhandler); // pipe for child process output
       
    if (fork() == 0) { // Child process execution
        dup2(fdhandler[1], STDOUT); // pipe stdout to the write-in of the pipe
        
        close(fdhandler[0]); // we don't need to write to the pipe in the child

        execv(args[0], args);
        exit(0);
    }

    FILE *resultStream = fdopen(fdhandler[0], "r");
    close(fdhandler[1]); // close the write end of the pipe in parent
                         // this forces the output to flush, otherwise we'd
                         // have to wait for the pipe to fill up entirely

    readline(resultStream, result); // Only read one line

    close(fdhandler[0]); // close the read end of the pipe
}

// precondiction: *line is a pointer to some buffer with enough space for 
// the next line
//
// read a line from stream by:
// * reading until newline character or EOF, placing characters into line
// * not including the newline character
//
// returns the last character read, either newline or EOF
int readline(FILE *stream, char *line) {
    
    int count = 0;
    int character;

    do {
        character = fgetc(stream);
        line[count++] = character;
    } while (character != '\n' && character != EOF);
    line[count-1] = 0;

    return character;
}
