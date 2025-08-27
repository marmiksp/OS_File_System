# Virtual_OS_File_System
- This file system is a simplified version of a typical UNIX file system and thus
serves to introduce some of the basic on-disk structures, access methods, and
various policies that you will find in many file systems today.


## Architecture Overview:
The following things will be present in the application:
#### Section1:
This is the region outside the disks. From here you will create a disk and then
mount/open it to perform all basic operations mentioned in section 2.
- create disk: Creates an empty disk of size 500Mb.
While creating an empty disk a unique name will be given to it which will be used
to mount it.
- mount disk: Opens the specified disk for various file operations. As mentioned
in section 2.
- exit: Close the application.

#### Section 2:
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
#### Section 1:
Disk Menu:
1. create disk
- On press 1: Take unique disk name as input on next line.
2. mount disk
- On press 2: Open the disk for mounting purposes(display options of section2 for
that disk).
3. exit
- On press 3: Exit the application.

#### Section 2:
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



# Virtual OS File System Implementation Analysis

This is a comprehensive implementation of a **virtual file system** in C++ that simulates disk operations using a regular file as storage. Let me break down the code in detail:

## 1. System Configuration & Constants

```cpp
#define DISK_BLOCKS 131072        /* ~512MB virtual disk (131072 * 4KB blocks) */
#define BLOCK_SIZE 4096           /* Standard 4KB block size */
#define NO_OF_INODES 32768        /* Maximum 32,768 files */
#define NO_OF_FILE_DESCRIPTORS 32 /* Up to 32 files can be open simultaneously */
```

**Analysis**: This creates a virtual disk of approximately 512MB with standard 4KB blocks, supporting up to 32,768 files.

## 2. Core Data Structures

### 2.1 Inode Structure (32 bytes)
```cpp
struct inode {
    int filesize;           // Size of file in bytes
    int first_block_num;    // Pointer to first data block
    char filename[24];      // File name (max 24 characters)
};
```

**Purpose**: Stores metadata for each file. Uses a **linked list allocation** strategy where each file has a chain of blocks.

### 2.2 Super Block Structure
```cpp
struct super_block {
    int nob_superblock;                    // Number of blocks for superblock
    int sti_inode;                         // Starting block of inode table
    int nob_inodes;                        // Number of blocks for inodes
    int sti_DB;                            // Starting block of data area
    int total_no_of_available_blocks;      // Free blocks available
    bool inode_freelist[NO_OF_INODES];     // Bitmap for free inodes
    bool datablock_freelist[DISK_BLOCKS];  // Bitmap for free blocks
    int next_disk_block[DISK_BLOCKS];      // Linked list pointers for blocks
};
```

**Purpose**: Contains file system metadata and free space management information.

### 2.3 File Descriptor Management
```cpp
struct fdnode {
    int in;    // Inode number
    int mo;    // Mode (0=read, 1=write, 2=append)
};

map<int, fdnode> fd_map;  // Maps file descriptor to inode and mode
```

## 3. Key Functions Analysis

### 3.1 `create_disk(string disk_name)`

**Purpose**: Creates a new virtual disk file and initializes the file system structure.

**Process**:
1. Creates a file of size `DISK_BLOCKS × BLOCK_SIZE`
2. Initializes superblock with metadata
3. Marks system blocks (superblock, inode table) as reserved
4. Initializes all inodes and data blocks as free
5. Writes superblock and empty inode table to disk

**Key Features**:
- Uses `fwrite()` to create the physical disk file
- Implements proper disk layout with reserved system areas
- Initializes free space management structures

### 3.2 `mount_disk(string disk_name)`

**Purpose**: Opens an existing virtual disk and loads file system structures into memory.

**Process**:
1. Opens the disk file for read/write access
2. Reads superblock from disk into memory
3. Reads inode table from disk
4. Reconstructs in-memory data structures:
   - `file_to_inode_map`: Maps filenames to inode numbers
   - `free_inode_vt`: Vector of available inodes
   - `free_DB_vt`: Vector of available data blocks
   - `free_fd_vt`: Vector of available file descriptors

### 3.3 `create_file(string filename)`

**Purpose**: Creates a new file entry in the file system.

**Process**:
1. Checks if inodes and data blocks are available
2. Verifies file doesn't already exist
3. Allocates a free inode and data block
4. Initializes inode with filename and first block pointer
5. Updates mapping structures

**File Allocation Strategy**: Each file gets at least one data block allocated immediately, even if empty.

### 3.4 `open_file(string filename)`

**Purpose**: Opens a file and returns a file descriptor.

**Features**:
- Supports three modes: READ (0), WRITE (1), APPEND (2)
- Prevents multiple write/append access to same file
- Allows multiple read access simultaneously
- Returns file descriptor for subsequent operations

### 3.5 `write_file(int fd)`

**Purpose**: Writes data to a file, replacing existing content.

**Process**:
1. Reads input until TAB character (ASCII 9)
2. **Deallocates all existing blocks** of the file
3. Calculates required blocks for new content
4. Allocates new blocks and creates linked chain
5. Writes data block by block using `fwrite()`

**Important**: This is a **complete overwrite** operation - existing data is lost.

### 3.6 `append_file(int fd)`

**Purpose**: Adds data to the end of an existing file.

**Complex Logic**:
1. Finds the last block of the file by traversing the linked list
2. Calculates remaining space in the last block
3. If new data fits in last block, writes there directly
4. Otherwise, fills remaining space in last block, then allocates new blocks
5. Maintains the linked list structure

### 3.7 `read_file(int fd)`

**Purpose**: Reads and displays entire file content.

**Process**:
1. Starts from first block of file
2. Follows linked list chain using `next_disk_block[]`
3. Reads each block and displays content
4. Handles partial last block correctly

### 3.8 `delete_file(string filename)`

**Purpose**: Removes a file from the file system.

**Safety Checks**:
1. Verifies file exists
2. Ensures file is not currently open
3. Deallocates all blocks in the file's chain
4. Frees the inode
5. Removes from filename mapping

## 4. Memory Management Strategy

### Block Allocation
- **Linked List Allocation**: Files use a chain of blocks connected via `next_disk_block[]` array
- **Immediate Allocation**: Each file gets at least one block upon creation
- **Dynamic Growth**: Files can grow by allocating additional blocks

### Free Space Management
- **Bitmap**: Uses boolean arrays to track free inodes and blocks
- **Vector Optimization**: Maintains vectors of free resources for O(1) allocation
- **Lazy Updates**: Free space bitmaps updated only during unmount

## 5. File System Layout

```
| Superblock | Inode Table | Data Blocks |
|    1-3     |    4-5      |   6-131072  |
```

- **Superblock**: Contains file system metadata
- **Inode Table**: Array of all file metadata structures
- **Data Blocks**: Actual file content storage

## 6. Limitations and Design Choices

### Limitations:
1. **No Directory Structure**: Flat file system only
2. **Fixed Block Size**: 4KB blocks for all files (inefficient for small files)

### Strengths:
1. **Complete Implementation**: All basic file operations supported
2. **Persistent Storage**: Data survives program restarts
3. **Memory Efficient**: Loads only necessary structures into RAM
4. **Flexible File Sizes**: Files can be any size up to disk capacity
5. **Safe Operations**: Proper error checking and validation

## 7. Technical Implementation Details

### Data Persistence
- Uses standard C file I/O (`fopen`, `fread`, `fwrite`, `fseek`)
- Binary format for efficiency
- Explicit disk layout management

### Error Handling
- Comprehensive validation for all operations
- Proper resource cleanup
- User-friendly error messages with color coding

### Performance Considerations
- **O(1) file creation** using free vectors
- **O(n) file deletion** where n = number of blocks in file
- **Sequential access optimized** for reading/writing

## 8. User Interface

The system provides a menu-driven interface supporting:
- Disk creation and mounting
- File CRUD operations
- File listing and status display
- Clean unmount with data persistence

This implementation demonstrates fundamental file system concepts including inodes, block allocation, free space management, and file descriptor handling, making it an excellent educational example of how operating systems manage persistent storage.
