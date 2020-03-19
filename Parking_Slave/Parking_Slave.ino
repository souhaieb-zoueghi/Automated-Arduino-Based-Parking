#include <Wire.h>
#include <Ultrasonic.h>
#include <Servo.h>
#include <SPI.h>

void turnOnRedLed();
void turnOnGreenLed();
void turnOffLeds();
int getDistance();

//ultrasonic pins
int pingPin = 9;
int echoPin = 8;

int numberOfCarsOut = 0;

enum ParkingStatus{PARKING_FULL = 0, PARKING_AVAILABLE};

ParkingStatus parkingStatus = 0;

Servo myServo;

void setup() {
  //put your setup code here, to run once:
  Serial.begin(9600);
  Wire.begin(8);

  //utilisé quand le maitre nous envoi des données (Wire.write())
  Wire.onReceive(receiveEvent);

  //utilisé quand le maitre demande des données de l'esclave (Wire.requestfrom())
  Wire.onRequest(requestEvent);

//Initialement la barriére est fermée
  myServo.attach(3);
  myServo.write(90);

  pinMode(SS, OUTPUT);                //configure controller's Slave Select pin to output
  digitalWrite(SS, HIGH);             //disable Slave Select
  SPI.begin();
  //IODIRB register is already configured to input by default
  SPI.beginTransaction(SPISettings (SPI_CLOCK_DIV8, MSBFIRST, SPI_MODE0));

  //Configuration de l'expander MCP sur le Bank 0
  digitalWrite(SS, LOW);              
  SPI.transfer(0x40);        
  SPI.transfer(0x05);              
  SPI.transfer(0x00);     
  digitalWrite(SS, HIGH);
                
//Configuration du GPIOA comme sortie en ecrivant 0x00 dans le registre IODIRA (0x00)
  digitalWrite(SS, LOW);            
  SPI.transfer(0x40);        
  SPI.transfer(0x00);              
  SPI.transfer(0x00);     
  digitalWrite(SS, HIGH);             
}

void loop() {
  
  // put your main code here, to run repeatedly:

 //il y a une voiture a la sortie du parking si la distance est <= 9
  if(getDistance() <= 9){
    //On léve la barriere pendant 4 secondes et on inremente le nombre des voitures sortées
    numberOfCarsOut++;
    myServo.write(0);
    delay(4000);
    myServo.write(90);
  }
  
  Serial.print(" number of cars that got out : ");
  
  Serial.println(numberOfCarsOut);

  //On eteint les LEDS
  turnOffLeds();
  
  delay(1000);
  
}

void receiveEvent(){

  //On recupere l'etat du parking du maitre 
  parkingStatus = Wire.read();
   
  if(parkingStatus == PARKING_FULL){
    
      turnOnRedLed();   
      delay(2000);
     
  }
  else{
        
      turnOnGreenLed();    
      delay(2000);
      
  }

}
void requestEvent(){

  //Si le maitre fait un requestFrom on lui envoie le nombre des voitures sortées et on le remet a 0
  Wire.write(numberOfCarsOut);
  numberOfCarsOut = 0;
  
}

//Calcul du distance
int getDistance(){
  
  int duration = 0;
  digitalWrite(pingPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(pingPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  return duration * 0.034 / 2;
 
}


void turnOnRedLed(){
//La LED rouge est connectée au GPIOA 7, pur l'allumer on doit ecrire 0x80 dans le registre 0x12(registre du GPIOA)
      digitalWrite(SS, LOW);
      SPI.transfer(0x40);        //read command
      SPI.transfer(0x12);
      SPI.transfer(0x80); 
      digitalWrite(SS, HIGH);
      
  }

  void turnOnGreenLed(){
//La LED verte est connectée au GPIOA 6, pur l'allumer on doit ecrire 0x40 dans le registre 0x12(registre du GPIOA)
      digitalWrite(SS, LOW);
      SPI.transfer(0x40);        //read command
      SPI.transfer(0x12);
      SPI.transfer(0x40); 
      digitalWrite(SS, HIGH);
      
  }
  
void turnOffLeds(){
//pour eteindre les leds il suffit d'ecrire 0x00 dans le registre 0x12
      digitalWrite(SS, LOW);
      SPI.transfer(0x40);        //read command
      SPI.transfer(0x12);
      SPI.transfer(0x00); 
      digitalWrite(SS, HIGH);
      
  }


    
