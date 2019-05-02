// This script is designed to allow an impact on a piece of Smartfoam
// Sam Wilding
// 9/13/18


/************** USER: PLEASE INPUT YOUR RAIL VOLTAGE HERE *******************************/
//            if you had a rails voltage of 3.3V then please type
//            #define RAILS_VOLTAGE 3.3
//            do not write V or volts or anything after the number. Decimals values OK
//
              #define RAILS_VOLTAGE 5
//
/****************************************************************************************/


#include <stdlib.h>
#include <Smoothed.h>
#include <SPI.h>
#include <SD.h>


#define array_size 50
#define buffer_size 20
#define fs 0.5

//Array to collect the last 200 or so points of data and then spit it all out once an impact has occured
const int historySize = 150;
int historyIndex = 0;
int history[historySize] = {0};
int time_history[historySize] = {0};

double baseline;

//new baseline calc variables
int avgOfAvg;
int avgOfMax;
int avgOfMin;

boolean is37Battery = false;
boolean isArduino5v = false;
boolean isDebug = false;
boolean hasSDCard = false;

/* Running average data */
const int runningLength = 30;
int runningAvg[runningLength] = {};

int currentAvg = 0;
int currentMax = 0;
int currentMin = 0;
boolean duringImpact = false;
int impactMaxPeak = 0;
int impactMaxAvg = 0;
int impactMinPeak = 1023; //the "max" min
int impactCounter = 0;
int localAvg = 0;
int localMax = 0;
int localMin = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin (9600);
  analogReference(DEFAULT);

  //setup the SD card reader
  if(!SD.begin(4)){
    Serial.println("SD Card not being used");
  }else{
    hasSDCard = true;
    Serial.println("Using SD card reader");
  }

  impactMaxAvg = avgOfAvg;
  impactMaxPeak = avgOfAvg; 
  
  //if A0 is plugged into rails, then debug mode is active
  //if A0 is plugged into nothing (or ground?) then its production mode
  int debugSetting = analogRead(A0);
  if(debugSetting > 500){
    isDebug = true;
    Serial.println("------------------ DEBUG MODE ACTIVE ------------------");
  }
  
  //read the voltage on A5, read where the plugged in the thing
  //if they plug it into ground, then they are using the battery
  //if they plug it into 5 volts, then they are using the 5v from arduino
  int railsSetting = analogRead(A5);
  if(railsSetting < 200){
    is37Battery = true;
    if(isDebug) Serial.println("Using 3.7v Battery");
  }else{
    isArduino5v = true;
    is37Battery = false;
    if(isDebug) Serial.println("Using internal 5v power source");
  }
  
  baseline = calculateBaseline(); 

  if(isDebug){
    //doTest();
  }

}

/***************   TESTING CODE  **********************/

void doTest(){

  for(int i = 0; i < 5; ++i){
    Serial.println("READY");
    delay(1000);
    Serial.println("SET");
    delay(1000);
    Serial.println("GO");
    delay(1000); 
    
    doImpactTest();
    for(int j = 0; j < 20; ++j){
      Serial.println("400.00");
    }
  }
  
}

int getAvgOfRunning(){
  int runSum = 0;
  for(int i = 0; i < runningLength; ++i){
    runningAvg[i] = analogRead(A2);
    runSum += runningAvg[i];
  }
  int avg = runSum / runningLength;
  return avg;
}

int getAvgOfRecentRunning(){
  int runSum = 0;
  for(int i = runningLength - 3; i < runningLength; ++i){
    runningAvg[i] = analogRead(A2);
    runSum += runningAvg[i];
  }
  int avg = runSum / 3;
  return avg;
}

/*******************   Main loop logic   ***********************/

void loop() {
  Smoothed <int> mySensor;
  mySensor.begin(SMOOTHED_AVERAGE, 10);
  for(int i = 0; i < runningLength; ++i){
    //mySensor.add(analogRead(A2));
    //int reading = mySensor.get();
    int reading = analogRead(A2);
    runningAvg[i] = reading;
    time_history[historyIndex] = millis();
    history[historyIndex++] = reading;
    if(historyIndex == historySize) historyIndex = 0;
    
    if(i % 3 == 0){
      doCalcs();
      String AVG = String(currentAvg);
      String MAX = String(currentMax);
      String MIN = String(currentMin);
      String toPrint = String(MIN + ", " + AVG + ", " + MAX);
      //Serial.println(toPrint);
      int dif;/*
      if(is37Battery){
        dif = localAvg - avgOfAvg;
        doBatteryImpact(dif);
      }else if(isArduino5v){
        doArduino5Impact();
      }*/
      doArduino5Impact();

      if(duringImpact){
        if(currentAvg > impactMaxAvg){
          impactMaxAvg = currentAvg;
        }
        if(currentMax > impactMaxPeak){
          impactMaxPeak = currentAvg;
        }
        if(currentMin < impactMinPeak){
          impactMinPeak = currentAvg;
        }
      }
    }
    delay(fs);
  }
  if(isDebug){
    //Serial.println("after return");
    //Serial.println(localAvg);
    //Serial.println(localMax);
  }

} 

/************* This is the hit scoring algorithm for the Arduino 5V internal power *****************************/

void doArduino5Impact(){

  int minDif = abs(avgOfMin - currentMin);
  int maxDif = abs(currentMax - avgOfMax);
  int minMaxDif = max(minDif, maxDif);

  int avgDif = avgOfAvg - currentAvg;

  if(abs(avgDif) > 60){
    impactCounter = 0;
    duringImpact = true;
    if(isDebug){
      Serial.print(impactMaxAvg);
      Serial.print(", ");
      Serial.print(currentAvg);
      Serial.print(", ");
      Serial.println(currentMax);
    }
  }else{
    impactCounter += 1;
    if(duringImpact && impactCounter > 10){
      if(impactMaxPeak < 500){
        smallImpact();
        if(isDebug)Serial.println(impactMaxAvg);
      }else if(impactMaxPeak < 750){
        mediumImpact();
        if(isDebug)Serial.println(impactMaxAvg);
      }else{
        bigImpact();
        if(isDebug)Serial.println(impactMaxAvg);
      }
      recordHistory();
      printHistory();
      doPostImpactReset();
    } 
  } 
}

/*************  This is the hit-scoring algorithm for the nice clearn 3.7 volt battery  ************************/

void doBatteryImpact(int dif){

  if(dif > 60){
    impactCounter = 0;
    duringImpact = true;
      //Serial.println("----------HIT?------------"); 
      //Serial.println(dif);
      //Serial.println(avgOfMax);
      //Serial.println(localMax);
      //to create a trace of the impact, I only want to print the localAvg
      Serial.print(currentAvg);
      Serial.print(",");
      Serial.println(currentMax);
   }else{
      impactCounter += 1;
      if(duringImpact && impactCounter > 10){ //we just ended our impact
        if(impactMaxPeak < 700){
          smallImpact();
        }else if(impactMaxPeak < 950){
          mediumImpact();
        }else{
          bigImpact();
        }
        recordHistory();
        printHistory();
        doPostImpactReset();       
      }
   }
}

/***********  The messages we print or actions we take for impact levels  *************/

void smallImpact(){
  Serial.println("WEAK");
}

void mediumImpact(){
  Serial.println("NOT BAD");
}

void bigImpact(){
  Serial.println("HULK SMASH");
}

/************ These algorithms should be unique regardless of the rail voltage **************/

void doPostImpactReset(){
  duringImpact = false;
  impactMaxAvg = avgOfAvg;
  impactMaxPeak = avgOfAvg; 
  impactMinPeak = 1023;
  impactCounter = 0;
  Serial.println();
  Serial.println();
}

void recordHistory(){
  for(int i = 0; i < 75; ++i){
    int reading = analogRead(A2);
    time_history[historyIndex] = millis();
    history[historyIndex++] = reading;
    if(historyIndex == historySize) historyIndex = 0;
    delay(fs);
  }
}

void printHistory(){
  Serial.println(RAILS_VOLTAGE);
  Smoothed <int> mySensor;
  mySensor.begin(SMOOTHED_AVERAGE, 10);
  for(int i = 0; i < historySize; ++i){
    mySensor.add(history[i]);
    int smoothed = mySensor.get();
    String SMOOTHED = String(smoothed);
    if(hasSDCard){
      String filename = "arduino_impact.csv";
      File historyFile = SD.open(filename);
      if(historyFile){
        historyFile.print(time_history[i]);
        historyFile.print(", ");
        historyFile.print(history[i]);
        historyFile.print(", ");
        historyFile.println(SMOOTHED);
      }
      historyFile.close();
    }else{
      Serial.print(time_history[i]);
      Serial.print(", ");
      Serial.print(history[i]);
      Serial.print(", ");
      Serial.println(SMOOTHED);
    }
  }
}

void doCalcs(){
  int localTotal = 0;
  int localMax = avgOfAvg;
  int localMin = avgOfAvg;
  for(int i = 0; i < runningLength; ++i){
      int reading = runningAvg[i];    
      localTotal = localTotal + reading;
      if(reading > localMax){
        localMax = reading;
      }
      if(reading < localMin){
        localMin = reading;
      }
    }
    int localAvg = localTotal / runningLength;
    currentMax = localMax;
    currentAvg = localAvg;
    currentMin = localMin;
    //Serial.println("before return");
    //Serial.println(localMin);
    //Serial.println(localAvg);
    //Serial.println(localMax);
    //Serial.println();
}

/*******************  BASELINE CALCS    ************************************************/

int calculateBaseline(){

  if(isDebug)  Serial.println("CALCULATE BASLINE");
 
  int numIterations = 15;
  int localTotal;
  int localAvg;
  int localMax;
  int totalLocalAvg = 0;
  int totalLocalMax = 0;
  int totalLocalMin = 0;
  for(int iterate = 0; iterate < numIterations; ++iterate){
    localTotal = 0;
    localMax = 0;
    localMin = 1023;
    for(int i = 0; i < runningLength; ++i){
      int reading = analogRead(A2);
      runningAvg[i] = reading;      
      localTotal = localTotal + reading;
      if(reading > localMax){
        localMax = reading;
      }
      if(reading < localMin){
        localMin = reading;
      }
      delay(fs);
    }
    int localAvg = localTotal / runningLength;
    totalLocalAvg = totalLocalAvg + localAvg;
    totalLocalMax = totalLocalMax + localMax;
    totalLocalMin = totalLocalMin + localMin;
  }

  avgOfAvg = totalLocalAvg / numIterations;
  avgOfMax = totalLocalMax / numIterations;
  avgOfMin = totalLocalMin / numIterations;

  if(isDebug){
    Serial.println(avgOfMin);
    Serial.println(avgOfAvg);
    Serial.println(avgOfMax);
  }
}

/*******************  Creates window for impact tests and prints out the values after reading  *******************/

void doImpactTest(){
  long total = 0;
  int minimum = 0;
  int maximum = 0;
  int avg = 0;
  int size1 = 300;
  int vals[size1] = {0};
  for(int i = 0; i < size1; ++i){
    int cVal = analogRead(A2);
    //delay(0.5);
    vals[i] = cVal;
    total += cVal;
    //Serial.println(cVal);
    if(cVal > avg){
      //the value we just read is greater than the avg, consider it for max
      if(cVal > maximum) maximum = cVal;
    }
    else if(cVal < avg){
      if(cVal < minimum) minimum = cVal;
    }
  }

  //print out the values
  for(int i = 0; i < size1; ++i){
    Serial.println(vals[i]);
  }

  //print out the normalized value
  Serial.println();
  Serial.println("Smoothed Data");
  Smoothed <int> mySensor;
  mySensor.begin(SMOOTHED_AVERAGE, 10); 
  for(int i = 0; i < size1; ++i){
    mySensor.add(vals[i]);
    int val = mySensor.get();
    String VAL = String(val);
    Serial.println(VAL);
  }
  
}

double rms(double *v, int n)
{
  int i;
  double sum = 0.0;
  for(i = 0; i < n; i++)
    sum += v[i] * v[i];
  return sqrt(sum / n);
}
