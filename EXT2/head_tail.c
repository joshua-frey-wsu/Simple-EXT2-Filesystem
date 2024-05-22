void head(char *pathname)
{
    //open the file for READ
    int fd = open_file(pathname, 0);

    //read BLKSIZE into char buf[ ]
    int lines = 0;
    char buf[BLKSIZE];
    char result[BLKSIZE];

    while(lines < 10)
    {
        bzero(buf, BLKSIZE);
        my_read(fd, buf, BLKSIZE);

        //scan buf[ ] for \n; For each \n: lines++ until 10; add a 0 after LAST \n
        for(int i = 0; buf[i] != NULL; i++)
        {
            //printf("%d\n", lines);
            if(lines == 10)
            {
                buf[i - 1] = 0;
                strncpy(result, buf, i);
                break;
            }

            if(buf[i] == '\n')
            {
                lines++;

                if(lines > 10)
                {
                    buf[i - 1] = 0;
                    strncpy(result, buf, i);
                    break;
                }
            }
        }
    }
    
    //print buf as %s
    printf("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n");
    printf("%s\n", result);
    printf("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n");
}

void tail(char *pathname)
{
    //open the file for READ
    int fd = open_file(pathname, 0);

    //get file_size (in its INODE.i_size)
    MINODE *mip = path2inode(pathname);
    int file_size = mip->INODE.i_size;

    int lines = 0;
    char buf[BLKSIZE];
    char result[BLKSIZE];

    //lssek to (file_size - BLKSIZE) OR to 0 if (file_size < BLKSIZE)
    if(file_size < BLKSIZE)
    {
        my_lseek(fd, 0);
        bzero(buf, BLKSIZE);
        my_read(fd, buf, file_size); //n = read BLKSIZE into buf[ ]
    }
    else
    {
        my_lseek(fd, file_size - BLKSIZE);
        bzero(buf, BLKSIZE);
        my_read(fd, buf, BLKSIZE); //n = read BLKSIZE into buf[ ]
    }

    //printf("%s\n", buf);
    //scan buf[ ] for \n; For each \n: lines++ until 10; add a 0 after LAST \n
    for(int i = strlen(buf) - 1; i >= 0; i--)
    {
        //printf("%d\n", lines);
        if(lines == 10)
        {
            buf[i - 1] = 0;
            strncpy(result, buf, i);
            break;
        }

        if(buf[i] == '\n')
        {
            lines++;

            if(lines > 10)
            {
                buf[i - 1] = 0;
                strncpy(result, buf, i);
                break;
            }
        }
    }

    //print buf as %s
    printf("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n");
    printf("%s\n", result);
    printf("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n");
}
