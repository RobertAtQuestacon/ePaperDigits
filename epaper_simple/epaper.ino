// Simple test to put digits on Waveshare E-Paper-IT8951-Driver-HAT using Sparkfun ESP32 Thing
// basic code from https://github.com/clashman/it8951

#include "IT8951.h"
#include <Adafruit_SleepyDog.h>

#include "img_0.h"
#include "img_1.h"
#include "img_2.h"
#include "img_3.h"
#include "img_4.h"
#include "img_5.h"
#include "img_6.h"
#include "img_7.h"
#include "img_8.h"
#include "img_9.h"
#include "img_decimal.h"
#include "img_blank.h"

unsigned char* pic[] = { img_0, img_1, img_2, img_3, img_4, img_5, img_6, img_7, img_8, img_9, img_decimal, img_blank};
unsigned int pic_width[] = { img_0_width, img_1_width, img_2_width, img_3_width, img_4_width, img_5_width, img_6_width, img_7_width, img_8_width, img_9_width, img_decimal_width, img_blank_width};
unsigned int pic_height[] = { img_0_height, img_1_height, img_2_height, img_3_height, img_4_height, img_5_height, img_6_height, img_7_height, img_8_height, img_9_height, img_decimal_height, img_blank_height};
int y_offset = 0;
int y_height = 400;
int x_offset = 20;

int x = 20;
int y = 20;
bool blank = true; 
char ls[30];
float ccnt[] = { 99.99999,99.999,99.999};
int places[] = { 8, 6, 6};

void setup(void)
{
    Serial.begin(115200);
    Serial.println("\n\n\n");

    /*
    Serial.printf("MISO %i\n", MISO);
    Serial.printf("MOSI %i\n", MOSI);
    Serial.printf("SCK %i\n", SCK);
    Serial.printf("CS %i\n", CS);
    Serial.printf("RESET %i\n", RESET);
    Serial.printf("HRDY %i\n", HRDY);
    */
    Serial.println("Display init ...");
    if(!display_begin()) {
        Serial.println("Display init failed");
        exit(1);
    };
    Serial.println("Display init ok");
}

void loop() {
  if(blank) {
    //x -= x%4; // somehow the image is distorted when not aligned like this
    //y -= y%4;
    display_buffer(img_blank, x, y, img_blank_width, img_blank_height);
    x += img_blank_width-8;
    if(x > display_width) {
      x = 0;
      y += img_blank_height;
      Serial.print("Y:");
      Serial.println(y);
      if(y > display_height) {
        blank = false;
      }
    }
  } else {
    char ss[10];
    for(int ni = 0; ni < 3; ni++) {
      sprintf(ss,"%7.5f",ccnt[ni]);
      for(int i = 0; i < places[ni]; i++) {
        if(ss[i] != ls[ni*10 + i]) {
          char c = ss[i];
          ls[ni*10 + i] = c;
          int ic = 11; // blank
          if (c == '.')
            ic = 10;
          else if (c >= '0' && c <= '9')
            ic = c - '0'; 
          x = x_offset + (8-i)*img_blank_width;  
          //x -= x%4; // somehow the image is distorted when not aligned like this
          y = y_offset + ni*y_height;
          //y -= y%4; // somehow the image is distorted when not aligned like this
          display_buffer(pic[ic], x, y, pic_width[ic], pic_height[ic]);
        }
      }
    }
    ccnt[0] += 0.00013;
    ccnt[1] = ccnt[0]*100/75;
    ccnt[2] = ccnt[0]*418/75;
  }
  delay(1);
  IT8951Sleep();
  //IT8951StandBy();
  int sleepMS = Watchdog.sleep(1000);
  Serial.print("Slept for:");
  Serial.println(sleepMS);
  //delay(2000);
  IT8951SystemRun();
}
