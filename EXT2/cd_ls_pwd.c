/*********** cd_ls_pwd.c file ***********/

void cd()
{
  //printf("cd: Under Construction\n");

  // write YOUR code for cd
  MINODE *mip = path2inode(pathname);

  if(!mip) //if mip DNE
  {
    printf("Could not find the specified pathname!!!\n");
    return;
  }

  if(!S_ISDIR(mip->INODE.i_mode)) //if pathname is not a directory
  {
    printf("The given pathname is not a directory!!!\n");
    return;
  }

  printf("cd to [dev ino]=[%d %d]\n", mip->dev, mip->ino);

  iput(running->cwd); //release current cwd
  running->cwd = mip;

  printf("after cd: cwd = [%d %d]\n", mip->dev, mip->ino);
}

char *t1 = "xwrxwrxwr-------";
char *t2 = "----------------";
char fileBuf[4096];

void ls_file(MINODE *mip, char *name)
{
  //printf("ls_file: under construction\n");
  // use mip->INODE to ls_file

  int i;
  char ftime[64];

  //printf("test 0*****\n"); //<========== testing

  if (S_ISREG(mip->INODE.i_mode)) // if (S_ISREG())
  {
    printf("%c",'-');
  }
  if (S_ISDIR(mip->INODE.i_mode)) // if (S_ISDIR())
  {
    printf("%c",'d');
  }
  if (S_ISLNK(mip->INODE.i_mode)) // if (S_ISLNK())
  {
    printf("%c",'l');
  }
  //printf("test 33*****\n"); //<========== testing

  for (i=8; i >= 0; i--)
  {
    //printf("(entered for loop*****)\n"); //<========== testing
    if (mip->INODE.i_mode & (1 << i)) // print r|w|x, max size one character
    {
      printf("%c", t1[i]);
    }
    else
    {
      printf("%c", t2[i]); // or print -
    }
  }

  printf("%4d ",mip->INODE.i_links_count);    // link count
  printf("%4d ",mip->INODE.i_gid);            // gid
  printf("%4d ",mip->INODE.i_uid);            // uid

  // print time
  time_t mytime = mip->INODE.i_mtime; //needs time_t feild item
  //strcpy(ftime, ctime(&((time_t)mip->INODE.i_mtime)));  // print time in calendar form
  strcpy(ftime, ctime(&mytime));      //ctime() address of mytime into ftime
  ftime[strlen(ftime)-1] = 0;         // kill \n at end
  printf(" %s",ftime);                //print time
  printf("%8d ",mip->INODE.i_size);   // file size
  
  // print name
  printf("  %s    ", basename(name)); // print file basename

  printf("[ %d %d ]", mip->dev, mip->ino);
  
  // print -> linkname if symbolic file
  if (S_ISLNK(mip->INODE.i_mode))
  {
    //printf("enterd if statement*****\n"); //<========== testing
    // use readlink() to read linkname
    printf(" --> %s", my_readlink(mip)); // print linked name
  }

  printf("\n");
}
  
void ls_dir(MINODE *pip)
{
  char sbuf[BLKSIZE], name[256];
  DIR  *dp;
  char *cp;
  MINODE *mip;

  get_block(dev, pip->INODE.i_block[0], sbuf);
  dp = (DIR *)sbuf;
  cp = sbuf;

  printf("i_block[0] = %d\n", pip->INODE.i_block[0]);
    
  while (cp < sbuf + BLKSIZE)
  {
    strncpy(name, dp->name, dp->name_len);
    name[dp->name_len] = 0;
    // printf("%s\n",  name);
    mip = iget(dev, dp->inode); //store the info of the file into mip
    ls_file(mip, name);
    iput(mip); //release the mip

    cp += dp->rec_len;
    dp = (DIR *)cp;
  }

}

void ls()
{
  MINODE *mip;
  int checker = 0;

  if(pathname[0] == 0) //if pathname[0] does not exist 
  {
    mip = running->cwd;
  }
  else
  {
    mip = path2inode(pathname); //check mip not NULL!!
    checker = 1;
  }

  if(S_ISDIR(mip->INODE.i_mode)) //check if mip->INODE is a directory
  {
    ls_dir(mip);
  }
  else //else mip->INODE is a file type
  {
    ls_file(mip, basename(pathname));
  }
  // //if the pathname[0] == 0, we ls the current working directory (cwd)
  // MINODE *mip = running->cwd;
 
  // ls_dir(mip);
  if(checker == 1)
  {
    iput(mip); //release current mip
  }
}

/********** pwd code **********/
void rpwd(MINODE *wd)
{
  char myname[256];

  if(wd == root)
    return;

  int myino;
  int parentino = findino(wd, &myino);

  MINODE *pip = iget(dev, parentino);

  findmyname(pip, myino, myname);

  rpwd(pip);

  iput(pip);

  printf("/%s", myname);
}

void pwd()
{
  //raise(SIGTRAP);
  //printf("CWD = %s\n", "/");

  MINODE *wd = running->cwd;

  printf("current location: ");

  if(wd == root)
  {
    printf("/\n");
  }
  else
  {
    rpwd(wd);
    printf("\n");
  }
}
