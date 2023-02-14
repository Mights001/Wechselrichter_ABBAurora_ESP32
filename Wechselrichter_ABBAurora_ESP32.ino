
#include <WiFiManager.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <ArduinoJson.h>
#include "index_html.h"

#include <ABBAurora.h>


//Json 
StaticJsonDocument<1024> jsonDocument, shellydaten;
char buffer[1024];



//RS485 Schnittstelle 
#define RX2 16
#define TX2 17
#define INVERTER_ADDRESS 2
#define TX_CONTROL_GPIO 4
ABBAurora *inverter;



//Webserver Initialisieren 
WebServer server(80);
const char* host = "esp32";
float l_DAY ,l_WEEK, l_MONTH,l_YEAR, l_MPP1, l_MPP2;


//Setup
void setup() {
  

  
 WiFiManager manager;    

    bool success = manager.autoConnect("ESP32_AP","");
 
    if(!success) {
        Serial.println("Failed to connect");
    } 
    else {
        Serial.println("Connected");
    }



    Serial.begin(115200);
    ABBAurora::setup(Serial2, RX2, TX2, TX_CONTROL_GPIO);
    inverter = new ABBAurora(INVERTER_ADDRESS);
    Serial.println("Setup done");


  /*use mdns for host name resolution*/
  if (!MDNS.begin(host)) { //http://esp32.local
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }

  
setup_routing(); 

}

  
void loop() {


 server.handleClient();  
 
    if (inverter->ReadVersion())
    {
        Serial.print("Inverter Name: ");
        Serial.println(inverter->Version.Par1);
    }
    else
    {
        Serial.print("Inverter could not be reached");
        delay(500);
        return;
    }

    if (inverter->ReadDSPValue(POWER_IN_1, MODULE_MESSUREMENT))
    {
        Serial.print("Power in MPP1 : ");
        l_MPP1 = inverter->CumulatedEnergy.Energy / 1000 ; 
        Serial.print(l_MPP1);
        Serial.println(" KW");
    }

    if (inverter->ReadDSPValue(POWER_IN_2, MODULE_MESSUREMENT))
    {
        Serial.print("Power in MPP2 : ");
        l_MPP2 = inverter->CumulatedEnergy.Energy / 1000 ; 
        Serial.print(l_MPP2);
        Serial.println(" KW");
    }
    
    if (inverter->ReadCumulatedEnergy(CURRENT_DAY))
    {
        Serial.print("Energy Day: ");
        l_DAY = inverter->CumulatedEnergy.Energy / 1000 ; 
        Serial.print(l_DAY);
        Serial.println(" KwH");
    }

       if (inverter->ReadCumulatedEnergy(CURRENT_WEEK))
    {
        Serial.print("Energy Week: ");
        l_WEEK = inverter->CumulatedEnergy.Energy / 1000 ; 
        Serial.print(l_WEEK);
        Serial.println(" KwH");
    }


       if (inverter->ReadCumulatedEnergy(CURRENT_MONTH))
    {
        Serial.print("Energy Month: ");
        l_MONTH = inverter->CumulatedEnergy.Energy / 1000 ; 
        Serial.print(l_MONTH);
        Serial.println(" KwH");
    }

    if (inverter->ReadCumulatedEnergy(CURRENT_YEAR))
    {
        Serial.print("Energy Year: ");
        l_YEAR = inverter->CumulatedEnergy.Energy / 1000 ; 
        Serial.print(l_YEAR);
        Serial.println(" KwH");
    }

 
delay(100);
 

}



void setup_routing() {
  //server.on("/temperature", getTemperature);
  //server.on("/pressure", getPressure);
  //server.on("/humidity", getHumidity);
  
  server.on("/env", getEnv);
  server.on("/", sendhtmlindex);
  //server.on("/hand", hand);
 
  
  //server.on("/getSunset", getSunset);
  
  // Send a GET request to <ESP_IP>/get?diff=<inputMessage>
  server.on("/get", HTTP_GET, []() {
    /*
    if( server.hasArg("diff")) {
    
    String argument = server.arg("diff");
    
    Diff = argument.toFloat(); 
    server.send(200, "text/html", index_html);
    Serial.println(argument);
    }
*/

  });

  server.on("/firmware", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", firmwarepage);
  });



    /*handling uploading firmware file */
  server.on("/update", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("Update: %s\n", upload.filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      /* flashing firmware to ESP*/
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { //true to set the size to the current progress
        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
    }
  });

  // start server
  server.begin();
}

  /*
void getTemperature() {


    Serial.println("Get temperature");
  create_json("temperature", lastAussen.temperature, "°C");
  server.send(200, "application/json", buffer);
}
 
void getHumidity() {
  Serial.println("Get humidity");
  create_json("humidity", lastAussen.humidity, "%");
  server.send(200, "application/json", buffer);
}
 
void getPressure() {
  Serial.println("Get pressure");
  create_json("pressure", 0, "mBar");
  server.send(200, "application/json", buffer);
}
 */
 
void getEnv() {
  Serial.println("Get env");
  jsonDocument.clear(); // Clear json buffer
  
  add_json_object("Strom Tag", round(l_DAY), "KwH");
  add_json_object("Strom Woche", round(l_WEEK), "KwH");
  add_json_object("Strom Monat", round(l_MONTH), "KwH");
  add_json_object("Strom Jahr", round(l_YEAR), "KwH");
  add_json_object("Strom akt. MPP1", round(l_MPP1), "Kw");
  add_json_object("Strom akt. MPP2", round(l_MPP2), "Kw");
  add_json_object("Strom akt. Gesamt", round((l_MPP1 + l_MPP2)), "Kw");
  
 /*
  add_json_object("Aussentemperatur", lastAussen.temperature, "°C");
  add_json_object("Luftfeuchte Innen", lastRaum.humidity, "%");
  add_json_object("Innentemperatur", lastRaum.temperature, "°C");
  add_json_object("Taupunktdifferenz", DiffTautemp, "°C");
  add_json_object("Taupunktaussen", TAussen.temp, "°C");
  add_json_object("Taupunktinnen", TRaum.temp, "°C");
  add_json_object("Diff Einstellung", Diff, "°C");
  add_json_object("Start Luftfeuchte", starthumi, "%");
  add_json_object("Min Innentemperatur", MinTemp, "°C");  
  add_json_object("Luefter ein ", luefterein, "Bool");
  */
  serializeJson(jsonDocument, buffer);
  server.send(200, "application/json", buffer);
}

void create_json(char *tag, float value, char *unit) {  
  jsonDocument.clear();
  jsonDocument["type"] = tag;
  jsonDocument["value"] = value;
  jsonDocument["unit"] = unit;
  serializeJson(jsonDocument, buffer);  
}
 
void add_json_object(char *tag, float value, char *unit) {
  JsonObject obj = jsonDocument.createNestedObject();
  obj["type"] = tag;
  obj["value"] = value;
  obj["unit"] = unit; 
}

void sendhtmlindex() {
  Serial.println("Index");
  server.send(200, "text/html", index_html);
}



float round(float var)
{
    // 37.66666 * 100 =3766.66
    // 3766.66 + .5 =3767.16    for rounding off value
    // then type cast to int so value is 3767
    // then divided by 100 so the value converted into 37.67
    float value = (int)(var * 100 + .5);
    return (float)value / 100;
}

/*
void hand(){
   if( server.hasArg("hand")) {
    
    if(server.arg("hand") == "true"){
      varhand = true;      
      };
      if(server.arg("hand") == "false"){
      varhand = false;      
      };
    server.send(200, "text/html", index_html);
    Serial.println(varhand);
    }
  
  }
*/
