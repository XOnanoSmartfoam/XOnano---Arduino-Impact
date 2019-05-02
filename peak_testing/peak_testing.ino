#include <stdlib.h>
#include <math.h>
#define SAMPLE_LENGTH 100


void setup() {
  // put your setup code here, to run once:
  Serial.begin (9600);
  analogReference(DEFAULT);
  
  Serial.println("STARTED");

  float vals[SAMPLE_LENGTH] = {};

  for(int i = 0; i < SAMPLE_LENGTH; ++i){

    vals[i] = analogRead(A2);
    
  }

  Serial.println("A");

  int signals[SAMPLE_LENGTH] = {0};
  //memset(signals, 0, sizeof(float) * SAMPLE_LENGTH);

  Serial.println("A2");

  thresholding(vals, signals, 5, 5.0, 0.0);

  Serial.println("finished threshold");
  

}

void loop() {
  // put your main code here, to run repeatedly:

}

void thresholding(float y[], int signals[], int lag, float threshold, float influence) {
    Serial.println("B");
    
    memset(signals, 0, sizeof(float) * SAMPLE_LENGTH);
    float filteredY[SAMPLE_LENGTH];
    memcpy(filteredY, y, sizeof(float) * SAMPLE_LENGTH);
    float avgFilter[SAMPLE_LENGTH];
    float stdFilter[SAMPLE_LENGTH];

    Serial.println("C");
    
    avgFilter[lag - 1] = mean(y, lag);
    stdFilter[lag - 1] = stddev(y, lag);
    Serial.println("C2");
    
    for (int i = lag; i < SAMPLE_LENGTH; i++) {
        if (fabsf(y[i] - avgFilter[i-1]) > threshold * stdFilter[i-1]) {
          /*
          Serial.println("C2-1");
            if (y[i] > avgFilter[i-1]) {
                signals[i] = 1;
            } else {
                signals[i] = -1;
            }
            Serial.println("C2-2");
            filteredY[i] = influence * y[i] + (1 - influence) * filteredY[i-1];
            */
        } else {
            signals[i] = 0;
        }
        Serial.println("C2-3");
        avgFilter[i] = mean(filteredY + i-lag, lag);
        stdFilter[i] = stddev(filteredY + i-lag, lag);
    }
    
    Serial.println("D");
}

float mean(float data[], int len) {
    float sum = 0.0, mean = 0.0;

  Serial.println("M1");

    int i;
    for(i=0; i<len; ++i) {
        sum += data[i];
    }

    mean = sum/len;

    Serial.println("M2");
    
    return mean;


}

float stddev(float data[], int len) {

    Serial.println("S1");
  
    float the_mean = mean(data, len);
    float standardDeviation = 0.0;

    int i;
    for(i=0; i<len; ++i) {
        standardDeviation += pow(data[i] - the_mean, 2);
    }

    Serial.println("S2");

    return sqrt(standardDeviation/len);
}
