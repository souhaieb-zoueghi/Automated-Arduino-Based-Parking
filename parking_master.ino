#include <Ultrasonic.h>
#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <RTClib.h>
#include <SD.h>
#define ADDR_DS1621 0x48
#define ADDR_CONF 0xAC
#define CONF 0x02
#define BEGIN 0xEE
#define TEMP_REGISTRE 0xAA
#define ARDUINO_SLAVE 8
#define TARIF 2
#define CLOSE_BARRIER 90
#define OPEN_BARRIER 0

//Ultrasonic pins
int pingPin = 9;
int echoPin = 6;

int switchPin = 8;
int csPin = 10;

int numberOfCarsEntered = 0, totalCarsNumber = 0;

int earning = 0;
char statistics[64] = {0};
char change[6] = "Dinar";

int s, mi, h, d, mo, y;

int startedWaitingAt = 0;

float temp = 0;

bool endOfDay = true;
bool carEntered = false;

typedef enum {PARKING_FULL = 0, PARKING_AVAILABLE, DISPLAY_TIME_TEMP, VERIFY_ENTRANCE,
 VERIFY_PARKING_STATUS, SAVE_EARNINGS}ParkinStatus;

Servo myServo;

RTC_Millis rtc;

LiquidCrystal_I2C lcd(0x3F, 16, 2);

File myFile;

ParkinStatus parkingStatus = DISPLAY_TIME_TEMP;

void setup() {
  // put your setup code here, to run once:

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
    
  }else{
    
    Serial.println("SD card initialization failed");
    return;
  }

  //DS1621 configuration (temperature mode)
  Wire.beginTransmission(ADDR_DS1621);
  Wire.write(ADDR_CONF);
  Wire.write(CONF);
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
          temp = getTemperature();
          displayTemperature(temp);

          DateTime now = rtc.now();
          if((now.minute() == 59) && (now.hour() == 23) && (endOfDay == true))
            parkingStatus = SAVE_EARNINGS;

          if((now.minute() == 0) && (now.hour() == 0))
            endOfDay = true;
            
          parkingStatus = VERIFY_ENTRANCE;
      
          break;

        case VERIFY_ENTRANCE : 

          if (getDistance() <= 9)
            parkingStatus = VERIFY_PARKING_STATUS;
          else
            parkingStatus = DISPLAY_TIME_TEMP;

          break;

        case VERIFY_PARKING_STATUS : 

          numberOfCarsEntered = numberOfCarsEntered - getNumberOfCarsOut();
        
          if (numberOfCarsEntered >= 7)
            parkingStatus = PARKING_FULL;
          else
            parkingStatus = PARKING_AVAILABLE;

          break;

        case PARKING_FULL :

          Wire.beginTransmission(ARDUINO_SLAVE);
          Wire.write(PARKING_FULL);
          Wire.endTransmission();

          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Parking Complet !");
          delay(1000);
          lcd.clear();

          parkingStatus = DISPLAY_TIME_TEMP;
          
          break;

        case PARKING_AVAILABLE :

          Wire.beginTransmission(ARDUINO_SLAVE);
          Wire.write(PARKING_AVAILABLE);
          Wire.endTransmission();

          myServo.write(OPEN_BARRIER);

          now = rtc.now();
          startedWaitingAt = now.second();

          while(now.second() <= (startedWaitingAt + 4)){
        
            displayTime();
            if(digitalRead(switchPin))
            carEntered = true;
        
           }

           if(carEntered){
      
             numberOfCarsEntered++;
             totalCarsNumber++;

            }

            myServo.write(CLOSE_BARRIER);
            carEntered = false;

            parkingStatus = DISPLAY_TIME_TEMP;

            break;

          case SAVE_EARNINGS : 

            myFile = SD.open("stats.txt", FILE_WRITE);

            sprintf(statistics, "Today %d/%d/%d you have earned %d %s", now.year(), now.month(),
            now.day(), totalCarsNumber * TARIF, change);
            myFile.print(statistics);
            myFile.close();
            endOfDay = false;
            totalCarsNumber = 0;
            
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
  Wire.requestFrom(ARDUINO_SLAVE,1);
  numberOfCarsOut = Wire.read();
  Wire.endTransmission();
  return numberOfCarsOut;
  
}

float getTemperature(){

//Debut de mesure de température du DS1621
  Wire.beginTransmission(ADDR_DS1621);
  Wire.write(BEGIN);
  Wire.endTransmission();

  //delay(800);

  //Lecture du température du DS1621
  Wire.beginTransmission(ADDR_DS1621);
  Wire.write(TEMP_REGISTRE);
  Wire.endTransmission(false);            //pour utiliser un repeeted start

//on commence la lecture a partir de l'adresse 0xAA
  Wire.requestFrom(ADDR_DS1621, 2);
  int temperatureFirstByte = Wire.read();
  int temperatureSecondByte = Wire.read();
  Wire.endTransmission();

  temp = int(temperatureFirstByte);
  
  if(temperatureSecondByte)
     return temp += 0.5;
     
  return temp;
  
}


void displayTemperature(float temp){
  
  lcd.setCursor(9, 1);
  lcd.print("T:");
  lcd.setCursor(11,1);
  lcd.print(temp);

}
