#include "stdio.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <stdint.h>

#define DEV_NAME   "/dev/my_pwm_led"

#define LED_PWM_CMD_SET_DUTY         0x01
#define LED_PWM_CMD_SET_PERIOD       0x02
#define LED_PWM_CMD_SET_BOTH         0x03
#define LED_PWM_CMD_ENABLE           0x04
#define LED_PWM_CMD_DISABLE          0x05

struct led_pwm_param {
    int duty_ns;
    int period_ns;
};

void sleep_ms(unsigned int ms)
{
    struct timeval delay;
	delay.tv_sec = 0;
	delay.tv_usec = ms * 1000; 
	select(0, NULL, NULL, NULL, &delay);
}

int main(int argc, char **argv)
{
    int fd;
    int ret;
  
	/* 2. 打开文件 */
	fd = open(DEV_NAME, O_RDWR | O_NONBLOCK);   // | O_NONBLOCK

	if (fd < 0)
	{
		printf("can not open file %s, %d\n", DEV_NAME, fd);
		perror("open");
		return -1;
	}
     
    int buf = 3;
	struct led_pwm_param led_pwm;
	
	led_pwm.duty_ns = 500;
	led_pwm.period_ns = 5000;
    write(fd, &led_pwm, sizeof(led_pwm));
    sleep_ms(3000);

	for (int i = 0; i < 5; i++)
	{
		led_pwm.duty_ns += 300;
		if (led_pwm.duty_ns < led_pwm.period_ns)
			ioctl(fd, LED_PWM_CMD_SET_DUTY, &led_pwm.duty_ns);
		sleep_ms(1000);
	}

	// led_pwm.duty_ns = 0;
	// ioctl(fd, LED_PWM_CMD_SET_DUTY, &led_pwm.duty_ns);

	sleep_ms(3000);

	while(1)
	{
		if(led_pwm.duty_ns<=500)
		{
			while(led_pwm.duty_ns<led_pwm.period_ns)
			{
				ioctl(fd, LED_PWM_CMD_SET_DUTY, &led_pwm.duty_ns);
				sleep_ms(50);
				led_pwm.duty_ns += 300;
			}
		}
		else
		{
			while(led_pwm.duty_ns > 500)
			{
				ioctl(fd, LED_PWM_CMD_SET_DUTY, &led_pwm.duty_ns);
				sleep_ms(50);
				led_pwm.duty_ns -= 300;
			}
		}
		
	}

	close(fd);
    return 0;
}

