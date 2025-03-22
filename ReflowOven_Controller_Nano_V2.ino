//Reflow Oven controller program intended for an arduino nano combined with 3 SSRs

#include "max6675.h"

int heaterBottom = A0;
int heaterTop = A1;
int fanAndLight = A2;
int startButton = A6;
int greenLED = A4;
int yellowLED = A5;
int buzzer = A3;
int keypad = A7;

float frontTempF = 0;
float backTempF = 0;
float leftTempF = 0;
float rightTempF = 0;
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
int nominalTempLogInterval = 3000;
int tempLogsMade = 0;

double overallHighestTemp = 0;
int absoluteMaxTemp = 550;

int keypadValue = 0;

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
bool profileComplete = false;
bool keypadPressed = false;
String keypadNumAsStr = "";
char keypadChar = 'n';
bool profileSelected = false;
int selectedProfile = -1;

int serialPrintFrequency = 5000; //in milliseconds
long serialPrintCycleStart = 0;


bool heatersOn = false;
long timeOfLastRead = 0;

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
  analogReference(EXTERNAL);
  Serial.begin(9600);
  Serial.println("GarageScience Reflow Program V2.0");
  pinMode(startButton, INPUT);
  pinMode(greenLED, OUTPUT);
  pinMode(yellowLED, OUTPUT);  
  pinMode(fanAndLight, OUTPUT);
  pinMode(heaterBottom, OUTPUT);
  pinMode(heaterTop, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(keypad, INPUT);

  //analogWrite(fanAndLight,255);
  digitalWrite(fanAndLight, HIGH);
  digitalWrite(heaterBottom, LOW);
  digitalWrite(heaterTop, LOW);

  digitalWrite(greenLED, LOW);
  digitalWrite(yellowLED, LOW);
  digitalWrite(buzzer, LOW);
  //analogWrite(7,255);
  //initiateFinishedBuzzer();

  //startTime = millis();
  //while (analogRead(startButton) > 500){
  //  //digitalWrite(buzzer, HIGH);
  //  readAllTCs();
  //  //printTemperaturesToSerial();
  //  //Serial.print("Time elapsed (sec): ");
  //  //Serial.print(timeElapsed);
  //  delay(100);
  //  timeElapsed = (millis()-startTime)/1000; 
  //}
  
  /*
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
  */
}

void loop() {
  //make log entry if needed
  makeLog();


  //keypadNumAsStr = "";
  keypadChar = 'n';
  keypadChar = getKeypadInput();
  if(keypadChar != 'n'){
    //profile selection being made
    if(keypadChar != '#' && keypadChar != '*'){
      //add char to string
      keypadNumAsStr = keypadNumAsStr + keypadChar;
      Serial.print("char added to numAsString, keypadNumAsStr = ");
      Serial.println(keypadNumAsStr);
    }
    else if(keypadChar == '#'){
      //selection complete
      profileSelected = true;
      Serial.println("***profile selection complete***");
    }
    else if(keypadChar == '*'){
      //reset selection
      profileSelected = false;
      keypadNumAsStr = "";
      keypadChar = 'n';
      selectedProfile = -1;
      Serial.println("***profile selection cancelled***");
    }
  }

  if(profileSelected){
    selectedProfile = keypadNumAsStr.toInt();
    switch(selectedProfile){
      case 1:
        //insert reference to profile
        Serial.println("Running profile 1");
        runProfileOne();
        break;
      case 2:
        //insert reference to profile
        Serial.println("Running profile 2");
        runProfileTwo();
        break;
      case 3:
        Serial.println("Running profile 3");
        runProfileThree();
        break;
      case 4:
        Serial.println("Running profile 4");
        runProfileFour();
        break;
      case 5:
        Serial.println("Running profile 5");
        runProfileFive();
        break;

      default:
        //invalid profile
        digitalWrite(buzzer, HIGH);
        delay(3000);
        digitalWrite(buzzer, LOW);
        break;
    }
    profileSelected = false;
    keypadNumAsStr = "";
    keypadChar = 'n';
    selectedProfile = -1;
  }




  /*
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
  */
}

void makeLog(){
  //Serial.println(int(millis()/nominalTempLogInterval));
  long thisMillis = millis();
  
  int bufferInt = int(thisMillis/nominalTempLogInterval);
  
  if(bufferInt > tempLogsMade){
    readAllTCs();
    printTemperaturesToSerial();
    tempLogsMade = bufferInt;
  }
}

char getKeypadInput(){
  //n = none
  int offset = 0;
  long startWait = 0;
  keypadValue = analogRead(keypad);
  if(keypadValue > 45){ //ensure value measured is the stable value
    delay(50);
    keypadValue = analogRead(keypad);
    Serial.print("keypadValue is: ");
    Serial.println(keypadValue);
  }

  if(keypadValue > 45){
    keypadPressed = true;
    digitalWrite(buzzer, HIGH);
    delay(100);
    digitalWrite(buzzer, LOW);
    //keypadValue = analogRead(keypad);
    if(keypadValue > 542-40-offset && keypadValue < 542+40-offset){
      Serial.print("button 0 pressed");
      Serial.print(", keypadValue is: ");
      Serial.println(keypadValue);
      while(abs(analogRead(keypad) - keypadValue) < 50){
        delay(50);
      }
      keypadValue = 0;
      keypadPressed = false;
      return '0';
    }
    else if(keypadValue > 752-35-offset && keypadValue < 752+30-offset){
      Serial.print("button 1 pressed");
      Serial.print(", keypadValue is: ");
      Serial.println(keypadValue);
      while(abs(analogRead(keypad) - keypadValue) < 50){
        delay(50);
      }
      keypadValue = 0;
      keypadPressed = false;
      return '1';
    }
    else if(keypadValue > 368-40-offset && keypadValue < 368+35-offset){
      Serial.print("button 2 pressed");
      Serial.print(", keypadValue is: ");
      Serial.println(keypadValue);
      while(abs(analogRead(keypad) - keypadValue) < 50){
        delay(50);
      }
      keypadValue = 0;
      keypadPressed = false;
      return '2';
    }
    else if(keypadValue > 71-20-offset && keypadValue < 71+15-offset){
      Serial.print("button 3 pressed");
      Serial.print(", keypadValue is: ");
      Serial.println(keypadValue);
      while(abs(analogRead(keypad) - keypadValue) < 50){
        delay(50);
      }
      keypadValue = 0;
      keypadPressed = false;
      return '3';
    }
    else if(keypadValue > 830-30-offset && keypadValue < 830+20-offset){
      Serial.print("button 4 pressed");
      Serial.print(", keypadValue is: ");
      Serial.println(keypadValue);
      while(abs(analogRead(keypad) - keypadValue) < 50){
        delay(50);
      }
      keypadValue = 0;
      keypadPressed = false;
      return '4';
    }
    else if(keypadValue > 458-35-offset && keypadValue < 458+40-offset){
      Serial.print("button 5 pressed");
      Serial.print(", keypadValue is: ");
      Serial.println(keypadValue);
      while(abs(analogRead(keypad) - keypadValue) < 50){
        delay(50);
      }
      keypadValue = 0;
      keypadPressed = false;
      return '5';
    }
    else if(keypadValue >106-15-offset && keypadValue < 106+40-offset){
      Serial.print("button 6 pressed");
      Serial.print(", keypadValue is: ");
      Serial.println(keypadValue);
      while(abs(analogRead(keypad) - keypadValue) < 50){
        delay(50);
      }
      keypadValue = 0;
      keypadPressed = false;
      return '6';
    }
    else if(keypadValue > 877-20-offset && keypadValue < 877+50-offset){
      Serial.print("button 7 pressed");
      Serial.print(", keypadValue is: ");
      Serial.println(keypadValue);
      while(abs(analogRead(keypad) - keypadValue) < 50){
        delay(50);
      }
      keypadValue = 0;
      keypadPressed = false;
      return '7';
    }
    else if(keypadValue > 631-40-offset && keypadValue < 631+35-offset){
      Serial.print("button 8 pressed");
      Serial.print(", keypadValue is: ");
      Serial.println(keypadValue);
      while(abs(analogRead(keypad) - keypadValue) < 50){
        delay(50);
      }
      keypadValue = 0;
      keypadPressed = false;
      return '8';
    }
    else if(keypadValue > 194-40-offset && keypadValue < 194+35-offset){
      Serial.print("button 9 pressed");
      Serial.print(", keypadValue is: ");
      Serial.println(keypadValue);
      while(abs(analogRead(keypad) - keypadValue) < 50){
        delay(50);
      }
      keypadValue = 0;
      keypadPressed = false;
      return '9';
    }
    else if(keypadValue > 1015-50-offset && keypadValue < 1015+200-offset){
      Serial.print("button * pressed");
      Serial.print(", keypadValue is: ");
      Serial.println(keypadValue);
      while(abs(analogRead(keypad) - keypadValue) < 50){
        delay(50);
      }
      keypadValue = 0;
      keypadPressed = false;
      return '*';
    }
    else if(keypadValue > 271-35-offset && keypadValue < 271+40-offset){
      Serial.print("button # pressed");
      Serial.print(", keypadValue is: ");
      Serial.println(keypadValue);
      while(abs(analogRead(keypad) - keypadValue) < 50){
        delay(50);
      }
      keypadValue = 0;
      keypadPressed = false;
      return '#';
    }
  }
  startWait = millis();
  while(abs(analogRead(keypad) - keypadValue) < 25){
    delay(50);
    if(millis() - startWait > 3000){
      break;
    }
  }
  keypadValue = 0;
  keypadPressed = false;
  return 'n';
}

void runProfileOne(){
  profileComplete = false;

  tempSet = 500;
  tempDrift = 0;
  preheatTemp = 135;

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

  while(profileComplete == false){
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
      delay(30000);
      initiateFinishedBuzzer();
      profileComplete = true;
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
}

void runProfileTwo(){
  delay(500);
  beepBuzzer(1000,0.5,2);
  heatToTempAllHeaters(30, 450, 35, 1.0);
  beepBuzzer(200,0.5,3);
  digitalWrite(yellowLED, HIGH);
  while(analogRead(startButton) > 500){
    delay(50);
  }
  digitalWrite(yellowLED, LOW);
  digitalWrite(greenLED, HIGH);
  heatToTempAllHeaters(120, 450, 35, 1.0);
  beepBuzzer(200,0.5,3);
  //crack open door and wait
  waitUntilSingleTCReachesTemp(400);
  //close door
  heatToTempAllHeaters(1200, 400, 35, 1.0);
  setHeaters("Off");
  digitalWrite(greenLED, LOW);
  beepBuzzer(400,0.5,4);
  beepBuzzer(200,0.5,8);
  beepBuzzer(5000,0.5,1);
}

void runProfileThree(){
  int cureTimeHours = 2;
  int cureTimeSeconds = cureTimeHours*60*60;
  delay(500);
  digitalWrite(greenLED, HIGH);
  beepBuzzer(1000,0.5,3);
  heatToTempAllHeaters(cureTimeSeconds, 160, 25, 0.5);
  setHeaters("Off");
  digitalWrite(greenLED, LOW);
  beepBuzzer(400,0.5,4);
  beepBuzzer(200,0.5,8);
  beepBuzzer(5000,0.5,1);
}

void runProfileFour(){
  int cureTimeHours = 4;
  int cureTimeSeconds = cureTimeHours*60*60;
  delay(500);
  digitalWrite(greenLED, HIGH);
  beepBuzzer(1000,0.5,4);
  heatToTempAllHeaters(cureTimeSeconds, 160, 25, 0.5);
  setHeaters("Off");
  digitalWrite(greenLED, LOW);
  beepBuzzer(400,0.5,4);
  beepBuzzer(200,0.5,8);
  beepBuzzer(5000,0.5,1);
}

void runProfileFive(){
  int cureTimeHours = 6;
  int cureTimeSeconds = cureTimeHours*60*60;
  delay(500);
  digitalWrite(greenLED, HIGH);
  beepBuzzer(1000,0.5,5);
  heatToTempAllHeaters(cureTimeSeconds, 160, 25, 0.5);
  setHeaters("Off");
  digitalWrite(greenLED, LOW);
  beepBuzzer(400,0.5,4);
  beepBuzzer(200,0.5,8);
  beepBuzzer(5000,0.5,1);
}

void waitUntilSingleTCReachesTemp(int tempGoal){
  readAllTCs();
  while(getlowestTemp() > tempGoal){
    digitalWrite(yellowLED, HIGH);
    delay(1000);
    digitalWrite(yellowLED, LOW);
    delay(1000);
    readAllTCs();
    printTemperaturesToSerial();
  }
  beepBuzzer(2000,0.5,1);
}

String setAppropriateHeaterHeatupState(long timeAtLastHeaterStateChange, double dutyCycle){
  if(heatersOn){
    if((millis()-timeAtLastHeaterStateChange)/10000 > dutyCycle){ //if the length of time the heaters have been on exceed the duty cycle then change heater status
      setHeaters("Off");
      return "off";
    }
  }
  else{
    if((millis()-timeAtLastHeaterStateChange)/10000 > (1-dutyCycle)){ //if the length of time the heaters have been off exceed the duty cycle amount then change heater status
      setHeaters("On");
      return "on";
    }
  }
  return "";
}

void heatToTempAllHeaters(int timetoBake, int tempToBake, int heatupDrift, double heatupDutyCycle){//time in seconds, temp in Farenheit
  bool setTempReached = false;
  bool bakeTimeComplete = false;
  int driftDelay = 30000;
  int i = 0;
  tempSet = tempToBake;
  tempDrift = heatupDrift;
  preheatTemp = 135;
  int heaterCycles = 0;
  long heaterCycleBufferTime = 0;
  String heaterHeatupStateChange = "";

  readAllTCs();
  avgTemp = getAverageTemp();
  if(avgTemp > tempSet){
    setHeaters("Off");
    heaterCycles++;
    if(heaterCycles == 1){
      startTime = millis();
    }
    setTempReached = true;
  }
  else{
    setHeaters("On");
    heaterCycleBufferTime = millis();
  }
  
  while(!bakeTimeComplete){
    //The while loop to setTemp is for performing a heatup.
    makeLog();
    while(!setTempReached){
      makeLog();
      timeElapsed = (millis()-startTime)/1000;
      if(timeElapsed > timetoBake && heaterCycles > 0){
        bakeTimeComplete = true;
        break;
      }
      readAllTCs();
      avgTemp = getAverageTemp();
      heaterHeatupStateChange = setAppropriateHeaterHeatupState(heaterCycleBufferTime,heatupDutyCycle);
      if(heaterHeatupStateChange.length() > 0){
        Serial.println(heaterHeatupStateChange);
        heaterCycleBufferTime = millis();
      }
      //printTemperaturesToSerial();
      delay(100);
      if(avgTemp > tempSet - tempDrift){
        setHeaters("Off");
        heaterCycles++;
        //wait a specified delay time
        delay(driftDelay);
        if(heaterCycles == 1){
          startTime = millis();
        }
        setTempReached = true;
      }
    }


    timeElapsed = (millis()-startTime)/1000;
    tempDrift = tempDrift/2;
    driftDelay = 3000;
    if(timeElapsed < timetoBake){
      //check temp and keep baking
      readAllTCs();
      avgTemp = getAverageTemp();
      printTemperaturesToSerial();
      delay(100);
      if(avgTemp < tempSet - tempDrift/2){
        setHeaters("On");
        heaterCycleBufferTime = millis();
        setTempReached = false;
      }
    }
    else{
      bakeTimeComplete = true;
    }
  }
  setHeaters("Off");
  
  //for(i = 0; i < 3; i++){
  //  digitalWrite(buzzer, HIGH);
  //  delay(100);
  //  digitalWrite(buzzer, LOW);
  //  delay(100);
  //}
}

void beepBuzzer(int periodOfBeep, double dutyCycleOfBeep, int beepCount){
  int i = 0;
  for(i = 0; i < beepCount; i++){
    digitalWrite(buzzer, HIGH);
    delay(periodOfBeep*dutyCycleOfBeep);
    digitalWrite(buzzer, LOW);
    delay(periodOfBeep*(1-dutyCycleOfBeep));
  }
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
  profileComplete = true;
}

double getlowestTemp(){
  double lowestTemp = 1000;

  if(enableRightTC){
    if(rightTempF < lowestTemp){
      lowestTemp = rightTempF;
    }
  }
  if(enableFrontTC){
    if(frontTempF < lowestTemp){
      lowestTemp = frontTempF;
    }
  }
  if(enableLeftTC){
    if(leftTempF < lowestTemp){
      lowestTemp = leftTempF;
    }
  }
  if(enableBackTC){
    if(backTempF < lowestTemp){
      lowestTemp = backTempF;
    }
  }
  return lowestTemp;
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
  int highestTemp = 0;
  int lowestTemp = 1000;


  if(millis() - timeOfLastRead > 500){ //Max6675 requires about 220ms between read operations.  Make sure I wait a comfortable 500ms to make sure I get a good number
    frontTempF = tcFront.readFahrenheit();
    //delay(50);
    //Serial.println(frontTempF);
    backTempF = tcBack.readFahrenheit();
    //delay(50);
    //Serial.println(backTempF);
    leftTempF = tcLeft.readFahrenheit();
    //delay(50);
    //Serial.println(leftTempF);
    rightTempF = tcRight.readFahrenheit();
    //delay(50);
    //Serial.println(rightTempF);
    timeOfLastRead = millis();
  }

  //ensure TCs are reading accurately
  if(enableFrontTC){
    if(frontTempF > highestTemp){
      highestTemp = frontTempF;
    }
    if(frontTempF < lowestTemp){
      lowestTemp = frontTempF;
    }
    if(frontTempF > overallHighestTemp){
      overallHighestTemp = frontTempF;
    }
  }
  if(enableBackTC){
    if(backTempF > highestTemp){
      highestTemp = backTempF;
    }
    if(backTempF < lowestTemp){
      lowestTemp = backTempF;
    }
    if(backTempF > overallHighestTemp){
      overallHighestTemp = backTempF;
    }
  }
  if(enableLeftTC){
    if(leftTempF > highestTemp){
      highestTemp = leftTempF;
    }
    if(leftTempF < lowestTemp){
      lowestTemp = leftTempF;
    }
    if(leftTempF > overallHighestTemp){
      overallHighestTemp = leftTempF;
    }
  }
  if(enableRightTC){
    if(rightTempF > highestTemp){
      highestTemp = rightTempF;
    }
    if(rightTempF < lowestTemp){
      lowestTemp = rightTempF;
    }
    if(rightTempF > overallHighestTemp){
      overallHighestTemp = rightTempF;
    }
  }

  //Serial.print("highestTemp = ");
  //Serial.println(highestTemp);
  //Serial.print("lowestTemp = ");
  //Serial.println(lowestTemp);

  if(highestTemp - lowestTemp > 100 && highestTemp > overallHighestTemp){ //something is very wrong, could have localized fire
    emergencyStop();
  }
  if(highestTemp - tempSet > 100){
    if(heatersOn){ //not in a cooling cycle so something is wrong and I'm overheating
      emergencyStop();
    }
  }
  if(highestTemp > absoluteMaxTemp){ //could have gotten stuck somehow in a heatup and I'm overheating the oven
    emergencyStop();
  }
}

void emergencyStop(){
  setHeaters("Off");
  while(true){
    digitalWrite(buzzer, HIGH);
    digitalWrite(yellowLED, HIGH);
    delay(200);
    digitalWrite(buzzer, LOW);
    digitalWrite(yellowLED, LOW);
    delay(200);
  }
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
  Serial.print("    averageTemp = ");
  Serial.println(getAverageTemp());
  Serial.print("startButton status: ");
  Serial.println(analogRead(startButton));
  if(heatersOn){
    Serial.println("heater status: ON");
  }
  else{
    Serial.println("heater status: OFF");
  }
}
