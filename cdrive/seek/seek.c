#include <unistd.h> 
#include <fcntl.h> 
#include <sys/types.h> 
#include <stdio.h> 
 
#define CHAR_DEVICE "toto.txt" 
 
int main(int argc, char **argv) 
{ 
    int fd= 0; 
    char buf[20]; 
    if ((fd = open(CHAR_DEVICE, O_RDONLY)) < -1) 
        return 1; 
 
    /* 读取20字节*/ 
    if (read(fd, buf, 20) != 20) 
        return 1; 
    printf("%s\n", buf); 
 
    /* 将光标相对于其实际位置移动10次*/ 
    if (lseek(fd, 10, SEEK_CUR) < 0) 
        return 1; 
    if (read(fd, buf, 20) != 20) 
        return 1; 
    printf("%s\n",buf); 
 
    /* 将光标相对于文件的起始位置移动10次 */ 
    if (lseek(fd, 7, SEEK_SET) < 0) 
        return 1; 
    if (read(fd, buf, 20) != 20)         
	    return 1; 
    printf("%s\n",buf); 
 
    close(fd);  
    return 0; 
}
