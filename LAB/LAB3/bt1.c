#include <signal.h>
#include <stdio.h>
#include <unistd.h>

int loop_forever = 1;

void on_sigint()
{
    printf("\ncount.sh has stopped! Goodbye!\n");
    loop_forever = 0;
}

int main()
{
    signal(SIGINT, on_sigint);
    printf("Welcome to IT007, I am 23520161!\n");
    if (fork())
        while (loop_forever)
            ;
    else
        execl("./count.sh", "./count.sh", "120", NULL);
    return 0;
}
