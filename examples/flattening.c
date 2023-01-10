#include "stdio.h"
#include "stdlib.h"

int main() {
    time_t t;
    srand((unsigned) time(&t));

    int n = rand();
    printf("n = %d\n", n);

    if (n < 10)
        printf("n is smaller than 10\n");
    else if (n == 10)
        printf("n is equal to 10\n");
    else
        printf("n is bigger than 10\n");

    return 0;
}