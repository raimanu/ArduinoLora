#include <Wire.h>
#include <MKRWAN.h>

#define address 100   // adresse I2C EC Conductivite

char computerdata[32];           //we make a 32 byte character array to hold incoming data from a pc/mac/other.
byte received_from_computer = 0; //we need to know how many characters have been received.
bool serial_event = true;           //a flag to signal when data has been received from the pc/mac/other.
byte code = 0;                   //used to hold the I2C response code.
char ec_data[32];                //we make a 32 byte character array to hold incoming data from the EC circuit.
byte in_char = 0;                //used as a 1 byte buffer to store inbound bytes from the EC Circuit.
byte i = 0;                      //counter used for ec_data array.
int time_ = 570;                 //used to change the delay needed depending on the command sent to the EZO Class EC Circuit.

char *ec;                        //char pointer used in string parsing.
char *tds;                       //char pointer used in string parsing.
char *sal;                       //char pointer used in string parsing.
char *sg;                        //char pointer used in string parsing.

float ec_float;                  //float var used to hold the float value of the conductivity.
float tds_float;                 //float var used to hold the float value of the TDS.
float sal_float;                 //float var used to hold the float value of the salinity.
float sg_float;                  //float var used to hold the float value of the specific gravity.


void setup()                     //hardware initialization.
{
  Serial.begin(9600);            //enable serial port.
  Wire.begin();                  //enable I2C port.
  delay(2000);
  Serial.println("Start");
}


void loop() {
  if (Serial.available()) {     
    Serial.println("Command Send");  
    received_from_computer = Serial.readBytesUntil(13, computerdata, 32);
    computerdata[received_from_computer-1] = 0;
    for (i = 0; i <= received_from_computer; i++) {     
      computerdata[i] = tolower(computerdata[i]);
    }
    Serial.println(computerdata);
    i=0;                                                                          //reset i, we will need it later 


    Wire.beginTransmission(address);                                            //call the circuit by its ID number.
    Wire.write(computerdata);                                                   //transmit the command that was sent through the serial port.
    Wire.endTransmission();                                                     //end the I2C data transmission.

    if (strcmp(computerdata, "sleep") != 0) {                                   //if the command that has been sent is NOT the sleep command, wait the correct amount of time and request data.

      delay(1000);
      

      Wire.requestFrom(address, 48, 11);                                         //call the circuit and request 32 bytes (this could be too small, but it is the max i2c buffer size for an Arduino)
      code = Wire.read();                                                       //the first byte is the response code, we read this separately.

      switch (code) {                           //switch case based on what the response code is.
        case 1:                                 //decimal 1.
          Serial.println("Success");            //means the command was successful.
          break;                                //exits the switch case.

        case 2:                                 //decimal 2.
          Serial.println("Failed");             //means the command has failed.
          break;                                //exits the switch case.

        case 254:                               //decimal 254.
          Serial.println("Pending");            //means the command has not yet been finished calculating.
          break;                                //exits the switch case.

        case 255:                               //decimal 255.
          Serial.println("No Data");            //means there is no further data to send.
          break;                                //exits the switch case.
      }

      while (Wire.available()) {                 //are there bytes to receive.
        in_char = Wire.read();                   //receive a byte.
        ec_data[i] = in_char;                    //load this byte into our array.
        i += 1;                                  //incur the counter for the array element.
        if (in_char == 0) {                      //if we see that we have been sent a null command.
          i = 0;                                 //reset the counter i to 0.
          break;                                 //exit the while loop.
        }
      }

      Serial.println(ec_data);                  //print the data.
      Serial.println();                         //this just makes the output easier to read by adding an extra blank line 
    }

    if (computerdata[0] == 'r') string_pars(); //uncomment this function if you would like to break up the comma separated string into its individual parts.
  }

}
void string_pars() {                  //this function will break up the CSV string into its 4 individual parts. EC|TDS|SAL|SG.

  ec = strtok(ec_data, ",");          
  tds = strtok(NULL, ",");
  sal = strtok(NULL, ",");
  sg = strtok(NULL, ",");

  Serial.print("EC:");
  Serial.println(ec);

  Serial.print("TDS:");
  Serial.println(tds);

  Serial.print("SAL:");
  Serial.println(sal);

  Serial.print("SG:");
  Serial.println(sg);
  Serial.println();
     
}