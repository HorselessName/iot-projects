// Programa: NodeMCU e MQTT - Controle e Monitoramento IoT
// Autores: Raul Chiarella, Danilo Rodrigues

#include <ESP8266WiFi.h>  // Importa a Biblioteca ESP8266WiFi
#include <PubSubClient.h> // Importa a Biblioteca PubSubClient

/* ----- Defines ----- */
#define TOPICO_SUBSCRIBE    "ENV/ACCD0101"                      // Nome do Topico de Escuta no MQTT Broker (Faz o Subscribe)
#define TOPICO_PUBLISH      "REC/ACCD0101"                      // Topico de Envio - Envia mensagens para este topico no MQTT Broker.
#define MQTT_CLIENT_NAME    "ACCD0101"                          // ID MQTT p/ identificar a sessao.

#define MQTT_TOPIC_IP_MAC   "REC/" MQTT_CLIENT_NAME "/_ip_mac"  // Topico de Envio do IP e MAC
#define RELE                16                                  // GPIO16 - PIN D0 - IN no rele.
#define BUTTON              14                                  // GPIO14 - PIN D5
#define VALIDADOR           12                                  // GPIO2 - PIN D6 - Boteo vai validar a eletrica.

// Controle para sincronizar o MySQL e o MQTT
bool andonAcionado = false;

/* ----- Constantes ----- */
const char* ssid = "Rede WiFi";                            // Nome da rede WI-FI que deseja se conectar
const char* password = "Senha WiFi";                       // Senha da rede WI-FI que deseja se conectar
const char* mqtt_server = "API do Servidor MQTT";          // URL do Broker MQTT
const int mqtt_port = 1883;                                // Porta do Broker MQTT

/* ----- Global Variables ----- */
int estado_da_saida = 2;                                   // Variavel que armazena o estado atual da saida.
int tempo_pressionado = 0;                                 // Utilizado para calcular pressionada dos botoes
int contador_ipmac = 0;

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
void desligarLed();
void ligarLed();
void rebootESP();
void VerificaConexoes();
void StatusBotao();

/* 
 *  Funcoes Executadas quando o Arduino Liga.
 */
void setup() 
{
  initSerial();   // Iniciar conexao Serial.
  initOutput();   // Iniciar sinais dos PINs.
  initWiFi();     // Conectar no WiFi - Faz reboot se nao der certo.
  initMQTT();     // Conectar no MQTT - Faz reboot se nao der certo.
  desligarLed();  // Se chegou nessa funcao, conectou ao MQTT e ao WiFi, entao, desligo o LED.
}

void loop() 
{   
  // Functions that Loop
  VerificaConexoes();
  StatusBotao();
  publicarTopicos();  // Publica o Status do Botao e o IP, MAC.
  MQTT_CONTROLLER.loop();

  delay(500);
}

////////////////////////////////////////  SETUP - Funoees de Inicializacao  ////////////////////////////////////////
void initSerial() 
{
    Serial.begin(115200);
    delay(5000);

    Serial.println("\n");
    Serial.println("----- Inicializando o Setup do Arduino ----- ");
    Serial.println("");
}

/* Reference: https://www.arduino.cc/reference/en/language/functions/digital-io/pinmode/ */
void initOutput()
{
  ///// Inicializacao dos pinos do Arduino /////
  Serial.print("Inicializando pinos... ");
  delay(1000);
  
  // Inicio com o RELE ligado para acionar o LED assim que o Arduino ligar.
  Serial.print("Pino do RELE... ");
  pinMode(RELE, OUTPUT);          // O PIN pode enviar um sinal eletrico

  // Se inicializar com HIGH, ele fecha contato entre o IN e o NC, desligando o LED.
  digitalWrite(RELE, LOW);
  delay(1000);
  
  // Inicio o PIN do Botao para poder ler o sinal ao manter pressionado.
  Serial.print("Pino do botao... ");
  pinMode(BUTTON, INPUT_PULLUP);  
  delay(1000);

  // Inicializar o botao validador.
  pinMode(VALIDADOR, INPUT_PULLUP);  
  delay(1000);

  Serial.println("... Pinos Inicializados! \n");
}

/* Funaoo:  Conexao Inicial no Wifi
 *          Tenta por 15 segundos, caso nao da certo, tenta um reboot.
 * Parametros: Nenhum
 * Retorno: Nenhum
 */
void initWiFi(){
  Serial.print("Inicializando WiFi...");
  delay(150);
  WiFi.begin(ssid, password);
  delay(150);

  // Tenta por 15 segundos conectar no WiFi.
  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 15) {
    tentativas++;
    Serial.print(".");

    if (WiFi.status() == WL_CONNECTED) break;
    delay(1000);
  }

  // Se o maximo de tentativas foi atingida, rebooto o ESP.
  if (tentativas == 15) rebootESP();

  String ip = WiFi.localIP().toString();
  String mac = WiFi.macAddress();

  // WiFi Connected - Serial Prints.
  if (ip != "0.0.0.0") Serial.println(" ... WiFi inicializado! IP: " + ip + ", MAC: " + mac);
}

////////////////////////////////////////  MQTT - Funcoes Relacionadas ao MQTT  ////////////////////////////////////////
/* Funaoo:  Configura e utiliza a Controller.
      Faz e mantem a conexao com o MQTT Broker usando a Controller.
      Tenta por 15 segundos, caso nao da certo, tenta um reboot.
 * Parametros: nenhum
 * Retorno: Nenhum */
void initMQTT(){
  Serial.println("");
  Serial.print("Inicializando conexao ao MQTT Broker... ");

  MQTT_CONTROLLER.setServer(mqtt_server, mqtt_port);
  MQTT_CONTROLLER.setCallback(mqtt_callback);   // Topic Listener

  Serial.print("Status MQTT Broker: ");
  Serial.print(MQTT_CONTROLLER.connected());

  // Tentar se conectar ao MQTT Broker por 15 segundos.
  int tentativas = 0;
  Serial.print("\nConectando ao MQTT Broker...");
  while (!MQTT_CONTROLLER.connected()) {
    tentativas++;
    Serial.print(".");
    // Conectar o MQTT Client no MQTT Broker.
    if (MQTT_CONTROLLER.connect(MQTT_CLIENT_NAME)) MQTT_CONTROLLER.subscribe(TOPICO_SUBSCRIBE, 1);  // Se inscrever no Topico de Escuta.
    if (MQTT_CONTROLLER.connected()) return;  // Sair do metodo

    if (tentativas == 15) break;  // Sair do loop

    delay(1000);
  }

  // Entra aqui se saiu do loop e nao do metodo.
  rebootESP();
}

/* Funcao:      Implementa a Logica de Tratativa da Mensagem do Topico.
 * Par metros:  Topico, Payload (Mensagem do Topico em Formato Binario) e Length.
 * Status:      - Estado de Saida 0: O topico inscrito foi avisado que o botao do ANDON foi pressionado.
 *              - Estado de Saida 1: O topico inscrito recebeu uma mensagem para desligar a sirene.
 */
void mqtt_callback(char* topic, byte* payload, unsigned int length) 
{
  // Callback e a funcao de retorno em que ao ser liberado um ANDON, a API envia uma mensagem para o TOPICo avisando.
  Serial.print("\nSubscribe realizado no Topico: ");
  Serial.println(TOPICO_SUBSCRIBE);

  Serial.println("\nAguardando Libera  es e Acionamentos!");

  String mensagem;
  for(int i = 0; i < length; i++) 
  {
      char caractere = (char)payload[i];  // Interpretar bin rio como char.
      mensagem += caractere;
  }

  if (mensagem.equals("L"))
  {
      desligarLed();
      Serial.println("Mensagem Equals L => " + mensagem);

      // Estado 1 => ANDON Liberado
      estado_da_saida = 1; 

      andonAcionado == false;
  }

  if (mensagem.equals("D"))
  {
      Serial.println("\nMensagem Equals D => " + mensagem);

      // Estado 0 => ANDON Acionado
      ligarLed();  // Ligo de novo o LED mesmo se j  estiver ligado por desencargo.
      estado_da_saida = 0;
  }
}

void publicarTopicos(){
  enviarStatusInicial();
  enviarStatusBotao();
}

/* Fun  o:  Ele publica no topico o status inicial, assim que o Arduino   ligado.
 *          Vai mandar tamb m na primeira vez o IP e o MAC.
 *          Ele envia o Status que foi gerado no m todo StatusAndon().
 */
void enviarStatusInicial(){
  if (estado_da_saida == 2){
    MQTT_CONTROLLER.publish(TOPICO_PUBLISH, "R"); // Se ANDON foi iniciado/reiniciado, envio R para atualizar o status.
    enviarIpMac_Arduino();                        // Envio o IP e o MAC uma vez... Quando o estado inicial   R.
    estado_da_saida = 3;                          // Mudo o estado da saida pra n o passar aqui novamente.
    delay(1000);
    }
}

/* Fun  o:  Publicar mensagem ao topico no MQTT Broker com o Status do Bot o
 *          Ele envia o Status que foi gerado no m todo StatusAndon().
 */
void enviarStatusBotao(){
  // Estado 0 => ANDON Acionado - Acionar o ANDON.
  if (estado_da_saida == 0 && andonAcionado == false) {
    // Envio somente uma vez a mensagem - De forma garantida (QoS)
    MQTT_CONTROLLER.publish(TOPICO_PUBLISH, "D", 1);
    andonAcionado == true;
  }

  // Estado 1 => ANDON n o acionado.
  if (estado_da_saida == 1) {
    MQTT_CONTROLLER.publish(TOPICO_PUBLISH, "L", 1);
    desligarLed();
  }
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
  Serial.print("\nEnviando o Topico: ");
  Serial.println(MQTT_TOPIC_IP_MAC);
  delay(100);

  // Enviar o IP e o MAC em um Topico apenas.
  MQTT_CONTROLLER.publish(MQTT_TOPIC_IP_MAC, ip_mac.c_str());  // Pointer to Variables
}

////////////////////////////////////////  LED - Controle do LED.  ////////////////////////////////////////
void desligarLed(){
  // Fecha o sinal entre IN do RELE e NC - Normally Closed.
  digitalWrite(RELE, HIGH);
}

void ligarLed(){
  // Fecha o sinal entre IN do RELE e NO - Normally Open.
   digitalWrite(RELE, LOW);
}

/* 
 * M todo: rebootESP
 * Descri  o: Esse m todo tenta reiniciar o ESP8266.
 * Par metros: nenhum
 * Retorno: nenhum
 */
void rebootESP() {
  Serial.print("... O Arduino ser  reiniciado em... ");
  
  for (int i = 15; i > 0; i--) {
    Serial.print(i); // Imprime o valor atual da contagem regressiva
    Serial.print("... ");
    delay(1000); // Aguarda 1 segundo
  }

  // Desconecta do Wi-Fi
  WiFi.disconnect();

  delay(500);

  // Limpa os buffers do Serial
  Serial.flush();
  Serial.end();

  delay(500);

  // Desabilita interrup  es
  noInterrupts();

  // Aguarda 1 segundo antes de reiniciar
  delay(1000);

  // Reinicia o Arduino
  ESP.restart();

  // Garante que o rein cio ocorra apenas uma vez
  while (true) {
    // Aguarda indefinidamente
  }
}

/* 
 * M todo: VerificaConexoes
 * Descri  o: Esse m todo verifica as conex es com o WiFi e o MQTT Broker.
 * Par metros: nenhum
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
 * M todo: StatusBotao
 * Descri  o: Esse m todo verifica se o bot o do ANDON est  pressionado
 *            Ele utiliza os defines RELE (PIN D0) e BUTTON (PIN D5).
 *            Este   o m todo principal que trata os acionamentos do ANDON.
 * Status:  - Estado de Sa da 0: Bot o do ANDON foi Pressionado.
 */
void StatusBotao(){ 
  Serial.print("\nStatus do ANDON... ");
  Serial.print("LED: ");
  Serial.print(digitalRead(RELE));
  Serial.print(", ");
  Serial.print("BUTTON: ");
  Serial.print(digitalRead(BUTTON));
  Serial.print(", ");
  Serial.print("VALIDADOR: ");
  Serial.print(digitalRead(VALIDADOR));
  Serial.print(", ");
  Serial.print("STATUS SAIDA: ");
  Serial.print(estado_da_saida);

  // Se o bot o est  pressionado (Logica invertida - Apertado   LOW e n o apertado   HIGH.)
  if (digitalRead(BUTTON) == LOW && digitalRead(VALIDADOR) == HIGH) {
          tempo_pressionado ++;  
          delay(250);
      }

  // Se o bot o ficou pressionado por tempo suficiente
  if( tempo_pressionado == 2 && digitalRead(BUTTON) == LOW && digitalRead(VALIDADOR) == HIGH ){
    Serial.print("\nLigando LED e mudando mudando estado_da_saida para 0 ... ");

    // Ligar o RELE para ativar a sirene.
    ligarLed();           

    // Variavel que controla se o ANDON est  acionado
    estado_da_saida = 0; 

    Serial.print(" -> ANDON acionado! \n"); 

    tempo_pressionado = 0;
    } else {
      // Se soltar o bot o pressionado
      if( digitalRead(BUTTON) == HIGH ){
        tempo_pressionado = 0;
    }
  }
}
