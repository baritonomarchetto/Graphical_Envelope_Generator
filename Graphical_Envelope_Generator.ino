 /*Graphical Quad Envelope Generator
 * 4X envelopes generator for eurorack synthesizers
 * Hardware:
 * Arduino nano 328p
 * ST7735/89 SPI display with touchscreen;
 * MCP4728 I2C 12-bit DAC
 *
 * by barito (last update dec 2024)
 */

#include <TS_Display.h>             // Touch to display mapping functions lib (Ted Toal)
#include <Adafruit_GFX.h>           // graphic library
#include <Adafruit_ST7789.h>        // Hardware-specific library for ST7789
#include <XPT2046_Touchscreen_TT.h> // XPT2046 touch library (Paul Stoffregen & Ted Toal)
#include <SPI.h>                    // SPI communication library
#include <Wire.h>                   // I2C communication library
#include <mcp4728.h>                // DAC MCP4728 library (Benoit Shillings https://github.com/BenoitSchillings/mcp4728)

//RGB565 format, inverted
#define BLACK 0xFFFF
#define WHITE 0x0000
#define VIOLET 0x15E2
#define GREEN 0xB8B7
#define BLUE 0xBD22
#define YELLOW 0x18B7
#define RED 0x16BE

// Display pins
#define TFT_CS_PIN 10
#define TFT_DC_PIN 9
#define TFT_RST_PIN  8  // to Vcc if not used
//#define TFT_MOSI_PIN 11 //hardware SPI, no need to define
//#define TFT_MISO_PIN 12
//#define TFT_CLK_PIN  13

// Touchscreen XPT2046 pins
#define TOUCH_CS_PIN 7

// DAC MCP4728 pins
#define LDAC_PIN 2 // DAC latch pin
//#define SDA A4
//#define SCL A5

const int OUT_NUM = 4;
const int POT_NUM = 3;

int CH_COLOR[OUT_NUM] = {GREEN, YELLOW, BLUE, RED};

const int GATE_PIN[OUT_NUM] = {5, 6, A1, A0};
bool gateState[OUT_NUM];

// Buttons pins
const int BTN_PIN[OUT_NUM] = {4, 3, 0, A2};
bool btnState[OUT_NUM];
unsigned long btnDb[OUT_NUM];

//Potentiometers
const int POT_PIN[POT_NUM] = {A3, A6, A7};
int potVal[OUT_NUM][POT_NUM];
int potLockVal[OUT_NUM][POT_NUM]; //pot hold value
bool potEnable[OUT_NUM][POT_NUM]; //pot status (enabled/disabled)

// Display and touchscreen objects
Adafruit_ST7789* tft; // Pointer to display object.
XPT2046_Touchscreen* ts; // Pointer to touchscreen object.
TS_Display* ts_display;// Pointer to touchscreen-display object.

//DAC object
mcp4728 dac = mcp4728(0); // initiate mcp4728 object, Device ID = 0 (default)

// envelope draw profiles varibles
const int MAX_POINTS = 240;  // maximun data EG_form lenght for storage
int pointIndex = -1;  // index to track the running EG
byte EG_Y[OUT_NUM][MAX_POINTS] = {{42, 42, 42, 42, 48, 50, 58, 63, 64, 68, 72, 75, 79, 85, 88, 90, 94, 96, 99, 102, 105, 109, 110, 114, 116, 120, 122, 126, 129, 133, 133, 137, 139, 142, 143, 147, 149, 149, 152, 154, 156, 158, 160, 162, 164, 164, 167, 169, 169, 171, 172, 174, 174, 176, 177, 177, 179, 179, 180, 180, 181, 183, 184, 185, 184, 185, 185, 187, 187, 188, 190, 190, 190, 191, 193, 193, 193, 194, 194, 195, 194, 194, 196, 196, 196, 197, 197, 197, 198, 197, 198, 198, 198, 198, 199, 198, 199, 199, 198, 198, 197, 197, 197, 197, 195, 195, 195, 195, 194, 194, 192, 192, 192, 190, 189, 188, 187, 187, 185, 184, 183, 182, 181, 180, 180, 178, 177, 175, 174, 172, 170, 169, 168, 166, 165, 162, 160, 158, 156, 154, 152, 151, 148, 147, 144, 142, 140, 137, 135, 133, 130, 128, 126, 125, 123, 120, 117, 116, 115, 114, 111, 108, 107, 106, 104, 101, 99, 97, 95, 92, 90, 88, 87, 83, 81, 79, 77, 75, 72, 70, 69, 66, 64, 60, 58, 56, 54, 49, 46, 45, 43, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                                  {42, 42, 42, 42, 48, 50, 58, 63, 64, 68, 72, 75, 79, 85, 88, 90, 94, 96, 99, 102, 105, 109, 110, 114, 116, 120, 122, 126, 129, 133, 133, 137, 139, 142, 143, 147, 149, 149, 152, 154, 156, 158, 160, 162, 164, 164, 167, 169, 169, 171, 172, 174, 174, 176, 177, 177, 179, 179, 180, 180, 181, 183, 184, 185, 184, 185, 185, 187, 187, 188, 190, 190, 190, 191, 193, 193, 193, 194, 194, 195, 194, 194, 196, 196, 196, 197, 197, 197, 198, 197, 198, 198, 198, 198, 199, 198, 199, 199, 198, 198, 197, 197, 197, 197, 195, 195, 195, 195, 194, 194, 192, 192, 192, 190, 189, 188, 187, 187, 185, 184, 183, 182, 181, 180, 180, 178, 177, 175, 174, 172, 170, 169, 168, 166, 165, 162, 160, 158, 156, 154, 152, 151, 148, 147, 144, 142, 140, 137, 135, 133, 130, 128, 126, 125, 123, 120, 117, 116, 115, 114, 111, 108, 107, 106, 104, 101, 99, 97, 95, 92, 90, 88, 87, 83, 81, 79, 77, 75, 72, 70, 69, 66, 64, 60, 58, 56, 54, 49, 46, 45, 43, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                                  {42, 42, 42, 42, 48, 50, 58, 63, 64, 68, 72, 75, 79, 85, 88, 90, 94, 96, 99, 102, 105, 109, 110, 114, 116, 120, 122, 126, 129, 133, 133, 137, 139, 142, 143, 147, 149, 149, 152, 154, 156, 158, 160, 162, 164, 164, 167, 169, 169, 171, 172, 174, 174, 176, 177, 177, 179, 179, 180, 180, 181, 183, 184, 185, 184, 185, 185, 187, 187, 188, 190, 190, 190, 191, 193, 193, 193, 194, 194, 195, 194, 194, 196, 196, 196, 197, 197, 197, 198, 197, 198, 198, 198, 198, 199, 198, 199, 199, 198, 198, 197, 197, 197, 197, 195, 195, 195, 195, 194, 194, 192, 192, 192, 190, 189, 188, 187, 187, 185, 184, 183, 182, 181, 180, 180, 178, 177, 175, 174, 172, 170, 169, 168, 166, 165, 162, 160, 158, 156, 154, 152, 151, 148, 147, 144, 142, 140, 137, 135, 133, 130, 128, 126, 125, 123, 120, 117, 116, 115, 114, 111, 108, 107, 106, 104, 101, 99, 97, 95, 92, 90, 88, 87, 83, 81, 79, 77, 75, 72, 70, 69, 66, 64, 60, 58, 56, 54, 49, 46, 45, 43, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                                  {42, 42, 42, 42, 48, 50, 58, 63, 64, 68, 72, 75, 79, 85, 88, 90, 94, 96, 99, 102, 105, 109, 110, 114, 116, 120, 122, 126, 129, 133, 133, 137, 139, 142, 143, 147, 149, 149, 152, 154, 156, 158, 160, 162, 164, 164, 167, 169, 169, 171, 172, 174, 174, 176, 177, 177, 179, 179, 180, 180, 181, 183, 184, 185, 184, 185, 185, 187, 187, 188, 190, 190, 190, 191, 193, 193, 193, 194, 194, 195, 194, 194, 196, 196, 196, 197, 197, 197, 198, 197, 198, 198, 198, 198, 199, 198, 199, 199, 198, 198, 197, 197, 197, 197, 195, 195, 195, 195, 194, 194, 192, 192, 192, 190, 189, 188, 187, 187, 185, 184, 183, 182, 181, 180, 180, 178, 177, 175, 174, 172, 170, 169, 168, 166, 165, 162, 160, 158, 156, 154, 152, 151, 148, 147, 144, 142, 140, 137, 135, 133, 130, 128, 126, 125, 123, 120, 117, 116, 115, 114, 111, 108, 107, 106, 104, 101, 99, 97, 95, 92, 90, 88, 87, 83, 81, 79, 77, 75, 72, 70, 69, 66, 64, 60, 58, 56, 54, 49, 46, 45, 43, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
                                };

int MAXindex[OUT_NUM] = {191, 191, 191, 191}; // actual latest value stored index
int firstX[OUT_NUM] = {1024, 1024, 1024, 1024}; //first point X position
int relIndex[OUT_NUM] = {112, 112, 112, 112};

byte chNum = 0; //EG channel in use. We start from "A" channel

// DAC out EG_form
const int offsetY = 300;
int iDAC[OUT_NUM];//instant DAC value
byte EG_count[OUT_NUM] = {0, 0, 0, 0};
unsigned long EG_runTime[OUT_NUM];
int del_time[OUT_NUM][POT_NUM]; //delay execution time between points
int res[OUT_NUM][POT_NUM]; //execution resolution (determines how many points of the curve are missed)
int pre_DAC[OUT_NUM];

//Flags
bool isDrawing[OUT_NUM] = {0, 0, 0, 0};   // drawing flag
bool EG_Stored[OUT_NUM] = {0, 0, 0, 0};  // EG_form flag

//display visuals and sectors consts
const int relDrawX = 200;
int relPosX = map(relDrawX, 0, 320, 0, 4095);
int resIndex = 13;//touch data acquisition space interval.

void setup() {
  // Display init
  tft = new Adafruit_ST7789(TFT_CS_PIN, TFT_DC_PIN, TFT_RST_PIN);
  tft->init(240, 320);   // standard init ST7789, portrait 240x320
  tft->setRotation(1);  // Screen orientation (0-3)
  tft->fillScreen(BLACK);  // clean display
  //tft->invertDisplay(true);
  //tft->setSPISpeed(40000000);

  // Touch screen init
  ts = new XPT2046_Touchscreen(TOUCH_CS_PIN); // no interrupt
  //ts = new XPT2046_Touchscreen(TOUCH_CS_PIN, TOUCH_IRQ_PIN); // interrupt enabled polling
  ts->begin();
  ts->setRotation(tft->getRotation());  // touchscreen orientation, same as TFT
  ts->setThresholds(Z_THRESHOLD); // Change the pressure threshold

  // Allocate and initialize the touchscreen-display object.
  ts_display = new TS_Display();
  ts_display->begin(ts, tft);

  pinMode(LDAC_PIN, OUTPUT);
  digitalWrite(LDAC_PIN, LOW);

  for (int a = 0; a < OUT_NUM; a++){
    pinMode(BTN_PIN[a], INPUT_PULLUP);
    btnState[a] = digitalRead(BTN_PIN[a]);
    pinMode(GATE_PIN[a], INPUT); //V-TRIG.
    gateState[a] = digitalRead(GATE_PIN[a]);
    EG_runTime[a] = millis();
    for (int p = 0; p < POT_NUM; p++){
      pinMode(POT_PIN[p], INPUT);
      del_time[a][p] = 0;
      res[a][p] = 1;
    }
  }
  potEnable[0][0] = true;//channel A pot1 ENABLED
  potEnable[0][1] = true;//channel A pot2 ENABLED
  potEnable[0][2] = true;//channel A pot3 ENABLED

  dac.begin();  // initialize i2c interface
  // Inizializzazione del DAC
  dac.setPowerDown(0, 0, 0, 0); // set Power-Down ( 0 = Normal , 1-3 = shut down most channel circuit, no voltage out) (1 = 1K ohms to GND, 2 = 100K ohms to GND, 3 = 500K ohms to GND)
  dac.setVref(0, 0, 0, 0); // set to use internal voltage reference (2.048V) ( 0 = external, 1 = internal )
  //dac.setGain(0, 0, 0, 0); // set the gain of internal voltage reference ( 0 = gain x1, 1 = gain x2 )
  dac.analogWrite(0, 0, 0, 0); //set all outs to zero

  for (int a = 1; a < OUT_NUM; a++){
    EG_Stored[a] = true;
    EG_RECALL(a);
  }
  EG_Stored[0] = true;
  EG_RECALL(0);
  //TFT_DRAW_LINES();
  //Serial.begin(9600);
}

void loop() {
  Switches();
  PotRead();
  GatesChange();
  TouchDraw(); 
  EG_Out();
}

void TouchDraw(){
  boolean isTouched = ts->touched();
  if (isTouched){ //touchscreen has been touched...
    TS_Point p = ts->getPoint();  // get touch coordinates
    int16_t x, y;
    ts_display->mapTStoDisplay(p.x, p.y, &x, &y); // convert touch coordinates to display coordinates
    p.x = 4095 - p.x; //rectify touch coordinates (both axis coordinates are inverted in landscape mode)
    p.y = 4095 - p.y; //rectify touch coordinates (both axis coordinates are inverted in landscape mode)
    // new drawing start, init
    if(isDrawing[chNum] == false && p.x < relPosX){//this makes not possible to draw a release-only envelope
      isDrawing[chNum] = true;  // drawing has started
      tft->fillScreen(BLACK);  // clean display
      TFT_DRAW_LINES();
      pointIndex = -1;  //point index init
      MAXindex[chNum] = -1; //max value init
      relIndex[chNum] = -1; //release index init
      EG_Stored[chNum] = false;  // memory state reset
      firstX[chNum] = p.x;//new drawing start
    }
    if(pointIndex < MAX_POINTS){//check for left space in the array
      if(p.x > firstX[chNum] + (pointIndex + 1) * resIndex){//if the point follows the one before
        pointIndex++;
        tft->fillCircle(x, y, 2, CH_COLOR[chNum]);//EG dots
        EG_Y[chNum][pointIndex] = 240 - y;  // save the (rectified) Y value in the EG_ array        
        
        //set release indexes
        if (p.x > relPosX - 80 && relIndex[chNum] == -1){
          relIndex[chNum] = pointIndex;
          tft->fillCircle(x, y, 10, VIOLET);//draw release dot
        }
        EG_Stored[chNum] = true;  // vaweform progress recorded
        dac.analogWrite(chNum, (EG_Y[chNum][pointIndex]<<4) - offsetY);
      }
    }
  }
  else { //no touch, draw ended
    if (isDrawing[chNum]){ //no touch, draw ended
      isDrawing[chNum] = false;
      MAXindex[chNum] = pointIndex;
      isTouched = false;
      TFT_DRAW_LINES();
    }
  }
}

void EG_Out(){
  //this runs continuously
  for(int a = 0; a < OUT_NUM; a++){
    if(isDrawing[a] == false){//if the EG is under draw, I want the routine defined in TouchDraw()
      if(relIndex[a] > 0){//a release has been drawn, ADSR mode
        if (gateState[a] == HIGH || btnState[a] == LOW){ //gate open or button pressed - ATTACK/DECAY/SUSTAIN PHASE
          if(EG_count[a] + res[a][1] < relIndex[a]){ //ATTACK-DECAY STAGE. The sum avoids going into release region
            EG_Flush(a, del_time[a][1], res[a][1]); //POT 2
          }
        }
        else if (gateState[a] == LOW || btnState[a] == HIGH){ //gate closed or button not pressed - RELEASE STAGE
          if (EG_count[a] + res[a][2]<= MAXindex[a] && EG_count[a] >= relIndex[a]){
            EG_Flush(a, del_time[a][2], res[a][2]); //POT 3
          }
        }
      }
      else{ //no release: LFO MODE!!
        if(EG_count[a] + res[a][1] < MAXindex[a]){
          EG_Flush(a, del_time[a][1], res[a][1]); //POT 2
        }
        else {
          EG_count[a] = 0;
        }
      }
    }
  }
}

void EG_Flush(int ch_fl, int i_del, int i_res){
  if(millis() - EG_runTime[ch_fl] > i_del){
    EG_runTime[ch_fl] = millis();
    EG_count[ch_fl] = EG_count[ch_fl] + i_res;
    if(EG_count[ch_fl] > MAXindex[ch_fl]){
      EG_count[ch_fl] = MAXindex[ch_fl];
    }
    iDAC[ch_fl] = (EG_Y[ch_fl][EG_count[ch_fl]]<<4) - offsetY + (potLockVal[ch_fl][0]<<3); //POT 1
    if (iDAC[ch_fl] <0){
      iDAC[ch_fl] = 0;
    }
    //else if(iDAC[ch_fl] > 4095){iDAC[ch_fl] = 4095;}//not possible
    dac.analogWrite(ch_fl, iDAC[ch_fl]);
  }
}

void Switches(){
  for(int a = 0; a < OUT_NUM; a++){
    if(digitalRead(BTN_PIN[a]) != btnState[a] && millis()-btnDb[a] > 40){ //on state change...
      btnDb[a] = millis();
      btnState[a] = !btnState[a];
      if (btnState[a] == LOW){ //button pressed
        if(a == chNum){ //same button, trig the envelope!
          EG_count[a] = 0;
        }
        else{ //new button
          for(int b = 0; b < OUT_NUM; b++){
            if(btnState[b] == LOW){//there's another button kept pressed
              COPY_ENV(b, a);//copy the envelope
            }
          }
          //new channel ...
          potEnable[a][0] = false;//disable all pots for all channels
          potEnable[a][1] = false;
          potEnable[a][2] = false;
          chNum = a;//now, change channel
          EG_RECALL(a); //recall the envelope
        }
      }
      else{//button released
        if(a == chNum){ //same button, go to release phase
          EG_count[a] = relIndex[a];//RELEASE PHASE STARTS
        }
      }
    }
  }
}

void GatesChange(){
  for(int a = 0; a < OUT_NUM; a++){
    if (digitalRead(GATE_PIN[a]) != gateState[a]) { // GATE state change...
      gateState[a] = !gateState[a];
      pre_DAC[a] = iDAC[a]; //we keep track of the voltage at gate state change for envelope smoothing routines
      if (gateState[a] == HIGH){//new incoming trig!
        EG_count[a] = 0;
      }
      else {//gate closes
        EG_count[a] = relIndex[a];//RELEASE PHASE START
      }
    }
  }
}

void PotRead(){
  for(int a = 0; a < POT_NUM; a++){
    potVal[chNum][a] = analogRead(POT_PIN[a]);
    potVal[chNum][a] = potVal[chNum][a] >> 3; //rescale pots values from 0-1023 to 0-127
    if(potVal[chNum][a] == potLockVal[chNum][a]){
      potEnable[chNum][a] = true; //enable pot reading
    }
    if(potEnable[chNum][a] == true){
      if(potVal[chNum][a] > 63){//this sets a time stretch
        del_time[chNum][a] = potVal[chNum][a] - 63;
        res[chNum][a] = 1;
      }
      else{//this sets a resolution decrease
        del_time[chNum][a] = 0;
        res[chNum][a] = (63 - potVal[chNum][a])>>3;
        if(res[chNum][a] <=0){
          res[chNum][a] = 1;
        }
      }
      potLockVal[chNum][a] = potVal[chNum][a];//update pot value. This is the actual value DAC outputs
    }
  }
}

void EG_RECALL(int c){
  tft->fillScreen(BLACK);  // clean display
  float resScaled = map(resIndex, 0, 4095, 0, 320);
  float firstScaled = map(firstX[c], 0, 4095, 320, 0);//map touch index to screen index for drawing
  if(EG_Stored[c] == true){
    for(int d = 0; d < MAXindex[c]; d++){  
      tft->fillCircle(map(firstScaled - resScaled*d, 0, 320, 320, 0), 240 - EG_Y[c][d], 2, CH_COLOR[c]);
      //tft->fillCircle(firstScaled - resScaled*d, 240 - EG_Y[c][d], 2, CH_COLOR[c]);
    }
    if(relIndex[c] > 0){//in case of an LFO we don't want the release circle to be drawn
      tft->fillCircle(map(firstScaled - relIndex[c]*resScaled - 8, 0, 320, 320, 0), 240 - EG_Y[c][relIndex[c]], 10, VIOLET);
    }
  }
  TFT_DRAW_LINES();
}

void TFT_DRAW_LINES(){
  tft->drawFastVLine(relDrawX, 0, 240, WHITE); //x0, y0, lenght, color
  //tft->drawFastHLine(0, 23, 320, WHITE); //x0, y0, lenght, color
}

void COPY_ENV(int a_from, int b_to){
  for (int e = 0; e < MAX_POINTS; e++){
    EG_Y[b_to][e] = EG_Y[a_from][e];
  }
  MAXindex[b_to] = MAXindex[a_from];
  relIndex[b_to] = relIndex[a_from];
  firstX[b_to] = firstX[a_from];
  EG_Stored[b_to] = true;
}