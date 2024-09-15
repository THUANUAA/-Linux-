#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
int main(int argc,char *argv[])
{
	int fd;
	
	char buf[64] = "0";
	
	fd = open( "/dev/chrdev_led",O_RDWR);//打开设备节点
	
	if(fd < 0)
	{
		perror( "open error \n");
		return fd;
	}

	buf[0]= atoi( argv[1]);
	
	write( fd,buf,sizeof(buf)); //向内核层写数据
	
	close( fd);
	
	return 0;
}

