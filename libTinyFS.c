#include "libTinyFS.h"
#include "libDisk.c"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>


fileDescriptor currentfd = 0; // incremented each time we create a new file

openFileTableEntry *openFileTable; // array of open file table entries, indexed by file descriptor

Disk* mountedDisk = NULL; // currently mounted Disk

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
        perror("LIBTINYFS: error creating disk in tfs_mkfs");
        return -1; // error 
    }

    /* SUPERBLOCK INIT */
    char *data = (char *)malloc(BLOCKSIZE);
    memset(data, 0, BLOCKSIZE); // zero out the data buffer
    data[0] = 1; // block type -> super block
    data[1] = MAGIC_NUMBER;
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
        data[0] = 4; // block type -> inode block
        data[1] = MAGIC_NUMBER;
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
    if(mountedDisk != NULL) { // do you want to automatically unmount the currently mounted disk or nah?
        perror("LIBTINYFS: Error: A disk is already mounted, unmount current\ndisk to mount a new disk");
        return -1; // error 
    }
    // check if successfully retrieved the disk number
    int diskNum = openDisk(diskname, 0);
    if (diskNum == -1) {
        perror("LIBTINYFS: Error: Could not open disk");
        return -1; // error 
    }
    // check the file system type
    // make a block sized buffer
    char *data = (char *)malloc(BLOCKSIZE*sizeof(char));
    // read the super block
    int readSuccess = readBlock(diskNum, 0, data);
    // successful read? correct FS type? 
    // ---NICK: Check the magic number in a loop, it should be on every block.
    if (readSuccess != 1 || data[2] != MAGIC_NUMBER) {
        perror("LIBTINYFS: Error: Invalid magic number"); // error
    }
    //deallocate buffer
    free(data);
    return diskNum; // success
}

int tfs_unmount(void){
    // is there a disk mount?
    if (mountedDisk == NULL) {
        perror("LIBTINYFS: No disk to unmount");
        return -1; // error
    }
    // unmount the currently mounted disk
    mountedDisk = NULL;
    return 1; // success, do you want to return unmounted disk num instead?
}

fileDescriptor tfs_openFile(char *name){
    // search our inode list for that file name
    
    // if not in our list, allocate a new inode for the file

}
int tfs_closeFile(fileDescriptor FD){
    
}
int tfs_writeFile(fileDescriptor FD,char *buffer, int size){
    
}
int tfs_deleteFile(fileDescriptor FD){
    
}
int tfs_readByte(fileDescriptor FD, char *buffer){
    
}
int tfs_seek(fileDescriptor FD, int offset){
    
}