#define SMALL_FAN_PWM 9
#define SMALL_FAN_DIR 8
#define BIG_FAN_PWM 5
#define BIG_FAN_DIR 4
#define PUMP_PWM 3
#define PUMP_DIR 2
#define FAR 10

//Small fan -> Motor 1
//Big fan -> Motor 2
//Pump -> Motor 3
#include <Adafruit_Sensor.h>
#include "BlueDot_BME280.h"
#include "Adafruit_Si7021.h"
Adafruit_Si7021 sensor = Adafruit_Si7021();
BlueDot_BME280 bme1;                                     //Object for Sensor 1
BlueDot_BME280 bme2;                                     //Object for Sensor 2
int bme1Detected = 0;                                    //Checks if Sensor 1 is available
int bme2Detected = 0;                                    //Checks if Sensor 2 is available

//SETTINGS
boolean csv = true;
boolean constantPrint = false;
boolean autoLevel = true;

int waterLevel = 0; // reading of analog0
String cmd = ""; // Input Command
int val = 0; // Input Value
boolean newInput = false;
int pumpSpeed = 0;
int fan1Speed = 0;
int fan2Speed = 0;
unsigned long startTime = 0;

void setup() {
  Serial.begin(9600);
  while (!Serial);
  Serial.setTimeout(100);
  if (!sensor.begin()) {
    Serial.println("Did not find Si7021 sensor!");
    while (true);
  }
  setupBME();
  pinSetup();
  motorSetup();
  Serial.println("Commands: fan1, fan2, pump, water, display (\"display 1\" -> constantly display)");
  Serial.println("Usage: \"<command> <value>\"");
  setMotor(1, 0);
  setMotor(2, 0);
  Serial.println("Time,Water,Pump,Fan1,Fan2,Si Humidity,BME1 Humidity,BME2 Humidity,Si Temperature,BME1 Temperature,BME2 Temperature");
}

void loop() {
  getInput();
  waterLevel = map(constrain(analogRead(A0), 0, 1024), 0, 1024, 0, 1024);
  if (newInput) {
    if (cmd.equalsIgnoreCase("fan1")) {
      if (!constantPrint) {
        Serial.print("Setting fan1 to ");
        Serial.println(val);
      }
      fan1Speed = constrain(val, 0, 100);
      setMotor(1, fan1Speed);
    } else if (cmd.equalsIgnoreCase("fan2")) {
      if (!constantPrint) {
        Serial.print("Setting fan2 to ");
        Serial.println(val);
      }
      fan2Speed = constrain(val, 0, 100);
      setMotor(2, fan2Speed);
    } else if (cmd.equalsIgnoreCase("pump")) {
      autoLevel = false;
      if (!constantPrint) {
        Serial.print("Setting pump to ");
        Serial.println(val);
      }
      pumpSpeed = constrain(val, -100, 100);
      setMotor(3, pumpSpeed);
    } else if (cmd.equalsIgnoreCase("water")) {
      autoLevel = true;
      if (!constantPrint) {
        Serial.print("Autolevel on");
      }
    } else if (cmd.equalsIgnoreCase("display")) {
      if (val == 1) {
        constantPrint = true;
        startTime = millis();
      } else {
        constantPrint = false;
        printDisplay();
      }
    } else {
      if (!constantPrint) {
        Serial.println("Command not recognized. Usage: \"<command> <value(integer)>\"");
      }
    }
    newInput = false;
  }
  if (autoLevel) {
    balanceWater();
  }
  if (constantPrint) {
    if (csv) {
      printCSV();
      delay(100);
    } else {
      printDisplay();
      delay(500);
    }
    Serial.println("");
  }
}

void pinSetup() {
  pinMode(9, OUTPUT);
  pinMode(8, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(2, OUTPUT);
}

void motorSetup() {
  //If fan not working, switch HIGH/LOW argument
  digitalWrite(SMALL_FAN_DIR, LOW);  //MOTOR 1
  digitalWrite(BIG_FAN_DIR, HIGH); //MOTOR 2
  digitalWrite(PUMP_DIR, HIGH); //MOTOR 3
}

void getInput() {
  while (Serial.available() > 0) {
    String newCmd = Serial.readStringUntil(' ');
    int newVal = Serial.parseInt();
    if (Serial.read() == '\n') {
      String out = "Input: command \'";
      out += newCmd;
      out += "\' with val = \'";
      out += newVal;
      out += "\'";
      if (!constantPrint) {
        Serial.println(out);
      }
      cmd = newCmd;
      val = newVal;
      newInput = true;
      //return true;
    } else {
      Serial.flush();
      if (!constantPrint) {
        Serial.println("No value provided. Default val = 0");
        cmd = newCmd;
        val = 0;
        newInput = true;
      }
      //return false;
    }
  }
}

void balanceWater() {
  if (waterLevel == 0) {
    pumpSpeed = 100;
  } else {
    pumpSpeed = 0;
  }
  setMotor(3, pumpSpeed);
}

void setMotor(int motor, int speed) {
  int target ;
  if (motor == 3) {
    target = PUMP_PWM;
    if (speed < 0) {
      digitalWrite(PUMP_DIR, LOW); //MOTOR 3
    } else {
      digitalWrite(PUMP_DIR, HIGH); //MOTOR 3
    }
    speed = abs(speed);
  } else if (motor == 2) {
    target = BIG_FAN_PWM;
  } else if (motor == 1) {
    target = SMALL_FAN_PWM;
  } else {
    Serial.println("Motor Write Error");
  }
  analogWrite(target, map(speed, 0, 100, 0, 254));
}

void printCSV() {
  Serial.print(millis() - startTime);
  Serial.print(",");
  Serial.print(waterLevel);
  Serial.print(",");
  Serial.print(pumpSpeed);
  Serial.print(",");
  Serial.print(fan1Speed);
  Serial.print(",");
  Serial.print(fan2Speed);
  Serial.print(",");
  Serial.print(sensor.readHumidity(), 2);
  Serial.print(",");
  Serial.print(bme1.readHumidity(), 2);
  Serial.print(",");
  Serial.print(bme2.readHumidity(), 2);
  Serial.print(",");
  Serial.print(sensor.readTemperature(), 2);
  Serial.print(",");
  Serial.print(bme1.readTempC(), 2);
  Serial.print(",");
  Serial.print(bme2.readTempC(), 2);
}

void printDisplay() {
  Serial.print("Water:");
  Serial.print(waterLevel);
  Serial.print("\tPump:");
  Serial.print(pumpSpeed);
  Serial.print("\tFan1:");
  Serial.print(fan1Speed);
  Serial.print("\tFan2:");
  Serial.print(fan2Speed);
  Serial.print("\tSi Hum:");
  Serial.print(sensor.readHumidity(), 2);
  Serial.print("\tBME1 Hum:");
  Serial.print(bme1.readHumidity(), 2);
  Serial.print("\tBME2 Hum:");
  Serial.print(bme2.readHumidity(), 2);
  Serial.print("\tSi Temp:");
  Serial.print(sensor.readTemperature(), 2);
  Serial.print("\tBME1 Temp:");
  Serial.print(bme1.readTempC(), 2);
  Serial.print("\tBME2 Temp:");
  Serial.print(bme2.readTempC(), 2);
}
