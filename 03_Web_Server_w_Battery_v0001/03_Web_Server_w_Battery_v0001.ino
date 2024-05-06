/*
  Web Server with Battery level checker.

  Based on these tutorials:
  Setup webserver: https://docs.arduino.cc/tutorials/mkr-wifi-1010/hosting-a-webserver/
  Setup SD card: https://docs.arduino.cc/tutorials/mkr-sd-proto-shield/mkr-sd-proto-shield-data-logger/
 */

#include <SPI.h>
#include <WiFiNINA.h>
#include <BQ24195.h>
#include <SPI.h>
#include <SD.h>

float rawADC;
float voltADC;
float voltBat;

float R1 =  330000.0;
float R2 = 1000000.0;

float max_Source_voltage;
float batteryFullVoltage = 4.4;
float batteryEmptyVoltage = 3.3;
float batteryCapacity = 2.0;


#include <Arduino_MKRENV.h>
float temperature = 0;
float humidity = 0;
float pressure = 0;

#include "arduino_secrets.h" 
//please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID; // your network SSID (name)
char pass[] = SECRET_PASS; // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;          // your network key Index number (needed only for WEP)
int status = WL_IDLE_STATUS;
WiFiServer server(80);

bool serial=true; // 'true' if we connect arduino to usb port on laptop

void setup() {

  //Initialize serial and wait for port to open:
  if (serial==true) {
    Serial.begin(9600);
    while (!Serial) {
      ; // wait for serial port to connect. Needed for native USB port only
    }
  }

  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    if (serial==true) {
      Serial.println("Communication with WiFi module failed!");
    }
    // don't continue
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    if (serial==true) {
      Serial.println("Please upgrade the firmware");
    }
  }

  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) {
    if (serial==true) {
      Serial.print("Attempting to connect to SSID: ");
      Serial.println(ssid);
    }
    //Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(10000);
  }
  server.begin();
  // you're connected now, so print out the status:
  if (serial==true) {
    printWifiStatus();
  }

  // Initialize MKR ENV Shield
  if (!ENV.begin()) {
    if (serial==true) {
      Serial.println("Failed to initialize MKR ENV shield!");
    }
    while (1);
  }

  // Battery settings
  // ensure the analog reference is set to the default value
  // of 3.3V (analogReference(AR_Default)) and set the ADC
  // resolution to 12 bits (analogReadResolution(12)).
  // A 12bit ADC will provide an output from 0 (0V) to 4093
  // (3.3V), depending on the voltage the ADC experiences.
  analogReference(AR_DEFAULT);
  analogReadResolution(12);
  PMIC.begin();
  PMIC.enableBoostMode();
  PMIC.setMinimumSystemVoltage(batteryEmptyVoltage);
  PMIC.setChargeVoltage(batteryFullVoltage);
  PMIC.setChargeCurrent(batteryCapacity/2);
  PMIC.enableCharge();
  max_Source_voltage = (3.3 * (R1 + R2))/R2;
}


void loop() {
  // listen for incoming clients
  WiFiClient client = server.available();
  
  if (client) {
    if (serial==true) {
      Serial.println("new client");
    }

    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        if (serial==true) {
          Serial.write(c);
        }
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          client.println("Refresh: 5");  // refresh the page automatically every 5 sec
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<html>");
          // output the value of each analog input pin
          // for (int analogChannel = 0; analogChannel < 6; analogChannel++) {
          //   int sensorReading = analogRead(analogChannel);
          //   client.print("analog input ");
          //   client.print(analogChannel);
          //   client.print(" is ");
          //   client.print(sensorReading);
          //   client.println("<br />");
          // }
          // output sensor values
          temperature = ENV.readTemperature();
          client.print("Temperature is: ");
          client.print(temperature);
          client.println("<br />");
          humidity = ENV.readHumidity();
          client.print("Humidity is: ");
          client.print(humidity);
          client.println("<br />");
          pressure = ENV.readPressure();
          client.print("Pressure is (kPa): ");
          client.print(pressure);
          client.println("<br />");

          // check battery status
          rawADC = analogRead(ADC_BATTERY);
          voltADC = rawADC * (3.3/4095.0);
          voltBat = voltADC * (max_Source_voltage/3.3);
          int new_batt = (voltBat - batteryEmptyVoltage) * (100) / (batteryFullVoltage - batteryEmptyVoltage);
          client.print("Battery status:");
          client.println("<br />");
          client.print("The ADC on PB09 reads a value of ");
          client.print(rawADC);
          client.print(" which is equivialent to ");
          client.print(voltADC);
          client.print("V. This means the battery voltage is ");
          client.print(voltBat);
          client.print("V. Which is equivalent to a charge level of ");
          client.print(new_batt);
          client.println("%.");
          client.println("<br />");

          client.print("DEBUG: max_Source_voltage = ");
          client.print(max_Source_voltage);
          client.println("<br />");

          client.println("</html>");
          break;
          //delay(500);
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        } else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
      
    }
    // give the web browser time to receive the data
    delay(1);

    // close the connection:
    client.stop();
    if (serial==true) {
      Serial.println("client disconnected");
    }
  }
}


void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}