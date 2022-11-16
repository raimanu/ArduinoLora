#include <MKRWAN.h>

struct tabData {
    byte prot;
    int16_t tempAir;
    int16_t tempEau;
    int16_t Ec;
    int16_t pH;
    int16_t Lum;
    int16_t hauteur;
};

/* Données fictives à enregistrer dans la structure */
tabData encryptData(tabData data){
    data.prot = 0x01;
    data.tempAir = 26.5*10.0;
    data.tempEau = 21.6*10.0;
    data.Ec = 6000;
    data.pH = 7.8*10.0;
    data.Lum = 79.2*10.0;
    data.hauteur = 256;
    return data;
}

/* Affiche les données de la structure dans le terminal */
void decryptData(tabData data){
    Serial.println(data.prot, HEX);
    Serial.println(data.tempAir/10.0, DEC);
    Serial.println(data.tempEau/10.0, DEC);
    Serial.println(data.Ec, DEC);
    Serial.println(data.pH/10.0, DEC);
    Serial.println(data.Lum/10.0, DEC);
    Serial.println(data.hauteur, DEC);
}

LoRaModem modem;
String devAddr = "00000100";
String nwkSKey = "c9e170f1beae7d5913ea5ba5d3852643";
String appSKey = "2204f90ae7d7a3728d12cc629897a546";

void setup(){
    Serial.begin(9600);
    if (!modem.begin(EU868)) {
        Serial.println("Failed to start module");
        while (1) {}
    }

    Serial.println(String("Setup : version du module : " + modem.version()));
    Serial.println(String("Setup : device EUI : " + modem.deviceEUI()));

    devAddr.trim();
    nwkSKey.trim();
    appSKey.trim();
    int connected = modem.joinABP(devAddr, nwkSKey, appSKey);
    if (!connected) {
        Serial.println("Erreur de connexion, réesayer");
        while (1) {}
    }
    modem.minPollInterval(60);
    tabData data;
    delay(2000);
    
    data = encryptData(data);
    decryptData(data);

    modem.setPort(3);
    modem.beginPacket();
    modem.write(data);
    modem.endPacket(true);
}

void loop(){

}

