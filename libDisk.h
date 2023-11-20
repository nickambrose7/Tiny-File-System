#ifndef libDisk_h
#define libDisk_h
#define BLOCK_SIZE 256

// Global variables
extern int diskCounter;

// Function prototypes

int openDisk(char *filename, int nBytes);
int closeDisk(int disk);
int readBlock(int disk, int bNum, void *block);
int writeBlock(int disk, int bNum, void *block);

// Struct to hold the disk information
typedef struct {
    int disk;
    int nBytes;
    int offset; // Offset in blocks
} Disk;

#endif