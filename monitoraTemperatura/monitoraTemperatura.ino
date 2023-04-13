/* 
 *  Monitoramento de Temperatura com REST API
 *  Sensor: BMP180 
 *  Módulo: WiFi ESP8266 NodeMCU
 *  Conexão: 3.3V VIN
 *  Importante: Não conectar acima de 5V pois senão o sensor poderá ser danificado.
 */

// Bibliotecas para se comunicar com a placa
#include <SFE_BMP180.h>
#include <Wire.h>

// Bibliotecas para comunicar no Wifi e criar os dados JSON
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>

// Biblioteca pra criar o REST API
#include <ESPAsyncWebServer.h>

// You will need to create an SFE_BMP180 object, here called "pressure":
SFE_BMP180 pressure;

#define ALTITUDE 935.0 // Ajuste de acordo com sua altitude

String dadosJson = ""; // Vou mapear essa variável com os dados JSON

// Definindo variável com os dados JSON com 200MB disponível
StaticJsonDocument<200> dadosSensor;

// Criar webserver na porta 80
AsyncWebServer server(80);

void setup()
{
  // Configurar taxa de transferência
  Serial.begin(9600); // Velocidade padrão
  // Serial.begin(115200);
  Serial.println("Monitoramento de Temperatura com sensor BMP180.");

  // Conectar no WiFi
  WiFi.begin("ANDON", "andon@aro");

  // Initialize the sensor
  Serial.print("Conectando...");
  
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  // Printar MAC e IP.
  Serial.println();
  Serial.print("IP Conectado. IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("Endereço MAC: ");
  Serial.println(WiFi.macAddress());

  // Inicializar sensor
  if (pressure.begin())
    Serial.println("Sensor BMP180 inicializado com sucesso.");
  else
  {
    Serial.println("Falha ao iniciar sensor BMP180\n\n");
    while(1); // Faz loop infinito no script.
  }

  // Caminho para consulta do Objeto JSON
  // server.on("/informacao", HTTP_GET, getInformacao);  

  // Inicializar WebServer
  // Tradução: On Request - Quando alguém acessar o endereço.
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
  request->send(200, "text/plain", "Monitoramento de Temperatura da Sala dos Servidores");
  });

  // Criar caminho para temperatura usando a função getTemperatura
  server.on("/dadosSensor", HTTP_GET, getTemperatura);
  
  // Erro 404 - Parte 1
  server.onNotFound(notFound);
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  server.begin();
  
}

void loop()
{
  // Variáveis utilizadas junto ao sensor
  char status;
  double T,P,p0,a;

 // Zerei as variáveis para poder mapear com os valores
 dadosJson = "";
 dadosSensor["altitude"] = "";
 dadosSensor["pes"]    = "";
 dadosSensor["temperatura_celsius"] = "";
 dadosSensor["pressao_seaLevel"] = "";
 dadosSensor["pressao_seaLevel_mb"] = "";
 dadosSensor["pressao_seaLevel_inHg"] = "";
 dadosSensor["altitude_pressaoBased_metros"] = "";        
 dadosSensor["altitude_pressaoBased_pes"] = "";        

   
  // Adicionar valores e dados JSON
  // Formula pés para kilometros: Multiplicar pelo valor 0.0003048
  
  dadosSensor["altitude"] = ALTITUDE,0;
  dadosSensor["pes"] = ALTITUDE*3.28084,0;

  // ***** Temperatura *****
  status = pressure.startTemperature();
  if (status != 0)
  {
    // Aguardar pelo status de temperatura do sensor
    delay(status);

    // Retorna o valor da temperatura e guarda na variável T
    status = pressure.getTemperature(T);

    // ***** Temperatura *****
    if (status != 0)
    {
      // Armazena dados de temperatura celsius
      dadosSensor["temperatura_celsius"] = T,2;

      // Começar coleta de pressão
      status = pressure.startPressure(3);

      // ***** Pressão *****
      if (status != 0)
      {

        // Aguardar pelos dados de pressão do sensor
        delay(status);

        // Retorna o valor da temperatura e guarda na variável P
        // Note que a função utiliza a variável de Temperatura (T)
        status = pressure.getPressure(P,T);

        if (status != 0)
        {
        // Pressão com relação à sua altitude
        dadosSensor["pressao_mb"] = P,2;
        dadosSensor["pressao_inHg"] = P*0.0295333727,2;

        // Pressão com relação à altitude do nível do mar
        p0 = pressure.sealevel(P,ALTITUDE);
        dadosSensor["pressao_seaLevel"] = p0,2;
        dadosSensor["pressao_seaLevel_mb"] = P,2;
        dadosSensor["pressao_seaLevel_inHg"] = p0*0.0295333727,2;

        // Altitude com base dos níveis de pressão
        // Calculada usando pressão base com relação ao nível do mar
        a = pressure.altitude(P,p0);
        dadosSensor["altitude_pressaoBased_metros"] = a,0;        
        dadosSensor["altitude_pressaoBased_pes"] = a*3.28084,0;        
          }
          else Serial.println("Erro nos cálculos de pressão com os dados recebidos.\n");
        }
        else Serial.println("Erro ao receber dados de pressão do sensor.\n");
      }
      else Serial.println("Erro nos cálculos de temperatura dos dados recebidos do sensor.\n");
    }
else Serial.println("Erro ao receber dados de temperatura do sensor.\n");

  // Guardo os dados JSON na variável de resposta para consulta HTTP
  serializeJsonPretty(dadosSensor, dadosJson);

  // Break line
  Serial.println();

  // Atualiza as informacoes da variável a cada 1 minuto
  delay(6000);
}

// Erro 404 - Parte 2
void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "404 - Not Found");
}

// Pega temperatura sob demanda, ao fazer um request http e determino content-type APP/JSON.
// Caminho definido na linha 74
void getTemperatura(AsyncWebServerRequest *request){
  request->send(200, "application/json", dadosJson);
  }