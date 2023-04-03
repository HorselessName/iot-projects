#include <PubSubClient.h>
#include <ESP8266WiFi.h>

// WiFi credentials
const char* ssid = "ANDON";
const char* password = "andon@aro";

// MQTT Broker details
const char* mqtt_server = "andon.arotubi.com.br";
const int mqtt_port = 1883;

// ID do MQTT - Nome de Identificação (Ex: ELLS0404)
const char* mqtt_client_name = "arduino_publisher";

// MQTT Topics for MAC and IP.
const char* mqtt_topic_arduino_mac = ("ZBX/" + String(mqtt_client_name) + "/mac").c_str();
const char* mqtt_topic_arduino_ip = ("ZBX/" + String(mqtt_client_name) + "/ip").c_str();

// WiFi Client and MQTT Client Instances
WiFiClient wifi_client; // Create a WiFi Controller Object
PubSubClient mqtt_client(wifi_client); // Use this WiFi Controller to establish connection to the MQTT Broker

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Setup Inicial - Conectar no WiFi e no MQTT Broker
  conectar_no_wifi();
  conectar_no_mqtt_broker();
}

void loop() {
  // Reconectar o WiFi caso caia e publicar mensagens no MQTT Broker
  manter_wifi_conectado();
  enviar_topicos_mqtt_broker();
  delay(5000);
}

void conectar_no_wifi(){
  // Connect to WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  // WiFi Connected - Serial Prints.
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("MAC address: ");
  Serial.println(WiFi.macAddress());
}

void manter_wifi_conectado(){
  // Check if WiFi is connected
  if (WiFi.status() != WL_CONNECTED) {
    // Attempt to reconnect to WiFi
    WiFi.begin(ssid, password);

    // Tenta dez vezes se conectar ao WiFi
    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 10) {
      retries++;
      delay(1000);
      Serial.print(".");
    }

    if (WiFi.status() != WL_CONNECTED) {
      // Failed to reconnect to WiFi, reboot the Arduino
      delay(1000);
      ESP.restart();
    }
  }
}

void enviar_topicos_mqtt_broker(){
  // Keep MAC and IP on a Variable to send to the respective MQTT Topics.
  String arduino_mac = WiFi.macAddress();
  String arduino_ip = WiFi.localIP().toString();

  // Verifica conexao com o MQTT Broker
  int retries = 0; // Adicionando um contador
  while (retries < 10 && !mqtt_client.connected()) { // Definindo o número de tentativas aqui como 10
    mqtt_client.connect(mqtt_client_name);
    retries++;
    delay(1000);
  }

  if (mqtt_client.connected()) { // Verificando se o cliente está conectado antes de publicar
    // Publicar os Topicos no MQTT Broker
    mqtt_client.publish(mqtt_topic_arduino_mac, arduino_mac.c_str());
    mqtt_client.publish(mqtt_topic_arduino_ip, arduino_ip.c_str());
    
  }
}

void conectar_no_mqtt_broker(){
  // Connect to MQTT Broker
  mqtt_client.setServer(mqtt_server, mqtt_port);

  while (!mqtt_client.connected()) {
    Serial.println("Connecting to MQTT broker...");
    if (mqtt_client.connect(mqtt_client_name)) {
      Serial.println("Connected to MQTT broker");
    } else {
      Serial.print("Failed to connect to MQTT broker, rc=");
      Serial.print(mqtt_client.state());
      Serial.println(" retrying in 5 seconds");
      delay(5000);
    }
  }
}