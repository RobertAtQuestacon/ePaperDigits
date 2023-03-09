#ifndef IT8951
#define IT8951

uint8_t IT8951_Init();
void IT8951SystemRun();
void IT8951StandBy();
void IT8951Sleep();
void IT8951_BMP_Example(uint32_t x, uint32_t y, uint32_t w, uint32_t h);
void IT8951Load1bppImage(uint8_t* p1bppImgBuf, uint16_t usX, uint16_t usY, uint16_t usW, uint16_t usH);
uint8_t display_begin();
void display_buffer(uint8_t* addr, uint32_t x, uint32_t y, uint32_t w, uint32_t h);
void display_clear();
extern uint16_t display_width;
extern uint16_t display_height;

#define INIT_Mode 0
#define DU_Mode 1  // based on guiess from https://www.waveshare.com/w/upload/c/c4/E-paper-mode-declaration.pdf
#define GC16_Mode  2
//A2_Mode's value is not fixed, is decide by firmware's LUT 
#define A2_Mode 6  // true for 10.3" https://www.waveshare.com/wiki/6inch_HD_e-Paper_HAT

extern uint16_t drawMode;

#endif
