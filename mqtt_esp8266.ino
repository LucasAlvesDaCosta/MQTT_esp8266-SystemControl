#include <stdlib.h>
//Bibliotecas do sensor DHT11
#include <Adafruit_Sensor.h>
#include <DHTesp.h>
#include <DHT.h>
#include <DHT_U.h>
//Bibliotecas do Firebase
#include <Firebase.h>
#include <FirebaseArduino.h>
#include <FirebaseCloudMessaging.h>
#include <FirebaseError.h>
#include <FirebaseHttpClient.h>
#include <FirebaseObject.h>
#include <FirebaseArduino.h>
//Bibliotecas do ESP8266 e de conexao com o Broker
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
// variaveis pre definidas
#define FIREBASE_HOST "sciia-e93c7.firebaseio.com"
#define FIREBASE_AUTH "vo5YW042uuVyXYOKmN55HPktMmOChGcPgTmomB9P"
#define DHTPIN D4      // o sensor dht11 foi conectado ao pino 2( D4 do node MCU)
#define DHTTYPE DHT11
// Update these with values suitable for your network.
 DHT dht(DHTPIN, DHTTYPE);

const char* ssid = "Helio"; // Nome da rede wifi
const char* password = "50281961"; // Senha da rede wifi
const char* mqttServer = "postman.cloudmqtt.com"; // servidor MQTT
const int mqttPort = 11487; //porta do servidor
const char* mqttUser = "gxgcwlgu"; //Nome do usiario
const char* mqttPassword = "SEEC9pFZ1OlS"; //Senha do usuario

WiFiClient espClient;
PubSubClient client(espClient);
// Pequeno mapeamento das portas utilizadas na programacao
/*
-------------------------------------------------
NodeMCU / ESP8266  |  NodeMCU / ESP8266  |
D0 = 16            |  D6 = 12            |
D1 = 5             |  D7 = 13            |
D2 = 4             |  D8 = 15            |
D3 = 0             |  D9 = 3             |
D4 = 2             |  D10 = 1            |
D5 = 14            |                     |
-------------------------------------------------
*/
const int lume_Rigth = 12; //iluminacao lado direito
const int lume_Left = 13; //iluminacao lado esquerdo
const int alter_1 = 14;
const int alter_2 = 5;
const int LED2 = LED_BUILTIN;
const unsigned int portaAnalogica = A0;//Porta analogica do NodeMCU
int valor_analogico; 
long previousMillis = 0; //Variavel de tempo do higrometro
long previousMillis2= 0; // Variavel de tempo do LDR
long umidadeInterval = 2000;// Tempo em ms do intervalo a ser executado
long ldrInterval = 5000;
float Lux;
int event;
int respBomba =0;
int luzON;
int alterValueLux;
int bombaAcionada;
void mqtt_callback(char* topic, byte* dados_tcp, unsigned int length);

void setup() { 
   
   pinMode(portaAnalogica, INPUT);//porta configurada como Entrada 
   pinMode(lume_Rigth, OUTPUT);//porta configurada como saida *
   pinMode(lume_Left, OUTPUT);// *
   pinMode(alter_1, OUTPUT);// *
   pinMode(alter_2, OUTPUT);//*
   digitalWrite(lume_Left, HIGH);//inicia os reles como desligados
   digitalWrite(lume_Rigth, HIGH);

   delay(300);
   Serial.begin(115200);
 
  WiFi.begin(ssid, password);
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  
  while (WiFi.status() != WL_CONNECTED) 
  {   
     delay(100);
    Serial.println("Conectando a WiFi..");
  }
  Serial.println("Conectado!"); 
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
 
  while (!client.connected()) {
    Serial.println("Conectando ao servidor MQTT...");
    
    if (client.connect("Projeto", mqttUser, mqttPassword ))
    {
 
      Serial.println("Conectado ao servidor MQTT!");  
 
    } else {
 
      Serial.print("Falha ao conectar ");
      Serial.print(client.state());
      delay(2000);
 
    }
  }
 
  client.publish("Status ","Reiniciado!");//Publicacao em topicos especificos
  client.publish("Placa","Em funcionamento");
  client.subscribe("LED");//topicos de interesse subInscritos 
  client.subscribe("EVT");
  client.subscribe("IRG");
}
/*-----CONTROLE DA LUZ-----*/
void controle_luz(int state){
  if(state == 1){    
    digitalWrite(lume_Rigth, LOW);//logica para acionar um rele
    client.publish("LUZ","iluminacao 01: ON"); 
  }
  if(state == 2){
    digitalWrite(lume_Left, LOW);  
    client.publish("LUZ","iluminacao 02: ON");   
  }
  if(state == 3){
     digitalWrite(lume_Rigth, LOW); 
    digitalWrite(lume_Left, LOW); 
    luzON = 1;
    client.publish("LUZ","iluminacao total: ON");
    }
  if(state == -1 && event != 1){//Logica para desligar um rele
    digitalWrite(lume_Rigth, HIGH); 
    client.publish("LUZ","iluminacao 01: OFF");
  }
  if(state == -2 && event != 1){
    digitalWrite(lume_Left, HIGH); 
    client.publish("LUZ","iluminacao 02: OFF");
  }
  if(state == -3 && event != 1){
     digitalWrite(lume_Rigth, HIGH); 
    digitalWrite(lume_Left, HIGH);
    luzON = 0;  
    client.publish("LUZ","iluminacao total: OFF");
   }

}
/*-------SENSOR DE DEPENDENCIA LUMINOSA - LDR-------*/
int sensorLdr(){
      Serial.println("Sensor LDR ");
 float valorLume;
 int ADC; 
 char MsgLumeMQTT[12];
 
 ADC = analogRead (portaAnalogica);
 Lux = (float)ADC; 
 sprintf(MsgLumeMQTT, "%f",Lux);
 if(event == 1){
    client.publish("LUME",MsgLumeMQTT);
 }
     float testBase = Firebase.getFloat("LDR");//testando o valor do Firebase
     Serial.print("Valor do Fire Base: ");
     Serial.println(testBase);
    if(Firebase.getFloat("LDR") != 35.5){
      valorLume = Firebase.getFloat("LDR");
    }else{
         valorLume = 37.0;
          Serial.print("Valor padrao: ");
          Serial.println(valorLume);
    }
  
  if(Lux <= valorLume && event == 1){
     controle_luz(3);
  }else if(Lux > valorLume && luzON == 1 && event == 1){
          controle_luz(-3);
          luzON = 0; 
        }
   
 return Lux;
}
/*----SENSOR DE TEMPERATRA DA GRAMA DHT11-----*/
float sensorDht11(){
  Serial.println("Sensor DHT");
  float umidade = dht.readHumidity();
  float temperatura = dht.readTemperature();
  
  char MsgUmidadeMQTT[12];
  char MsgTemperaturaMQTT[12];
   float faketemp = 30.0;
  if (isnan(temperatura) || isnan(umidade)) // caso o sensor apresente falha e retornado um valor falso para testes
  {
    
    Serial.println("Falha na leitura do dht11...");
    return faketemp;

  } 
  else{
     return temperatura;
  }
 
}
/*-------------UMIDADE DA GRAMA-HIGROMETRO----------------*/
int umidadeGrama(){
      Serial.println("Sensor Higrometro");
  String porcentStr = "";
   char MsgUmidMQTT[12];
   char MsgTempMQTT[12];
  float porcent = 0.0, porcentSeco = 0.0, temperaturaGrama;
  //Le o valor do pino A0 do sensor
  valor_analogico = analogRead(portaAnalogica);
  temperaturaGrama = sensorDht11();
 
  //Mostra o valor da porta analogica no serial monitor
  Serial.print("Porta analogica: ");
  Serial.println(valor_analogico);
  
   porcentSeco = (float)valor_analogico/1024*100;//Convertendo o valor em (%)
   porcent = 100 - porcentSeco;// retirando a diferenca para obter a (%)umidade
   sprintf(MsgTempMQTT,"%f",temperaturaGrama);
   sprintf(MsgUmidMQTT,"%f",porcent);
   
   client.publish("hdd", MsgUmidMQTT);//publica o valor da umidade
   client.publish("tmpr",MsgTempMQTT);//publica o valor da temperatura
   
  //Caso nao esteja ocorrendo um evento esta estrutura e verificada
  if(event != 1){
    if (porcent <= 50.0 && temperaturaGrama >= 24.0 && bombaAcionada != 1){
      Serial.print("Porcentagem de umidade: ");//Testes no monitor serial
      Serial.print(porcent);
      Serial.println("Temperatura da Grama: ");
      Serial.print(temperaturaGrama);
      client.publish("IRG","ON");
      delay(100);
       if(respBomba == 1){    
        bombaAcionada = 1;    
        client.publish("AVS","Irrigacao Acionada");//Topico de avisos
        umidadeInterval = 5000;
       }else{
          if(respBomba == 0){
             umidadeInterval = 50000;// Intervalo de verificacao
             bombaAcionada =0;
          }else if(respBomba == -1) umidadeInterval = 1000;// intervalo de verificacao     
        }             
     }
        Serial.print("Estado da Bomba :");
        Serial.print(bombaAcionada);
     if(porcent >= 80.0 && bombaAcionada == 1 || temperaturaGrama < 24.0){// Caso a irrigacao seja acionada
         Serial.println("Temperatura da Grama 2: ");
         Serial.print(temperaturaGrama);
         Serial.println("umidade 2: ");
         Serial.print(porcent);
        client.publish("IRG","OFF"); 
        client.publish("AVS","Irrigacao desligada");
         bombaAcionada = 0;
         respBomba = -1;
         umidadeInterval = 5000;          
      }
    if(temperaturaGrama > 35.0 && porcent < 75.0 && bombaAcionada != 1){//Tratar a temperatura ideal
       client.publish("IRG","ON");
       delay(100);
        if(respBomba == 1){
          client.publish("AVS","Irrigacao acionada: Temper.");
          umidadeInterval = 8000;
          bombaAcionada = 1;
        }else{
             if(respBomba == 0){
              umidadeInterval = 50000;
             bombaAcionada = 0;
          }else if(respBomba == -1) umidadeInterval = 1000;   
        }
    }else if(temperaturaGrama < 24.0){//Caso a temperatura do gramado esteja muito baixa
      client.publish("AVS","Temperatura do gramado muito Baixa. Gramado sujeito a doencas..");
      client.publish("AVS",MsgTempMQTT);
    }
  }else{
       client.publish("AVS","Irrigacao bloq..");
  }
 
}
/*-----------------------TOPICOS(DADOS)---------------------------*/
void callback(char* topic, byte* dados_tcp, unsigned int length) {
      Serial.println("Topicos MQTT");
    for (int i = 0; i < length; i++) {
      }
  if(strcmp(topic,"IRG")== 0){// Se for o topico de IRRIGACAO
     if ((char)dados_tcp[0] == 'O' && (char)dados_tcp[1] == 'K') {
      respBomba = 1;//confirmacao da bomda d agua
      bombaAcionada = 1;// simboliza o acionamento da bomba d agua
    } 
    if ((char)dados_tcp[0] == 'W' && (char)dados_tcp[1] == 'T') {
      respBomba = 0;//Bomba nao acionada-Aguardar...
    } 
  }

  if(strcmp(topic,"EVT") == 0){//se o topico for o de Evento
     if ((char)dados_tcp[0] == 'O' && (char)dados_tcp[1] == 'N') {
        sensorLdr();//verificar a iluminacao  
        event = 1;//evento acionado
    } else if((char)dados_tcp[0] == 'O' && (char)dados_tcp[1] == 'F'){     
      event = 0;//evento encerrado
      bombaAcionada = 0;//bomba desligada
      controle_luz(-3);//desliga todas as luzes
      alternador();//muda de sensor analogico
    }
    if((char)dados_tcp[0] == 'A' && (char)dados_tcp[1] == 'V'){
      alterValueLux = 1;//variavel nao configurada aqui pois existe a limitacao
    }                   // do aplicativo utilizado em trata-la, em outros casos
  }                     //pode ser utilizada.
  if(strcmp(topic,"LED")==0){
    if ((char)dados_tcp[0] == 'L' && (char)dados_tcp[1] == '1') {
        controle_luz(1);   
    } else if((char)dados_tcp[0] == 'D' && (char)dados_tcp[1] == '1' && event != 1){
     controle_luz(-1);  
    }
    if ((char)dados_tcp[0] == 'L' && (char)dados_tcp[1] == '2') {
      controle_luz(2);
    } else if((char)dados_tcp[0] == 'D' && (char)dados_tcp[1] == '2' && event != 1){
      controle_luz(-2);
    }
    if ((char)dados_tcp[0] == 'L' && (char)dados_tcp[1] == 'A') {
      //logica que verifica se o valor padrao da iluminacao deve ser alterado
        if(event == 1 && luzON != 1){
          float newLux = (float)sensorLdr();
          Firebase.setFloat("LDR",newLux);   
        }
      controle_luz(3);
    } else if((char)dados_tcp[0] == 'D' && (char)dados_tcp[1] == 'A' && event != 1){
     controle_luz(-3);  
    }
  }
} 
/*---------------ALTERNADOR DE SENSORES ANALOGICOS-------------------*/
void alternador(){
  int ativo1, ativo2;
  if(event != 1 && ativo1 != 1){// Alterna para o sensor de umidade        
     digitalWrite(alter_1, LOW);
      ativo1 = 1;
      ativo2 = 0;
     digitalWrite(alter_2, HIGH);     
  }else if(event == 1 && ativo2 != 1){//alterna para o sensor de luminosidade
    digitalWrite(alter_2, LOW); 
        ativo1 = 0;
    digitalWrite(alter_1, HIGH); 
        ativo2 = 1;
  }
}
//Utilize se necessario
/*---------CONVERSOR DE PONTO FLUTUANTE-----------------------------
String conversor(float val) {
  int i;
  char buff[10];
  String valueString = "";
    dtostrf(val, 4, 6, buff);  //4 is mininum width, 6 is precision
    Serial.print("String convertida: ");
    valueString += buff;
    Serial.println(valueString);
  return valueString;
}//----------------------------------------------------------------*/
void loop() {  
   client.loop();
   unsigned long currentMillis = millis();//Tempo atual em ms
   alternador();//inicia o alternador de portas em loop
   if(event == 1){
      if(currentMillis - previousMillis2 > ldrInterval)
        previousMillis2 = currentMillis;//intervalo de funcionamento
    sensorLdr();// sensor utilizado somente em eventos
   }
  
 //Logica de verificacao do tempo
  if (currentMillis - previousMillis > umidadeInterval) { 
    previousMillis = currentMillis;    // Salva o tempo atual
    umidadeGrama();// sensor utilizado em intervalos diferentes
  }
  
 }
