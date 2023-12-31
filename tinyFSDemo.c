#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libTinyFS.h"
#include "tinyFS_errno.h"


#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>

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

    // preamble
    char *preamble = (char *)malloc(sizeof(char)*327);
    memcpy(preamble, "We the People of the United States, in Order to form a more perfect Union, establish Justice, insure domestic Tranquility, provide for the common defence, promote the general Welfare, and secure the Blessings of Liberty to ourselves and our Posterity, do ordain and establish this Constitution for the United States of America.", 327);

    fileDescriptor fd1;
    fileDescriptor fd2;
    fileDescriptor fd3;
    fileDescriptor fd4;
    int status;

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
    
    // write to file 1
    printf("\nWriting to file1\n");
    if (tfs_writeFile(fd1, "Hello World!", strlen("Hello World!")) < 0) {
        return -1;
    }
    printf("Theoretically, wrote 'Hello World!'\n");

    // read byte 0
    printf("\nRead first character of file1\n");
    char *oneByte = (char *)malloc(sizeof(char));
    if (tfs_readByte(fd1, oneByte) < 0) {
        printf("Read 1\n");
        return -1;
    }
    printf("%c\n", *oneByte);
    printf("\nFile pointer of file1 is now at 1\n");
 
    // seek f1 by 6
    printf("\nSeek file pointer by 6\n");
    if(tfs_seek(fd1, 6) < 0) {
        return -1;
    }

    printf("\nRead 7th character of file1\n");
    // read byte to show file pointer is at 'o'
    if (tfs_readByte(fd1, oneByte) < 0) {
        printf("Read 2\n");
        return -1;
    }
    printf("%c\n", *oneByte);

    printf("\nWriting to file1 again\n");
    // overwrite the file, pointer should be reset to 0
    if(tfs_writeFile(fd1, "abc", strlen("abc")) < 0) {
        return -1;
    }
    printf("Theoretically, wrote 'abc'\n");

    // read all the way through file
    printf("Reading all of file1\n");
    while(tfs_readByte(fd1, oneByte) > 0) {
        printf("%c", *oneByte);
    }
    printf("\n");

    if (tfs_writeFile(fd4, preamble, 327) < 0) {
        return -1;
    }
    printf("Theoretically, wrote preamble of the constitution.\n");

    int it = 20;
    while(it != 0) {
        tfs_readByte(fd4, oneByte);
        printf("%c", *oneByte);
        it--;
    }
    printf("\n");

    printf("\nSeek file pointer to second block\n");
    if(tfs_seek(fd4, 250) < 0) {
        return -1;
    }
    tfs_readByte(fd4, oneByte);
    printf("%c\n", *oneByte);

    printf("\nWriting to file1 again\n");
    // overwrite the file, pointer should be reset to 0
    if(tfs_writeFile(fd4, "abc", strlen("abc")) < 0) {
        return -1;
    }
    printf("Theoretically, wrote 'abc' to file4\n");
    printf("\nSeek file pointer past new write length\n");
    if(tfs_seek(fd4, 5) < 0) {
        return -1;
    }
    if(tfs_readByte(fd4, oneByte) < 0) {
        printf("Data overwritten correctly\n");
    }

    // renaming
    printf("\nCan we rename file2 to mainfile.txt?\n");
    // rename file 1 to "mainfile.txt" -> error
    if (tfs_rename(fd2, "mainfile.txt") < 0) {
        printf("No\n");
        printf("Can we rename file 2 to main.c?\n");
        // rename to main.c -> good
        if (tfs_rename(fd2, "main.c") > 0) {
            printf("Yes! Renamed.\n\n");
        }
        else {
            printf("No, renaming failed.\n");
        }
    }
    else {
        printf("\nYes, that's not good\n");
    }

    // opening and closing file
    if (tfs_openFile("file1") < 0) {
        printf("Fails to open file1, because it is already open\n");
    }

    if (tfs_closeFile(fd1) < 0) {
        printf("failed to close file1, this shouldn't happen\n");
        return 1;
    }
    printf("closed file 1\n");
    printf("Should be able to open file1 again...\n");
    fd1 = tfs_openFile("file1");
    if (fd1 < 0) {
        printf("failed to open file1 after closing, this shouldn't happen\n");
        return 1;
    }
    printf("Opened file1...\n");

    // readdir
    printf("\nCurrent Working Directory...\n");
    if(tfs_readdir() < 0) {
        return -1;
    }

    // get file info
    printf("Get all file info...\n");
    tfs_readFileInfo(fd1);
    tfs_readFileInfo(fd2);
    tfs_readFileInfo(fd3);
    tfs_readFileInfo(fd4);

    printf("file descriptors before delete: %d, %d, %d, %d\n", fd1, fd2, fd3, fd4);
    if (tfs_deleteFile(fd1) < 0) {
        printf("failed to delete file1\n");
        return 1;
    }

    // should now be able to open file1 again, the fd should be reused
    fd1 = tfs_openFile("file1");
    if (fd1 < 0) {
        printf("failed to open file1 after deleting, this shouldn't happen\n");
        return 1;
    }
    printf("file descriptors after delete and reopen (should be the same): %d, %d, %d, %d\n", fd1, fd2, fd3, fd4);


    // test unmount then remount
    if (tfs_unmount() < 0) {
        printf("failed to unmount\n");
        return 1;
    }
    if (tfs_mount("test.dsk") < 0) {
        printf("failed to remount\n");
        return 1;
    }
    // open then delete file 4, delete it, and view the hex dump
    fd4 = tfs_openFile("file4");
    if (tfs_deleteFile(fd4) < 0) {
        printf("failed to open file4 after 2nd mount, this shouldn't happen\n");
        return 1;
    }
    printf("Done with demo\n");
    // clean up
    status = remove("test.dsk");
    if (status == 0) {
        printf("File deleted successfully.\n");
    } 
    else {
        printf("Error deleting the file\n");
    }
    return 0;
    
}
