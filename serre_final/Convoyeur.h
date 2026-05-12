#pragma once

#include <Arduino.h>
#include <OneButton.h>
#include <U8g2lib.h>

enum class ConvState {
  INACTIF,
  ACTIF,
  MANUEL,
  CONSTANT
};

class Convoyeur {
public:
  Convoyeur(
    byte moteurPin1,
    byte moteurPin2,
    byte manettePin,
    byte boutonPin,
    byte afficheurCLK,
    byte afficheurDIN,
    byte afficheurCS
  );

  void begin();
  void update();

  bool estEnFonction() const;
  int getVitesse() const;
  int getVitesseMoteur() const;
  ConvState getState() const;
  void setNumeroEtudiantPair(bool estPair);
  bool setVitesseConsigne(int vitesse);

private:
  byte _moteurPin1;
  byte _moteurPin2;
  byte _manettePin;

  OneButton _bouton;
  U8G2_MAX7219_8X8_F_4W_SW_SPI _afficheur;

  ConvState _etat = ConvState::INACTIF;

  bool _clicDetecte = false;
  bool _appuiLongDetecte = false;
  bool _numeroEtudiantPair = true;

  int _vitesseActuelle = 0;
  int _vitesseConstante = 0;
  int _dernierSymbole = -1;

  const int _zoneMorte = 5;

  void changerEtat(ConvState nouvelEtat);

  void etatInactif();
  void etatActif();
  void etatManuel();
  void etatConstant();

  int lireVitesseManette();
  void appliquerVitesseMoteur(int vitesse);
  void afficherSymbole(byte symbole);

  static void onClick(void *context);
  static void onLongPress(void *context);

  const uint8_t bitmapStop[8] = {
    B10000001,
    B01000010,
    B00100100,
    B00011000,
    B00011000,
    B00100100,
    B01000010,
    B10000001
  };

  const uint8_t bitmapAttentePair[8] = {
    B01100110,
    B01100110,
    B01100110,
    B01100110,
    B01100110,
    B00000000,
    B01100110,
    B01100110
  };

  const uint8_t bitmapAttenteImpair[8] = {
    B00000000,
    B00011000,
    B00011000,
    B00011000,
    B00011000,
    B00000000,
    B00011000,
    B00000000
  };

  const uint8_t bitmapAvancePair[8] = {
    B00011000,
    B00111100,
    B01111110,
    B11111111,
    B00011000,
    B00011000,
    B00011000,
    B00000000
  };

  const uint8_t bitmapReculePair[8] = {
    B00011000,
    B00011000,
    B00011000,
    B11111111,
    B01111110,
    B00111100,
    B00011000,
    B00000000
  };

  const uint8_t bitmapAvanceImpair[8] = {
    B00011000,
    B00111100,
    B01111110,
    B11111111,
    B00011000,
    B00011000,
    B00011000,
    B00000000
  };

  const uint8_t bitmapReculeImpair[8] = {
    B00011000,
    B00011000,
    B00011000,
    B11111111,
    B01111110,
    B00111100,
    B00011000,
    B00000000
  };
};
