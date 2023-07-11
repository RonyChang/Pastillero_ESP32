#include <Arduino.h>

#include <WiFi.h>
#include <HTTPClient.h>

#include <Arduino_JSON.h>

#include <LiquidCrystal_I2C.h>
#include <DHT.h>//https://github.com/adafruit/DHT-sensor-library

#include <time.h>

#define buzzer 2

#define ledG 12
#define ledR 13
#define ledB 14

#define DHTPIN 4 
#define DHTTYPE DHT11

int pin_PWM = 15;
int ledc_channel_0 = 0;
int ledc_channel_1 = 1;
int freq_ch0 = 1000;
int freq_ch1 = 5000;
int bits_resolution = 8;
float h;
float t;
String nombre_med;
String hora_med;

bool val;
struct Button {
  const uint8_t PIN;
  uint32_t numberKeyPresses;
  bool pressed;
};

Button button1 = {32, 0, false};
//volatile bool interruptorAbierto = false;
LiquidCrystal_I2C lcd(0x27, 16, 2);

DHT dht(DHTPIN, DHTTYPE);

const char *ssid = "Daru";
const char *password = "JohnTitor";

//const char *ssid = "iPhone de Rony";
//const char *password = "12345678";

//const char *ssid = "WIN";
//const char *password = "M3014272605";

// Nombre del dominio o ruta de dirección IP
const char *serverName = "http://18.119.166.10:8000/";

unsigned long lastTime = 0;
// Set timer to 5 seconds (5000)
unsigned long timerDelay = 5000;

void IRAM_ATTR isr(){
  button1.numberKeyPresses += 1;
  button1.pressed = true;
}

void setup() {
  //pinMode(interruptorPin, INPUT_PULLUP);
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Timer set to 5 seconds (timerDelay variable), it will take 5 seconds before publishing the first reading.");

  ledcSetup(ledc_channel_0,freq_ch0,bits_resolution);
  ledcAttachPin(pin_PWM,ledc_channel_0);

  pinMode(ledR, OUTPUT);
  pinMode(ledG, OUTPUT);
  pinMode(ledB, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(button1.PIN, INPUT_PULLUP);
  attachInterrupt(button1.PIN, isr, FALLING);
  //attachInterrupt(digitalPinToInterrupt(interruptorPin), handleInterrupt, CHANGE);
  //dht11
  
  dht.begin();
  //Configuración LCD
  lcd.init();//inicializar la pantalla lcd
  lcd.backlight();//Encender la luz de fondo
  lcd.setCursor (0, 0);//poner el cursor en las coordenadas (x,y)
  lcd.print(" Pastillero ");//muestra en la pantalla max 20 caracteres
  lcd.setCursor (0, 1);//poner el cursor en las coordenadas (x,y)
  lcd.print("  Inteligente   ");//muestra en la pantalla max 20 caracteres
  delay(2000);
  lcd.clear();

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
      Serial.print(F("Humidity: "));
      Serial.print(h);
      Serial.print(F("%  Temperature: "));
      Serial.print(t);
      Serial.println(F("°C "));
      // Specify content-type header
      http.addHeader("Content-Type", "application/json");

      // Send HTTP POST request
      int httpResponseCode = http.POST("{\"value\":\"" + String(t) + "\",\"type\":\"Temperature\"}");
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);

      http.end();

      
      
      http.begin(client, "http://18.119.166.10:8000/pills/is_pill_time/");
      int httpGetCode = http.GET();

      String med = "{}";

      if (httpGetCode==200) {

      int httpGetCode = http.GET();

      Serial.print("HTTP Response code: ");
      Serial.println(httpGetCode);
      med = http.getString();

      JSONVar myObject = JSON.parse(med);

      // JSON.typeof(jsonVar) can be used to get the type of the var
      if (JSON.typeof(myObject) == "undefined") {
        Serial.println("Parsing input failed!");
        return;
      }

      Serial.print("JSON object = ");
      Serial.println(myObject);

      // myObject.keys() can be used to get an array of all the keys in the object
      JSONVar keys = myObject.keys();
      int sensorReadingsArr[4];
      for (int i = 0; i < keys.length(); i++) {
        JSONVar value = myObject[keys[i]];

        if (i==3){
          
          String nombre_med = value;
          lcd.clear();
          lcd.setCursor (0, 0);
          lcd.print(nombre_med);
          Serial.println(nombre_med);
          
        }
        if (i==0){
          String hora_med = value;
          lcd.setCursor (0, 1);
          lcd.print("Hora: ");
          lcd.print(hora_med);  
          Serial.println(hora_med);
          
        }
      }
      
      intbuzzer();

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

  delay(10);
  digitalWrite(ledG, LOW); //G
  digitalWrite(ledB, HIGH);//B
  digitalWrite(ledR, HIGH);//r

  lcd.setCursor (0, 0);//poner el cursor en las coordenadas (x,y)
  lcd.print("T: ");//muestra en la pantalla max 20 caracteres
  //La funcion millis() regresa los ms que lleva encendido
  //Lo dividimos entre 1000 para que nos muestre en segundos.

  //lcd.clear();
  lcd.print(t);
  lcd.print(" C");//Esperamos 1 segundo antes de repetir el loop
  lcd.setCursor (0, 1);
  lcd.print("Hum: ");
  lcd.print(h);
  lcd.print("%");
  if(h >= 70.0){
      ledcWrite(ledc_channel_0,127);
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