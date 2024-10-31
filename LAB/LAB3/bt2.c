#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main()
{

    execlp("cat", "", "MSSV.txt", NULL);
    return 0;
}
