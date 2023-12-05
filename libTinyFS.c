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
    
    // return 1 or new disk number?

}

int tfs_mount(char *diskname){
    // check if there is already a disk mounted...only one disk can be mounted at a time
    if(mountedDisk != NULL) { // do you want to automatically unmount the currently mounted disk or nah?
        perror("LIBTINYFS: Error: A disk is already mounted, unmount current\ndisk to mount a new disk");
        return -1 // error 
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
        perror("LIBTINYFS: No disk to unmount");
        return -1; // error
    }
    // unmount the currently mounted disk
    mountedDisk = NULL;
    return 1; // success, do you want to return unmounted disk num instead?
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