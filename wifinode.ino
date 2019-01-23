
#define BLYNK_PRINT Serial
#include <ESP8266_Lib.h>
#include <BlynkSimpleShieldEsp8266.h>
#include <SoftwareSerial.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "DHT.h"
#include <stdlib.h>

char auth[] = "7b20ed7c49074c3ab791ca9c690fd487";
char ssid[] = "ooredoo_CE3188"; //your wifi ssid here 
char pass[] = "00000000";       //your wifi password here 
// replace with your channel's thingspeak API key
String apiKey = "F7JOV5JDG2D61N4F";
// LED 
int ledPin = 2;
//int sensor_pin=A0;  // variable for sensor
//float sample=0;
float bat_volt =45;     // for temperature
//#define EspSerial Serial
// or Software Serial on Uno, Nano...
SoftwareSerial ser(8,7); // RX, TX
#define ESP8266_BAUD 115200
ESP8266 wifi(&EspSerial);
// Thingspeak  
String statusChWriteKey = "672022";  // Status Channel id: 999999
BlynkTimer timer;
//#define HARDWARE_RESET 8
// DS18B20
#define ONE_WIRE_BUS 5 // DS18B20 on pin D5 
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);
int soilTemp = 0;
//DHT
int pinoDHT = 11;
int tipoDHT =  DHT11;
DHT dht(pinoDHT, tipoDHT); 
int airTemp = 0;
int airHum = 0;
// LDR (Light)
#define ldrPIN 2
int light = 0;
// Soil humidity
#define soilHumPIN A0
int soilHum = 0;
// Variables to be used with timers
long writeTimingSeconds = 17; // ==> Define Sample time in seconds to send data
long startWriteTiming = 0;
long elapsedWriteTime = 0;
// Variables to be used with Actuators
boolean pump = 0; 
boolean lamp = 0; 
int spare = 0;
boolean error;

void setup()
{
  Serial.begin(9600);
 // pinMode(HARDWARE_RESET,OUTPUT);
  
  //digitalWrite(HARDWARE_RESET, HIGH);
  //EspHardwareReset(); //Reset do Modulo WiFi
    Serial.println(airTemp);

   // Set ESP8266 baud rate
  EspSerial.begin(ESP8266_BAUD);
  delay(10);
  DS18B20.begin();
  dht.begin();

  Blynk.begin(auth, wifi, ssid, pass);
  
  startWriteTiming = millis(); // starting the "program clock"
   // Setup a function to be called every second
  timer.setInterval(1000L, readSensors);


}

void loop(){
  
  Blynk.run();
  timer.run();
  start: //label 
  error=0;
  
  elapsedWriteTime = millis()-startWriteTiming; 
  
  if (elapsedWriteTime > (writeTimingSeconds*1000)) 
  {
    readSensors();
    writeThingSpeak();
    startWriteTiming = millis();   
  }
  
  if (error==1) //Resend if transmission is not completed 
  {       
    Serial.println(" <<<< ERROR >>>>");
    delay (2000);  
    goto start; //go to label "start"
  }
}

/********* Read Sensors value *************/
void readSensors(void)
{
  airTemp = dht.readTemperature();
  airHum = dht.readHumidity();

  DS18B20.requestTemperatures(); 
  soilTemp = DS18B20.getTempCByIndex(0); // Sensor 0 will capture Soil Temp in Celcius
             
  light = map(analogRead(ldrPIN), 1023, 0, 0, 100); //LDRDark:0  ==> light 100%  
  soilHum = map(analogRead(soilHumPIN), 1023, 0, 0, 100); 

  Blynk.virtualWrite(V5, airTemp);
    Blynk.virtualWrite(V6, soilTemp);
  Blynk.virtualWrite(V7, light);
    Blynk.virtualWrite(V8, soilHum);
  
  
  
    Serial.println(airTemp);
    Serial.println(soilTemp);
    Serial.println(light);
    Serial.println(soilHum);



}

/********* Conexao com TCP com Thingspeak *******/
void writeThingSpeak(void)
{

  startThingSpeakCmd();

 // prepare GET string
  String getStr = "GET /update?api_key=";
  getStr += apiKey;
  getStr += statusChWriteKey;
  getStr +="&field1=";
  getStr += String(pump);
  getStr +="&field2=";
  getStr += String(lamp);
  getStr +="&field3=";
  getStr += String(airTemp);
  getStr +="&field4=";
  getStr += String(airHum);
  getStr +="&field5=";
  getStr += String(soilTemp);
  getStr +="&field6=";
  getStr += String(soilHum);
  getStr +="&field7=";
  getStr += String(light);
  getStr +="&field8=";
  getStr += String(spare);
  getStr += "\r\n\r\n";

  sendThingSpeakGetCmd(getStr); 
}

/********* Reset ESP *************/
void EspHardwareReset(void)
{
  Serial.println("Reseting......."); 
  digitalWrite(HARDWARE_RESET, LOW); 
  delay(500);
  digitalWrite(HARDWARE_RESET, HIGH);
  delay(8000);//Tempo necessário para começar a ler 
  Serial.println("RESET"); 
}

/********* Start communication with ThingSpeak*************/
void startThingSpeakCmd(void)
{
  EspSerial.flush();//limpa o buffer antes de começar a gravar
  
  String cmd = "AT+CIPSTART=\"TCP\",\"";
  cmd += "184.106.153.149"; // Endereco IP de api.thingspeak.com
  cmd += "\",80";
  EspSerial.println(cmd);
  Serial.print("enviado ==> Start cmd: ");
  Serial.println(cmd);

  if(EspSerial.find("Error"))
  {
    Serial.println("AT+CIPSTART error");
    return;
  }
}

/********* send a GET cmd to ThingSpeak *************/
String sendThingSpeakGetCmd(String getStr)
{
  String cmd = "AT+CIPSEND=";
  cmd += String(getStr.length());
  EspSerial.println(cmd);
  Serial.print("enviado ==> lenght cmd: ");
  Serial.println(cmd);

  if(EspSerial.find((char *)">"))
  {
    EspSerial.print(getStr);
    Serial.print("enviado ==> getStr: ");
    Serial.println(getStr);
    delay(500);//tempo para processar o GET, sem este delay apresenta busy no próximo comando

    String messageBody = "";
    while (EspSerial.available()) 
    {
      String line = EspSerial.readStringUntil('\n');
      if (line.length() == 1) 
      { //actual content starts after empty line (that has length 1)
        messageBody = EspSerial.readStringUntil('\n');
      }
    }
    Serial.print("MessageBody received: ");
    Serial.println(messageBody);
    return messageBody;
  }
  else
  {
    EspSerial.println("AT+CIPCLOSE");     // alert user
    Serial.println("ESP8266 CIPSEND ERROR: RESENDING"); //Resend...
    spare = spare + 1;
    error=1;
    return "error";
  } 
}
