int write_file(int fd, char *buf)
{
    // Preprations:
    //ask for a fd   and   a text string to write;
    //e.g.     write 1 1234567890;    write 1 abcdefghij
    printf("Enter as: write (fd) (string to write)\n");
		     
    //verify fd is indeed opened for WR or RW or APPEND mode
    if(oft[fd].mode == 0)
    {
        printf("File has not been opened for R, RW or APPEND mode yet !!");
        return -1;
    }

    //copy the text string into a buf[] and get its length as nbytes.
    strcpy(buf, parameter);
    int nbytes = strlen(parameter);

    //return mywrite(fd, buf, nbytes);
    return mywrite(fd, buf, nbytes);
}

int mywrite(int fd, char buf[], int nbytes)
{
    int lbk = 0;
    int startByte = 0;
    int blk = 0;
    MINODE *mip = oft[fd].inodeptr;
    int remain = 0;
    char *cq = buf;  // cq points at buf[ ]
    int bytes_wrote = nbytes;

    while(nbytes > 0)
    {
        //Compute LOGICAL BLOCK number (lbk) and startByte in that block from offset;
        lbk = oft[fd].offset / BLKSIZE;
        startByte = oft[fd].offset % BLKSIZE; 

        if(lbk < 12) // direct block
        {
            if(mip->INODE.i_block[lbk] == 0) //if no data block yet
            {
                mip->INODE.i_block[lbk] = balloc(mip->dev); //allocate data block
            }

            blk = mip->INODE.i_block[lbk]; //blk should be a disk block now
        }
        else if((lbk >= 12) && (lbk <= 256 + 12)) // indirect block
        {
            if(mip->INODE.i_block[12] == 0)
            {
                mip->INODE.i_block[12] = balloc(mip->dev); //allocate a block for it
                
                //zero out the block on disk
                char tbuf[BLKSIZE];
                get_block(mip->dev, mip->INODE.i_block[12], tbuf);
                bzero(tbuf, BLKSIZE);
                put_block(mip->dev, mip->INODE.i_block[12], tbuf); //write ibuf[ ] back to disk block i_block[12];                          
            }

            int ibuf[256];
            get_block(mip->dev, mip->INODE.i_block[12], ibuf); //get i_block[12] into an int ibuf[256]
            blk = ibuf[lbk - 12];

            if(blk == 0)
            {
                blk = balloc(mip->dev); //allocate a disk block;
                ibuf[lbk - 12] = blk;   //record it in ibuf[lbk - 12];
            }

            put_block(mip->dev, mip->INODE.i_block[12], ibuf); //write ibuf[ ] back to disk block i_block[12];
        }
        else // double indirect block
        {
            if(mip->INODE.i_block[13] == 0)
            {
                mip->INODE.i_block[13] = balloc(mip->dev); //allocate a block for it
                
                //zero out the block on disk
                char tbuf[BLKSIZE];
                get_block(mip->dev, mip->INODE.i_block[13], tbuf);
                bzero(tbuf, BLKSIZE);
                put_block(mip->dev, mip->INODE.i_block[13], tbuf); //write ibuf[ ] back to disk block i_block[13];
            }

            int dibuf[256];
            get_block(dev, mip->INODE.i_block[13], dibuf);
            lbk = lbk - 12 - 256;

            if(dibuf[lbk / 256] == 0)
            {
                dibuf[lbk / 256] = balloc(mip->dev); //allocate a disk block;

                char tbuf[BLKSIZE];
                get_block(mip->dev, dibuf[lbk/256], tbuf);
                bzero(tbuf, BLKSIZE);
                put_block(mip->dev, dibuf[lbk/256], tbuf); //write ibuf[ ] back to disk block i_block[lbk];
            }

            put_block(mip->dev, mip->INODE.i_block[13], dibuf); //write ibuf[ ] back to disk block i_block[13];

            int ibk = lbk / 256;  // calculating the actual indirect block position
            int dbk = lbk % 256;  // calculating the actual double indirect block position
            int ibuf[256];
            get_block(mip->dev, dibuf[ibk], ibuf);
            blk = ibuf[dbk];

            if(blk == 0)
            {
                blk = balloc(mip->dev); //allocate a disk block;
                ibuf[dbk] = blk;        //record it in ibuf[lbk - 12];
            }

            put_block(mip->dev, dibuf[ibk], ibuf);
        }

        /* all cases come to here : write to the data block */
        char wbuf[BLKSIZE];
        get_block(mip->dev, blk, wbuf);   // read disk block into wbuf[ ]  
        char *cp = wbuf + startByte;      // cp points at startByte in wbuf[]
        remain = BLKSIZE - startByte;     // number of BYTEs remain in this block

        //resize remain if nbytes to write is smaller than remain
        if(remain > nbytes)
        {
            remain = nbytes;
        }

        while (remain > 0)              // write as much as remain allows  
        {             
            strncpy(cp, cq, remain);    //*cp++ = *cq++; // cq points at buf[ ]
            oft[fd].offset += remain;   // advance offset
            nbytes -= remain; 
            remain -= remain;           // dec counts
            
            if (oft[fd].offset > mip->INODE.i_size) // especially for RW|APPEND mode
            {
                mip->INODE.i_size += remain;   // inc file size (if offset > fileSize)
            }  
                
            if (nbytes <= 0) // if already nbytes, break
            {
                break;                 
            }
        }

        put_block(mip->dev, blk, wbuf);   // write wbuf[ ] to disk
        // loop back to outer while to write more .... until nbytes are written
    }

    mip->modified = 1;       // mark mip modified for iput()

    // update the file size if offset over takes the file size
    if(oft[fd].offset > mip->INODE.i_size)
    {
        mip->INODE.i_size = oft[fd].offset;
    }

    printf("wrote %d char into file descriptor fd=%d\n", bytes_wrote, fd);           
    return nbytes;
}

void cp()
{
    if(strcmp(pathname, parameter) == 0)
    {
        printf("Cannot copy the same file to itself !!\n");
        return -1;
    }

    int fd = open_file(pathname, 0);  //open src for read
    int gd = open_file(parameter, 1); //open source for write, if file dne, create it first

    int n = 0;
    char buf[BLKSIZE];
    while (n = my_read(fd, buf, BLKSIZE))
    {
        mywrite(gd, buf, n);
    }

    close_file(fd);
    close_file(gd);
}
