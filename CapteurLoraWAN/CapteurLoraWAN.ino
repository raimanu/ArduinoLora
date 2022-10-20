#include <MKRWAN.h>
#include <Wire.h>
#include <ArduinoLowPower.h>
#include <RTClib.h>
#include <SD.h>
//#include <SPI.h>

#include "Key.h"
#define addressEC 100   // adresse I2C EC Conductivite
#define addressPh 99    // adresse I2C Ph

LoRaModem modem;

// Declaration paramètres LoraWan
String messageLora;

// Declaration capteurs
const int capteurTemperatureEau = A0;   // temperature de l'eau : A0
const int capteurTemperatureAir = A1;   // temperature de l'air TMP36 : A1
const int capteurLuminosite = A2;       // luminosité : A2

// Capteur ULtrasons broche Trigg et Echo
const byte TRIGGER_PIN = 2;
const byte ECHO_PIN = 3;

// Variables utilisées pour la carte SD
const int   sdCardPinChipSelect = 6;
File DataLog;
File ErrorLog;

// Variables utlisées pour le RTC
RTC_PCF8523 rtc;
String date;
String heure;

byte codeError = 0;       // code erreur sondes Atlas
char reponse[48];    // réponse des sondes Atlas (48 octets)

void setup() {
  Serial.begin(9600);

  /* Initialise Carte SD*/
  SD.begin(sdCardPinChipSelect);
  if (!SD.exists("dataLog.csv")) {
    DataLog = SD.open("dataLog.csv", FILE_WRITE);
    if (DataLog) {
      DataLog.println("Date;Heure;TempEau;TempAir;Lum;Ec;Ph;Hauteur");
      DataLog.close();     
    }
  }

  /* Initialise RTC */
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }

  if (! rtc.initialized() || rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
   }
  rtc.start();

  /* Initialise les broches */
  pinMode(TRIGGER_PIN, OUTPUT);
  digitalWrite(TRIGGER_PIN, LOW);
  pinMode(ECHO_PIN, INPUT);

  //------- Connection au réseau LoraWan UNC
  
  if (!modem.begin(EU868)) {
    Serial.println("Failed to start module");
    while (1) {}
  }

  Serial.print("La version de votre module est : ");
  Serial.println(modem.version());
  if (modem.version() != ARDUINO_FW_VERSION) {
    Serial.println("Vérifié que la dernière version du firmware est installé.");
    Serial.println("Pour mettre à jour le firmwaire installé 'MKRWANFWUpdate_standalone.ino' sketch.");
  }
  Serial.print("Votre device EUI est : ");
  Serial.println(modem.deviceEUI());

  int connected = modem.joinABP(devAddr, nwkSKey, appSKey);
  if (!connected) {
    Serial.println("Quelque chose ne vas pas ; êtes vous à l'intérieur ? Allez prés d'une fenêtre et réessayer");
    while (1) {}
  }
  modem.minPollInterval(60);

  //Initialisation EC Cond  
   Wire.begin();                 // Initialisation du port I2C

}

String requestSondeAtlas(int addrSonde) {
  Wire.beginTransmission(addrSonde);  // activation du composant d'adresse "address" avant transmission (protocole I2C)
  Wire.write("r");                  // demande d'une mesure unitaire sur la sonde Atlas Scientific 
  Wire.endTransmission();
  delay(600);
  Wire.requestFrom(addrSonde,48,1);   // demande de récupération de la mesure stockée dans la sonde
  codeError = Wire.read();
  byte in_char = 0;
  byte i = 0;
  while (Wire.available()) {        // tant que de la donnée est disponible en entrée sur le port I2C
    in_char = Wire.read();
    reponse[i] = in_char;
    i += 1;
    if (in_char == 0) {              // si une commande nulle est envoyé : fin de la réponse
      i = 0;
      break;
    }
  }
  return String(reponse).substring(0,4);
}

void errorLogger(String error){
  ErrorLog = SD.open("Log.txt", FILE_WRITE);
  if (ErrorLog) {
    DateTime now = rtc.now();
    date = String(String(now.day()) + '/' + String(now.month()) + '/' + String(now.year()));
    heure = String(now.hour()) + ':' + String(now.minute())+ ':' + String(now.second());
    ErrorLog.print(date);
    ErrorLog.print(" ");
    ErrorLog.print(heure);
    ErrorLog.print("-->");
    ErrorLog.println(error);
    ErrorLog.close();
  }
}

void sleepEC(){
  Wire.beginTransmission(addressEC);
  Wire.write("SLEEP");
  Wire.endTransmission();
}

void sleepPH(){
  Wire.beginTransmission(addressPh);
  Wire.write("SLEEP");
  Wire.endTransmission();
}

void loop() {

  modem.setPort(3);
  modem.beginPacket();

  //------ Mesure temperature Eau (sonde Atlas)------//
  float temperatureEau = analogRead(capteurTemperatureEau); // Valeur en sortie du convertisseur A/N
  temperatureEau = 3.3/1023*temperatureEau*1000;      // Tension en mV à l'entrée du convertisseur
  temperatureEau = (0.0512*temperatureEau)-20.5128;         // Formule de calcul issue de la documentation technique
  modem.print(String(temperatureEau) + "/");

  //------ Mesure temperature Air TMP36 ------//
  float temperatureAir = analogRead(capteurTemperatureAir); // Valeur brute en sortie du convertisseur A/N
  temperatureAir = 3.3/1023*temperatureAir;           // Tension en V à l'entrée du convertisseur (règle de 3)
  temperatureAir =((temperatureAir*1000)-500)/10;     // Formule de calcul issue de la documentation technique
  modem.print(String(temperatureAir) + "/");

  int luminosite = analogRead(capteurLuminosite);    // Valeur en sortie du convertisseur A/N
  modem.print(String(luminosite) + "/");

  //------ Commande carte Atlas EC ------//

  String ec = requestSondeAtlas(addressEC);
  modem.print(ec + "/");

  //------ Commande carte Atlas Ph ------//

  String ph = requestSondeAtlas(addressPh);
  modem.print(ph + "/");

  //------ Mesure distance capteur ultrasons -------//

  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);
  long measure = pulseIn(ECHO_PIN, HIGH, 25000UL);
  float distance_cm = (measure * 0.34 / 2.0)/ 10.0;
  modem.print(String(distance_cm));

  //------ Transmission LoRaWAN ------//
 
  int err = modem.endPacket(true);
  if (err > 0) {
    Serial.println("Message envoyé correctement!");
    errorLogger("Succes: envoi trame LoRa");
  } else {
    Serial.println("Erreur envoi message");
    errorLogger("Error: envoi trame LoRa");
  }

  //------ DataLogger ------//

  DataLog = SD.open("dataLog.csv", FILE_WRITE);
  if (DataLog) {
    Serial.println("Enregistrement Carte SD");
    DateTime now = rtc.now();
    date = String(String(now.day()) + '/' + String(now.month()) + '/' + String(now.year()));
    heure = String(now.hour()) + ':' + String(now.minute())+ ':' + String(now.second());
    DataLog.print(date);
    DataLog.print(";");
    DataLog.print(heure);
    DataLog.print(";");
    DataLog.print(temperatureEau);
    DataLog.print(";");
    DataLog.print(temperatureAir);
    DataLog.print(";");
    DataLog.print(luminosite);
    DataLog.print(";");
    DataLog.print(ec);
    DataLog.print(";");
    DataLog.print(ph);
    DataLog.print(";");
    DataLog.println(distance_cm);
    DataLog.close();     
  }

  //------ Mise en Veille ------//

  modem.sleep();
  sleepEC();
  sleepPH();
  //LowPower.deepSleep(2000 * 60);
  //setup();
  delay(2000*60);
}

