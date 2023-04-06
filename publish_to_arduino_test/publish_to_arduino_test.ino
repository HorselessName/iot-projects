#include <PubSubClient.h>
#include <ESP8266WiFi.h>

/* ----- Declaração das minhas Variáveis ----- */
// WiFi credentials
const char* ssid = "ANDON";
const char* password = "andon@aro";

// MQTT Broker details
const char* mqtt_server = "andon.arotubi.com.br";
const int mqtt_port = 1883;

// ID do MQTT Client
const char* mqtt_client_name = "arduino_publisher";

// MQTT Topics for MAC and IP.
String mqtt_topic_arduino_mac_str = "ZBX/" + String(mqtt_client_name) + "/mac";
String mqtt_topic_arduino_ip_str = "ZBX/" + String(mqtt_client_name) + "/ip";

/* ----- Declaração dos meus Controllers ----- */
// WiFi Client and MQTT Client Instances
WiFiClient wifi_client; // Create a WiFi Controller Object
PubSubClient mqtt_client(wifi_client); // Use this WiFi Controller to establish connection to the MQTT Broker

// Ref. https://en.cppreference.com/w/cpp/language/function
/* ----- Declaração dos meus Prototypes ----- */
void conectar_no_wifi();
void manter_wifi_conectado();
void enviar_topicos_mqtt_broker(); 
void conectar_no_mqtt_broker();

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
  delay(1000);
}

/* Função: Conexão Inicial no Wifi
 * Parâmetros: Nenhum
 * Retorno: Nenhum
 */
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

/* Função: Verifica a conexão do WiFi
 * Caso falhe por 10 vezes, reinicia o aparelho.
 */
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

/* Sintaxe do Publish: 
 * mqtt_client.publish(String(nome_do_topico), [Type?] mensagem_a_ser_enviada.c_str());
 * Onde a "Mensagem a ser enviada" eh passada por referencia. Ou seja, a variavel deve existir.
 */
void enviar_topicos_mqtt_broker(){
  // Create MAC and IP Variable with Data that will be sent to the respective MQTT Topics.
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
    /* https://en.cppreference.com/w/cpp/string/basic_string/c_str
    * Usando esse metodo, destroi a variavel caso tenha algum valor, 
    * e armazena somente o ponteiro na memoria da variavel. 
    * Ou seja: Cria uma referencia para acessar uma variavel na memoria.
    */
    
    // Publicar os Topicos no MQTT Broker - Topico: String e Mensagem: Referencia em outra variavel.
    // Serial.println("Enviando para o Topico MAC: " + String(mqtt_topic_arduino_mac_str));
    mqtt_client.publish(mqtt_topic_arduino_mac_str.c_str(), arduino_mac.c_str());  // Pointer to Variables
    mqtt_client.loop(); // Wait until message is sent

    // Serial.println("Enviando para o Topico IP: " + String(mqtt_topic_arduino_ip_str));
    mqtt_client.publish(mqtt_topic_arduino_ip_str.c_str(), arduino_ip.c_str());  // Pass Variables as References
    mqtt_client.loop(); // Wait until message is sent
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

/* Função: Mantém a conexão com o MQTT e se inscreve novamente nos tópicos, pois ele não reconecta sozinho. 
 * Parâmetros: nenhum
 * Retorno: Nenhum */
void manter_conexao_mqtt_broker() 
{
  while (!mqtt_client.connected()) {
    Serial.println("Connecting to MQTT broker...");
    if (mqtt_client.connect(mqtt_client_name)) {
      // Entra aqui caso esteja conectado no MQTT Broker.
      mqtt_client.subscribe(TOPICO_SUBSCRIBE); // Se reinscreve no Topico ao estabelecer nova conexão no Broker
      Serial.println("Connected to MQTT broker");
    } else {
      Serial.print("Failed to connect to MQTT broker. Status: ");
      Serial.print(mqtt_client.state());
      delay(5000);
    }
  }
}