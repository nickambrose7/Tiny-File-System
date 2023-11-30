#include "libTinyFS.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

fileDescriptor currentfd = 0; // incremented each time we create a new file

openFileTableEntry *openFileTable; // array of open file table entries, indexed by file descriptor

// now write our functions
