# OS_File_System
- This file system is a simplified version of a typical UNIX file system and thus
serves to introduce some of the basic on-disk structures, access methods, and
various policies that you will find in many file systems today.


## Architecture Overview:
The following things will be present in the application:
## Section1:
This is the region outside the disks. From here you will create a disk and then
mount/open it to perform all basic operations mentioned in section 2.
- create disk: Creates an empty disk of size 500Mb.
While creating an empty disk a unique name will be given to it which will be used
to mount it.
- mount disk: Opens the specified disk for various file operations. As mentioned
in section 2.
- exit: Close the application.

## Section 2:
This is the region inside the disk. You may have multiple disks. You will open only 1 disk
at a time. After opening/mounting a particular disk, you will perform below mentioned
operations in the disk:
1. create file: creates an empty text file.
2. open file: opens a particular file in read/write/append mode as specified in input,
multiple files can be opened simultaneously.
3. read file: Displays the content of the file.
4. write file: Write fresh data to file(override previous data in file).
5. append file: Append new data to an existing file data.
6. close file: Closes the file.
7. delete file: Deletes the file.
8. list of files: List all files present in the current disk.
9. list of opened files: List all opened files and specify the mode they are open in.
10. unmount: Closes the currently mounted disk.

## Working:
For this assignment, you have to perform operations in a menu-driven fashion as
specified below:
## Section 1:
Disk Menu:
1. create disk
- On press 1: Take unique disk name as input on next line.
2. mount disk
- On press 2: Open the disk for mounting purposes(display options of section2 for
that disk).
3. exit
- On press 3: Exit the application.

## Section 2:
Mounting Menu:
1. create file
- On press 1: Take unique file name as input on next line.
2. open file
- On press 2: Take file name as input on next line.
Then take file mode as input as mentioned below on next line:
0. read mode
1. write mode
2. append mode
** After specifying the mode, display the file descriptor allocated to the opened
file along with the mode in which the file is opened.
3. read file
- On press 3: Take input file descriptor of the file which you want to read.
- File descriptor has been obtained in the open file command.
4. write file
- On press 4: Take input file descriptor of the file which you want to write.
- File descriptor has been obtained in the open file command.
- Enter file content that you want to write in the file.
5. append file
- On press 5: Take input file descriptor of the file to which you want to append
further text.
- File descriptor has been obtained in the open file command.
- Enter the file content that you want to append to the file.
6. close file
- On press 6: Take the input file descriptor of the file to which you want to close.
7. delete file
- On press 7: Take the input file name which you want to delete.
8. list of files
- On press 8: List all existing files on the disk.
9. list of opened files
- On press 8: List all existing files which are currently open along with their file
descriptors and mode in which they are open.
10. unmount
- On press 10: Unmount/close the disk which is current mount(in which you are
working currently) and return to the previous menu.
