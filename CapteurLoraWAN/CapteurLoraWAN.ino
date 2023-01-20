#include "Extension.h"
#include "Key.h"
#include <SPI.h>

#define addressEC 100   // adresse I2C EC Conductivite
#define addressPh 99    // adresse I2C Ph

LoRaModem modem;

// Declaration paramètres LoraWan
String messageLora;

// Declaration capteurs
const int capteurTemperatureEau = A0;   // temperature de l'eau : A0
const int capteurTemperatureAir = A1;   // temperature de l'air TMP36 : A1
const int capteurLuminosite = A2;       // luminosité : A2

// Variables capteur ultrasons
unsigned char data[4]={};
float distance;

// Variables utilisées pour la carte SD
const int sdCardPinChipSelect = 4;
File DataLog;
File ErrorLog;

// Variables utlisées pour le RTC
RTC_PCF8523 rtc;

byte codeError = 0;       // code erreur sondes Atlas
char reponse[48];    // réponse des sondes Atlas (48 octets)

void setup() {
  Serial.begin(57600); //DEBUG
  
  Serial1.begin(9600); 

  /* Initialise Carte SD*/
  SD.begin(sdCardPinChipSelect);
  if (!SD.exists("dataLog.csv")) {
    DataLog = SD.open("dataLog.csv", FILE_WRITE);
    if (DataLog) {
      DataLog.println("Date;Heure;TempEau;TempAir;Lum;Ec;Salinité;Ph;Hauteur");
      DataLog.close();     
    }
  }

  /* Initialise RTC */
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC"); //DEBUG
    Serial.flush();
    errorLogger("Couldn't find RTC");
    //while (1) delay(10);
  }

  if (! rtc.initialized() || rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
   }
  rtc.start();

  //------- Connection au réseau LoraWan UNC
  if (!modem.begin(EU868)) {
    errorLogger("Failed to start module");
    while (1) {}
  }

  errorLogger(String("Setup : version du module : " + modem.version()));
  errorLogger(String("Setup : device EUI : " + modem.deviceEUI()));

  devAddr.trim();
  nwkSKey.trim();
  appSKey.trim();
  int connected = modem.joinABP(devAddr, nwkSKey, appSKey);
  if (!connected) {
    Serial.println("Erreur de connexion, réessayer"); //DEBUG
    errorLogger("Erreur de connexion, réesayer");
    while (1) {}
  }
  modem.minPollInterval(60);

  //Initialisation EC Cond  
  Wire.begin();                 // Initialisation du port I2C
}

char* requestSondeAtlas(int addrSonde, int dataType) {
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
  if (addrSonde == addressEC) {
  char * conductivity = strtok(reponse, ",");
  char * salinity = strtok(NULL, ",");
  switch (dataType) {
    case 1 :
      return conductivity;
      break;
    case 2 :
      return salinity;
      break;
    }
  }
  if (addrSonde == addressPh) return reponse;
}

void errorLogger(String error){
  ErrorLog = SD.open("Log.txt", FILE_WRITE);
  if (ErrorLog) {
    DateTime now = rtc.now();
    ErrorLog.print(String(String(now.day()) + '/' + String(now.month()) + '/' + String(now.year()) + ' '));
    ErrorLog.print(String(String(now.hour()) + ':' + String(now.minute())+ ':' + String(now.second())));
    ErrorLog.print("-->");
    ErrorLog.println(error);
    ErrorLog.close();
  }
}

void sleepAtlas(int addrSonde){
  Wire.beginTransmission(addrSonde);
  Wire.write("SLEEP");
  Wire.endTransmission();
}

void loop() {
  modem.setPort(3);
  modem.beginPacket();
  tabData dataTab;
  dataTab.prot = 0x01;

  //------ Mesure temperature Eau (sonde Atlas)------//
  float temperatureEau = analogRead(capteurTemperatureEau); // Valeur en sortie du convertisseur A/N
  temperatureEau = 3.3/1023*temperatureEau*1000;      // Tension en mV à l'entrée du convertisseur
  temperatureEau = (0.0512*temperatureEau)-20.5128;         // Formule de calcul issue de la documentation technique
  dataTab.tempEau = temperatureEau*100;
  Serial.println(temperatureEau); //DEBUG

  //------ Mesure temperature Air TMP36 ------//
  float temperatureAir = analogRead(capteurTemperatureAir); // Valeur brute en sortie du convertisseur A/N
  temperatureAir = 3.3/1023*temperatureAir;           // Tension en V à l'entrée du convertisseur (règle de 3)
  temperatureAir =((temperatureAir*1000)-500)/10;     // Formule de calcul issue de la documentation technique
  dataTab.tempAir = temperatureAir*100;
  Serial.println(temperatureAir); //DEBUG

  int luminosite = analogRead(capteurLuminosite);    // Valeur en sortie du convertisseur A/N
  dataTab.Lum = luminosite;
  Serial.println(luminosite); //DEBUG

  //------ Commande carte Atlas EC / Salinity ------//
  String ec = requestSondeAtlas(addressEC, 1);
  String sal = requestSondeAtlas(addressEC, 2);
  dataTab.Ec = ec.toInt();
  dataTab.Sel = sal.toInt();
  Serial.println(ec); //DEBUG
  Serial.println(sal); //DEBUG

  //------ Commande carte Atlas Ph ------//
  String ph = requestSondeAtlas(addressPh, 0);
  dataTab.pH = ph.toInt();
  Serial.println(ph); //DEBUG

  //------ Mesure distance capteur ultrasons A01NYUB-------//
  distance = 0;
  while (distance == 0){
      do {
        for(int i=0;i<4;i++)
        {
      data[i]=Serial1.read();
      }
  } while(Serial1.read()==0xff);

  Serial1.flush();

  //Serial.print("data[0] ="); //DEBUG
  //Serial.println(data[0]); //DEBUG

  if(data[0]==0xff) {
      int sum;
      sum=(data[0]+data[1]+data[2])&0x00FF;
      if(sum==data[3]) {
        distance=(data[1]<<8)+data[2];
      }
     }
      delay(150);
     }
  dataTab.hauteur = distance;
  Serial.println("distance = " + String(distance/10)); //DEBUG

  //------ Transmission LoRaWAN ------//
  modem.write(dataTab);
  int err = modem.endPacket(true);
  if (err > 0) {
    Serial.println("Message envoyé correctement!");
    errorLogger("Success: envoi trame LoRa");
  } else {
    Serial.println("Erreur envoi message");
    errorLogger("Error: envoi trame LoRa");
  }

  //------ DataLogger ------//
  DataLog = SD.open("dataLog.csv", FILE_WRITE);
  if (DataLog) {
    Serial.println("Enregistrement Carte SD");
    DateTime now = rtc.now();
    DataLog.print(String(String(now.day()) + '/' + String(now.month()) + '/' + String(now.year()) + ';'));
    DataLog.print(String(String(now.hour()) + ':' + String(now.minute())+ ':' + String(now.second()) + ';'));
    String allData[7] = {String(temperatureEau),String(temperatureAir),String(luminosite),ec,sal,ph,String(distance)}; 
    for (int i = 0; i < 7; i++) {
      DataLog.print(allData[i]);
      DataLog.print(";");
    }
    DataLog.println();
    DataLog.close();     
  }

  //------ Mise en Veille ------//
  modem.sleep();
  sleepAtlas(addressEC);
  sleepAtlas(addressPh);
  //LowPower.deepSleep(2000 * 60);
  //setup();
  delay(2000*60);
}
