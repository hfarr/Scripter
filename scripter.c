#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define STDIN 0
#define STDOUT 1
#define STDERR 2

//int startProcess(char *args, int myIn, int myOut);
//
int readline(FILE *stream, char* line);

void handle(char* input, char *result, char *args[], int pid);

int main(int argc, char **argv) {

    if (argc != 3) {

        printf("Usage: ./scripter <process> <handler>\n");
        exit(1);
    }

    int fdprocess[2];    // Communications to <process>

    // Create communication pipes
    pipe(fdprocess);

    int pid = fork();
    if (pid == 0) {
        dup2(fdprocess[1], STDOUT);   // send stdout to the pipe
        dup2(fdprocess[1], STDERR);   // send stdout to the pipe
        close(fdprocess[0]);

        printf("Started process\n");

        //char *argpist[] = {"/usr/bin/python3", "womp.py"};
        char *argpist[] = {argv[1], argv[1]};
        execv(argpist[0], argpist);

    }
    else {

        char buffer[2048]; // meh?
        char result[2048];
        FILE *inputstream = fdopen(fdprocess[0], "r");
        FILE *outputstream = fdopen(fdprocess[1], "a");
        close(fdprocess[1]);


        while (readline(inputstream, buffer) != EOF) { 

            // These lines of code work fine here
            /* char *arr[] = {"/bin/echo", "from dawn till dust"};
            int val = execv(arr[0], arr);
            printf("%d\n", val); */


            char *argHandler[] = {argv[2], argv[2]}; 

            // but placed here they do not work
            char *arr[] = {"/bin/echo", "from dawn till dust", (char *) NULL};
            int val = execv(arr[0], arr);
            printf("%d\n", val);


            handle(buffer, result, argHandler, pid);
            printf("RESULT: %s\n", result);
            fprintf(outputstream, "%s\n", result);
        }

        // char *argpist[] = {argv[1], argv[1]};
        // execv(argpist[0], argpist);

        fclose(inputstream);
        fclose(outputstream);
    }
}

// Spawn the handling process
void handle(char *input, char *result, char *args[], int pid) {

    int fdhandler[2];
    pipe(fdhandler);
       
    if (fork() == 0) {
        printf("%s\n", input);
        dup2(fdhandler[1], STDOUT); // pipe stdout to the write-in of the pipe
        
        close(fdhandler[0]); // we don't need to write to the pipe in the child

        // Test that STDOUT is redirected to the pipe
        printf("This should go in the pipe\n");
        printf("This should go in the pipe too\n");

        execv(args[0], args);
        exit(0);
    }

    FILE *resultStream = fdopen(fdhandler[0], "r");
    close(fdhandler[1]); // close the write end of the pipe in parent
                        // this forces the output to flush, otherwise we'd
                        // have to wait for the pipe to fill up entirely

    //readline(resultStream, result); // Only read one line

    // Using a loop to read arbitrary # of lines now
    while (readline(resultStream, result) != EOF) { 
        // reads one line into the result buffer at a time

        printf("blah: %s\n", result);
    }

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
