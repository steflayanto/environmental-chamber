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

#include "Adafruit_Si7021.h"
Adafruit_Si7021 sensor = Adafruit_Si7021();


boolean constantPrint = false;
boolean newInput = false;
boolean autoLevel = false;
String cmd = ""; // Input Command
int val = 0; // Input Value
float waterLevel = 0; // ~current percentage of sensor covered
int targetLevel = 75;

void setup() {
  Serial.begin(9600);
  while (!Serial);
  Serial.setTimeout(100);
  if (!sensor.begin()) {
    Serial.println("Did not find Si7021 sensor!");
    while (true)
      ;
  }
  pinSetup();
  motorSetup();
  Serial.println("Commands: fan1, fan2, pump, water, display (\"display 1\" -> constantly display)");
  Serial.println("Usage: \"<command> <value>\"");
  setMotor(1, 0);
  setMotor(2, 0);
}

void loop() {
  getInput();
  if (newInput) {
    if (cmd.equalsIgnoreCase("fan1")) {
      Serial.print("Setting fan1 to ");
      Serial.println(val);
      setMotor(1, constrain(val, 0, 100));
    } else if (cmd.equalsIgnoreCase("fan2")) {
      Serial.print("Setting fan2 to ");
      Serial.println(val);
      setMotor(2, constrain(val, 0, 100));
    } else if (cmd.equalsIgnoreCase("pump")) {
      autoLevel = false;
      Serial.print("Setting pump to ");
      Serial.println(val);
      setMotor(3, constrain(val, -100, 100));
    } else if (cmd.equalsIgnoreCase("water")) {
      autoLevel = true;
      Serial.print("Setting target water level to ");
      val = constrain(val, 0, 1024);
      Serial.println(val);
      targetLevel = val;
    } else if (cmd.equalsIgnoreCase("display")) {
      Serial.print("Water level: ");
      Serial.println(waterLevel);
      Serial.print("Target level: ");
      Serial.println(targetLevel);
      if (val == 1) {
        constantPrint = true;
      } else {
        constantPrint = false;
      }
    } else {
      Serial.println("Command not recognized");
    }
    newInput = false;
  }
  if (autoLevel) {
    balanceWater(targetLevel);
  }
  if (constantPrint) {
    Serial.print("Water level: ");
    Serial.print(waterLevel);
    Serial.print("\tTarget level: ");
    Serial.print(targetLevel);
    Serial.print("\tHumidity:    ");
    Serial.print(sensor.readHumidity(), 2);
    Serial.print("\tTemperature: ");
    Serial.println(sensor.readTemperature(), 2);
    delay(500);
  }
  waterLevel = map(constrain(analogRead(A0), 0, 1024), 0, 1024, 0, 1024);
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
      Serial.println(out);
      cmd = newCmd;
      val = newVal;
      newInput = true;
      return true;
    } else {
      Serial.flush();
      Serial.println("Invalid Input. Usage: \"<command> <value(integer)>\"");
      return false;
    }
  }
}

void balanceWater(int targetLevel) {
  float target = (float) targetLevel;
  int pumpSpeed ;
  if (waterLevel < target) {
    if (abs(waterLevel - target) > FAR) {
      //Serial.println(waterLevel);
      pumpSpeed = 100;
    } else {
      //Serial.println(waterLevel);
      pumpSpeed = 75;
    }
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
