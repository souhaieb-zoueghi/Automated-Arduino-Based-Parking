#include <Wire.h>
#include <Ultrasonic.h>
#include <Servo.h>
#include <SPI.h>

//MCP expander specfications
#define MCP_GENERAL_CONF_REGISTRE 0x05
#define MCP_WRITE_COMMAND 0x40
#define MCP_IODIRA_REGISTRE 0x00
#define MCP_GPIOA_REGISTRE 0x12

#define CLOSE_BARRIER 90
#define OPEN_BARRIER 0
#define MAXIMUM_DISTANCE_TO_OPEN_BARRIER 9
#define ARDUINO_SLAVE_ADDR 8

//Get distance from the ultrasonic sensor HC-SR04
int getDistance();

//Turn on the red led via the MCP expander
void turnOnRedLed();

//Turn on the green led via the MCP expander
void turnOnGreenLed();

//Turn off all the leds via the MCP expander
void turnOffLeds();


//Ultrasonic pins
const int pingPin = 9;
const int echoPin = 8;

int numberOfCarsOut = 0;

enum ParkingStatus{PARKING_FULL = 0, PARKING_AVAILABLE};

ParkingStatus parkingStatus = 0;

Servo myServo;

void setup() {

  Serial.begin(9600);
  Wire.begin(ARDUINO_SLAVE_ADDR);

  //Function called when master send data (via Wire.write())
  Wire.onReceive(receiveEvent);

  //Function called when master request data from slave
  Wire.onRequest(requestEvent);

  myServo.attach(3);
  myServo.write(CLOSE_BARRIER);

  pinMode(SS, OUTPUT);                //configure controller's Slave Select pin to output
  digitalWrite(SS, HIGH);             //disable Slave Select
  SPI.begin();

  //IODIRB register is already configured to input by default
  SPI.beginTransaction(SPISettings (SPI_CLOCK_DIV8, MSBFIRST, SPI_MODE0));

  //Configuration of MCP expander on Bank 0
  digitalWrite(SS, LOW);
  SPI.transfer(MCP_WRITE_COMMAND);
  SPI.transfer(MCP_GENERAL_CONF_REGISTRE);
  SPI.transfer(0x00);
  digitalWrite(SS, HIGH);

  //Configuration of GPIOA as output(0 : output, 1 : input)
  digitalWrite(SS, LOW);
  SPI.transfer(MCP_WRITE_COMMAND);
  SPI.transfer(MCP_IODIRA_REGISTRE);
  SPI.transfer(0x00);
  digitalWrite(SS, HIGH);

}

void loop() {

  if(getDistance() <= MAXIMUM_DISTANCE_TO_OPEN_BARRIER){

    numberOfCarsOut++;
    myServo.write(OPEN_BARRIER);
    delay(4000);
    myServo.write(CLOSE_BARRIER);

  }

  Serial.print(" number of cars that got out : ");
  Serial.println(numberOfCarsOut);

  turnOffLeds();

  delay(1000);

}

void receiveEvent(){

  //Get parking's status from the master
  parkingStatus = Wire.read();

  if(parkingStatus == PARKING_FULL){

      turnOnRedLed();
      delay(1000);

  }else{

      turnOnGreenLed();
      delay(1000);

  }

}

void requestEvent(){

  //Send the number of cars that got out to the master
  Wire.write(numberOfCarsOut);
  numberOfCarsOut = 0;

}

int getDistance(){

  int duration = 0;
  digitalWrite(pingPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(pingPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  return duration * 0.034 / 2;

}

void turnOnRedLed(){
//The red led is connected to GPIOA 7 on the MCP
      digitalWrite(SS, LOW);
      SPI.transfer(MCP_WRITE_COMMAND);
      SPI.transfer(MCP_GPIOA_REGISTRE);
      SPI.transfer(0x80);
      digitalWrite(SS, HIGH);

  }

  void turnOnGreenLed(){
//The green led is connected to GPIOA 6 on the MCP
      digitalWrite(SS, LOW);
      SPI.transfer(MCP_WRITE_COMMAND);
      SPI.transfer(MCP_GPIOA_REGISTRE);
      SPI.transfer(0x40);
      digitalWrite(SS, HIGH);

  }

void turnOffLeds(){
//pour eteindre les leds il suffit d'ecrire 0x00 dans le registre 0x12
      digitalWrite(SS, LOW);
      SPI.transfer(MCP_WRITE_COMMAND);        //read command
      SPI.transfer(MCP_GPIOA_REGISTRE);
      SPI.transfer(0x00);
      digitalWrite(SS, HIGH);

  }
