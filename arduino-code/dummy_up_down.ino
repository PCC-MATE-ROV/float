#include "Arduino.h"
// #include "uRTCLib.h"
#include <RTClib.h>
#include "uEEPROMLib.h"

#include <SparkFun_PHT_MS8607_Arduino_Library.h> // Click here to get the library: http://librarymanager/All#SparkFun_PHT_MS8607

#include <AltSoftSerial.h>

#include "Wire.h" //allows communication over i2c devices
// #include "LiquidCrystal_I2C.h" //allows interfacing with LCD screens

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif

#define LED_PIN 11
#define BUTTON_PIN 4
bool button_pressed = false;

AltSoftSerial softSerial;

int val = 0;
const int pump = 5;
const int valve = 6;
Adafruit_NeoPixel strip = Adafruit_NeoPixel(19, LED_PIN, NEO_GRB + NEO_KHZ800);

void colorWipe(uint32_t c, uint8_t wait)
{
  for (uint16_t i = 0; i < strip.numPixels(); i++)
  {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}

void theaterChase(uint32_t c, uint8_t wait)
{
  for (int j = 0; j < 10; j++)
  { // do 10 cycles of chasing
    for (int q = 0; q < 3; q++)
    {
      for (int i = 0; i < strip.numPixels(); i = i + 3)
      {
        strip.setPixelColor(i + q, c); // turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (int i = 0; i < strip.numPixels(); i = i + 3)
      {
        strip.setPixelColor(i + q, 0); // turn every third pixel off
      }
    }
  }
}

void setup(void)
{
  Wire.begin();
  Serial.begin(9600); // Set serial baud rate to 9600
  softSerial.begin(9600);
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  pinMode(pump,OUTPUT);
  pinMode(valve,OUTPUT);
  // Serial.println(init_pressure);
  softSerial.println("Program has started. The float will stay above water before the rovermoves it into position.");
  theaterChase(strip.Color(0, 255, 0), 50); // Green
  digitalWrite(pump, HIGH);              // Turn the pump on
  digitalWrite(valve, HIGH);             // Turn the valve on
  delay(9000); // Wait 1 second
  digitalWrite(pump,LOW);
}
void loop()
{
  val = softSerial.read();
  if(val!=-1)
  {
    if(val=='D')
    {
      softSerial.println("   Float's first descent...");
      theaterChase(strip.Color(255, 0, 0), 50); // RED
      digitalWrite(pump, LOW);               // Turn the pump off
      digitalWrite(valve, LOW);              // Turn the valve off
      delay(30000);                           // Wait 30 second
      digitalWrite(pump, HIGH);               // Turn the pump off
      digitalWrite(valve, HIGH);              // Turn the valve off
      theaterChase(strip.Color(0, 0, 255), 50); // BLUE
      softSerial.println("   Float's ascend...");
      delay(20000);
      theaterChase(strip.Color(0, 255, 0), 50); // BLUE
      digitalWrite(pump, LOW);               // Turn the pump off
      softSerial.write("a");
      val = -1;
    }
  }
}