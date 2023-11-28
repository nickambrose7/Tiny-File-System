#ifndef libDisk_h
#define libDisk_h
#define BLOCKSIZE 256
#include <stdio.h>

typedef struct Disk Disk; // Forward declaration

// Struct to hold the disk information
struct Disk {
    int diskNumber;    // unique disk identifier
    int nBytes;        // Size of the disk in bytes
    char *filename;    // Name of the backing file for our disk
    Disk *next;        // Pointer to the next disk in the list
    FILE *filePointer; // file pointer to the unix file
};


// Global variables
extern int diskCounter;
extern Disk *diskListHead;

// Function prototypes

int openDisk(char *filename, int nBytes);
int closeDisk(int disk);
int readBlock(int disk, int bNum, void *block);
int writeBlock(int disk, int bNum, void *block);



#endif
