 /*New wave shaper is part of Graphical Envelope Generator project.
 * draw your waveshape and have it tabulated.
 *
 * by barito (last update dec 2024)
 */

#include <TS_Display.h>             // Touch to display mapping functions lib (Ted Toal)
#include <Adafruit_GFX.h>           // graphic library
#include <Adafruit_ST7789.h>        // Hardware-specific library for ST7789
#include <XPT2046_Touchscreen_TT.h> // XPT2046 touch library (Paul Stoffregen & Ted Toal)
#include <SPI.h>                    // SPI communication library

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

// Buttons pins
const int BTN_PIN = 4;
bool btnState;

// Display and touchscreen objects
Adafruit_ST7789* tft; // Pointer to display object.
XPT2046_Touchscreen* ts; // Pointer to touchscreen object.
TS_Display* ts_display;// Pointer to touchscreen-display object.

// envelope draw profiles varibles
const int MAX_POINTS = 240;  // maximun data EG_form lenght for storage
int pointIndex = -1;  // index to track the running EG
byte EG_Y[MAX_POINTS];

int MAXindex; // actual latest value stored index
int firstX; //first point X position

//Flags
bool isDrawing = 0;   // drawing flag
bool EG_Stored = 0;  // EG_form flag

//display visuals and sectors consts
const int relDrawX = 200;
int relPosX = map(relDrawX, 0, 320, 0, 4095);
int relIndex;
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

  pinMode(BTN_PIN, INPUT_PULLUP);
  btnState = digitalRead(BTN_PIN);

  TFT_DRAW_LINES();

  Serial.begin(9600);
}

void loop() {
  Switches();
  TouchDraw(); 
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
    if(isDrawing == false && p.x < relPosX){//this makes not possible to draw a release-only envelope
      isDrawing = true;  // drawing has started
      tft->fillScreen(BLACK);  // clean display
      TFT_DRAW_LINES();
      pointIndex = -1;  //point index init
      MAXindex = -1; //max value init
      relIndex = -1; //release index init
      EG_Stored = false;  // memory state reset
      firstX = p.x;//new drawing start
    }
    if(pointIndex < MAX_POINTS){//check for left space in the array
      if(p.x > firstX + (pointIndex + 1) * resIndex){//if the point follows the one before
        pointIndex++;
        tft->fillCircle(x, y, 2, YELLOW);//EG dots
        EG_Y[pointIndex] = 240 - y;  // save the (rectified) Y value in the EG_ array        
        
        //set release indexes
        if (p.x > relPosX - 80 && relIndex == -1){
          relIndex = pointIndex;
          tft->fillCircle(x, y, 10, VIOLET);//draw release dot
        }
        EG_Stored = true;  // vaweform progress recorded
      }
    }
  }
  else { //no touch, draw ended
    if (isDrawing){ //no touch, draw ended
      isDrawing = false;
      MAXindex = pointIndex;
      isTouched = false;
      TFT_DRAW_LINES();
    }
  }
}

void dataOut(){
//output envelope data to serial monitor
Serial.println("");
Serial.print("{");
for(int a = 0; a < MAXindex; a++){
  Serial.print(EG_Y[a]);
  Serial.print(", ");
}
//complete the array
for(int a = MAXindex; a < MAX_POINTS-1; a++){
  Serial.print(0);
  Serial.print(", ");
}
Serial.println("0}");

Serial.println("");
Serial.print("MAXindex = ");
Serial.print(MAXindex);
Serial.println(";");
Serial.print("firstX = ");
Serial.print(firstX);
Serial.println(";");
Serial.print("relIndex = ");
Serial.print(relIndex);
Serial.println(";");
}


void Switches(){
  if(digitalRead(BTN_PIN) != btnState){ //on state change...
    delay(100); //cheap debounce
    btnState = !btnState;
    if (btnState == LOW){ //button pressed
      dataOut();
    }
  }
}

void TFT_DRAW_LINES(){
  tft->drawFastVLine(relDrawX, 0, 240, WHITE); //x0, y0, lenght, color
  //tft->drawFastHLine(0, 23, 320, WHITE); //x0, y0, lenght, color
}

