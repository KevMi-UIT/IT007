#include <ctype.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#define MAX_CMD_LEN 1024
#define MAX_ARGS 128
#define HISTORY_SIZE 10

// global
int subprocessID = -1;
char PROMPT[] = "it007sh>";

// Command history
char history[HISTORY_SIZE][MAX_CMD_LEN];
int history_count = 0;

// UTILS to trim string
void trim_whitespace(char str[])
{
    int length = strlen(str);
    if (length <= 0)
    {
        return;
    }

    int start = 0;
    while (start < length && isspace(str[start]))
    {
        start++;
    }

    int end = length - 1;
    while (end >= 0 && isspace(str[end]))
    {
        end--;
    }

    int i = 0;
    while (start <= end)
    {
        str[i++] = str[start++];
    }
    str[i] = '\0';
}

void trim_parentheses(char str[])
{
    if (str[0] != '(' || str[0] == '\0')
    {
        return;
    }
    int i = 0;
    while (str[i] != ')')
    {
        str[i] = str[i + 1];
        ++i;
    }
    str[i - 1] = '\0';
}

// Original terminal attributes (to restore later)
struct termios orig_termios;

void disable_raw_mode()
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enable_raw_mode()
{
    // No disable due to subprocess can disable it too
    struct termios raw;
    tcgetattr(STDIN_FILENO, &raw);
    raw.c_lflag &= ~(ECHO | ICANON); // Disable echo, canonical mode
    raw.c_cc[VMIN] = 1;              // Minimum number of bytes for read
    raw.c_cc[VTIME] = 0;             // No timeout for read
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void sigint_handler(int sig)
{
    if (subprocessID == -1)
    {
        return;
    }
    kill(subprocessID, SIGINT);
}

void empty_string(char *string, size_t size)
{
    memset(string, 0, size);
}

void reprint_prompt(char cmd[])
{
    printf("\r\033[K%s %s", PROMPT, cmd);
    fflush(stdout);
}

void add_to_history(const char *cmd)
{
    if (strlen(cmd) == 0)
        return;
    for (int i = HISTORY_SIZE - 1; i > 0; --i)
    {
        strncpy(history[i], history[i - 1], MAX_CMD_LEN);
    }
    strncpy(history[0], cmd, MAX_CMD_LEN);
    if (history_count < HISTORY_SIZE)
    {
        history_count++;
    }
}

void execute_command(char *cmd)
{
    char *args[MAX_ARGS];
    char *token = strtok(cmd, " ");
    int i = 0;

    // Parse command into arguments
    while (token != NULL)
    {
        args[i++] = token;
        token = strtok(NULL, " ");
    }
    args[i] = NULL; // Set the last arg is NULL for execvp

    // Handle redirection
    int in_redirect = -1, out_redirect = -1;
    for (int j = 0; j < i; j++)
    {
        if (strcmp(args[j], "<") == 0)
        {
            in_redirect = open(args[j + 1], O_RDONLY);
            if (in_redirect < 0)
            {
                perror("Input redirection error");
                return;
            }
            dup2(in_redirect, STDIN_FILENO); // Copy content of file to stdin for command to exec later
            close(in_redirect);
            args[j] = NULL;
            break;
        }
        else if (strcmp(args[j], ">") == 0)
        {
            // if doesn't exist create with 644 perms or clear data if exist
            out_redirect = open(args[j + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (out_redirect < 0)
            {
                perror("Output redirection error");
                return;
            }
            dup2(out_redirect, STDOUT_FILENO);
            close(out_redirect);
            args[j] = NULL;
            break;
        }
    }

    // Execute command
    execvp(args[0], args);

    // Handle error if cannot replace current process
    perror("Exec failed"); // redirect to stderr with message of the command execcuted above
    exit(1);
}

void handle_pipe(char *cmd)
{
    char *pipe_parts[2];
    pipe_parts[0] = strtok(cmd, "|");
    pipe_parts[1] = strtok(NULL, "|"); // Use NULL for the last prev string

    // fd mean file descriptor
    int pipe_fd[2];
    pipe(pipe_fd);

    pid_t pid = fork();
    if (pid == 0)
    {
        // Child process: write to pipe
        close(pipe_fd[0]);
        dup2(pipe_fd[1], STDOUT_FILENO);
        close(pipe_fd[1]);
        execute_command(pipe_parts[0]);
    }
    else
    {
        // Parent process: read from pipe
        close(pipe_fd[1]);
        dup2(pipe_fd[0], STDIN_FILENO);
        close(pipe_fd[0]);
        wait(NULL);
        execute_command(pipe_parts[1]);
    }
}

void handle_continuous(char *cmd)
{
    trim_parentheses(cmd);
    char *command = strtok(cmd, ";");

    while (command != NULL)
    {
        if (fork() == 0)
        {
            execute_command(command);
        }
        else
        {
            wait(NULL);
            command = strtok(NULL, ";"); // Get the next command
        }
    }
}

void read_command(char *cmd)
{
    int pos = 0; // command position to remember to append or delete to cmd
    int history_index = -1;
    char c;

    printf("%s ", PROMPT);
    fflush(stdout); // Need for immediately print prompt

    while (1)
    {
        c = getchar();

        if (c == '\n')
        {
            cmd[pos] = '\0';
            printf("\n");
            break;
        }

        // Escape sequence
        if (c == '\033')
        {
            // Handle Up / Down arrow
            if (getchar() == '[')
            {
                switch (getchar())
                {
                case 'A': {
                    // Up arrow
                    if (history_index < history_count - 1)
                    {
                        ++history_index;
                        empty_string(cmd, sizeof(cmd));
                        strcpy(cmd, history[history_index]);
                        reprint_prompt(cmd);
                        pos = strlen(cmd);
                    }
                    break;
                }
                case 'B': {
                    // Down arrow
                    if (history_index >= 0)
                    {
                        --history_index;
                        empty_string(cmd, sizeof(cmd));
                        strcpy(cmd, history[history_index]);
                        reprint_prompt(cmd);
                        pos = strlen(cmd);
                    }
                    else
                    {
                        // No history left, return back to new command prompt
                        empty_string(cmd, sizeof(cmd));
                        reprint_prompt(cmd);
                        pos = 0;
                    }
                    break;
                }
                }
            }
        }
        // Check if backspace
        else if (c == 127)
        {
            if (pos == 0)
            {
                continue;
            }
            cmd[--pos] = '\0';
            reprint_prompt(cmd);
        }
        // Max cmd length reached
        else if (pos == MAX_CMD_LEN - 1)
        {
            continue;
        }
        // Regular char
        else
        {
            cmd[pos++] = c;
            reprint_prompt(cmd);
        }
    }
}

int main()
{
    enable_raw_mode();
    signal(SIGINT, sigint_handler); // Listen SIGINT to trigger sigint_handler
    char cmd[MAX_CMD_LEN];

    while (1)
    {
        empty_string(cmd, sizeof(cmd));
        read_command(cmd);
        add_to_history(cmd);
        trim_whitespace(cmd);

        if (strcmp(cmd, "exit") == 0)
        {
            break; // instead of checking should_run
        }

        subprocessID = fork();
        if (subprocessID == 0)
        {
            // Child process
            // Handle pipe
            if (strchr(cmd, '|'))
            {
                handle_pipe(cmd);
            }
            // Handle continuous commands
            else if (strchr(cmd, ';'))
            {
                handle_continuous(cmd);
            }
            else
            {
                execute_command(cmd);
            }
            exit(0); // Exit subprocess
        }
        else
        {
            // Parent process
            wait(NULL);
        }
    }

    // disable_raw_mode(); // Currently disable due to mis alignment
    return 0;
}
