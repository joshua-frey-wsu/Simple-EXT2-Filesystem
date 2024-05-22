int read_file(int fd, char *buf, int nbytes)
{
    //Preparations: 
    //ASSUME: file is opened for RD or RW;  e.g. fd = 0 opened for READ
    //ask for a fd  and  nbytes to read;    e.g. read 0 10; read 0 123
    printf("Enter as: read (fd) (nbytes)\n");
		      
    //verify fd is indeed opened for RD or RW;
    if(oft[fd].mode != 0 && oft[fd].mode != 2) //R = 0, RW = 2
    {
        printf("File has not been opened for R or RW mode yet !!\n");
        return -1;
    }

    int result = my_read(fd, buf, nbytes);
    buf[nbytes - 1] = 0;
    printf("%s\n", buf);

    return result;
}

int my_read(int fd, char *buf, int nbytes)
{
    int count = 0;
    int avil = running->fd[fd]->inodeptr->INODE.i_size - running->fd[fd]->offset; // number of bytes still available in file.

    char *cq = buf;                                             // cq points at buf[ ]
    
    int blk = 0;
    int remain = 0;                                             

    while(nbytes && avil)
    {
        //Compute LOGICAL BLOCK number lbk and startByte in that block from offset;
        int lbk = running->fd[fd]->offset / BLKSIZE;
        int startByte = running->fd[fd]->offset % BLKSIZE;

        MINODE *mip = running->fd[fd]->inodeptr;

        if(lbk < 12) //direct blocks
        {
            blk = mip->INODE.i_block[lbk];
            printf("read a block\n");
        }
        else if((lbk >= 12) && (lbk < 256 + 12)) //indirect blocks
        {
            int ibuf[256];
            get_block(dev, mip->INODE.i_block[12], ibuf);
            lbk -= 12;
            blk = ibuf[lbk];
        }
        else //double indirect blocks 
        {
            int dibuf[256];
            get_block(dev, mip->INODE.i_block[13], dibuf);
            lbk = lbk - 12 - 256; 

            int ibk = lbk / 256;  // calculating the actual indirect block position
            int dbk = lbk % 256;  // calculating the actual double indirect block position
            int ibuf[256];
            get_block(dev, dibuf[ibk], ibuf);
            
            blk = ibuf[dbk];
        }

        //get the data block into readbuf[BLKSIZE]
        char readbuf[BLKSIZE];
        get_block(mip->dev, blk, readbuf);

        //copy from startByte to buf[ ], at most remain bytes in this block
        char *cp = readbuf + startByte;
        remain = BLKSIZE - startByte; // number of bytes remain in readbuf[]

        while(remain > 0)
        {
            if(remain < avil) //check if its not a full block of data
            {
                strncpy(cq, cp, remain);     // grabing whats remaining inside readbuf[]
                oft[fd].offset += remain;    // advance offset 
                count += remain;             // inc count as number of bytes read
                avil -= remain;              // adjust avil by remaining bytes to read in readbuf
                nbytes -= remain; 
                remain -= remain;
            }
            else
            {
                strncpy(cq, cp, avil);       //copy bytes from readbuf[] into buf[] by size of remain
                oft[fd].offset += avil;      // advance offset 
                count += avil;               // inc count as number of bytes read
                remain -= avil;              // adjust remain by avalible bytes to read from the data block
                nbytes = 0;                  // read the entire remaining bytes at once
                avil -= avil;                
            }

            if (nbytes <= 0 || avil <= 0)    // nothing left to read
            {
                break;
            }    
        }

        // if one data block is not enough, loop back to OUTER while for more ...
    }

    //printf("myread: read %d char from file descriptor %d\n", count, fd);  
    return count;   // count is the actual number of bytes read
}

void cat(char *name)
{
    char mybuf[BLKSIZE], dummy = 0;  // a null char at end of mybuf[ ]
    int n;
    
    //int fd = open filename for READ;
    int fd = open_file(name, 0);

    while(n = my_read(fd, mybuf, 1024))
    {
        mybuf[n] = dummy;          // as a null terminated string
        printf("%s", mybuf);       // <=== THIS works but not good
        //spit out chars from mybuf[ ] but handle \n properly;
    }

    close_file(fd);
}