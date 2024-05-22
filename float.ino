// #include <Wire.h>
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

const int pressureInput = A3;            // select the analog input pin for the pressure transducer
float pressureZero = 90.5;                // analog reading of pressure transducer at 0psi
float pressureMax = 920.7;                 // analog reading of pressure transducer at 30psi
const int pressuretransducermaxPSI = 30; // psi value of transducer being used
const int baudRate = 9600;               // constant integer to set the baud rate for serial monitor
const int sensorreadDelay = 250;         // constant integer to set the sensor read delay in milliseconds

// Pump & valve pin numbers
// Change value  s according to available pins on the circuit
const int pump = 5;
const int valve = 6;

bool start = false;
bool descend = false;
bool ascend = false;
bool written = false;
bool bottom = false;
bool transmit = false;

// float pressureValue = 0; //variable to store the value coming from the pressure transducer
AltSoftSerial softSerial;
MS8607 barometricSensor;

// uRTCLib rtc(0x68);      // Create objects and assign module I2c Addresses
RTC_DS3231 rtc;
uEEPROMLib eeprom(0x57);
int val = 0;
float init_pressure;
unsigned int addr_EEPROM;
uint8_t month;
uint8_t day;
uint16_t year;
uint8_t hour;
uint8_t minute;
uint8_t second;

float temperature;
float humidity;
float pressure;
// float compensated_RH;
// float dew_point;
float outer_pressure = 0;
float analog_pressure =0;

uint8_t read_month;
uint8_t read_day;
uint16_t read_year;
uint8_t read_hour;
uint8_t read_minute;
uint8_t read_second;
float read_humidity;
// float read_compensated_RH;
// float read_dew_point;
float read_temperature;
float read_pressure;
float read_outer_pressure = 0; // Stores the value of external pressure

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
  rtc.begin();
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  Serial.begin(9600); // Set serial baud rate to 9600

  softSerial.begin(9600);

  softSerial.println(" Qwiic PHT Sensor MS8607 Example");

  // if barometer could not be opened
  if (barometricSensor.begin() == false)
  {
    softSerial.println("MS8607 sensor did not respond. Trying again...");
    if (barometricSensor.begin() == false)
    {
      softSerial.println("MS8607 sensor did not respond. Please check wiring.");
      while (1)
        ;
    }
  }

  int err = barometricSensor.set_humidity_resolution(MS8607_humidity_resolution_12b); // 12 bits
  if (err != MS8607_status_ok)
  {
    softSerial.print("Problem setting the MS8607 sensor humidity resolution. Error code = ");
    softSerial.println(err);
    softSerial.println("Freezing.");
    while (1)
      ;
  }

  err = barometricSensor.disable_heater();
  if (err != MS8607_status_ok)
  {
    softSerial.print("Problem disabling the MS8607 humidity sensor heater. Error code = ");
    softSerial.println(err);
    softSerial.println("Freezing.");
    while (1)
      ;
  }

// This is for Trinket 5V 16MHz, you can remove these three lines if you are not using a Trinket
#if defined(__AVR_ATtiny85__)
  if (F_CPU == 16000000)
    clock_prescale_set(clock_div_1);
#endif
  // End of trinket special code

  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  // for the float to stay above water before
  init_pressure = barometricSensor.getPressure() / 10; // assume to be pressure above water divide by 10 to covert hpa to kpa

  // softSerial.println(pressureZero);
  // softSerial.println(pressureMax);
  pinMode(pump, OUTPUT);
  pinMode(valve, OUTPUT);
  // Serial.println(init_pressure);
  softSerial.println("Program has started. The float will stay above water before the rover moves it into position.");
  colorWipe(strip.Color(0, 255, 0), 50); // Green
  digitalWrite(pump, HIGH);              // Turn the pump on
  digitalWrite(valve, HIGH);             // Turn the valve on
  start = true;
  delay(9000);
  digitalWrite(pump,LOW); // Wait 1 second
}

void loop()
{
  if (start)
  {
    float previous_outer_pressure;
    // Ascend = A, Descend = D
    val = softSerial.read(); // the command the float will receive from the PC
    int counter = 0;
    // softSerial.println("Writing data");
    // bool currentState = digitalRead(BUTTON_PIN);
    if (val != -1)
    {
      if (val == 'D')
      {
        softSerial.println("   Float's first descent...");
        colorWipe(strip.Color(255, 0, 0), 50); // RED
        digitalWrite(pump, LOW);               // Turn the pump off
        digitalWrite(valve, LOW);              // Turn the valve off
        delay(1000);                           // Wait 1 second

        addr_EEPROM = 0;

        // size of the data
        int f_size = 4; // float has four bytes
        int i_size = 1;
        int year_size = 2;
        bool done = false;
        descend = true;

        if (descend)
        {
          softSerial.println("Writing data while the float is decending");
          do
          {
            counter++;   // this counter will count how many times the collected data
            delay(1000); // will be 5000 for the finalize code
            softSerial.println("inside do while loop now");
            previous_outer_pressure = outer_pressure;
            DateTime now = rtc.now();
            month = now.month();
            day = now.day();
            year = now.year(); // returns uint_16t
            hour = now.hour();
            minute = now.minute();
            second = now.second();
            temperature = barometricSensor.getTemperature();
            pressure = barometricSensor.getPressure() / 10; // the sensor give pressure in hpa so we divide by 10 to get kpa
            humidity = barometricSensor.getHumidity();
            analog_pressure = analogRead(pressureInput);
            //float voltage = analog_pressure *(5.0/1023.0);


            outer_pressure = (analog_pressure - pressureZero) * (pressuretransducermaxPSI / (pressureMax - pressureZero));
            outer_pressure = (outer_pressure * 6.89476) + pressure; // converting unit from PSI to Kpa

            eeprom.eeprom_write(addr_EEPROM, month);
            addr_EEPROM += i_size;
            eeprom.eeprom_write(addr_EEPROM, day);
            addr_EEPROM += i_size;
            eeprom.eeprom_write(addr_EEPROM, year);
            addr_EEPROM += year_size;
            eeprom.eeprom_write(addr_EEPROM, hour);
            addr_EEPROM += i_size;
            eeprom.eeprom_write(addr_EEPROM, minute);
            addr_EEPROM += i_size;
            eeprom.eeprom_write(addr_EEPROM, second);
            addr_EEPROM += i_size;

            eeprom.eeprom_write(addr_EEPROM, temperature);
            addr_EEPROM += f_size;
            eeprom.eeprom_write(addr_EEPROM, pressure);
            addr_EEPROM += f_size;
            eeprom.eeprom_write(addr_EEPROM, humidity);
            addr_EEPROM += f_size;
            // eeprom.eeprom_write(addr_EEPROM, compensated_RH); addr_EEPROM += f_size;
            // eeprom.eeprom_write(addr_EEPROM, dew_point); addr_EEPROM += f_size;
            eeprom.eeprom_write(addr_EEPROM, outer_pressure);
            addr_EEPROM += f_size;

            softSerial.print("Analog Pressure= ");
            softSerial.print(analogRead(pressureInput));
            softSerial.println(" Kpa");
            softSerial.print("Outer Pressure= ");
            softSerial.print(outer_pressure);
            softSerial.println(" Kpa");

            softSerial.print("init Pressure= ");
            softSerial.print(init_pressure);
             softSerial.print("inside Pressure= ");
            softSerial.print(pressure);
            softSerial.println(" Kpa");
            if (abs(outer_pressure - previous_outer_pressure) <= 1.0)
            {
              written = true;
              bottom = true;
              softSerial.println("Reached the bottom...");
              softSerial.println(" Float's preparing to ascend...");
              digitalWrite(pump, HIGH);  // Turn the pump off
              digitalWrite(valve, HIGH);
              delay(9000);
              digitalWrite(pump, LOW); // Turn the valve off
              ascend = true;
              colorWipe(strip.Color(0, 0, 255), 50); // Blue)
            }
          } while (outer_pressure - init_pressure <= 0.5); // this condition makes the float to collect data until it is above water
          descend = false;
          bottom = false;
          softSerial.println("outside do while loop now");
        }

        if ((outer_pressure - init_pressure <= 0.5 || outer_pressure >= init_pressure) && ascend && written && !descend && !bottom) // this means it is above water, we can transmit data
        {
          // reset eeprom
          addr_EEPROM = 0;
          softSerial.println(" Data Transmiting: ");
          colorWipe(strip.Color(0, 255, 0), 50); // green
          for (int i = 0; i < counter; i++)
          {
            eeprom.eeprom_read(addr_EEPROM, &read_month);
            addr_EEPROM += i_size;
            eeprom.eeprom_read(addr_EEPROM, &read_day);
            addr_EEPROM += i_size;
            eeprom.eeprom_read(addr_EEPROM, &read_year);
            addr_EEPROM += year_size;
            eeprom.eeprom_read(addr_EEPROM, &read_hour);
            addr_EEPROM += i_size;
            eeprom.eeprom_read(addr_EEPROM, &read_minute);
            addr_EEPROM += i_size;
            eeprom.eeprom_read(addr_EEPROM, &read_second);
            addr_EEPROM += i_size;

            eeprom.eeprom_read(addr_EEPROM, &read_temperature);
            addr_EEPROM += f_size;
            eeprom.eeprom_read(addr_EEPROM, &read_pressure);
            addr_EEPROM += f_size;
            eeprom.eeprom_read(addr_EEPROM, &read_humidity);
            addr_EEPROM += f_size;
            // eeprom.eeprom_read(addr_EEPROM, &read_compensated_RH);  addr_EEPROM += f_size;
            // eeprom.eeprom_read(addr_EEPROM, &read_dew_point);  addr_EEPROM += f_size;
            eeprom.eeprom_read(addr_EEPROM, &read_outer_pressure);
            addr_EEPROM += f_size;

            softSerial.print("Date: ");
            softSerial.print(read_month);
            softSerial.print('/');
            softSerial.print(read_day);
            softSerial.print('/');
            softSerial.print(read_year);

            softSerial.print("  Time: ");
            softSerial.print(read_hour);
            softSerial.print(':');
            softSerial.print(read_minute);
            softSerial.print(':');
            softSerial.print(read_second);
            softSerial.print(" ");

            softSerial.print("Temperature= ");
            softSerial.print(read_temperature);
            softSerial.print("C, ");

            softSerial.print("Pressure= ");
            // pressure *= 0.1; // pressure reads in hPa, 1hPa = 0.1kPa
            softSerial.print(read_pressure);
            softSerial.print("kPa, ");

            softSerial.print("Humidity= ");
            softSerial.print(read_humidity);
            softSerial.print("%RH, ");

            softSerial.print("Outer Pressure= ");
            softSerial.print(read_outer_pressure);
            softSerial.println(" Kpa");

            transmit = true;
          }

          if (ascend && transmit)
          {
            softSerial.println("Clearing the entire EEPROM.  This will take a few moments");
            for (addr_EEPROM = 0; addr_EEPROM < counter; addr_EEPROM++)
            {
              eeprom.eeprom_write(addr_EEPROM, (unsigned char)(0));
              if (int i = addr_EEPROM % 100 == 0)
                softSerial.print("."); // Prints a '.' every 100 writes to EEPROM
            }
            softSerial.println();
            softSerial.println("Memory Erase Complete");
            written = false;
            ascend = false;
            // while (1); //Stops further execution of the program
            softSerial.println("Finished sending data. Ready to Decend Again.");
            softSerial.println("Finished sending data. Ready to Decend Again. Press D to Descend.");
            val = -1;
          }
        }
      }
    }
  }
  else
    softSerial.println(" Start failed..");
}
