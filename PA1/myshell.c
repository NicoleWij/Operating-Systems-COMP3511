/*
    COMP3511 Fall 2022
    PA1: Simplified Linux Shell (MyShell)

    Your name: Nicole Wijkman
    Your ITSC email: nwijkman@connect.ust.hk

    Declaration:

    I declare that I am not involved in plagiarism
    I understand that both parties (i.e., students providing the codes and students copying the codes) will receive 0 marks.

*/

// Note: Necessary header files are included
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h> // For constants that are required in open/read/write/close syscalls
#include <sys/wait.h> // For wait() - suppress warning messages
#include <fcntl.h>    // For open/read/write/close syscalls

// Assume that each command line has at most 256 characters (including NULL)
#define MAX_CMDLINE_LEN 256

// Assume that we have at most 8 arguments
#define MAX_ARGUMENTS 8

// Assume that we only need to support 2 types of space characters:
// " " (space) and "\t" (tab)
#define SPACE_CHARS " \t"

// The pipe character
#define PIPE_CHAR "|"

// Assume that we only have at most 8 pipe segements,
// and each segment has at most 256 characters
#define MAX_PIPE_SEGMENTS 8

// Assume that we have at most 8 arguments for each segment
//
// We also need to add an extra NULL item to be used in execvp
//
// Thus: 8 + 1 = 9
//
// Example:
//   echo a1 a2 a3 a4 a5 a6 a7
//
// execvp system call needs to store an extra NULL to represent the end of the parameter list
//
//   char *arguments[MAX_ARGUMENTS_PER_SEGMENT];
//
//   strings stored in the array: echo a1 a2 a3 a4 a5 a6 a7 NULL
//
#define MAX_ARGUMENTS_PER_SEGMENT 9

// Define the  Standard file descriptors here
#define STDIN_FILENO 0  // Standard input
#define STDOUT_FILENO 1 // Standard output

// This function will be invoked by main()
// TODO: Implement the multi-level pipes below
void process_cmd(char *cmdline);

// read_tokens function is given
// This function helps you parse the command line
// Note: Before calling execvp, please remember to add NULL as the last item
void read_tokens(char **argv, char *line, int *numTokens, char *token);

/* The main function implementation */
int main()
{
    char cmdline[MAX_CMDLINE_LEN];
    fgets(cmdline, MAX_CMDLINE_LEN, stdin);
    process_cmd(cmdline);
    return 0;
}

// TODO: implementation of process_cmd
void process_cmd(char *cmdline)
{
    char *pipe_segments[MAX_PIPE_SEGMENTS]; // character array buffer to store the pipe segements
    int num_pipe_segments;                  // an output integer to store the number of pipe segment parsed by this function
    int i, j, k;
    int fd;
    int pfds[2];
    char *arguments[MAX_ARGUMENTS_PER_SEGMENT] = {NULL};
    int num_arguments;
    // strcpy(cmdline, "ls | sort -r | sort | sort -r | sort | sort -r | sort | sort -r");
    read_tokens(pipe_segments, cmdline, &num_pipe_segments, PIPE_CHAR);

    for (i = 0; i < num_pipe_segments; i++)
    {
        read_tokens(arguments, pipe_segments[i], &num_arguments, SPACE_CHARS);

        for (j = 0; j < num_arguments; j++)
        {
            if (*arguments[j] == '<') // deals with reading reading from a specified text file
            {
                arguments[j] = NULL; // removes the '<'

                fd = open(arguments[j + 1],   // input file name will be the next argument
                          O_RDONLY,           // flag
                          S_IRUSR | S_IWUSR); // user permission: 600
                close(STDIN_FILENO);
                dup2(fd, STDIN_FILENO);
                close(fd);
            }
            else if (*arguments[j] == '>') // deals with writing to a specified text file
            {
                arguments[j] = NULL; // removes the '>'

                fd = open(arguments[j + 1],   // output file name will be the next argument
                          O_CREAT | O_WRONLY, // flags
                          S_IRUSR | S_IWUSR); // user permission: 600
                close(STDOUT_FILENO);
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }
        }

        arguments[num_arguments] = NULL; // adds NULL as the last item

        if (num_pipe_segments == 1)
        {
            execvp(arguments[0], arguments);
        }
        else if ((i + 1) != num_pipe_segments) // runs until the loop has iterated through all segments
        {
            pipe(pfds);
            pid_t pid = fork(); // 0 = child, non-zero = parent

            if (pid == 0) // The child process
            {
                close(STDOUT_FILENO);         // close stdout
                dup2(pfds[1], STDOUT_FILENO); // put stdout as the pipe input
                close(pfds[1]);               // dup to STDOUT
                close(pfds[0]);               // close input

                execvp(arguments[0], arguments); // execute current commands
            }
            else // The parent process
            {
                close(STDIN_FILENO);         // close stdin
                dup2(pfds[0], STDIN_FILENO); // put stdin as the pipe output
                close(pfds[1]);              // close output
                wait(0);                     // parent waits for child to finish
            }
        }
        else
        {
            execvp(arguments[0], arguments); // final execute when the loop has iterated through all segments
        }
    }
}

// Implementation of read_tokens function
void read_tokens(char **argv, char *line, int *numTokens, char *delimiter)
{
    int argc = 0;
    char *token = strtok(line, delimiter);
    while (token != NULL)
    {
        argv[argc++] = token;
        token = strtok(NULL, delimiter);
    }
    *numTokens = argc;
}