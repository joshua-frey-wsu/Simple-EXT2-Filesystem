#include "type.h"
#include <stdio.h>

// start up files
#include "util.c"
#include "link_unlink.c"
#include "cd_ls_pwd.c"
#include "mkdir_rmdir_creat.c"
#include "open_close_lseek.c"
#include "read_cat.c"
#include "write_cp.c"
#include "head_tail.c"

/************** globals **************/
PROC   proc[NPROC];
PROC   *running;

MINODE minode[NMINODE];   // in memory INODES
MINODE *freeList;         // free minodes list
MINODE *cacheList;        // cached minodes list

MINODE *root;             // root minode pointer

OFT    oft[NOFT];         // for level-2 only

char gline[256];          // global line hold token strings of pathname
char *name[64];           // token string pointers
int  n;                   // number of token strings                    

int ninodes, nblocks;     // ninodes, nblocks from SUPER block
int bmap, imap, inodes_start, iblk;  // bitmap, inodes block numbers

int  fd, dev;
char cmd[16], pathname[128], parameter[128];
int  requests, hits;

//variables to calculate mail man's algorithim
int inode_size;
int INODEsize;
int inode_per_block;
int ifactor;

void init()
{
  int i, j;
  // initialize minodes into a freeList
  for (i=0; i<NMINODE; i++)
  {
    MINODE *mip = &minode[i];
    mip->dev = mip->ino = 0;
    mip->id = i;
    mip->next = &minode[i+1];
  }
  minode[NMINODE-1].next = 0;
  freeList = &minode[0];       // free minodes list

  cacheList = 0;               // cacheList = 0

  for (i=0; i<NOFT; i++)
  {
    oft[i].shareCount = 0;     // all oft are FREE
  }
    
  for (i=0; i<NPROC; i++)
  {     // initialize procs
    PROC *p = &proc[i];    
    p->uid = p->gid = i;      // uid=0 for SUPER user
    p->pid = i+1;             // pid = 1,2,..., NPROC-1

    for (j=0; j<NFD; j++)
    {
      p->fd[j] = 0;           // open file descritors are 0
    }
  }
  
  running = &proc[0];          // P1 is running
  requests = hits = 0;         // for hit_ratio of minodes cache
}

int quit()
{
  MINODE *mip = cacheList;
  while(mip)
  {
    if (mip->shareCount)
    {
      mip->shareCount = 1;
      iput(mip);    // write INODE back if modified
    }
    mip = mip->next;
  }
  exit(0);
}

char *disk = "disk2";

int main(void) //int argc, char *argv[ ]
{
  char line[128];
  char buf[BLKSIZE];

  init();
  
  fd = dev = open(disk, O_RDWR);
  printf("dev = %d\n", dev);  // YOU should check dev value: exit if < 0

  // get super block of dev
  get_block(dev, 1, buf);
  SUPER *sp = (SUPER *)buf;  // you should check s_magic for EXT2 FS
  if(sp->s_magic != SUPER_MAGIC) // sp->s_magic MUST be 0xEF53
  {
    printf("Super_magic = 0x%x is NOT an EXT2 file system\n", sp->s_magic);
    exit(0);
  }
  else
  {
    printf("check: superblock magic = 0x%x OK\n", sp->s_magic);
  }

  //get size from super block
  inode_size = sp->s_inode_size;          // 256
  INODEsize = sizeof(INODE);              // still 128
  inode_per_block = BLKSIZE / inode_size; // calculate 1024 / 256 = 4
  ifactor = inode_size / INODEsize;       // calculate 256 / 128 = 2
  
  ninodes = sp->s_inodes_count;
  nblocks = sp->s_blocks_count;
  printf("ninodes=%d  nblocks=%d inode_size=%d\n", ninodes, nblocks, inode_size);
  printf("inodes_per_block=%d  ifactor=%d\n", inode_per_block, ifactor);

  get_block(dev, 2, buf);
  GD *gp = (GD *)buf;

  bmap = gp->bg_block_bitmap;
  imap = gp->bg_inode_bitmap;
  iblk = inodes_start = gp->bg_inode_table;

  printf("bmap=%d  imap=%d  iblk=%d\n", bmap, imap, iblk);

  printf("Creating P1 as running process\n");  

  root = iget(dev, 2); 
  running->cwd = iget(dev, 2);
  printf("root shareCount = %d\n", root->shareCount);
  
  while(1)
  {
    printf("**************************************************\n");
    printf("P%d running\n", running->pid);
    pathname[0] = parameter[0] = 0;
    
    printf("enter command [cd|ls|pwd|mkdir|create|rmdir|link|unlink|symlink  open|close|pfd|read|write|cat|cp  hits|show|exit] : ");
    fgets(line, 128, stdin);
    line[strlen(line)-1] = 0;    // kill \n at end

    if (line[0]==0)
      continue;
    
    sscanf(line, "%s %s %64s", cmd, pathname, parameter);
    //printf("pathname=%s parameter=%s\n", pathname, parameter); //<========== testing
    
    if (strcmp(cmd, "ls")==0)
      ls();
    if (strcmp(cmd, "cd")==0)
      cd();
    if (strcmp(cmd, "pwd")==0)
      pwd();
    
    if (strcmp(cmd, "mkdir") == 0)
      make_dir();
    if (strcmp(cmd, "create") == 0)
      creat_file(pathname); //***************************
    if (strcmp(cmd, "rmdir") == 0)
      rmdir();

    if (strcmp(cmd, "link") == 0)
      link();
    if (strcmp(cmd, "unlink") == 0)
      unlink();
    if (strcmp(cmd, "symlink") == 0)
      symlink();

    if (strcmp(cmd, "open") == 0)
      myopen();
    if (strcmp(cmd, "close") == 0)
      close_file(atoi(pathname));
    if (strcmp(cmd, "lseek") == 0)
      my_lseek(atoi(pathname), atoi(parameter));
    if (strcmp(cmd, "pfd") == 0)
      pfd();

    // if (strcmp(cmd, "read") == 0)
    // {
    //   char rbuf[BLKSIZE];
    //   read_file(atoi(pathname), rbuf, atoi(parameter));
    // }

    // if (strcmp(cmd, "write") == 0)
    // {
    //   char wbuf[BLKSIZE];
    //   write_file(atoi(pathname), wbuf);
    // }

    if (strcmp(cmd, "cat") == 0)
      cat(pathname);
    if(strcmp(cmd, "cp") == 0)
      cp();
    if (strcmp(cmd, "head") == 0)
      head(pathname);
    if (strcmp(cmd, "tail") == 0)
      tail(pathname);
    
    if (strcmp(cmd, "show") == 0)
      show_dir(running->cwd);
    if (strcmp(cmd, "hits") == 0)
      hit_ratio();
    if (strcmp(cmd, "exit") == 0)
	    quit();
  }
}

/******************** functions ********************/ 
void show_dir(MINODE *mip)
{
  // show contents of mip DIR: same as in LAB5
  printf("(entered show_dir function)\n"); //<********** testing
  char sbuf[BLKSIZE], temp[256];
  DIR *dp;
  char *cp;

  // ASSUME only one data block i_block[0]
  // YOU SHOULD print i_block[0] number here
  //printf("(i_block[%d])\n", ip->i_block[0]);

  get_block(fd, mip->INODE.i_block[0], sbuf);
  //printf("(exited get_block function)\n"); //<********** testing

  dp = (DIR *)sbuf;
  cp = sbuf;

  printf("i_block[0] = %d\n", mip->INODE.i_block[0]);

  printf("inode_num  rec_len  name_len  file_name\n");

  while(cp < sbuf + BLKSIZE)
  {
    //printf("(entered while loop)\n"); //<********** testing
    strncpy(temp, dp->name, dp->name_len);
    temp[dp->name_len] = 0;

    printf("  %4d      %4d     %4d        %s\n", dp->inode, dp->rec_len, dp->name_len, temp);

    cp += dp->rec_len;
    dp = (DIR *)cp;
  }
  //printf("(finish showing the root directories)\n"); //<********** testing
}

void hit_ratio()
{
  // print cacheList;
  // print the list as  freeList=[pid pri]->[pid pri] ->..-> NULL
  sortQueue(cacheList);

   MINODE *mip = cacheList;
   printf("cacheList = ");

   while(mip)
   {
    printf("c%d[%d %d]s%d --> ", mip->cacheCount, mip->dev, mip->ino, mip->shareCount);
    mip = mip->next;
   }
   printf("NULL\n");

  // compute and print hit_ratio
  int ratio_val = (100 * hits) / requests;

  printf("Request = %d  Hits = %d  Hits Ratio = %d%%\n", requests, hits, ratio_val);
}
