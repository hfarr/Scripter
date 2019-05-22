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

        dup2(fdpin[0], STDIN);          // standard in is now pipe read?

        // this closes fdpin[0] which is not what we want
        // dup2(STDIN, fdpin[0]);      // send pipe to stdin

        close(fdpout[0]);   // child will not read from the output

        // close(fdpin[1]);    // child will not write to the input pipe
        
        /**
        outStream = popen(argv[1], "w");
        printf("Started process\n");
        dup2(fileno(outStream), STDIN);      
        **/

        //char *argpist[] = {"/usr/bin/python3", "womp.py"};
        char *argpist[] = {argv[1], argv[1], (char *) NULL};
        execv(argpist[0], argpist);

    }
    else { // the main scripter process

        char buffer[BUF_SIZE]; // meh?
        char result[BUF_SIZE];
        FILE *inputstream = fdopen(fdpout[0], "r");
        FILE *outputstream = fdopen(fdpin[1], "a");

        close(fdpout[1]);   // we won't write to the output pipe
        close(fdpin[0]);    // we won't read from the input pipe
        
        while (readline(inputstream, buffer) != EOF) { 

            printf("Read a line\n");

            char *argHandler[] = {argv[2], argv[2], (char *) NULL}; 

            handle(buffer, result, argHandler, pid);

            fprintf(stdout, "RESULT: %s\n", result);
            dprintf(fdpin[1], "%s\n", result);
            //fprintf(outputstream, "%s\n", result); // response from handler
            printf("Message sent. (1)\n");
            //fflush(outputstream);


        }
        printf("Process terminated, good bye\n");

        fclose(inputstream);
    }
/*
    if (fdpin[0]) {
        //FILE *outS = fdopen(fdpin[1], "a");
        dprintf(fdpin[1], "Hello there!\n");
    }
*/
    /**
    if (outStream) {
        printf("well %p\n", outStream);
        fprintf(outStream, "Hello world!\n");
        //fflush(outputstream);

    }
    **/

    //close(fdpin[0]);
    //close(fdpin[1]);
}

// Spawn the handling process
void handle(char *input, char *result, char *args[], int pid) {

    int fdhandler[2];
    pipe(fdhandler); // pipe for child process output
       
    if (fork() == 0) { // Child process execution
        printf("FROM PROCESS: %s\n", input);
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
// read a line from STREAM
// reads until the newline character or EOF,
// does not include the newline character.
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
