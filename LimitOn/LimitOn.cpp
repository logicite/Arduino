/*
LimitOn.cpp
Bibliothèque pour controler les durées d'allumage et d'extinction d'un relai
Créé par @logicite
*/

#include "Arduino.h"
#include "LimitOn.h"

LimitOn::LimitOn(int pin, boolean onState, unsigned long on1, unsigned long on2, unsigned int rst, unsigned long off1, unsigned long off2){
// pin  : broche du relai de commande concerné
// onState : état quand le relai doit être mis sur ON (pour pouvoir gérer les relais en NO ou NC)
// on1  : durée de mise en marche, pour le premier départ demandé (en milli)
// on2  : durée de mise en marche, pour les départs suivants (en milli)
// rst  : numéro d'allumage après lequel il faudra faire une relance de durée on1
// off1  : durée d'attente avant la deuxième mise en marche (en milli)
// off2  : durée d'attente avant les mises en marche suivantes (en milli)
_pin = pin;
_onState = onState;

pinMode(_pin, OUTPUT);
digitalWrite(_pin, !_onState); // on démarre avec le relai sur OFF

_durON[1] = on1;
_durON[2] = on2;
_numAllu = 0;
_rst = rst;

_durOFF[1] = off1;
_durOFF[2] = off2;
_durOFF[0] = 60000;    // on laisse 1mn après la mise sous tension avant le premier ON

_timeON = 0;
_timeOFF = 0;
}


void LimitOn::OnToOff(){    unsigned long currentMilli = millis();    // si la commande est en marche depuis plus longtemps que la durée prévue : la mettre OFF    if ((digitalRead(_pin) == _onState) && (currentMilli - _timeON >= _durON[0])) {  digitalWrite(_pin, !_onState);  _timeOFF = currentMilli;    }}
void LimitOn::OffToOn(){ unsigned long currentMilli = millis();    // si la commande est arretée depuis plus longtemps que la durée prévue : accepter de la mettre ON    if ((digitalRead(_pin) != _onState) && (currentMilli - _timeOFF >= _durOFF[0])) {  digitalWrite(_pin, _onState);  _timeON = currentMilli;     _numAllu += 1;  if ((_numAllu == 1) || (_numAllu > _rst)) {   _durON[0] = _durON[1];   _durOFF[0] = _durOFF[1];       } else {         _durON[0] = _durON[2];         _durOFF[0] = _durOFF[2];       }    }}
void LimitOn::ForceOff(){    _numAllu = 0;    _durON[0] = 0;}
unsigned long *LimitOn::GiveInfo(unsigned long *info){ unsigned long currentMilli = millis(); info[0] = {currentMilli}; if (digitalRead(_pin) == _onState) { info[1] = 1; } else { info[1] = 0; } info[2] = _numAllu; info[3] = _rst; info[4] = _durON[0]; info[5] = _durON[1]; info[5] = _durON[2]; info[6] = _durOFF[0]; info[7] = _durOFF[1]; info[8] = _durOFF[2]; return info;}
void LimitOn::UpdtSet(unsigned long on1, unsigned long on2, unsigned int rst, unsigned long off1, unsigned long off2){    _durON[1] = on1;    _durON[2] = on2;  _rst = rst;        _durOFF[1] = off1;    _durOFF[2] = off2;}
