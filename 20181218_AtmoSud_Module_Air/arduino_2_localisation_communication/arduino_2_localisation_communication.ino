#include "ArduinoJson.h";

#include <stdarg.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include <Wire.h> //I2C
#include <SPI.h>

int8_t answer;
int onModulePin = 2;
int counter;
long previous;

char dataF[5];
char data[256]; // De base "char" .Doit etre strictement superieur a  la valeur recue (ici 140).
int data_size;  // Taille de la chaîne

char aux_str[150]; //Envoi
char aux_str2[150]; //Recup
char aux;
int x = 0;

int essai = 0; // Variable test pour reboot
int tour = 0;
int VerGPS = 0; // Compteur de 15min (en millisecondes) pour GPS Setup
int errgsm = 0; //Compteur d'erreur GSM
int power = 0;

char pin[] = "****"; // A renseigner si présent sinon laisser vide
char apn[] = "****"; // A renseigner en fonction de votre opérateur
char user_name[] = "****"; // A renseigner en fonction de votre opérateur
char password[] = "****"; // A renseigner en fonction de votre opérateur

char envoi[14];

unsigned long TryGPS;

//Variables GPS et parametres URL
char Basic_str[100];
char RMC_str[100];

char longitude[15];
char latitude[15];
char verifbasic[15];
char testlong[15] = "0.000000";

char altitude[6];
char horaire[16];
char polluant[10] = "NO2";
int echeance = 0;
char GPST[3]; // Variable verif si GPS actif

char url[100] = "http://apigeoloc.herokuapp.com/getpollution?"; // URL recup et ses parametres (paraURL)
char paraURL[100];
char url2[100] = "http://...................:9876/import?"; // URL (a completer) pour envoi sur API d'enregistrement des données et ses parametres (paraURL2). API a installer sur serveur a partir du code Github suivant (https://github.com/jnth/realtime-data-with-map/blob/master/INSTALL.md) 
char paraURL2[100];

char PARTM10[8] = "0";
char PARTM25[8] = "0";
char Co2[8] = "0";
char NO2[5] = "ND";

//Variables traitement donnees GPS
String string_return_dmd2dd;
char return_dmd2dd[11];
char response[100];

void setup()
{
  pinMode(onModulePin, OUTPUT);
  Serial.begin(115200);
  Serial.println("Starting...");
  power_on();
  delay(3000);

  Wire.begin(8); // join i2c bus (address optional for master)
  Wire.onRequest(ReqEcran);

  //sets the PIN code
  snprintf(aux_str, sizeof(aux_str), "AT+CPIN=%s", pin);
  sendATcommand(aux_str, "OK", 2000);
  delay(3000);

  while (sendATcommand2("AT+CREG?", "+CREG: 0,1", "+CREG: 0,5", 2000) == 0, essai++);
  if (essai == 10)
  {
    software_Reset();
  }

  // sets APN , user name and password
  sendATcommand("AT+SAPBR=3,1,\"Contype\",\"GPRS\"", "OK", 2000);
  snprintf(aux_str, sizeof(aux_str), "AT+SAPBR=3,1,\"APN\",\"%s\"", apn);
  sendATcommand(aux_str, "OK", 2000);

  snprintf(aux_str, sizeof(aux_str), "AT+SAPBR=3,1,\"USER\",\"%s\"", user_name);
  sendATcommand(aux_str, "OK", 2000);

  snprintf(aux_str, sizeof(aux_str), "AT+SAPBR=3,1,\"PWD\",\"%s\"", password);
  sendATcommand(aux_str, "OK", 2000);
  while (sendATcommand("AT+SAPBR=1,1", "OK", 20000) == 0, tour++);

  sendATcommand("AT+CGPSPWR=1", "OK", 2000);
  sendATcommand("AT+CGPSRST=0", "OK", 2000);

  TryGPS = millis();
  // waits for fix GPS
  while ((sendATcommand("AT+CGPSSTATUS?", "2D Fix", 2000) ||
          sendATcommand("AT+CGPSSTATUS?", "3D Fix", 2000)) == 0)
  {
    if ((millis() - TryGPS) > 90000 && (sendATcommand("AT+CGPSSTATUS?", "3D Fix", 2000)) == 0 ) // Test de fixation GPS sur 15 minutes
    {
      //Serial.println("Fix GPS OK");
      Serial.println("Temps dépassé");
      Serial.print("VerGPS :");
      Serial.println(VerGPS);
      VerGPS++;
      Serial.print("VerGPS :");
      Serial.println(VerGPS);
      strcpy (longitude, "5.374090");
      strcpy (latitude, "43.300816");
      break;
    }
  }
}

void loop()
{ //a completer avec les fonction I2C CO2 PM et GSM Envoi
  if (VerGPS == 1)
  {
    GSMRecup();
    //Co2PM();
    delay(5000);
    //GSMEnvoi();
    if (errgsm == 5)
    {
      software_Reset();
    }
  }
  else if (VerGPS == 0)
  {
    GPS();
    GSMRecup();
    //Co2PM();
    delay(5000);
    //GSMEnvoi();
    if (errgsm == 5)
    {
      software_Reset();
    }
  }
}

//************************************************************************************

void GSMRecup()
{
  // Initializes HTTP service
  answer = sendATcommand("AT+HTTPINIT", "OK", 10000);
  if (answer == 1)
  {
    // Sets CID parameter
    answer = sendATcommand("AT+HTTPPARA=\"CID\",1", "OK", 5000);
    if (answer == 1)
    {
      // Met en place l'URL
      sprintf(aux_str, "AT+HTTPPARA=\"URL\",\"%s", url);
      Serial.print(aux_str);
      sprintf(paraURL, "pol=%s&lon=%s&lat=%s&ech=p%d", polluant, longitude, latitude, echeance);
      Serial.print(paraURL);
      answer = sendATcommand("\"", "OK", 5000);
      if (answer == 1)
      {
        // Atteint l'URL
        answer = sendATcommand("AT+HTTPACTION=0", "+HTTPACTION:0,200", 30000); //10000 au debut
        if (answer == 1)
        {
          x = 0;
          do {
            sprintf(aux_str, "AT+HTTPREAD=%d,150", x);
            // Lit le contenu de l'URL
            if (sendATcommand2(aux_str, "+HTTPREAD:", "ERROR", 30000) == 1)
            {
              data_size = 0;
              while (Serial.available() == 0);
              aux = Serial.read();
              do
              {
                data_size *= 10;
                data_size += (aux - 0x30);
                while (Serial.available() == 0);
                aux = Serial.read();
              }
              while (aux != 0x0D);
              Serial.print("Data received: ");
              Serial.println(data_size);
              if (data_size > 0)
              {
                Serial.print("Valeur Serial.available():");
                Serial.println(Serial.available());
                Serial.println("Debut de telechargement du data...");
                Serial.read(); // permet de virer le retour a la ligne de println
                for (int y = 0; y < data_size; y++)
                {
                  data[x] = Serial.read();
                  Serial.print(data[x]);// obligatoire de faire un serial print par tour de boucle
                  x++;
                  //Serial.print(" "); //Si absente resultat mauvais
                }
                data[x] = '\0';
                Serial.println("Telechargement fini");
              }
            }
            else if (answer == 2)
            {
              Serial.println("Error from HTTP");
            }
            else
            {
              Serial.println("No more data available");
              data_size = 0;
            }
            sendATcommand("", "+HTTPACTION:0,200", 20000);
          }
          while (data_size > 0);
          Serial.println("Data au format JSON: ");

          const size_t bufferSize = JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(3) + 50;
          DynamicJsonBuffer jsonBuffer(bufferSize);

          const char* json = data;

          JsonObject& root = jsonBuffer.parseObject(json);

          JsonObject& data = root["data"];
          const char* data_ech = data["ech"]; // "p0"
          const char* data_pol = data["pol"]; // "NO2"
          const char* data_valeur = data["valeur"]; // "40"

          const char* status = root["status"]; // "ok"

          Serial.print("Poluant: ");
          Serial.println(data_pol);
          Serial.print("valeur du Poluant: ");
          Serial.println(data_valeur);

          strcpy(NO2, data_valeur);
        }
        else
        {
          Serial.println("Error getting the url");
          //software_Reset();
          errgsm++;
        }
      }
      else
      {
        Serial.println("Error setting the url");
        errgsm++;
      }
    }
    else
    {
      Serial.println("Error setting the CID");
      errgsm++;
    }
  }
  else
  {
    Serial.println("Error initializating");
    errgsm++;
    //software_Reset();
  }
  sendATcommand("AT+HTTPTERM", "OK", 5000);
}

//************************************************************************************

void GSMEnvoi()
{
  // Initializes HTTP service
  answer = sendATcommand("AT+HTTPINIT", "OK", 10000);
  if (answer == 1)
  {
    // Sets CID parameter
    answer = sendATcommand("AT+HTTPPARA=\"CID\",1", "OK", 5000);
    if (answer == 1)
    {
      // Met en place l'URL à cibler
      sprintf(aux_str2, "AT+HTTPPARA=\"URL\",\"%s", url2);
      Serial.print(aux_str2);
      sprintf(paraURL2, "lon=%s&lat=%s&time=2017-11-02_12:34:56&value1=%s&value2=%s&value3=%s", longitude, latitude, PARTM25, PARTM10, Co2);
      Serial.print(paraURL2);
      answer = sendATcommand("\"", "OK", 5000);
      if (answer == 1)
      {
        // Atteint l'URL, (n'y accede pas)
        answer = sendATcommand("AT+HTTPACTION=0", "+HTTPACTION:0,200", 30000); //10000 au debut
        Serial.println("Donnees passees a l'API");
      }
      else
      {
        Serial.println("Error setting the url");
      }
    }
    else
    {
      Serial.println("Error setting the CID");
    }
  }
  else
  {
    Serial.println("Error initializating");
  }
  sendATcommand("AT+HTTPTERM", "OK", 5000);
}

//************************************************************************************

void GPS()
{
  // Clean the input buffer
  while ( Serial.available() > 0) Serial.read();
  delay(100);
  // request Basic string
  sendATcommand("AT+CGPSINF=0", "AT+CGPSINF=0\r\n\r\n", 2000);

  counter = 0;
  answer = 0;
  // Initialisation des variables
  memset(Basic_str, '\0', 100);
  memset(RMC_str, '\0', 100);
  memset(latitude, '\0', 15);
  memset(longitude, '\0', 15);
  memset(altitude, '\0', 6);
  memset(verifbasic, '\0', 15);
  previous = millis();

  // this loop waits for the NMEA string
  do
  {
    if (Serial.available() != 0)
    {
      Basic_str[counter] = Serial.read();
      counter++;
      // check if the desired answer is in the response of the module
      if (strstr(Basic_str, "OK") != NULL)
      {
        answer = 1;
      }
    }
    // Waits for the asnwer with time out
  }
  while ((answer == 0) && ((millis() - previous) < 2000));

  Basic_str[counter - 3] = '\0';

  int TestLongChaine = strlen(Basic_str); //En test pour éviter les bugs qui font planter tout Arduino
  if ( TestLongChaine > 100 && TestLongChaine < 70)
  {
    software_Reset();
  }
  Serial.println("*************************************************");
  Serial.print("Basic string: ");
  Serial.println(Basic_str);

  // Traite Basic_str pour en extraire les données

  strtok(Basic_str, ",");

  strcpy(verifbasic, strtok(NULL, ","));


  if (strcmp(verifbasic, testlong) != 0)
  {
    strcpy(longitude, verifbasic);
    Serial.print("Longitude :");
    Serial.println(longitude);
    dmd2dd_arduino(longitude); // Récupère le contenu de "dmd2dd" et le convertit en char
    strcpy(longitude, return_dmd2dd);

    strcpy(latitude, strtok(NULL, ","));
    Serial.print("Latitude :");
    Serial.println(latitude);
    dmd2dd_arduino(latitude);
    strcpy(latitude, return_dmd2dd);

    strcpy(altitude, strtok(NULL, ","));
    Serial.print("Altitude :");
    Serial.println(altitude);
  }
  delay(3500);
}

//************************************************************************************

void dmd2dd_arduino( char s_valeurDMD[] ) // Code de recalcul des coordonnées
{
  // remise à zero de return_dmd2dd
  memset(return_dmd2dd, '\0', 11);

  // extraction de la part entiere
  // sachant que la part décimale est toujours de 9 char
  int taille_part_entiere = strlen(s_valeurDMD) - 9;
  char s_part_entiere[taille_part_entiere];
  memset(s_part_entiere, '\0', taille_part_entiere);
  for (int i = 0; i < taille_part_entiere; i++)
  {
    s_part_entiere[i] =  s_valeurDMD[i];
  }
  // On clot la chaine
  s_part_entiere[taille_part_entiere] = '\0';
  String string_part_entiere = String(s_part_entiere);

  // extraction de la part décimale & division par 100
  int j = 0;
  char s_part_decimale[8];
  memset(s_part_decimale, '\0', 8);
  for (int i = taille_part_entiere ; i <= strlen(s_valeurDMD) - 1; i++)
  {
    if ( s_valeurDMD[i] != '.')
    {
      s_part_decimale[j] = s_valeurDMD[i];
      j++;
    }
  }
  s_part_decimale[j] = '\0';
  String string_part_decimale = s_part_decimale;

  // conversion de la chaine en INT et division par 0.6
  //int i_part_decimale = strtol(s_part_decimale, NULL, 10);
  long i_part_decimale = string_part_decimale.toInt();
  i_part_decimale = i_part_decimale / 0.6;

  // convertir i_part_decimale en string
  String string_part_decimale_div = String(i_part_decimale);

  // concaténation du tout
  string_return_dmd2dd = String( string_part_entiere + '.' + string_part_decimale_div);

  // Conversion du String en char[]
  string_return_dmd2dd.toCharArray(return_dmd2dd, 11);
}

//************************************************************************************

int8_t sendATcommand(char* ATcommand, char* expected_answer1, unsigned int timeout)
{
  uint8_t x = 0,  answer = 0;
  char response[150];
  unsigned long previous;

  memset(response, '\0', 100);    // Initialize the string
  delay(100);

  while ( Serial.available() > 0) Serial.read();   // Clean the input buffer
  Serial.println(ATcommand);    // Send the AT command
  x = 0;
  previous = millis();
  // this loop waits for the answer
  do {
    if (Serial.available() != 0)
    {
      response[x] = Serial.read();
      x++;
      // check if the desired answer is in the response of the module
      if (strstr(response, expected_answer1) != NULL)
      {
        answer = 1;
      }
    }
    // Waits for the asnwer with time out
  }
  while ((answer == 0) && ((millis() - previous) < timeout));
  return answer;
}

//************************************************************************************

void power_on()
{
  uint8_t answer = 0;

  // checks if the module is started
  answer = sendATcommand("AT", "OK", 2000);
  if (answer == 0)
  {
    // power on pulse
    digitalWrite(onModulePin, HIGH);
    delay(3000);
    digitalWrite(onModulePin, LOW);

    // waits for an answer from the module
    while (answer == 0)
    {
      // Send AT every two seconds and wait for the answer
      answer = sendATcommand("AT", "OK", 2000);
    }
  }
}

//************************************************************************************

int8_t sendATcommand2(char* ATcommand, char* expected_answer1, char* expected_answer2, unsigned int timeout)
{
  uint8_t x = 0,  answer = 0;
  char response[150];
  unsigned long previous;

  memset(response, '\0', 100);    // Initialize the string
  delay(100);

  while ( Serial.available() > 0) Serial.read();   // Clean the input buffer
  Serial.println(ATcommand);    // Send the AT command

  x = 0;
  previous = millis();
  // this loop waits for the answer
  do
  {
    if (Serial.available() != 0)
    {
      response[x] = Serial.read();
      x++;
      // check if the desired answer 1 is in the response of the module
      if (strstr(response, expected_answer1) != NULL)
      {
        answer = 1;
      }
      // check if the desired answer 2 is in the response of the module
      if (strstr(response, expected_answer2) != NULL)
      {
        answer = 2;
      }
    }
    // Waits for the asnwer with time out
  }
  while ((answer == 0) && ((millis() - previous) < timeout));
  return answer;
}

//************************************************************************************

void ReqEcran()
{
  sprintf(GPST, "%d", VerGPS);

  memset(envoi, '\0', 14);
  strcat(envoi, " ,");
  strcat(envoi, NO2);
  strcat(envoi, ",");
  strcat(envoi, GPST);
  Wire.write(envoi, 14);
}

void Co2PM()
{
  memset(Co2, '\0', 5);
  memset(PARTM25, '\0', 6);
  memset(PARTM10, '\0', 6);
  
  Wire.requestFrom(10, 20);    // request from #10, 8 char
  while (Wire.available())
  { // slave may send less than requested
    char s[20];
    int x = 0;
    for (int y = 0; y < 21 ; y++)
    {
      s[x] = Wire.read(); // receive a byte as character
      x++;
    }
    s[x] = '\0';
    Serial.print("S: ");
    Serial.println(s);         // print the character
    //Coupe à chaque "," le contenu de S
    strtok(s, ",");

    //Attribut à Val1,2,3 les valeurs recuperees a chaques decoupage
    strcpy(PARTM25, strtok(NULL, ", "));
    Serial.print("Pm2.5 :");
    Serial.println(PARTM25);
    strcpy(PARTM10, strtok(NULL, ", "));
    Serial.print("Pm10 :");
    Serial.println(PARTM10);
    strcpy(Co2, strtok(NULL, ","));
    Serial.print("CO2 :");
    Serial.println(Co2);
  }
}

//************************************************************************************

void software_Reset()
{
  asm volatile ("jmp 0");
}

