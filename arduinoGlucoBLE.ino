#include <MySignals.h>
#include "Wire.h"
#include "SPI.h"
#include <MySignals_BLE.h>

void(* resetFunc) (void) = 0;

#define GLUCO_HANDLE1 99
#define GLUCO_HANDLE2 83

// Write here the MAC address of BLE device to find
char MAC_GLUCO[14] = "187A93090C89";
boolean controlloop = false;
uint8_t available_gluco = 0;
uint8_t connected_gluco = 0;
uint8_t connection_handle_gluco = 0;

//!Struct to store data of the glucometer.
struct glucometerBLEDataVector
{
  uint16_t glucose;
  uint8_t info;
};

//!Vector to store the glucometer measures and dates.
glucometerBLEDataVector glucometerBLEData;


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
    //    MySignals.initSensorUART();
    //    MySignals.enableSensorUART(GLUCOMETER);
    //    delay(1000);
    //    MySignals.getGlucose();
    //    Serial.begin(115200);
    //    MySignals.disableMuxUART();
    //    for (uint8_t i = 0; i < MySignals.glucoseLength; i++)
    //    {
    //      if (MySignals.glucometerData[i].glucose) {
    //        Serial.println(MySignals.glucometerData[i].glucose);
    //        delay(1000);
    //        resetFunc();
    //      } else if (!MySignals.glucometerData[i].glucose) {
    //        delay(1000);
    //        resetFunc();
    //      }
    //    }
    //    MySignals.enableMuxUART();

    controlloop = true;
    MySignals.initSensorUART();
    MySignals.enableSensorUART(BLE);

    //Enable BLE module power -> bit6: 1
    bitSet(MySignals.expanderState, EXP_BLE_POWER);
    MySignals.expanderWrite(MySignals.expanderState);

    //Enable BLE UART flow control -> bit5: 0
    bitClear(MySignals.expanderState, EXP_BLE_FLOW_CONTROL);
    MySignals.expanderWrite(MySignals.expanderState);


    //Disable BLE module power -> bit6: 0
    bitClear(MySignals.expanderState, EXP_BLE_POWER);
    MySignals.expanderWrite(MySignals.expanderState);

    delay(500);

    //Enable BLE module power -> bit6: 1
    bitSet(MySignals.expanderState, EXP_BLE_POWER);
    MySignals.expanderWrite(MySignals.expanderState);
    delay(1000);

    MySignals_BLE.initialize_BLE_values();

    if (MySignals_BLE.initModule() == 1)
    {

      if (MySignals_BLE.sayHello() == 1)
      {
        MySignals.println("BLE init ok 1");
      }
      else
      {
        MySignals.println("BLE init fail");

        while (1)
        {
        };
      }
    }
    else
    {
      MySignals.println("BLE init fail");

      while (1)
      {
      };
    }

    while (available_gluco != 1 && controlloop == true  ) {

      available_gluco = MySignals_BLE.scanDevice(MAC_GLUCO, 1000, TX_POWER_MAX);

      MySignals.disableMuxUART();
      Serial.print(F("Gluco available: 111"));
      Serial.println(available_gluco);
      MySignals.enableMuxUART();


      if (available_gluco == 1)
      {

        if (MySignals_BLE.connectDirect(MAC_GLUCO) == 1)
        {
          MySignals.println("Connected device 222");
          connected_gluco = 1;

          uint8_t gluco_subscribe_message[2] = { 0x01 , 0x00 };
          delay(200);

          MySignals_BLE.attributeWrite(MySignals_BLE.connection_handle, GLUCO_HANDLE1, gluco_subscribe_message, 2);
          MySignals_BLE.attributeWrite(MySignals_BLE.connection_handle, GLUCO_HANDLE2, gluco_subscribe_message, 2);

          delay(200);
          MySignals.println("Insert blood stripe (40s) 333");

          unsigned long previous = millis();
          do
          {
            Serial.println(444);
            if (MySignals_BLE.waitEvent(1000) == BLE_EVENT_ATTCLIENT_ATTRIBUTE_VALUE)
            {
              MySignals.disableMuxUART();
              Serial.print(F("Glucose(mg/dl):"));

              if (MySignals_BLE.event[8] == 0x0c)
              {
                uint8_t gh = MySignals_BLE.event[12] - 48;
                uint8_t gl = MySignals_BLE.event[13] - 48;
                glucometerBLEData.glucose = (gh * 10) + gl;
                glucometerBLEData.info = 0;

                Serial.println(glucometerBLEData.glucose);
              }
              if (MySignals_BLE.event[8] == 0x0d)
              {

                uint8_t gh = MySignals_BLE.event[12] - 48;
                uint8_t gm = MySignals_BLE.event[13] - 48;
                uint8_t gl = MySignals_BLE.event[14] - 48;
                glucometerBLEData.glucose = (gh * 100) + (gm * 10) + gl;
                glucometerBLEData.info = 0;

                Serial.println(glucometerBLEData.glucose);
              }
              if (MySignals_BLE.event[8] == 0x0e)
              {
                if (MySignals_BLE.event[12] == 0x4c)
                {
                  glucometerBLEData.glucose = 0;
                  glucometerBLEData.info = 0xAA;

                  Serial.println(F("Low glucose"));
                }
                else if (MySignals_BLE.event[12] == 0x48)
                {
                  glucometerBLEData.glucose = 360;
                  glucometerBLEData.info = 0xBB;

                  Serial.println(F("High glucose"));

                }
              }
              MySignals_BLE.disconnect(MySignals_BLE.connection_handle);
              connected_gluco = 0;
              available_gluco = 0;
              controlloop = false;
            }
          }
          while ((connected_gluco == 1) && ((millis() - previous) < 40000)); //Timeout 40 seconds
          //Serial.println("while loop exit");
          connected_gluco = 0;
          available_gluco = 0;
          controlloop = false;
        }
        else
        {
          connected_gluco = 0;
          available_gluco = 0;
          controlloop = false;
          MySignals.println("Not Connected ");
        }


      }
      else if (available_gluco == 0)
      {
        //Serial.println("available_gluco == 0");
      }
      else
      {
        MySignals_BLE.hardwareReset();
        MySignals_BLE.initialize_BLE_values();
        delay(100);

      }
      delay(1000);
    }
    //Serial.println("Disconnected");
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
    //    MySignals.initSensorUART();
    //    MySignals.enableSensorUART(PULSIOXIMETER_MICRO);
    //    statusPulsioximeter = MySignals.getPulsioximeterMicro();
    //    Serial.print(statusPulsioximeter);
    //    if (statusPulsioximeter == 1)
    //    {
    //      Serial.print("A");
    //      Serial.print(MySignals.pulsioximeterData.BPM);
    //      Serial.print("A");
    //      Serial.print(MySignals.pulsioximeterData.O2);
    //    }
    MySignals.initSensorUART();
    MySignals.enableSensorUART(PULSIOXIMETER_MICRO);
    //MySignals.enableSensorUART(PULSIOXIMETER);
    for (int i = 0; i <= 20; i++) {
      delay(10);
      uint8_t getPulsioximeterMicro_state = MySignals.getPulsioximeterMicro();

      if (getPulsioximeterMicro_state == 1)
      {
        String spo2data = "A" + String(MySignals.pulsioximeterData.BPM) + "A" + String(MySignals.pulsioximeterData.O2) + "A" + String(i);
        Serial.println(spo2data);

      }

      delay(500);
    }
    MySignals.disableMuxUART();




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