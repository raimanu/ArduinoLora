/*
  First Configuration
  This sketch demonstrates the usage of MKR WAN 1300/1310 LoRa module.
  This example code is in the public domain.
*/

#include <MKRWAN.h>
#include <Wire.h>
#include <ArduinoLowPower.h>
LoRaModem modem; 
#define address 100   // adresse I2C EC Cond

/// Declaration paramètres LoraWan
String appEui;
String appKey;
String devAddr;
String nwkSKey;
String appSKey;
String messageLora;

/// Declaration capteurs
int capteurTemperatureEau = A0;   // capteur de temperature de l'eau sur l'entrée analogique A0
int capteurTemperatureAir = A1;   // capteur de temperature de l'air TMP36  sur l'entrée analogique A1
int capteurLuminosite = A2;       // capteur de temperature de la luminosité sur l'entrée analogique A2

float temperatureEau;             // Lecture de la mesure de la température de l'eau
float temperatureAir;             // Lecture de la mesure de la température de l'air
float luminosite;

byte code = 0;       // Sonde EC : 1 octet code d'erreur
char reponse[48];    // Réponse de la sonde EC (réservation de 48 octets en mémoire)
byte in_char = 0;    // La lecture de la réponse de la sonde EC se fait caractère par caractère  
byte i = 0;          // variable positionnelle (reponse)

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  //!\ Attention --> while (!Serial);

  //------- Connection au réseau LoraWan UNC
  
  if (!modem.begin(EU868)) {
    Serial.println("Failed to start module");
    while (1) {}
  };
  Serial.print("Your module version is: ");
  Serial.println(modem.version());
  if (modem.version() != ARDUINO_FW_VERSION) {
    Serial.println("Please make sure that the latest modem firmware is installed.");
    Serial.println("To update the firmware upload the 'MKRWANFWUpdate_standalone.ino' sketch.");
  }
  Serial.print("Your device EUI is: ");
  Serial.println(modem.deviceEUI());

  int connected;
  devAddr = "00000100";
  nwkSKey = "c9e170f1beae7d5913ea5ba5d3852643";
  appSKey = "2204f90ae7d7a3728d12cc629897a546";
  
  devAddr.trim();
  nwkSKey.trim();
  appSKey.trim();

  connected = modem.joinABP(devAddr, nwkSKey, appSKey);
  if (!connected) {
    Serial.println("Something went wrong; are you indoor? Move near a window and retry");
    while (1) {}
  }

  // Set poll interval to 60 secs.
  modem.minPollInterval(60);
  // NOTE: independent of this setting, the modem will
  // not allow sending more than one message every 2 minutes,
  // this is enforced by firmware and can not be changed.

  //Initialisation EC Cond  
   Wire.begin();                 // Initialisation du port I2C
}

void loop() {

  /////////// Mesure temperature Eau (sonde Atlas) //////////
  temperatureEau = analogRead(capteurTemperatureEau); // Valeur en sortie du convertisseur A/N
  temperatureEau = 3.3/1023*temperatureEau*1000;      // Tension en mV à l'entrée du convertisseur
  temperatureEau = (0.0512*temperatureEau)-20.5128;         // Formule de calcul issue de la documentation technique
  messageLora = "TE:" + String(temperatureEau) + "/";

  /////////// Mesure temperature Air TMP36 //////////
  temperatureAir = analogRead(capteurTemperatureAir); // Valeur brute en sortie du convertisseur A/N
  temperatureAir = 3.3/1023*temperatureAir;           // Tension en V à l'entrée du convertisseur (règle de 3)
  temperatureAir =((temperatureAir*1000)-500)/10;     // Formule de calcul issue de la documentation technique
  messageLora += "TA:" + String(temperatureAir) + "/";

  luminosite = analogRead(capteurLuminosite);    // Valeur en sortie du convertisseur A/N
  messageLora += "L:" + String(luminosite) + "/";

  /////////// Commande carte Atlas EC Conductivité //////////
  Wire.beginTransmission(address);  // activation du composant d'adresse "address" avant transmission (protocole I2C)
  Wire.write("r");                  // demande d'une mesure unitaire sur la sonde EC (Atlas Scientific) 
      
  Wire.requestFrom(address,48,1);   // demande de récupération de la mesure stockée dans la sonde I2C
  code = Wire.read();               // lecture du code d'erreur (1er octet)
  while (Wire.available()) {        // tant que de la donnée est disponible en entrée sur le port I2C
   in_char = Wire.read();           // lecture, un part un, des octets suivant (resultat de la mesure)
   reponse[i] = in_char;            // remplissage du tableau (reponse) 
   i += 1;                          // incrémentation i+1 (position suivante)
   if (in_char == 0) {              // si une commande nulle est envoyé : fin de la réponse
     i = 0;                         // remet le compteur i à 0
     Wire.endTransmission();        // fin de la transmission I2C
     break;                         // sortir de la boucle 
   }
  }
  //messageLora += "EC:" + String(reponse) + "/";
  messageLora += "EC:" + String(reponse).substring(0,5) + "/";
  Serial.println("Taille message : " + messageLora.length());
  Serial.println(messageLora);

  int err;
  modem.setPort(3);
  modem.beginPacket();
  modem.print(messageLora);
  err = modem.endPacket(true);
  if (err > 0) {
    Serial.println("Message sent correctly!");
  } else {
    Serial.println("Error sending message :(");
    Serial.println("(you may send a limited amount of messages per minute, depending on the signal strength");
    Serial.println("it may vary from 1 message every couple of seconds to 1 message every minute)");
  }
  LowPower.deepSleep(2000 * 60);
}
