#include "libTinyFS.h"
#include "libDisk.c"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>



openFileTableEntry **openFileTable = NULL; // array of pointers to open file table entries, indexed by file descriptor

int mountedDisk = 0; // currently mounted Disk, just need to keep track of disk number,
// bc that's all that libDisk needs for read, write, and close. 0 means no disk mounted. 

int maxNumberOfFiles = 0; // max number of files that can be open/in the file system at once

int getTimestamp(char *buffer, size_t bufferSize) {
    time_t now;
    time(&now);
    struct tm *localTime = localtime(&now);
    strftime(buffer, bufferSize, "%Y-%m-%d %H:%M:%S", localTime);
    //printf("Current Timestamp: %s\n", buffer);
    return 1;
}

int tfs_mkfs(char *filename, int nBytes){
    /******************** BLOCK STRUCTURE DOCUMENTATION ****************************/
    /* 
    * BLOCKSIZE = 256 bytes
    * The documentation below assumes you are starting at position 0 in each block:
    * Pointers to blocks are their block numbers, not their addresses.
    * Since we use 4 byte pointers, we can address 2^31 - 1 blocks.


    ***SUPER BLOCK***
    | block number = 1 | MAGIC_NUMBER | free block LL head pointer | Root inode LL head pointer | 
    | 1 byte           | 1 byte       | 4 bytes                    | 4 bytes                    |
    
    ***FREE BLOCKS***
    | block number = 4 | MAGIC_NUMBER | next free block pointer    |
    | 1 byte           | 1 byte       | 4 bytes                    |
    
    ***INODE BLOCKS***
    | block number = 2 | MAGIC_NUMBER | next inode pointer    | file size | data block pointer | file name | time stamp |
    | 1 byte           | 1 byte       | 4 bytes               | 4 bytes   | 4 bytes            | 9 bytes   | 25 bytes   |
    
    ***DATA BLOCKS***
    | block number = 3 | MAGIC_NUMBER | pointer to next data block | data            |
    | 1 byte           | 1 byte       | 4 bytes                    |  250 bytes max  |
    
    */

    int numBlocks = (nBytes / BLOCKSIZE) - 1; 
    if (numBlocks < 3) {
        perror("LIBTINYFS: Error: File system size too small");
        return -1; // error
    }
    // check nbytes is in range
    if (nBytes < 0 || nBytes > MAX_BYTES) {
        perror("LIBTINYFS: Error: File system size out of range");
        return -1; // error
    }
    int diskNum = openDisk(filename, nBytes);
    if (diskNum < 0) {
        perror("LIBTINYFS: Error creating disk in tfs_mkfs");
        return -1; // error 
    }
    /* Set max number of files constant */
    maxNumberOfFiles = numBlocks / 2; // 2 blocks per file (inode block and data block)
    if (maxNumberOfFiles < 1) {
        perror("LIBTINYFS: Error: File system size too small");
        return -1; // error
    }

    /* SUPERBLOCK INIT */
    char *data = (char *)malloc(BLOCKSIZE);
    memset(data, 0, BLOCKSIZE); // zero out the data buffer
    data[BLOCK_NUMBER_OFFSET] = 1; // block type -> super block
    data[MAGIC_NUMBER_OFFSET] = MAGIC_NUMBER;
    uint32_t freeBlockHead = 1; // always 1
    *((uint32_t *)(data + 2)) = freeBlockHead; // free block LL head pointer
    // write the super block to the disk
    int writeSuccess = writeBlock(diskNum, 0, data);
    if (writeSuccess < 0) {
        perror("LIBTINYFS: Error writing super block to disk");
        return -1; // error
    }
    free(data); // deallocate the data buffer

    /* FREEBLOCK INITIALIZATION: */
    for (int i = 1; i <= numBlocks; i++) {
        // setup each free block
        char *data = (char *) malloc(BLOCKSIZE);
        memset(data, 0, BLOCKSIZE); // zero out the data buffer
        data[BLOCK_NUMBER_OFFSET] = 4; // block type -> inode block
        data[MAGIC_NUMBER_OFFSET] = MAGIC_NUMBER;
        // set up linked list chain for free blocks
        if (i < numBlocks) {
            uint32_t nextFreeBlock = i + 1;
            *((uint32_t *)(data + 2)) = nextFreeBlock; // next free block pointer
        } else {
            *((uint32_t *)(data + 2)) = 0; // no more free blocks, 0 means end of list
        }
        int writeSuccess = writeBlock(diskNum, i, data);
        if (writeSuccess < 0) {
            // print out the block number that failed to write
            printf("LIBTINYFS: Error writing inode block %d to disk", i);
            return -1; // error
        }
        free(data); // deallocate the data buffer
    }
    return 1; // success
}

int tfs_mount(char *diskname){
    // check if there is already a disk mounted...only one disk can be mounted at a time
    if(mountedDisk != 0) { // do you want to automatically unmount the currently mounted disk or nah?
        perror("LIBTINYFS: Error: A disk is already mounted, unmount current\ndisk to mount a new disk");
        return -1; // error 
    }

    // check if successfully retrieved the disk number
    mountedDisk = openDisk(diskname, 0);
    if (mountedDisk == -1) {
        perror("LIBTINYFS: Error: Could not open disk");
        return -1; // error 
    }

    // make a block sized buffer
    char *data = (char *)malloc(BLOCKSIZE*sizeof(char));
    // successful read? correct FS type? 
    int i = 0;
    while(readBlock(mountedDisk, i, data) < 0) {
        if (data[BLOCK_NUMBER_OFFSET] <= 0 || data[BLOCK_NUMBER_OFFSET] > 4) {
            perror("LIBTINYFS: Error: Invalid block type");
            return -1; // error 
        }
        if (data[MAGIC_NUMBER_OFFSET] != MAGIC_NUMBER) {
            perror("LIBTINYFS: Error: Invalid magic number");
            return -1; // error 
        }
    }

    // allocate open file table
    openFileTable = (openFileTableEntry **)malloc(maxNumberOfFiles * sizeof(openFileTableEntry *));
    if (openFileTable == NULL) {
        perror("LIBTINYFS: Error: Could not allocate memory for open file table");
        return -1; // error
    }
    // initialize open file table
    for (int i = 0; i < maxNumberOfFiles; i++) {
        openFileTable[i] = NULL;
    }

    //deallocate buffer
    free(data);
    return mountedDisk; // success - will be a positive number
}

int tfs_unmount(void){
    // is there a disk to unmount?
    if (mountedDisk == 0) {
        perror("LIBTINYFS: tfs_unmount: No disk to unmount");
        return -1; // error
    }
    // unmount the currently mounted disk
    mountedDisk = 0;
    // reset openFileTable
    for (int i = 0; i < maxNumberOfFiles; i++) {
        if (openFileTable[i] != NULL) {
            free(openFileTable[i]);
            openFileTable[i] = NULL;
        }
    }
    free(openFileTable);

    openFileTable = NULL;
    return 1; // success
}

fileDescriptor tfs_openFile(char *name){
    // creates or opens a file for reading and writing

    // search our inode list for that file name
    // read in our inode LL head pointer from the super block
    char *superData = (char *)malloc(BLOCKSIZE);
    int success = readBlock(mountedDisk, SUPER_BLOCK, superData);
    if (success < 0) {
        perror("LIBTINYFS: Error: Issue with super block read when opening file. (openFile)");
        return -1; // error
    }
    // get root inode LL head pointer
    int inodeHead;
    memcpy(&inodeHead, superData + IB_OFFSET, sizeof(int));
    // read each inode in the linked list looking for a file name that matches ours
    int currentInode = inodeHead;
    char *inodeData = (char *)malloc(BLOCKSIZE);
    char fileName[MAX_FILE_NAME_SIZE];
    while (1) {
        // read in the inode
        success = readBlock(mountedDisk, currentInode, inodeData);
        if (success < 0) {
            perror("LIBTINYFS: Error: Invalid pointer to inode block (openFile)");
            return -1; // error
        }
        // get file name
        memcpy(fileName, inodeData + INODE_FILE_NAME_OFFSET, MAX_FILE_NAME_SIZE*sizeof(char));
        // check if file name matches
        if (strcmp(fileName, name) == 0) {
            // found the file
            // check if file is already open
            for (int i = 0; i < maxNumberOfFiles; i++) {
                if (openFileTable[i] != NULL && openFileTable[i]->inodeNumber == currentInode) {
                    // file is already open
                    perror("LIBTINYFS: tfs_openfile: File is already open");
                    return -1; // error
                }
            }
            // file is not already open
            // add to open file table
            openFileTableEntry *newEntry = (openFileTableEntry *)malloc(sizeof(openFileTableEntry));
            if (newEntry == NULL) {
                perror("LIBTINYFS: tfs_openfile: Could not allocate memory for new open file table entry");
                return -1; // error
            }
            newEntry->filePointer = 0; // set file pointer to beginning of file
            int currentfd = 0;
            while (openFileTable[currentfd] != NULL) {  // find the next empty spot in the open file table
                currentfd++;
            }
            newEntry->inodeNumber = currentInode; // set inode number
            openFileTable[currentfd] = newEntry; // set the entry
            return currentfd; // return file descriptor
        }
        // clear fileName for the next iteration
        memset(fileName, 0, MAX_FILE_NAME_SIZE*sizeof(char));
        if (inodeData[INODE_NEXT_INODE_OFFSET] == 0) {
            // no more inodes to check, need this pointer to our last inode
            break;
        }
        // get next inode 
        memcpy(&currentInode, inodeData + INODE_NEXT_INODE_OFFSET, sizeof(int)); 
        // clear inodeData for the next iteration
        memset(inodeData, 0, BLOCKSIZE);
    }
    
    // if not in our list, allocate a new inode for the file
    // read in our free block LL head pointer from the super block
    int freeBlockHead;
    memcpy(&freeBlockHead, superData + FB_OFFSET, sizeof(int));
    // check if there are any free blocks
    if (freeBlockHead == 0) {
        perror("LIBTINYFS: Error: No free blocks");
        return -1; // error
    }
    // get the first free block
    char *freeBlockData = (char *)malloc(BLOCKSIZE);
    success = readBlock(mountedDisk, freeBlockHead, freeBlockData);
    if (success < 0) {
        perror("LIBTINYFS: Error: Invalid pointer to free block (openFile)");
        return -1; // error
    }

    // get the next free block
    int nextFreeBlock;
    memcpy(&nextFreeBlock, freeBlockData + FREE_NEXT_BLOCK_OFFSET, sizeof(int));
    // update the free block LL head pointer in the super block, removing the block from the free
    // block LL
    memcpy(superData + FB_OFFSET, &nextFreeBlock, sizeof(int));
    // turn the free block into an inode block
    int newInodeBlockNum = freeBlockHead;
    freeBlockData[BLOCK_NUMBER_OFFSET] = 2; // block type -> inode block
    freeBlockData[MAGIC_NUMBER_OFFSET] = MAGIC_NUMBER;
    // set the next inode pointer
    // get the next inode block from the super block
    memcpy(&inodeHead, superData + IB_OFFSET, sizeof(int));
    memcpy(freeBlockData + INODE_NEXT_INODE_OFFSET, &inodeHead, sizeof(int));
    // update the super block to point to our new inode block
    memcpy(superData + IB_OFFSET, &newInodeBlockNum, sizeof(int));
    // set the file size to 0
    int fileSize = 0;
    memcpy(freeBlockData + INODE_FILE_SIZE_OFFSET, &fileSize, sizeof(int));
    // set data block pointer to 0
    int dataBlockPointer = 0;
    memcpy(freeBlockData + INODE_DATA_BLOCK_OFFSET, &dataBlockPointer, sizeof(int));
    // set file name
    memset(freeBlockData + INODE_FILE_NAME_OFFSET, 0, MAX_FILE_NAME_SIZE*sizeof(char));
    memcpy(freeBlockData + INODE_FILE_NAME_OFFSET, name, strlen(name)*sizeof(char)); 
    // set time stamp
    char * timeStampBuffer = (char *)malloc(TIMESTAMP_BUFFER_SIZE);
    getTimestamp(timeStampBuffer, TIMESTAMP_BUFFER_SIZE);
    memcpy(freeBlockData + INODE_TIME_STAMP_OFFSET, timeStampBuffer, TIMESTAMP_BUFFER_SIZE);
    // write the super block back to disk
    int writeSuccess = writeBlock(mountedDisk, SUPER_BLOCK, superData); 
    if (writeSuccess < 0) {
        perror("LIBTINYFS: Error: Issue with super block write when opening file. (openFile)");
        return -1; // error
    }
    // write the inode block back to disk
    writeSuccess = writeBlock(mountedDisk, newInodeBlockNum, freeBlockData);
    if (writeSuccess < 0) {
        perror("LIBTINYFS: Error: Issue with inode block write when opening file. (openFile)");
        return -1; // error
    }
    // add to open file table
    openFileTableEntry *newEntry = (openFileTableEntry *)malloc(sizeof(openFileTableEntry));
    if (newEntry == NULL) {
        perror("LIBTINYFS: Error: Could not allocate memory for new open file table entry");
        return -1; // error
    }
    newEntry->filePointer = 0; // set file pointer to beginning of file
    // find the next empty spot in the open file table
    int currentfd = 0;
    while (openFileTable[currentfd] != NULL) {
        currentfd++;
    }
    newEntry->inodeNumber = newInodeBlockNum; // set inode number
    // set the entry
    openFileTable[currentfd] = newEntry;
    // free everything we dont need anymore
    free(superData);
    free(inodeData);
    free(freeBlockData);
    free(timeStampBuffer);
    return currentfd; // return file descriptor
}

int tfs_closeFile(fileDescriptor FD) {
    
}

int tfs_writeFile(fileDescriptor FD,char *buffer, int size){
    if (mountedDisk == NULL) {
        perror("LIBTINYFS: Error: No disk mounted. Cannot find file. (writeFile)");
        return -1; // error
    }
    // read super block
    char *superData = (char *)malloc(BLOCKSIZE*sizeof(char));
    int success = readBlock(mountedDisk, SUPER_BLOCK, superData);

    if (success < 0) {
        perror("LIBTINYFS: Error: Issue with super block read when writing to file. (writeFile)");
        return -1; // error
    }

    // get free block head which is a block number
    int freeBlockHead;
    memcpy(&freeBlockHead, superData[FB_OFFSET], sizeof(int));

    // get inode head
    int fileInode;
    int inodeNum = openFileTable[FD];
    int inodeHead;
    memcpy(&inodeHead, superData[IB_OFFSET], sizeof(int));

    // iterate through inode list until get to inodeNum_th inode
    fileInode = inodeHead;
    while (inodeNum != 0) {
        // get inode info
        char *inodeData = (char *)malloc(BLOCKSIZE*sizeof(char));
        int success = readBlock(mountedDisk, fileInode, inodeData);

        if (success < 0) {
            perror("LIBTINYFS: Error: Invalid pointer to inode block (writeFile)");
            return -1; // error
        }
        
        // get next inode
        memcpy(&fileInode, inodeData[NEXT_INODE_OFFSET], sizeof(int));
        // decrement inodeNum
        inodeNum--;
    }

    // // way to error check this?
    // if (file condition) {
    //     perror("LIBTINYFS: Error: File is not open or does not exist.");
    //     return -1; // error
    // }

    // if file open

    int blocksNeeded = size / USEABLE_DATA_SIZE + 1;

    // IMPLEMENTATION CONSIDERATION: IF CURRENT WRITE CANNOT BE FULLY WRITTEN: ERROR AND EXIT? OR WRITE AS MUCH AS POSSIBLE AND THEN ERROR EXIT?

    // Check if there are current data blocks under the file
        // if there are blocks write data where file pointer is set
        // if need more room (indicated by file pointer is larger than size) look at super block on disk to get next free block
            // if no more free blocks error

    // if new blocks are need at super block on disk to get next free block
            // if no more free blocks error
        
    // increment the file size as the file is written in case of error
    
    // get current time to modify timestamp
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    // total of 24 bytes (4 bytes/int * 6 ints)
    int year = tm.tm_year + 1900;
    int month = tm.tm_mon + 1;
    int day = tm.tm_mday;
    int hour = tm.tm_hour;
    int min = tm.tm_min;
    int sec = tm.tm_sec;

}
int tfs_deleteFile(fileDescriptor FD){
    
}
int tfs_readByte(fileDescriptor FD, char *buffer){
    
}
int tfs_seek(fileDescriptor FD, int offset){
    
}