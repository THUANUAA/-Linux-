/***************************************************************
Copyright  2024-2029. All rights reserved.
文件名     : test_18_oled.c
作者       : tangmingfei2013@126.com
版本       : V1.0
描述       : 验证dev_oled.c 
其他       : 无
日志       : 初版V1.0 2024/02/23
***************************************************************/
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/types.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <linux/fs.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include "drv_oled.h"
 
 
void test_oled( void )
{
    oled_setCharSize( FONT_16 );  
    oled_printfString(6,0,(unsigned char *)"hello world!");  
    
    oled_setCharSize( FONT_12 );
    oled_printfString(0,6,(unsigned char*)"lululu");  
    oled_printfString(60,6,(unsigned char*)"lalalal");
}
 
int main(void)
{
    int set;
 
    set = oled_init();
    if( set < 0){
        printf("initial oled failure.\n");
        return -1;
    }
    
    test_oled();
    oled_release();
 
    return 0;
}
