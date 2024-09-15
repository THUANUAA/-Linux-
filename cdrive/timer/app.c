#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>


#define CLOSE_CMD          _IO(0XEF, 0x1)  /* 关闭定时器 */
#define OPEN_CMD           _IO(0XEF, 0x2)  /* 打开定时器 */
#define SETPERIOD_CMD      _IO(0XEF, 0x3)  /* 设置定时器周期命令 */

/** ./timerAPP /dev/timer
  * 
  * 
*/
int main(int argc, char *argv[])
{
    char *pathname;
    int fd, ret;
    unsigned int cmd, arg;

    pathname = argv[1];

    if(argc != 2)
    {
        printf("%s 输入错误!\n", pathname);
        return -1;
    }

    fd = open(pathname, O_RDWR);
    if(fd < 0)
    {
        printf("open %s error!\n", pathname);
    }
    while (1)
    {
        printf("输入命令:");
        ret = scanf("%d", &cmd);
        if (ret != 1) 
        {
            /* 参数输入错误 */
            printf("参数输入错误!\n");
            while('\n' != getchar());  /* 防止卡死 */
        }
        switch(cmd)
        {
            case 1: /* 关闭 LED 灯 */
                cmd = CLOSE_CMD;
                break;
            case 2: /* 打开 LED 灯 */
                cmd = OPEN_CMD;
                break;
            case 3: /* 设置周期值 */
                cmd = SETPERIOD_CMD; 
                printf("设置周期:");
                ret = scanf("%d", &arg);
                if (ret != 1)
                { 
                    /* 参数输入错误 */
                    printf("参数输入错误!\n");
                    while('\n' != getchar());  /* 防止卡死 */
                }
                break;
            default: 
                printf("输入错误!\n");
                break;
        }
        
        ioctl(fd, cmd, arg); /* 控制定时器的打开和关闭 */
    }

    if(close(fd) == -1)
    {
        printf("close %s error!\n", pathname);
    }

    return 0;
}

