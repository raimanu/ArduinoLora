#include <MKRWAN.h>
#include <Wire.h>
#include <ArduinoLowPower.h>
#include "Key.h"
#define address 100   // adresse I2C EC Conductivite

LoRaModem modem;

/// Declaration paramètres LoraWan
String messageLora;

/// Declaration capteurs
int capteurTemperatureEau = A0;   // capteur de temperature de l'eau sur le pin A0
int capteurTemperatureAir = A1;   // capteur de temperature de l'air TMP36  sur le pin A1
int capteurLuminosite = A2;       // capteur de temperature de la luminosité sur le pin A2

float temperatureEau;             // Variable température de l'eau
float temperatureAir;             // Variable température de l'air
float luminosite;

byte code = 0;       // code erreur Sonde EC
char reponse[48];    // Réponse de la sonde EC (réservation de 48 octets en mémoire)
byte in_char = 0;
byte i = 0;

void setup() {
  Serial.begin(9600);

  //------- Connection au réseau LoraWan UNC
  
  if (!modem.begin(EU868)) {
    Serial.println("Failed to start module");
    while (1) {}
  };
  Serial.print("La version de votre module est : ");
  Serial.println(modem.version());
  if (modem.version() != ARDUINO_FW_VERSION) {
    Serial.println("Vérifié que la dernière version du firmware est installé.");
    Serial.println("Pour mettre à jour le firmwaire installé 'MKRWANFWUpdate_standalone.ino' sketch.");
  }
  Serial.print("Votre device EUI est : ");
  Serial.println(modem.deviceEUI());

  devAddr.trim();
  nwkSKey.trim();
  appSKey.trim();

  int connected = modem.joinABP(devAddr, nwkSKey, appSKey);
  if (!connected) {
    Serial.println("Quelque chose ne vas pas ; êtes vous à l'intérieur ? Allez prés d'une fenêtre et réessayer");
    while (1) {}
  }

  // Regler intervalle à 60 sec
  modem.minPollInterval(60);
  // NOTE: independent de ce parametre, le modem n'envoie
  // pas plus de deux messages toutes les 2 minutes,
  // c'est restreint par le firmware et ne peux être changé.

  //Initialisation EC Cond  
   Wire.begin();                 // Initialisation du port I2C
}

void loop() {

  //------ Mesure temperature Eau (sonde Atlas)------//
  temperatureEau = analogRead(capteurTemperatureEau); // Valeur en sortie du convertisseur A/N
  temperatureEau = 3.3/1023*temperatureEau*1000;      // Tension en mV à l'entrée du convertisseur
  temperatureEau = (0.0512*temperatureEau)-20.5128;         // Formule de calcul issue de la documentation technique
  messageLora = "TE:" + String(temperatureEau) + "/";

  //------ Mesure temperature Air TMP36 ------//
  temperatureAir = analogRead(capteurTemperatureAir); // Valeur brute en sortie du convertisseur A/N
  temperatureAir = 3.3/1023*temperatureAir;           // Tension en V à l'entrée du convertisseur (règle de 3)
  temperatureAir =((temperatureAir*1000)-500)/10;     // Formule de calcul issue de la documentation technique
  messageLora += "TA:" + String(temperatureAir) + "/";

  luminosite = analogRead(capteurLuminosite);    // Valeur en sortie du convertisseur A/N
  messageLora += "L:" + String(luminosite) + "/";

  //------ Commande carte Atlas EC Conductivité ------//
  Wire.beginTransmission(address);  // activation du composant d'adresse "address" avant transmission (protocole I2C)
  Wire.write("r");                  // demande d'une mesure unitaire sur la sonde EC (Atlas Scientific) 
  Wire.endTransmission();
  delay(600);
  Wire.requestFrom(address,48,1);   // demande de récupération de la mesure stockée dans la sonde
  code = Wire.read();               // lecture du code d'erreur
  while (Wire.available()) {        // tant que de la donnée est disponible en entrée sur le port I2C
    in_char = Wire.read();
    reponse[i] = in_char;
    i += 1;
    if (in_char == 0) {              // si une commande nulle est envoyé : fin de la réponse
      i = 0;
      break;
    }
  }
  
  messageLora += "EC:" + String(reponse).substring(0,5) + "/";
  Serial.println("Taille message : " + messageLora.length());
  Serial.println(messageLora);

  modem.setPort(3);
  modem.beginPacket();
  modem.print(messageLora);
  int err = modem.endPacket(true);
  if (err > 0) {
    Serial.println("Message envoyé correctement!");
  } else {
    Serial.println("Erreur envoi message");
  }
  LowPower.deepSleep(2000 * 60);
  setup();
}
