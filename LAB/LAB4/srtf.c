#include <stdio.h>
#include <stdlib.h>

typedef enum
{
    SB_PID,
    SB_ARRVIAL,
    SB_BURST,
    SB_REMAIN,
    SB_START
} SORT_BY;

typedef struct
{
    FILE *in;
    FILE *out;
} Files;

typedef struct
{
    int iPID;
    int iArrival, iBurst;
    int iStart, iFinish, iWaiting, iResponse, iTaT, iRemain;
} PCB;

typedef struct
{
    int n;
    PCB *processes;
} ArrPCB;

Files openFiles();
void closeFiles(Files *);
ArrPCB initArrPCB(int);
ArrPCB inputFromFile(FILE *);
void swapProcess(PCB *, PCB *);
int selectByiCriteria(PCB[], int, SORT_BY);
int partition(PCB[], int, int, SORT_BY);
void quickSort(PCB *, int, int, SORT_BY);
void sort(ArrPCB, SORT_BY);
void pushProcess(ArrPCB *, PCB);
void removeProcess(ArrPCB *, int);
void increaseWaitingTForOther(ArrPCB, int);
float calcAvgWaitingT(ArrPCB);
float calcAvgTaT(ArrPCB);
void outputToFile(FILE *, ArrPCB);
void freeResources(ArrPCB *);

int main()
{
    Files files = openFiles();

    ArrPCB input = inputFromFile(files.in);
    ArrPCB readyQueue = initArrPCB(input.n);
    ArrPCB doneProcesses = initArrPCB(input.n);

    sort(input, SB_ARRVIAL);

    int currentTime;

    while (readyQueue.n > 0 || input.n > 0)
    {
        // TH readyQueue co process
        if (readyQueue.n > 0)
        {
            PCB *currentP = &readyQueue.processes[0];

            // Gan iStart cho currentP neu currentP duoc chay lan dau tiem
            if (currentP->iRemain == currentP->iBurst)
            {
                currentP->iStart = currentTime;
                currentP->iResponse = currentTime - currentP->iArrival;
            }

            // Neu input con process va iArrival cua process do < thoi diem ket thuc du kien cua currentP
            // Voi: Thoi diem ket thuc du tinh = iRemain + currentTime
            if (input.n > 0 && input.processes[0].iArrival < currentP->iRemain + currentTime)
            {
                // Tam ngung currentP
                int pausedTime = input.processes[0].iArrival;
                int elapsedTime = pausedTime - currentTime;
                currentP->iRemain -= elapsedTime;

                // Tang thoi gian waiting cua cac process con lai trong readyQueue (tru currentP)
                increaseWaitingTForOther(readyQueue, elapsedTime);

                // Them cac process(es) co cung thoi diem xuat hien vao readyQueue
                while (input.n > 0 && input.processes[0].iArrival == pausedTime)
                {
                    pushProcess(&readyQueue, input.processes[0]);
                    removeProcess(&input, 0);
                }

                sort(readyQueue, SB_REMAIN);

                // Cap nhat currentTime = thoi diem process(es) moi xuat hien
                currentTime = pausedTime;
            }
            // Truong hop khong co process nao xuat hien trong qua trinh chay den thoi diem ket thuc du kien cua
            // currentP
            // Tuc currentP se chay den het va hoan thanh
            else
            {
                // Cap nhat currentTime
                currentTime += currentP->iRemain;

                // Tang thoi gian cho cua cac process con lai trong readyQueue (tru currentP)
                increaseWaitingTForOther(readyQueue, currentP->iRemain);

                // Tinh toan cac truong du lieu cho currentP
                currentP->iFinish = currentTime;
                currentP->iTaT = currentP->iFinish - currentP->iArrival;
                currentP->iRemain = 0;

                // Chuyen currentP tu readyQueue sang doneProcesses
                pushProcess(&doneProcesses, *currentP);
                removeProcess(&readyQueue, 0);
            }
        }
        // TH readyQueue khong co process nao, va trong input van con process
        else
        {
            currentTime = input.processes[0].iArrival;

            // Them cac process(es) co cung thoi diem xuat hien vao readyQueue
            while (input.n > 0 && input.processes[0].iArrival == currentTime)
            {
                pushProcess(&readyQueue, input.processes[0]);
                removeProcess(&input, 0);
            }

            sort(readyQueue, SB_REMAIN);
        }
    }

    // Sort lai doneProcesses theo PID de output theo thu tu PID
    sort(doneProcesses, SB_PID);

    outputToFile(files.out, doneProcesses);

    freeResources(&input);
    freeResources(&readyQueue);
    freeResources(&doneProcesses);

    closeFiles(&files);

    return 0;
}

Files openFiles()
{
    Files files;
    files.in = fopen("input.txt", "r");
    if (files.in == NULL)
    {
        printf("Opening \"input.txt\" failed!");
        exit(1);
    }
    files.out = fopen("output.txt", "w");
    if (files.out == NULL)
    {
        printf("Opening \"output.txt\" failed!");
        exit(1);
    }
    return files;
}

void closeFiles(Files *files)
{
    fclose(files->in);
    fclose(files->out);
}

ArrPCB initArrPCB(int numberOfProcess)
{
    return (ArrPCB){0, (PCB *)malloc(sizeof(PCB[numberOfProcess]))};
}

ArrPCB inputFromFile(FILE *in)
{
    int n;
    fscanf(in, "%d", &n);
    ArrPCB arrPCB = initArrPCB(n);
    arrPCB.n = n;
    for (int i = 0; i < n; i++)
    {
        fscanf(in, "%d%d%d", &arrPCB.processes[i].iPID, &arrPCB.processes[i].iArrival, &arrPCB.processes[i].iBurst);
        arrPCB.processes[i].iRemain = arrPCB.processes[i].iBurst;
    }
    return arrPCB;
}

void swapProcess(PCB *P, PCB *Q)
{
    PCB temp = *P;
    *P = *Q;
    *Q = temp;
}

int selectByiCriteria(PCB P[], int index, SORT_BY iCriteria)
{
    switch (iCriteria)
    {
    case SB_ARRVIAL:
        return P[index].iArrival;
    case SB_PID:
        return P[index].iPID;
    case SB_BURST:
        return P[index].iBurst;
    case SB_REMAIN:
        return P[index].iRemain;
    case SB_START:
        return P[index].iStart;
    default:
        exit(1);
    }
}

int partition(PCB P[], int low, int high, SORT_BY iCriteria)
{
    int pivot = selectByiCriteria(P, high, iCriteria);
    int i = low - 1;
    for (int j = low; j < high; j++)
    {
        int currentValue = selectByiCriteria(P, j, iCriteria);
        if (currentValue <= pivot)
        {
            i++;
            swapProcess(&P[i], &P[j]);
        }
    }
    swapProcess(&P[i + 1], &P[high]);
    return i + 1;
}

void quickSort(PCB *P, int low, int high, SORT_BY iCriteria)
{
    if (low < high)
    {
        int pi = partition(P, low, high, iCriteria);
        quickSort(P, low, pi - 1, iCriteria);
        quickSort(P, pi + 1, high, iCriteria);
    }
}

void sort(ArrPCB arrPCB, SORT_BY iCriteria)
{
    quickSort(arrPCB.processes, 0, arrPCB.n - 1, iCriteria);
}

void pushProcess(ArrPCB *arr, PCB P)
{
    arr->processes[arr->n] = P;
    arr->n++;
}

void removeProcess(ArrPCB *arr, int index)
{
    for (int i = index; i < arr->n - 1; i++)
    {
        arr->processes[i] = arr->processes[i + 1];
    }
    arr->n--;
}

void increaseWaitingTForOther(ArrPCB arr, int time)
{
    for (int i = 1; i < arr.n; i++)
    {
        arr.processes[i].iWaiting += time;
    }
}

float calcAvgWaitingT(ArrPCB arr)
{
    int total = 0;
    for (int i = 0; i < arr.n; i++)
    {
        total += arr.processes[i].iWaiting;
    }
    return (float)total / arr.n;
}

float calcAvgTaT(ArrPCB arr)
{
    int total = 0;
    for (int i = 0; i < arr.n; i++)
    {
        total += arr.processes[i].iTaT;
    }
    return (float)total / arr.n;
}

void outputToFile(FILE *out, ArrPCB arr)
{
    for (int i = 0; i < arr.n; i++)
    {
        PCB P = arr.processes[i];
        fprintf(out, "%d %d %d %d\n", P.iPID, P.iResponse, P.iWaiting, P.iTaT);
    }
    fprintf(out, "%.2f\n", calcAvgWaitingT(arr));
    fprintf(out, "%.2f\n", calcAvgTaT(arr));
}

void freeResources(ArrPCB *arrPCB)
{
    free(arrPCB->processes);
}
