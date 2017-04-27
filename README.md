# project3-fs

## folders

The /src folder contains sources from the minix file system. We did not modify it, but we use various structures or defined values from these files.

The /tools folder contains the first implementations of the DirectoryWalker, InodeBitmapWalker and ZoneBitmapWalker that we made while dicovering how the Minix filesystem was implemented. These functions have been reimplemented as part of the /repair sources used to implement the filesystem repair tool.

The /repair folder contains the source of the filesytem repair tool, and the sources of an utility used to damage the system.

the /dev-tools folder contains utilities that we used during the development to update files on aur VM


## how to build the sources

A GNU makefile is provided in the /repair and /tools folders. You simply need to run `gmake` is those folders.

On the provided VM, these source files are located under the /home/repo directory.


## what our repair tool is able to do

Our repair tool is able to:
* detect incorrect values in the superblock
* compare the imap and the ilist to find inconsistency between the two, and fix it by changing imap values.
* compare the zmap and the zones used in inodes to find inconsistency between the two, and fix it by changing zmap values.
* check the directory tree by verifying . and .. links consistence. It is able to add missing values, or update incorrect ones to restore a complete directory tree.
* browse all directoy files to check file reachability/corrupted directories.

## VM link

https://drive.google.com/file/d/0B2pQrdghmjeKUFFnenM2Z2dvQ0E/view?usp=sharing
