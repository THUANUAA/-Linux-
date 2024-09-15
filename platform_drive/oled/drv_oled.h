#ifndef __DRV__OLED_H
#define __DRV__OLED_H
 
#ifdef __cplusplus
extern "C" {
#endif
 
#define OLED_ADDR          0x3c
 
typedef enum
{
   FONT_16 = 0,
   FONT_12 = 1,
}OLED_CHARSIZE;
 
typedef enum
{
   PARA = 0,
   DATA = 0x40,
}OLED_PARE_REG;
 
typedef enum
{
   X_WIDTH = 128,
   Y_WIDTH = 64,
}OLED_SCREEN_SIZE;
 
int oled_init(void);
void oled_release( void );
 
void oled_clear(void);
 
void oled_setCharSize( unsigned char val );
void oled_setPostion( unsigned char x, unsigned char y) ;
void oled_displayOn(void);
void oled_displayOff(void);
 
void oled_printfChar(unsigned char x,unsigned char y,unsigned char val );
void oled_printfString(unsigned char x,unsigned char y,unsigned char *str );
 
 
 
#ifdef __cplusplus
}
#endif
 
#endif /* __DRV__OLED_H */
