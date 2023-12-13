#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libTinyFS.h"
#include "tinyFS_errno.h"


#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

void printHexDump(const char *filename) {
    /* be carful to wait before calling this function 
    because sometimes files take a long time to write. */
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }

    unsigned char buffer[16];
    size_t bytesRead;
    unsigned long offset = 0;

    while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        // Print the offset in the file
        printf("%08lx: ", offset);

        // Print the hex values
        for (size_t i = 0; i < 16; ++i) {
            if (i < bytesRead) {
                printf("%02x ", buffer[i]);
            } else {
                printf("   "); // Pad if the line is short
            }

            if (i == 7) {
                printf(" "); // Extra space after 8 characters
            }
        }

        printf(" |");

        // Print ASCII characters
        for (size_t i = 0; i < bytesRead; ++i) {
            if (isprint(buffer[i])) {
                printf("%c", buffer[i]);
            } else {
                printf(".");
            }
        }

        printf("|\n");
        offset += bytesRead;
    }

    fclose(file);
}


// test mk_tfs:
int main() {
    fileDescriptor fd1;
    fileDescriptor fd2;
    fileDescriptor fd3;
    fileDescriptor fd4;

    if (tfs_mount("test.dsk") < 0) {
        perror("test.dsk not found, creating new disk");
        tfs_mkfs("test.dsk", 10240);
        if (tfs_mount("test.dsk") < 0) {
            perror("failed to mount disk");
            return 1;
        }
    }
    printf("Finished inital mounting phase\n");

    fd1 = tfs_openFile("file1");
    fd2 = tfs_openFile("file2");
    fd3 = tfs_openFile("file3");
    fd4 = tfs_openFile("file4");
    printf("file descriptors: %d, %d, %d, %d\n", fd1, fd2, fd3, fd4);


    if (tfs_openFile("file1") != -1) {
        printf("should fail to open file1, why didn't it?");
    }

    if (tfs_closeFile(fd1) < 0) {
        printf("failed to close file1, this shouldn't happen");
        return 1;
    }
    printf("should be able to open file1 again\n");
    fd1 = tfs_openFile("file1");
    if (fd1 < 0) {
        printf("failed to open file1 after closing, this shouldn't happen");
        return 1;
    }

    printf("get the timestamps:\n");
    tfs_readFileInfo(fd1);
    tfs_readFileInfo(fd2);
    tfs_readFileInfo(fd3);
    tfs_readFileInfo(fd4);

    printf("file descriptors before delete: %d, %d, %d, %d\n", fd1, fd2, fd3, fd4);
    if (tfs_deleteFile(fd1) < 0) {
        perror("failed to delete file1");
        return 1;
    }
    // should now be able to open file1 again, the fd should be reused
    fd1 = tfs_openFile("file1");
    if (fd1 < 0) {
        perror("failed to open file1 after deleting, this shouldn't happen");
        return 1;
    }
    printf("file descriptors after delete and reopen (should be the same): %d, %d, %d, %d\n", fd1, fd2, fd3, fd4);


    // test unmount then remount
    if (tfs_unmount() < 0) {
        perror("failed to unmount");
        return 1;
    }
    if (tfs_mount("test.dsk") < 0) {
        perror("failed to remount");
        return 1;
    }
    // open then delete file 4, delete it, and view the hex dump
    fd4 = tfs_openFile("file4");
    if (tfs_deleteFile(fd4) < 0) {
        perror("failed to open file4 after 2nd mount, this shouldn't happen");
        return 1;
    }
    printf("Done with demo\n");
    // clean up
    int status = remove("test.dsk");
    if (status == 0) {
        printf("File deleted successfully.\n");
    } 
    else {
        perror("Error deleting the file");
    }
    return 0;
    
}
