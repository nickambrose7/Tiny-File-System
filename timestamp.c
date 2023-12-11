#include <stdio.h>
#include <time.h>

int main() {
    time_t now;
    time(&now);

    struct tm* localTime = localtime(&now);

    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", localTime);

    printf("Current Timestamp: %s\n", buffer);

    return 0;
}
