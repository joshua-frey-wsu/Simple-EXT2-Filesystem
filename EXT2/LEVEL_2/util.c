/*********** globals in main.c ***********/
extern PROC   proc[NPROC];
extern PROC   *running;

extern MINODE minode[NMINODE];   // minodes
extern MINODE *freeList;         // free minodes list
extern MINODE *cacheList;        // cacheCount minodes list

extern MINODE *root;

extern OFT    oft[NOFT];

extern char gline[256];   // global line hold token strings of pathname
extern char *name[64];    // token string pointers
extern int  n;            // number of token strings                    

extern int ninodes, nblocks;
extern int bmap, imap, inodes_start, iblk;  // bitmap, inodes block numbers

extern int  fd, dev;
extern char cmd[16], pathname[128], parameter[128];
extern int  requests, hits;

//mailman's algorithim values from main.c
extern int inode_size;
extern int INODEsize;
extern int inode_per_block;
extern int ifactor;

/******************** queue functions ********************/

//insert variable into queue by cacheCount in increasing order
void enqueue(MINODE **queue, MINODE *p)
{
    // enter p into queue by cacheCount; FIFO if same priority
    //printf("(Entered enqueue function)\n"); /////
    MINODE *q = *queue;

    //if queue is empty or p has lower cacheCount than q
    if(q == 0 || (p->cacheCount < q->cacheCount))
    {
        //printf("(entered if statement_1)\n"); /////

        *queue = p; //insert p as the first element
        p->next = q; //point p's next_pointer to q

        //printf("(exited if statement_1)\n"); /////
    }
    else 
    {
        //printf("(entered if statement_2)\n"); /////

        while(q->next && p->cacheCount >= q->next->cacheCount) //set p to last element
        {
            //printf("(entered while loop)\n");

            q = q->next;

            //printf("(exited while loop)\n");
        }
        
        p->next = q->next;
        q->next = p;

        //printf("(exited if statement_2)\n"); /////
    }
    //printf("(exited enqueue function)\n"); /////
}

//search for a variable inside the queue and remove it
MINODE *dequeue(MINODE **queue)
{
    //printf("(entered dequeue function)\n");
    MINODE *p = *queue;
    if(p)
    {
        //printf("(entered if statement)\n");
        *queue = (*queue)->next;
    }

    return p;
}

//sort cacheList in ascending order
void sortQueue(MINODE **queue)
{
  MINODE *p, *temp = 0;

  while(p = dequeue(&cacheList)) //take out every inode in cacheList, resort it and put it into temp
  {
    enqueue(&temp, p);
  }
  cacheList = temp; //set cacheList equal to temp
}

/**************** util.c file **************/

int get_block(int dev, int blk, char buf[ ])
{
  lseek(dev, blk*BLKSIZE, SEEK_SET);
  int n = read(fd, buf, BLKSIZE);
  return n;
}

int put_block(int dev, int blk, char buf[ ])
{
  lseek(dev, blk*BLKSIZE, SEEK_SET);
  int n = write(fd, buf, BLKSIZE);
  return n; 
}       

void tokenize(char *pathname)
{
  // tokenize pathname into n token strings in (global) gline[ ]
  char *s;
  strcpy(gline, pathname);   // copy into global gline[]
  s = strtok(gline, "/");    
  n = 0;
  while(s)
  {
    name[n++] = s;           // token string pointers   
    s = strtok(0, "/");
  }
  name[n] = 0;               // name[n] = NULL pointer
}

MINODE *iget(int dev, int ino) // return minode pointer of (dev, ino)
{
  requests ++;
  //printf("iget %d\n", ino); //<========== testing
  /********** Write code to implement these ***********/
  /*1. search cacheList for minode=(dev, ino);
  if (found){
      inc minode's cacheCount by 1;
      inc minode's shareCount by 1;
      return minode pointer;
  }*/
  MINODE *header = cacheList;

  while(header)
  {
    if(header->dev == dev && header->ino == ino)
    {
      //printf("found minode %d [ %d %d ] in cache\n", header->id, dev, ino);
      hits ++;
      header->cacheCount ++;
      header->shareCount ++;
      return header;
    }
    header = header->next;
  }

  // needed (dev, ino) NOT in cacheList
  /*2. if (freeList NOT empty){
        remove a minode from freeList;
        set minode to (dev, ino), cacheCount=1 shareCount=1, modified=0;

        load INODE of (dev, ino) from disk into minode.INODE;

        enter minode into cacheList; 
        return minode pointer;
      }*/
      
  MINODE *mip = dequeue(&freeList);
  if(mip)
  {
    mip->dev = dev;
    mip->ino = ino;
    mip->cacheCount = 1;
    mip->shareCount = 1;
    mip->modified = 0;

    //load INODE of (dev, ino) into mip->INODE
    int blk = (ino-1) / inode_per_block + iblk;
    int offset = (ino-1) % inode_per_block;
    
    char buf[BLKSIZE];
    get_block(dev, blk, buf); //get blk into char buf[BLKSIZE]
    
    INODE *ip = (INODE*)buf + offset * ifactor;
    mip->INODE = *ip;

    //enter mip into cacheList ordered by INCREASING cacheCount
    enqueue(&cacheList, mip);
    return mip;
  }

  // freeList empty case: you are reusing a minode in cacheList 
  /*3. find a minode in cacheList with shareCount=0, cacheCount=SMALLest
      set minode to (dev, ino), shareCount=1, cacheCount=1, modified=0
      return minode pointer;

  NOTE: in order to do 3:
        it's better to order cacheList by INCREASING cacheCount,
        with smaller cacheCount in front ==> search cacheList
  */
  
  //get an inode with the smallest cacheCount value
  sortQueue(cacheList); //resort the cacheList first
  header = cacheList; //reset header pointer position

  header->dev = dev;
  header->ino = ino;
  header->cacheCount = 1;
  header->shareCount = 1;
  header->modified = 0;

  //load INODE of (dev, ino) into mip->INODE
  int blk = (ino-1) / inode_per_block + iblk;
  int offset = (ino-1) % inode_per_block;
  
  char buf[BLKSIZE];
  get_block(dev, blk, buf);
  
  INODE *ip = (INODE *)buf + offset * ifactor;
  header->INODE = *ip;

  return header;
}

void iput(MINODE *mip)  // release a mip
{
  //printf("iput %d\n", mip->ino); //<========== testing
  INODE *ip;
  int blk, offset;
  char buf[BLKSIZE];
  /*
  1.  if (mip==0)                return;

     mip->shareCount--;         // one less user on this minode

     if (mip->shareCount > 0)   return;
     if (!mip->modified)        return;
  */

  if(mip == 0)
    return;
  
  mip->shareCount--;  // one less user on this minode
  if(mip->shareCount < 0) //check if shareCount is a negetive number
  {
    printf("WARNING : Invalid shareCount!!!\n");
    return;
  }

  if(mip->shareCount > 0)
    return;
  
  if(!mip->modified)
    return;
  /*
  2. // last user, INODE modified: MUST write back to disk
      Use Mailman's algorithm to write minode.INODE back to disk)
      // NOTE: minode still in cacheList;
  */

  // write INODE back to disk
  blk = (mip->ino-1) / inode_per_block + iblk;
  offset = (mip->ino-1) % inode_per_block;
    

  //get block containing this inode
  get_block(mip->dev, blk, buf);
  ip = (INODE *)buf + offset * ifactor;     // ip points at INODE [* sizeof(INODE)]
  *ip = mip->INODE;               // copy INODE to inode in block
  put_block(mip->dev, blk, buf);  //write back to disk

  dequeue(&cacheList);            //remove minode from cacheList
} 

int search(MINODE *mip, char *name)
{
  /******************
  search mip->INODE data blocks for name:
  if (found) return its inode number;
  else       return 0;
  ******************/
  int i;
  char sbuf[BLKSIZE], temp[256];
  DIR *dp;
  char *cp;

  // ASSUME only one data block i_block[0]
  // YOU SHOULD print i_block[0] number here
  //printf("(i_block[%d])\n", mip->INODE.i_block[0]); //<========== testing

  for(i = 0; i < 12; i++) //search Direct blocks only
  {
    if(mip->INODE.i_block[i] == 0)
      return 0;

    get_block(fd, mip->INODE.i_block[i], sbuf);

    dp = (DIR *)sbuf;
    cp = sbuf;

    while(cp < sbuf + BLKSIZE)
    {
      //printf("(entered while loop)\n"); //<********** testing
      strncpy(temp, dp->name, dp->name_len);
      temp[dp->name_len] = 0;

      //printf("  %4d      %4d     %4d        %s\n", dp->inode, dp->rec_len, dp->name_len, temp); //<========== testing

      //if the file name is a match
      if(strcmp(name, temp) == 0)
      {
          //return the inode number
          //printf("found %s: inode_num = %d\n", name, dp->inode); //<========== testing
          return dp->inode;
      }

      cp += dp->rec_len;
      dp = (DIR *)cp;
    }
  }
  return 0;
}

MINODE *path2inode(char *pathname) 
{
  /*******************
  return minode pointer of pathname;
  return 0 if pathname invalid;
  
  This is same as YOUR main loop in LAB5 without printing block numbers
  *******************/

  MINODE *mip;

  if(pathname[0] == '/')  //check if pathname is absolute
    mip = root;
  else
    mip = running->cwd;

  tokenize(pathname); //tokenize the input command

  for(int i = 0; i < n; i++)
  {
    if(!S_ISDIR(mip->INODE.i_mode)) //check if the given name is a directory type
    {
      printf("The given pathname is not a directory type!!!\n");
      return 0;
    }

    int ino = search(mip, name[i]); //check if the given name exists
    if(ino == 0)
    {
      printf("Can not find the specified file!!!\n");
      return 0;
    }

    if(i != 0) //release current mip only if we called it multiple times
    {
      iput(mip);            
    }

    mip = iget(dev, ino); //change to new mip of (dev, ino)
  }
  return mip;
}   

int findmyname(MINODE *pip, unsigned int myino, char myname[256]) 
{
  /****************
  pip points to parent DIR minode: 
  search for myino;    // same as search(pip, name) but search by ino
  copy name string into myname[256]
  ******************/
  char sbuf[BLKSIZE], temp[256];
  DIR *dp;
  char *cp;

  // ASSUME only one data block i_block[0]
  // YOU SHOULD print i_block[0] number here
  //printf("(i_block[%d])\n", pip->INODE.i_block[0]); //<========== testing

  if(pip->INODE.i_block[0] == 0)
    return 0;

  get_block(fd, pip->INODE.i_block[0], sbuf);

  dp = (DIR *)sbuf;
  cp = sbuf;

  while(cp < sbuf + BLKSIZE)
  {
    //printf("(entered while loop)\n"); //<********** testing
    strncpy(temp, dp->name, dp->name_len);
    temp[dp->name_len] = 0;

    //printf("  %4d      %4d     %4d        %s\n", dp->inode, dp->rec_len, dp->name_len, temp); //<========== testing

    //if the myino number is a match
    if(dp->inode == myino)
    {
      //#pragma GCC diagnostic push
      //#pragma GCC diagnostic ignored "-Wsizeof-pointer-memaccess"
      strncpy(myname, temp, sizeof(temp));
      //#pragma GCC diagnostic pop
      //return the inode number
      //printf("found %s: inode_num = %d\n", dp->name, dp->inode); //<========== testing
      return dp->inode;
    }

    cp += dp->rec_len;
    dp = (DIR *)cp;
  }

  return 0;
}

int findino(MINODE *mip, int *myino) 
{
  /*****************
  mip points at a DIR minode
  i_block[0] contains .  and  ..
  get myino of .
  return parent_ino of ..
  *******************/
  *myino = search(mip, "."); //returns current directory
  return search(mip, "..");  //returns the parent_ino of ..

}
