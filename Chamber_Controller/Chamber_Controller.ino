#define SMALL_FAN_PWM 9
#define SMALL_FAN_DIR 8
#define BIG_FAN_PWM 5
#define BIG_FAN_DIR 4
#define PUMP_PWM 3
#define PUMP_DIR 2
#define HUM_PWM 10
#define HUM_DIR 12
#define MAX_HUM 97   //Humidity not allowed to pass this value
#define INTERVAL 500 //Time in milliseconds between print statements (1000ms -> 1 Hz)

//Small fan -> Motor 1
//Big fan -> Motor 2
//Pump -> Motor 3
#include <Adafruit_Sensor.h>
#include "BlueDot_BME280.h"
BlueDot_BME280 bme1;                                     //Object for Sensor 1
BlueDot_BME280 bme2;                                     //Object for Sensor 2
int bme1Detected = 0;                                    //Checks if Sensor 1 is available
int bme2Detected = 0;                                    //Checks if Sensor 2 is available

//SETTINGS
boolean csv = true;
boolean constantPrint = false;
boolean autoLevel = true;
boolean autoWetHum = false;
boolean autoDryHum = false;

int waterLevel = 0; // reading of analog0
float wetHum = 0.0;
float dryHum = 0.0;

String cmd = ""; // Input Command
int val = 0; // Input Value
boolean newInput = false;
float wetHumSetpoint = 0.0;
float dryHumSetpoint = 0.0;
int pumpSpeed = 0;
int fan1Speed = 0;
int fan2Speed = 0;
int humSpeed = 0;
unsigned long startTime = 0;
unsigned long printTimer = 0;

void setup() {
  Serial.begin(9600);
  while (!Serial);
  Serial.setTimeout(100);
  setupBME();
  pinSetup();
  motorSetup();
  Serial.println("Commands: fan1 (small fan), fan2 (big fan), pump, hum (auto-control humidity), water (auto-control water), display (\"display 1\" -> constantly display)");
  Serial.println("Usage: \"<command> <value>\"");
  setMotor(1, 0);
  setMotor(2, 0);
  digitalWrite(HUM_DIR, HIGH);
}

void loop() {
  getInput();
  waterLevel = map(constrain(analogRead(A0), 0, 1024), 0, 1024, 0, 1024);
  wetHum = bme2.readHumidity();
  dryHum = bme1.readHumidity();
  if (newInput) {
    if (cmd.equalsIgnoreCase("fan1")) {
      autoWetHum = false;
      autoDryHum = false;
      if (!constantPrint) {
        Serial.print("Setting fan1 to ");
        Serial.println(val);
      }
      fan1Speed = constrain(val, 0, 100);
    } else if (cmd.equalsIgnoreCase("fan2")) {
      if (!constantPrint) {
        Serial.print("Setting fan2 to ");
        Serial.println(val);
      }
      fan2Speed = constrain(val, 0, 100);
    } else if (cmd.equalsIgnoreCase("pump")) {
      autoLevel = false;
      if (!constantPrint) {
        Serial.print("Setting pump to ");
        Serial.println(val);
      }
      pumpSpeed = constrain(val, -100, 100);
    } else if (cmd.equalsIgnoreCase("hum")) {
      humSpeed = constrain(val, 0, 100);
      autoWetHum = false;
      autoDryHum = false;
      if (!constantPrint) {
        Serial.print("Setting humidifier to ");
        Serial.println(humSpeed);
      }
    } else if (cmd.equalsIgnoreCase("water")) {
      autoLevel = true;
      if (!constantPrint) {
        Serial.print("Autolevel on");
      }
    } else if (cmd.equalsIgnoreCase("wet")) {
      autoWetHum = true;
      autoDryHum = false;
      wetHumSetpoint = constrain(val, 0, MAX_HUM);
      if (!constantPrint) {
        Serial.print("Auto wet humidity set to: ");
        Serial.print(wetHumSetpoint);
        Serial.print("%");
      }
    } else if (cmd.equalsIgnoreCase("dry")) {
      autoWetHum = true;
      autoDryHum = true;
      dryHumSetpoint = constrain(val, 0, MAX_HUM);
      if (!constantPrint) {
        Serial.print("Auto dry humidity set to: ");
        Serial.print(dryHumSetpoint);
        Serial.print("%");
      }
    } else if (cmd.equalsIgnoreCase("test")) {
      Serial.println("Testing Fan1...");
      setMotor(1, 100);
      delay(1000);
      setMotor(1, 0);
      Serial.println("Testing Fan2...");
      setMotor(2, 100);
      delay(1000);
      setMotor(2, 0);
      Serial.println("Testing Pump..");
      setMotor(3, 100);
      delay(500);
      setMotor(3, -100);
      delay(500);
      setMotor(3, 0);
      Serial.println("Testing Humidifier...");
      for (int i = 0; i < 3; i++) {
        setMotor(4, 100);
        delay(50);
        setMotor(4, 0);
        delay(250);
      }
      Serial.println("Finished Diagnostics.");
    } else if (cmd.equalsIgnoreCase("display")) {
      if (val == 1) {
        constantPrint = true;
        startTime = millis();
        Serial.println("Time,Hum,Water,Pump,SmallFan,BigFan,Wet Humidity,Dry Humidity,Wet Temperature,Dry Temperature,Wet Pressure,Dry Pressure");
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
  if (autoDryHum) {
    setDryHumidity();
  }
  if (autoWetHum) {
    setWetHumidity();
  }
  
  if (constantPrint && millis() - printTimer > INTERVAL) {
    printTimer = millis();
    if (csv) {
      printCSV();
    } else {
      printDisplay();
    }
    Serial.println("");
  }
  writeMotors();
}

void writeMotors() {
  setMotor(1, fan1Speed);
  setMotor(2, fan2Speed);
  setMotor(3, pumpSpeed);
  setMotor(4, humSpeed);
}

void pinSetup() {
  pinMode(HUM_DIR, OUTPUT);
  pinMode(HUM_PWM, OUTPUT);
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
      val = constrain(newVal, -100, 100);
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
    }
  }
}

void setWetHumidity() {
  if (wetHumSetpoint - wetHum < 0.1) { // at setpoint
    humSpeed = 0;
    fan1Speed = 0;
  } else if (wetHumSetpoint - wetHum < 1.0) { //approaching setpoint
    humSpeed = 0;
    fan1Speed = 50;
  } else if (wetHumSetpoint - wetHum < 5) { //approaching setpoint
    humSpeed = 0;
    fan1Speed = 100;
  } else { //far away
    humSpeed = 75;
    fan1Speed = 100;
  }
}

void setDryHumidity() {
  if (dryHumSetpoint - dryHum < 0.1) { // at setpoint or beyond
    wetHumSetpoint = dryHumSetpoint + 5;
    fan2Speed = 15;
  } else if (dryHumSetpoint - dryHum < 3.0) { //approaching setpoint
    wetHumSetpoint = dryHumSetpoint + 7;
    fan2Speed = 50;
  } else { //far away
    wetHumSetpoint = dryHumSetpoint + 12;
    fan2Speed = 100;
  }
//  Serial.println("Trying to set dry hum");
  wetHumSetpoint = constrain(wetHumSetpoint,0,MAX_HUM);
}

void balanceWater() {
  if (waterLevel == 0) {
    pumpSpeed = 100;
  } else {
    pumpSpeed = 0;
  }
}

void setMotor(int motor, int speed) {
  int target ;
  if (motor == 4) {
    target = HUM_PWM;
    speed = abs(speed);
  } else if (motor == 3) {
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
  //  Serial.print(wetHum);
  //  return;
  Serial.print(millis() - startTime);
  Serial.print(",");
  Serial.print(humSpeed);
  Serial.print(",");
  Serial.print(waterLevel);
  Serial.print(",");
  Serial.print(pumpSpeed);
  Serial.print(",");
  Serial.print(fan1Speed);
  Serial.print(",");
  Serial.print(fan2Speed);
  Serial.print(",");
  Serial.print(wetHum, 2);
  Serial.print(",");
  Serial.print(dryHum, 2);
  Serial.print(",");
  Serial.print(bme2.readTempC(), 2);
  Serial.print(",");
  Serial.print(bme1.readTempC(), 2);
  Serial.print(",");
  Serial.print(bme2.readPressure());
  Serial.print(",");
  Serial.print(bme1.readPressure());
}

void printDisplay() {
  Serial.print("Hum:");
  Serial.print(humSpeed);
  Serial.print("Water:");
  Serial.print(waterLevel);
  Serial.print("\tPump:");
  Serial.print(pumpSpeed);
  Serial.print("\tSmallFan:");
  Serial.print(fan1Speed);
  Serial.print("\tBigFan:");
  Serial.print(fan2Speed);
  Serial.print("\tWet Hum:");
  Serial.print(wetHum, 2);
  Serial.print("\tDry Hum:");
  Serial.print(dryHum, 2);
  Serial.print("\tWet Temp:");
  Serial.print(bme2.readTempC(), 2);
  Serial.print("\tDry Temp:");
  Serial.print(bme1.readTempC(), 2);
  Serial.print("\tWet Pressure:");
  Serial.print(bme2.readPressure());
  Serial.print("\tDry Pressure:");
  Serial.print(bme1.readPressure());
}
