#include <SPI.h>
#include <SD.h>
File monFichier;


// constantes
const unsigned long mnt = 60000;    	// 1 minute = 60 secondes = 60000 millis

const int Capt1 = 8;            // broche du capteur on/off du thermostat
boolean Capt1State = 1;

const boolean NO_OFF = HIGH;    // quand on utilise relai en NO
const boolean NO_ON = LOW;      // quand on utilise relai en NO


class Allumage_controle {
  int ACpin;
  int ACnum_allu;                  // numéro de cycle d'allumage depuis le dernier OffManuel
  unsigned long ACdurON[3];        // durée souhaitée de mise en marche (en Milli) ([0]= durée réglée en cours, [1]=durée de on1, [2]=durée de on2)
  unsigned long ACdurOFF[3];       // durée souhaitée avant prochaine mise en marche (en Milli) ([0]=durée réglée en cours, [1]=durée de prem, [2]=durée de off)
  unsigned long ACtimeON;          // dernier instant de mise en marche en Milli
  unsigned long ACtimeOFF;         // dernier instant d'arrêt en Milli
  
  public:
  Allumage_controle(int pin, unsigned int on1, unsigned int on2, unsigned int off1, unsigned int off2) {
  // pin  : broche du relai de commande concerné
  // on1  : durée de mise en marche, pour le premier départ demandé (en mn, < 60)
  // on2  : durée de mise en marche, pour les départs suivants, jusqu'au stop demandé (en mn, < 60)
  // off1 : durée d'attente avant la deuxième mise en marche (en mn, <60)
  // off2 : durée d'attente avant les mises en marche suivantes (en mn, < 60)
    ACpin = pin;
    pinMode(ACpin, OUTPUT);
    digitalWrite(ACpin, NO_OFF);
    
    ACdurON[1] = on1 * mnt;
    ACdurON[2] = on2 * mnt;
    ACdurON[0] = ACdurON[1];
    ACnum_allu = 0;
    
    ACdurOFF[1] = off1 * mnt;
    ACdurOFF[2] = off2 * mnt;
    ACdurOFF[0] = mnt;
    
    ACtimeON = 0;
    ACtimeOFF = 0;
  }

  void OnVersOff() {
    unsigned long currentMilli = millis();
    // si la commande est en marche depuis plus longtemps que la durée prévue : la mettre OFF
    if ((GetPin() == NO_ON) && (currentMilli - ACtimeON >= ACdurON[0])) {
      digitalWrite(ACpin, NO_OFF);
      ACtimeOFF = currentMilli;
    }
  }

  void OffVersOn() {
    unsigned long currentMilli = millis();
    // si la commande est arretée depuis plus longtemps que la durée prévue : accepter de la mettre ON
    if ((GetPin() == NO_OFF) && (currentMilli - ACtimeOFF >= ACdurOFF[0])) {
      digitalWrite(ACpin, NO_ON);
      ACtimeON = currentMilli;
	  
      ACnum_allu += 1;
      if ((ACnum_allu == 1) || (ACnum_allu > 40)) {
        ACdurON[0] = ACdurON[1];
        ACdurOFF[0] = ACdurOFF[1];
      } else {
        ACdurON[0] = ACdurON[2];
        ACdurOFF[0] = ACdurOFF[2];
      }
    }
  }
  
  void OffManuel() {
    ACnum_allu = 0;
    ACdurON[0] = 0;
  }
  
/*  void VerifMilli() {
    // si le compteur millis() a été réinitialisé (après 50 jours) alors réinitialiser les heures de référence
    unsigned long currentMilli = millis();
    if ((currentMilli < ACtimeON) || (currentMilli < ACtimeOFF)) {
      ACtimeON = 0;
      ACtimeOFF = 0;
    }
  }*/

  int GetPin() {
    return(digitalRead(ACpin));
  }

  void GetInfos() {
    unsigned long currentMilli = millis();

    Serial.println(" ");
    Serial.print("ACnum_allu = ");Serial.print(ACnum_allu);Serial.print(" // ");
    Serial.print("ACstate = ");
    if (GetPin() == NO_OFF) {
      Serial.println("OFF"); }
    else {
      Serial.println("ON"); }

    Serial.print("Duree ON = ");Serial.print(currentMilli - ACtimeON);Serial.print(" >=? ");Serial.print(ACdurON[0]);
    if (currentMilli - ACtimeON >= ACdurON[0]) {
      Serial.println(" => on peut eteindre"); }
    else {
      Serial.println(" => on n'eteind pas"); }

    Serial.print("Duree OFF = ");Serial.print(currentMilli - ACtimeOFF);Serial.print(" >=? ");Serial.print(ACdurOFF[0]);
    if (currentMilli - ACtimeOFF >= ACdurOFF[0]) {
      Serial.println(" => on peut allumer"); }
    else {
      Serial.println(" => on ne peut pas allumer"); }
  }
  
  
  void GetInfosSD() {
    unsigned long currentMilli = millis();

    monFichier = SD.open("datas", FILE_WRITE);

    monFichier.print(currentMilli);
    monFichier.print(";");
    
    if (GetPin() == NO_OFF) {
      monFichier.print("OFF"); }
    else {
      monFichier.print("ON"); }
    monFichier.print(";");
    
    monFichier.print(ACnum_allu);
    monFichier.print(";");
    

    monFichier.print(currentMilli - ACtimeON);
    monFichier.print(";");
    
    monFichier.print(ACdurON[0]);
    monFichier.print(";");
    
    if (currentMilli - ACtimeON >= ACdurON[0]) {
      monFichier.print(" => eteindre"); }
    else {
      monFichier.print(" => ne pas eteindre"); }
    monFichier.print(";");

    monFichier.print(currentMilli - ACtimeOFF);
    monFichier.print(";");
        
    monFichier.print(ACdurOFF[0]);
    monFichier.print(";");
    
    if (currentMilli - ACtimeOFF >= ACdurOFF[0]) {
      monFichier.print(" => allumer autorisé"); }
    else {
      monFichier.print(" => allumer interdit"); }
    monFichier.print(";");
    
    monFichier.close();
  }
};

Allumage_controle AC1(2,30,15,13,13);



void setup() {
  SD.begin(4);

  pinMode(Capt1, INPUT_PULLUP);
  
  Serial.begin(9600);
  Serial.println("Bonjour");delay(1000);
  Serial.print("mnt ");Serial.println(mnt);
} 


void loop() {
  delay(4000);

//  AC1.VerifMilli();
  AC1.GetInfos();
  AC1.GetInfosSD();

  AC1.OnVersOff();
    
  // vérifier ce que demande le thermostat
  Capt1State = digitalRead(Capt1);
  
  Serial.print("Etat du bouton = ");Serial.print(Capt1State);

  monFichier = SD.open("datas", FILE_WRITE);
  
  if (Capt1State == LOW) { // nous sommes en INPUT_PULLUP > LOW = contact ferme = ON
    Serial.println(" => on veut allumer (OffVersOn)");
    monFichier.println("ON demandé");
    AC1.OffVersOn();
  }
  else {
    Serial.println(" => on veut eteindre (OffManuel)");
    monFichier.println("OFF demandé");
    AC1.OffManuel();
  }

  monFichier.close();
  
}
