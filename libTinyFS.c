#include "libTinyFS.h"
#include "libDisk.c"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


fileDescriptor currentfd = 0; // incremented each time we create a new file

openFileTableEntry *openFileTable; // array of open file table entries, indexed by file descriptor

Disk mountedDisk = NULL; // currently mounted Disk

int tfs_mkfs(char *filename, int nBytes){
    /******************** BLOCK STRUCTURE DOCUMENTATION ****************************/
    /* 
    * BLOCKSIZE = 256 bytes
    * The documentation below assumes you are starting at position 0 in each block:


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
    
    int diskNum = openDisk(filename, nBytes);
    if (diskNum < 0) {
        perror("LIBTINYFS: error creating disk in tfs_mkfs");
        return -1; // error 
    }

    /*IMPORTANT: STRUCTURE OF OUR SUPER BLOCK BELOW: */
    char *data = (char *)malloc(BLOCKSIZE);
    data[0] = 1; // block type -> super block
    data[1] = MAGIC_NUMBER;
    int numInodes = ((nBytes / BLOCKSIZE) - 1) / 2;  // calculate the number of inodes needed
    data[2] = 1; // free inode list head
    data[4] = 1 + numInodes; // free block list head
    // write the super block to the disk
    int writeSuccess = writeBlock(diskNum, 0, data);
    if (writeSuccess < 0) {
        perror("LIBTINYFS: Error writing super block to disk");
        return -1; // error
    }
    /* INODE INITIALIZATION: */
    for (int i = 1; i < numInodes + 1; i++) {
        // setup each inode block
        char *data = (char *)malloc(BLOCKSIZE*sizeof(char));
        data[0] = 2; // block type -> inode block
        data[1] = MAGIC_NUMBER;
        // set up linked list for free inode list
        if (i < numInodes) {
            data[2] = i + 1; // next free inode
        } else {
            data[2] = 0; // no more free inodes, 0 means end of list
        }
        // set up inode
        data[4] = 0; // file size
        // data[5] -> pointer to the first data block -- fill in later
        // data[6] -> file name -- fill in later
        int writeSuccess = writeBlock(diskNum, i, data);
        if (writeSuccess < 0) {
            // print out the block number that failed to write
            printf("LIBTINYFS: Error writing inode block %d to disk", i);
            return -1; // error
        }
    }
    // Free block list implementation
    int freeBlockListHead = 1 + numInodes; // the first free block is the first block after the inode blocks
    for (int i = freeBlockListHead; i < nBytes / BLOCKSIZE; i++) {
        // setup each free block
        char *data = (char *)malloc(BLOCKSIZE*sizeof(char));
        data[0] = 4; // block type -> free block
        data[1] = MAGIC_NUMBER;
        // set up linked list for free block list
        if (i < nBytes / BLOCKSIZE - 1) {
            data[2] = i + 1; // next free block
        } else {
            data[2] = 0; // no more free blocks, 0 means end of list
        }
        int writeSuccess = writeBlock(diskNum, i, data);
        if (writeSuccess < 0) {
            // print out the block number that failed to write
            printf("LIBTINYFS: Error writing free block %d to disk", i);
            return -1; // error
        }
    }
    return 1; // success
    /* FUNCTION DONE*/

    /* BELOW IS INFO ABOUT HOW WE WILL MANIPULATE OUR LINKED LISTS
       REMEMBER WE HAVE ONE LINKED LIST TO KEEP TRACK OF OUR FREE INODES
       ONE TO KEEP TRACK OF OUR FREE DATA BLOCKS
       AND ONE TO KEEP TRACK OF OUR DATA BLOCKS THAT ARE IN USE (FOR EACH FILE)

    - Initialize the superblock's free block pointer to point to the first free block
    (which will be the first block after the superblock and inode blocks).
    - Init each consecutive free blocks pointer to point to the next free block.
    (the one right next to it)
    - Pointers in this case are just block numbers, not addresses.
    - The last free block's pointer should be set to 0. This is the tail of the list.
    - To get a free block, read the free block pointer, then update the superblock's
    free block pointer to point to the next free block.
    - To add a free block, set the free block pointer of the block you want to add to
    point to the current free block pointer, then update the superblock's free block
    pointer to point to the block you just added. So the superblock's free block pointer
    essentiall acts as the dummy head of the free block list.
    */
}

int tfs_mount(char *diskname){
    // check if there is already a disk mounted...only one disk can be mounted at a time
    if(mountedDisk != NULL) {
        perror("LIBTINYFS: Error: A disk is already mounted, unmount current\ndisk to mount a new disk");
        return -1 // error 
    }
    // check if successfully retrieved the disk number
    int diskNum = openDisk(diskname, 0);
    if (diskNum == -1) {
        perror("LIBTINYFS: Error: See LIBDISK error.");
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
        perror("LIBTINYFS: Error: Invalid magic number") // error
    }
    //deallocate buffer
    free(data);
    return diskNum; // success
}

int tfs_unmount(void){
    // is there a disk mount?
    if (mountedDisk == NULL) {
        perror("LIBTINYFS: No mounted disk");
        return -1; // error
    }
    // unmount the currently mounted disk
    mountedDisk = NULL;
    return 1; // success
}

fileDescriptor tfs_openFile(char *name){
    // search our inode list for that file name. 
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