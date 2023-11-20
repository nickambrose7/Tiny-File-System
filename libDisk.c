#include <stdio.h>

int diskCounter = 0; // global to keep track of number of disks opened

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
        // check if disk already exists
        Disk *currentDisk = diskListHead;
        while (currentDisk != NULL) {
            if (strcmp(currentDisk->filename, filename) == 0) {
                // disk already exists
                return currentDisk->diskNumber;
            }
            currentDisk = currentDisk->next;
        }
        // disk doesn't exist
        printf("LIBDISK: Error: Disk doesn't exist\n");
        return -1;

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
            perror("LIBDISK: Error opening file");
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
            perror("LIBDISK: Error allocating memory for new disk");
            return -1;
        }
        newDisk->diskNumber = diskCounter++;
        newDisk->filename = filename;
        newDisk->nBytes = nBytes;
        newDisk->next = diskListHead;
        diskListHead = newDisk;
        return newDisk->diskNumber;
    }

}

int closeDisk(int disk) {
    /* This function closes the open disk (identified by ‘disk’).
    Remove the disk from the disk list and delete the file  */

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
            if (bNum < 0 || (bNum > currentDisk->nBytes / BLOCKSIZE) {
                printf("LIBDISK: Error: bNum out of range\n");
                return -1;
            }
            // open file
            FILE *fp = fopen(currentDisk->filename, "r");
            if (fp == NULL) {
                perror("LIBDISK: Error opening file");
                return -1;
            }
            // seek to correct position
            if (fseek(fp, bNum * BLOCKSIZE, SEEK_SET) != 0) {
                perror("LIBDISK: Error seeking to position");
                return -1;
            }
            // read block
            if (fread(block, sizeof(char), BLOCKSIZE, fp) != BLOCKSIZE) {
                perror("LIBDISK: Error reading block");
                return -1;
            }
            // close file
            if (fclose(fp) != 0) {
                perror("LIBDISK: Error closing file");
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
            if (bNum < 0 || (bNum > currentDisk->nBytes / BLOCKSIZE) {
                printf("LIBDISK: Error: bNum out of range\n");
                return -1;
            }
            // open file
            FILE *fp = fopen(currentDisk->filename, "r+");
            if (fp == NULL) {
                perror("LIBDISK: Error opening file");
                return -1;
            }
            // seek to correct position
            if (fseek(fp, bNum * BLOCKSIZE, SEEK_SET) != 0) {
                perror("LIBDISK: Error seeking to position");
                return -1;
            }
            // write block
            if (fwrite(block, sizeof(char), BLOCKSIZE, fp) != BLOCKSIZE) {
                perror("LIBDISK: Error writing block");
                return -1;
            }
            // close file
            if (fclose(fp) != 0) {
                perror("LIBDISK: Error closing file");
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

