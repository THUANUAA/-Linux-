/***************************************************************
Copyright  2024-2029. All rights reserved.
文件名     : test_14_dht11.c
作者       : tangmingfei2013@126.com
版本       : V1.0
描述       : 测试dth11驱动程序
其他       : 无
日志       : 初版V1.0 2024/02/15
***************************************************************/
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/types.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <linux/fs.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <time.h>
 
#define DEV_FILE                              "/dev/treedht11"
 
 
int main(void)
{
    int fd;
    int count_run = 0;
    unsigned char data[4];
 
    fd = open(DEV_FILE, O_RDWR);
    if (fd == -1){
        printf("can not open file: %s \n", DEV_FILE);
        return -1;
    }
 
    while( count_run < 10000)
    {
        count_run++;
        if (read(fd, data, 4) == 4) {
            printf("get humidity  : %d.%d\n", data[0], data[1]);
            printf("get temprature: %d.%d\n", data[2], data[3]);
        } 
        else {
           perror("read dht11 device fail!\n");
        }
        sleep(1);
    }
 
    close(fd);
 
    return 0;
}
