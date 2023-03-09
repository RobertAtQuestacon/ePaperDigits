// basic code from https://github.com/clashman/it8951

//v4 = wouldn't fit all 12 images of double size onto ESP32
//v14 = loading from external eeprom but having trouble with startup background
//v15 = added initial blanking, but after a night off, blanking actually makes things worse
//v16 = had the funny 1
//v17 = the one corrected
//v18 = added PROGMEM ot digit defines but made no difference to compile size (371717 prog mem, 88684 RAM)
//v19 - used esp sleep commands instead off sleepy dog
//v21 - align numbers to cutout
#define VERSION "V21"

#include "IT8951.h"
//#include <Adafruit_SleepyDog.h> - replaced with esp commands

const int BUTTON_PIN = 0;
const int LED_PIN = 5;

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
int x_offset[3] = {32, 32, 32};  // needs to be on increments of 4
int y_offset[3] = {35, 560, 1085};
int edit_index = 0;
int current_line = 0;
int current_place = 0;
char line_string[10];
char last_string[30];
const int line_length = 10;
bool refresh = true;  // so that it refreshes immediately
bool refresh_next = false;
bool eco_mode = true;

// Waveshare 10.3" e-Paper has 1872×1404 resolution or 234bytes wide at 1bpp, total bytes = 328536

int x = 0;
int y = 0;
int blank_laps = 0; // no blanking unless requested
char ls[30];
unsigned long lcnt = 0;  // last count should be equal to ccnt[0] after save to EEPROM
unsigned long ccnt[] = { 0, 0, 0};
const int text_width = 8;
int places[] = { 5, 3, 3};
unsigned long last_reset_tm = 0;
unsigned long last_save_tm = 0;
bool new_line = false;
bool new_page = false;
int refresh_period = 60;
int standard_mode = A2_Mode;  // DU_Mode; // A2 mode is faster but with ghosting 
int refresh_mode = GC16_Mode;  // doesn't just look for changes
int draw_modes[] = {INIT_Mode, DU_Mode, GC16_Mode, A2_Mode };

long lastLoopUS = 0;

int verbose_flag = 0;
#define MAX_PC_MSG 128
char pcMsg[MAX_PC_MSG+1];  // allow for terminator
int pcMsgIndex = 0;
float pcMsgVal = 0;
uint32_t pcMsgHex = 0;
bool valid;

#include "Adafruit_EEPROM_I2C.h"

/* Example code for the Adafruit I2C EEPROM breakout */

/* Connect SCL    to SCL
   Connect SDA    to SDA
   Connect VDD    to 3 - 5V DC
   Connect GROUND to common ground */
   
Adafruit_EEPROM_I2C i2ceeprom;

#define EEPROM_ADDR 0x50  // the default address!
unsigned rec_addr = 0;
uint8_t usage; 

typedef union saveType {
  unsigned long ul;
  byte b[4];
};

saveType ul;

void load_data(union saveType &ut, unsigned addr, int len = 4) {
  //Serial.print("Reading:");
  for(int i = 0; i < len; i++) {
    ut.b[i] = i2ceeprom.read(addr + i);
    //Serial.print(ut.b[i]); Serial.print(":");
  }  
  //Serial.println();
}
void save_data(union saveType &ut, unsigned addr, int len = 4) {
  //Serial.print("Writing:");
  for(int i = 0; i < len; i++) {
    i2ceeprom.write(addr + i,ut.b[i]);
    //Serial.print(ut.b[i]); Serial.print(":");
  }  
  //Serial.println();
}

bool isVerbose(int flag) {
  return((verbose_flag & flag) != 0);
}

bool isCmd(const char* test, bool vb = false) {  
  int lng = strlen(test);
  //Serial.println(test);
  if(strncmp(pcMsg,test,lng) != 0) 
    return false;
  valid = (strlen(pcMsg) > lng + 1);
  if(valid) {  
    pcMsgVal = atof(pcMsg + lng + 1);
    if(isVerbose(vb)) {
      Serial.print(test);
      Serial.print(" = ");
      Serial.println(pcMsgVal);
    }  
  } 
  return true;
}

bool isCmdHex(const char* test, bool vb = false) { 
  int lng = strlen(test);
  //Serial.println(test);
  if(strncmp(pcMsg,test,lng) != 0) 
    return false;
  //Serial.println(strlen(pcMsg));
  //Serial.println(lng);  
  valid = (strlen(pcMsg) > lng + 1);
  if(valid) {  
    pcMsgHex = strtol(pcMsg + lng + 1, 0, 16);
    if(isVerbose(vb)) {
      Serial.print(test);
      Serial.print(" = 0x");
      Serial.println(pcMsgHex, HEX);
    }  
  } 
  return true;
}

void findOld() { 
  // find next 5 byte address
  usage = i2ceeprom.read(rec_addr+4);  // usage of that EEPROM location
  if(usage == 0)  //zero can't be used
    usage = 1;
  Serial.print("Usage at addr 0 = "); Serial.println(usage);
  for(int i = 0; i < 819; i++) { 
    rec_addr += 5;
    uint8_t cnt = i2ceeprom.read(rec_addr+4);
    load_data(ul,rec_addr);
    Serial.print(i); Serial.print("Usage at next = "); Serial.print(cnt); Serial.print("="); Serial.println(ul.ul);
    if(cnt != usage) {
      load_data(ul,rec_addr-5);  //load calories assumes the previous record is true
      return;
    }
  }
  rec_addr = 0;   // when all else fails   
}

void saveNew() {
  save_data(ul,rec_addr);
  i2ceeprom.write(rec_addr+4, usage);  // must happen after saving data so on reboot we know this data is valid
  uint8_t cnt = i2ceeprom.read(rec_addr+4);
  if(cnt != usage) {  // we've gone past the end of the EEPROM
    rec_addr = 0;  // so reuse the first address
    usage += 1;
    i2ceeprom.write(rec_addr+4, usage);
  } 
  if(isVerbose(64)) {
    Serial.print("Saving "); Serial.print(ul.ul);
    Serial.print(":"); Serial.print(usage);
    Serial.print(" to "); Serial.println(rec_addr); 
  }
  rec_addr += 5;
} 

void setup(void)
{
    pinMode(GPIO_NUM_13, INPUT_PULLUP);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT);
    
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
    x = 0;
    y = 0;
    drawMode = standard_mode;
    Serial.println("Command options:\nversion\nverbose flag\nblank [loops]\nreset [val]\nrefresh [seconds]");
    Serial.println("edit [line]\nx [pos]\ny [pos]\nPress and release button 0 on ESP32 before using command");

    /*
    drawMode = DU_Mode;  // something about this mode sets up the Init mode
    display_buffer(img_blank, 0, 0, img_blank_width, img_blank_height);
    drawMode = INIT_Mode; // for intial clear
    Serial.println("Display clear");
    display_clear();
    */

    // force first line to refresh
    current_place = text_width + 2;
    current_line = -1;

    delay(3000);   // temporary while we debug

    if (i2ceeprom.begin(0x50)) {  // you can stick the new i2c addr in here, e.g. begin(0x51);
      Serial.println("Found I2C EEPROM");
      findOld();
      ccnt[0] = ul.ul;
      Serial.print("Last value:"); Serial.println(ccnt[0]);
    } else {
      Serial.println("I2C EEPROM not identified ... check your connections?\r\n");
    }

    gpio_wakeup_enable((GPIO_NUM_13),GPIO_INTR_LOW_LEVEL);   //Low on GPIO_13 allows interrupt
    esp_sleep_enable_gpio_wakeup();    
    esp_sleep_enable_timer_wakeup(10*1e10);  // very long sleep

}

void loop() {
  long t = millis();
  long dt = t - lastLoopUS;
  lastLoopUS = t;
  if (digitalRead(BUTTON_PIN) == LOW)
  { // Check if button has been pressed
    Serial.println("Eco kill button pressed");
    while (digitalRead(BUTTON_PIN) == LOW)
      delay(100); // Wait for button to be released
    Serial.println("Eco kill button released");
    digitalWrite(LED_PIN, HIGH); // Turn on LED
    eco_mode = false;  // so now serial in should work
  }
  
  if(Serial.available()) {
    int inByte = Serial.read();
    if(isVerbose(1)) {
      Serial.print(">"); Serial.println(inByte);
    }  
    if(inByte == 10) {  // end of line
      pcMsg[pcMsgIndex] = 0;   
      pcMsgIndex = 0;
      if(isCmd("version")) {
        Serial.println(VERSION);  
      /*} else if(isCmd("dt")) {     
        Serial.print("Loop ms:");
        Serial.println(dt);
        verbose_flag = 0;*/
      } else if(isCmd("v")) {       
        if(valid) {         
          verbose_flag = pcMsgVal;
        } else {
          Serial.print("Verbose:"); Serial.println(verbose_flag);
          Serial.println("Options:\n1=Comms\n2=EP_status");  
        }
      } else if(isCmd("blank")) { 
        blank_laps = 1;      
        drawMode = DU_Mode;  // something about this mode sets up the Init mode
        display_buffer(img_blank, 0, 0, img_blank_width, img_blank_height);
 
        refresh_next = true;
        x = 0;
        y = 0;
        if(valid) {         
          blank_laps = pcMsgVal;
          if(blank_laps == 0) {
            drawMode = INIT_Mode; // for intial clear
            Serial.println("Display clear");
            display_clear();
            refresh = false;  // refresh was worse than normal print
          }  
        }      
        Serial.print("Blanking laps:");Serial.println(blank_laps);
      } else if(isCmd("edit")) {
        if(valid) {         
          edit_index = pcMsgVal;
        } 
        Serial.print("Edit index:");Serial.println(edit_index);          
      } else if(isCmd("reset")) {
        if(valid) {         
          ccnt[0] = pcMsgVal;
        } else {
          ccnt[0] = 0;
        }
        Serial.print("Count:");Serial.println(ccnt[0]);          
      } else if(isCmd("refresh")) {
        if(valid) {         
          refresh_period = pcMsgVal;
        }
        Serial.print("refresh seconds:");Serial.println(refresh_period);                    
      } else if(isCmd("x")) {
        if(valid) {         
          x_offset[edit_index] = pcMsgVal;
        } 
        Serial.print("x offset:");Serial.println(x_offset[edit_index]);          
      } else if(isCmd("y")) {
        if(valid) {         
          y_offset[edit_index] = pcMsgVal;
        } 
        Serial.print("y offset:");Serial.println(y_offset[edit_index]);          
      }
    } else if(pcMsgIndex < MAX_PC_MSG) {
      pcMsg[pcMsgIndex]= inByte;
      pcMsgIndex++;
    } else {
      Serial.print("#PC REPLY > ");
      Serial.println(MAX_PC_MSG);
      pcMsg[MAX_PC_MSG] = 0;   
      if(isVerbose(1))
        Serial.println(pcMsg);
      pcMsgIndex = 0;  
    } 
  }
  if(blank_laps > 0) {
    drawMode = INIT_Mode;
    x -= x%4; // somehow the image is distorted when not aligned like this
    //y -= y%4;
    //display_buffer(img_blank, 0, 0, display_width, display_height);
    display_buffer(img_blank, x, y, img_blank_width, img_blank_height);
    x += img_blank_width;
    if(new_line) {
      new_line = false;
      x = 0;
      y += img_blank_height;
      Serial.print("Y:"); Serial.print(y); Serial.print(":"); Serial.println(display_height);
      if(new_page) {
        new_page = false;
        y = 0;
        blank_laps -= 1;
      } else if (y + img_blank_height > display_height) {
        y = display_height - img_blank_height;
        new_page = true;
      }
    } else if (x + img_blank_width > display_width) {
      x = display_width - img_blank_width;
      new_line = true;
    }
  } else {
    unsigned long ts = millis()/1000;  // one second
    if(isVerbose(16)) {
      Serial.print("Tm:"); Serial.println(ts); 
    }   
    if((ts != last_save_tm) && (ccnt[0] != lcnt)){
      ul.ul = ccnt[0];
      lcnt = ccnt[0];
      saveNew();
      last_save_tm = ts;
    }  
    if(refresh_period > 0) {
      ts = millis()/refresh_period/1000;
      if(ts != last_reset_tm) {
        refresh_next = true;  // refresh on next loop of writing
        if(isVerbose(2))
          Serial.println("RefreshNext");
        last_reset_tm = ts;
      }
    }  
    if(isVerbose(4)) {
      Serial.print("Line:"); Serial.print(current_line); Serial.print(" Place:"); Serial.print(current_place);
      if(refresh)
        Serial.println(" Refresh");
      else if(refresh_next)
        Serial.println(" Refresh next");
      else
        Serial.println();  
    }  
    if(current_place > text_width ) {
      if(eco_mode) {
        //int sleepMS = Watchdog.sleep(1000);
        //esp_sleep_enable_timer_wakeup(10000000); // 10 sec  - now wakeup using GPIO 13
        if(digitalRead(GPIO_NUM_13) == HIGH) {  // only go to sleep if there is no call to wakeup
          Serial.print("Going to sleep");
           IT8951Sleep();
          //IT8951StandBy();
          esp_light_sleep_start();
          Serial.print("Waking up");
          IT8951SystemRun();
        }  
      //} else {
      //  Serial.println("Loop"); 
      } 
      current_place = 0;
      current_line += 1;
      if(current_line > 2) {
        current_line = 0;
        if(refresh_next) {
          refresh_next = false;
          refresh = true;
           if(isVerbose(2))
           Serial.println("StartRefresh");
        } else if(refresh) {
          refresh = false;  // end refresh of screen
          if(isVerbose(2))
            Serial.println("EndRefresh");
        }
      }  
      sprintf(line_string,"%0*.*f ",text_width,places[current_line],(float)ccnt[current_line]/100000.0);
    }  
    char c = line_string[current_place];
    int ll = current_line*line_length + current_place;
    if((c != last_string[ll]) || refresh) {
      last_string[ll] = c;
      int ic = 11; // blank
      drawMode = standard_mode;
      if((current_place >= (text_width - places[current_line]-2) && current_place < 8) || (c != '0')) {  // only show one 0 before decimal
        if (c == '.')
          ic = 10;
        else if (c >= '0' && c <= '9') {
          ic = c - '0'; 
        }  
      }  
      if(refresh) {
        if(refresh_mode == INIT_Mode) {  // then use number 8 to clear ghosting
          drawMode = INIT_Mode;
          display_buffer(pic[8], x, y, pic_width[ic], pic_height[ic]);
          drawMode = DU_Mode;
        } else {
          drawMode = refresh_mode; 
        }
        //Serial.print("R:");
        //Serial.println(drawMode); 
      }  
      x = x_offset[current_line] + (text_width-current_place)*img_blank_width;  
      x -= x%4; // somehow the image is distorted when not aligned like this
      y = y_offset[current_line];
      //y -= y%4; // somehow the image is distorted when not aligned like this
      display_buffer(pic[ic], x, y, pic_width[ic], pic_height[ic]);
    }
    current_place += 1;
    ccnt[0] += (unsigned long)analogRead(A0)*dt/1000;
    if(ccnt[0] > 999999)
      ccnt[0] = 0;
    ccnt[1] = ccnt[0]*100/75;
    ccnt[2] = ccnt[0]*418/75;
    if(isVerbose(8)) {
      Serial.print("A:"); Serial.print(ccnt[0]);
      Serial.print(" B:"); Serial.print(ccnt[1]); 
      Serial.print(" C:"); Serial.print(ccnt[2]);  
    }
  }
  delay(1);
}
