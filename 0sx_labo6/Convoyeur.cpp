#include "Convoyeur.h"

Convoyeur::Convoyeur(
  byte moteurPin1,
  byte moteurPin2,
  byte manettePin,
  byte boutonPin,
  byte afficheurCLK,
  byte afficheurDIN,
  byte afficheurCS
)
  : _moteurPin1(moteurPin1),
    _moteurPin2(moteurPin2),
    _manettePin(manettePin),
    _bouton(boutonPin, true),
    _afficheur(U8G2_R0, afficheurCLK, afficheurDIN, afficheurCS, U8X8_PIN_NONE) {
}

void Convoyeur::begin() {
  pinMode(_moteurPin1, OUTPUT);
  pinMode(_moteurPin2, OUTPUT);
  pinMode(_manettePin, INPUT);

  _bouton.setDebounceMs(30);
  _bouton.setPressMs(600);
  _bouton.attachClick(onClick, this);
  _bouton.attachLongPressStart(onLongPress, this);

  _afficheur.begin();

  appliquerVitesseMoteur(0);
  afficherSymbole(0);
}

void Convoyeur::update() {
  _bouton.tick();

  switch (_etat) {
    case ConvState::INACTIF:
      etatInactif();
      break;

    case ConvState::ACTIF:
      etatActif();
      break;

    case ConvState::MANUEL:
      etatManuel();
      break;

    case ConvState::CONSTANT:
      etatConstant();
      break;
  }
}

bool Convoyeur::estEnFonction() const {
  return _etat != ConvState::INACTIF;
}

int Convoyeur::getVitesse() const {
  return _vitesseActuelle;
}

int Convoyeur::getVitesseMoteur() const {
  return _vitesseActuelle;
}

ConvState Convoyeur::getState() const {
  return _etat;
}

void Convoyeur::setNumeroEtudiantPair(bool estPair) {
  _numeroEtudiantPair = estPair;
  _dernierSymbole = -1;
}

bool Convoyeur::setVitesseConsigne(int vitesse) {
  if (!estEnFonction()) {
    return false;
  }

  _vitesseConstante = constrain(vitesse, -100, 100);

  if (_vitesseConstante == 0) {
    appliquerVitesseMoteur(0);
    afficherSymbole(1);
    changerEtat(ConvState::ACTIF);
    return true;
  }

  appliquerVitesseMoteur(_vitesseConstante);

  if (_vitesseConstante > 0) {
    afficherSymbole(2);
  } else {
    afficherSymbole(3);
  }

  changerEtat(ConvState::CONSTANT);
  return true;
}

void Convoyeur::changerEtat(ConvState nouvelEtat) {
  if (_etat == nouvelEtat) {
    return;
  }

  _etat = nouvelEtat;
  _clicDetecte = false;
  _appuiLongDetecte = false;
}

int Convoyeur::lireVitesseManette() {
  int valeurBrute = analogRead(_manettePin);
  int vitesse = map(valeurBrute, 0, 1023, -100, 100);

  if (abs(vitesse) <= _zoneMorte) {
    vitesse = 0;
  }

  return vitesse;
}

void Convoyeur::appliquerVitesseMoteur(int vitesse) {
  _vitesseActuelle = vitesse;
  int pwm = map(abs(vitesse), 0, 100, 0, 255);

  if (vitesse > 0) {
    analogWrite(_moteurPin1, pwm);
    analogWrite(_moteurPin2, 0);
  } else if (vitesse < 0) {
    analogWrite(_moteurPin1, 0);
    analogWrite(_moteurPin2, pwm);
  } else {
    analogWrite(_moteurPin1, 0);
    analogWrite(_moteurPin2, 0);
  }
}

void Convoyeur::afficherSymbole(byte symbole) {
  if (_dernierSymbole == symbole) {
    return;
  }

  _dernierSymbole = symbole;
  _afficheur.clearBuffer();

  switch (symbole) {
    case 0:
      _afficheur.drawBitmap(0, 0, 1, 8, bitmapStop);
      break;

    case 1:
      if (_numeroEtudiantPair) {
        _afficheur.drawBitmap(0, 0, 1, 8, bitmapAttentePair);
      } else {
        _afficheur.drawBitmap(0, 0, 1, 8, bitmapAttenteImpair);
      }
      break;

    case 2:
      if (_numeroEtudiantPair) {
        _afficheur.drawBitmap(0, 0, 1, 8, bitmapAvancePair);
      } else {
        _afficheur.drawBitmap(0, 0, 1, 8, bitmapAvanceImpair);
      }
      break;

    case 3:
      if (_numeroEtudiantPair) {
        _afficheur.drawBitmap(0, 0, 1, 8, bitmapReculePair);
      } else {
        _afficheur.drawBitmap(0, 0, 1, 8, bitmapReculeImpair);
      }
      break;
  }

  _afficheur.sendBuffer();
}

void Convoyeur::etatInactif() {
  appliquerVitesseMoteur(0);
  afficherSymbole(0);

  if (_appuiLongDetecte) {
    changerEtat(ConvState::ACTIF);
  }
}

void Convoyeur::etatActif() {
  appliquerVitesseMoteur(0);
  afficherSymbole(1);

  if (_appuiLongDetecte) {
    changerEtat(ConvState::INACTIF);
    return;
  }

  int vitesse = lireVitesseManette();

  if (vitesse != 0) {
    changerEtat(ConvState::MANUEL);
  }
}

void Convoyeur::etatManuel() {
  int vitesse = lireVitesseManette();

  if (_appuiLongDetecte) {
    changerEtat(ConvState::INACTIF);
    return;
  }

  if (vitesse == 0) {
    appliquerVitesseMoteur(0);
    changerEtat(ConvState::ACTIF);
    return;
  }

  appliquerVitesseMoteur(vitesse);

  if (vitesse > 0) {
    afficherSymbole(2);
  } else {
    afficherSymbole(3);
  }

  if (_clicDetecte) {
    _vitesseConstante = vitesse;
    changerEtat(ConvState::CONSTANT);
  }
}

void Convoyeur::etatConstant() {
  if (_appuiLongDetecte) {
    changerEtat(ConvState::INACTIF);
    return;
  }

  appliquerVitesseMoteur(_vitesseConstante);

  if (_vitesseConstante > 0) {
    afficherSymbole(2);
  } else {
    afficherSymbole(3);
  }

  if (_clicDetecte) {
    appliquerVitesseMoteur(0);
    changerEtat(ConvState::INACTIF);
  }
}

void Convoyeur::onClick(void *context) {
  Convoyeur *convoyeur = static_cast<Convoyeur *>(context);
  convoyeur->_clicDetecte = true;
}

void Convoyeur::onLongPress(void *context) {
  Convoyeur *convoyeur = static_cast<Convoyeur *>(context);
  convoyeur->_appuiLongDetecte = true;
}
