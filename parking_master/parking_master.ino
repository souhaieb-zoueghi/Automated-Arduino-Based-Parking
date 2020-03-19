#include <Ultrasonic.h>
#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <RTClib.h>
#include <SD.h>

//temperature sensor DS1621 specifications
#define DS1621_ADDR 0x48
#define DS1621_ADDR_CONF 0xAC
#define DS1621_CONF_TEMP_MODE 0x02
#define DS1621_BEGIN_SENSING 0xEE
#define DS1621_TEMP_REGISTRE 0xAA

//LCD displayer specifications
#define LCD_ADDR 0x3F
#define LCD_COLUMNS 16
#define LCD_LINES 2

#define ARDUINO_SLAVE_ADDR 8
#define PARKING_COST 2
#define CLOSE_BARRIER 90
#define OPEN_BARRIER 0
#define PARKING_MAX_CAPACITY 7
#define STATISTICS_SIZE 64
#define MAXIMUM_DISTANCE_TO_OPEN_BARRIER 9
#define WAITING_TIME 4

typedef enum {PARKING_FULL = 0, PARKING_AVAILABLE, DISPLAY_TIME_TEMP, VERIFY_ENTRANCE, VERIFY_PARKING_STATUS, SAVE_EARNINGS}ParkingStatus; 

//get number of cars that got out of the parking from the slave to calculate the right current cars number in the parking 
int getNumberOfCarsOut();

//display temperature on the LCD
void displayTemperature(float);

//get current temperature from the DS1621
float getTemperature();

//get time from the RTC1307 and display it on the LCD
void displayTime();

//get distance from the ultrasonic sensor HC-SR04
int getDistance();

//inform slave about  parking's status
void informSlave(ParkingStatus);

//Ultrasonic pins
const int pingPin = 9; 
const int echoPin = 6; 

//Button pin to make sure that the car entered the parking
const int switchPin = 8; 

//chip select pin for the SD card
const int csPin = 10; 

//current cars number in the parking
int currentCarsNumberIn = 0;

//total cars number that entered the parking today 
int totalCarsNumber = 0;

int earning = 0;
char statistics[STATISTICS_SIZE] = {0};

const char* change = "Dinar";

const char* fileName = "stats.txt";

bool endOfDay = true;
bool carEntered = false;
bool sdCardIntialized = false;

Servo myServo;

RTC_Millis rtc;

LiquidCrystal_I2C lcd(LCD_ADDR, LCD_COLUMNS, LCD_LINES);

File myFile;

ParkingStatus parkingStatus = DISPLAY_TIME_TEMP;

void setup() {
  
  //pins configuration
  pinMode(pingPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(switchPin, INPUT);

  Wire.begin();

  //SD card initialisation
  Serial.println("Initializing SD card...");
  pinMode(csPin, OUTPUT);

  if (SD.begin()){

    Serial.println("SD card is ready to use.");
    sdCardIntialized = true;
  }else{

    Serial.println("SD card initialization failed");
    
  }

  //DS1621 configuration (temperature mode)
  Wire.beginTransmission(DS1621_ADDR);
  Wire.write(DS1621_ADDR_CONF);
  Wire.write(DS1621_CONF_TEMP_MODE);
  Wire.endTransmission();

  lcd.init();
  lcd.backlight();
  lcd.clear();
  rtc.begin(DateTime(F(__DATE__), F(__TIME__)));

  Serial.begin(9600);
  myServo.attach(3);
  myServo.write(CLOSE_BARRIER);

}

void loop() {
  // put your main code here, to run repeatedly:

  switch(parkingStatus){

        case DISPLAY_TIME_TEMP :

          displayTime();
          displayTemperature(getTemperature());

          DateTime now = rtc.now();
          if((now.minute() == 59) && (now.hour() == 23) && (endOfDay == true))
            parkingStatus = SAVE_EARNINGS;

          if((now.minute() == 0) && (now.hour() == 0))
            endOfDay = true;

          parkingStatus = VERIFY_ENTRANCE;

          break;

        case VERIFY_ENTRANCE :

          if (getDistance() <= MAXIMUM_DISTANCE_TO_OPEN_BARRIER) 
            parkingStatus = VERIFY_PARKING_STATUS;
          else
            parkingStatus = DISPLAY_TIME_TEMP;

          break;

        case VERIFY_PARKING_STATUS :

          currentCarsNumberIn = currentCarsNumberIn - getNumberOfCarsOut();

          if (currentCarsNumberIn >= PARKING_MAX_CAPACITY) 
            parkingStatus = PARKING_FULL;
          else
            parkingStatus = PARKING_AVAILABLE;

          break;

        case PARKING_FULL :

          informSlave(PARKING_FULL);

          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Parking Complet !");
          delay(1000);
          lcd.clear();

          parkingStatus = DISPLAY_TIME_TEMP;

          break;

        case PARKING_AVAILABLE :

          informSlave(PARKING_AVAILABLE);

          myServo.write(OPEN_BARRIER);

          now = rtc.now();
          int startedWaitingAt = now.second();

          if(startedWaitingAt >= 56){

            while(now.second() >= (startedWaitingAt + (60 - WAITING_TIME)) % 60){
              
              displayTime();
              if(digitalRead(switchPin))
                carEntered = true;
            }
            
          }else{

            while(now.second() <= (startedWaitingAt + WAITING_TIME)){

              displayTime();
              if(digitalRead(switchPin))
              carEntered = true;

             }
          }

           if(carEntered){

             currentCarsNumberIn++;
             totalCarsNumber++;

            }

            myServo.write(CLOSE_BARRIER);
            carEntered = false;

            parkingStatus = DISPLAY_TIME_TEMP;

            break;

          case SAVE_EARNINGS :

            if(sdCardIntialized){
              
              myFile = SD.open(fileName, FILE_WRITE);

              sprintf(statistics, "Today %d/%d/%d you have earned %d %s\n", now.year(), now.month(),
              now.day(), totalCarsNumber * PARKING_COST, change);
              myFile.print(statistics);
              myFile.close();
              memset(statistics, 0, STATISTICS_SIZE); 
              endOfDay = false;
              totalCarsNumber = 0;
              
            }

            parkingStatus = DISPLAY_TIME_TEMP;

            break;

  }

}

void displayTime(){

  DateTime now = rtc.now();

  lcd.setCursor(0, 0);
  lcd.print(now.year());

  lcd.setCursor(4, 0);
  lcd.print('/');

  lcd.setCursor(5, 0);
  lcd.print(now.month());

  lcd.setCursor(7, 0);
  lcd.print('/');

  lcd.setCursor(8, 0);
  lcd.print(now.day());

  if(now.hour() < 10){

    lcd.setCursor(0,1);
    lcd.print(0);

    lcd.setCursor(1,1);
    lcd.print(now.hour());

  }else{

    lcd.setCursor(0, 1);
    lcd.print(now.hour());
  }

  lcd.setCursor(2, 1);
  lcd.print(':');

  if(now.minute() < 10){

    lcd.setCursor(3,1);
    lcd.print(0);

    lcd.setCursor(4,1);
    lcd.print(now.minute());

  }else{

    lcd.setCursor(3, 1);
    lcd.print(now.minute());
  }

  lcd.setCursor(5, 1);
  lcd.print(':');

  if(now.second() < 10){

    lcd.setCursor(6,1);
    lcd.print(0);

    lcd.setCursor(7,1);
    lcd.print(now.second());

  }else{

    lcd.setCursor(6, 1);
    lcd.print(now.second());
  }
}

//Calcul du distance grace a l'ultrason
int getDistance(){

  digitalWrite(pingPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(pingPin, LOW);
  int duration = pulseIn(echoPin, HIGH);
  return duration * 0.034 / 2;

}

int getNumberOfCarsOut(){

  int numberOfCarsOut = 0;
  Wire.requestFrom(ARDUINO_SLAVE_ADDR,1);
  numberOfCarsOut = Wire.read();
  Wire.endTransmission();
  return numberOfCarsOut;

}

float getTemperature(){

//Debut de mesure de température du DS1621
  Wire.beginTransmission(DS1621_ADDR);
  Wire.write(DS1621_BEGIN_SENSING);
  Wire.endTransmission();

  delay(800);

  //Lecture du température du DS1621
  Wire.beginTransmission(DS1621_ADDR);
  Wire.write(DS1621_TEMP_REGISTRE);
  Wire.endTransmission(false);            //pour utiliser un repeeted start

//on commence la lecture a partir de l'adresse 0xAA
  Wire.requestFrom(DS1621_ADDR, 2);
  char temperatureFirstByte = Wire.read();
  char temperatureSecondByte = Wire.read();
  Wire.endTransmission();
  
  float temp = temperatureFirstByte;
  if(temperatureSecondByte)
     temp += 0.5;
     
  return temp;
  
}

void displayTemperature(float temp){

  lcd.setCursor(9, 1);
  lcd.print("T:");
  lcd.setCursor(11,1);
  lcd.print(temp);

}

void informSlave(ParkingStatus parkingstatus){
  
  Wire.beginTransmission(ARDUINO_SLAVE_ADDR);
  Wire.write(parkingstatus);
  Wire.endTransmission();
    
}
