#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// --- Définition des broches des capteurs ---
#define DHTPIN 4      
#define DHTTYPE DHT22 
#define MQ2PIN 35    
#define MQ135PIN 34 

// --- Connexion Wi-Fi ---
const char* ssid = "Wokwi-GUEST";   
const char* password = "";        

// --- Connexion MQTT ---
const char* thingsboard_server = "demo.thingsboard.io";  
const char* thingsboard_token = <ADD TOKEN HERE>"; 
const int mqtt_port = 1883;

// --- Connexion MQTT à Google Colab (utilise ton IP locale) ---
const char* colab_server = "broker.hivemq.com"; 
const int colab_port = 1883;
const char* TOPIC_PUB = "esp32/data"; 
const char* TOPIC_SUB = "esp32/indicator"; 

// --- Initialisation des objets ---
WiFiClient espClient;
PubSubClient clientThingsBoard(espClient);
PubSubClient clientColab(espClient);
DHT dht(DHTPIN, DHTTYPE);

String MSG="\"indicator\":0";

void setup() {
  Serial.begin(115200);
  
  // Connexion Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connexion WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnecté au WiFi!");

  // Configuration MQTT ThingsBoard
  clientThingsBoard.setServer(thingsboard_server, mqtt_port);
  
  // Configuration MQTT Google Colab
  clientColab.setServer(colab_server, colab_port);
  clientColab.setCallback(callback); // Fonction callback pour recevoir l'indicateur

  // Initialisation du capteur DHT
  dht.begin();
}

void reconnect(PubSubClient &client, const char* token = nullptr) {
  while (!client.connected()) {
    Serial.print("Connexion au serveur MQTT...");
    if (token) {
      if (client.connect("ESP32_Client", token, "")) {
        Serial.println("Connecté à ThingsBoard !");
      }
    } else {
      if (client.connect("ESP32_Client")) {
        Serial.println("Connecté à Google Colab !");
        client.subscribe(TOPIC_SUB);
      }
    }
    if (!client.connected()) {
      Serial.println("Échec, tentative dans 5s...");
      delay(5000);
    }
  }
}

// Fonction pour ajouter du bruit à une valeur (simule la variabilité en simulation)
template<typename T>
T addNoise(T value, float noiseLevel) {
  return value + ((random(-100, 101) / 100.0) * noiseLevel);
}


void loop() {
  
  // --- Lecture des capteurs ---
  float temperature = addNoise(dht.readTemperature(), 4.0); 
  float humidity = addNoise(dht.readHumidity(), 15.0);       
  int mq2_value = addNoise(analogRead(MQ2PIN), 2000);         
  int mq135_value = addNoise(analogRead(MQ135PIN), 2000);     

  Serial.print("Température: "); Serial.print(temperature); Serial.println(" °C");
  Serial.print("Humidité: "); Serial.print(humidity); Serial.println(" %");
  Serial.print("MQ2 Gaz: "); Serial.println(mq2_value);
  Serial.print("MQ135 Qualité de l'air: "); Serial.println(mq135_value);
    // --- Création du JSON pour Colab ---
  payload = "{";
  payload += "\"temperature\":" + String(temperature) + ",";
  payload += "\"humidity\":" + String(humidity) + ",";
  payload += "\"mq2_gas\":" + String(mq2_value) + ",";
  payload += "\"mq135_air_quality\":" + String(mq135_value);
  payload += "}";

  if (!clientColab.connected()) reconnect(clientColab);
  clientColab.loop();
  clientColab.publish(TOPIC_PUB, payload.c_str());
  
  // --- Création du JSON pour Thingsboard ---
  String payload = "{";
  payload += "\"temperature\":" + String(temperature) + ",";
  payload += "\"humidity\":" + String(humidity) + ",";
  payload += "\"mq2_gas\":" + String(mq2_value) + ",";
  payload += "\"mq135_air_quality\":" + String(mq135_value)+ ",";
  payload += MSG;
  payload += "}";

  // Vérifier la connexion MQTT
  if (!clientThingsBoard.connected()) reconnect(clientThingsBoard, thingsboard_token);
  clientThingsBoard.loop();
  
  Serial.print("Envoi MQTT: "); Serial.println(payload);
  
  // Envoi des données aux deux serveurs
  clientThingsBoard.publish("v1/devices/me/telemetry", payload.c_str());
  


  delay(5000);
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message reçu sur ");
  Serial.print(topic);
  Serial.print(": ");

  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  MSG = message.substring(1, message.length() - 1);
  Serial.println(message);

}