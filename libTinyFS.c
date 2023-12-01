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

}

int tfs_mount(char *diskname){
    // check if there is already a disk mounted...only one disk can be mounted at a time
    if(mountedDisk != NULL) {
        perror("LIBTINYFS: A disk is already mounted, unmount current\ndisk to mount a new disk");
        return -1
    }
    // check if successfully retrieved the disk number
    int diskNum = openDisk(diskname, 0);
    if (diskNum == -1) {
        perror("LIBTINYFS: See LIBDISK error.");
        return -1;
    }
    int magicNumber = readBlock(diskNum, 0, );
    if (magicNumber != MAGIC_NUMBER) {
        perror("LIBTINYFS: Invalid magic number")
    }
    return diskNum;
}

int tfs_unmount(void){
    if (mountedDisk == NULL) {
        perror("LIBTINYFS: No mounted disk");
        return -1;
    }
    mountedDisk = NULL;
    return 1;
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