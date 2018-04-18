#include <MySignals.h>
#include "Wire.h"
#include "SPI.h"
void(* resetFunc) (void) = 0;

/**Declaring variables*/
int signal;
uint8_t statusPulsioximeter;
void setup()
{
  Serial.begin(115200);
  MySignals.begin();
}

void loop()
{
  /**Waiting for inpur signal that comes from node js
  */
  while (!Serial.available());
  signal = Serial.read();
  /**for measuring temperature
  */
  if (signal == 6) {
    float temperature = MySignals.getCalibratedTemperature(100, 10, -3.4, TEMPERATURE);
    float temp = (((temperature) * 9) / 5) + 32;
    Serial.println(temp);
  }
  /**for measuring glucose
  */
  if (signal == 2) {
    MySignals.initSensorUART();
    MySignals.enableSensorUART(GLUCOMETER);
    delay(1000);
    MySignals.getGlucose();
    Serial.begin(115200);
    MySignals.disableMuxUART();
    for (uint8_t i = 0; i < MySignals.glucoseLength; i++)
    {
      if (MySignals.glucometerData[i].glucose) {
        Serial.println(MySignals.glucometerData[i].glucose);
        delay(1000);
        resetFunc();
      } else if (!MySignals.glucometerData[i].glucose) {
        delay(1000);
        resetFunc();
      }
    }
    MySignals.enableMuxUART();
  }
  /**for measuring blood presure
  */
  if (signal == 3) {
    MySignals.initSensorUART();
    MySignals.enableSensorUART(BLOODPRESSURE);
    for (int i = 0; i < 6; i++) {
      if (MySignals.getStatusBP())
      {
        delay(1000);
        if (MySignals.getBloodPressure() == 1)
        {
          MySignals.disableMuxUART();
          if (MySignals.bloodPressureData.diastolic && MySignals.bloodPressureData.systolic) {
            Serial.println(MySignals.bloodPressureData.diastolic);
            Serial.println("A");
            Serial.println(MySignals.bloodPressureData.systolic);
            delay(1000);
            resetFunc();
          } else if (!MySignals.bloodPressureData.diastolic || !MySignals.bloodPressureData.systolic) {
            delay(1000);
            resetFunc();
          }
          MySignals.enableMuxUART();
        } else if (MySignals.getBloodPressure() != 1) {
          delay(1000);
          resetFunc();
        }
      } else if (!MySignals.getStatusBP()) {
        delay(1000);
        resetFunc();
      }
      delay(1000);
    }
  }
  /**for measuring spo2
  */
  if (signal == 4) {
    MySignals.initSensorUART();
    MySignals.enableSensorUART(PULSIOXIMETER_MICRO);
    statusPulsioximeter = MySignals.getPulsioximeterMicro();
    Serial.print(statusPulsioximeter);
    if (statusPulsioximeter == 1)
    {
      Serial.print("A");
      Serial.print(MySignals.pulsioximeterData.BPM);
      Serial.print("A");
      Serial.print(MySignals.pulsioximeterData.O2);
    }
  }
  /**for measuring gsr
  */
  if (signal == 9) {
    float resistance = MySignals.getGSR(RESISTANCE);
    Serial.print(resistance, 2);
  }
  /**for measuring spirometer
  */
  if (signal == 7) {
    int count = 0;
    MySignals.initSensorUART();
    MySignals.enableSensorUART(SPIROMETER);

    while (MySignals.getStatusSpiro() == 0)
    {
      delay(100);
    }
    MySignals.deleteSpiroMeasures();

    for (int i = 0; i < 30; i++) {
      count++;
      if (count == 30) {
        resetFunc();
      }
      MySignals.getSpirometer();
      MySignals.disableMuxUART();

      for (int i = 0; i < MySignals.spir_measures; i++)
      {
        Serial.print(MySignals.spirometerData[i].spir_pef);
        Serial.print("A");
        Serial.println(MySignals.spirometerData[i].spir_fev);
        delay(500);
        resetFunc();
      }
      MySignals.enableMuxUART();
      delay(2000);
    }
  }
}
