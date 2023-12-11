#ifndef libTinyFS_h
#define libTinyFS_h
/* The default size of the disk and file system block */
#define BLOCKSIZE 256

/* Your program should use a 10240 Byte disk size giving you 40 blocks
total. This is a default size. You must be able to support different
possible values */
#define DEFAULT_DISK_SIZE 10240

/* use this name for a default emulated disk file name */
#define DEFAULT_DISK_NAME “tinyFSDisk”

/* GENERAL DEFINITIONS */

#define MAGIC_NUMBER 0x44
#define BLOCK_NUMBER_OFFSET 0
#define MAGIC_NUMBER_OFFSET 1
#define TIMESTAMP_BUFFER_SIZE 25


/* SUPER BLOCK DEFINITIONS */
#define SUPER_BLOCK 0 // super block number
#define FB_OFFSET 2 // offset to get free block LL head from super block
#define IB_OFFSET 6 // offset to get inode LL head from super block

/* INODE BLOCK DEFINITIONS */
#define INODE_NEXT_INODE_OFFSET 2 // offset to get next inode in LL
#define INODE_FILE_SIZE_OFFSET 6 // offset to get file size from inode block
#define INODE_DATA_BLOCK_OFFSET 10 // offset to get data block LL pointer from inode block
#define INODE_FILE_NAME_OFFSET 14 // offset to get file name from inode block
#define INODE_TIME_STAMP_OFFSET 23 // offset to get time stamp from inode block



/* FREE BLOCK DEFINITIONS */
#define FREE_NEXT_BLOCK_OFFSET 2 // offset to get next free block from free block


#define MAX_FILE_NAME_SIZE 9 // include the null terminator

#define NEXT_INODE_OFFSET 2

#define MAX_BYTES 2147483647

#define USEABLE_DATA_SIZE

/* use as a special type to keep track of files */
typedef int fileDescriptor;


typedef struct openFileTableEntry {
    int inodeNumber; // pointer the the inode
    int filePointer; // pointer to the current location in the file
} openFileTableEntry;

int tfs_mkfs(char* filename, int nBytes);
/* Makes a blank TinyFS file system of size nBytes on the unix file
specified by ‘filename’. This function should use the emulated disk
library to open the specified unix file, and upon success, format the
file to be a mountable disk. This includes initializing all data to 0x00,
setting magic numbers, initializing and writing the superblock and
inodes, etc. Must return a specified success/error code. */

int tfs_mount(char* diskname);
int tfs_unmount(void);
/* tfs_mount(char *diskname) “mounts” a TinyFS file system located within
‘diskname’. tfs_unmount(void) “unmounts” the currently mounted file
system. As part of the mount operation, tfs_mount should verify the file
system is the correct type. In tinyFS, only one file system may be
mounted at a time. Use tfs_unmount to cleanly unmount the currently
mounted file system. Must return a specified success/error code. */

fileDescriptor tfs_openFile(char* name);
/* Creates or Opens a file for reading and writing on the currently
mounted file system. Creates a dynamic resource table entry for the file,
and returns a file descriptor (integer) that can be used to reference
this entry while the filesystem is mounted. */

int tfs_closeFile(fileDescriptor FD);
/* Closes the file, de-allocates all system resources, and removes table
entry */

int tfs_writeFile(fileDescriptor FD, char* buffer, int size);
/* Writes buffer ‘buffer’ of size ‘size’, which represents an entire
file’s content, to the file system. Previous content (if any) will be
completely lost. Sets the file pointer to 0 (the start of file) when
done. Returns success/error codes. */

int tfs_deleteFile(fileDescriptor FD);
/* deletes a file and marks its blocks as free on disk. */

int tfs_readByte(fileDescriptor FD, char* buffer);
/* reads one byte from the file and copies it to buffer, using the
current file pointer location and incrementing it by one upon success.
If the file pointer is already past the end of the file then
tfs_readByte() should return an error and not increment the file pointer.
*/

int tfs_seek(fileDescriptor FD, int offset);
/* change the file pointer location to offset (absolute). Returns
success/error codes.*/

// EXTRA CREDIT FUNCTIONS:

int tfs_rename(fileDescriptor FD, char* newName); /* renames a
file. New name should be passed in. File has to be open. */

int tfs_readdir(); /* lists all the files and directories on the disk, print the
list to stdout -- Note: if you don’t have hierarchical directories, this just reads
the root directory aka “all files” */

int tfs_readFileInfo(fileDescriptor FD); /* returns the file’s
creation time or all info (up to you if you want to make 
multiple functions) */

#endif