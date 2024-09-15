CFLAGS= -Wall -O2
CC=gcc
 
test_oled: test_oled.o drv_oled.o
	$(CC) $(CFLAGS) -o test_oled test_oled.o drv_oled.o
 
clean:
	rm -f test_oled test_oled.o drv_oled.o

