#include "U8glib.h"

// macros from DateTime.h 
/* Useful Constants */
#define SECS_PER_MIN  (60UL)
#define SECS_PER_HOUR (3600UL)
#define SECS_PER_DAY  (SECS_PER_HOUR * 24L)
 
/* Useful Macros for getting elapsed time */
#define numberOfSeconds(_time_) (_time_ % SECS_PER_MIN)  
#define numberOfMinutes(_time_) ((_time_ / SECS_PER_MIN) % SECS_PER_MIN) 
#define numberOfHours(_time_) (( _time_% SECS_PER_DAY) / SECS_PER_HOUR)

// INPUTS AND OUTPUTS
U8GLIB_SH1106_128X64 u8g(U8G_I2C_OPT_NONE); // I2C / TWI 
int buttonResetTimer = 2; // digital Pin 2
int pump1 = 8; // Digital pin 8
int pump2 = 9; // Digital pin 9
int pumpController1 = 3; // Digital pin 3
int pumpController2 = 4; // Digital pin 4

// Constants
unsigned long interval10h = (unsigned long) 1000 * 60 * 60 * 10;
unsigned long interval14h = (unsigned long) 1000 * 60 * 60 * 14;
unsigned long intervalPump1 = (unsigned long) 1000 * 60 * 20; // 20 min delay on pump1
unsigned long intervalPump2 = (unsigned long) 1000 * 60 * 20; // 20 min delay on pump1

// Global variables to keep the state of the system
unsigned long previousMillisGlobalTimer = 0;
unsigned long previousMillisButtonResetTimer = 0;
unsigned long previousMillisPump1 = 0;
unsigned long previousMillisPump2 = 0;


boolean systemIsActive = true;
boolean timerIsSet = false;
boolean pump1IsActive = true;
boolean pump2IsActive = true;
boolean pump1IsRunning = false;
boolean pump2IsRunning = false;





void u8g_prepare(void) {
  u8g.setFont(u8g_font_04b_03r);
  u8g.setFontRefHeightExtendedText();
  u8g.setDefaultForegroundColor();
  u8g.setFontPosTop();
}


// Return a string displaying the remaining time given a system state and a time interval
void remaining_time_to_string(char* text, unsigned long interval) {
  long remainingTime = (interval - (millis() - previousMillisGlobalTimer))/ 1000;

  int hours = numberOfHours(remainingTime);
  int minutes = numberOfMinutes(remainingTime);
  int seconds = numberOfSeconds(remainingTime);

  if (systemIsActive)
  {
    sprintf(text,"%d:%d:%d",hours,minutes,seconds);
  } else {
    sprintf(text,"%d:%d:%d",hours,minutes,seconds);
  }
}

void draw_pumps_header() {
  u8g.drawLine(0,10,128, 10);
  u8g.drawStr(4,12, "Name");
  u8g.drawStr(35,12, "Status");
  u8g.drawStr(70, 12, "Level");
  u8g.drawStr(100, 12, "Delay");
  u8g.drawLine(0,20,128, 20);
  // Vertical lines
  u8g.drawLine(31,10,31, 64);
  u8g.drawLine(66,10,66, 64);
  u8g.drawLine(93,10,93, 64);

}

void draw_plant_data(int position, char const *name, boolean pumpIsActive, boolean pumpIsRunning, unsigned long previousMillisPump, unsigned long interval) {
  char const *pumpActivityText;
  unsigned long currentMillis = millis();

  if (pumpIsActive)
  {
    if (pumpIsRunning) // Pump is delivering water
    {
      pumpActivityText = "RUNNING";
    } else if (currentMillis - previousMillisPump < interval) // Pump is paused because it has been used recently
    {
      pumpActivityText = "PAUSED";
    } else { // Pump is on, ready to work but not yet needed
      pumpActivityText = "READY";
    }
  } else {
    pumpActivityText = "OFF";
  }
  u8g.drawStr(0,22, name);
  u8g.drawStr(35, 22, pumpActivityText);
}

// Diplaying the main screen with global system informations
void main_screen(uint8_t a) {

  // First line, System state and timer state
  if(systemIsActive){
    if(timerIsSet) {
      char text1[35];
      remaining_time_to_string(text1, interval14h);
      u8g.drawStr( 0, 0, "System ON for ");
      u8g.drawStr( 63, 0, text1);
    } else {
      u8g.drawStr( 0, 0, "System ON. No timer set.");
    }
  } else {
    if(timerIsSet) {
      char text2[35];
      remaining_time_to_string(text2, interval10h);
      u8g.drawStr( 0, 0, "System OFF for ");
      u8g.drawStr( 67, 0, text2);
    } else {
      u8g.drawStr( 0, 0, "System OFF. No timer set.");
    }
  }
  draw_pumps_header();

  // Second line, first plant
  draw_plant_data(1, "Pump 1", pump1IsActive, pump1IsRunning, previousMillisPump1, intervalPump1);
  // u8g.drawStr(0,12, plant1Text);

}


uint8_t draw_state = 3; // goes from 0 to 71
          // draw_state >> 3 goes from 0 to 8, each contains 8 draw_state values
          // draw_state&7  goes from 0 to 7 and repeat itself matching draw_state pace

void draw(void) {
  u8g_prepare();
  switch(draw_state >> 3) {
    case 0: main_screen(draw_state&7); break;
  }
}






void manage_pump_1() {

}



void setup(void) {

  pinMode(buttonResetTimer, INPUT);
  pinMode(pump1, OUTPUT);           
  pinMode(pump2, OUTPUT);           
  pinMode(pumpController1, INPUT);           
  pinMode(pumpController2, INPUT);           


  Serial.begin(115200);
  Serial.println("Initialization done.");
}

void loop(void) {
  
  // picture loop for the display
  u8g.firstPage();  
  do {
    draw();
  } while( u8g.nextPage() );

  // Disable the system after 14h of ON time
  // Enable the system after 10h of OFF time
  unsigned long currentMillis = millis();
  if (  timerIsSet &&
       (((currentMillis - previousMillisGlobalTimer > interval14h) && systemIsActive) ||
        ((currentMillis - previousMillisGlobalTimer > interval10h) && !systemIsActive) )) {
    previousMillisGlobalTimer = currentMillis;
    systemIsActive = !systemIsActive;
  }

  // Button control for timer setting
  // Set the OFF time at now, launch a new cycle of ON/OFF time every 14/10H
  // Delay of 300 ms between 2 button press
  if( digitalRead(buttonResetTimer) && currentMillis - previousMillisButtonResetTimer > 600 ) { 
    previousMillisButtonResetTimer = currentMillis;
    previousMillisGlobalTimer = currentMillis;
    systemIsActive = timerIsSet == systemIsActive;
    timerIsSet = !timerIsSet;
  }

  if (systemIsActive)
  {
    manage_pump_1();
    // manage_pump_2();
    pump1IsActive = digitalRead(pumpController1);
    pump2IsActive = digitalRead(pumpController2);

    
  }
  



  // increase the state
//   draw_state++;
//   if ( draw_state >= 9*8 ) // 9: number of pictures, 8: framerate of a picture
//     draw_state = 0;

}
