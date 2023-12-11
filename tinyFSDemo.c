#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libTinyFS.h"
#include "libTinyFS.c"
#include "tinyFS_errno.h"

// test mk_tfs:
int main() {
    int res = tfs_mkfs("test", 1024);
    if (res < 0) {
        perror("failed to make disk");
        return 1;
    }
    // try to mount the file system:

    // try to open a file:
}

