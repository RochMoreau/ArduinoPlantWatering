#include "U8glib.h"

// macros from DateTime.h 
/* Useful Constants */
#define SECS_PER_MIN  (60UL)
#define SECS_PER_HOUR (3600UL)
 
/* Useful Macros for getting elapsed time */
#define numberOfSeconds(_time_) (_time_ % SECS_PER_MIN)  
#define numberOfMinutes(_time_) ((_time_ / SECS_PER_MIN) % SECS_PER_MIN) 
#define numberOfHours(_time_) (( _time_% SECS_PER_DAY) / SECS_PER_HOUR)

// INPUTS AND OUTPUTS
U8GLIB_SH1106_128X64 u8g(U8G_I2C_OPT_NONE); // I2C / TWI 
int buttonResetTimer = 2; // digital Pin 2

// Constants
unsigned long interval10h = (unsigned long) 1000 * 60 * 60 * 10;
unsigned long interval14h = (unsigned long) 1000 * 60 * 60 * 14;

// Global variables to keep the state of the system
unsigned long previousMillisGlobalTimer = 0;
unsigned long previousMillisButtonResetTimer = 0;

boolean systemIsActive = true;
boolean timerIsSet = false;




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
    sprintf(text,"System ON for %d:%d:%d",hours,minutes,seconds);
  } else {
    sprintf(text,"System OFF for %d:%d:%d",hours,minutes,seconds);
  }
}


// Diplaying the main screen with global system informations
void main_screen(uint8_t a) {

  // First line, System state and timer state
  if(systemIsActive){
    if(timerIsSet) {
      char text1[35];
      remaining_time_to_string(text1, interval14h);
      u8g.drawStr( 0, 0, text1);
    } else {
      u8g.drawStr( 0, 0, "System ON. No timer set.");
    }
  } else {
    if(timerIsSet) {
      char text2[35];
      remaining_time_to_string(text2, interval10h);
      u8g.drawStr( 0, 0, text2);
    } else {
      u8g.drawStr( 0, 0, "System OFF. No timer set.");
    }
  }
  u8g.drawLine(0,10,128, 10);

  // Second line, first plant
  u8g.drawStr(0,12, "Plant1");

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


bool toggle_led(void *) {
  digitalWrite(8, !digitalRead(8)); // toggle the LED
  return true; // keep timer active? true
}


void setup(void) {

  pinMode(8, OUTPUT);           
  pinMode(buttonResetTimer, INPUT);



  // Serial.begin(115200);
  // Serial.println("Initialization done.");
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
  if( digitalRead(buttonResetTimer) && currentMillis - previousMillisButtonResetTimer > 300 ) { 
    previousMillisButtonResetTimer = currentMillis;
    previousMillisGlobalTimer = currentMillis;
    systemIsActive = timerIsSet == systemIsActive;
    timerIsSet = !timerIsSet;
  }





  // increase the state
//   draw_state++;
//   if ( draw_state >= 9*8 ) // 9: number of pictures, 8: framerate of a picture
//     draw_state = 0;

}
