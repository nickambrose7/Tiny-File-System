#include <stdio.h>
#include <stdlib.h>

int main() {
    char *buffer = malloc(10); // Allocate 10 bytes

    // Create an unaligned pointer to an int
    // Assuming sizeof(int) is 4, buffer + 1 is likely unaligned for an int
    int *unaligned_ptr = (int *)(buffer + 1);

    // Attempt to write to the unaligned pointer
    *unaligned_ptr = 123;

    // Attempt to read from the unaligned pointer
    int value = *unaligned_ptr;

    printf("Value: %d\n", value);

    free(buffer);
    return 0;
}