#ifndef libDisk_h
#define libDisk_h
#define BLOCK_SIZE 256

// Global variables
extern int diskCounter;
extern Disk *diskListHead;

// Function prototypes

int openDisk(char *filename, int nBytes);
int closeDisk(int disk);
int readBlock(int disk, int bNum, void *block);
int writeBlock(int disk, int bNum, void *block);

// Struct to hold the disk information
typedef struct {
    int disk; // unique disk identifier
    int nBytes; // Size of the disk in bytes
    char *filename; // Name of the backing file for our disk
    int offset; // Offset in blocks
    Disk *next; // Pointer to the next disk in the list
} Disk;

#endif