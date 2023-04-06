// Programa: NodeMCU e MQTT - Controle e Monitoramento IoT
// Autores: Danilo Rodrigues e Raul Chiarella

#include <ESP8266WiFi.h>  // Importa a Biblioteca ESP8266WiFi
#include <PubSubClient.h> // Importa a Biblioteca PubSubClient

/* ----- Defines ----- */
#define TOPICO_SUBSCRIBE    "ENV/BCTA6969"                      // Topico de Escuta
#define TOPICO_PUBLISH      "REC/BCTA6969"                      // Topico de Envio
#define MQTT_CLIENT_NAME    "BCTA6969"                          // ID MQTT p/ identificar a sessão.
#define MQTT_TOPIC_IP_MAC   "REC/" MQTT_CLIENT_NAME "/_ip_mac"  // Topico de Envio do IP e MAC
#define RELE                16                                  // GPIO16 - PIN D0
#define BUTTON              14                                  // GPIO14 - PIN D5
#define RST_PIN             12                                  // GPIO12 - PIN D6

/* ----- Constantes ----- */
const char* ssid = "ANDON";                       // Nome da rede WI-FI que deseja se conectar
const char* password = "andon@aro";               // Senha da rede WI-FI que deseja se conectar
const char* mqtt_server = "andon.alufrost.com";   // URL do Broker MQTT [IP: 192.168.1.97]
const int mqtt_port = 1883;                       // Porta do Broker MQTT

/* ----- Global Variables ----- */
char estado_da_saida = '2';                       // Variável que armazena o estado atual da saída.
int tempo_pressionado = 0;                        // Utilizado para calcular pressionada dos botões

/* ----- Controllers ----- */
WiFiClient WIFI_CONTROLLER;
PubSubClient MQTT_CONTROLLER(WIFI_CONTROLLER);

/* ----- Prototypes ----- */
void initSerial();
void initOutput();
void initWiFi();
void initMQTT();
void mqtt_callback(char* topic, byte* payload, unsigned int length);
void publicarTopicos();
void enviarStatusInicial();
void enviarStatusBotao();
void enviarIpMac_Arduino();
void desligarRele();
void ligarRele();
void rebootESP();
void VerificaConexoes();
void StatusBotao();

/* 
 *  Funções Executadas quando o Arduino Liga.
 */
void setup() 
{
  // Functions that Execute when Arduino is Turned On
  initSerial();
  initOutput();
  initWiFi();
  initMQTT();
  desligarRele();  // Inicio com o RELE desligado.
}

void loop() 
{   
  // Functions that Loop
  VerificaConexoes();
  StatusBotao();
  publicarTopicos();  // Publica o Status do Botão e o IP, MAC.
  MQTT_CONTROLLER.loop();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void initSerial() 
{
    Serial.begin(115200);
    delay(1000);
}

/* Reference: https://www.arduino.cc/reference/en/language/functions/digital-io/pinmode/ */
void initOutput()
{
    /* IMPORTANTE: No caso do LED instalado na placa, ele é acionado com lógica invertida. 
    Quando enviar uma saída do tipo HIGH, a LED apaga, e ao enviar a saída do tipo LOW, o LED acende. */

    pinMode(RELE, OUTPUT);
    digitalWrite(RELE, HIGH);  
    pinMode(BUTTON, INPUT_PULLUP); 

    // Seto o PIN de Reset pois vai enviar um sinal para o RST PIN
    pinMode(RST_PIN, OUTPUT);
    digitalWrite(RST_PIN, HIGH);
}

/* Função:  Conexão Inicial no Wifi
 *          Tenta por 15 segundos, caso não dê certo, tenta um reboot.
 * Parâmetros: Nenhum
 * Retorno: Nenhum
 */
void initWiFi(){
  Serial.println("");
  Serial.println("----- Conexão WiFi -----");
  delay(150);
  WiFi.begin(ssid, password);
  delay(150);
  int tentativas = 0, segundos = 15 * 1000, milisegundos = 200;  // Tentativas X (Segundos / Delay)
  while (WiFi.status() != WL_CONNECTED && tentativas < segundos) {
    // Piscar o LED a cada loop do While.
    if(digitalRead(D0) == HIGH){
      digitalWrite(D0, LOW); 
      } else {
        digitalWrite(D0, HIGH); 
        }
    tentativas++;
    Serial.print(".");

    if (WiFi.status() == WL_CONNECTED) break;
    delay(milisegundos);
  }

  if (tentativas == segundos/milisegundos) rebootESP();
  String ip = WiFi.localIP().toString();
  String mac = WiFi.macAddress();

  // WiFi Connected - Serial Prints.
  if (ip != "0.0.0.0") Serial.println(" -> WiFi connected! IP: " + ip + ", MAC: " + mac);
}

////////////////////////////////////////  MQTT - Funções Relacionadas ao MQTT  ////////////////////////////////////////
/* Função: 	Configura e utiliza a Controller.
			Faz e mantém a conexão com o MQTT Broker usando a Controller.
      Tenta por 15 segundos, caso não dê certo, tenta um reboot.
 * Parâmetros: nenhum
 * Retorno: Nenhum */
void initMQTT(){
  Serial.println("");
  Serial.println("----- Conexão MQTT Broker -----");
  delay(150);
  MQTT_CONTROLLER.setServer(mqtt_server, mqtt_port);
  MQTT_CONTROLLER.setCallback(mqtt_callback);   // Topic Listener
  delay(150);
  Serial.print("Status MQTT Broker: ");
  Serial.println(MQTT_CONTROLLER.connected());
  int tentativas = 0, segundos = 15 * 1000, milisegundos = 200;  // Tentativas X (Segundos / Delay)
  while (!MQTT_CONTROLLER.connected()) {
	  tentativas++;
	  Serial.print(".");
	  // Conectar o MQTT Client no MQTT Broker.
    if (MQTT_CONTROLLER.connect(MQTT_CLIENT_NAME)) MQTT_CONTROLLER.subscribe(TOPICO_SUBSCRIBE);  // Se inscrever no Topico de Escuta.
	  if (MQTT_CONTROLLER.connected()) return;  // Sair do método
    if (tentativas == segundos/milisegundos) break;  // Sair do loop
	  delay(milisegundos);
	}

  // Entra aqui se saiu do loop e não do método.
	rebootESP();
}

/* Função:      Implementa a Lógica de Tratativa da Mensagem do Tópico.
 * Parâmetros:  Topico, Payload (Mensagem do Tópico em Formato Binário) e Length.
 * Status:      - Estado de Saída 0: O topico inscrito foi avisado que o botão do ANDON foi pressionado.
 *              - Estado de Saída 1: O topico inscrito recebeu uma mensagem para desligar a sirene.
 */
void mqtt_callback(char* topic, byte* payload, unsigned int length) 
{
  Serial.println("");
  Serial.println("----- MQTT Callback -----");

  Serial.print("Mensagem Recebida no Topico: ");
  Serial.println(TOPICO_SUBSCRIBE);

  String mensagem;
  for(int i = 0; i < length; i++) 
  {
      char caractere = (char)payload[i];  // Interpretar binário como char.
      mensagem += caractere;
  }

  if (mensagem.equals("L"))
  {
      desligarRele();
      estado_da_saida = '1'; 
  }

  if (mensagem.equals("D"))
  {
      ligarRele();
      estado_da_saida = '0';
  }
}

void publicarTopicos(){
  enviarStatusInicial();
  enviarStatusBotao();
}

/* Função:  Ele publica no topico o status inicial, assim que o Arduino é ligado.
 *          Vai mandar também na primeira vez o IP e o MAC.
 *          Ele envia o Status que foi gerado no método StatusAndon().
 */
void enviarStatusInicial(){
  if (estado_da_saida == '2'){
    MQTT_CONTROLLER.publish(TOPICO_PUBLISH, "R"); // Se foi reiniciado, envio R para atualizar o status.
    enviarIpMac_Arduino();  // Envio o IP e o MAC uma vez... Quando o estado inicial é R.
    }
    delay(100);
}

/* Função:  Publicar mensagem ao topico no MQTT Broker com o Status do Botão
 *          Ele envia o Status que foi gerado no método StatusAndon().
 */
void enviarStatusBotao(){
  if (estado_da_saida == '0') MQTT_CONTROLLER.publish(TOPICO_PUBLISH, "D");
  if (estado_da_saida == '1') MQTT_CONTROLLER.publish(TOPICO_PUBLISH, "L");
  delay(500);
}

/* Sintaxe do Publish: 
 * MQTT_CLIENT_NAME.publish(String(nome_do_topico), [Type?] mensagem_a_ser_enviada.c_str());
 * Onde a "Mensagem a ser enviada" eh passada por referencia. Ou seja, a variavel deve existir.
 */
void enviarIpMac_Arduino(){
  // Create MAC and IP Variable with Data that will be sent to the respective MQTT Topics.
  String ip_mac = WiFi.localIP().toString() + " - " + WiFi.macAddress();

  /* https://en.cppreference.com/w/cpp/string/basic_string/c_str
    * Usando esse metodo, destroi a variavel caso tenha algum valor, 
    * e armazena somente o ponteiro na memoria da variavel. 
    * Ou seja: Cria uma referencia para acessar uma variavel na memoria.
    */

  // Publicar os Topicos no MQTT Broker - Topico: String e Mensagem: Referencia em outra variavel.
  Serial.print("Enviando o Topico: ");
  Serial.println(MQTT_TOPIC_IP_MAC);
  delay(100);

  // Enviar o IP e o MAC em um Topico apenas.
  MQTT_CONTROLLER.publish(MQTT_TOPIC_IP_MAC, ip_mac.c_str());  // Pointer to Variables
}

////////////////////////////////////////  MQTT - Fim das Funções Relacionadas ao MQTT  ////////////////////////////////////////
void desligarRele(){
   digitalWrite(RELE, LOW); // Desligar o RELE
}

void ligarRele(){
   digitalWrite(RELE, HIGH); // Ligar o RELE
}

/* 
 * Método: rebootESP
 * Descrição: Esse método tenta reiniciar o ESP8266.
 * Parâmetros: nenhum
 * Retorno: nenhum
 */
void rebootESP() {
  Serial.print(" -> Entering Reboot ESP Mode. Will reboot in 3 Seconds");

  delay(2000);
  digitalWrite(RST_PIN, LOW);
  delay(2000);
  digitalWrite(RST_PIN, HIGH);

  // Se não rebootou, falhou o reboot aqui nessa etapa.
  delay(2000);
  Serial.println(" ESP Reboot Failed. Continuing...");
  }

/* 
 * Método: VerificaConexoes
 * Descrição: Esse método verifica as conexões com o WiFi e o MQTT Broker.
 * Parâmetros: nenhum
 * Retorno: nenhum
 */
void VerificaConexoes() {
  if(WiFi.status() != WL_CONNECTED){
    initWiFi();
    }
  if (!MQTT_CONTROLLER.connected()) {
    initMQTT(); 
    }
}

/* 
 * Método: StatusBotao
 * Descrição: Esse método verifica se o botão do ANDON está pressionado
 *            Ele utiliza os defines RELE (PIN D0) e BUTTON (PIN D5).
 *            Este é o método principal que trata os acionamentos do ANDON.
 * Status:  - Estado de Saída 0: Botão do ANDON foi Pressionado.
 */
void StatusBotao(){ 

  delay(500);

  Serial.println("----- Status do ANDON -----");
  Serial.print("Status do RELE, BUTTON: ");
  Serial.print(digitalRead(D2));
  Serial.print(", ");
  Serial.println(digitalRead(D5));

  // Se o botão está pressionado.
  if (digitalRead(BUTTON) == LOW) {
          tempo_pressionado ++;  
          delay(500);
      }

  // Se o botão ficou pressionado por tempo suficiente
  if( tempo_pressionado == 2 && digitalRead(BUTTON) == LOW ){
    // Ligar o RELE para ativar a sirene.
    ligarRele();           
    estado_da_saida = '0';
    Serial.println(" -> ANDON acionado! "); 
    tempo_pressionado = 0;
    } else {
      // Se soltar o botão pressionado
      if( digitalRead(BUTTON) == HIGH ){
        tempo_pressionado = 0;
    }
  }
}