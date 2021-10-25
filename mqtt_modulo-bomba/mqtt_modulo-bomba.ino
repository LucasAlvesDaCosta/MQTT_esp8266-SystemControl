#include <stdlib.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

const char* ssid = "Helio";
const char* password = "50281961";
const char* mqttServer = "postman.cloudmqtt.com";
const int mqttPort = 11487;
const char* mqttUser = "gxgcwlgu";
const char* mqttPassword = "SEEC9pFZ1OlS";

WiFiClient espClient;
PubSubClient client(espClient);
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
const int boia = 14;//pino do sensor boia
const int solenoide = 4;//canal da valvula solenoide
const int bomba = 0;//canal da bomba d agua
int event;
int action;
const int LED2 = LED_BUILTIN;
int valor_analogico;
long previousMillis = 0; // Variavel de controle do tempo
long tempoEspera = 0;
void mqtt_callback(char* topic, byte* dados_tcp, unsigned int length);

void setup() { 
   pinMode(boia, INPUT);//configurado como entrada
   pinMode(solenoide, OUTPUT);//configurado com saida *
   pinMode(bomba, OUTPUT);// *
   digitalWrite(solenoide, HIGH);// iniciado como desligado *
   digitalWrite(bomba, HIGH);// *
   delay(300);
   Serial.begin(115200);
 
  WiFi.begin(ssid, password);  
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
 
  client.publish("Status ","Reiniciado!");
  client.publish("motobomba","Em funcionamento");

  client.subscribe("IRG");//Topicos de interesse
  client.subscribe("EVT");
}
/*-------------------INTERRUPTOR DA MOTOBOMBA----------------------*/
void interruptor(int state){
  if(state == 1){//liga a solenoide e em seguida a bomba
     digitalWrite(solenoide, LOW);
      delay(1000);// atraso de 1 seg 
     digitalWrite(bomba, LOW);   
  }
  if(state == -1){
    digitalWrite(bomba, HIGH);// desliga a bomba e em segurida a solenooide
      delay(1000);// atraso se 1 seg
     digitalWrite(solenoide, HIGH);
   client.publish("AVS","Bomba desligada");
  }
}
/*-----------CONTROLE DA BOMBA-------------------*/
void controle_bomba(){
  if(action == 1 && event != 1){
     if(!digitalRead(boia)){
      client.publish("IRG","OK");
      client.publish("AVS","Bomba acionada");
      interruptor(1);      
     }else{
       client.publish("IRG","WT");
       tempoEspera = 5000; 
       client.publish("AVS","Reservatorio: Pouca agua"); 
      } 
  }else{
    Serial.println("Nenhuma acao para bomba de agua");
  }

}

/*----------------TOPICOS(DADOS)-----------------*/
void callback(char* topic, byte* dados_tcp, unsigned int length) {
      Serial.println("Topicos MQTT");
    for (int i = 0; i < length; i++) {
      }
      
 if(strcmp(topic,"EVT") == 0){
   if ((char)dados_tcp[0] == 'O' && (char)dados_tcp[1] == 'N') {
        event = 1;
    } else if((char)dados_tcp[0] == 'O' && (char)dados_tcp[1] == 'F'){     
       event = 0;      
    }

  }
  if(strcmp(topic,"IRG")== 0){//solicitacao de acionamento da bomba d agua
     if ((char)dados_tcp[0] == 'O' && (char)dados_tcp[1] == 'N') {
      action = 1;
      controle_bomba();
    } //solicitacao de desligamento da bomba d agua
    if ((char)dados_tcp[0] == 'O' && (char)dados_tcp[1] == 'F' && (char)dados_tcp[2] == 'F') {
      action = 0;
      interruptor(-1);
    } 
    
  }

} 

void loop() {  
   client.loop();
   unsigned long currentMillis = millis(); //Tempo atual em ms
 //Lógica de verificacaoo do tempo
  if (currentMillis - previousMillis > tempoEspera) { 
    previousMillis = currentMillis;    // Salva o tempo atual
    controle_bomba();
     if(!digitalRead(boia)){//informa entre intervalos o estado da boia
      client.publish("AVS","Reservatorio: Cheio");
     }
  }
  
 }
