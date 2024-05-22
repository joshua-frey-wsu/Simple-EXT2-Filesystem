/******************** Set ,Clear and Remove Bits ********************/
int tst_bit(char *buf, int bit)
{
  return buf[bit / 8] & (1 << (bit % 8)); //bitwise operation (AND)
}

int set_bit(char *buf, int bit)
{
  buf[bit / 8] |= (1 << (bit % 8)); //bitwise operation (OR)
}

int clr_bit(char *buf, int bit) //clears bit in char buf[BLKSIZE]
{
  buf[bit / 8] &= ~(1 << (bit % 8)); //bitwise operation (NAND)
}

/******************** imap modification ********************/
int incFreeInodes(int dev)
{
  char buf[BLKSIZE];

  // inc free inodes count in SUPER and GD
  get_block(dev, 1, buf);
  SUPER *sp = (SUPER *)buf;
  sp->s_free_inodes_count++;
  put_block(dev, 1, buf);

  get_block(dev, 2, buf);
  GD *gp = (GD *)buf;
  gp->bg_free_inodes_count++;
  put_block(dev, 2, buf);
}

int decFreeInodes(int dev)
{
  char buf[BLKSIZE];

  // dec free inodes count by 1 in SUPER and GD
  get_block(dev, 1, buf);
  SUPER *sp = (SUPER *)buf;
  sp->s_free_inodes_count--;
  put_block(dev, 1, buf);

  get_block(dev, 2, buf);
  GD *gp = (GD *)buf;
  gp->bg_free_inodes_count--;
  put_block(dev, 2, buf);
}

int ialloc(int dev)  // allocate an ino number from imap
{
  int  i;
  char buf[BLKSIZE];

  // read inode_bitmap block
  get_block(dev, imap, buf);

  for (i=0; i < ninodes; i++)
  {
    if (tst_bit(buf, i) == 0)
    {
      set_bit(buf,i);
      put_block(dev, imap, buf);

      decFreeInodes(dev);

      printf("Allocated an inode from imap : ino=%d\n", i+1);		
      return i+1;
    }
  }
  return 0;
}

int idalloc(int dev, int ino)  // deallocate an ino number from imap 
{
  int i;  
  char buf[BLKSIZE];

  // return 0 if ino < 0 OR > ninodes (inode number is out of range)
  if(ino < 0 || ino > ninodes)
  {
    return 0;
  }

  // get inode_bitmap block
  get_block(dev, imap, buf);
  clr_bit(buf, ino-1);

  // write buf back
  put_block(dev, imap, buf);

  // update free inode count in SUPER and GD
  incFreeInodes(dev);
}

/******************** bmap modification ********************/
int incFreeBlocks(int dev)
{
  char buf[BLKSIZE];

  // dec free inodes count by 1 in SUPER and GD
  get_block(dev, 1, buf);
  SUPER *sp = (SUPER *)buf;
  sp->s_free_blocks_count++;
  put_block(dev, 1, buf);

  get_block(dev, 2, buf);
  GD *gp = (GD *)buf;
  gp->bg_free_blocks_count++;
  put_block(dev, 2, buf);
}

int decFreeBlocks(int dev)
{
  char buf[BLKSIZE];

  // dec free inodes count by 1 in SUPER and GD
  get_block(dev, 1, buf);
  SUPER *sp = (SUPER *)buf;
  sp->s_free_blocks_count--;
  put_block(dev, 1, buf);

  get_block(dev, 2, buf);
  GD *gp = (GD *)buf;
  gp->bg_free_blocks_count--;
  put_block(dev, 2, buf);
}

int balloc(int dev) // allocates a block number
{
  int  i;
  char buf[BLKSIZE];

  // read inode_bitmap block
  get_block(dev, bmap, buf);

  for (i=0; i < nblocks; i++)
  {
    if (tst_bit(buf, i) == 0)
    {
      set_bit(buf,i);
      put_block(dev, bmap, buf);

      decFreeBlocks(dev);

      printf("Allocated a data block from bmap : bno=%d\n", i+1);		
      return i + 1;
    }
  }
  return 0;
}

int bdalloc(int dev, int blk) // deallocate a block number
{
  int i;  
  char buf[BLKSIZE];

  // return 0 if blk < 0 OR > nblocks (block number is out of range)
  if(blk < 0 || blk > nblocks)
  {
    return 0;
  }

  // get block_bitmap block
  get_block(dev, bmap, buf);
  clr_bit(buf, blk-1);

  // write buf back
  put_block(dev, bmap, buf);

  // update free block count in SUPER and GD
  incFreeBlocks(dev);

}

/******************** make_dir, mymkdir, enter_child ********************/
int make_dir()
{
  /*
    1. Let  
        parent = dirname(pathname);   parent= "/a/b" OR "a/b"
        child  = basename(pathname);  child = "c"

      WARNING: strtok(), dirname(), basename() destroy pathname

    2. Get minode of parent:

      MINODE *pip = path2inode(parent);
      (print message if pip NULL; return -1 for error)

      Verify : (1). parent INODE is a DIR (HOW?)   AND
                (2). child does NOT exists in the parent directory (HOW?);
                  
    4. call mymkdir(pip, child);

    5. inc parent inodes's links count by 1; 
      touch its atime, i.e. atime = time(0L), mark it modified

    6. iput(pip);
  */
  
  char *parent = dirname(pathname); 
  char *child = basename(pathname);

  puts(parent);//<========== testing
  puts(child); //<========== testing

  //Get minode of parent
  MINODE *pip = path2inode(parent);
  if(pip == 0)
  {
    printf("The given pathname does not exist!!!\n");
    return -1;
  }

  //check if parent INODE is a DIR and child DNE in parent directory
  if(!S_ISDIR(pip->INODE.i_mode))
  {
    printf("Given pathname is not a DIRECTORY type!!!\n");
    return -1;
  }

  if(search(pip, child))
  {
    printf("Directory name already exist in this directory!!!\n");
    return -1;
  }

  //call mymkdir
  mymkdir(pip, child);

  //inc parent inodes's links count by 1; touch its atime, i.e. atime = time(0L), mark it modified
  pip->INODE.i_links_count++;
  pip->INODE.i_atime = time(0l);
  pip->modified = 1;

  //release parent inode
  iput(pip);
}

int mymkdir(MINODE *pip, char *name)
{
  int ino = ialloc(dev);
  int bno = balloc(dev);

  printf("ino=%d, bno=%d\n", ino, bno); //<========== testing

  MINODE *mip = iget(dev, ino); //load into a minode[] in order to write contents to the INODE in memory
  INODE *ip = &mip->INODE;

  ip->i_mode = 0x41ED;      // make it a file type
  ip->i_uid = running->uid; // load in current running's Owner id
  ip->i_gid = running->gid; // load in current running's Group id
  ip->i_size = BLKSIZE;     // Size in bytes
  ip->i_links_count = 2;    // Links count = 2 due to "." and ".."
  ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L); //set to current time
  ip->i_blocks = 2;         // LINUX: Blocks count in 512-byte chunks
  ip->i_block[0] = bno;
  
  //set ip->i_block[0] ~ ip->i_block[14] equal to 0;
  for(int i = 1; i < 15; i++)
  {
    ip->i_block[i] = 0;
  }

  mip->modified = 1;        //make minode MODIFIED
  iput(mip);                //write INODE to disk

  char buf[BLKSIZE] = {0};
  //bzero(buf, BLKSIZE); //optional: clear buf[] to 0
  DIR *dp = (DIR *)buf;

  //make "." entry
  dp->inode = ino;
  dp->rec_len = 12;
  dp->name_len = 1;
  dp->name[0] = '.';

  // make .. entry: pino = parent DIR ino, blk = allocated block
  int pino = pip->ino;

  dp = (char *)dp + 12;
  dp->inode = pino;
  dp->rec_len = BLKSIZE - 12; // rec_len spans block
  dp->name_len = 2;
  dp->name[0] = dp->name[1] = '.';
  put_block(dev, bno, buf);   // write to bno on diks

  enter_child(pip, ino, name); //enter name ENTRY into parent's directory by calling enter_name() function
}

int enter_child(MINODE *pip, int myino, char *myname)
{
  int i;
  char buf[BLKSIZE];

  int NEED_LEN = 4 * ((8 + strlen(myname) + 3) / 4); //multiple of 4
  char *cp = buf;
  DIR *dp;
  
  //for each data block of parent DIR
  for(i = 0; i < 12; i++)
  {
    if(pip->INODE.i_block[i] == 0)
    {
      printf("No disk size is avalible!!! Need to allocate new space :(\n");
      break;
    }
    
    get_block(pip->dev, pip->INODE.i_block[i], buf); //get parent's data block into a buf[]

    dp = (DIR *)buf;
    //int IDEAL_LEN = 4 * ((8 + dp->name_len + 3) / 4);

    //step through each DIR entry dp in parent data block
    while(cp + dp->rec_len < buf + BLKSIZE)
    {
      printf("(entered while loop!!!)\n"); //<========== testing
      cp += dp->rec_len;
      dp = (DIR *)cp;
    }
    //dp now points at the last entry of the data block

    int IDEAL_LEN = 4 * ((8 + dp->name_len + 3) / 4); //minimum space rounded to the nearest multiple of 4

   
    int REMAIN = dp->rec_len - IDEAL_LEN;   //compute remaining space
    if(REMAIN >= NEED_LEN)                  //found enough space for new entry
    {
      dp->rec_len = IDEAL_LEN;              // trim dp's rec_len to its IDEAL_LEN
      cp += dp->rec_len;                    // go to the next avalible space
      dp = (DIR *)cp;                       // points the directory pointer (dp) to the new avalible space

      //enter new entry as [mino, REMIAN, strlen(myname), myname]
      dp->inode = myino;
      dp->rec_len = REMAIN;
      dp->name_len = strlen(myname);
      strcpy(dp->name, myname);             //dp->name[dp->name_len] = 0; killing the final character (if needed)

      //write parent data back to disk
      put_block(pip->dev, pip->INODE.i_block[i], buf);
      printf("Succesfully entered new directory info :)\n");
      return;
    }
  }
  
  //no avalible space found by this step, hence needed to create new block
  int blockNum = balloc(pip->dev);    //allocating new data block
  pip->INODE.i_block[i] = blockNum;   //update the parents inode block to the newly allocated block
  get_block(pip->dev, blockNum, buf); //get the newly allocated block info
  dp = (DIR *)buf;                    //pointing the directory pointer to the new data block
  pip->INODE.i_size += BLKSIZE;       //increment parent's size by BLKSIZE

  //enter new entry as [mino, REMIAN, strlen(myname), myname]
  dp->inode = myino;
  dp->rec_len = BLKSIZE;
  dp->name_len = strlen(myname);
  strcpy(dp->name, myname);

  //write parent data back to disk
  put_block(pip->dev, blockNum, buf);
  printf("Succesfully allocated new space an entered new directory info :3\n");
}

/******************** creat_file, my_creat ********************/
void creat_file(char *pathname) // ************************************************
{
  char *parent = dirname(pathname); 
  char *child = basename(pathname);

  puts(parent);//<========== testing
  puts(child); //<========== testing

  //Get minode of parent
  MINODE *pip = path2inode(parent);
  if(pip == 0)
  {
    printf("The given pathname does not exist!!!\n");
    return -1;
  }

  //check if parent INODE is a DIRECTORY type and child DNE in parent directory
  if(!S_ISDIR(pip->INODE.i_mode))
  {
    printf("Given pathname is not a Directory type!!!\n");
    return -1;
  }

  if(search(pip, child))
  {
    printf("Directory name already exist in this directory!!!\n");
    return -1;
  }

  //call my_creat()
  my_creat(pip, child);

  //DO NOT inc parent inodes's links count by 1; touch its atime, i.e. atime = time(0L), mark it modified
  //pip->shareCount++;
  pip->INODE.i_atime = time(0l);
  pip->modified = 1;

  //release parent inode
  iput(pip);
}

void my_creat(MINODE *pip, char *name)
{
  int ino = ialloc(dev);
  //int bno = balloc(dev);

  printf("ino=%d\n", ino); //<========== testing

  MINODE *mip = iget(dev, ino); //load into a minode[] in order to write contents to the INODE in memory
  INODE *ip = &mip->INODE;

  ip->i_mode = 0x81A4;      // make it a file type
  ip->i_uid = running->uid; // load in current running's Owner id
  ip->i_gid = running->gid; // load in current running's Group id
  ip->i_size = 0;           // Size in bytes
  ip->i_links_count = 1;    // Links count = 1 only
  ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L); //set to current time
  ip->i_blocks = 1;         // LINUX: Blocks count in 512-byte chunks
  ip->i_block[0] = 0;

  //set ip->i_block[0] ~ ip->i_block[14] equal to 0;
  for(int i = 1; i < 15; i++)
  {
    ip->i_block[i] = 0;
  }

  mip->modified = 1;        //make minode MODIFIED
  iput(mip);                //write INODE to disk

  enter_child(pip, ino, name); //enter name ENTRY into parent's directory by calling enter_name() function
}

/*************************** rmdir ***************************/
int rmdir()
{
  printf("(entered rmdir!!!)\n"); //<========== testing

  char sbuf[BLKSIZE], temp[256];
  DIR *dp;
  char *cp;
  int loop_count = 0;

  char temp1[128];
  strcpy(temp1, pathname);
  char *parent = dirname(temp1); 

  // get minode of pathname
  MINODE *mip = path2inode(pathname);

  if(mip == 0) //check if pathname exist
  {
    printf("(entered first if statement!!!)\n"); //<========== testing
    printf("The given pathname does not exist!!!\n");
    return -1;
  }

  //check if parent INODE is a DIR, busy or not and is empty
  if(!S_ISDIR(mip->INODE.i_mode))
  {
    printf("(entered second if statement!!!)\n"); //<========== testing
    printf("Given pathname is not a DIRECTORY type!!!\n");
    return -1;
  }

  if(mip->shareCount > 1) //check for busy or not
  {
    printf("(entered third if statement!!!)\n"); //<========== testing
    printf("Given pathname is being used at the moment!!!\n");
    return -1;
  }
  
  // ASSUME only one data block i_block[0]
  // traverse through the data block to make sure there are no files other than "." and ".."
  get_block(mip->dev, mip->INODE.i_block[0], sbuf);

  dp = (DIR *)sbuf;
  cp = sbuf;
  
  while(cp < sbuf + BLKSIZE)
  {
    printf("(entered while loop!!!)\n"); //<========== testing
    printf("dp->rec_len = %d\n", dp->rec_len);
    cp += dp->rec_len;
    dp = (DIR *)cp;
    loop_count++;
  }

  if(mip->INODE.i_links_count > 2 || loop_count > 2) // check if directory is empty
  {
    printf("(entered fourth if statement!!!)\n"); //<========== testing
    printf("Given pathname is not empty inside!!!\n");
    return -1;
  }

  // above checks passed, perform the deletion of directory
  int temp_ino = 0;
  int pino = findino(mip, &temp_ino); // get parent DIR ino (in mip->INODE.i_block[0])
  
  // deallocate its block and inode
  for(int i = 0; i < 12; i++)
  {
    printf("(entered for loop!!!)\n"); //<========== testing
    if(mip->INODE.i_block[i] == 0) // move to the next data block if current one DNE
    {
      continue;
    }
    bdalloc(mip->dev, mip->INODE.i_block[i]); // deallocate the data block
  }
  idalloc(mip->dev, mip->ino); // deallocate the mip ino number
  iput(mip);                   // clears mip->shareCount to 0, still in cache

  MINODE *pip = iget(mip->dev, pino); // get parent minode
  rm_child(pip, pathname);                    // remove child

  pip->INODE.i_links_count--;     // decrement pip's link_count by 1
  pip->INODE.i_atime = time(0l);  // touch pip's atime, mtime fields
  //pip->modified = 1;
  iput(pip);                      // this will write parent INODE out eventually

  return;
}

int rm_child(MINODE *parent, char *name)
{
  printf("(entered rm_child!!!)\n"); //<========== testing
  int i;
  char buf[BLKSIZE], temp[256];
  char *cp = buf;
  DIR *dp, *prev_dp;

  //search parent INODE's data block(s) for the entry name
  //search(parent, name);

  for(i = 0; i < 12; i++)
  {
    printf("(entered for loop!)\n"); //<========== testing
    if(parent->INODE.i_block[i] == 0)
      return 0;

    get_block(fd, parent->INODE.i_block[i], buf);

    //setting where to start the interation through the data block
    dp = (DIR *)buf;
    cp = buf;

    while(cp < buf + BLKSIZE)
    {
      printf("(entered while loop!)\n"); //<========== testing
      //printf("(entered while loop)\n"); //<********** testing
      strncpy(temp, dp->name, dp->name_len);
      temp[dp->name_len] = 0;

      //if the file name is a match
      printf("%s\n", name);
      printf("%s\n", temp);
      if(strncmp(name, temp, dp->name_len) == 0)
      {
        //printf("(entered if statement !)\n"); //<========== testing
        //Only child dp->rec_len == BLKSIZE
        if(dp->rec_len == BLKSIZE)
        {
          //printf("(entered if statement !)\n"); //<========== testing
          bdalloc(parent->dev, parent->INODE.i_block[i]); // deallocate the data block
          parent->INODE.i_block[i] = 0;                   // set the data block num to 0 
          parent->INODE.i_blocks--;                       // decrease the parents block count by 1 
          parent->INODE.i_size -= BLKSIZE;                // reduce parent's file size by BLKSIZE

          for(int j = i + 1; j < 12; j++)
          {
            //printf("(entered for loop  (only child) !)\n"); //<========== testing
            parent->INODE.i_block[j - 1] = parent->INODE.i_block[j]; // moving evrything forward by 1 block
          }

          put_block(parent->dev, parent->INODE.i_block[i], buf);
          parent->modified = 1;
          printf("Successfully removed the specified directory! (only child)\n");
          return;
        }
        else if(cp + dp->rec_len >= buf + BLKSIZE) // last child case
        {
          //printf("(entered if statement (last child) !)\n"); //<========== testing
          prev_dp->rec_len += dp->rec_len; 
          put_block(parent->dev, parent->INODE.i_block[i], buf);
          parent->modified = 1;
          printf("Successfully removed the specified directory! (last child)\n");
          return;
        }
        else // middle child case
        {
          //printf("(entered if statement (middle child) !)\n"); //<========== testing
          char *cp2 = cp + dp->rec_len; // saving the current location in the data array
          DIR *dp2 = (DIR *)cp2;        // setting dp2 to cp2

          while(cp2 + dp2->rec_len < buf + BLKSIZE) //go to the very last file
          {
            cp2 += dp2->rec_len;
            dp2 = (DIR *)cp2;
          }

          dp2->rec_len += dp->rec_len;
          memmove(cp, (cp + dp->rec_len), ((buf + BLKSIZE) - (cp +dp->rec_len)));
          put_block(parent->dev, parent->INODE.i_block[i], buf);
          parent->modified = 1;
          printf("Successfully removed the specified directory! (middle child)\n");
          return;
        }
      }

      prev_dp = dp; //saving the entire directory pointer
      cp += dp->rec_len;
      dp = (DIR *)cp;
    }
  }
}