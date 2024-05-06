/*
  00 Simple Temp Reader v0001

  Read information from the temperature sensor on the MKR ENV Shield
  and send it back to the computer via the USB Cable

  (c) 2019 D. Cuartielles for Arduino

  This code is Free Software licensed under GPLv3
*/

#include <Arduino_MKRENV.h>

float temperature = 0;
float humidity = 0;

void setup() {
  Serial.begin(9600);
  while (!Serial);

  if (!ENV.begin()) {
    Serial.println("Failed to initialize MKR ENV shield!");
    while (1);
  }
}

void loop() {
  // read the sensor value
  temperature = ENV.readTemperature();
  humidity = ENV.readHumidity()/10;

  // print each of the sensor values
  Serial.print(temperature);
  Serial.print(",");
  Serial.println(humidity);

  // wait 1 second to print again
  delay(1000);
}