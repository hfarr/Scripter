#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define STDIN 0
#define STDOUT 1
#define STDERR 2

//int startProcess(char *args, int myIn, int myOut);
//
int readline(FILE *stream, char* line);

void handle(char* input, FILE *output, char *args[]);

int main(int argc, char **argv) {

    if (argc != 3) {

        printf("Usage: ./scripter <process> <handler>\n");
        exit(1);
    }

    printf("Running: %s\nRead by: %s\n", *(argv+1), *(argv+2));

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
        FILE *inputstream = fdopen(fdprocess[0], "r");
        FILE *outputstream = fdopen(fdprocess[1], "a");
        close(fdprocess[1]);
        close(fdprocess[1]);

        while (readline(inputstream, buffer) != EOF) { 
            char *arggggs[] = {argv[2], argv[2]}; // I am lazy TODO
            handle(buffer, outputstream, arggggs);
        }

        fclose(inputstream);
        fclose(outputstream);
    }

    //char *argpist[] = {"/usr/bin/python3", "-c", "print\\(4+4\\)"};
    //char *argpist[] = {"/usr/bin/java", "main"};
    //char *argpist[] = {"/usr/bin/python3", "womp.py"};

    //execv(argpist[0], argpist);

}

// Spawn the handling process
void handle(char* input, FILE *output, char *args[]) {
    if (fork() == 0) {
        printf("INPUT: %s\n", input);
        execv(args[0], args);
    }
}

// precondiction: *line is a pointer to some buffer with enough space for the next line
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
