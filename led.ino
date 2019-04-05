#define REDUCED_MODES // sketch is too big for Arduino w/32k flash, so invoke reduced modes
#include <WS2812FX.h>
#include <EEPROM.h> // Use EEPRO to save last LED mode. EEPROM is good 100,000 write/erase cycles

#define LED_COUNT 40
#define LED_PIN 6

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
WS2812FX ws2812fx = WS2812FX(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

String cmd = "";               // String to store incoming serial commands
boolean cmd_complete = false;  // whether the command string is complete
int EEPROaddress_mode = 120;    // EEPROM address where led mode is store
int EEPROaddress_color_r = 130;   // EEPROM address where red color value is store
int EEPROaddress_color_g = 131;   // EEPROM address where green color value is store
int EEPROaddress_color_b = 132;   // EEPROM address where blue color value is store

void setup() {
  Serial.begin(9600);
  Serial.setTimeout(10);
  ws2812fx.init();
  ws2812fx.setBrightness(100);
  ws2812fx.setSpeed(1000);
  ws2812fx.setColor(EEPROM.read(EEPROaddress_color_r), EEPROM.read(EEPROaddress_color_g), EEPROM.read(EEPROaddress_color_b));  // read color from EEPROM
  ws2812fx.setMode(EEPROM.read(EEPROaddress_mode));   // read mod from EEPROM
  ws2812fx.start();

  printModes();
  printUsage();
}

void(*resetFunc) (void) = 0; //----function for rest arduino-------

void loop() {
  ws2812fx.service();

  // On Atmega32U4 based boards (leonardo, micro) serialEvent is not called
  // automatically when data arrive on the serial RX. We need to do it ourself
  #if defined(__AVR_ATmega32U4__) || defined(ESP8266)
  serialEvent();
  #endif

  
    process_command();
 
}

/*
 * Checks received command and calls corresponding functions.
 */
void process_command() {
  if(cmd.startsWith(F("b+"))) { 
    ws2812fx.increaseBrightness(10);
    Serial.print(F("Increased brightness by 25 to: "));
    Serial.println(ws2812fx.getBrightness());
  }

  if(cmd.startsWith(F("b-"))) {
    ws2812fx.decreaseBrightness(10); 
    Serial.print(F("Decreased brightness by 25 to: "));
    Serial.println(ws2812fx.getBrightness());
  }

  if(cmd.startsWith(F("b "))) { 
    uint8_t b = (uint8_t) cmd.substring(2, cmd.length()).toInt();
    ws2812fx.setBrightness(b);
    Serial.print(F("Set brightness to: "));
    Serial.println(ws2812fx.getBrightness());
  }

  if(cmd.startsWith(F("s+"))) { 
  ws2812fx.increaseSpeed(10);
    ws2812fx.setSpeed(ws2812fx.getSpeed() * 1.2);
    Serial.print(F("Increased speed by 10 to: "));
    Serial.println(ws2812fx.getSpeed());
  }

  if(cmd.startsWith(F("s-"))) {
  ws2812fx.decreaseSpeed(10);
    ws2812fx.setSpeed(ws2812fx.getSpeed() * 0.8);
    Serial.print(F("Decreased speed by 10 to: "));
    Serial.println(ws2812fx.getSpeed());
  }

  if(cmd.startsWith(F("s "))) {
    uint16_t s = (uint16_t) cmd.substring(2, cmd.length()).toInt();
    ws2812fx.setSpeed(s); 
    Serial.print(F("Set speed to: "));
    Serial.println(ws2812fx.getSpeed());
  }

  if(cmd.startsWith(F("m "))) { 
    uint8_t m = (uint8_t) cmd.substring(2, cmd.length()).toInt();
    EEPROM.write(EEPROaddress_mode, m); // stor mod in EEPROM
    ws2812fx.setMode(m);
    Serial.print(F("Set mode to: "));
    Serial.print(ws2812fx.getMode());
    Serial.print(" - ");
    Serial.println(ws2812fx.getModeName(ws2812fx.getMode()));
  }

  if(cmd.startsWith(F("c "))) { 
    // To generate HEX color value
    uint32_t c = (uint32_t) strtol(&cmd.substring(2, cmd.length())[0], NULL, 16);
    ws2812fx.setColor(c);    
    Serial.print(F("Set color to: "));
    Serial.print(F("0x"));
    if(ws2812fx.getColor() < 0x100000) { Serial.print(F("0")); }
    if(ws2812fx.getColor() < 0x010000) { Serial.print(F("0")); }
    if(ws2812fx.getColor() < 0x001000) { Serial.print(F("0")); }
    if(ws2812fx.getColor() < 0x000100) { Serial.print(F("0")); }
    if(ws2812fx.getColor() < 0x000010) { Serial.print(F("0")); }
    Serial.println(ws2812fx.getColor(), HEX);

    // To generate RGB color value 
    long number = (long) strtol( &cmd.substring(2, cmd.length())[0], NULL, 16);
    int r = number >> 16;
    int g = number >> 8 & 0xFF;
    int b = number & 0xFF;

    Serial.print("red is ");
    EEPROM.write(EEPROaddress_color_r, r); // stor red  value color in EEPROM
    Serial.println(r);
    Serial.print("green is ");
    EEPROM.write(EEPROaddress_color_g, g); // stor green  value color in EEPROM
    Serial.println(g);
    Serial.print("blue is ");
    EEPROM.write(EEPROaddress_color_b, b); // stor blue  value color in EEPROM
    Serial.println(b);
   
    

  }

  if(cmd.startsWith(F("f "))) //----restart funcrion call-----
  {
     resetFunc();
     delay(1000); 
  }

  cmd = "";              // reset the commandstring
  cmd_complete = false;  // reset command complete
}

/*
 * Prints a usage menu.
 */
void printUsage() {
  Serial.println(F("Usage:"));
  Serial.println();
  Serial.println(F("m <n> \t : select mode <n>"));
  Serial.println();
  Serial.println(F("b+ \t : increase brightness"));
  Serial.println(F("b- \t : decrease brightness"));
  Serial.println(F("b <n> \t : set brightness to <n>"));
  Serial.println();
  Serial.println(F("s+ \t : increase speed"));
  Serial.println(F("s- \t : decrease speed"));
  Serial.println(F("s <n> \t : set speed to <n>"));
  Serial.println();
  Serial.println(F("c 0x007BFF \t : set color to 0x007BFF"));
  Serial.println();
  Serial.println();
  Serial.println(F("f space \t : to rest arduino"));
  Serial.println();
  Serial.println();
  Serial.println( EEPROM.read(EEPROaddress_mode) );
//  Serial.println( EEPROM.read(EEPROaddress_color));
  Serial.println();
  Serial.println();
  Serial.println(F("Have a nice day."));
  Serial.println(F("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"));
  Serial.println();
}


/*
 * Prints all available WS2812FX blinken modes.
 */
void printModes() {
  Serial.println(F("Supporting the following modes: "));
  Serial.println();
  for(int i=0; i < ws2812fx.getModeCount(); i++) {
    Serial.print(i);
    Serial.print(F("\t"));
    Serial.println(ws2812fx.getModeName(i));
  }
  Serial.println();
}


/*
 * Reads new input from serial to cmd string. Command is completed on \n
 */
void serialEvent() {
  while(Serial.available()) {
    String inChar =  Serial.readString();
    
    if(inChar == '\n') {
      cmd_complete = true;
    } else {
      cmd = inChar;
      
    }
  }
}



