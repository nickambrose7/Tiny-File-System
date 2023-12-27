# Tiny Filesystem

# Authors:
Nicholas Ambrose, Ethan Wagner


# TinyFS Implementation: Leveraging Linked Lists
Our TinyFS implementation utilizes linked lists, a fundamental data structure. We use linked lists to manage our free blocks, file extent blocks, and inode blocks efficiently. While linked lists are not the fastest, with O(n) search complexity impacting operations like locating a specific inode by file name, they offer distinct advantages. Notably, the O(1) insertion time at the head of the list is a critical feature we capitalize on in TinyFS, optimizing for frequent insertions. However, this choice does entail trade-offs, particularly in deletion operations, where removing elements other than at the head incurs O(n) time, impacting our system's performance in file deletions. Despite these compromises, our implementation leverages the strengths of linked lists to balance overall system efficiency and functionality.

# Enhancing TinyFS: Advanced Features
In extending the capabilities of our file system, we chose to integrate timestamps, file renaming, and directory listing. These features enhance user interaction and system utility. Our demo showcases the dynamic updating of timestamps aligned with file access, modifications, and creations. Additionally, we highlight the seamless process of file renaming and the practicality of directory listing, offering a glimpse into the versatile nature of our system.

# Understanding TinyFS Limitations and Reliability
TinyFS is free of bugs, ensuring a stable and reliable file system experience. While it doesn't encompass the complete array of features found in full-scale file systems, it excels within its defined scope and specifications. It's important to note that due to the absence of hierarchical directories, some performance aspects are not optimized to their fullest potential. Nonetheless, TinyFS stands as a functional system, tailored to meet specific user needs and operational requirements.

