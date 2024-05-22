void myopen()
{
    return open_file(pathname, atoi(parameter));
}   

int open_file(char *pathname, int mode)
{
    //Assume open 
    //get pathname's minode pointer
    printf("pathname = %s\n", pathname);
    MINODE *mip = path2inode(pathname);

    int i;
    //int mode = atoi(parameter); //convert userinput mode from string to integer

    if(!mip) //pathname does not exist
    {
        if(mode == 0) //READ: file must exist to be read
        {
            printf("File must exist to be READ !!");
            return -1;
        }
        
        creat_file(pathname); //mode = R|RW|APPEND: create file first and make sure create_file uses pathname ************************************************************
        mip = path2inode(pathname);
        printf("[dev:%d, ino:%d]\n", mip->dev, mip->ino); //print mip's [dev, ino]
    }

    if(!S_ISREG(mip->INODE.i_mode)) //check mip->INODE.i_mode is a regular file
    {
        printf("File type is NOT REG !!\n");
        return -1;
    }

    //loop through the fd list and check the file is ALREADY opened with INCOMPATIBLE mode: W, RW, APPEND 
    for(int j = 0; (j < NFD) && (running->fd[j] != NULL); j++) 
    {
        if(running->fd[j]->inodeptr->ino == mip->ino)
        {
            if((running->fd[j]->mode == 1) || (running->fd[j]->mode == 2) || (running->fd[j]->mode == 3))
            {
                printf("File cannot be open as it is being modified currently !!\n");
                return -1;
            }
        }
    }

    //iterate through the array of oft[] to find a free oft
    int index = 0;
    for(index = 0; index < NOFT; index++)
    {
        if(oft[index].shareCount == 0)
        {
            printf("Found a free oft[%d]\n", index);
            break;
        }
    }

    oft[index].mode = mode;    // mode = 0|1|2|3 for R|W|RW|APPEND 
    oft[index].shareCount = 1;
    oft[index].inodeptr = mip; // point at the file's minode[]

    switch(mode)
    {
        case 0 : oft[index].offset = 0;     // R: offset = 0
                break;

        case 1 : truncate(mip);        // W: truncate file to 0 size
                oft[index].offset = 0;
                break;

        case 2 : oft[index].offset = 0;     // RW: do NOT truncate file
                break;

        case 3 : oft[index].offset = mip->INODE.i_size;  // APPEND mode
                break;

        default: printf("invalid mode\n");
                return(-1);
    }

    //find the SMALLEST index i in running PROC's fd[ ] such that fd[i] is NULL
    //Let running->fd[i] point at the OFT entry
    for(i = 0; i < NFD; i++) 
    {
        if(running->fd[i] == 0)
        {
            running->fd[i] = &oft[index];
            break;
        }
    }

    if(mode == 0) //for R: touch atime. 
    {
        mip->INODE.i_atime = time(0l);
    }
    else if(mode == 1 || mode == 2 || mode == 3) // for W|RW|APPEND mode : touch atime and mtime
    {
        mip->INODE.i_atime = mip->INODE.i_mtime = time(0l);
    }

    //return i as the file descriptor
    return i;
}

int close_file(int fd)
{
    printf("fd = %d\n", fd);
    //verify fd is within range.
    if(fd > NFD || fd < 0) 
    {
        printf("The given file descriptor is out of range !!\n");
        return -1;
    }

    //verify running->fd[fd] is pointing at a OFT entry
    if(running->fd[fd] == 0) 
    {
        printf("File descriptor to be closed DNE exist !!\n");
        return -1;
    }

    OFT *oftp = running->fd[fd];
    running->fd[fd] = 0;

    oftp->shareCount--;
    if (oftp->shareCount > 0)
    {
        return 0;
    }
        
    // last user of this OFT entry ==> dispose of its minode
    MINODE *mip = oftp->inodeptr;
    iput(mip);

    return 0;
}

int my_lseek(int fd, int position)
{
    OFT *oftp = &oft[fd];         // From fd, find the OFT entry

    if(!oftp)
    {
        printf("File does not exist !!\n");
        return -1;
    }

    // make sure NOT to over run either end of the file.
    if((position < 0) || (position > oftp->inodeptr->INODE.i_size))
    {
        printf("Cannot seek before the begining or after the end of the file !!\n");
        return -1;
    }

    long int originalPosition = oftp->offset; // save the original oftp's offset

    oftp->offset = position; // change OFT entry's offset to position
    printf("offset = %d\n", (int)oftp->offset);

    return originalPosition; // return originalPosition
}

int pfd()
{
    //displays the currently opened files
    static char *mode[4] = {"READ","WRITE","RW","APPEND"}; //allocate in the memory once

    printf(" fd    mode    offset    INODE\n");
    printf("----  ------  --------  -------\n");

    for(int i = 0; i < NOFT; i++)
    {
        if(oft[i].shareCount > 0) //file is open
        {
            printf("%d  %s  %d  [%d, %d]\n", i, mode[oft[i].mode], oft[i].offset, oft[i].inodeptr->dev, oft[i].inodeptr->ino);
        }
    }
}

/******************** file descriptor operations ********************/
int my_dup(int fd)
{
    // verify fd is an opened descriptor
    if(oft[fd].shareCount == 0)
    {
        printf("File is not open yet !!\n");
        return -1;
    }

    //duplicates (copy) fd[fd] into FIRST empty fd[ ] slot
    for(int i = 0; i < NFD; i++)
    {
        if(running->fd[i] == 0)
        {
            printf("Found an empty fd[] slot\n");
            running->fd[i] = running->fd[fd];
            break;
        }
    }

    //increment OFT's shareCount by 1
    running->fd[fd]->shareCount++;
}

int my_dup2(int fd, int gd)
{
    // verify fd is an opened descriptor
    if(oft[fd].shareCount == 0)
    {
        printf("File is not open yet !!\n");
        return -1;
    }

    //CLOSE gd first if it's already opened;
    if(oft[gd].shareCount > 0)
    {
        close_file(gd);
    }

    //duplicates fd[fd] into fd[gd];
    running->fd[gd] = running->fd[fd];

    //increment OFT's shareCount by 1;
    running->fd[fd]->shareCount++;
}