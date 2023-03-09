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
extern uint16_t display_width;
extern uint16_t display_height;

#endif
