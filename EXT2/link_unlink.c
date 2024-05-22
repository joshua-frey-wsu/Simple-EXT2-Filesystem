void link() //mkdir
{
    printf("(entered link() function !!!)\n"); // <========== testing
    MINODE *mip = path2inode(pathname); //get the INODE of (/a/b/c) into memory (oldFile)

    if(S_ISDIR(mip->INODE.i_mode))      //check if /a/b/c is NOT a directory type
    {
        printf("The given pathname is a directory type, Link to DIR is NOT allowed!!!\n");
        return;
    }

    //break the parameter into parent (/x/y) and child (/z)
    puts(parameter);
    char *newChild = basename(parameter);
    char *newParent = dirname(parameter);

    MINODE *pip = path2inode(newParent);  //Get minode of the newFile

    //check if parent (/x/y) IS a directory type
    if(pip == 0)
    {
        printf("The given pathname does NOT exist!!!\n");
        return;
    }

    if(!S_ISDIR(pip->INODE.i_mode))    
    {
        printf("The given pathname is NOT directory type!!!\n");
        return;
    }

    // check if child (/z) already exist 
    if(search(pip, newChild)) 
    {
        printf("File name already exist in this directory!!!\n");
        return;
    }

    //add an entry [ino, rec_len, name_len, z] to the data block of /a/b/
    puts(newChild);
    enter_child(pip, mip->ino, newChild);

    mip->INODE.i_links_count++; // increment link_count of INODE by 1
    mip->modified = 1;          // mark as modified
    iput(mip);                  // write INODE back to disk (oldFile)
    iput(pip);                  // release the newFile inode
}

void unlink() //rmdir
{
    printf("(entered unlink() function !!!)\n"); // <========== testing
    MINODE *mip = path2inode(pathname); // get pathname's INODE into memory

    // verify it's a FILE (REG or LNK), can not be a DIR;
    if(S_ISDIR(mip->INODE.i_mode))  
    {
        printf("THe given pathname cannot be unlink due to it is a directory type!!!\n");
        return;
    }

    mip->INODE.i_links_count--;         // decrement INODE's i_links_count by 1;

    // if i_links_count == 0 ==> rm pathname by deallocating its data blocks
    if(mip->INODE.i_links_count == 0)
    {
        truncate(mip);
    }

    // break the parameter into parent and child
    char *parent = dirname(pathname); 
    char *child = basename(pathname);

    MINODE *pip = path2inode(parent);  //Get minode of the parent

    rm_child(pip, child); // remove the child from the parent

    mip->modified = 1;          // mark as modified
    iput(mip);                  // write INODE back to disk (oldFile)
    iput(pip);                  // release the parent inode
}

void symlink()
{
    printf("(entered symlink() function !!!)\n"); // <========== testing
    MINODE *mip = path2inode(pathname); // get oldNAME INODE into memory
    
    // verify oldNAME exists (either a DIR or a REG file)
    if(mip == 0)  
    {
        printf("The given pathname DOES NOT exist!!!\n");
        return;
    }

    // verify if oldNAME type is either a DIR or a REG file
    if(!S_ISDIR(mip->INODE.i_mode) && !S_ISREG(mip->INODE.i_mode))  
    {
        printf("The given pathname is NOT a DIR type or a REG type!!!\n");
        return;
    }

    //break the parameter into parent (/x/y) and child (/z)
    char *parent = dirname(parameter); 
    char *child = basename(parameter);

    MINODE *new_pip = path2inode(parent); // get newNAME INODE's parent into memory

    my_creat(new_pip, child);   // creat a FILE /x/y/z

    MINODE *new_mip = path2inode(child); // get newNAME INODE's child into memory

    // change /x/y/z's type to LNK
    new_mip->INODE.i_mode |= 0xA000;
    
    puts(parameter);
    strcpy(new_mip->INODE.i_block, pathname); // write the string oldNAME into the i_block[ ], which has room for 60 chars

    new_mip->INODE.i_size = strlen(pathname); // set /x/y/z file size = number of chars in oldName

    //strncpy(child, oldChild, strlen(oldChild));

    // write the INODE of /x/y/z back to disk.
    new_mip->modified = 1;
    iput(new_mip);
}

char *my_readlink(MINODE *mip)
{   
    if(!S_ISLNK(mip->INODE.i_mode))
    {
        printf("The given pathname is NOT a Symbolic link type!!!\n");
        return "DNE";
    }

    return mip->INODE.i_block; // return its string contents in INODE.i_block[ ].
}

/******************** truncate funcation ********************/
void truncate(MINODE *mip)
{
    int i;

    // Direct Blocks
    for(i = 0; i < 12; i++) // loop through all 12 Direct Blocks
    {
        if(mip->INODE.i_block[i] == 0) //exit loop if all data blocks have been looped through
        {
            break;
        }
            
        bdalloc(mip->dev, mip->INODE.i_block[i]); // deallocate the data block
    }

    // Indirect Blocks
   if(mip->INODE.i_block[12]) //if data block[12] exists
   {
        //printf("-------------------- INDIRECT BLOCKS --------------------\n");

        int indirectBuf[BLKSIZE / 4];
        get_block(mip->dev, mip->INODE.i_block[12], (char *)indirectBuf); //call get_block on currentRoot->i_block[i] and store it in a buffer

        int *ip = indirectBuf;
        while(*ip && ip < indirectBuf + BLKSIZE)
        {
            bdalloc(mip->dev, *ip);
            ip++;
        }
        //printf("\n\n");
   }

   // Double Indirect Blocks
   if(mip->INODE.i_block[13]) //if data block[13] exists
   {
        //printf("-------------------- DOUBLE INDIRECT BLOCKS --------------------\n");

        int doubleIndirectBuf[BLKSIZE / 4];
        get_block(mip->dev, mip->INODE.i_block[13], (char *)doubleIndirectBuf); //call get_block on currentRoot->i_block[i] and store it in a buffer

        int *ip = doubleIndirectBuf;
        while(*ip && ip < doubleIndirectBuf + BLKSIZE)
        {
            int i = 0;
            while (doubleIndirectBuf[i]) //if buffer exists
            {
                int looping[BLKSIZE / 4];
                get_block(fd, doubleIndirectBuf[i], (char *)looping); //call get_block on buffer and store it in a looping buffer

                int *lp = looping;
                while(*lp && lp < looping + BLKSIZE)
                {
                    bdalloc(mip->dev, *lp);
                    lp++;
                }
                i++;
            }
            ip++;
        }
    }

    mip->INODE.i_atime = time(0L);  // update INODE's time fields, set to current time
    mip->INODE.i_size = 0;          // set INODE's size to 0 and mark minode MODIFIED
}