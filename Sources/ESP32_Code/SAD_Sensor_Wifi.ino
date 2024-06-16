#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <DHT_U.h>
#include <MQ135.h>
#include <WiFi.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define PIN_MQ135 32
#define DHTPIN 26
#define DHTTYPE DHT22
#define PIN_BUZZER 27
#define PIN_FAN 33

const char* ssid     = "RizkiMP7";
const char* password = "12345678";

const char* host = "192.168.101.74";
const int port = 1999;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

DHT_Unified dht(DHTPIN, DHTTYPE);

MQ135 mq135_sensor(PIN_MQ135);

float temperature, humidity;

WiFiClient client;

void setup()
{
  Serial.begin(9600);
  analogReadResolution(12);

  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_FAN, OUTPUT);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }

  delay(2000);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  display.print("Connecting to: ");
  display.println(ssid);
  display.display();

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  display.println("");
  display.println("WiFi connected with IP address: ");
  display.println(WiFi.localIP());
  display.display();
  delay(3000);
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  dht.begin();
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);

  // Connect to the server once during setup
  if (!client.connect(host, port)) {
    Serial.println("Connection to host failed");
    return;
  }
}

void loop()
{
  if (!client.connected()) {
    // Reconnect if the connection is lost
    if (!client.connect(host, port)) {
      Serial.println("Reconnection failed");
      delay(1000);
      return;
    }
  }

  sensors_event_t tempEvent;
  dht.temperature().getEvent(&tempEvent);
  temperature = tempEvent.temperature;

  sensors_event_t humidEvent;
  dht.humidity().getEvent(&humidEvent);
  humidity = humidEvent.relative_humidity;

  int correctedPPM = mq135_sensor.getCorrectedPPM(temperature, humidity) * 3.8;
  int graph = map(correctedPPM, 0, 1000, 0, 127);

  display.clearDisplay();
  display.setTextSize(2);
  display.print("PPM: ");
  if (correctedPPM > 1000)
  {
    display.println("1000+");
    correctedPPM = 1000;
  }
  else display.println(correctedPPM);
  Serial.println(correctedPPM);

  display.fillRect(0, 52, graph, 10, WHITE);
  display.println("");
  display.setTextSize(1);
  display.print("TEMP: ");
  if (isnan(temperature)) {
    display.println(F("ERR"));
    Serial.print(temperature);
  } else {
    display.print(temperature);
    display.println(F(" C"));
  }

pz  display.print("HUMID: ");
  if (isnan(humidity)) {
    display.println(F("ERR"));
    Serial.print(humidity);
  } else {
    display.print(humidity);
    display.println(F(" %"));
  }

  display.display();
  display.setCursor(0, 0);

  // Control fan based on PPM levels
  if (correctedPPM > 800) {
    digitalWrite(PIN_BUZZER, HIGH);
    delay(5000);
    digitalWrite(PIN_BUZZER, LOW);
  }

  // Control buzzer based on temperature thresholds
  if (temperature < 10 || temperature > 40) {
    for (int j = 0; j < 3; j++) {
      for (int i = 0; i < 3; i++) {
        digitalWrite(PIN_BUZZER, HIGH);
        delay(250);
        digitalWrite(PIN_BUZZER, LOW);
        delay(250);
      }
      delay(1500);
    }
  }

  // Format the data to a fixed length
  char data[21];
  snprintf(data, sizeof(data), "%05.2f,%05.2f,%05d", temperature, humidity, correctedPPM);

  client.print(data);
  Serial.println("Data sent: " + String(data));

  delay(1000);
}
