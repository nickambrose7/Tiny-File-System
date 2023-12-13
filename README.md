# Project-4-Tiny-Filesystem

# Authors:
Nicholas Ambrose, Ethan Wagner


# Implementation:
Our TinyFS implementation relys heavily on the linked list data structure. We use a linked list to keep 
track of our free blocks, file extent blocks, and inode blocks. Linked lists are not the fastest data
structures. They take O(n) to search for a specific node, which comes into play frequently in a file
system. For example, if we want to find a specific inode based off of the file name, this will take 
O(n) time. The good news is, linked lists take O(1) to insert nodes at the head. In our TinyFS 
implementation, we often take advantage of this fact, always inserting into our linked lists
at their head. To remove, from a place other than the head of the list, linked lists take O(n) time.
This hurts our performance when deleting a file, where we have to find the inode to delete in our 
inode linked list. Overall, we took advantage of the fast insertion at the head that the linked 
list offers in our implementation of this file system. As a result of this choice, we sacrificed
performance on other operations, such as deleteion. 

# Additional Functionality:
For our additional functionality, we decided to implement timestamps, file renaming, and directory listing.
We show in our demo how timestamps are updated based on file access, modification, and creation. We also
display file renaming and directory listing in full effect in our demo. 

# Limitations/Bugs
Our file system is bug-free. It is limited in the fact that it does not provide the full suite of tools
that a regular file system does, but it functions well for what is is supposed to do according to the spec. 
