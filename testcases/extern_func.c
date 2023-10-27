#include <stdio.h>
#include <stdlib.h>

int GET()
{
    int64_t x;
    fprintf(stderr, "Please Input an Integer Value : ");
    scanf("%ld", &x);
    return x;
}

void * MALLOC(int size)
{
    return malloc(size);
}

void FREE(void *p)
{
    free(p);
}

void PRINT(int x)
{
    printf("%d", x);
}