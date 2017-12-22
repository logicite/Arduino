/*
LimitOn.h
Bibliothèque pour controler les durées d'allumage et d'extinction d'un relai
Créé par @logicite
*/

#ifndef LimitOn_h
#define LimitOn_h

#include "Arduino.h"
class LimitOn{
 public:
  LimitOn(int pin, boolean onState, unsigned long on1, unsigned long on2, unsigned int rst, unsigned long off1, unsigned long off2);
  void OnToOff();
  void OffToOn();
  void ForceOff();
  void UpdtSet(unsigned long on1, unsigned long on2, unsigned int rst, unsigned long off1, unsigned long off2);
  unsigned long *GiveInfo(unsigned long *info);
  
 private:
  int _pin;
  boolean _onState;
  int _numAllu;             // numéro de cycle d'allumage
  int _rst;                 // numéro d'allumage après lequel il faudra faire une relance de durée on1
  unsigned long _durON[3];  // durée souhaitée de mise en marche (en milli)
                            // ([0]= durée réglée en cours, [1]=durée de on1, [2]=durée de on2)
  unsigned long _durOFF[3]; // durée souhaitée avant prochaine mise en marche (en Milli)
                            // ([0]=durée réglée en cours, [1]=durée de off1, [2]=durée de off2)
  unsigned long _timeON;    // dernier instant de mise en marche en milli
  unsigned long _timeOFF;   // dernier instant d'arrêt en milli
};

#endif
