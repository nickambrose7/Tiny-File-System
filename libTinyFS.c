#include "libTinyFS.h"
#include "libDisk.c"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int MAGIC_NUMBER = 44; // Magic number for TinyFS

fileDescriptor currentfd = 0; // incremented each time we create a new file

openFileTableEntry *openFileTable; // array of open file table entries, indexed by file descriptor

Disk mountedDisk = NULL; // currently mounted Disk

int tfs_mkfs(char *filename, int nBytes){
    int diskNum = openDisk(filename, nBytes);
    if (diskNum < 0) {
        perror("LIBTINYFS: error creating disk in tfs_mkfs");
        return -1; // error 
    }
    // format the disk so that it is ready to be mounted
    // initialize the super block
    // init the correct number of inodes
    // Free block list implementation?
    /*
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