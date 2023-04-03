//Programa: NodeMCU e MQTT - Controle e Monitoramento IoT
//Autor: DANILO
// Ruam 
#include <ESP8266WiFi.h> // Importa a Biblioteca ESP8266WiFi
#include <PubSubClient.h> // Importa a Biblioteca PubSubClient
 
// Defines do ID MQTT e Tópicos para Publish/Subscribe
#define TOPICO_SUBSCRIBE    "ENV/ELLS0404"     //tópico MQTT de escuta
#define TOPICO_PUBLISH      "REC/ELLS0404"     //tópico MQTT de envio de informações para Broker                                            
#define ID_MQTT             "ELLS0404"         // ID MQTT - Nome do ANDON p/ identificar a sessão.

/* Esses dois abaixo ainda não foram implementados pois precisam de estudo.
O objetivo final será enviar também o IP e o MAC junto do ID para o MQTT Listener. */
#define TOPICO_PUBLISH_IP   "ZBX/ELLS0404_IP"   // Ainda não utilizado e Implementado.
#define TOPICO_PUBLISH_MAC   "ZBX/ELLS0404_MAC"   // Ainda não utilizado e Implementado.                              
 
//defines - mapeamento de pinos do NodeMCU
#define D0    16
#define D1    5
#define D2    4
#define D3    0
#define D4    2
#define D5    14
#define D6    12
#define D7    13
#define D8    15
#define D9    3
#define D10   1
 
 
// WIFI
const char* SSID = "ANDON"; // SSID / nome da rede WI-FI que deseja se conectar
const char* PASSWORD = "andon@aro"; // Senha da rede WI-FI que deseja se conectar

const char* arduino_mac = '';
const char* arduino_ip = '';

int quant = 0;
int counwf = 0;
bool flagwf = false;
char IP = ''

// URL do MQTT
const char* BROKER_MQTT = "andon.arotubi.com.br"; // URL do Broker MQTT [IP: 192.168.1.97]
int BROKER_PORT = 1883; // Porta do Broker MQTT
 
// Variáveis e objetos globais
WiFiClient espClient; // Cria o objeto espClient usando o WiFiClient da lib 'ESP8266WiFi'
PubSubClient MQTT(espClient); // Instancia o Cliente MQTT e Passa o Objeto 'espClient'

char EstadoSaida = '2';  // Variável que armazena o estado atual da saída
  
//Prototypes
void initSerial();
void initWiFi();
void initMQTT();
void reconectWiFi(); 
void mqtt_callback(char* topic, byte* payload, unsigned int length);
void VerificaConexoesWiFIEMQTT(void);
void InitOutput(void);
 
/* 
 *  Implementações das funções
 */
void setup() 
{
    //inicializações:
    InitOutput();
    initSerial();
    initWiFi();
    initMQTT();
    digitalWrite(D0, LOW); //inicia o rele desligado
}

void loop() 
{   
   // Serial.println(digitalRead(D0));
    VerificaConexoesWiFIEMQTT();
    AvisoButton();
    EnviaEstadoOutputMQTT();
    MQTT.loop();
}
  
//Função: inicializa comunicação serial com baudrate 115200 (para fins de monitorar no terminal serial 
//        o que está acontecendo.
//Parâmetros: nenhum
//Retorno: nenhum
void initSerial() 
{
    Serial.begin(115200);
}
 
//Função: inicializa e conecta-se na rede WI-FI desejada
//Parâmetros: nenhum
//Retorno: nenhum
void initWiFi() 
{
    delay(10);
    Serial.println("------Conexao WI-FI------");
    Serial.print("Conectando-se na rede: ");
    Serial.println(SSID);
    Serial.println("Aguarde");
     
    reconectWiFi();
}
  
// Função: Inicializa parâmetros de conexão MQTT( Endereço do 
//        broker, porta e função de callback)
// Parâmetros: nenhum
// Retorno: nenhum
void initMQTT() 
{
    MQTT.setServer(BROKER_MQTT, BROKER_PORT);   // Porta e Broker do MQTT para se Conectar
    MQTT.setCallback(mqtt_callback);            // Função de callback - Roda quando qualquer informação de tópicos 'subscribed' chega
}
  
// Função: Função de Callback do MQTT
//         Função chamada toda vez que uma informação de tópico subescritos chega.
// Parâmetros: nenhum
// Retorno: nenhum
void mqtt_callback(char* topic, byte* payload, unsigned int length) 
{
    String msg;
 
    // Obter a string do payload recebido
    for(int i = 0; i < length; i++) 
    {
       char c = (char)payload[i];
       msg += c;
    }
   
    if (msg.equals("L"))
    {
        digitalWrite(D0, LOW);
        EstadoSaida = '1';
    }
 
    // Verifica se deve colocar nivel alto de tensão na saída D0:
    if (msg.equals("D"))
    {
        digitalWrite(D0, HIGH);
        EstadoSaida = '0';
    }
     
}
  
//Função: reconecta-se ao broker MQTT (caso ainda não esteja conectado ou em caso de a conexão cair)
//        em caso de sucesso na conexão ou reconexão, o subscribe dos tópicos é refeito.
//Parâmetros: nenhum
//Retorno: nenhum
void reconnectMQTT() 
{
    while (!MQTT.connected()) 
    {
        Serial.print("* Tentando se conectar ao Broker MQTT: ");
        Serial.println(BROKER_MQTT);
        if (MQTT.connect(ID_MQTT)) 
        {
            Serial.println("Conectado com sucesso ao broker MQTT!");
            MQTT.subscribe(TOPICO_SUBSCRIBE); 
        } 
        else
        {
            Serial.println("Falha ao reconectar no broker.");
            Serial.println("Havera nova tentatica de conexao em 2s");
            delay(2000);
        }
    }
}

 
//Função: envia ao Broker o estado atual do output 
//Parâmetros: nenhum
//Retorno: nenhum
void EnviaEstadoOutputMQTT(void)
{
    if (EstadoSaida == '0')
      MQTT.publish(TOPICO_PUBLISH, "D");
    if (EstadoSaida == '1')
      MQTT.publish(TOPICO_PUBLISH, "L");
    if (EstadoSaida == '2')
        MQTT.publish(TOPICO_PUBLISH, "R"); // se foi reiniciado manda R para receber estado atual
        MQTT.publish(TOPICO_PUBLISH_IP, IP);
 
    //Serial.println("- Estado da saida D0 enviado ao broker!");
    delay(500);
}

// Função: Reconecta-se ao WiFi
// Parâmetros: nenhum
// Retorno: nenhum
void reconectWiFi() 
{
    //se já está conectado a rede WI-FI, nada é feito. 
    //Caso contrário, são efetuadas tentativas de conexão
    if (WiFi.status() == WL_CONNECTED)
        return;
         
    WiFi.begin(SSID, PASSWORD); // Conecta na rede WI-FI
     
    while (WiFi.status() != WL_CONNECTED) 
    {
        counwf = counwf +1;
      
        
        if( counwf % 40 == 0 ){

             if(digitalRead(D0) == HIGH){
             
                   digitalWrite(D0, LOW); 
             }else{

              digitalWrite(D0, HIGH); 
              
              }
          
        }

        if(counwf == 1000){
             ESP.restart();
        }
        
      
        delay(100);
    
    }
   
    Serial.println();
    Serial.println(TOPICO_PUBLISH);
    Serial.print("Conectado com sucesso na rede ");
    Serial.print(SSID);
    Serial.println("IP obtido: ");
    Serial.println(WiFi.localIP());


    

}
 
//Função: verifica o estado das conexões WiFI e ao broker MQTT. 
//        Em caso de desconexão (qualquer uma das duas), a conexão
//        é refeita.
//Parâmetros: nenhum
//Retorno: nenhum
void VerificaConexoesWiFIEMQTT(void)
{
    if (!MQTT.connected()) {

        if (WiFi.status() != WL_CONNECTED){
          
           reconectWiFi(); //se não há conexão com o WiFI, a conexão é refeita
        
        }else{
          
             reconnectMQTT(); //se não há conexão com o Broker, a conexão é refeita
     
         }
    
    }
    
}

void AvisoButton(void)
{ 
      Serial.println(digitalRead(D5));
       Serial.println(digitalRead(D2));
     
     if (digitalRead(D5) == HIGH && digitalRead(D2) == LOW) {

           // Serial.println("precionado");

          //  if (!MQTT.connected()) {
       
              // ESP.restart();

         // }


          quant = quant + 1;
          delay(500);      
      }

      
       

      if( quant == 2 && digitalRead(D5) == HIGH ){
        
                  digitalWrite(D0, HIGH);           
                  EstadoSaida = '0';
                //  Serial.println("LED Turned ON"); 
                  quant = 0;
                
        
       }else{

          if( digitalRead(D5) == LOW ){
            quant = 0;
          }

       }

}


void InitOutput(void)
{
    //IMPORTANTE: o Led já contido na placa é acionado com lógica invertida (ou seja,
    //enviar HIGH para o output faz o Led apagar / enviar LOW faz o Led acender)
    pinMode(D0, OUTPUT);
    digitalWrite(D0, HIGH);  
    pinMode(D5, INPUT_PULLUP); 
    pinMode(D2, INPUT_PULLUP); 
    
        
}
 
