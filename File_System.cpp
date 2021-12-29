#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <bits/stdc++.h>
using namespace std;

// total disk size = 2KB
// total no of blocks = 2KB/64 = 32

// int DISK_BLOCKS = 32;       /* number of blocks on the disk      */
// int BLOCK_SIZE = 64;           /* block size on "disk"              */
// int NO_OF_INODES = 4;        /* In 25% possible number of inodes. */
// int NO_OF_FILE_DESCRIPTORS = 6; /* this is predefined                */
#define DISK_BLOCKS 131072        /* number of blocks on the disk      */
#define BLOCK_SIZE 4096           /* block size on "disk"              */
#define NO_OF_INODES 32768        /* In 60% possible number of inodes. */
#define NO_OF_FILE_DESCRIPTORS 32 /* this is predefined                */


#define RED "\033[1;31m"
#define GREEN "\033[1;32m"
#define BLUE "\033[1;34m"
#define YELLOW "\033[1;33m"
#define PURPLE "\033[1;35m"
#define LIGHTBLUE "\033[1;36m"
#define BOLD "\033[1m"
#define DEFAULT "\033[0m"



// Methods to be implemented.

int create_disk(string disk_name);           /* create an empty, virtual disk file          */
int mount_disk(string disk_name);             /* open a virtual disk (file)                  */
int create_file(string filename);            /* to create file */
int open_file(string filename);              /* open file to get its file descriptor        */
int write_file(int fd);
int read_file(int fd);
int append_file(int fd);
int close_file(int fd);
int delete_file(string filename);
int unmount_disk();
void print_list_open_files();
void print_list_files();








struct inode // total size = 32Byte
{
    int filesize;
    int first_block_num; 
    char filename[24];
};

// struct disk_block {
// 	char data[4092];
// 	int next_block_num;
// };

struct super_block /* super block structure */
{
    // 20+4+32+(4*32) = 184
    // 184/64 = 3 (No of Blocks)
    int nob_superblock = ceil(((float)sizeof(super_block)) / BLOCK_SIZE);

    /* It denotes the position from where inode starts. 744 */
    int sti_inode = nob_superblock ;

    /*  Total size required to store all inodes = 4*32 = 128B   */
    int nob_inodes = ceil(((float)(NO_OF_INODES * sizeof(struct inode))) / BLOCK_SIZE); // 2

    /* 3 + 2 = 5 (reserved blocks) */
    int sti_DB = nob_superblock  + nob_inodes;

    /* 32 - 5 = 27 (free blocks to store data) */
    int total_no_of_available_blocks = DISK_BLOCKS - sti_DB;

    bool inode_freelist[NO_OF_INODES];    // to check which inode no is free to assign to file
    bool datablock_freelist[DISK_BLOCKS]; //to check which data block is free to allocate to file
    int next_disk_block[DISK_BLOCKS];
};




/******************************************************************************/
// Data Structure to be used.
 struct fdnode
 {
    int in;
    int mo;
 };

 struct super_block sb;
 struct inode inode_arr[NO_OF_INODES];

 FILE *diskptr;

 char disk_name[50], filename[30];
 int openfile_count;                           // keeps track of number of files opened.
 map<string, int> file_to_inode_map;           // filename->inode file name as key maps to inode (value)(index in Inode Array)
 // map<int, string> inode_to_file_map;           // indoe-> filename inode to file mapping

 vector<int> free_inode_vt;                // denote free inodes
 vector<int> free_DB_vt;           // denote free data blocks

 vector<int> free_fd_vt;       // denote free filedescriptor.
 map<int, fdnode> fd_map; // fd->(inode,file_seek_ptr) Stores files Descriptor as key and corresponding Inode number(First) and file pointer.
int active = 0;





int create_disk(string disk_name)
{
    char buffer[BLOCK_SIZE];
    struct super_block sb; //initializing sb

    memset(buffer, 0, BLOCK_SIZE); // intialize null buffer

    if(access( disk_name.c_str(), F_OK ) != -1)
    {
        cout << string(RED) << "Virtual Disk already exists !!!" << string(DEFAULT) << "\n";
        return -1;
    }

    diskptr = fopen(disk_name.c_str(), "wb");

    /* Using buffer to initialize whole disk as NULL  */
    for (int i = 0; i < DISK_BLOCKS; ++i)
        fwrite(buffer, 1, BLOCK_SIZE, diskptr);



/* Marking all data bloks as free */

    for(int i=0;i<DISK_BLOCKS;i++)
    {
    	sb.next_disk_block[i]=-1;
    }

    for (int i = sb.sti_DB; i < DISK_BLOCKS; i++)
        sb.datablock_freelist[i] = 0; // false means free

    /* marking all inodes as free */
    for (int i = 0; i < NO_OF_INODES; ++i)
        sb.inode_freelist[i] = 0;

     /* Marking all the blocks other than data blocks as used(super_block,mapping,inodes) */
    for (int i = 0; i < sb.sti_DB; ++i)
        sb.datablock_freelist[i] = 1; // true means reserved

    for (int i = 0; i < NO_OF_INODES; ++i)
    {
        inode_arr[i].first_block_num = -1;
    }

    int len;
    /* storing superblock into starting of the file */
    fseek(diskptr, 0, SEEK_SET);
    len = sizeof(struct super_block);
    char sb_buff[len];
    memset(sb_buff, 0, len);
    memcpy(sb_buff, &sb, sizeof(sb));
    fwrite(sb_buff, sizeof(char), sizeof(sb), diskptr);


    /* storing inodes after file_inode mapping */
    fseek(diskptr, (sb.sti_inode) * BLOCK_SIZE, SEEK_SET);
    len = sizeof(inode_arr);
    char inode_buff[len];
    memset(inode_buff, 0, len);
    memcpy(inode_buff, inode_arr, len);
    fwrite(inode_buff, sizeof(char), len, diskptr);

    fclose(diskptr);
    cout << string(GREEN) << "Virtual Disk Created!!!" << string(DEFAULT) << "\n";

    return 1;
}

int mount_disk(string disk_name)
{
    diskptr = fopen(disk_name.c_str(), "rb+");
    if (diskptr == NULL)
    {
        cout << string(RED) << "Disk does not exist :(" << string(DEFAULT) << "\n";
        return 0;
    }
    int len;
    /* retrieve super block from virtual disk and store into global struct super_block sb */
    char sb_buff[sizeof(sb)];
    memset(sb_buff, 0, sizeof(sb));
    fread(sb_buff, sizeof(char), sizeof(sb), diskptr);
    memcpy(&sb, sb_buff, sizeof(sb));


    /* retrieve Inode block from virtual disk and store into global struct inode 
  inode_arr[NO_OF_INODES] */
    fseek(diskptr, (sb.sti_inode) * BLOCK_SIZE, SEEK_SET);
    len = sizeof(inode_arr);
    char inode_buff[len];
    memset(inode_buff, 0, len);
    fread(inode_buff, sizeof(char), len, diskptr);
    memcpy(inode_arr, inode_buff, len);

    /* storing all filenames into map */
    for (int i = 0 ; i <NO_OF_INODES ; i++)
    {    
    	if (sb.inode_freelist[i] == 1)
        {
            file_to_inode_map[string(inode_arr[i].filename)] = i;
        }
        else free_inode_vt.push_back(i);
    }
    /* maintain free data block vector */
    for (int i = sb.sti_DB; i < DISK_BLOCKS ; i++)
    {    
        if (sb.datablock_freelist[i] == 0) free_DB_vt.push_back(i);
    }
    /* maintain free filedescriptor vector */
    for (int i = NO_OF_FILE_DESCRIPTORS - 1; i >= 0; i--)
    {
        free_fd_vt.push_back(i);
    }

    cout << string(GREEN) << "Disk is mounted!!!" << string(DEFAULT) << "\n";
    active = 1;
    return 1;
}


int create_file(string filename)
{
    if (free_inode_vt.size() == 0 || free_DB_vt.size() == 0)
    {
        cout << string(RED) << "No more Inodes available or No more DataBlock available (Error From create_file)" << string(DEFAULT) << "\n";
        return -1;
    }
    if (file_to_inode_map.find(filename) != file_to_inode_map.end())
    {
        cout << string(RED) << "File already exists (Error From create_file)" << string(DEFAULT) << "\n";
        return -1;
    }
    int next_avl_inode = free_inode_vt.back();
    inode_arr[next_avl_inode].filesize = 0;

    for(int i=0;i<filename.size();i++)
    {
        inode_arr[next_avl_inode].filename[i]=filename[i];
    }

    inode_arr[next_avl_inode].first_block_num= free_DB_vt.back();
    file_to_inode_map[filename] = next_avl_inode;
    free_inode_vt.pop_back();
    free_DB_vt.pop_back();
    
    cout << string(GREEN) << "New File Successfully Created " << string(DEFAULT) << "\n";
    return 1;
}

int open_file(string filename)
{
    

    if (free_fd_vt.size() == 0)
    {
        cout << string(RED) << "Open File Error : File descriptor not available !!!" << string(DEFAULT) << "\n";
        return -1;
    }
    if (file_to_inode_map.find(filename) == file_to_inode_map.end())
    {
        cout << string(RED) << "Open File Error : File not found !!!" << string(DEFAULT) << "\n";
        return -1;
    }
    
    
    /* asking for mode of file  */
    int file_mode = -1;
  
    cout << "0 : read mode | 1 : write mode | 2 : append mode\n";
    cin >> file_mode;
    if (file_mode < 0 || file_mode > 2)
    {
        cout << string(RED) << "Invalid File Mode choice" << string(DEFAULT) << "\n";
        return 0;
    }
    int cur_inode = file_to_inode_map[filename];


    
    for (int i = NO_OF_FILE_DESCRIPTORS-1; i >=0 ; i--)
    {
        if (fd_map.find(i) != fd_map.end() && fd_map[i].in == cur_inode && 
            (fd_map[i].mo == 2 || fd_map[i].mo == 1))
        {
            cout << string(RED) << "File is already in use with file descriptor : " << i << string(DEFAULT) << "\n";
            return -1;
        }
    }
    

    int fd = free_fd_vt.back();
    openfile_count++;

    fd_map[fd].in = cur_inode;
    fd_map[fd].mo = file_mode;

    cout << string(GREEN) << "File " << filename << " opened with file descriptor  : " << fd << string(DEFAULT) << "\n";
    free_fd_vt.pop_back();

    return fd;
}


// struct inode // total size = 32Byte
// {
//     int filesize;
//     int first_block_num;  
//     char filename[24];
// };


int write_file(int fd)
{

    if(fd_map.find(fd) == fd_map.end())
    {
        cout << string(RED) <<"Invalid File Discriptor (In write_file)"<< string(DEFAULT) << "\n";
        return -1; 
    }
    if(fd_map[fd].mo != 1)
    {
        cout << string(RED) <<"File is not opened with write mode (In write_file)"<< string(DEFAULT) << "\n";
        return -1;
    }
    int cur_inode = fd_map[fd].in;

    string content="";
    int charcount=0;
    char ch;
    ch = getchar();
    ch = getchar();

    while((int)ch!=9)
    {
        charcount++;
        content += ch;
        ch = getchar();
    }
    int num_of_blocks_req = content.size()/BLOCK_SIZE + 1;

    if(num_of_blocks_req > free_DB_vt.size())
    {
        cout << string(RED) << "File content exceeds max limit. Cannot Fit into Disk : " <<string(DEFAULT) << "\n";
        return -1;
    }


    int nxt_ptr = sb.next_disk_block[inode_arr[cur_inode].first_block_num];
    sb.next_disk_block[inode_arr[cur_inode].first_block_num] = -1;
    free_DB_vt.push_back(inode_arr[cur_inode].first_block_num);
    inode_arr[cur_inode].first_block_num=-1;
    while(nxt_ptr != -1)
    {
        int tp = nxt_ptr;
        free_DB_vt.push_back(tp);
        nxt_ptr=sb.next_disk_block[nxt_ptr];
        sb.next_disk_block[tp]=-1;
    }

    

    charcount = content.size();
    inode_arr[cur_inode].filesize=charcount;
    int cur_block_num = free_DB_vt.back();
    free_DB_vt.pop_back();
    inode_arr[cur_inode].first_block_num=cur_block_num;
    int chpt=0;
    while(charcount>=BLOCK_SIZE)
    {
        char writebuff[BLOCK_SIZE];

        for(int i=0;i<BLOCK_SIZE;i++)
        {
            writebuff[i]=content[chpt];
            chpt++;
        }



        fseek(diskptr, (cur_block_num)* BLOCK_SIZE, SEEK_SET);
        fwrite(writebuff, sizeof(char), BLOCK_SIZE, diskptr);
        charcount -= BLOCK_SIZE;

        int new_block= free_DB_vt.back();
        free_DB_vt.pop_back();
        sb.next_disk_block[cur_block_num] = new_block;
        cur_block_num = new_block;
    }   

    if(charcount<BLOCK_SIZE && charcount>0)
    {
        char writebuff[charcount];

        for(int i=0;i<charcount;i++)
        {
            writebuff[i]=content[chpt];
            chpt++;
        }

        fseek(diskptr, (cur_block_num)* BLOCK_SIZE, SEEK_SET);
        fwrite(writebuff, sizeof(char), charcount, diskptr);
        charcount =0;
    }

    return 1;
}


int append_file(int fd)
{
    if(fd_map.find(fd) == fd_map.end())
    {
        cout << string(RED) <<"Invalid File Discriptor (In append_file)"<< string(DEFAULT) << "\n";
        return -1; 
    }
    if(fd_map[fd].mo != 2)
    {
        cout << string(RED) <<"File is not opened with append mode (In write_file)"<< string(DEFAULT) << "\n";
        return -1;
    }
    int cur_inode = fd_map[fd].in;

    string content="";
    int charcount=0;
    char ch;
    ch = getchar();
    ch = getchar();

    while((int)ch!=9)
    {
        charcount++;
        content += ch;
        ch = getchar();
    }

    int num_of_blocks_req = content.size()/BLOCK_SIZE + 1;

    if(num_of_blocks_req > free_DB_vt.size())
    {
        cout << string(RED) << "File content exceeds max limit. Cannot Fit into Disk : " <<string(DEFAULT) << "\n";
        return -1;
    }

    int prev_ptr = inode_arr[cur_inode].first_block_num;
    int nxt_ptr = sb.next_disk_block[prev_ptr];
    // sb.next_disk_block[inode_arr[cur_inode].first_block_num] = -1;
    // free_DB_vt.push_back(inode_arr[cur_inode].first_block_num);
    // inode_arr[cur_inode].first_block_num=-1;
    while(nxt_ptr != -1)
    {
        prev_ptr = nxt_ptr;
        // free_DB_vt.push_back(tp);
        nxt_ptr=sb.next_disk_block[prev_ptr];
        // sb.next_disk_block[tp]=-1;
    }

    charcount = content.size();


    int sizebef = inode_arr[cur_inode].filesize;
    int lastblocksize = sizebef%BLOCK_SIZE;
    inode_arr[cur_inode].filesize += charcount;
    if(charcount <= BLOCK_SIZE - lastblocksize)
    {
        char writebuff[charcount];
        int chpt = 0;
        for(int i=0;i<charcount;i++)
        {
            writebuff[i]=content[chpt];
            chpt++;
        }



        fseek(diskptr, (prev_ptr)* BLOCK_SIZE + lastblocksize , SEEK_SET);
        fwrite(writebuff, sizeof(char), charcount, diskptr);
        charcount -= charcount;
        return 1;
    }
    
    char writebuff[BLOCK_SIZE - lastblocksize];
    int chpt1 = 0;
    for(int i=0;i<BLOCK_SIZE - lastblocksize;i++)
    {
        writebuff[i]=content[chpt1];
        chpt1++;
    }



    fseek(diskptr, (prev_ptr)* BLOCK_SIZE + lastblocksize , SEEK_SET);
    fwrite(writebuff, sizeof(char), BLOCK_SIZE - lastblocksize, diskptr);
    charcount = charcount - (BLOCK_SIZE - lastblocksize);


    int cur_block_num = free_DB_vt.back();
    sb.next_disk_block[prev_ptr] = cur_block_num;
    free_DB_vt.pop_back();
    int chpt= chpt1;
    while(charcount>=BLOCK_SIZE)
    {
        char writebuff[BLOCK_SIZE];

        for(int i=0;i<BLOCK_SIZE;i++)
        {
            writebuff[i]=content[chpt];
            chpt++;
        }



        fseek(diskptr, (cur_block_num)* BLOCK_SIZE, SEEK_SET);
        fwrite(writebuff, sizeof(char), BLOCK_SIZE, diskptr);
        charcount -= BLOCK_SIZE;

        int new_block= free_DB_vt.back();
        free_DB_vt.pop_back();
        sb.next_disk_block[cur_block_num] = new_block;
        cur_block_num = new_block;
    }   

    if(charcount<BLOCK_SIZE && charcount>0)
    {
        char writebuff[charcount];

        for(int i=0;i<charcount;i++)
        {
            writebuff[i]=content[chpt];
            chpt++;
        }

        fseek(diskptr, (cur_block_num)* BLOCK_SIZE, SEEK_SET);
        fwrite(writebuff, sizeof(char), charcount, diskptr);
        charcount =0;
    }
    return 1;

}

int read_file(int fd)
{
    if(fd_map.find(fd) == fd_map.end())
    {
        cout << string(RED) <<"Invalid File Discriptor (In Read_File)"<< string(DEFAULT) << "\n";
        return -1; 
    }
    if(fd_map[fd].mo != 0)
    {
        cout << string(RED) <<"File is not opened with read mode (In Read_File)"<< string(DEFAULT) << "\n";
        return -1;
    }
    int cur_inode = fd_map[fd].in;
    int filesize =  inode_arr[cur_inode].filesize;
    int cur_block_num = inode_arr[cur_inode].first_block_num;
    cout<<"\n\nFilename : "<<inode_arr[cur_inode].filename<<"\n------------------------------------\n";


    while(filesize>=BLOCK_SIZE)
    {
        char readbuff[BLOCK_SIZE];
        fseek(diskptr, (cur_block_num)* BLOCK_SIZE, SEEK_SET);
        memset(readbuff, 0, BLOCK_SIZE);
        int ptsize = fread(readbuff, sizeof(char), BLOCK_SIZE, diskptr);
        string pt = readbuff;
         

        cout<<pt.substr(0,BLOCK_SIZE);
        // cout<<ptsize<< "  " <<readbuff<<"\n";
        filesize -= BLOCK_SIZE;
        cur_block_num = sb.next_disk_block[cur_block_num];
    }

    if(filesize<BLOCK_SIZE && filesize>0)
    {
        char readbuff[filesize];
        fseek(diskptr, (cur_block_num)* BLOCK_SIZE, SEEK_SET);
        memset(readbuff, 0, filesize);
        int ptsize = fread(readbuff, sizeof(char), filesize, diskptr);
        string pt = readbuff;
         
        cout<<pt.substr(0,filesize);
        // cout<<ptsize<< "  " <<readbuff<<"\n";
        filesize =0;
    }
    cout<<"\n----------End of File ---------------";

    return 1;

}


int close_file(int fd)
{
    if (fd_map.find(fd) == fd_map.end())
    {
        cout << string(RED) << "close File Error : file is not opened yet !!!" << string(DEFAULT) << "\n";
        return -1;
    }

    fd_map.erase(fd);
    openfile_count--;
    free_fd_vt.push_back(fd);
    cout << string(GREEN) << "File closed successfully :) " << string(DEFAULT) << "\n";
    return 1;
}


int delete_file(string filename)
{

    //check if file exist or not
    if (file_to_inode_map.find(filename) == file_to_inode_map.end())
    {
        cout << string(RED) << "Delete File Error : File doesn't exist !!!" << string(DEFAULT) << "\n";
        return -1;
    }

    //getting inode of file
    int cur_inode = file_to_inode_map[filename];

    for (int i = NO_OF_FILE_DESCRIPTORS-1; i >=0 ; i--)
    {
        if (fd_map.find(i) != fd_map.end() && fd_map[i].in == cur_inode)
        {
            cout << string(RED) << "Delete File Error : File is opened, Can not delete an opened file !!!" << string(DEFAULT) << "\n";
            return -1;
        }
    }
    
    int nxt_ptr = sb.next_disk_block[inode_arr[cur_inode].first_block_num];
    sb.next_disk_block[inode_arr[cur_inode].first_block_num] = -1;
    free_DB_vt.push_back(inode_arr[cur_inode].first_block_num);
    while(nxt_ptr != -1)
    {
    	int tp = nxt_ptr;
        free_DB_vt.push_back(tp);
    	nxt_ptr=sb.next_disk_block[nxt_ptr];
    	sb.next_disk_block[tp]=-1;
    }

    free_inode_vt.push_back(cur_inode);

    file_to_inode_map.erase(filename);


    cout << string(GREEN) << "File Deleted successfully :) " << string(DEFAULT) << "\n";

    return 0;
}



int unmount_disk()
{
    if (!active)
    {
        cout << string(RED) << "close_disk: no open disk" << string(DEFAULT) << "\n";
        return -1;
    }

    /* Updating Super block before storing it into begining of virtual disk */
    for (int i =sb.sti_DB ; i < DISK_BLOCKS; i++)
    {
        sb.datablock_freelist[i] = 1;
    }
    /* updating free data block in super block */
    for (unsigned int i = 0; i < free_DB_vt.size(); i++)
    {
        sb.datablock_freelist[free_DB_vt[i]] = 0;
    }
    free_DB_vt.clear();
    /* Initializing inode free list to true */
    for (int i = 0; i < NO_OF_INODES; ++i)
    {
        sb.inode_freelist[i] = 1;
    }

    /* Making those inode nos which are free to false */
    for (unsigned int i = 0; i < free_inode_vt.size(); ++i)
    {
        sb.inode_freelist[free_inode_vt[i]] = 0;
    }

    int len;
    /* storing super block structure in starting of virtual disk */
    fseek(diskptr, 0, SEEK_SET);
    len = sizeof(struct super_block);
    char sb_buff[len];
    memset(sb_buff, 0, len);
    memcpy(sb_buff, &sb, sizeof(sb));
    fwrite(sb_buff, sizeof(char), sizeof(sb), diskptr);


    /* storing inodes after file_inode mapping into virtual disk */
    fseek(diskptr, (sb.sti_inode) * BLOCK_SIZE, SEEK_SET);
    len = sizeof(inode_arr);
    char inode_buff[len];
    memset(inode_buff, 0, len);
    memcpy(inode_buff, inode_arr, len);
    fwrite(inode_buff, sizeof(char), len, diskptr);

    //clear all in-memory data structures
    free_inode_vt.clear();
    free_DB_vt.clear();
    free_fd_vt.clear();
    fd_map.clear();
    file_to_inode_map.clear();

    cout << string(GREEN) << "Disk Unmounted!!!" << string(DEFAULT) << "\n";
    fclose(diskptr);

    active = 0;
    return 0;
}

void print_list_open_files()
{
    cout << string(GREEN) << "List of opened files " << string(DEFAULT) << "\n";
    for (auto i = fd_map.begin();i!=fd_map.end();i++)
    {
        int fd = i->first;
        cout << "Filename : " <<inode_arr[fd_map[fd].in].filename;
        cout << " | Fd : " << fd;
        cout<<" | Mode : ";
        if (fd_map[fd].mo == 0)
            cout << "READ mode" << "\n";
        else if (fd_map[fd].mo == 1)
            cout << "WRITE mode" << "\n";
        else if (fd_map[fd].mo == 2)
            cout << "APPEND mode" << "\n";
    }
    return;
}

void print_list_files()
{
    cout << string(GREEN) << "List of All files" << string(DEFAULT) << "\n";
    for (auto i = file_to_inode_map.begin();i!=file_to_inode_map.end();i++)
    {
        cout << "Filename : "<<i->first << " | inode : " << i->second << "\n";
    }
    return;
}


// int create_disk(string disk_name);           /* create an empty, virtual disk file          */
// int mount_disk(string disk_name);             /* open a virtual disk (file)                  */
// int create_file(string filename);            /* to create file */
// int open_file(string filename);              /* open file to get its file descriptor        */
// int write_file(int fd);
// int read_file(int fd);
// int close_file(int fd);
// int delete_file(string filename);
// int unmount_disk();
// void print_list_open_files();
// void print_list_files();


int user_handle()
{
    int choice;
    int fd = -1;
    while (1)
    {
        cout<<string(BLUE)     << "\n===============================================================" <<string(DEFAULT) << "\n";
        cout<<string(LIGHTBLUE)<< "                 Press 1  =>  create file" <<string(DEFAULT) << "\n";
        cout<<string(LIGHTBLUE)<< "                 Press 2  =>  open file" << string(DEFAULT) <<"\n";
        cout<<string(PURPLE)   << "                 Press 3  =>  read file" << string(DEFAULT) <<"\n";
        cout<<string(PURPLE)   << "                 Press 4  =>  write file" << string(DEFAULT) <<"\n";
        cout<<string(PURPLE)   << "                 Press 5  =>  append file" <<string(DEFAULT) << "\n";
        cout<<string(RED)      << "                 Press 6  =>  close file" <<string(DEFAULT) << "\n";
        cout<<string(RED)      << "                 Press 7  =>  delete file" <<string(DEFAULT) << "\n";
        cout<<string(YELLOW)   << "                 Press 8  =>  list of files" << string(DEFAULT) <<"\n";
        cout<<string(YELLOW)   << "                 Press 9  =>  list of opened files" <<string(DEFAULT) << "\n";
        cout<<string(YELLOW)   << "                 Press 10 =>  unmount" <<string(DEFAULT) << "\n";
        cout<<string(BLUE)     << "=================================================================\n" <<string(DEFAULT) << "\n";

        cin.clear();
        cout<<"Press Key : ";
        cin >> choice;
        if(choice == 1)
        {   
        	string filename;
         	cout << "Enter filename to create : ";
            cin >> filename;
            create_file(filename);
        }
        else if(choice == 2)
        {   
        	string filename;
        	cout << "Enter filename to open : ";
            cin >> filename;
           	open_file(filename);
        }
        else if(choice == 3)
        {
        	string filename;

        	cout << "Enter filedescriptor to read : ";
            cin >> fd;
            // int k;
            // cout << "Enter size to read in kb" << "\n";
            // cin >> k;
            read_file(fd);
            cin.clear();
            cout.flush();
        }
        else if(choice == 4)
        {
        	string filename;

        	cout << "Enter filedescriptor to write : ";
            cin >> fd;
            write_file(fd);
            cin.clear();
            cout.flush();
        }
        else if(choice == 5)
        {
        	string filename;

        	cout << "Enter filedescriptor to append : ";
            cin >> fd;
            append_file(fd);
            cin.clear();
            cout.flush();
        }
        else if(choice == 6)
        {
        	string filename;

        	cout << "Enter filedescriptor to close : ";
            cin >> fd;
            close_file(fd);
        }
        else if(choice == 7)
        {
        	string filename;

        	cout << "Enter filename to delete : ";
            cin >> filename;
            delete_file(filename);
        } 
        else if(choice == 8)
        {
        	string filename;

        	print_list_files();
        }
        else if(choice == 9)
        {
        	string filename;

        	print_list_open_files();
        }
        else if(choice == 10)
        {
        	string filename;

        	unmount_disk();
            break;
        	// return 0;
        }
        else
        {
        	cout << string(RED) << "Please make valid choice." << string(DEFAULT) << "\n";
            cin.clear();
        }
           
        
    }
    return 1;
}

int main()
{
    int choice;
    while (1)
    {
        cout<<string(DEFAULT)     << "\n===============================================================" <<string(DEFAULT) << "\n";
        cout<<string(BLUE) <<   "                 Press 1 =>  create disk"<< string(DEFAULT) << "\n";
        cout<<string(YELLOW) << "                 Press 2 =>  mount disk" << string(DEFAULT)<< "\n";
        cout<<string(RED) <<    "                 Press 5 =>  exit" << string(DEFAULT)<< "\n";
        cout<<string(DEFAULT)     << "\n===============================================================" <<string(DEFAULT) << "\n";
        

        cin.clear();
        cin >> choice;
        
        
        if(choice == 1)
        {	
        	string filename;
            cout << "Enter diskname : " << "\n";
            cin >> disk_name;
            create_disk(disk_name);
        }
        else if(choice == 2)
        {
        	cout << "Enter diskname : " << "\n";
            cin >> disk_name;
            if (mount_disk(disk_name))
            {
                user_handle();
            }
        }
        else if (choice == 5)
        {
            cout << string(GREEN) << "Thank You!!!" << string(DEFAULT) << "\n";
            break;
        }
        else
        {
            cout << string(RED) << "Please make valid choice." << string(DEFAULT) << "\n";
        }
        
    }
    return 0;
} 
