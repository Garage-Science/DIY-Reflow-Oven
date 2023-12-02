//Reflow Oven controller program intended for an arduino nano combined with 3 SSRs

#include "max6675.h"

int heaterBottom = A0;
int heaterTop = A1;
int fanAndLight = A2;
int startButton = A6;
int greenLED = A4;
int yellowLED = A5;
int buzzer = A3;

double frontTempF = 0;
double backTempF = 0;
double leftTempF = 0;
double rightTempF = 0;
double avgTemp = 0;

int tcFrontDO = 2;
int tcFrontCS = 3;
int tcFrontCLK = 4;

int tcBackDO = 5;
int tcBackCS = 6;
int tcBackCLK = 7;

int tcLeftDO = 8;
int tcLeftCS = 9;
int tcLeftCLK = 10;

int tcRightDO = 11;
int tcRightCS = 12;
int tcRightCLK = 13;

bool enableRightTC = true;
bool enableFrontTC = true;
bool enableLeftTC = true;
bool enableBackTC = false;

int tempSet = 460;
int tempDrift = 20;
int preheatTemp = 135;
long timeElapsed = 0;
long startTime = 0;


double dutyCycle = 0.2;
int cycleLength = 10000; // in milliseconds
double cycleOnLength = cycleLength*dutyCycle; //in milliseconds
double cycleOffLength = cycleLength*(1-dutyCycle); //in milliseconds
long cycleStart = 0;
long cycleOnLegStart = 0;
long cycleOffLegStart = 0;
bool withinCycleOnLeg = false;
bool withinCycleOffLeg = false;
bool dutyCycleReduced = false;

int serialPrintFrequency = 5000; //in milliseconds
long serialPrintCycleStart = 0;


bool heatersOn = false;

MAX6675 tcFront(tcFrontCLK, tcFrontCS, tcFrontDO);
MAX6675 tcBack(tcBackCLK, tcBackCS, tcBackDO);
MAX6675 tcLeft(tcLeftCLK, tcLeftCS, tcLeftDO);
MAX6675 tcRight(tcRightCLK, tcRightCS, tcRightDO);


//approximately 10min required to pre-heat to 85F using lightbulb
//After 15min temp via lightbulb only is ~88F
//Disregard backTC.  It may be faulty or may just be in a cooler area.
//The back TC is consistently 25-30F lower than the rest of the TCs

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println("GarageScience Reflow Program V1.0");
  pinMode(startButton, INPUT);
  pinMode(greenLED, OUTPUT);
  pinMode(yellowLED, OUTPUT);  
  pinMode(fanAndLight, OUTPUT);
  pinMode(heaterBottom, OUTPUT);
  pinMode(heaterTop, OUTPUT);
  pinMode(buzzer, OUTPUT);

  //analogWrite(fanAndLight,255);
  digitalWrite(fanAndLight, HIGH);
  digitalWrite(heaterBottom, LOW);
  digitalWrite(heaterTop, LOW);

  digitalWrite(greenLED, LOW);
  digitalWrite(yellowLED, LOW);
  digitalWrite(buzzer, LOW);
  //analogWrite(7,255);
  //initiateFinishedBuzzer();

  startTime = millis();
  while (analogRead(startButton) > 500){
    //digitalWrite(buzzer, HIGH);
    readAllTCs();
    //printTemperaturesToSerial();
    //Serial.print("Time elapsed (sec): ");
    //Serial.print(timeElapsed);
    delay(100);
    timeElapsed = (millis()-startTime)/1000; 
  }
  
  digitalWrite(yellowLED, HIGH);
  //delay(1000);
  digitalWrite(buzzer, HIGH);
  delay(500);
  digitalWrite(buzzer, LOW);
  delay(1000);
  digitalWrite(buzzer, HIGH);
  delay(500);
  digitalWrite(buzzer, LOW);
  delay(1000);
  digitalWrite(buzzer, HIGH);
  delay(1000);
  digitalWrite(buzzer, LOW);

  cycleStart = millis();
  cycleOnLegStart = cycleStart;
  withinCycleOnLeg = true;
  withinCycleOffLeg = false;
  setHeaters("on");
  Serial.println();
  Serial.print("Duty Cycle: ");
  Serial.println(dutyCycle);
  Serial.print("On Cycle Length in milliseconds: ");
  Serial.println(cycleOnLength);
  Serial.print("Off Cycle Length in milliseconds: ");
  Serial.println(cycleOffLength);
  Serial.println("***Beginning duty cycle controlled pre-heat cycle***");
  while(analogRead(startButton) > 500){
    if(withinCycleOnLeg){
      Serial.print("time within On-Cycle = ");
      Serial.println((millis() - cycleOnLegStart));
      if((millis() - cycleOnLegStart) < cycleOnLength){
        readAllTCs();
        avgTemp = getAverageTemp();
        if(!dutyCycleReduced && avgTemp > (preheatTemp - 20)){
          //if I'm this close to preheat temp then I need to slow my role
          dutyCycleReduced = true;
          Serial.println("*****Within 20F of preheatTemp, wait here a moment, then reduce duty cycle*****");
          delay(10000);
          dutyCycle = 0.1;
          cycleOnLength = cycleLength*dutyCycle; //in milliseconds
          cycleOffLength = cycleLength*(1-dutyCycle); //in milliseconds
        }
        Serial.print("Average Temp(F) = ");
        Serial.println(avgTemp);
        if(avgTemp > preheatTemp){
          //skip to end of on-cycle
          //done with on leg
          setHeaters("Off");
          withinCycleOnLeg = false;
          withinCycleOffLeg = true;
          cycleOffLegStart = millis();
          Serial.println();
          Serial.println("***Premptively ended On-Cycle, beginning off cycle***");
        }
        delay(500);
      }
      else{
        //done with on leg
        //if(avgTemp > preheatTemp){ //then turn off heater otherwise don't bother since I'll just turn it back on
        //  setHeaters("Off");
        //}
        setHeaters("Off");
        withinCycleOnLeg = false;
        withinCycleOffLeg = true;
        cycleOffLegStart = millis();
        Serial.println();
        Serial.println("***DONE with On-Cycle, beginning off cycle***");
      }
    }
    else if(withinCycleOffLeg){
      Serial.print("time within Off-Cycle = ");
      Serial.println((millis() - cycleOffLegStart));
      if((millis() - cycleOffLegStart) < cycleOffLength){
        readAllTCs();
        avgTemp = getAverageTemp();
        Serial.print("Average Temp(F) = ");
        Serial.println(avgTemp);
        //if(avgTemp < (preheatTemp - 30)){
        //  //significantly below preheatTemp so skip to end of off-cycle
        //  //done with on leg
        //  setHeaters("On");
        //  withinCycleOnLeg = true;
        //  withinCycleOffLeg = false;
        //  cycleOnLegStart = millis();
        //  Serial.println();
        //  Serial.println("***Premptively ended Off-Cycle, beginning on cycle***");
        //}
        delay(500);
      }
      else{
        //done with on leg
        if(avgTemp < preheatTemp){ //then turn on heater otherwise don't bother since I'll just turn it back off
          setHeaters("On");
        }
        withinCycleOnLeg = true;
        withinCycleOffLeg = false;
        cycleOnLegStart = millis();
        Serial.println();
        Serial.println("***DONE with Off-Cycle, beginning on cycle***");
      }
    }
    else{
      Serial.println("ERROR: during the warmup period I encountered an unexpected circumstance");
    }
  }

  digitalWrite(yellowLED, LOW);
  digitalWrite(greenLED, HIGH);
  Serial.println();
  Serial.println("*********************************************************");
  Serial.print("Start Heating Cycle to: ");
  Serial.println(tempSet);
  Serial.println("*********************************************************");
  Serial.println();
  startTime = millis();
  setHeaters("On");
}

void loop() {
  // put your main code here, to run repeatedly:
  //frontTempF = tcFront.readFahrenheit();
  //backTempF = tcBack.readFahrenheit();
  //leftTempF = tcLeft.readFahrenheit();
  //rightTempF = tcRight.readFahrenheit();

  readAllTCs();
  avgTemp = getAverageTemp();
  if(avgTemp > tempSet - tempDrift){
    setHeaters("Off");
    //wait a specified delay time
    delay(60000);
    initiateFinishedBuzzer();
  }

  //Serial.println();
  //if(frontTempF < tempSet || backTempF < tempSet || leftTempF < tempSet || rightTempF < tempSet){
  //  //active heaters
  //  setHeaters("On");
  //  //analogWrite(heaterBottom, 255);
  //  //analogWrite(heaterTop, 255);
  //  //Serial.println("heater status: ON");
  //  //heatersOn = true;
  //}
  //else{
  //  //turn off heaters
  //  setHeaters("Off");
  //  //analogWrite(heaterBottom, 0);
  //  //analogWrite(heaterTop, 0);
  //  //Serial.println("heater status: OFF");
  //  //heatersOn = false;
  //}
  printTemperaturesToSerial();
  Serial.print("Time elapsed (sec): ");
  Serial.print(timeElapsed);
  delay(5000);
  timeElapsed = (millis()-startTime)/1000; 
}

void initiateFinishedBuzzer(){
  int i;
  for(i = 0; i < 10; i++){
    digitalWrite(buzzer, HIGH);
    delay(200);
    digitalWrite(buzzer, LOW);
    delay(200);
  }
  for(i = 0; i < 10; i++){
    digitalWrite(buzzer, HIGH);
    delay(100);
    digitalWrite(buzzer, LOW);
    delay(100);
  }
  digitalWrite(buzzer, HIGH);
  delay(5000);
  digitalWrite(buzzer, LOW);
}

double getAverageTemp(){
  int numberOfTempsUsed = 0;
  double tempSum = 0;
  if(enableRightTC){
    tempSum = tempSum + rightTempF;
    numberOfTempsUsed++;
  }
  if(enableFrontTC){
    tempSum = tempSum + frontTempF;
    numberOfTempsUsed++;
  }
  if(enableLeftTC){
    tempSum = tempSum + leftTempF;
    numberOfTempsUsed++;
  }
  if(enableBackTC){
    tempSum = tempSum + backTempF;
    numberOfTempsUsed++;
  }

  return (tempSum/numberOfTempsUsed);
}

void setHeaters(String newStatus){
  if (newStatus.compareTo("On") == 0){
    analogWrite(heaterBottom, 255);
    analogWrite(heaterTop, 255);
    heatersOn = true;
  }
  else if(newStatus.compareTo("Off") == 0){
    analogWrite(heaterBottom, 0);
    analogWrite(heaterTop, 0);
    heatersOn = false;
  }
}
void readAllTCs(){
  frontTempF = tcFront.readFahrenheit();
  backTempF = tcBack.readFahrenheit();
  leftTempF = tcLeft.readFahrenheit();
  rightTempF = tcRight.readFahrenheit();
}
void printTemperaturesToSerial(){
  Serial.println();
  Serial.print("    tcFront: ");
  Serial.println(frontTempF);
  Serial.print("    tcBack: ");
  Serial.println(backTempF);
  Serial.print("    tcLeft: ");
  Serial.println(leftTempF);
  Serial.print("    tcRight: ");
  Serial.println(rightTempF);
  Serial.print("startButton status: ");
  Serial.println(analogRead(startButton));
  if(heatersOn){
    Serial.println("heater status: ON");
  }
  else{
    Serial.println("heater status: OFF");
  }
}
