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
int pump[2] = {8,9};
//Deprecated
//int pump1 = 8; // Digital pin 8
//int pump2 = 9; // Digital pin 9
int pumpController[2] = {3,4}; // Digital pin 3 and 4
//Deprecated
// int pumpController1 = 3; 
// int pumpController2 = 4; // Digital pin 4

// Constants
unsigned long interval10h = (unsigned long) 1000 * 60 * 60 * 10;
unsigned long interval14h = (unsigned long) 1000 * 60 * 60 * 14;
unsigned long intervalPump[2] = {(unsigned long) 1000 * 60 * 20, (unsigned long) 1000 * 60 * 20};
//unsigned long intervalPump1 = (unsigned long) 1000 * 60 * 20; // 20 min delay on pump1
//unsigned long intervalPump2 = (unsigned long) 1000 * 60 * 20; // 20 min delay on pump2
unsigned long runTimePump[2] = {(unsigned long) 4000, (unsigned long) 4000};
//unsigned long runTimePump1 = (unsigned long) 4000; // 4 secs run time
//unsigned long runTimePump2 = (unsigned long) 4000; // 4 secs run time
char const *pumpName[2] = {"Pump 1", "Pump 2"};


// Global variables to keep the state of the system
unsigned long previousMillisGlobalTimer = 0;
unsigned long previousMillisButtonResetTimer = 0;
unsigned long previousMillisPump[2] = {0, 0};
// unsigned long previousMillisPump1 = 0;
// unsigned long previousMillisPump2 = 0;


boolean systemIsActive = true;
boolean timerIsSet = false;
boolean pumpIsActive[2] = {true, true};
// boolean pump1IsActive = true;
// boolean pump2IsActive = true;
boolean pumpIsRunning[2] = {false, false};
// boolean pump1IsRunning = false;
// boolean pump2IsRunning = false;





void u8g_prepare(void) {
  u8g.setFont(u8g_font_04b_03r);
  u8g.setFontRefHeightExtendedText();
  u8g.setDefaultForegroundColor();
  u8g.setFontPosTop();
}


// Return a string displaying the remaining time given a system state and a time interval
void remaining_time_to_string(char* text, unsigned long previousTime, unsigned long interval) {
  long remainingTime = (interval - (millis() - previousTime))/ 1000;

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


void draw_plant_data(int pumpId){
  char const *pumpActivityText;
  unsigned long currentMillis = millis();
  boolean isPaused = false;

  if (pumpIsActive[pumpId])
  {
    if (pumpIsRunning[pumpId]) // Pump is delivering water
    {
      pumpActivityText = "RUN";
    } else if (currentMillis - previousMillisPump[pumpId] < intervalPump[pumpId]) // Pump is paused because it has been used recently
    {
      pumpActivityText = "PAUSED";
      isPaused = true;
    } else { // Pump is on, ready to work but not yet needed
      pumpActivityText = "READY";
    }
  } else {
    pumpActivityText = "OFF";
  }
  // Name
  u8g.drawStr(0,22, pumpName[pumpId]);
  // Status
  u8g.drawStr(35, 22, pumpActivityText);

  // Delay
  if (isPaused) {
    char remainingTime[10];
    remaining_time_to_string(remainingTime, previousMillisPump[pumpId], intervalPump[pumpId]);
    u8g.drawStr(98, 22, remainingTime);
  }
}

// Diplaying the main screen with global system informations
void main_screen(uint8_t a) {

  // First line, System state and timer state
  if(systemIsActive){
    if(timerIsSet) {
      char text1[10];
      remaining_time_to_string(text1, previousMillisGlobalTimer, interval14h);
      u8g.drawStr( 0, 0, "System ON for ");
      u8g.drawStr( 63, 0, text1);
    } else {
      u8g.drawStr( 0, 0, "System ON. No timer set.");
    }
  } else {
    if(timerIsSet) {
      char text2[10];
      remaining_time_to_string(text2, previousMillisGlobalTimer, interval10h);
      u8g.drawStr( 0, 0, "System OFF for ");
      u8g.drawStr( 67, 0, text2);
    } else {
      u8g.drawStr( 0, 0, "System OFF. No timer set.");
    }
  }
  draw_pumps_header();

  // Second line, first plant
  draw_plant_data(0);
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


// Always use this method to start pumps. Ensures that the pump state variable always reflect the real pump state
void start_pump(int pumpId) {
  previousMillisPump[pumpId] = millis();
  pumpIsRunning[pumpId] = true;
  digitalWrite(pump[pumpId], HIGH);
}
// Methods for stopping the pumps
void stop_pump(int pumpId) {
  pumpIsRunning[pumpId] = false;
  digitalWrite(pump[pumpId], LOW);
}


void manage_pump(int pumpId) {
  long currentMillis = millis();
  // We need to be able to stop pump even if system is disabled
  // If we disable the system while a pump is running for example.
  if (pumpIsRunning[pumpId] && (currentMillis - previousMillisPump[pumpId] > runTimePump[pumpId]))
  {
    stop_pump(pumpId);
  }

  pumpIsActive[pumpId] = digitalRead(pumpController[pumpId]);

  // TODO: Replace true with the function to determine if the moisture level is low enough to start pumping
  if (systemIsActive && pumpIsActive[pumpId] && !pumpIsRunning[pumpId] && true 
  && ((currentMillis - previousMillisPump[pumpId] > intervalPump[pumpId]) || previousMillisPump[pumpId] == 0))
  {
    start_pump(pumpId);
  }
  
  
}



void setup(void) {

  pinMode(buttonResetTimer, INPUT);
  pinMode(pump[0], OUTPUT);           
  pinMode(pump[1], OUTPUT);           
  pinMode(pumpController[0], INPUT);           
  pinMode(pumpController[1], INPUT);           


  Serial.begin(115200);
  Serial.println("Initialization done.");
  Serial.println(pump[0]);
  Serial.println(pump[1]);
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

  // Serial.print(digitalRead(pumpController[0]));

  manage_pump(0);
  



  // increase the state
//   draw_state++;
//   if ( draw_state >= 9*8 ) // 9: number of pictures, 8: framerate of a picture
//     draw_state = 0;

}
