/***************************************************************
Copyright  2024-2029. All rights reserved.
文件名     : drv_oled.c
作者       : tangmingfei2013@126.com
版本       : V1.0
描述       : oled 驱动程序
其他       : 无
日志       : 初版V1.0 2024/02/23
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
#include "drv_oled.h"
#include "font.h"
 
#define DEV_FILE               "/dev/i2c-1"
 
#define grayoled_swap(a, b)                                                    \
  (((a) ^= (b)), ((b) ^= (a)), ((a) ^= (b))) ///< No-temp-var swap operation
 
static int fd = -1;
static unsigned char charSize=0; 
 
static int oled_write_reg( unsigned char data, unsigned char mode)
{
    int              ret = -1;
    unsigned char    buf[2] = {0};
    
    if( mode ){
        buf[0] = 0x40;
    }
    else
    {
        buf[0] = 0;
    }
 
    buf[1] = data;
    ret = write(fd, buf, 2);
    if( ret < 0 )
    {
        printf("write cmd to oled register failure.\n");
        return -1;
    }
    
    return 0;
}
 
static void oled_drv_Init(void)
{ 
    oled_write_reg(0xAE,PARA);//--display off
    oled_write_reg(0x00,PARA);//---set low column address
    oled_write_reg(0x10,PARA);//---set high column address
    oled_write_reg(0x40,PARA);//--set start line address  
    oled_write_reg(0xB0,PARA);//--set page address
    oled_write_reg(0x81,PARA); // contract control
    oled_write_reg(0xFF,PARA);//--128   
    oled_write_reg(0xA1,PARA);//set segment remap 
    oled_write_reg(0xA6,PARA);//--normal / reverse
    oled_write_reg(0xA8,PARA);//--set multiplex ratio(1 to 64)
    oled_write_reg(0x3F,PARA);//--1/32 duty
    oled_write_reg(0xC8,PARA);//Com scan direction
    oled_write_reg(0xD3,PARA);//-set display offset
    oled_write_reg(0x00,PARA);//
 
    oled_write_reg(0xD5,PARA);//set osc division
    oled_write_reg(0x80,PARA);//
 
    oled_write_reg(0xD8,PARA);//set area color mode off
    oled_write_reg(0x05,PARA);//
 
    oled_write_reg(0xD9,PARA);//Set Pre-Charge Period
    oled_write_reg(0xF1,PARA);//
 
    oled_write_reg(0xDA,PARA);//set com pin configuartion
    oled_write_reg(0x12,PARA);//
 
    oled_write_reg(0xDB,PARA);//set Vcomh
    oled_write_reg(0x30,PARA);//
 
    oled_write_reg(0x8D,PARA);//set charge pump enable
    oled_write_reg(0x14,PARA);//
 
    oled_write_reg(0xAF,PARA);//--turn on oled panel
    
    oled_clear();
}
 
void oled_clear(void)  
{  
  unsigned char i,n;
  
  for(i=0;i<8;i++)  
  {  
      oled_write_reg (0xb0+i,PARA);   
      oled_write_reg (0x00,PARA);
      oled_write_reg (0x10,PARA); 
      
      for( n = 0;n < 128; n++)
      {
          oled_write_reg(0,DATA);
      } 
  } 
}
 
 
void oled_release( void )
{
    close( fd );
}
 
int oled_init(void)
{
    // init i2c 
	/* 打开I2C控制器节点 */
    fd = open(DEV_FILE, O_RDWR);
    if( fd < 0 )
    {
        close( fd );
        printf("%s %s i2c device open failure: %s\n", __FILE__, __FUNCTION__, strerror(errno));
        return -1;
    }
	
	/* 设置地址为长 0为7bit地址 */
    ioctl(fd, I2C_TENBIT, 0);

	/* 设置设备地址 */
    ioctl(fd, I2C_SLAVE, OLED_ADDR);
    
    oled_drv_Init();
     
    return fd;
}
 
/*
  oled 12864 API 
*/
 
void oled_setCharSize( unsigned char val )
{
   charSize = val;
}
 
void oled_setPostion( unsigned char x, unsigned char y) 
{
    oled_write_reg(0xb0+y,PARA);
    oled_write_reg(((x&0xf0)>>4)|0x10,PARA);
    oled_write_reg((x&0x0f),PARA); 
}
 
void oled_displayOn(void)
{
    oled_write_reg(0X8D,PARA); 
    oled_write_reg(0X14,PARA); 
    oled_write_reg(0XAF,PARA);
}
  
void oled_displayOff(void)
{
    oled_write_reg(0X8D,PARA);
    oled_write_reg(0X10,PARA);
    oled_write_reg(0XAE,PARA);
}
 
 
void oled_upScreenOn(void)  
{  
    unsigned char i,n;
    
    for(i=0;i<8;i++)  
    {  
        oled_write_reg (0xb0+i,PARA);
        oled_write_reg (0x00,PARA); 
        oled_write_reg (0x10,PARA);
        
        for(n=0;n<128;n++)
        {
            oled_write_reg(1,DATA); 
        }
    } 
}
 
void oled_printfChar(unsigned char x,unsigned char y,unsigned char val )
{ 
    unsigned char character = 0;
    unsigned char i=0;
    
    character = val-' ';
    if( x > X_WIDTH-1)
    {
      x = 0;
      y += 2;
    }
    
    if( charSize == FONT_16 )
    {
          oled_setPostion(x,y);	
          for( i = 0; i < 8; i++)
          {
              oled_write_reg(F8X16[character*16+i],DATA);
          }
          
          oled_setPostion(x,y+1);
          for(i=0;i<8;i++)
          {
              oled_write_reg(F8X16[character*16+i+8],DATA);
          }
     }
    else 
    {
        oled_setPostion(x,y);
        for(i=0;i<6;i++)
        {
            oled_write_reg(F6x8[character][i],DATA);
        }
    }
}



void oled_printfString(unsigned char x,unsigned char y,unsigned char *str  )
{
    unsigned char index = 0;
    
    while ( str[index]!='\0' )
    {
        oled_printfChar(x,y,str[index] );
       
        x += 8;
        if( x > 120 )
        {  
            x=0;
            y+=2;
        }
        index++;
    }
}
 
void oled_printmutiChar(unsigned char x,unsigned char y,unsigned char *str,  unsigned char length )
{
    unsigned char index = 0;
    
    for( index = 0; index < length; index++ )
    {
        oled_printfChar(x,y,str[index] );
       
        x += 8;
        if( x > 120 )
        {  
            x=0;
            y+=2;
        }
    }
}
 
 
/* Eed of this file */
 
