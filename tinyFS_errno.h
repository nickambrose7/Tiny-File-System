/*
implement the codes as a set of statically defined macros.

I'm going to just leave each error code as -1 with a print statement
and then choose how to define our error codes later.

Take a look at man 3 errno on the UNIX* machines for examples of the types of errors
you might want to catch and report
*/

#ifndef TINYFS_ERRNO_H
#define TINYFS_ERRNO_H

// bad file descriptor
#define EBADFD -1
// disk is full
#define ENOSPC -2
// file too large
#define EFBIG -3
// file system creation error
#define ECREATFS -4
// file system mounting error
#define EMOUNTFS -5
// file system unmounting error
#define EUNMOUNTFS -6
// file open error
#define EOPEN -7
// file close error
#define ECLOSE -8
// file deletion error
#define EDELETE -9
// deallocation error
#define EDEALLOC -10
// file info read error
#define EFREAD -11

#endif