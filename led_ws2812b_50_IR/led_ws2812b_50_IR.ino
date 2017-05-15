#include "FastLED.h"
#include <IRremote.h>

// Whats the maximum for LEDS
#define MAX_LEDS 120

// For led chips like Neopixels, which have a data line, ground, and power, you just
// need to define DATA_PIN.  For led chipsets that are SPI based (four wires - data, clock,
// ground, and power), like the LPD8806 define both DATA_PIN and CLOCK_PIN
#define IRRECV_PIN 10
#define DATA_PIN 11
#define CLOCK_PIN 12

// Define the color order, can be different from stripe to stripe!
#define COLOR_ORDER RGB

// How many leds in your strip?
unsigned int NUM_LEDS = 50;

// Adalight sends a "Magic Word" (defined in /etc/boblight.conf) before sending the pixel data
uint8_t prefix[] = {'A', 'd', 'a'}, hi, lo, chk, i;

// Baudrate, higher rate allows faster refresh rate and more LEDs (defined in /etc/boblight.conf)
#define serialRate 230400

// Define the array of leds
CRGB leds[MAX_LEDS];

// Define IR Receiver
IRrecv irrecv(IRRECV_PIN);
decode_results irresults;
unsigned int uilastir;
byte bRed = 255;
byte bGreen = 255;
byte bBlue = 255;
byte bRedStep = 8;
byte bGreenStep = 8;
byte bBlueStep = 8;

// Turned ON or OFF
bool bTurnedOn = true;

void setup() {
  // Uncomment/edit one of the following lines for your leds arrangement.
  // FastLED.addLeds<TM1803, DATA_PIN, RGB>(leds, NUM_LEDS);
  // FastLED.addLeds<TM1804, DATA_PIN, RGB>(leds, NUM_LEDS);
  // FastLED.addLeds<TM1809, DATA_PIN, RGB>(leds, NUM_LEDS);
  // FastLED.addLeds<WS2811, DATA_PIN, RGB>(leds, NUM_LEDS);
  // FastLED.addLeds<WS2812, DATA_PIN, RGB>(leds, NUM_LEDS);

  // FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  // FastLED.addLeds<UCS1903, DATA_PIN, RGB>(leds, NUM_LEDS);
  // FastLED.addLeds<UCS1903B, DATA_PIN, RGB>(leds, NUM_LEDS);
  // FastLED.addLeds<GW6205, DATA_PIN, RGB>(leds, NUM_LEDS);
  // FastLED.addLeds<GW6205_400, DATA_PIN, RGB>(leds, NUM_LEDS);

  // FastLED.addLeds<WS2801, RGB>(leds, NUM_LEDS);
  // FastLED.addLeds<SM16716, RGB>(leds, NUM_LEDS);
  // FastLED.addLeds<LPD8806, RGB>(leds, NUM_LEDS);

  // FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN, RGB>(leds, NUM_LEDS);
  // FastLED.addLeds<SM16716, DATA_PIN, CLOCK_PIN, RGB>(leds, NUM_LEDS);
  // FastLED.addLeds<LPD8806, DATA_PIN, CLOCK_PIN, RGB>(leds, NUM_LEDS);

  // This Sketch supports WS2812B Protocol!
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, MAX_LEDS /*NUM_LEDS*/);

  // initial RGB flash
  LEDS.showColor(CRGB(255, 0, 0));
  delay(100);
  LEDS.showColor(CRGB(0, 255, 0));
  delay(100);
  LEDS.showColor(CRGB(0, 0, 255));
  delay(100);
  LEDS.showColor(CRGB(0, 0, 0));

  // Start IR Receiver First!
  irrecv.enableIRIn();

  Serial.begin(serialRate);
  Serial.print("Ada\n"); // Send "Magic Word" string to host
}

void loop() {
  // ADA Light HANDLING here!
  if (Serial.available())
  {
    // wait for first byte of Magic Word
    for (i = 0; i < sizeof prefix; ++i) {
waitLoop: while (!Serial.available()) ;;
      // Check next byte in Magic Word
      if (prefix[i] == Serial.read()) continue;
      // otherwise, start over
      i = 0;
      goto waitLoop;
    }

    // Hi, Lo, Checksum
    while (!Serial.available()) ;;
    hi = Serial.read();
    while (!Serial.available()) ;;
    lo = Serial.read();
    while (!Serial.available()) ;;
    chk = Serial.read();

    // if checksum does not match go back to wait
    if (chk != (hi ^ lo ^ 0x55))
    {
      i = 0;
      goto waitLoop;
    }

    /* Combine HI and LO byte to LED Count */
    unsigned int ADA_NUMLEDS = (256L * (long)hi + (long)lo + 1L);
    if ( ADA_NUMLEDS > MAX_LEDS )
      ADA_NUMLEDS = MAX_LEDS; /* Limit to maximum LEDS! */

    memset(leds, 0, ADA_NUMLEDS /*NUM_LEDS*/ * sizeof(struct CRGB));
    // read the transmission data and set LED values
    for (uint8_t i = 0; i < ADA_NUMLEDS /*NUM_LEDS*/; i++) {
      byte r, g, b;
      while (!Serial.available());
      r = Serial.read();
      while (!Serial.available());
      g = Serial.read();
      while (!Serial.available());
      b = Serial.read();
      leds[i].r = r;
      leds[i].g = g;
      leds[i].b = b;
    }

    // Show only if turned on
    if (bTurnedOn) {
      // shows new values
      FastLED.show();
    }
  }

  /* IR Remote Code Handling FIRST! */
  if (irrecv.decode(&irresults)) {
    /*Only DEBUG      
    Serial.println(irresults.value, HEX);
    */
    if ( irresults.value != 0xFFFFFFFF ) /* Repeat last command */
      uilastir = irresults.value;
    switch (uilastir)
    {
      case 0xFFA05F:  /* +Light */
        if (bRed < 255 && bGreen < 255 && bBlue < 255) {
          bRed = (bRed > 0 ? bRed + bRedStep : bRed);
          bGreen = (bGreen > 0 ? bGreen + bGreenStep : bGreen);
          bBlue = (bBlue > 0 ? bBlue + bBlueStep : bBlue);
        }
        LEDS.showColor(CRGB(bRed, bGreen, bBlue));
        break;
      case 0xFF906F:  /* Red */
        bRed = 255; bGreen = 0; bBlue = 0; bRedStep = 8; bGreenStep = 8; bBlueStep = 8;
        LEDS.showColor(CRGB(bRed, bGreen, bBlue));
        break;
      case 0xFFB04F:  /* Red-Orange */
        bRed = 255; bGreen = 31; bBlue = 0; bRedStep = 8; bGreenStep = 1; bBlueStep = 0;
        LEDS.showColor(CRGB(bRed, bGreen, bBlue));
        break;
      case 0xFFA857:  /* Orange */
        bRed = 255; bGreen = 63; bBlue = 0; bRedStep = 8; bGreenStep = 2; bBlueStep = 0;
        LEDS.showColor(CRGB(bRed, bGreen, bBlue));
        break;
      case 0xFF9867:  /* Orange-yellow */
        bRed = 255; bGreen = 127; bBlue = 0; bRedStep = 8; bGreenStep = 4; bBlueStep = 0;
        LEDS.showColor(CRGB(bRed, bGreen, bBlue));
        break;
      case 0xFF8877:  /* Yellow */
        bRed = 255; bGreen = 255; bBlue = 0; bRedStep = 8; bGreenStep = 8; bBlueStep = 8;
        LEDS.showColor(CRGB(bRed, bGreen, bBlue));
        break;

      case 0xFF20DF:  /* -Light */
        bRed = (bRed > bRedStep ? bRed - bRedStep : bRed);
        bGreen = (bGreen > bGreenStep ? bGreen - bGreenStep : bGreen);
        bBlue = (bBlue > bBlueStep ? bBlue - bBlueStep : bBlue);
        LEDS.showColor(CRGB(bRed, bGreen, bBlue));
        break;
      case 0xFF10EF:  /* Green */
        bRed = 0; bGreen = 255; bBlue = 0; bRedStep = 8; bGreenStep = 8; bBlueStep = 8;
        LEDS.showColor(CRGB(bRed, bGreen, bBlue));
        break;
      case 0xFF30CF:  /* Green-light */
        bRed = 63; bGreen = 255; bBlue = 63; bRedStep = 2; bGreenStep = 8; bBlueStep = 2;
        LEDS.showColor(CRGB(bRed, bGreen, bBlue));
        break;
      case 0xFF28D7:  /* Cyan-light*/
        bRed = 127; bGreen = 255; bBlue = 255; bRedStep = 4; bGreenStep = 8; bBlueStep = 8;
        LEDS.showColor(CRGB(bRed, bGreen, bBlue));
        break;
      case 0xFF18E7:  /* Cyan */
        bRed = 0; bGreen = 255; bBlue = 255; bRedStep = 8; bGreenStep = 8; bBlueStep = 8;
        LEDS.showColor(CRGB(bRed, bGreen, bBlue));
        break;
      case 0xFF08F7:  /* Cyan-Dark */
        bRed = 0; bGreen = 191; bBlue = 191; bRedStep = 0; bGreenStep = 8; bBlueStep = 8;
        LEDS.showColor(CRGB(bRed, bGreen, bBlue));
        break;

      case 0xFF609F:  /* OFF */
        LEDS.showColor(CRGB(0, 0, 0));
        bTurnedOn = false;
        break;
      case 0xFF50AF:  /* Blue-dark */
        bRed = 0; bGreen = 0; bBlue = 127; bRedStep = 0; bGreenStep = 0; bBlueStep = 4;
        LEDS.showColor(CRGB(bRed, bGreen, bBlue));
        break;
      case 0xFF708F:  /* Blue */
        bRed = 0; bGreen = 0; bBlue = 255; bRedStep = 8; bGreenStep = 8; bBlueStep = 8;
        LEDS.showColor(CRGB(bRed, bGreen, bBlue));
        break;
      case 0xFF6897:  /* Purple */
        bRed = 127; bGreen = 0; bBlue = 127; bRedStep = 4; bGreenStep = 0; bBlueStep = 4;
        LEDS.showColor(CRGB(bRed, bGreen, bBlue));
        break;
      case 0xFF58A7:  /* Pink */
        bRed = 255; bGreen = 0; bBlue = 255; bRedStep = 8; bGreenStep = 8; bBlueStep = 8;
        LEDS.showColor(CRGB(bRed, bGreen, bBlue));
        break;
      case 0xFF48B7:  /* Pink-Light */
        bRed = 255; bGreen = 63; bBlue = 195; bRedStep = 8; bGreenStep = 2; bBlueStep = 4;
        LEDS.showColor(CRGB(bRed, bGreen, bBlue));
        break;

      case 0xFFE01F:  /* ON */
        LEDS.showColor(CRGB(bRed, bGreen, bBlue));
        bTurnedOn = true;
        break;
      case 0xFFD02F:  /* White */
        bRed = 255; bGreen = 255; bBlue = 255; bRedStep = 8; bGreenStep = 8; bBlueStep = 8;
        LEDS.showColor(CRGB(bRed, bGreen, bBlue));
        break;
      case 0xFFF00F:  /* Flash */
        break;
      case 0xFFE817:  /* Strobe */
        break;
      case 0xFFD827:  /* Fade */
        break;
      case 0xFFC837:  /* Smooth */
        break;
    }

    /* Relay ODROID Commands if configured
      if (RELAY_ODROID_IR == 1){
      switch(uilastir)
      {
        case 0x4DB23BC4:  /* Power Toggle /
        case 0x4DB241BE:  /* Home /
        case 0x4DB253AC:  /* Up /
        case 0x4DB24BB4:  /* Down /
        case 0x4DB29966:  /* Left /
        case 0x4DB2837C:  /* Right /
        case 0x4DB2738C:  /* Ok /
        case 0x4DB259A6:  /* Exit /
        case 0x4DB2A35C:  /* Menu /
          // TODO ReSend to IR transmitter
          break;
      }
      }
    */

    irrecv.resume(); // Receive the next value
  }
  
}

