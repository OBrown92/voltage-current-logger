#include <Wire.h>
#include <INA226_WE.h>
#include <SPI.h>
#include <SD.h>
#include <LiquidCrystal.h>
#include <TimeLib.h>
#define I2C_ADDRESS 0x40

INA226_WE ina226(I2C_ADDRESS);
LiquidCrystal lcd(9, 10, 5, 6, 7, 8);
File logFile;

const int chipSelect = 4;
String filename = "log";
unsigned long previousLogMillis = 0;
unsigned long previousLcdMillis = 0;
const long logInterval = 10000;
const long lcdInterval = 500;
unsigned long savedSoFar = 0;
String dataString = "";

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  Wire.begin();
  ina226.init();
  lcd.begin(16, 2);
  lcd.print("Initialisieren");

  while (!Serial) {
    ; // wait for serial port
  }
  Serial.println("Begin initialization");

  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    while (1);
  }

  // wait for ina226 to be ready and set it up
  ina226.setAverage(AVERAGE_16);
  ina226.setMeasureMode(CONTINUOUS);
  ina226.setCorrectionFactor(0.9);

  // create Textfile with random ending
  randomSeed(analogRead(0));
  filename += String(random(0, 9999));
  filename += ".txt";

  logFile = SD.open(filename, FILE_WRITE);
  if (logFile) {
    Serial.println("File created: " + filename);  
  }
  logFile.close();
  

  Serial.println("Initialization completed!");
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousLcdMillis >= lcdInterval) {
    previousLcdMillis = currentMillis;
    unsigned long currentSeconds = currentMillis / 1000;

    float shuntVoltage_mV = 0.0;
    float loadVoltage_V = 0.0;
    float busVoltage_V = 0.0;
    float current_mA = 0.0;
    float power_mW = 0.0;

    ina226.readAndClearFlags();
    shuntVoltage_mV = ina226.getShuntVoltage_mV();
    busVoltage_V = ina226.getBusVoltage_V();
    current_mA = ina226.getCurrent_mA();
    power_mW = ina226.getBusPower();
    loadVoltage_V  = busVoltage_V + (shuntVoltage_mV / 1000);

    dataString = "";
    dataString += String(currentSeconds) + ",";
    dataString += String(shuntVoltage_mV) + ",";
    dataString += String(busVoltage_V) + ",";
    dataString += String(current_mA) + ",";
    dataString += String(power_mW) + ",";
    dataString += String(loadVoltage_V);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("U:" + String(busVoltage_V) + " " + "I:" + String(current_mA));
    lcd.setCursor(0, 1);
    lcd.print(formatDigits(hour(currentSeconds))
      + ":" + formatDigits(minute(currentSeconds))
      + ":" + formatDigits(second(currentSeconds)));
    lcd.setCursor(10, 1);
    lcd.print(savedSoFar);
  }
  if (currentMillis - previousLogMillis >= logInterval) {
    previousLogMillis = currentMillis;
    Serial.print("Try to write ");
    Serial.println(dataString);
    File dataFile = SD.open(filename, FILE_WRITE);

    // if the file is available, write to it:
    if (dataFile) {
      dataFile.println(dataString);
      dataFile.close();
      savedSoFar++;
      Serial.println("Success");
    }
    // if the file isn't open, pop up an error:
    else {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("SD Fehler");
      Serial.print("error opening ");
      Serial.println(filename);
    }
  }
}

String formatDigits(int digit) {
  if (digit < 10) {
    return 0 + String(digit);
  }
  return String(digit);
}
