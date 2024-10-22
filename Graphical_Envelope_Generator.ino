 /*Graphical Quad Envelope Generator
 * 4X envelopes generator for eurorack synthesizers
 * Hardware:
 * Arduino nano 328p
 * ST7735/89 SPI display with touchscreen;
 * MCP4728 I2C 12-bit DAC
*
 * Known bugs: if the first EG is not drawn, the others are locked
 *
 * by barito (last update october 2024)
 */

#include <Adafruit_GFX.h>           // graphic library
#include <Adafruit_ST7789.h>        // Hardware-specific library for ST7789
#include <XPT2046_Touchscreen.h>    // XPT2046 touch library (Paul Stoffregen)
#include <SPI.h>                    // SPI communication library
#include <Wire.h>                   // I2C communication library
#include <mcp4728.h>                // DAC MCP4728 library (Benoit Shillings https://github.com/BenoitSchillings/mcp4728)

const int OUT_NUM = 4;
const int POT_NUM = 3;

//RGB565 format, inverted
#define BLACK 0xFFFF
#define WHITE 0x0000
#define VIOLET 0x15E2
#define GREEN 0xB8B7
#define BLUE 0xBD22
#define YELLOW 0x18B7
#define RED 0x16BE

int CH_COLOR[OUT_NUM] = {GREEN, YELLOW, BLUE, RED};

// Display pins
const int TFT_CS = 10;
const int TFT_DC = 9;
const int TFT_RST =  8;   // to Vcc if not used
//#define TFT_MOSI 11 //hardware SPI, no need to define
//#define TFT_MISO 12
//#define TFT_CLK  13

// Touchscreen XPT2046 pins
const int TOUCH_CS = 7;

// DAC MCP4728 pins
const int LDAC = 2;  // DAC latch pin
//#define SDA A4
//#define SCL A5

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

// Display and touchscreen definitions
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
XPT2046_Touchscreen ts(TOUCH_CS);
mcp4728 dac = mcp4728(0); // initiate mcp4728 object, Device ID = 0 (default)

// envelope draw profiles varibles
const byte MAX_POINTS = 160;  // maximun Y data EG_form lenght for storage
int pointIndex = -1;  // index to track the running EG
byte EG_Xform[OUT_NUM][MAX_POINTS];//initializing the EG_Xform array bugs the output ...

byte MAXindex[OUT_NUM]; // actual latest value stored index
int firstY[OUT_NUM]; //first point Y position

int TFT_y[OUT_NUM];
int TFT_x[OUT_NUM];
byte chNum = 0; //EG channel in use. We start from "A" channel

// DAC out EG_form
const int offsetX = 300;
int iDAC[OUT_NUM];//instant DAC value
byte EG_count[OUT_NUM] = {0, 0, 0, 0};
unsigned long EG_runTime[OUT_NUM];
int del_time[OUT_NUM][POT_NUM]; //delay execution time between points
int res[OUT_NUM][POT_NUM]; //execution resolution (determines how many points of the curve are missed)

//Flags
bool isDrawing[OUT_NUM] = {0, 0, 0, 0};   // drawing flag
bool EG_Stored[OUT_NUM] = {0, 0, 0, 0};  // EG_form flag
bool newDrawing[OUT_NUM] = {1, 1, 1, 1};  // new drawing flag

//display visuals and sectors consts
const int relDrawY = 120;
int relPosY = map(relDrawY, 0, 320, 4095, 0);
int relIndex[OUT_NUM];
byte resIndex = 26;//data acquisition space interval.

void setup() {
  pinMode(LDAC, OUTPUT);
  digitalWrite(LDAC, LOW);

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

  // Display and touchscreen init
  tft.init(240, 320);   // Init ST7789 320x240
  tft.setRotation(3);  // Screen orientation (0-3)
  tft.fillScreen(BLACK);  // clean display
  //tft.invertDisplay(true);
  //tft.setSPISpeed(40000000);
  TFT_DRAW_LINES();

  ts.begin();
  ts.setRotation(2);  // touchscreen orientation (0-3)
  
  dac.begin();  // initialize i2c interface
  // Inizializzazione del DAC
  dac.setPowerDown(0, 0, 0, 0); // set Power-Down ( 0 = Normal , 1-3 = shut down most channel circuit, no voltage out) (1 = 1K ohms to GND, 2 = 100K ohms to GND, 3 = 500K ohms to GND)
  dac.setVref(0, 0, 0, 0); // set to use internal voltage reference (2.048V) ( 0 = external, 1 = internal )
  //dac.setGain(0, 0, 0, 0); // set the gain of internal voltage reference ( 0 = gain x1, 1 = gain x2 )
  dac.analogWrite(0, 0, 0, 0); //set all outs to zero

  //Load_EG_Par();
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
  if (ts.touched()){ //check if touchscreen has been touched...
    TS_Point p = ts.getPoint();  // Ottieni le coordinate del tocco
    // new drawing start, init
    if(newDrawing[chNum] && p.y < relPosY){//this makes not possible to draw a release-only envelope
      newDrawing[chNum] = false;  // drawing has started
      tft.fillScreen(BLACK);  // clean display
      TFT_DRAW_LINES();
      pointIndex = -1;  //point index init
      MAXindex[chNum] = 0; //max value init
      relIndex[chNum] = 0; //release index init
      EG_Stored[chNum] = false;  // memory state reset
      firstY[chNum] = p.y;//new drawing start
    }
    if(pointIndex < MAX_POINTS){//check for left space in the array
      if(p.y > firstY[chNum] + (pointIndex+1)*resIndex){//if the point follows the one before
        pointIndex++;
        // Map touch coordinates on screen dimension (320x240)
        TFT_y[chNum] = map(p.y, 4095, 0, 0, 320); //map(value, fromLow, fromHigh, toLow, toHigh)
        TFT_x[chNum] = p.x >> 4;//tracks the pen better than map.....
        //Overflow(TFT_y[chNum], 0, 320);
        //Overflow(TFT_x[chNum], 0, 240);
        //tft.drawFastVLine(TFT_y[chNum], 0, TFT_x[chNum], CH_COLOR[chNum]); //x0, y0, lenght, color
        tft.fillCircle(TFT_y[chNum], TFT_x[chNum], 2, CH_COLOR[chNum]);//EG dots
        EG_Xform[chNum][pointIndex] = TFT_x[chNum];  // save the X value in the EG_ array        
        MAXindex[chNum] = pointIndex;
        
        //set release indexes
        if (p.y > relPosY - 13 && relIndex[chNum] == 0){
          relIndex[chNum] = pointIndex;
          tft.fillCircle(TFT_y[chNum], EG_Xform[chNum][relIndex[chNum]], 10, VIOLET);//draw release dot
        }
        EG_Stored[chNum] = true;  // vaweform progress recorded
        isDrawing[chNum] = true;
        dac.analogWrite(chNum, (TFT_x[chNum]<<4) - offsetX);
      }
    }
  }
  else if (isDrawing[chNum]){ //no touch, draw ended
    isDrawing[chNum] = false;
    newDrawing[chNum] = true;  // new draw flag reset
    //firstY[chNum] = 0;
    TFT_DRAW_LINES();
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
    Overflow (EG_count[ch_fl], 0, MAXindex[ch_fl]);
    //iDAC = map (EG_form[e]<<4, Xmin, 4095, 0, 4095);
    iDAC[ch_fl] = (EG_Xform[ch_fl][EG_count[ch_fl]]<<4) - offsetX - 1023 + (potLockVal[ch_fl][0]<<4); //POT 1
    Overflow(iDAC[ch_fl], 0, 4095);
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
          potEnable[chNum][0] = false;//pots for "going" channel are disabled
          potEnable[chNum][1] = false;
          potEnable[chNum][2] = false;
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
    //Overflow(potVal[chNum][a], 0, 1023);
    potVal[chNum][a] = potVal[chNum][a] >> 3; //rescale pots values from 0-1023 to 0-127
    if(potVal[chNum][a] == potLockVal[chNum][a]){
      potEnable[chNum][a] = true; //enable pot reading
    }
    if(potEnable[chNum][a] == true){
      if(potVal[chNum][a] > 63){//this sets a time stretch
        del_time[chNum][a] = potVal[chNum][a] - 63;
        //Overflow(del_time[chNum][a], 0, 63)
        res[chNum][a] = 1;
      }
      else{//this sets a resolution decrease
        del_time[chNum][a] = 0;
        res[chNum][a] = (63 - potVal[chNum][a])>>3;
        //Overflow(res[chNum][a], 1, 15)
        if(res[chNum][a] <=0){
          res[chNum][a] = 1;
        }
      }
      potLockVal[chNum][a] = potVal[chNum][a];//update pot value. This is the actual value DAC outputs
    }
  }
}

void EG_RECALL(int c){
  tft.fillScreen(BLACK);  // clean display
  float resScaled = map(resIndex, 0, 4095, 0, 320);
  float firstScaled = map(firstY[c], 0, 4095, 0, 320);
  if(EG_Stored[c] == true){
    for(int d = 0; d < MAXindex[c]; d++){  
      //tft.drawFastVLine(map(firstScaled + resScaled*d, 0, 320, 320, 0), 0, EG_Xform[c][d], CH_COLOR[c]); //x0, y0, lenght, color
      tft.fillCircle(map(firstScaled + resScaled*d, 0, 320, 320, 0), EG_Xform[c][d], 2, CH_COLOR[c]);
    }
    if(relIndex[c] > 0){//in case of an LFO we don't want the release circle to be drawn
      tft.fillCircle(map(firstScaled + relIndex[c]*resScaled, 0, 320, 320, 0), EG_Xform[c][relIndex[c]], 10, VIOLET);
    }
  }
  TFT_DRAW_LINES();
}

void Overflow (int data, int vMin, int vMax){
  if (data > vMax){data = vMax;}
  else if (data < vMin){data = vMin;}
}

void TFT_DRAW_LINES(){
  tft.drawFastVLine(relDrawY, 23, 240, WHITE); //x0, y0, lenght, color
  tft.drawFastHLine(0, 23, 320, WHITE); //x0, y0, lenght, color
}

void COPY_ENV(int a_from, int b_to){
  for (int e = 0; e < MAX_POINTS; e++){
    EG_Xform[b_to][e] = EG_Xform[a_from][e];
  }
  MAXindex[b_to] = MAXindex[a_from];
  relIndex[b_to] = relIndex[a_from];
  firstY[b_to] = firstY[a_from];
  EG_Stored[b_to] = true;
}