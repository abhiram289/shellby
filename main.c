#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <signal.h>

#define MAX_INPUT 1024
#define MAX_ARGS 64

// tokenize a string into args[]
void parse_args(char *cmd, char *args[])
{
    int i = 0;
    char *token = strtok(cmd, " ");

    while (token != NULL && i < MAX_ARGS - 1)
    { // -1 cause final NULL
        args[i++] = token;
        token = strtok(NULL, " ");
    }

    args[i] = NULL;
}

// trims spaces at the start of the command (if entered)
char *trim_left(char *s)
{
    while (*s == ' ')
        s++;
    return s;
}

void handle_redirection(char *args[], char **infile, char **outfile, int *append)
{
    *infile = NULL;
    *outfile = NULL;

    for (int i = 0; args[i] != NULL; i++)
    {
        if (strcmp(args[i], "<") == 0)
        {
            if (args[i + 1] != NULL)
            {
                *infile = args[i + 1];
                args[i] = NULL;
            }
            break;
        }

        if (strcmp(args[i], ">>") == 0)
        {
            if (args[i + 1] != NULL)
            {
                *outfile = args[i + 1];
                *append = 1;
                args[i] = NULL;
            }
            break;
        }

        if (strcmp(args[i], ">") == 0)
        {
            if (args[i + 1] != NULL)
            {
                *outfile = args[i + 1];
                args[i] = NULL;
            }
            break;
        }
    }
}

void handle_sigint(int sig)
{
    write(STDOUT_FILENO, "\nmyshell> ", 10);
}

void handle_sigchld(int sig)
{
    while (waitpid(-1, NULL, WNOHANG) > 0);
}


int main()
{
    char input[MAX_INPUT];
    signal(SIGINT, handle_sigint);
    signal(SIGCHLD, handle_sigchld);


    while (1)
    {
        printf("myshell> ");
        // prevents buffering
        fflush(stdout);

        // reads line
        if (!fgets(input, MAX_INPUT, stdin))
        {
            break;
        }

        // removes "\n"
        input[strcspn(input, "\n")] = 0;

        if (strlen(input) == 0)
        {
            continue;
        }

        // built-in exit
        if (strcmp(input, "exit") == 0)
        {
            break;
        }

        // checks if pipe exists, strchr searches for "|"
        char *pipe_pos = strchr(input, '|');

        if (pipe_pos != NULL)
        {
            // split into left and right commands
            *pipe_pos = '\0';
            char *left_cmd = trim_left(input);
            char *right_cmd = trim_left(pipe_pos + 1);

            // an array each for left and right parts
            char *left_args[MAX_ARGS];
            char *right_args[MAX_ARGS];

            parse_args(left_cmd, left_args);
            parse_args(right_cmd, right_args);

            int fd[2];
            if (pipe(fd) == -1)
            {
                perror("pipe");
                continue;
            }

            pid_t pid1 = fork();
            if (pid1 == 0)
            {
                // 1st child - runs left command and sends o/p goes to pipe
                signal(SIGINT, SIG_DFL);

                dup2(fd[1], STDOUT_FILENO);
                close(fd[0]);
                close(fd[1]);

                execvp(left_args[0], left_args);
                perror("execvp left");
                exit(1);
            }

            pid_t pid2 = fork();
            if (pid2 == 0)
            {
                // 2nd child - runs right command and i/p comes from pipe
                // child2 reads output by child1
                signal(SIGINT, SIG_DFL);

                dup2(fd[0], STDIN_FILENO);
                close(fd[1]);
                close(fd[0]);

                execvp(right_args[0], right_args);
                perror("execvp right");
                exit(1);
            }

            // parent closes both ends of the pipe
            close(fd[0]);
            close(fd[1]);

            // waits for children to finsih
            waitpid(pid1, NULL, 0);
            waitpid(pid2, NULL, 0);

            continue;
        }

        // if no pipe -  normal single command execution
        char temp[MAX_INPUT];
        strcpy(temp, input);

        char *args[MAX_ARGS];
        parse_args(temp, args);

        int background = 0;
        for (int i = 0; args[i] != NULL; i++)
        {
            if (strcmp(args[i], "&") == 0)
            {
                background = 1;
                args[i] = NULL;
                break;
            }
        }

        char *infile = NULL;
        char *outfile = NULL;

        int append = 0;

        handle_redirection(args, &infile, &outfile, &append);

        // Built-in cd
        if (strcmp(args[0], "cd") == 0)
        {
            if (args[1] == NULL)
            {
                fprintf(stderr, "myshell: expected argument to \"cd\"\n");
            }
            else
            {
                if (chdir(args[1]) != 0)
                {
                    perror("myshell");
                }
            }
            continue;
        }

        pid_t pid = fork();

        if (pid == 0)
        {
            signal(SIGINT, SIG_DFL);

            if (infile != NULL)
            {
                int fd_in = open(infile, O_RDONLY);
                if (fd_in < 0)
                {
                    perror("input redirection");
                    exit(1);
                }
                dup2(fd_in, STDIN_FILENO);
                close(fd_in);
            }

            if (outfile != NULL)
            {
                int fd_out;
                if (append)
                {
                    fd_out = open(outfile, O_WRONLY | O_CREAT | O_APPEND, 0644);
                }
                else
                {
                    fd_out = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                }

                if (fd_out < 0)
                {
                    perror("output redirection");
                    exit(1);
                }
                dup2(fd_out, STDOUT_FILENO);
                close(fd_out);
            }

            execvp(args[0], args);
            perror("execvp");
            exit(1);
        }
        else if (pid < 0)
        {
            perror("fork");
        }
        else
        {
            if (!background)
            {
                wait(NULL);
            }
        }
    }

    return 0;
}
