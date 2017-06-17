//NeoPixel clock with FLORA based on Kevin Alford's NeoPixel ring clock face
//https://github.com/zeroeth/time_loop
//http://www.youtube.com/watch?v=b-mROp-ZKuk
//modified by Becky Stern for Adafruit Industries
#include <Adafruit_NeoPixel.h>
#include <Time.h>
#include <Adafruit_GPS.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <Timeout.h>

// what's the name of the hardware serial port?
#define GPSSerial Serial1
 
Adafruit_GPS GPS(&GPSSerial);
 
#define PIN 12
 
// Set GPSECHO to 'false' to turn off echoing the GPS data to the Serial console
// Set to 'true' if you want to debug and listen to the raw GPS sentences
#define GPSECHO true
 
// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_RGB     Pixels are wired for RGB bitstream
//   NEO_GRB     Pixels are wired for GRB bitstream
//   NEO_KHZ400  400 KHz bitstream (e.g. FLORA pixels)
//   NEO_KHZ800  800 KHz bitstream (e.g. High Density LED strip)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(36, PIN, NEO_GRB + NEO_KHZ800);
 
//--------------------------------------------------|
//Your NeoPixel rings may not line up with ours, or each other.     |
//Enter which NeoPixel leds are on top LED (0-23 and 0-13).  |
#define outerTOP_LED                16
#define innerTOP_LED                9
//--------------------------------------------------|
 
int outertopLED = outerTOP_LED;
int innertopLED = innerTOP_LED;

/*
//tequila sunrise color scheme 
uint32_t hour_color   = strip.Color (  44, 42,  0); //yellow
uint32_t minute_color = strip.Color ( 43,   0,  5); //red
uint32_t second_color = strip.Color ( 44,  30,  0); //slightly yellower
uint32_t milli_color  = strip.Color ( 22,  10,  0); //redest orange
uint32_t off_color    = strip.Color (  0,   0,  0);
*/

//blue, green, & purple color scheme
uint32_t hour_color   = strip.Color (  0, 10,  44); //royal blue
uint32_t minute_color = strip.Color (  0, 44,  10); //green
uint32_t second_color = strip.Color ( 17,  0,  44); //purple
uint32_t milli_color  = strip.Color ( 12,  0,  12); //magenta
uint32_t off_color    = strip.Color (  0,  0,   0);

// Offset hours from gps time (UTC)
const int offset = 3;
 
/* CLOCK */
class ClockPositions {
 public:
  uint8_t milli;
  uint8_t currentsecond;
  uint8_t currentminute;
  uint8_t currenthour;
 
  ClockPositions ();
  void update    ();
};
 
ClockPositions::ClockPositions() {}
 
void ClockPositions::update() {
  currenthour = innertopLED + map ((hour() % 12), 0,  12, 24, 35);
  if (currenthour > 35) currenthour -= 12;
  currentminute = outertopLED + map (minute() % 60, 0,  60, 0, 23);
  if (currentminute > 23) currentminute -= 24;
  currentsecond = outertopLED + map ((second() % 60), 0, 60, 0, 23);
  if (currentsecond > 24) currentsecond -= 24;
  milli = map ((millis() %  1000), 0,  1000, 0, 24);
}
  
/* CLOCK VIEW */
 
class ClockSegments {
 public:
  ClockPositions    &positions;
  Adafruit_NeoPixel &strip;
 
  ClockSegments (Adafruit_NeoPixel&, ClockPositions&);
  void draw  ();
  void clear ();
  void add_color (uint8_t position, uint32_t color);
  uint32_t blend (uint32_t color1, uint32_t color2);
};
 
 
ClockSegments::ClockSegments (Adafruit_NeoPixel& n_strip, ClockPositions& n_positions): strip (n_strip), positions (n_positions) {}
 
void ClockSegments::draw() {
  clear(); 
  add_color(positions.currenthour, hour_color);
  add_color(positions.currentminute, minute_color);
  add_color(positions.currentsecond     % 24, second_color);
  add_color((positions.currentsecond+1) % 24, second_color);
  add_color(positions.milli % 24, milli_color);
  strip.show();
}
 
void ClockSegments::add_color (uint8_t position, uint32_t color) {
  uint32_t blended_color = blend(strip.getPixelColor(position), color);
  strip.setPixelColor (position, blended_color);
}
 
uint32_t ClockSegments::blend (uint32_t color1, uint32_t color2) {
  uint8_t r1,g1,b1;
  uint8_t r2,g2,b2;
  uint8_t r3,g3,b3;
 
  r1 = (uint8_t)(color1 >> 16),
  g1 = (uint8_t)(color1 >>  8),
  b1 = (uint8_t)(color1 >>  0);
 
  r2 = (uint8_t)(color2 >> 16),
  g2 = (uint8_t)(color2 >>  8),
  b2 = (uint8_t)(color2 >>  0);
 
  return strip.Color(constrain(r1+r2, 0, 255), constrain(g1+g2, 0, 255), constrain(b1+b2, 0, 255));
}
 
void ClockSegments::clear () {
  for(uint16_t i=0; i<strip.numPixels (); i++) {
      strip.setPixelColor (i, off_color);
  }
}
  
/* APP */
ClockPositions positions;
ClockSegments  segments(strip, positions);
 
void setup () {
  // connect at 115200 so we can read the GPS fast enough and echo without dropping chars
  Serial.begin(115200);
  Serial.println("--- NeoClock ---");
    
  // 9600 NMEA is the default baud rate for Adafruit MTK GPS's- some use 4800
  GPS.begin(9600);
  // uncomment this line to turn on RMC (recommended minimum) and GGA (fix data) including altitude
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  // uncomment this line to turn on only the "minimum recommended" data
  //GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);
  // For parsing data, we don't suggest using anything but either RMC only or RMC+GGA since
  // the parser doesn't care about other sentences at this time
  // Set the update rate
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ); // 1 Hz update rate
  // For the parsing code to work nicely and have time to sort thru the data, and
  // print it out we don't suggest using anything higher than 1 Hz

  // strip
  strip.begin ();
  strip.show (); // Initialize all pixels to 'off'
}
 
Timeout gpsTimeout(0);

void loop () {
  // read data from the GPS in the 'main loop'
  while (GPSSerial.available()) {
    char c = GPS.read();
    // if you want to debug, this is a good time to do it!
    if (GPSECHO && c) Serial.print(c);
    // if a sentence is received, we can check the checksum, parse it...
    if (GPS.newNMEAreceived()) {
      GPS.parse(GPS.lastNMEA());
    }
  }
  
  // approximately every 60 seconds or so, update time
  if (GPS.fix && gpsTimeout.check()) {
    gpsTimeout.reset(60000L);
    Serial.print("## GPS time: ");
    Serial.print(GPS.hour, DEC); Serial.print(':');
    Serial.print(GPS.minute, DEC); Serial.print(':');
    Serial.print(GPS.seconds, DEC); Serial.print('.');
    Serial.println(GPS.milliseconds);
    // set the Time to the latest GPS reading
    setTime(GPS.hour, GPS.minute, GPS.seconds, GPS.day, GPS.month, GPS.year);
    adjustTime(offset * SECS_PER_HOUR);  
  }

  // diplay
  positions.update();
  segments.draw();  
}
 
// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint32_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, c);
      strip.show();
      delay(wait);
  }
}
 
// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;
 
  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}
 
// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
   return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170;
   return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}
