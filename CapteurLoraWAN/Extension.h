#include <MKRWAN.h>
#include <Wire.h>
#include <ArduinoLowPower.h>
#include <RTClib.h>
#include <SD.h>


struct tabData {
    byte prot;
    int16_t tempAir;
    int16_t tempEau;
    int16_t Ec;
    int16_t Sel;
    int16_t pH;
    int16_t Lum;
    int16_t hauteur;
};