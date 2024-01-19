#include <Wire.h>
#include "Arduino.h"
//#include "uRTCLib.h"
#include <RTClib.h>
#include "uEEPROMLib.h"

#include <SparkFun_PHT_MS8607_Arduino_Library.h> // Click here to get the library: http://librarymanager/All#SparkFun_PHT_MS8607

#include <AltSoftSerial.h>

AltSoftSerial softSerial;
MS8607 barometricSensor;

// uRTCLib rtc(0x68);      // Create objects and assign module I2c Addresses
RTC_DS3231 rtc;
uEEPROMLib eeprom(0x57);
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
  float compensated_RH;
  float dew_point;

void setup(void)
{
  
  Wire.begin();
  rtc.begin();
  rtc.adjust(DateTime(F(__DATE__),F(__TIME__)));
  
  Serial.begin(9600);             //Set serial baud rate to 9600
  softSerial.begin(9600);

  Serial.begin(9600);
  Serial.println("Qwiic PHT Sensor MS8607 Example");
  Wire.begin();

//if barometer could not be opened
  if (barometricSensor.begin() == false)
  {
    Serial.println("MS8607 sensor did not respond. Trying again...");
    if (barometricSensor.begin() == false)
    {
      Serial.println("MS8607 sensor did not respond. Please check wiring.");
      while (1)
        ;
    }
  }

  int err = barometricSensor.set_humidity_resolution(MS8607_humidity_resolution_12b); // 12 bits
  if (err != MS8607_status_ok)
  {
    Serial.print("Problem setting the MS8607 sensor humidity resolution. Error code = ");
    Serial.println(err);
    Serial.println("Freezing.");
    while (1)
      ;
  }

  err = barometricSensor.disable_heater();
  if (err != MS8607_status_ok)
  {
    Serial.print("Problem disabling the MS8607 humidity sensor heater. Error code = ");
    Serial.println(err);
    Serial.println("Freezing.");
    while (1)
      ;
  }
}
void loop(void)
{
  int count = 0;
  
  addr_EEPROM = 0;

  //size of the data
  int f_size = 4; //float has four bytes
  int i_size = 1;
  int year_size = 2;
  bool done = false;

  softSerial.println("Writing data");
  for(int i = 0; i < 10; i ++){

    delay(1000);
    DateTime now=rtc.now();
    month = now.month();
    day = now.day();
    year = now.year(); //returns uint_16t
    hour = now.hour();
    minute = now.minute();
    second = now.second();
    temperature = barometricSensor.getTemperature(); 
    pressure = barometricSensor.getPressure();
    humidity = barometricSensor.getHumidity();

//calculating compensated Humidity
   int err = barometricSensor.get_compensated_humidity(temperature, humidity, &compensated_RH);
  if (err != MS8607_status_ok)
  {
    Serial.println();
    Serial.print("Problem getting the MS8607 compensated humidity. Error code = ");
    Serial.println(err);
    return;
  }
 //calculating dew_point
  err = barometricSensor.get_dew_point(temperature, humidity, &dew_point);
  if (err != MS8607_status_ok)
  {
    Serial.println();
    Serial.print("Problem getting the MS8607 dew point. Error code = ");
    Serial.println(err);
    return;
  }

     softSerial.print("Date: ");
     softSerial.print(month);  softSerial.print('/');
     softSerial.print(day);    softSerial.print('/');
     softSerial.print(year);

      softSerial.print("  Time: ");
  softSerial.print(hour);   softSerial.print(':');
  softSerial.print(minute);   softSerial.print(':');
  softSerial.print(second);   softSerial.print(" ");

 softSerial.print("Temperature= ");
  softSerial.print(temperature); //the numbers are the amount of decimal places
  softSerial.print("C, ");

  softSerial.print("Pressure= ");
  pressure *= 0.1; // pressure reads in hPa, 1hPa = 0.1kPa
  softSerial.print(pressure);
  softSerial.print("kPa, "); 

  softSerial.print("Humidity= ");
  softSerial.print(humidity);
  softSerial.print("%RH, ");

  softSerial.print("Compensated humidity= ");
  softSerial.print(compensated_RH);
  softSerial.print("%RH ");

  softSerial.print("Dew point= ");
  softSerial.print(dew_point);
  softSerial.println("C");

    eeprom.eeprom_write(addr_EEPROM, month); addr_EEPROM += i_size;
    eeprom.eeprom_write(addr_EEPROM, day); addr_EEPROM += i_size;
    eeprom.eeprom_write(addr_EEPROM, year); addr_EEPROM += year_size;
    eeprom.eeprom_write(addr_EEPROM, hour); addr_EEPROM += i_size;
    eeprom.eeprom_write(addr_EEPROM, minute); addr_EEPROM += i_size;
    eeprom.eeprom_write(addr_EEPROM, second); addr_EEPROM += i_size;

    eeprom.eeprom_write(addr_EEPROM, temperature); addr_EEPROM += f_size;
    eeprom.eeprom_write(addr_EEPROM, pressure); addr_EEPROM += f_size;
    eeprom.eeprom_write(addr_EEPROM, humidity); addr_EEPROM += f_size;
    eeprom.eeprom_write(addr_EEPROM, compensated_RH); addr_EEPROM += f_size;
    eeprom.eeprom_write(addr_EEPROM, dew_point); addr_EEPROM += f_size;
    
   
    
  }

  //now writing is complete

  //reset eeprom
  addr_EEPROM = 0;
  softSerial.println("READING");
  uint8_t read_month;
  uint8_t read_day;
  uint16_t read_year; 
  uint8_t read_hour;
  uint8_t read_minute;
  uint8_t read_second;

  
  float read_humidity;
  float read_compensated_RH;
  float read_dew_point;
  float read_temperature;
  float read_pressure;

  for(int i = 0; i < 10; i++){
    int loop_counter= 0;
    delay(1000);
      //now read from the eeprom
   
    eeprom.eeprom_read(addr_EEPROM, &read_month); addr_EEPROM += i_size;
    eeprom.eeprom_read(addr_EEPROM, &read_day);   addr_EEPROM += i_size;
    eeprom.eeprom_read(addr_EEPROM, &read_year);  addr_EEPROM += year_size;
    eeprom.eeprom_read(addr_EEPROM, &read_hour);  addr_EEPROM += i_size;
    eeprom.eeprom_read(addr_EEPROM, &read_minute);  addr_EEPROM += i_size;
    eeprom.eeprom_read(addr_EEPROM, &read_second);  addr_EEPROM += i_size;

    eeprom.eeprom_read(addr_EEPROM, &read_temperature);  addr_EEPROM += f_size;
    eeprom.eeprom_read(addr_EEPROM, &read_pressure);  addr_EEPROM += f_size;
    eeprom.eeprom_read(addr_EEPROM, &read_humidity);  addr_EEPROM += f_size;
    eeprom.eeprom_read(addr_EEPROM, &read_compensated_RH);  addr_EEPROM += f_size;
    eeprom.eeprom_read(addr_EEPROM, &read_dew_point);  addr_EEPROM += f_size; 

    softSerial.print("Date: ");
     softSerial.print(read_month);  softSerial.print('/');
     softSerial.print(read_day);    softSerial.print('/');
     softSerial.print(read_year);

      softSerial.print("  Time: ");
  softSerial.print(read_hour);   softSerial.print(':');
  softSerial.print(read_minute);   softSerial.print(':');
  softSerial.print(read_second);   softSerial.print(" ");

 softSerial.print("Temperature= ");
  softSerial.print(read_temperature);
  softSerial.print("C, ");

  softSerial.print("Pressure= ");
  //pressure *= 0.1; // pressure reads in hPa, 1hPa = 0.1kPa
  softSerial.print(read_pressure);
  softSerial.print("kPa, "); 

  softSerial.print("Humidity= ");
  softSerial.print(read_humidity);
  softSerial.print("%RH, ");

  softSerial.print("Compensated humidity= ");
  softSerial.print(read_compensated_RH);
  softSerial.print("%RH ");

  softSerial.print("Dew point= ");
  softSerial.print(read_dew_point);
  softSerial.println("C");
    done = true;

  }


  //only read 10 data points for now
  if (done) {
    done = false;

    softSerial.println("Clearing the entire EEPROM.  This will take a few moments");
    for (addr_EEPROM = 0; addr_EEPROM < 13; addr_EEPROM++) {
      eeprom.eeprom_write(addr_EEPROM, (unsigned char) (0));
      if (int i = addr_EEPROM % 100 == 0) softSerial.print("."); // Prints a '.' every 100 writes to EEPROM
    }
    softSerial.println();
    softSerial.println("Memory Erase Complete");
    while (1); //Stops further execution of the program
  }
}