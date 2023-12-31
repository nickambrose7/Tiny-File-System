#include "libTinyFS.h"
#include "libDisk.h"
#include "tinyFS_errno.h"
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
    | block number = 1 | MAGIC_NUMBER | free block LL head pointer | Root inode LL head pointer | Max number of files |
    | 1 byte           | 1 byte       | 4 bytes                    | 4 bytes                    |     4 bytes
    
    ***FREE BLOCKS***
    | block number = 4 | MAGIC_NUMBER | next free block pointer    |
    | 1 byte           | 1 byte       | 4 bytes                    |
    
    ***INODE BLOCKS***
    | block number = 2 | MAGIC_NUMBER | next inode pointer    | file size | data block pointer | file name | time stamp - creation | time stamp - last modified | time stamp - last accessed |
    | 1 byte           | 1 byte       | 4 bytes               | 4 bytes   | 4 bytes            | 9 bytes   |       25 bytes        |           25 bytes         |           25 bytes         |
    
    ***DATA BLOCKS***
    | block number = 3 | MAGIC_NUMBER | pointer to next data block | data            |
    | 1 byte           | 1 byte       | 4 bytes                    |  250 bytes max  |
    
    */

    int numBlocks = (nBytes / BLOCKSIZE) - 1; 
    if (numBlocks < 3) {
        printf("LIBTINYFS-mkfs: File system size too small\n");
        return ECREATFS; // error
    }
    // check nbytes is in range
    if (nBytes < 0 || nBytes > MAX_BYTES) {
        printf("LIBTINYFS-mkfs: File system size out of range\n");
        return ECREATFS; // error
    }
    int diskNum = openDisk(filename, nBytes);
    if (diskNum < 0) {
        printf("LIBTINYFS-mkfs: creating disk in tfs_mkfs\n");
        return ECREATFS; // error 
    }
    /* Set max number of files constant */
    maxNumberOfFiles = numBlocks / 2; // 2 blocks per file (inode block and data block)
    if (maxNumberOfFiles < 1) {
        printf("LIBTINYFS-mkfs: File system size too small\n");
        return ECREATFS; // error
    }

    /* SUPERBLOCK INIT */
    char *data = (char *)malloc(BLOCKSIZE);
    memset(data, 0, BLOCKSIZE); // zero out the data buffer
    data[BLOCK_NUMBER_OFFSET] = 1; // block type -> super block
    data[MAGIC_NUMBER_OFFSET] = MAGIC_NUMBER;
    uint32_t freeBlockHead = 1; // always 1
    *((uint32_t *)(data + 2)) = freeBlockHead; // free block LL head pointer
    // write max number of files into super block
    memcpy(data + SUPER_MAX_NUM_FILES_OFFSET, &maxNumberOfFiles, sizeof(int));
    // write the super block to the disk
    int writeSuccess = writeBlock(diskNum, 0, data);
    if (writeSuccess < 0) {
        printf("LIBTINYFS-mkfs: Error writing super block to disk\n");
        return ECREATFS; // error
    }
    free(data); // deallocate the data buffer

    /* FREEBLOCK INITIALIZATION: */
    for (int i = 1; i <= numBlocks; i++) {
        // setup each free block
        char *data = (char *) malloc(BLOCKSIZE);
        memset(data, 0, BLOCKSIZE); // zero out the data buffer
        data[BLOCK_NUMBER_OFFSET] = 4; // block type -> free block
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
            printf("LIBTINYFS-mkfs: Error writing inode block %d to disk\n", i);
            return ECREATFS; // error
        }
        free(data); // deallocate the data buffer
    }
    return 1; // success
}

int tfs_mount(char *diskname){
    // check if there is already a disk mounted...only one disk can be mounted at a time
    if(mountedDisk != 0) { // do you want to automatically unmount the currently mounted disk or nah?
        printf("LIBTINYFS-mount: A disk is already mounted, unmount current\ndisk to mount a new disk\n");
        return EMOUNTFS; // error 
    }

    // check if successfully retrieved the disk number
    mountedDisk = openDisk(diskname, 0);
    if (mountedDisk == -1) {
        mountedDisk = 0; // it failed so we must reset the mountedDisk global
        printf("LIBTINYFS-mount: Could not open disk\n");
        return EMOUNTFS; // error 
    }
    // get the max number of files value
    char *superData = (char *)malloc(BLOCKSIZE);
    int success = readBlock(mountedDisk, SUPER_BLOCK, superData);
    if (success < 0) {
        printf("LIBTINYFS-mount: Issue with super block read when mounting disk\n");
        return EMOUNTFS; // error 
    }
    memcpy(&maxNumberOfFiles, superData + SUPER_MAX_NUM_FILES_OFFSET, sizeof(int));

    // make a block sized buffer
    char *data = (char *)malloc(BLOCKSIZE*sizeof(char));
    // successful read? correct FS type? 
    int i = 0;
    while(readBlock(mountedDisk, i, data) < 0) {
        if (data[BLOCK_NUMBER_OFFSET] <= 0 || data[BLOCK_NUMBER_OFFSET] > 4) {
            printf("LIBTINYFS-mount: Invalid block type\n");
            return EMOUNTFS; // error 
        }
        if (data[MAGIC_NUMBER_OFFSET] != MAGIC_NUMBER) {
            printf("LIBTINYFS-mount: Invalid magic number\n");
            return EMOUNTFS; // error 
        }
    }

    // allocate open file table
    openFileTable = (openFileTableEntry **)malloc(maxNumberOfFiles * sizeof(openFileTableEntry *));
    if (openFileTable == NULL) {
        printf("LIBTINYFS-mount: Could not allocate memory for open file table\n");
        return EMOUNTFS; // error
    }
    // initialize open file table
    for (i = 0; i < maxNumberOfFiles; i++) {
        openFileTable[i] = NULL;
    }

    //deallocate buffer
    free(data);
    return mountedDisk; // success - will be a positive number
}

int tfs_unmount(void){
    // is there a disk to unmount?
    if (mountedDisk == 0) {
        printf("LIBTINYFS-unmount: No disk to unmount\n");
        return EUNMOUNTFS; // error
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
        printf("LIBTINYFS-openFile: Issue with super block read when opening file\n");
        return EOPEN; // error
    }
    // get root inode LL head pointer
    int inodeHead;
    memcpy(&inodeHead, superData + IB_OFFSET, sizeof(int));
    // read each inode in the linked list looking for a file name that matches ours]
    if (inodeHead != 0) { // there exists an inode to check
        int currentInode = inodeHead;
        char *inodeData = (char *)malloc(BLOCKSIZE);
        char fileName[MAX_FILE_NAME_SIZE];
        while (1) {
            // read in the inode
            success = readBlock(mountedDisk, currentInode, inodeData);
            if (success < 0) {
                printf("LIBTINYFS-openFile: Invalid pointer to inode block\n");
                return EOPEN; // error
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
                        printf("LIBTINYFS-openFile: File is already open\n");
                        return EOPEN; // error
                    }
                }
                // file is not already open
                // add to open file table
                openFileTableEntry *newEntry = (openFileTableEntry *)malloc(sizeof(openFileTableEntry));
                if (newEntry == NULL) {
                    printf("LIBTINYFS-openFile: Could not allocate memory for new open file table entr\ny");
                    return EOPEN; // error
                }
                newEntry->filePointer = 0; // set file pointer to beginning of file
                int currentfd = 0;
                while (openFileTable[currentfd] != NULL) {  // find the next empty spot in the open file table
                    currentfd++;
                }
                newEntry->inodeNumber = currentInode; // set inode number
                openFileTable[currentfd] = newEntry; // set the entry
                // update the time stamps
                char *timeStampBuffer = (char *)malloc(TIMESTAMP_BUFFER_SIZE);
                getTimestamp(timeStampBuffer, TIMESTAMP_BUFFER_SIZE);
                // zero out the last accessed timestamp
                memset(inodeData + INODE_ACC_TIME_STAMP_OFFSET, 0, TIMESTAMP_BUFFER_SIZE);
                // set the last accessed timestamp
                memcpy(inodeData + INODE_ACC_TIME_STAMP_OFFSET, timeStampBuffer, TIMESTAMP_BUFFER_SIZE);
                // write the inode back to disk
                int writeSuccess = writeBlock(mountedDisk, currentInode, inodeData);
                if (writeSuccess < 0) {
                    printf("LIBTINYFS-openFile: Issue with inode block write when opening file\n");
                    return EOPEN; // error
                }
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
        free(inodeData);
    }
    // if not in our list, allocate a new inode for the file
    // read in our free block LL head pointer from the super block
    int freeBlockHead;
    memcpy(&freeBlockHead, superData + FB_OFFSET, sizeof(int));
    // check if there are any free blocks
    if (freeBlockHead == 0) {
        printf("LIBTINYFS-openFile: No free blocks\n");
        return ENOSPC; // error
    }
    // get the first free block
    char *freeBlockData = (char *)malloc(BLOCKSIZE);
    success = readBlock(mountedDisk, freeBlockHead, freeBlockData);
    if (success < 0) {
        printf("LIBTINYFS-openFile: Invalid pointer to free block\n");
        return EOPEN; // error
    }

    // get the next free block
    int nextFreeBlock;
    memcpy(&nextFreeBlock, freeBlockData + FREE_NEXT_BLOCK_OFFSET, sizeof(int));
    // update the free block LL head pointer in the super block, removing the block from the free
    // block LL
    memcpy(superData + FB_OFFSET, &nextFreeBlock, sizeof(int));
    // turn the free block into an inode block
    int newInodeBlockNum = freeBlockHead;
    freeBlockData[BLOCK_NUMBER_OFFSET] = INODE_BLOCK_TYPE; // block type -> inode block
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
    memcpy(freeBlockData + INODE_CR8_TIME_STAMP_OFFSET, timeStampBuffer, TIMESTAMP_BUFFER_SIZE);
    memcpy(freeBlockData + INODE_MOD_TIME_STAMP_OFFSET, timeStampBuffer, TIMESTAMP_BUFFER_SIZE);
    memcpy(freeBlockData + INODE_ACC_TIME_STAMP_OFFSET, timeStampBuffer, TIMESTAMP_BUFFER_SIZE);
    // write the super block back to disk
    int writeSuccess = writeBlock(mountedDisk, SUPER_BLOCK, superData); 
    if (writeSuccess < 0) {
        printf("LIBTINYFS-openFile: Issue with super block write when opening file\n");
        return EOPEN; // error
    }
    // write the inode block back to disk
    writeSuccess = writeBlock(mountedDisk, newInodeBlockNum, freeBlockData);
    if (writeSuccess < 0) {
        printf("LIBTINYFS-openFile: Issue with inode block write when opening file\n");
        return EOPEN; // error
    }
    // add to open file table
    openFileTableEntry *newEntry = (openFileTableEntry *)malloc(sizeof(openFileTableEntry));
    if (newEntry == NULL) {
        printf("LIBTINYFS-openFile: Could not allocate memory for new open file table entry\n");
        return EOPEN; // error
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
    free(freeBlockData);
    free(timeStampBuffer);
    return currentfd; // return file descriptor
}

int tfs_closeFile(fileDescriptor FD) {
    // check if FD is valid
    if (openFileTable[FD] == NULL) {
        printf("LIBTINYFS-closeFile: Invalid file descriptor. Cannot close file\n");
        return EBADFD; // error
    }
    // check if there is a disk mounted
    if (mountedDisk == 0) {
        printf("LIBTINYFS-closeFile: No disk mounted. Cannot close file\n");
        return ECLOSE; // error
    }
    // check if file descriptor is valid
    if (FD < 0 || FD >= maxNumberOfFiles) {
        printf("LIBTINYFS-closeFile: Invalid file descriptor. Cannot close file\n");
        return EBADFD; // error
    }
    // free the open file table entry
    free(openFileTable[FD]);
    openFileTable[FD] = NULL;
    return 1; // success
}

int deallocateBlock(int blockNum) {
    /* This function takes a block number of an 
    inode or data block and deallocates it, and 
    adds it to the free block list */
    // read in the block
    char *data = (char *)malloc(BLOCKSIZE);
    int success = readBlock(mountedDisk, blockNum, data);
    if (success < 0) {
        printf("LIBTINYFS-deallocateBlock: Invalid pointer to block\n");
        return EDEALLOC; // error
    }
    // zero out the data buffer
    memset(data, 0, BLOCKSIZE);
    // prep the data buffer to be written as a free block
    data[BLOCK_NUMBER_OFFSET] = FREE_BLOCK_TYPE; // block type -> free block
    data[MAGIC_NUMBER_OFFSET] = MAGIC_NUMBER;
    // read in the super block
    char *superData = (char *)malloc(BLOCKSIZE);
    success = readBlock(mountedDisk, SUPER_BLOCK, superData);
    if (success < 0) {
        printf("LIBTINYFS-deallocateBlock: Issue with super block read when deallocating block\n");
        return EDEALLOC; // error
    }
    // get the free block LL head pointer
    int freeBlockHead;
    memcpy(&freeBlockHead, superData + FB_OFFSET, sizeof(int));
    // set the next free block pointer to the current free block LL head pointer
    memcpy(data + FREE_NEXT_BLOCK_OFFSET, &freeBlockHead, sizeof(int));
    // update the super block to point to the new free block
    memcpy(superData + FB_OFFSET, &blockNum, sizeof(int));
    // write the super block back to disk
    int writeSuccess = writeBlock(mountedDisk, SUPER_BLOCK, superData);
    if (writeSuccess < 0) {
        printf("LIBTINYFS-deallocateBlock: Issue with super block write when deallocating block\n");
        return EDEALLOC; // error
    }
    // write the free block back to disk
    writeSuccess = writeBlock(mountedDisk, blockNum, data);
    if (writeSuccess < 0) {
        printf("LIBTINYFS-deallocateBlock: Issue with free block write when deallocating block\n");
        return EDEALLOC; // error
    }
    return 1; // success
}

int tfs_writeFile(fileDescriptor FD,char *buffer, int size){
    if (mountedDisk == 0) {
        printf("LIBTINYFS: Error: No disk mounted. Cannot find file. (writeFile)\n");
        return EMOUNTFS; // error
    }

    // check if FD is in OFT
    openFileTableEntry *oftEntry = openFileTable[FD];
    if (oftEntry == NULL) {
        printf("LIBTINYFS: Error: File has not been opened. (writeFile)\n");
        return EBADFD; // error
    }
    int fileInode = oftEntry->inodeNumber;

    // read super block
    char *superData = (char *)malloc(BLOCKSIZE*sizeof(char));
    int success = readBlock(mountedDisk, SUPER_BLOCK, superData);

    if (success < 0) {
        free(superData);
        printf("LIBTINYFS: Error: Issue with super block read. (writeFile)\n");
        return EFREAD; // error
    }

    // if file open
    char *inodeData = (char *)malloc(BLOCKSIZE*sizeof(char)); // the block data of the file's inode
    success = readBlock(mountedDisk, fileInode, inodeData);
    if (success < 0) {
        free(superData);
        free(inodeData);
        printf("LIBTINYFS: Error: Issue with inode read. (writeFile)\n");
        return EFREAD; // error
    }
    
    int currentFileSize; // get file size, used for computation
    memcpy(&currentFileSize, inodeData + INODE_FILE_SIZE_OFFSET, sizeof(int));
    
    int dataBlock; // are there any data blocks that are currently used by the file
    memcpy(&dataBlock, inodeData + INODE_DATA_BLOCK_OFFSET, sizeof(int));

    // IMPLEMENTATION : Overwrite current data, write until cannot write anymore, then error
    int blocksNeeded = size / USEABLE_DATA_SIZE + (size % USEABLE_DATA_SIZE > 0 ? 1 : 0); // number of blocks needed
    int bufferPointer = 0;
    int remainingBytes = size;

    // Check if there are current data blocks under the file
    // if new blocks are needed, get pointer on super block on disk to get next free block
    if (currentFileSize != 0){ // free all data blocks being used right now
        // for each data extent block
        int blocksToDeallocate = currentFileSize/USEABLE_DATA_SIZE + (size % USEABLE_DATA_SIZE > 0 ? 1 : 0);
        for (int i=0; i<blocksToDeallocate; i++) {
            char *dataBuffer = (char *)malloc(BLOCKSIZE*sizeof(char)); // the block data of the file's current data extent block
            success = readBlock(mountedDisk, dataBlock, dataBuffer);

            if (success < 0) {
                free(inodeData);
                free(superData);
                printf("LIBTINYFS: Error: Data block could not be read. (writeFile)\n");
                return EFREAD; // error
            }
            
            // get the next data block before deallocating
            int nextBlock;
            memcpy(&nextBlock, dataBuffer + DATA_NEXT_BLOCK_OFFSET, sizeof(int));

            // deallocate the block
            success = deallocateBlock(dataBlock);
            if (success < 0) {
                free(inodeData);
                free(superData);
                printf("LIBTINYFS: Error: Could not deallocate data block. (writeFile)\n");
                return EDEALLOC; // error
            }

            dataBlock = nextBlock;
            free(dataBuffer);
        }
        
    }

    // get free block head which is a block number
    int freeBlock;
    memcpy(&freeBlock, superData + FB_OFFSET, sizeof(int));
    int dataExtentHead = freeBlock;

    // write to free blocks
    char *freeBuffer = (char *)malloc(BLOCKSIZE*sizeof(char));
    while (blocksNeeded != 0) { // done writing
        success = readBlock(mountedDisk, freeBlock, freeBuffer);

        if (success < 0) {
            free(inodeData);
            free(superData);
            free(freeBuffer);
            printf("LIBTINYFS: Error: Free block could not be read. (writeFile)\n");
            return EFREAD; // error
        }

        // edit block buffer
        freeBuffer[BLOCK_NUMBER_OFFSET] = DATA_BLOCK_TYPE; // change type to a data extent block
        int writeBufferSize = (remainingBytes >= USEABLE_DATA_SIZE ? USEABLE_DATA_SIZE : remainingBytes)*sizeof(char);
        memcpy(freeBuffer + DATA_BLOCK_DATA_OFFSET, buffer + bufferPointer, writeBufferSize); // copy the spliced buffer into the block data
        bufferPointer = bufferPointer + writeBufferSize;
        remainingBytes = remainingBytes - writeBufferSize;

        // get the next free block 
        int dataBlock = freeBlock;
        memcpy(&freeBlock, freeBuffer + FREE_NEXT_BLOCK_OFFSET, sizeof(int));

        // decrement blocks needed
        blocksNeeded--;

        if (blocksNeeded == 0) { // null next block for tail of data extent, need to save the next block pointer
            int zero = 0;
            memcpy(freeBuffer + DATA_NEXT_BLOCK_OFFSET, &zero, sizeof(int));
        }

        // write to block
        success = writeBlock(mountedDisk, dataBlock, freeBuffer);

        if (success < 0) {
            free(inodeData);
            free(superData);
            free(freeBuffer);
            printf("LIBTINYFS: Error: Free block could not be written to. (writeFile)\n");
            return EFWRITE; // error
        }

        // did we run out of free blocks before finishing?
        if (freeBlock == 0 && blocksNeeded != 0) {
            break;
        }

    }

    // UPDATE SUPER NODE
    memcpy(superData + FB_OFFSET, &freeBlock, sizeof(int)); // was IB offset
    success = writeBlock(mountedDisk, SUPER_BLOCK, superData);
    if (success < 0) {
        free(inodeData);
        free(superData);
        free(freeBuffer);
        printf("LIBTINYFS: Error: Super block could not be updated. (writeFile)\n");
        return EFWRITE; // error
    }

    // UPDATE INODE BLOCK
    // update file size
    int finalSize = size - remainingBytes;
    memcpy(inodeData + INODE_FILE_SIZE_OFFSET, &finalSize, sizeof(int));

    // change head of data extent
    memcpy(inodeData + INODE_DATA_BLOCK_OFFSET, &dataExtentHead, sizeof(int));

    // get current time to modify timestamp
    char * timeStampBuffer = (char *)malloc(TIMESTAMP_BUFFER_SIZE);
    getTimestamp(timeStampBuffer, TIMESTAMP_BUFFER_SIZE);
    memcpy(inodeData + INODE_MOD_TIME_STAMP_OFFSET, timeStampBuffer, TIMESTAMP_BUFFER_SIZE);
    free(timeStampBuffer);
    // write the updated inode block
    success = writeBlock(mountedDisk, fileInode, inodeData);
    if (success < 0) {
        free(inodeData);
        free(superData);
        free(freeBuffer);
        printf("LIBTINYFS: Error: Inode block could not be updated. (writeFile)\n");
        return EFWRITE; // error
    }

    oftEntry->filePointer = 0;

    // free memory
    free(inodeData);
    free(superData);
    free(freeBuffer);

    // error if incomplete write
    if (blocksNeeded > 0) {
        printf("LIBTINYFS: Error: No free blocks. Incomplete write (writeFile)\n");
        return EFWRITE; // error
    }

    return 1; // success
}

int tfs_deleteFile(fileDescriptor FD) {
    // remove the file from the inode linked list
    // deallocate all of its data blocks
    // add all of the above blocks to the free block linked list- make a function for this prob
    // remove the file from the open file table
    if (openFileTable[FD] == NULL) {
        printf("LIBTINYFS-deleteFile: invalid FD. Cannot delete file\n");
        return EBADFD; // error
    }
    int inodeToDelete = openFileTable[FD]->inodeNumber;
    // read in our inode LL head pointer from the super block
    char *superData = (char *)malloc(BLOCKSIZE);
    int success = readBlock(mountedDisk, SUPER_BLOCK, superData);   
    if (success < 0) {
        printf("LIBTINYFS-deleteFile: Issue with super block read when deleting file\n");
        return EDELETE; // error
    }
    // get root inode LL head pointer
    int curInode;
    char *curInodeData = (char *)malloc(BLOCKSIZE);
    memcpy(&curInode, superData + IB_OFFSET, sizeof(int));
    success = readBlock(mountedDisk, curInode, curInodeData);
    if (success < 0) {
        printf("LIBTINYFS-deleteFile: Invalid pointer to inode block\n");
        return EDELETE; // error
    }
    // check if inode to delete is the head of the inode LL
    if (curInode == inodeToDelete) {
        // inode to delete is the head of the inode LL
        // update the super block to point to the next inode
        memcpy(superData + IB_OFFSET, curInodeData + INODE_NEXT_INODE_OFFSET, sizeof(int));
        // write the super block back to disk
        int writeSuccess = writeBlock(mountedDisk, SUPER_BLOCK, superData); 
        if (writeSuccess < 0) {
            printf("LIBTINYFS-deleteFile: Issue with super block write when deleting file\n");
            return EDELETE; // error
        }
    }
    else { // inode to delete is not at the head of the linked list
        // iterate through the inode LL until we find the inode to delete
        int nextInode;
        memcpy(&nextInode, curInodeData + INODE_NEXT_INODE_OFFSET, sizeof(int));
        while (nextInode != inodeToDelete) {
            // read in the next inode
            success = readBlock(mountedDisk, nextInode, curInodeData);
            if (success < 0) {
                printf("LIBTINYFS-deleteFile: Invalid pointer to inode block\n");
                return EDELETE; // error
            }
            curInode = nextInode;
            // get the next inode
            memcpy(&nextInode, curInodeData + INODE_NEXT_INODE_OFFSET, sizeof(int));
        }
        char *nextInodeData = (char *)malloc(BLOCKSIZE);
        // read in the next inode data
        success = readBlock(mountedDisk, nextInode, nextInodeData);
        if (success < 0) {
            printf("LIBTINYFS-deleteFile: Invalid pointer to inode block\n");
            return EDELETE; // error
        }
        // now we have the inode before the inode to delete
        // update the inode before the inode to delete to point to the inode after the inode to delete
        int inodeAfterToDelete;
        memcpy(&inodeAfterToDelete, nextInodeData + INODE_NEXT_INODE_OFFSET, sizeof(int));
        memcpy(curInodeData + INODE_NEXT_INODE_OFFSET, &inodeAfterToDelete, sizeof(int));
        // write the inode before the inode to delete back to disk
        int writeSuccess = writeBlock(mountedDisk, curInode, curInodeData);
        if (writeSuccess < 0) {
            printf("LIBTINYFS-deleteFile: Issue with inode block write when deleting file\n");
            return EDELETE; // error
        }
        free(nextInodeData);
    }
    // now that we have removed the inode from the inode LL, deallocate the inode and all of its data blocks
    // read in the inode data
    success = readBlock(mountedDisk, inodeToDelete, curInodeData);
    if (success < 0) {
        printf("LIBTINYFS-deleteFile: Invalid pointer to inode block\n");
        return EDELETE; // error
    }
    // get the data block pointer
    int dataBlockPointer;
    memcpy(&dataBlockPointer, curInodeData + INODE_DATA_BLOCK_OFFSET, sizeof(int));
    if (dataBlockPointer != 0) { // need to deallocate data blocks
        // deallocate the data blocks
        char *dataBlock = (char *)malloc(BLOCKSIZE);
        while (1) {
            success = readBlock(mountedDisk, dataBlockPointer, dataBlock);
            if (success < 0) {
                printf("LIBTINYFS-deleteFile: Invalid pointer to data block\n");
                return EDELETE; // error
            }
            // get the next data block pointer
            int nextDataBlockPointer;
            memcpy(&nextDataBlockPointer, dataBlock + DATA_NEXT_BLOCK_OFFSET, sizeof(int));
            // deallocate the data block
            deallocateBlock(dataBlockPointer);
            // update the data block pointer
            dataBlockPointer = nextDataBlockPointer;
            if (dataBlockPointer == 0) {
                // no more data blocks to deallocate
                break;
            }
        }
        free(dataBlock);
    }
    // deallocate the inode
    deallocateBlock(inodeToDelete);
    tfs_closeFile(FD);
    free(superData);
    free(curInodeData);
    return 1; // success
}

int tfs_seek(fileDescriptor FD, int offset){
    if (mountedDisk == INT_NULL) {
        printf("LIBTINYFS: Error: No disk mounted. Cannot find file. (seek)\n");
        return EMOUNTFS; // error
    }

    // check if FD is in OFT
    openFileTableEntry *oftEntry = openFileTable[FD];
    if (oftEntry == NULL) {
        printf("LIBTINYFS: Error: File has not been opened. (seek)\n");
        return EBADFD; // error
    }

    int fp = oftEntry->filePointer + offset;
    oftEntry->filePointer = fp;

    return fp; // success, returns new file pointer
}

int tfs_readByte(fileDescriptor FD, char *buffer){
    if (mountedDisk == INT_NULL) {
        printf("LIBTINYFS: Error: No disk mounted. Cannot find file. (readByte)\n");
        return EMOUNTFS; // error
    }

    // check if FD is in OFT
    openFileTableEntry *oftEntry = openFileTable[FD];
    if (oftEntry == NULL) {
        printf("LIBTINYFS: Error: File has not been opened. (readByte)\n");
        return EBADFD; // error
    }
    int fileInode = oftEntry->inodeNumber;
    int filePointer = oftEntry->filePointer;

    // if file open
    char *inodeData = (char *)malloc(BLOCKSIZE*sizeof(char)); // the block data of the file's inode
    int success = readBlock(mountedDisk, fileInode, inodeData);
    if (success < 0) {
        free(inodeData);
        printf("LIBTINYFS: Error: Issue with inode read. (readByte)\n");
        return EFREAD; // error
    }
    
    int currentFileSize; // get file size, used for computation
    memcpy(&currentFileSize, inodeData + INODE_FILE_SIZE_OFFSET, sizeof(int));
    
    int dataBlock; // are there any data blocks that are currently used by the file
    memcpy(&dataBlock, inodeData + INODE_DATA_BLOCK_OFFSET, sizeof(int));

    if (filePointer >= currentFileSize) {
        free(inodeData);
        printf("\nLIBTINYFS: Error: File pointer out of bounds, EOF. (readByte)\n");
        return EBREAD; // error
    }

    int blockNumber = filePointer / USEABLE_DATA_SIZE; // which block to seek to
    int byteNumber = filePointer % USEABLE_DATA_SIZE; // which byte to seek to in blockNumber

    // printf("Block Num: %d, Byte Num:%d\n", blockNumber, byteNumber);
    
    char *blockData = (char *)malloc(BLOCKSIZE*sizeof(char));
    success = readBlock(mountedDisk, dataBlock, blockData);
    if (success < 0) {
        free(inodeData);
        free(blockData);
        printf("LIBTINYFS: Error: Issue with data read. (readByte)\n");
        return EFREAD; // error
    }
    while (blockNumber != 0) {
        memcpy(&dataBlock, blockData + DATA_NEXT_BLOCK_OFFSET, sizeof(int)); // get the next data block
        success = readBlock(mountedDisk, dataBlock, blockData);
        if (success < 0) {
            free(inodeData);
            free(blockData);
            printf("LIBTINYFS: Error: Issue with data read. (readByte)\n");
            return EFREAD; // error
        }

        blockNumber--; // decrement block number
    }

    memcpy(buffer, blockData + DATA_BLOCK_DATA_OFFSET + byteNumber, sizeof(char)); // get byte of data at byteNumbe in blockNumber 

    tfs_seek(FD, 1); // increment pointer

    // UPDATE INODE BLOCK
    // get current time to modify timestamp
    char * timeStampBuffer = (char *)malloc(TIMESTAMP_BUFFER_SIZE);
    getTimestamp(timeStampBuffer, TIMESTAMP_BUFFER_SIZE);
    memcpy(inodeData + INODE_ACC_TIME_STAMP_OFFSET, timeStampBuffer, TIMESTAMP_BUFFER_SIZE);
    free(timeStampBuffer);
    // write the updated inode block
    success = writeBlock(mountedDisk, fileInode, inodeData);
    if (success < 0) {
        free(inodeData);
        free(blockData);
        printf("LIBTINYFS: Error: Inode block could not be updated. (readByte)\n");
        return EFWRITE; // error
    }

    // free memory
    free(inodeData);
    free(blockData);

    return 1; // success
}

int tfs_readFileInfo(fileDescriptor FD) {
    if (openFileTable[FD] == NULL) {
        printf("LIBTINYFS-readFileInfo: File is not open. Cannot read file info\n");
        return EOPEN; // error
    }
    // read in the inode
    char *inodeData = (char *)malloc(BLOCKSIZE);
    int success = readBlock(mountedDisk, openFileTable[FD]->inodeNumber, inodeData);
    if (success < 0) {
        printf("LIBTINYFS-readFileInfo: Invalid pointer to inode block\n");
        return EFREAD; // error
    }
    // get the three time stamps and display them
    char *fileName = (char *)malloc(MAX_FILE_NAME_SIZE);
    int fileSize;
    char *created = (char *)malloc(TIMESTAMP_BUFFER_SIZE);
    char *modified = (char *)malloc(TIMESTAMP_BUFFER_SIZE);
    char *accessed = (char *)malloc(TIMESTAMP_BUFFER_SIZE);
    memcpy(fileName, inodeData + INODE_FILE_NAME_OFFSET, MAX_FILE_NAME_SIZE);
    memcpy(&fileSize, inodeData + INODE_FILE_SIZE_OFFSET, sizeof(int));
    memcpy(created, inodeData + INODE_CR8_TIME_STAMP_OFFSET, TIMESTAMP_BUFFER_SIZE);
    memcpy(modified, inodeData + INODE_MOD_TIME_STAMP_OFFSET, TIMESTAMP_BUFFER_SIZE);
    memcpy(accessed, inodeData + INODE_ACC_TIME_STAMP_OFFSET, TIMESTAMP_BUFFER_SIZE);
    printf("\n%s Information:", fileName);
    printf("\nFile Size: %d\n", fileSize);
    printf("Created: %s\n", created);
    printf("Modified: %s\n", modified);
    printf("Accessed: %s\n\n", accessed);
    free(fileName);
    free(created);
    free(modified);
    free(accessed);
    return 1; // success
}

int tfs_readdir() {
    if (mountedDisk == INT_NULL) {
        printf("LIBTINYFS: Error: No disk mounted. Cannot find file. (readdir)\n");
        return EMOUNTFS; // error
    }

    // read super block
    char *superData = (char *)malloc(BLOCKSIZE*sizeof(char));
    int success = readBlock(mountedDisk, SUPER_BLOCK, superData);

    if (success < 0) {
        free(superData);
        printf("LIBTINYFS: Error: Issue with super block read. (readdir)\n");
        return EFREAD; // error
    }

    // get inode head
    int inodeHead;
    memcpy(&inodeHead, superData + IB_OFFSET, sizeof(int));
    printf("\nFILE SYSTEM:\nroot directory:\n");
    char *inodeData = (char *)malloc(BLOCKSIZE*sizeof(char));
    while (inodeHead != 0) { // make this a recursive function for hierarchical
        success = readBlock(mountedDisk, inodeHead, inodeData);
        if (success < 0) {
            free(superData);
            free(inodeData);
            printf("LIBTINYFS: Error: Issue with inode block read. (readdir)\n");
            return EFREAD; // error
        }

        char *fileName = (char *)malloc(MAX_FILE_NAME_SIZE*sizeof(char));
        memcpy(fileName, inodeData + INODE_FILE_NAME_OFFSET, MAX_FILE_NAME_SIZE*sizeof(char));

        printf("%s\n", fileName);

        memcpy(&inodeHead, inodeData + INODE_NEXT_INODE_OFFSET, sizeof(int));
    }
    printf("\n");

    return 1; // success
}

int tfs_rename(fileDescriptor FD, char* newName) {
    if (strlen(newName) >= MAX_FILE_NAME_SIZE) {
        printf("LIBTINYFS: Error: File name is too long, cannot be supported. (rename)\n");
        return ERENAME; // error
    }

    if (mountedDisk == INT_NULL) {
        printf("LIBTINYFS: Error: No disk mounted. Cannot find file. (rename)\n");
        return EMOUNTFS; // error
    }

    // check if FD is in OFT
    openFileTableEntry *oftEntry = openFileTable[FD];
    if (oftEntry == NULL) {
        printf("LIBTINYFS: Error: File has not been opened. (rename)\n");
        return EBADFD; // error
    }
    int fileInode = oftEntry->inodeNumber;

    // get the inode block
    char *inodeData = (char *)malloc(BLOCKSIZE*sizeof(char));
    int success = readBlock(mountedDisk, fileInode, inodeData);
    if (success < 0) {
        free(inodeData);
        printf("LIBTINYFS: Error: Issue with inode block read. (rename)\n");
        return EFREAD; // error
    }

    // zero out the current file name
    memset(inodeData + INODE_FILE_NAME_OFFSET, 0, MAX_FILE_NAME_SIZE*sizeof(char));

    // MODIFY INODE BLOCK DATA
    // change name
    memcpy(inodeData + INODE_FILE_NAME_OFFSET, newName, strlen(newName)*sizeof(char)); 

    // get current time to modify timestamp
    char *timeStampBuffer = (char *)malloc(TIMESTAMP_BUFFER_SIZE);
    getTimestamp(timeStampBuffer, TIMESTAMP_BUFFER_SIZE);
    memcpy(inodeData + INODE_MOD_TIME_STAMP_OFFSET, timeStampBuffer, TIMESTAMP_BUFFER_SIZE);
    free(timeStampBuffer);

    // write updated inode block
    success = writeBlock(mountedDisk, fileInode, inodeData);
    if (success < 0) {
        free(inodeData);
        printf("LIBTINYFS: Error: Issue with inode block write. (rename)\n");
        return EFREAD; // error
    }
    free(inodeData);

    return 1; // success

}