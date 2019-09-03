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
int buttonResetTimer = 2; // Digital pin 2
int pump[2] = {8,9}; // Digital pins 8 and 9
int pumpController[2] = {3,4}; // Digital pins 3 and 4
int moistureController[2] = {6, 7}; // Digital pins 6 and 7 to power the moisture sensors. I don't want to connect
// them to 5V because being always ON will corrode the sensors.
int moistureSensor[2] = {A0, A1};


// Constants
unsigned long interval10h = (unsigned long) 1000 * 60 * 60 * 10;
unsigned long interval14h = (unsigned long) 1000 * 60 * 60 * 14;
unsigned long intervalPump[2] = {(unsigned long) 1000 * 60 * 20, (unsigned long) 1000 * 60 * 20};
unsigned long runTimePump[2] = {(unsigned long) 4000, (unsigned long) 4000};
char const *pumpName[2] = {"Pump 1", "Pump 2"};
const int lowestAcceptableLevel = 10; // The pump should not be triggered below this level, probable sign of error in reading
const int highestAcceptableLevel = 85; // The pump should not be triggered over this level also.
unsigned long intervalMoisture[2] = {(unsigned long) 1000 * 60 * 5, (unsigned long) 1000 * 60 * 5}; // 1 read every 5 mins
const int moistureThreshold[2] = {40, 30}; // Percentage below which the pump should be activated


// Global variables to keep the state of the system
unsigned long previousMillisGlobalTimer = 0;
unsigned long previousMillisButtonResetTimer = 0;
unsigned long previousMillisPump[2] = {0, 0};
int moistureLevel[2] = {0, 0};
unsigned long previousMillisMoisture[2] = {0, 0};


boolean systemIsActive = true;
boolean timerIsSet = false;
boolean manualMode = false;
boolean pumpIsActive[2] = {true, true};
boolean pumpIsRunning[2] = {false, false};





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
  u8g.drawStr(102, 12, "Wait");
  u8g.drawLine(0,20,128, 20);
  // Vertical lines
  u8g.drawLine(31,10,31, 40);
  u8g.drawLine(66,10,66, 40);
  u8g.drawLine(93,10,93, 40);

}


void draw_plant_data(int pumpId){
  char const *pumpActivityText;
  unsigned long currentMillis = millis();
  boolean isPaused = false;
  boolean isReady = false;

  int verticalPos = 22 + pumpId * 10;

  if (pumpIsActive[pumpId])
  {
    if (pumpIsRunning[pumpId]) // Pump is delivering water
    {
      pumpActivityText = "RUN";
    } else if (currentMillis - previousMillisPump[pumpId] < intervalPump[pumpId] && previousMillisPump[pumpId] != 0) // Pump is paused because it has been used recently
    {
      pumpActivityText = "PAUSED";
      isPaused = true;
    } else { // Pump is on, ready to work (not blocked by any delay) but not yet needed
      pumpActivityText = "READY";
      isReady = true;
    }
  } else {
    pumpActivityText = "OFF";
  }
  // Name
  u8g.drawStr(0, verticalPos, pumpName[pumpId]);
  // Status
  u8g.drawStr(35, verticalPos, pumpActivityText);
  // Moisture
  char moistureLevelText[6];
  sprintf(moistureLevelText, "%d%%", moistureLevel[pumpId]);
  u8g.drawStr(73, verticalPos, moistureLevelText);
  // Delay
  if (!systemIsActive)
  {
    u8g.drawStr(98, verticalPos, "Sys ON");
  } else if (isPaused) {
    char remainingTime[10];
    remaining_time_to_string(remainingTime, previousMillisPump[pumpId], intervalPump[pumpId]);
    u8g.drawStr(98, verticalPos, remainingTime);
  } else if (isReady)
  {
    char waitThresholdText[10];
    sprintf(waitThresholdText, "<%d%%", moistureThreshold[pumpId]);
    u8g.drawStr(98, verticalPos, waitThresholdText);
  }
  

  u8g.drawLine(0, verticalPos+8, 128, verticalPos+8);
}

void draw_excluded_values() {
  int verticalPos = 42;
  char workingRangeText[35];
  sprintf(workingRangeText, "Working range: %d%% - %d%%", lowestAcceptableLevel, highestAcceptableLevel);
  u8g.drawStr(0, verticalPos, workingRangeText);
}

void draw_sensor_refresh() {
  int verticalPos = 50;
  long currentMillis = millis();
  u8g.drawStr(0, verticalPos+1, "Sensor refresh");
  for (int i = 0; i < 2; i++)
  {
    if (pumpIsActive[i] && systemIsActive)
    {
      char sensorRefreshTime[10];
      remaining_time_to_string(sensorRefreshTime, previousMillisMoisture[i], intervalMoisture[i]);
      u8g.drawStr(i*40, verticalPos+8, sensorRefreshTime);
    } else {
      u8g.drawStr(i*40, verticalPos+8, "OFF");
    }
  }

  u8g.drawLine(0, verticalPos, 128, verticalPos);
  u8g.drawLine(66,verticalPos,66, 64);
  
}

// Diplaying the main screen with global system informations
void main_screen() {

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

  // Pumps and sensor data
  draw_plant_data(0);
  draw_plant_data(1);

  draw_excluded_values();
  draw_sensor_refresh();

  u8g.drawStr(72, 55, "Roch Moreau");
}

void manual_mode_screen() {
  u8g.setFontPosCenter();
  u8g.drawStr( 64, 10, "MANUAL MODE");
  for (int pumpId = 0; pumpId < 2; pumpId++)
  {
    u8g.drawStr( 32+64*pumpId, 22, pumpName);
    if (pumpIsActive[pumpId])
    {
      u8g.drawStr( 32+64*pumpId, 42, "RUNNING");
    } else {
      u8g.drawStr( 32+64*pumpId, 42, "OFF");
    }
    
  }
  
}

void draw(void) {
  u8g_prepare();
  switch(manualMode) {
    case false: main_screen(); break;
    case true: manual_mode_screen(); break;
  }
}




// Control Section

// Only read in periodic interval defined in global constants to avoid corroding sensors
// The sensors are deactivated when a pump is disabled and when the system is disabled
// Once the system and the pump are available again, a reading is done instantly.
void read_moisture_level(int pumpId) {
  long currentMillis = millis();
  if ( systemIsActive && pumpIsActive[pumpId] &&
    (currentMillis - previousMillisMoisture[pumpId] > intervalMoisture[pumpId]) || previousMillisMoisture[pumpId] == 0)
  {
    previousMillisMoisture[pumpId] = currentMillis;
    digitalWrite(moistureController[pumpId], HIGH);
    delay(10);
    int rawReading = analogRead(moistureSensor[pumpId]);
    // Sensors gives 1005 in air and 160 fully submerged in water, we want data from 0% to 100%
    moistureLevel[pumpId] = map(rawReading, 1005, 160, 0, 100); 
    digitalWrite(moistureController[pumpId], LOW);
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
// Same method as before but in manual mode. Does not start a timer
void start_pump_manual(int pumpId) {
  pumpIsRunning[pumpId] = true;
  digitalWrite(pump[pumpId], HIGH);
}


void manage_pump(int pumpId) {
  long currentMillis = millis();
  // We need to be able to stop pump even if system is disabled
  // If we disable the system while a pump is running for example.
  if ((pumpIsRunning[pumpId] && (currentMillis - previousMillisPump[pumpId] > runTimePump[pumpId])) //stop pump if it has completed its runtime
      || !pumpIsActive[pumpId]) // Immediately stop pump if pump switch is OFF
  {
    stop_pump(pumpId);
  }

  if (systemIsActive && pumpIsActive[pumpId] && !pumpIsRunning[pumpId]
  && moistureLevel[pumpId] < moistureThreshold[pumpId] && moistureLevel[pumpId] > lowestAcceptableLevel 
  && moistureLevel[pumpId] < highestAcceptableLevel
  && ((currentMillis - previousMillisPump[pumpId] > intervalPump[pumpId]) || previousMillisPump[pumpId] == 0))
  {
    start_pump(pumpId);
  }
}



void setup(void) {

  pinMode(buttonResetTimer, INPUT);
  for (int i = 0; i < 2; i++)
  {
    pinMode(pump[i], OUTPUT);           
    pinMode(pumpController[i], INPUT);           
    pinMode(moistureController[i], OUTPUT);
    pinMode(moistureSensor[i], INPUT);

    digitalWrite(moistureController[i], LOW);
    digitalWrite(pump[i], LOW);
  }
  
  


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
  // Delay of 600 ms between 2 button press
  // Flow: 1: System ON, no timer / 2: System OFF, timer set / 3: System OFF, no timer 
  // (4): (if pumps are both disabled) man / 5: System ON, timer set
  if( digitalRead(buttonResetTimer) && currentMillis - previousMillisButtonResetTimer > 600 ) { 
    previousMillisButtonResetTimer = currentMillis;
    previousMillisGlobalTimer = currentMillis;

    if (!systemIsActive && !timerIsSet && !manualMode && !pumpIsActive[0] && !pumpIsActive[1])
    {
      manualMode = true;
    } else {
      manualMode = false;
      systemIsActive = timerIsSet == systemIsActive;
      timerIsSet = !timerIsSet;
    }
    
  }


  for (int pumpId = 0; pumpId < 2; pumpId++)
  {
    pumpIsActive[pumpId] = digitalRead(pumpController[pumpId]);
    
    if (manualMode)
    {
      if(pumpIsActive[pumpId] && !pumpIsRunning[pumpId])
      {
        start_pump_manual(pumpId);
      } else if (!pumpIsActive[pumpId] && pumpIsRunning[pumpId])
      {
        stop_pump(pumpId);
      }

    } else {
      // Serial.print(digitalRead(pumpController[0]));
      read_moisture_level(pumpId);
      manage_pump(pumpId);
    }
  }


}
