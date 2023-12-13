#include "libDisk.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int diskCounter = 1; // global to keep track of number of disks opened

Disk *diskListHead = NULL; // global to keep track of list of disks


int openDisk(char *filename, int nBytes) {
    /* This functions opens a regular UNIX file and designates the first
    nBytes of it as space for the emulated disk. If nBytes is not exactly a
    multiple of BLOCKSIZE then the disk size will be the closest multiple
    of BLOCKSIZE that is lower than nByte (but greater than 0) If nBytes is
    less than BLOCKSIZE failure should be returned. If nBytes > BLOCKSIZE
    and there is already a file by the given filename, that file’s content
    may be overwritten. If nBytes is 0, an existing disk is opened, and the
    content must not be overwritten in this function. There is no requirement
    to maintain integrity of any file content beyond nBytes. The return value
    is negative on failure or a disk number on success. */
    if (nBytes == 0) {
        // open existing disk, can't overwirte content
        // File should already exist, open it
        FILE *fp = fopen(filename, "r+");
        if (fp == NULL) {
            printf("LIBDISK: File did not exist, it should have!\n");
            return -1;
        }
        // get file size
        fseek(fp, 0, SEEK_END);
        int fileSize = ftell(fp);
        if (fileSize % BLOCKSIZE != 0) {
            printf("LIBDISK: File size is not a multiple of BLOCKSIZE\n");
            return -1;
        }
        // return position to the beginning
        fseek(fp, 0, SEEK_SET);
        // add disk to disk list
        Disk *newDisk = malloc(sizeof(Disk));
        if (newDisk == NULL) {
            printf("LIBDISK: Error allocating memory for new disk\n");
            return -1;
        }
        newDisk->diskNumber = diskCounter++;
        char *filenameCopy = malloc(strlen(filename) + 1);
        if (filenameCopy == NULL) {
            printf("LIBDISK: Error allocating memory for filename\n");
            return -1;
        }
        strcpy(filenameCopy, filename);
        newDisk->filename = filenameCopy;
        newDisk->nBytes = fileSize;
        newDisk->next = diskListHead;
        newDisk->filePointer = fp;
        diskListHead = newDisk;
        return newDisk->diskNumber;
    } else {
        // create new disk
        if (nBytes < BLOCKSIZE) {
            printf("LIBDISK: Error: nBytes must be at least BLOCKSIZE\n");
            return -1;
        }
        if (nBytes % BLOCKSIZE != 0) {
            nBytes = nBytes - (nBytes % BLOCKSIZE);
        }
        // create file
        FILE *fp = fopen(filename, "w"); // will truncate file to 0 if it exists
        if (fp == NULL) {
            printf("LIBDISK: Error opening file\n");
            return -1;
        }
        // write nBytes of 0s to file
        char zero = 0;
        for (int i = 0; i < nBytes; i++) {
            fwrite(&zero, sizeof(char), 1, fp);
        }
        // add disk to disk list
        Disk *newDisk = malloc(sizeof(Disk));
        if (newDisk == NULL) {
            printf("LIBDISK: Error allocating memory for new disk\n");
            return -1;
        }
        newDisk->diskNumber = diskCounter++;
        char *filenameCopy = malloc(strlen(filename) + 1);
        if (filenameCopy == NULL) {
            printf("LIBDISK: Error allocating memory for filename\n");
            return -1;
        }
        strcpy(filenameCopy, filename);
        newDisk->filename = filenameCopy;
        newDisk->nBytes = nBytes;
        newDisk->next = diskListHead;
        newDisk->filePointer = fp;
        diskListHead = newDisk;
        return newDisk->diskNumber;
    }

}

int closeDisk(int disk) {
    /* This function closes the open disk (identified by ‘disk’).
    Remove the disk from the disk list and delete the file  */
    // close the unix file and free memory we allocated
    Disk *currentDisk = diskListHead;
    Disk *previousDisk = NULL;
    while (currentDisk != NULL) {
        if (currentDisk->diskNumber == disk) {
            // found disk
            // close file
            if (fclose(currentDisk->filePointer) != 0) {
                printf("LIBDISK: Error closing file\n");
                return -1;
            }
            // remove disk from disk list
            if (previousDisk == NULL) { // disk is head of list
                // disk is head of list
                diskListHead = currentDisk->next;
            } else { // disk is not head of list
                previousDisk->next = currentDisk->next;
            }
            // free memory
            free(currentDisk);
            return 0; // success
        }
        previousDisk = currentDisk;
        currentDisk = currentDisk->next;
    }
    // disk not found
    printf("LIBDISK: Error: Disk not found\n");
    return -1;
}


int readBlock(int disk, int bNum, void *block) {
    /* readBlock() reads an entire block of BLOCKSIZE bytes from the open
    disk (identified by ‘disk’) and copies the result into a local buffer
    (must be at least of BLOCKSIZE bytes). The bNum is a logical block
    number, which must be translated into a byte offset within the disk. The
    translation from logical to physical block is straightforward: bNum=0
    is the very first byte of the file. bNum=1 is BLOCKSIZE bytes into the
    disk, bNum=n is n*BLOCKSIZE bytes into the disk. On success, it returns
    0. -1 or smaller is returned if disk is not available (hasn’t been
    opened) or any other failures. You must define your own error code
    system. */
    Disk *currentDisk = diskListHead;
    while (currentDisk != NULL) {
        if (currentDisk->diskNumber == disk) {
            // found disk
            if (bNum < 0 || (bNum > currentDisk->nBytes / BLOCKSIZE)) {
                printf("LIBDISK: Error: bNum out of range\n");
                return -1;
            }
            // open file
            FILE *fp = currentDisk->filePointer;
            // seek to correct position, use SEEK_SET to seek from beginning of file
            if (fseek(fp, bNum * BLOCKSIZE, SEEK_SET) != 0) {
                printf("LIBDISK: Error seeking to position\n");
                return -1;
            }
            // read block
            if (fread(block, sizeof(char), BLOCKSIZE, fp) != BLOCKSIZE) {
                printf("LIBDISK: Error reading block\n");
                return -1;
            }
            return 0;
        }
        currentDisk = currentDisk->next;
    }
    // disk not found
    printf("LIBDISK: Error: Disk not found\n");
    return -1;
}

int writeBlock(int disk, int bNum, void *block) {
    /* writeBlock() takes disk number ‘disk’ and logical block number ‘bNum’
    and writes the content of the buffer ‘block’ to that location. ‘block’
    must be integral with BLOCKSIZE. Just as in readBlock(), writeBlock()
    must translate the logical block bNum to the correct byte position in
    the file. On success, it returns 0. -1 or smaller is returned if disk
    is not available (i.e. hasn’t been opened) or any other failures. You
    must define your own error code system. */
    Disk *currentDisk = diskListHead;
    while (currentDisk != NULL) {
        if (currentDisk->diskNumber == disk) {
            // found disk
            if (bNum < 0 || (bNum > currentDisk->nBytes / BLOCKSIZE)) {
                printf("LIBDISK: Error: bNum out of range\n");
                return -1;
            }
            // open file
            FILE *fp = currentDisk->filePointer;
            // seek to correct position
            if (fseek(fp, bNum * BLOCKSIZE, SEEK_SET) != 0) {
                printf("LIBDISK: Error seeking to position\n");
                return -1;
            }
            // write block
            if (fwrite(block, sizeof(char), BLOCKSIZE, fp) != BLOCKSIZE) {
                printf("LIBDISK: Error writing block\n");
                return -1;
            }
            return 0;
        }
        currentDisk = currentDisk->next;
    }
    // disk not found
    printf("LIBDISK: Error: Disk not found\n");
    return -1;
}

