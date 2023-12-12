#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libTinyFS.h"
#include "libTinyFS.c"
#include "tinyFS_errno.h"

// test mk_tfs:
int main() {
    int res = tfs_mkfs("test.dsk", 1024);
    if (res < 0) {
        perror("failed to make disk");
        return 1;
    }
    // try to mount the file system:
    res = tfs_mount("test.dsk");
    if (res < 0) {
        perror("failed to mount disk");
        return 1;
    }
    fileDescriptor fd = tfs_openFile("testFile");
    // try to open again, should get an error:
    tfs_openFile("testFile");

    tfs_unmount();

    tfs_mount("test.dsk");

    tfs_openFile("testFile"); // should not get an error

    // now need to test with close file & delete file. 
    //Write that next.

    // try to open a file:
    printf("passed demo with no errors\n");
    return 0;
}

