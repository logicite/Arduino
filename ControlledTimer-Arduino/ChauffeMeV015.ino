#include <avr/wdt.h>
#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <EasyScheduler.h>


// constantes
const unsigned long MNT = 60000;	// 1 minute = 60 secondes = 60000 millis
const bool NO_OFF = HIGH;			// quand on utilise relai en NO
const bool NO_ON = LOW;				// quand on utilise relai en NO

const int PINTHERM = 8;         // broche du capteur on/off du thermostat
const int PINCHAUD = 2;        // broche de la cde de relai

// Ethernet & MQTT
byte MAC[] = { 0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02 };
//byte ip[] = { 192, 168, 1, 50 };
char MQTTSERVER[]= "jarvis.lan";
char MQTTIDCLIENT[] = "IOT/chaudiere";

// variables
unsigned long chaudDurOn[3] = {30 * MNT, 32 * MNT, 14 * MNT};	// durée souhaitée de mise en marche (en Milli)
								    // ([0]= durée réglée en cours, [1]=durée de premier lancement, [2]=durée des lancements suivants)
unsigned long chaudDurOff[3] = {1 * MNT, 14 * MNT, 13 * MNT};	// durée souhaitée avant prochaine mise en marche (en Milli)
								    // ([0]=durée réglée en cours, [1]=durée de premiere interruption, [2]=durée des interruptions suivantes)
unsigned long chaudTimOn = 0;		// dernier instant de mise en marche en Milli
unsigned long chaudTimOff = 0;		// dernier instant d'arrêt en Milli
int chaudNumLanc = 0;				// numéro de cycle d'allumage depuis le dernier OffManuel

bool chaudSTT;    					// état de la Commande (relai)
bool chaudDDE = false;				// est-ce qu'on demande du chauffage

unsigned long mqttLastConnect = 0; 	// dernier instant de contact du broker MQTT


EthernetClient OethClient;
PubSubClient OmqttClient(MQTTSERVER, 1883,  OethClient);
Schedular TAlive;


// fonctions Ethernet
void ETHmaintain() {
switch (Ethernet.maintain()) {
    case 1: //renewed fail -> on met la carte en boucle pour activer le watchdog
    case 3: //rebind fail -> on met la carte en boucle pour activer le watchdog
      while(1){;}

    case 2: //renewed success
    case 4: //rebind success
      ;
  }
}


// fonctions MQTT
void MqttPub(String tpc, String msg) {
  char mqttMsg[30];
  char mqttTpc[30];

  tpc.toCharArray(mqttTpc, tpc.length()+1);
  msg.toCharArray(mqttMsg, msg.length()+1); 
  OmqttClient.publish(mqttTpc, mqttMsg, 1);
}


void MqttCallback(char* topic, byte* payload, unsigned int length) {
  // handle message arrived
  // va mettre à jour les variables :
  // topic sb/chauf/tps : xxYYzzTT
  //	chaudDurOn[1 et 2] < xx et YY
  //	chaudDurOff[1 et 2] < zz et TT
  //	+ gérer la demande
  // VALEURS MAXIMUMS ???
  
  String strTopic = String(topic);

  if (strTopic == "sb/chaud/tps") {
    chaudDurOn[1] = MNT * ((payload[0] - 48 ) * 10 + (payload[1] - 48 ));
    chaudDurOn[2] = MNT * ((payload[2] - 48 ) * 10 + (payload[3] - 48 ));
    chaudDurOff[1] = MNT * ((payload[4] - 48 ) * 10 + (payload[5] - 48 ));
    chaudDurOff[2] = MNT * ((payload[6] - 48 ) * 10 + (payload[7] - 48 ));
  }

  if (strTopic == "sb/chaud/dde") {
	  chaudDDE = (payload[0] == 49);	// 49 = 1 en ASCII
  }

  if ((strTopic == MQTTIDCLIENT) && (payload[0] == 73)) {
    MqttPub(MQTTIDCLIENT, String(chaudDurOn[1]/MNT) + String(chaudDurOn[2]/MNT) + String(chaudDurOff[1]/MNT) + String(chaudDurOff[2]/MNT));
    mqttLastConnect = millis();
  }
}


void MqttConnect() {
  if (!OmqttClient.connected()) {                 // si je ne suis pas connecté au broker
    OmqttClient.connect(MQTTIDCLIENT);            // essayer de se connecter
    OmqttClient.subscribe("sb/chaud/dde");
    OmqttClient.subscribe("sb/chaud/tps");
    OmqttClient.subscribe(MQTTIDCLIENT);
    MqttPub(MQTTIDCLIENT, "connect");
  }
}


void MqttAlive(){
  unsigned long CurrentInter = millis() - mqttLastConnect;
  
//  if (CurrentInter > 30 * MNT) {        // si le dernier message MQTT est vieux de 30 MN
//    while(1){;}                          // activer le watchdog pour reboot
//  }
//  if (CurrentInter > 2 * MNT) {         // si le dernier message MQTT est vieux de 2 MN
//    Ethernet.begin(MAC);
//  }
    
  MqttPub(MQTTIDCLIENT, "I");                       // on publie un 'I' (dde info) pour
                                                    // faire réagir la fonction de callback (qui datera le dernier message)
  MqttPub("nb/chauf/therm", String(chaudDDE));      // on publie aussi ttes nos données actuelles
  MqttPub("nb/chaud/stt", String(chaudSTT));        //
}


void GetTherm() {
// donne l'état de la demmande de chauffage
// à enlever si le thermostat n'est plus branché à cette carte
  bool currentTherm = !digitalRead(PINTHERM); // nous sommes en INPUT_PULLUP > LOW = contact ferme = ON
  if (currentTherm != chaudDDE) {
  	chaudDDE = currentTherm;
  }
}


void ChkCommande() {
// donne l'état de la cde de relai en cours (ON/true ou OFF/false)
  bool currentCde = (digitalRead(PINCHAUD) == NO_ON);
  if (currentCde != chaudSTT){
    chaudSTT = currentCde;
  }
}


void OnVersOff() {
// si la cde est en marche depuis plus longtemps que la durée prévue : la mettre OFF
  if ((chaudSTT == true) && (millis() - chaudTimOn >= chaudDurOn[0])) {
    digitalWrite(PINCHAUD, NO_OFF);
    chaudTimOff = millis();
  }
}


void OffVersOn() {
// si la cde est arretée depuis plus longtemps que la durée prévue : accepter de la mettre ON
  if ((chaudSTT == false) && (millis() - chaudTimOff >= chaudDurOff[0])) {
    digitalWrite(PINCHAUD, NO_ON);
    chaudTimOn = millis();
  
    chaudNumLanc += 1;
    if (chaudNumLanc == 1) {
      chaudDurOn[0] = chaudDurOn[1];
      chaudDurOff[0] = chaudDurOff[1];
    } else {
      chaudDurOn[0] = chaudDurOn[2];
      chaudDurOff[0] = chaudDurOff[2];
    }
  }
}


void OffManuel() {
  chaudNumLanc = 0;
  chaudDurOn[0] = 0;
}


void setup() {
  // watchdog
  wdt_enable(WDTO_8S);

  // on crée la connexion internet après le watchdog
  // comme ça on réessaiera au moins toutes les 8 secondes
//  if (Ethernet.begin(MAC) == 0) {
    // si pas de réseau, inutile de continuer ???
//    while(1){;}
//  }

//  wdt_reset();

  OmqttClient.setCallback(MqttCallback);

  pinMode(PINTHERM, INPUT_PULLUP);
  pinMode(PINCHAUD, OUTPUT);
  
  digitalWrite(PINCHAUD, NO_OFF);  // au démarrage la cde est sur OFF

  TAlive.start(500);
} 


void loop() {
  ETHmaintain();
  OmqttClient.loop();

  delay(500);
  wdt_reset();

  // vérifier si le broker MQTT est connecté et qu'il répond. Publier les infos. Toutes les minutes.
  MqttConnect();
  TAlive.check(MqttAlive,MNT);

  // vérifier ce que demande le thermostat
  GetTherm();     // à enlever si le thermostat n'est plus branché à cette carte

  // vérifier l'état de la commande et s'il ne faudrait pas l'éteindre
  ChkCommande();
  OnVersOff();
 
  if (chaudDDE == true) {
    OffVersOn();
  } else {
    OffManuel();
  }
}
