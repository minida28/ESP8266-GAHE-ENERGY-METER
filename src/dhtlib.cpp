#include <Arduino.h>

#include "dhtlib.h"
#include "mqtt.h"

#define DHTPIN ESP_PIN_4 //D2 wemos     //equalto pin 4 (GPIO4), what digital pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
DHT dht(DHTPIN, DHTTYPE);

boolean measureDHT = 0;

const int intervalDHT = 15000;
unsigned long previousMillisDHT;

char bufHumidity[10];
char bufTemperature[10];
char bufHeatIndex[10];

int countTemp;

float sum_t;
float sum_f;
float sum_h;




void dht_loop() {
  if (measureDHT == HIGH) {

    if (millis() - previousMillisDHT >= intervalDHT) {
      // Read temperature as Celsius (the default)
      float t = dht.readTemperature();
      // Read temperature as Fahrenheit (isFahrenheit = true)
      float f = dht.readTemperature(true);
      // Read humidity
      float h = dht.readHumidity();

      // Check if any reads failed and exit early (to try again).
      if (isnan(h) || isnan(t) || isnan(f)) {
        Serial1.println("Failed to read from DHT sensor!");
        return;
      }

      // If reading is good (return a number), then publish the reading to mqtt broker
      else if (!isnan(h) || !isnan(t) || !isnan(f)) {


        sum_t += t;
        sum_f += f;
        sum_h += h;   // x += y;   equivalent to the expression x = x + y;
        ++countTemp;  // ++x;   increment x by one and returns the new value of x


        /*
          sum_t = sum_t + t;
          sum_f = sum_f + f;
          sum_h = sum_h + h;
          countTemp = countTemp + 1;
        */

        previousMillisDHT = millis();

        return;
      }
    }

    if (countTemp == 4) {

      float avrg_t = sum_t / countTemp;
      float avrg_f = sum_f / countTemp;
      float avrg_h = sum_h / countTemp;

      // reset counter and sum value
      countTemp = 0;
      sum_t = 0.0;
      sum_f = 0.0;
      sum_h = 0.0;

      // Compute heat index in Fahrenheit (the default)
      float hif = dht.computeHeatIndex(avrg_f, avrg_h);
      // Compute heat index in Celsius (isFahreheit = false)
      float hic = dht.computeHeatIndex(avrg_t, avrg_h, false);

      /* Publish Humidity */
      dtostrf(avrg_h, 1, 1, bufHumidity);
      mqttClient.publish("/rumah/sts/kwh1/Humidity", 0, 0, bufHumidity);
      //Serial1.print(", ");
      //Serial1.print("Humidity:");
      //Serial1.println(bufHumidity);

      /* Publish Temperature in Celcius*/
      dtostrf(avrg_t, 1, 1, bufTemperature);
      mqttClient.publish("/rumah/sts/kwh1/Temperature", 0, 0, bufTemperature);
      //Serial1.print(", ");
      //Serial1.print("Humidity:");
      //Serial1.println(bufHumidity);

      /* Publish Heat Index in Celcius*/
      dtostrf(hic, 1, 1, bufHeatIndex);
      mqttClient.publish("/rumah/sts/kwh1/HeatIndex", 0, 0, bufHeatIndex);
      //Serial1.print(", ");
      //Serial1.print("HeatIndex:");
      //Serial1.println(bufHeatIndex);

      Serial1.print("Humidity: ");
      Serial1.print(avrg_h);
      Serial1.print(" %\t");
      Serial1.print("Temperature: ");
      Serial1.print(avrg_t);
      Serial1.print(" *C ");
      Serial1.print(avrg_f);
      Serial1.print(" *F\t");
      Serial1.print("Heat index: ");
      Serial1.print(hic);
      Serial1.print(" *C ");
      Serial1.print(hif);
      Serial1.println(" *F");

    }
  }
  else if (measureDHT != 1) {

    String stringNA = "N/A";
    stringNA.toCharArray(bufTemperature, 4);
    stringNA.toCharArray(bufHumidity, 4);
    stringNA.toCharArray(bufHeatIndex, 4);

  }
}

void dht_setup() {
  dht.begin();
}

