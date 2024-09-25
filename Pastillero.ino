#include <Arduino.h>

#include <WiFi.h>
#include <HTTPClient.h>

#include <ArduinoJson.h>

#include <LiquidCrystal_I2C.h>
#include <DHT.h>//https://github.com/adafruit/DHT-sensor-library

#include <time.h>
#include <string.h>

#define buzzer 12

#define ledG 16
#define ledR 4
#define ledB 17

#define DHTPIN 19
#define DHTTYPE DHT11

int pin_PWM = 15;
int ledc_channel_0 = 0;
int ledc_channel_1 = 1;
int freq_ch0 = 1000;
int freq_ch1 = 5000;
int bits_resolution = 8;
float h;
float t;


bool val;
struct Button {
  const uint8_t PIN;
  uint32_t numberKeyPresses;
  bool pressed;
};

Button button1 = {18, 0, false};
//volatile bool interruptorAbierto = false;
LiquidCrystal_I2C lcd(0x27, 16, 2);

DHT dht(DHTPIN, DHTTYPE);

const char *ssid = "SSID";
const char *password = "PASSWORD";

// Nombre del dominio o ruta de dirección IP
const char *serverName = "http://18.119.166.10:8000/";

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastTime = 0;
// Timer set to 10 minutes (600000)
// unsigned long timerDelay = 600000;
// Set timer to 5 seconds (5000)
unsigned long timerDelay = 5000;

void IRAM_ATTR isr(){
  button1.numberKeyPresses += 1;
  button1.pressed = true;
}

void setup() {
  //pinMode(interruptorPin, INPUT_PULLUP);
  //Serial.begin(115200);

  //Configuración LCD
  lcd.init();//inicializar la pantalla lcd
  lcd.backlight();//Encender la luz de fondo
  lcd.setCursor (0, 0);//poner el cursor en las coordenadas (x,y)
  lcd.print(" Pastillero ");//muestra en la pantalla max 20 caracteres
  lcd.setCursor (0, 1);//poner el cursor en las coordenadas (x,y)
  lcd.print("  Inteligente   ");//muestra en la pantalla max 20 caracteres
  delay(2000);
  lcd.clear();
  WiFi.begin(ssid, password);
  lcd.println("Connecting......");
  delay(2000);
  lcd.clear();
  /*while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    lcd.print(".");
  }
  lcd.clear();*/
  /*
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Timer set to 5 seconds (timerDelay variable), it will take 5 seconds before publishing the first reading.");
  */
  ledcSetup(ledc_channel_0,freq_ch0,bits_resolution);
  ledcAttachPin(pin_PWM,ledc_channel_0);

  pinMode(ledR, OUTPUT);//configuración LED rojo
  pinMode(ledG, OUTPUT);//configuración LED verde
  pinMode(ledB, OUTPUT);//configuración LED blue
  pinMode(buzzer, OUTPUT);//configuración buzzer
  pinMode(button1.PIN, INPUT_PULLDOWN);//configuración pulsador
  attachInterrupt(button1.PIN, isr, RISING);//interrupción de pulsador
  
  dht.begin();//Inicio del DHT11

}

void loop() {

  // Send an HTTP POST request every 10 minutes
  if ((millis() - lastTime) > timerDelay)
  {
    // Check WiFi connection status
    if (WiFi.status() == WL_CONNECTED)
    {
      WiFiClient client;
      HTTPClient http;
    
      //const char *post_endpoint = serverName + "/create-metric/";
      // Your Domain name with URL path or IP address with path
      http.begin(client, "http://18.119.166.10:8000/metrics/create-metric");

      // If you need Node-RED/server authentication, insert user and password below
      // http.setAuthorization("REPLACE_WITH_SERVER_USERNAME", "REPLACE_WITH_SERVER_PASSWORD");
      t = dht.readTemperature();
      h = dht.readHumidity();
      /*Serial.print(F("Humidity: "));
      Serial.print(h);
      Serial.print(F("%  Temperature: "));
      Serial.print(t);
      Serial.println(F("°C "));*/
      // Specify content-type header
      http.addHeader("Content-Type", "application/json");

      // Send HTTP POST request
      int httpResponseCode = http.POST("{\"value\":\"" + String(t) + "\",\"type\":\"Temperature\"}");
      /*Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);*/

      http.end();

      http.begin(client, "http://18.119.166.10:8000/pills/is_pill_time/");
      int httpGetCode = http.GET();

      String med = "{}";

      if (httpGetCode==200) {

      int httpGetCode = http.GET();

      Serial.print("HTTP Response code: ");
      Serial.println(httpGetCode);
      med = http.getString();
      Serial.println(med);

      // Parse the JSON payload
      DynamicJsonDocument jsonDoc(128); // Adjust the buffer size as per your JSON payload
      DeserializationError error = deserializeJson(jsonDoc, med);

      // Check for parsing errors
      if (error) {
        Serial.print("JSON parsing failed: ");
        Serial.println(error.c_str());
        return;
      }
      const char* nombre_med = jsonDoc["pill_name"];
      int charCount = strlen(nombre_med);
      int inicio = 16 - charCount;
      Serial.println(charCount);
      const char* hora_med = jsonDoc["time"];


      lcd.clear();
      lcd.setCursor (0, 0);
      lcd.print(" Hora:     ");
      lcd.println(hora_med);
      lcd.setCursor (inicio, 1);
      lcd.println(nombre_med);
      intbuzzer2();
      delay (3000);
      lcd.clear();

      }
      
      else{
        Serial.print("Error code: ");
        Serial.println(httpGetCode);
      }
      
      // Free resources
      http.end();

    }
    else
    {
      Serial.println("WiFi Disconnected");
    }
    lastTime = millis();
  }


  val= digitalRead(button1.PIN);
  if(button1.pressed){
    delay(2000);
    Serial.printf("Button 1 has been pressed %u times\n", button1.numberKeyPresses);
    val= digitalRead(button1.PIN);
    while(val==false){
    digitalWrite(ledG, HIGH);
    digitalWrite(ledB, HIGH);
    digitalWrite(ledR, LOW);
    lcd.clear();
    lcd.setCursor (0, 0);
    lcd.print("Pastillero");
    lcd.setCursor (0, 1);
    lcd.print(" Abierto");
    intbuzzer();
    val= digitalRead(button1.PIN);
    }
    digitalWrite(ledR, HIGH);
    button1.pressed = false;
    lcd.clear();
  }
  
  t = dht.readTemperature();
  h = dht.readHumidity();

  delay(10);
  digitalWrite(ledG, LOW); //G
  digitalWrite(ledB, HIGH);//B
  digitalWrite(ledR, HIGH);//r

  lcd.setCursor (0, 0);//poner el cursor en las coordenadas (x,y)
  lcd.print("T: ");//muestra en la pantalla max 20 caracteres
  lcd.print(t);
  lcd.print("C");//Esperamos 1 segundo antes de repetir el loop
  lcd.setCursor (0, 1);
  lcd.print("Hum:");
  lcd.print(h);
  lcd.print("%");
  if(h >= 70.0){
      ledcWrite(ledc_channel_0,250);
    }
  if(h <= 66.0){
      ledcWrite(ledc_channel_0,0);
    }

}

void intbuzzer(){

  digitalWrite(buzzer, HIGH);
  delay(100);
  digitalWrite(buzzer, LOW);
  delay(100);
}

void intbuzzer2(){

  digitalWrite(buzzer, HIGH);
  delay(1000);
  digitalWrite(buzzer, LOW);
  delay(500);
}
