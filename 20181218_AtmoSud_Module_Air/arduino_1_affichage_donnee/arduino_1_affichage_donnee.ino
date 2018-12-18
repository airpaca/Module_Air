#include <string.h>

#include <Adafruit_GFX.h> // Ecran
#include <RGBmatrixPanel.h>

#include <Wire.h> // I2C
#include <SPI.h>

#include <NDIRZ16.h> // CO2
#include <SC16IS750.h>

#include <SDS011.h> // PM

#define OE   9
#define LAT 10
#define CLK 11
#define A   A12
#define B   A13
#define C   A14
#define D   A15

/*****************************PM10/2.5***********************************************/
#define TX   14
#define RX   15

SDS011 my_sds;
unsigned char displayTemp[8];
unsigned int Pm25 = 0;
unsigned int Pm10 = 0;
float P25 = 0;// vrai calcul PM2.5
float P10 = 0;// vrai calcul PM10

RGBmatrixPanel matrix(A, B, C, D, CLK, LAT, OE, false, 64);

char Valeur1[5];
char Valeur2[5];
char Valeur3[5];

float Nrouge = 5.1;
float Nvert = 5.1;

//Var I2C
char MtoS[20]; //MasterToSlave
char valo3[5];
char valno2[5];
char CO2[5];
char pm10[5];
char pm25[5];

// Parametres echelle / affichage
int pixel_VL = 48;
int Red;
int Green;
int Blue;

// code couleur de chaque classe de l'echelle
int RGB_0[] = {0, 240, 20}; // classe 0 à 1 (0VL à 0.2VL)
int RGB_1[] = {0, 240, 20}; // classe 1 à 2 (0.2VL à 0.4VL)
int RGB_2[] = {0, 230, 0}; // classe 2 à 3 (0.4VL à 0.6VL)
int RGB_3[] = {255, 255, 0}; // classe 3 à 4 (0.6VL à 0.8VL)
int RGB_4[] = {255, 170, 0}; // classe 4 à 5 (0.8VL à 1VL)
int RGB_5[] = {255, 0, 0}; // classe 5 à 6 (1VL à 2VL)
int RGB_6[] = {128, 0, 0}; // classe 6 (2VL)

//Valeur Limite a modifier
int VLISA = 75; //Horaire
char VISA[5];
char GPST[3];

//CO2
SC16IS750 i2cuart = SC16IS750(SC16IS750_PROTOCOL_I2C, SC16IS750_ADDRESS_BB);
NDIRZ16 mySensor = NDIRZ16(&i2cuart);

void setup()
{
  matrix.begin();
  Wire.begin(10);                // join i2c bus with address #10

  Serial.println("Bonjour");

  Serial.begin(115200);
  i2cuart.begin(9600);

  my_sds.begin(RX, TX);
  Serial3.begin(9600);

  randomSeed(analogRead(0)); // permet un "meilleur" alea

  DemMHZ16();
}

void loop()
{
  matrix.fillRect(0, 0, 64, 32, matrix.Color444(0, 0, 0));

  LogoAtmoSud();
  delay(5000);
  matrix.fillRect(0, 0, 64, 32, matrix.Color444(0, 0, 0));

  RegionSud();
  delay(5000);
  matrix.fillRect(0, 0, 64, 32, matrix.Color444(0, 0, 0));

  LogoModuleAir();
  delay(5000);
  matrix.fillRect(0, 0, 64, 32, matrix.Color444(0, 0, 0));

  AirInterieur();
  delay(5000);
  matrix.fillRect(0, 0, 64, 32, matrix.Color444(0, 0, 0));

  MHZ16DIR();

  SDS011PM();
  delay(7500);
  matrix.fillRect(0, 0, 64, 32, matrix.Color444(0, 0, 0));

  GSMI2C();

  PolExt();
}

//************************************************************************************

void PolExt()
{
  if (strcmp(VISA, "ND") != 0)
  {
    if (strcmp(GPST, "0") == 0)
    {
      AirExtGPS();
      delay(5000);
      matrix.fillRect(0, 0, 64, 32, matrix.Color444(0, 0, 0));
    }
    else if (strcmp(GPST, "1") == 0)
    {
      AirExt();
      delay(5000);
      matrix.fillRect(0, 0, 64, 32, matrix.Color444(0, 0, 0));
    }

    ISA();
  }
}

void ISA()
{
  int isa = 0;
  isa = atoi(VISA);
  matrix.setCursor(2, 0);   // next line
  matrix.setTextSize(1);    // size 1 == 8 pixels high
  matrix.setTextColor(matrix.Color333(7, 7, 7));
  matrix.println("INDICE AIR");
  matrix.setCursor(5, 9);   // next line
  matrix.setTextSize(2);    // size 2 == 16 pixels high
  matrix.setTextColor(matrix.Color888(0, 83, 229));
  matrix.println(isa);

  code_RGB(VLISA, pixel_VL, isa);

  if (isa < 100)
  {
    matrix.fillRect(32, 9, 31, 14, matrix.Color888(Red, Green, Blue));
  } else if (isa >= 100)
  {
    matrix.fillRect(42, 9, 41, 14, matrix.Color888(Red, Green, Blue));
  }

  degrade();

  int posc2 = (isa * 48) / (VLISA);
  Serial.print("Posc2 = ");
  Serial.println(posc2);
  matrix.drawPixel(posc2, 27, matrix.Color444(255, 255, 255));
  matrix.drawLine(posc2 - 1, 26, posc2 + 1, 26, matrix.Color444(255, 255, 255));
  matrix.drawLine(posc2 - 2, 25, posc2 + 2, 25, matrix.Color444(255, 255, 255));
  delay(7500);

  matrix.fillRect(0, 0, 64, 32, matrix.Color444(0, 0, 0));
}

//************************************************************************************

void degrade()
{
  for (int i = 0; i < 65; i++) // degrade de couleur
  {
    code_RGB2(VLISA, pixel_VL, i);
    matrix.drawLine(i, 28, i, 31, matrix.Color888(Red, Green, Blue));
  }
  code_RGB2(VLISA, pixel_VL, pixel_VL);
  matrix.drawLine(pixel_VL, 26, pixel_VL, 27, matrix.Color888(Red, Green, Blue));
}

//**************************************Calcul Ecran*********************************************

//Calcule Degrade de couleur en fonction de la valeur, et de la valeur limite du polluant.
int code_RGB2(int VL_pol, int pixel_VL, int Val_pol)   // VL_pol = VL du polluant à Afficher, pixel_VL = numero du pixel qui marquera la VL sur la Matrice LED, Val_pol = Valeur renvoyée par la sonde ou l'API
{
  int pixel = Val_pol;
  // classe 0 à 1 (0VL à 0.2VL)
  if ( ((pixel_VL * 0) <= pixel) && (pixel <= (pixel_VL * 0.2)) )
  {
    Red = RGB_0[0] + ( (RGB_1[0] - RGB_0[0]) / (pixel_VL * 0.2 - pixel_VL * 0) ) * ( pixel - pixel_VL * 0 ) ; //                   RGB_0[1];
    Green = RGB_0[1] + ( (RGB_1[1] - RGB_0[1]) / (pixel_VL * 0.2 - pixel_VL * 0) ) * ( pixel - pixel_VL * 0 ) ;
    Blue = RGB_0[2] + ( (RGB_1[2] - RGB_0[2]) / (pixel_VL * 0.2 - pixel_VL * 0) ) * ( pixel - pixel_VL * 0 ) ;

    return Red;
    return Green;
    return Blue;

    // classe 1 à 2 (0.2VL à 0.4VL)
  } else if ( ((pixel_VL * 0.2) < pixel) && (pixel <= (pixel_VL * 0.4)) )
  {
    Red = RGB_1[0] + ( (RGB_2[0] - RGB_1[0]) / (pixel_VL * 0.4 - pixel_VL * 0.2) ) * ( pixel - pixel_VL * 0.2 ) ; //                   RGB_0[1];
    Green = RGB_1[1] + ( (RGB_2[1] - RGB_1[1]) / (pixel_VL * 0.4 - pixel_VL * 0.2) ) * ( pixel - pixel_VL * 0.2 ) ;
    Blue = RGB_1[2] + ( (RGB_2[2] - RGB_1[2]) / (pixel_VL * 0.4 - pixel_VL * 0.2) ) * ( pixel - pixel_VL * 0.2 ) ;

    return Red;
    return Green;
    return Blue;

    // classe 2 à 3 (0.4VL à 0.6VL)
  } else if ( ((pixel_VL * 0.4) < pixel) && (pixel <= (pixel_VL * 0.6)) )
  {
    Red = RGB_2[0] + ( (RGB_3[0] - RGB_2[0]) / (pixel_VL * 0.6 - pixel_VL * 0.4) ) * ( pixel - pixel_VL * 0.4 ) ; //                   RGB_0[1];
    Green = RGB_2[1] + ( (RGB_3[1] - RGB_2[1]) / (pixel_VL * 0.6 - pixel_VL * 0.4) ) * ( pixel - pixel_VL * 0.4 ) ;
    Blue = RGB_2[2] + ( (RGB_3[2] - RGB_2[2]) / (pixel_VL * 0.6 - pixel_VL * 0.4) ) * ( pixel - pixel_VL * 0.4 ) ;

    return Red;
    return Green;
    return Blue;

    // classe 3 à 4 (0.6VL à 0.8VL)
  } else if ( ((pixel_VL * 0.6) < pixel) && (pixel <= (pixel_VL * 0.8)) )
  {
    Red = RGB_3[0] + ( (RGB_4[0] - RGB_3[0]) / (pixel_VL * 0.8 - pixel_VL * 0.6) ) * ( pixel - pixel_VL * 0.6 ) ; //                   RGB_0[1];
    Green = RGB_3[1] + ( (RGB_4[1] - RGB_3[1]) / (pixel_VL * 0.8 - pixel_VL * 0.6) ) * ( pixel - pixel_VL * 0.6 ) ;
    Blue = RGB_3[2] + ( (RGB_4[2] - RGB_3[2]) / (pixel_VL * 0.8 - pixel_VL * 0.6) ) * ( pixel - pixel_VL * 0.6 ) ;

    return Red;
    return Green;
    return Blue;

    // classe 4 à 5 (0.8VL à 1VL)
  } else if ( ((pixel_VL * 0.8) < pixel) && (pixel < (pixel_VL * 1)) )
  {
    Red = RGB_4[0] + ( (RGB_5[0] - RGB_4[0]) / (pixel_VL * 1 - pixel_VL * 0.8) ) * ( pixel - pixel_VL * 0.8 ) ; //                   RGB_0[1];
    Green = RGB_4[1] + ( (RGB_5[1] - RGB_4[1]) / (pixel_VL * 1 - pixel_VL * 0.8) ) * ( pixel - pixel_VL * 0.8 ) ;
    Blue = RGB_4[2] + ( (RGB_5[2] - RGB_4[2]) / (pixel_VL * 1 - pixel_VL * 0.8) ) * ( pixel - pixel_VL * 0.8 ) ;

    return Red;
    return Green;
    return Blue;

    // classe 5 à 6 (1VL au delà et l'infini)
  } else if ((pixel_VL * 1) <= pixel)
  {
    Red = RGB_5[0] + ( (RGB_6[0] - RGB_5[0]) / (pixel_VL * 2 - pixel_VL * 1) ) * ( pixel - pixel_VL * 1 ) ; //                   RGB_0[1];
    Green = RGB_5[1] + ( (RGB_6[1] - RGB_5[1]) / (pixel_VL * 2 - pixel_VL * 1) ) * ( pixel - pixel_VL * 1 ) ;
    Blue = RGB_5[2] + ( (RGB_6[2] - RGB_5[2]) / (pixel_VL * 2 - pixel_VL * 1) ) * ( pixel - pixel_VL * 1 ) ;

    return Red;
    return Green;
    return Blue;
  }
} // Fin de la fonction code_RGB


//Calcule Carre de couleur en fonction de la valeur, et de la valeur limite du polluant.
int code_RGB(int VL_pol, int pixel_VL, int Val_pol)   // VL_pol = VL du polluant à Afficher, pixel_VL = numero du pixel qui marquera la VL sur la Matrice LED, Val_pol = Valeur renvoyée par la sonde ou l'API
{
  double pixel = (pixel_VL * Val_pol);
  pixel = pixel / VL_pol ;

  if (pixel > 64)
  {
    pixel = 64;
  }

  // classe 0 à 1 (0VL à 0.2VL)
  if ( ((pixel_VL * 0) <= pixel) && (pixel <= (pixel_VL * 0.2)) )
  {
    Red = RGB_0[0] + ( (RGB_1[0] - RGB_0[0]) / (pixel_VL * 0.2 - pixel_VL * 0) ) * ( pixel - pixel_VL * 0 ) ; //                   RGB_0[1];
    Green = RGB_0[1] + ( (RGB_1[1] - RGB_0[1]) / (pixel_VL * 0.2 - pixel_VL * 0) ) * ( pixel - pixel_VL * 0 ) ;
    Blue = RGB_0[2] + ( (RGB_1[2] - RGB_0[2]) / (pixel_VL * 0.2 - pixel_VL * 0) ) * ( pixel - pixel_VL * 0 ) ;

    return Red;
    return Green;
    return Blue;

    // classe 1 à 2 (0.2VL à 0.4VL)
  } else if ( ((pixel_VL * 0.2) < pixel) && (pixel <= (pixel_VL * 0.4)) )
  {
    Red = RGB_1[0] + ( (RGB_2[0] - RGB_1[0]) / (pixel_VL * 0.4 - pixel_VL * 0.2) ) * ( pixel - pixel_VL * 0.2 ) ; //                   RGB_0[1];
    Green = RGB_1[1] + ( (RGB_2[1] - RGB_1[1]) / (pixel_VL * 0.4 - pixel_VL * 0.2) ) * ( pixel - pixel_VL * 0.2 ) ;
    Blue = RGB_1[2] + ( (RGB_2[2] - RGB_1[2]) / (pixel_VL * 0.4 - pixel_VL * 0.2) ) * ( pixel - pixel_VL * 0.2 ) ;

    return Red;
    return Green;
    return Blue;

    // classe 2 à 3 (0.4VL à 0.6VL)
  } else if ( ((pixel_VL * 0.4) < pixel) && (pixel <= (pixel_VL * 0.6)) )
  {
    Red = RGB_2[0] + ( (RGB_3[0] - RGB_2[0]) / (pixel_VL * 0.6 - pixel_VL * 0.4) ) * ( pixel - pixel_VL * 0.4 ) ; //                   RGB_0[1];
    Green = RGB_2[1] + ( (RGB_3[1] - RGB_2[1]) / (pixel_VL * 0.6 - pixel_VL * 0.4) ) * ( pixel - pixel_VL * 0.4 ) ;
    Blue = RGB_2[2] + ( (RGB_3[2] - RGB_2[2]) / (pixel_VL * 0.6 - pixel_VL * 0.4) ) * ( pixel - pixel_VL * 0.4 ) ;

    return Red;
    return Green;
    return Blue;

    // classe 3 à 4 (0.6VL à 0.8VL)
  } else if ( ((pixel_VL * 0.6) < pixel) && (pixel <= (pixel_VL * 0.8)) )
  {
    Red = RGB_3[0] + ( (RGB_4[0] - RGB_3[0]) / (pixel_VL * 0.8 - pixel_VL * 0.6) ) * ( pixel - pixel_VL * 0.6 ) ; //                   RGB_0[1];
    Green = RGB_3[1] + ( (RGB_4[1] - RGB_3[1]) / (pixel_VL * 0.8 - pixel_VL * 0.6) ) * ( pixel - pixel_VL * 0.6 ) ;
    Blue = RGB_3[2] + ( (RGB_4[2] - RGB_3[2]) / (pixel_VL * 0.8 - pixel_VL * 0.6) ) * ( pixel - pixel_VL * 0.6 ) ;

    return Red;
    return Green;
    return Blue;

    // classe 4 à 5 (0.8VL à 1VL)
  } else if ( ((pixel_VL * 0.8) < pixel) && (pixel < (pixel_VL * 1)) )
  {
    Red = RGB_4[0] + ( (RGB_5[0] - RGB_4[0]) / (pixel_VL * 1 - pixel_VL * 0.8) ) * ( pixel - pixel_VL * 0.8 ) ; //                   RGB_0[1];
    Green = RGB_4[1] + ( (RGB_5[1] - RGB_4[1]) / (pixel_VL * 1 - pixel_VL * 0.8) ) * ( pixel - pixel_VL * 0.8 ) ;
    Blue = RGB_4[2] + ( (RGB_5[2] - RGB_4[2]) / (pixel_VL * 1 - pixel_VL * 0.8) ) * ( pixel - pixel_VL * 0.8 ) ;

    return Red;
    return Green;
    return Blue;

    // classe 5 à 6 (1VL au delà et l'infini)
  } else if ((pixel_VL * 1) <= pixel)
  {
    Red = RGB_5[0] + ( (RGB_6[0] - RGB_5[0]) / (pixel_VL * 2 - pixel_VL * 1) ) * ( pixel - pixel_VL * 1 ) ; //                   RGB_0[1];
    Green = RGB_5[1] + ( (RGB_6[1] - RGB_5[1]) / (pixel_VL * 2 - pixel_VL * 1) ) * ( pixel - pixel_VL * 1 ) ;
    Blue = RGB_5[2] + ( (RGB_6[2] - RGB_5[2]) / (pixel_VL * 2 - pixel_VL * 1) ) * ( pixel - pixel_VL * 1 ) ;

    return Red;
    return Green;
    return Blue;
  }
} // Fin de la fonction code_RGB

void GSMI2C()
{
  Wire.requestFrom(8, 14);    // request 14 bytes from slave device #8
  while (Wire.available())
  { // slave may send less than requested
    char c[20];
    int x = 0;
    for (int y = 0; y < 14 ; y++)
    {
      c[x] = Wire.read(); // receive a byte as character
      x++;
    }
    c[x] = '\0';
    Serial.print("C: ");
    Serial.println(c);         // print the character
    strcpy(Valeur3, c);
    delay(1000);

    //Coupe à chaque "," le contenu de C
    strtok(c, ",");

    //Attribut à Val1,2,3 les valeurs recuperees a chaques decoupage
    strcpy(VISA, strtok(NULL, ","));
    Serial.print("Val1: ");
    Serial.println(VISA);
    strcpy(GPST, strtok(NULL, ","));
    Serial.print("GPS: ");
    Serial.println(GPST);
  }
  delay(500);
}

/********************************************SondeCO2********************************************/

void DemMHZ16() // Commande du Setup pour vérifier si la sonde répond.
{
  if (i2cuart.ping())
  {
    Serial.println("SC16IS750 found.");
    Serial.println("Wait 10 seconds for sensor initialization...");

    power(1);
    //Wait for the NDIR sensor to initialize.
    delay(10000);
  } else
  {
    Serial.println("SC16IS750 not found.");
    while (1);
  }
}

void power (uint8_t state) // Commande d'allumage de la Sonde.
{
  i2cuart.pinMode(0, INPUT);      //set up for the power control pin

  if (state) {
    i2cuart.pinMode(0, INPUT);  //turn on the power of MH-Z16
  } else {
    i2cuart.pinMode(0, OUTPUT);
    i2cuart.digitalWrite(0, 0); //turn off the power of MH-Z16
  }
}

void MHZ16DIR() // Lecture + message de prévention du CO2
{
  if (mySensor.measure())
  { // mesure CO2
    matrix.fillRect(0, 8, 64, 24, matrix.Color444(0, 0, 0));
    //CO2
    matrix.fillRect(0, 0, 64, 32, matrix.Color444(0, 0, 0));
    matrix.setCursor(2, 0);   // next line
    matrix.setTextSize(1);    // size 1 == 8 pixels high
    matrix.setTextColor(matrix.Color888(0, 82, 228));
    matrix.println(" CO ppm");

    //2 de CO2 en indice
    matrix.drawLine(20, 3, 21, 3, matrix.Color888(0, 82, 228));
    matrix.drawLine(22, 4, 22, 4, matrix.Color888(0, 82, 228));
    matrix.drawLine(21, 5, 21, 5, matrix.Color888(0, 82, 228));
    matrix.drawLine(20, 6, 20, 6, matrix.Color888(0, 82, 228));
    matrix.drawLine(20, 7, 22, 7, matrix.Color888(0, 82, 228));
    
    //Petite maison
    matrix.drawLine(55, 4, 56, 4, matrix.Color333(7, 7, 7));
    matrix.drawLine(62, 4, 63, 4, matrix.Color333(7, 7, 7));
    matrix.drawLine(59, 0, 59, 0, matrix.Color333(7, 7, 7));
    matrix.drawLine(58, 1, 58, 1, matrix.Color333(7, 7, 7));
    matrix.drawLine(60, 1, 60, 1, matrix.Color333(7, 7, 7));
    matrix.drawLine(57, 2, 57, 2, matrix.Color333(7, 7, 7));
    matrix.drawLine(61, 2, 61, 2, matrix.Color333(7, 7, 7));
    matrix.drawLine(56, 3, 56, 3, matrix.Color333(7, 7, 7));
    matrix.drawLine(62, 3, 62, 3, matrix.Color333(7, 7, 7));
    matrix.drawLine(56, 5, 56, 5, matrix.Color333(7, 7, 7));
    matrix.drawLine(62, 5, 62, 5, matrix.Color333(7, 7, 7));
    matrix.drawLine(56, 6, 56, 6, matrix.Color333(7, 7, 7));
    matrix.drawLine(62, 6, 62, 6, matrix.Color333(7, 7, 7));
    matrix.drawLine(56, 7, 62, 7, matrix.Color333(7, 7, 7));

    matrix.setCursor(5, 9);   // next line
    matrix.setTextSize(2);    // size 2 == 16 pixels high

    Serial.print("CO2 :");
    Serial.println(mySensor.ppm);

    if (mySensor.ppm <= 1000)
    { // CO2 niv bon
      matrix.fillRect(47, 9, 17, 14, matrix.Color444(0, 15, 0));
      matrix.setCursor(21, 25);   // next line
      matrix.setTextSize(1);    // size 1 == 8 pixels high
      matrix.setTextColor(matrix.Color333(0, 7, 0));
      matrix.println("BIEN");
      matrix.setCursor(6, 9);
      matrix.setTextSize(2);
      matrix.setTextColor(matrix.Color333(7, 7, 7));
      matrix.println(mySensor.ppm);
    } // fin CO2 niv bon

    if (mySensor.ppm > 1000 && mySensor.ppm <= 1700)
    { // CO2 niv moyen
      matrix.fillRect(47, 9, 17, 14, matrix.Color444(15, 15, 0));
      matrix.setCursor(7, 25);   // next line
      matrix.setTextSize(1);    // size 1 == 8 pixels high
      matrix.setTextColor(matrix.Color333(7, 7, 0));
      matrix.println("AERER SVP");
      matrix.setTextColor(matrix.Color333(7, 7, 7));
      matrix.setCursor(0, 9);
      matrix.setTextSize(2);
      matrix.println(mySensor.ppm);
    } // fin CO2 niv moyen

    if (mySensor.ppm > 1700)
    { // CO2 niv mauvais
      matrix.fillRect(47, 9, 17, 14, matrix.Color444(15, 0, 0));
      matrix.setCursor(3, 25);   // next line
      matrix.setTextSize(1);    // size 1 == 8 pixels high
      matrix.setTextColor(matrix.Color333(7, 0, 0));
      matrix.println("AERER VITE");
      matrix.setTextColor(matrix.Color333(7, 7, 7));
      matrix.setCursor(0, 9);
      matrix.setTextSize(2);
      matrix.println(mySensor.ppm);
    } // fin CO2 niv mauvais
    snprintf(CO2, sizeof CO2, "%lu", (unsigned long)mySensor.ppm);
    delay(7500);
    matrix.fillRect(0, 0, 64, 32, matrix.Color444(0, 0, 0));
  } // fin mesure CO2
}

/********************************************SondePM********************************************/

// Lecture PM
void SDS011PM()
{
  uint8_t mData = 0;
  uint8_t i = 0;
  uint8_t mPkt[10] = {0};
  uint8_t mCheck = 0;
  while (Serial3.available() > 0)
  {
    mData = Serial3.read();     delay(2);//wait until packet is received

    if (mData == 0xAA) //head1 ok
    {
      mPkt[0] =  mData;
      mData = Serial3.read();

      if (mData == 0xc0) //head2 ok
      {
        mPkt[1] =  mData;
        mCheck = 0;
        for (i = 0; i < 6; i++) //data recv and crc calc
        {
          mPkt[i + 2] = Serial3.read();
          delay(2);
          mCheck += mPkt[i + 2];
        }
        mPkt[8] = Serial3.read();
        delay(1);
        mPkt[9] = Serial3.read();
        if (mCheck == mPkt[8]) //crc ok
        {
          Serial3.flush();

          Pm25 = (uint16_t)mPkt[2] | (uint16_t)(mPkt[3] << 8);
          Pm10 = (uint16_t)mPkt[4] | (uint16_t)(mPkt[5] << 8);
          if (Pm25 > 9999)
            Pm25 = 9999;
          if (Pm10 > 9999)
            Pm10 = 9999;

          P25 = (float(Pm25) / 10);
          P10 = (float(Pm10) / 10);

          matrix.fillRect(0, 0, 64, 32, matrix.Color444(0, 0, 0));
          matrix.setCursor(0, 0);   // next line
          matrix.setTextSize(1);    // size 1 == 8 pixels high
          matrix.setTextColor(matrix.Color888(0, 82, 228));
          matrix.println("PM10");

          matrix.drawLine(51, 0, 52, 0, matrix.Color888(0, 82, 228));
          matrix.drawLine(59, 0, 59, 0, matrix.Color333(7, 7, 7));
          matrix.drawLine(43, 1, 43, 1, matrix.Color888(0, 82, 228));
          matrix.drawLine(53, 1, 53, 1, matrix.Color888(0, 82, 228));
          matrix.drawLine(58, 1, 58, 1, matrix.Color333(7, 7, 7));
          matrix.drawLine(60, 1, 60, 1, matrix.Color333(7, 7, 7));
          matrix.drawLine(28, 2, 28, 2, matrix.Color888(0, 82, 228));
          matrix.drawLine(32, 2, 32, 2, matrix.Color888(0, 82, 228));
          matrix.drawLine(35, 2, 37, 2, matrix.Color888(0, 82, 228));
          matrix.drawLine(42, 2, 42, 2, matrix.Color888(0, 82, 228));
          matrix.drawLine(52, 2, 53, 2, matrix.Color888(0, 82, 228));
          matrix.drawLine(57, 2, 57, 2, matrix.Color333(7, 7, 7));
          matrix.drawLine(61, 2, 61, 2, matrix.Color333(7, 7, 7));
          matrix.drawLine(28, 3, 28, 3, matrix.Color888(0, 82, 228));
          matrix.drawLine(32, 3, 32, 3, matrix.Color888(0, 82, 228));
          matrix.drawLine(34, 3, 34, 3, matrix.Color888(0, 82, 228));
          matrix.drawLine(37, 3, 38, 3, matrix.Color888(0, 82, 228));
          matrix.drawLine(42, 3, 42, 3, matrix.Color888(0, 82, 228));
          matrix.drawLine(45, 3, 46, 3, matrix.Color888(0, 82, 228));
          matrix.drawLine(48, 3, 48, 3, matrix.Color888(0, 82, 228));
          matrix.drawLine(53, 3, 53, 3, matrix.Color888(0, 82, 228));
          matrix.drawLine(56, 3, 56, 3, matrix.Color333(7, 7, 7));
          matrix.drawLine(62, 3, 62, 3, matrix.Color333(7, 7, 7));
          matrix.drawLine(28, 4, 28, 4, matrix.Color888(0, 82, 228));
          matrix.drawLine(32, 4, 32, 4, matrix.Color888(0, 82, 228));
          matrix.drawLine(34, 4, 34, 4, matrix.Color888(0, 82, 228));
          matrix.drawLine(37, 4, 38, 4, matrix.Color888(0, 82, 228));
          matrix.drawLine(41, 4, 41, 4, matrix.Color888(0, 82, 228));
          matrix.drawLine(45, 4, 45, 4, matrix.Color888(0, 82, 228));
          matrix.drawLine(47, 4, 47, 4, matrix.Color888(0, 82, 228));
          matrix.drawLine(49, 4, 49, 4, matrix.Color888(0, 82, 228));
          matrix.drawLine(51, 4, 52, 4, matrix.Color888(0, 82, 228));
          matrix.drawLine(55, 4, 56, 4, matrix.Color333(7, 7, 7));
          matrix.drawLine(62, 4, 63, 4, matrix.Color333(7, 7, 7));
          matrix.drawLine(28, 5, 28, 5, matrix.Color888(0, 82, 228));
          matrix.drawLine(31, 5, 32, 5, matrix.Color888(0, 82, 228));
          matrix.drawLine(35, 5, 36, 5, matrix.Color888(0, 82, 228));
          matrix.drawLine(38, 5, 38, 5, matrix.Color888(0, 82, 228));
          matrix.drawLine(41, 5, 41, 5, matrix.Color888(0, 82, 228));
          matrix.drawLine(45, 5, 45, 5, matrix.Color888(0, 82, 228));
          matrix.drawLine(47, 5, 47, 5, matrix.Color888(0, 82, 228));
          matrix.drawLine(49, 5, 49, 5, matrix.Color888(0, 82, 228));
          matrix.drawLine(56, 5, 56, 5, matrix.Color333(7, 7, 7));
          matrix.drawLine(62, 5, 62, 5, matrix.Color333(7, 7, 7));
          matrix.drawLine(28, 6, 30, 6, matrix.Color888(0, 82, 228));
          matrix.drawLine(32, 6, 32, 6, matrix.Color888(0, 82, 228));
          matrix.drawLine(38, 6, 38, 6, matrix.Color888(0, 82, 228));
          matrix.drawLine(40, 6, 40, 6, matrix.Color888(0, 82, 228));
          matrix.drawLine(45, 6, 45, 6, matrix.Color888(0, 82, 228));
          matrix.drawLine(47, 6, 47, 6, matrix.Color888(0, 82, 228));
          matrix.drawLine(49, 6, 49, 6, matrix.Color888(0, 82, 228));
          matrix.drawLine(56, 6, 56, 6, matrix.Color333(7, 7, 7));
          matrix.drawLine(62, 6, 62, 6, matrix.Color333(7, 7, 7));
          matrix.drawLine(28, 7, 28, 7, matrix.Color888(0, 82, 228));
          matrix.drawLine(35, 7, 37, 7, matrix.Color888(0, 82, 228));
          matrix.drawLine(45, 7, 45, 7, matrix.Color888(0, 82, 228));
          matrix.drawLine(47, 7, 47, 7, matrix.Color888(0, 82, 228));
          matrix.drawLine(49, 7, 49, 7, matrix.Color888(0, 82, 228));
          matrix.drawLine(56, 7, 62, 7, matrix.Color333(7, 7, 7));

          matrix.setCursor(5, 9);   // next line
          matrix.setTextSize(2);    // size 2 == 16 pixels high

          if (P10 <= 15) { // PM10 niv IDEAL
            matrix.fillRect(47, 9, 17, 14, matrix.Color444(0, 15, 0));

            matrix.setCursor(18, 25);   // next line
            matrix.setTextSize(1);    // size 1 == 8 pixels high
            matrix.setTextColor(matrix.Color333(0, 7, 0));
            matrix.println("IDEAL");

            if (P10 >= 5 && P10 < 10)
            {
              matrix.setCursor(6, 9);//placement du curseur differents >5 et <10
              matrix.setTextSize(2);
              matrix.setTextColor(matrix.Color333(7, 7, 7));
              matrix.println(P10, 1);
            }
            else if (P10 < 5)
            {
              matrix.setCursor(0, 9);
              matrix.setTextSize(2);
              matrix.setTextColor(matrix.Color333(7, 7, 7));
              matrix.println("< 5"); // affichage borné inf à 5
            }
            else
            {
              matrix.setCursor(0, 9);
              matrix.setTextSize(2);
              matrix.setTextColor(matrix.Color333(7, 7, 7));
              matrix.println(P10, 1);
            }
          } // fin PM10 niv bon

          if (P10 > 75)
          { // CO2 niv mauvais
            //matrix.setTextColor(matrix.Color444(15, 0, 0));
            matrix.fillRect(47, 9, 17, 14, matrix.Color444(15, 0, 0));
            matrix.setCursor(3, 25);   // next line
            matrix.setTextSize(1);    // size 1 == 8 pixels high
            matrix.setTextColor(matrix.Color333(7, 0, 0));
            matrix.println("MAUVAIS");
            matrix.setTextColor(matrix.Color333(7, 7, 7));
            matrix.setCursor(0, 9);
            matrix.setTextSize(2);
            if (P10 > 99)
            {
              matrix.println(P10, 0);
            }
            else
            {
              matrix.println(P10, 1);
            }
          } // fin PM10 niv mauvais

          if (P10 > 15 && P10 <= 30)
          { // CO2 niv BON
            // matrix.setTextColor(matrix.Color444(15, 15, 0));
            matrix.fillRect(47, 9, 17, 14, matrix.Color444(15, 15, 0));
            matrix.setCursor(22, 25);   // next line
            matrix.setTextSize(1);    // size 1 == 8 pixels high
            matrix.setTextColor(matrix.Color333(7, 7, 0));
            matrix.println("BON");
            matrix.setTextColor(matrix.Color333(7, 7, 7));
            matrix.setCursor(0, 9);
            matrix.setTextSize(2);
            matrix.println(P10, 1);
          }

          if (P10 > 30 && P10 <= 75)
          { // CO2 niv moyen
            // matrix.setTextColor(matrix.Color444(15, 15, 0));
            matrix.fillRect(47, 9, 17, 14, matrix.Color444(15, 15, 0));
            matrix.setCursor(15, 25);   // next line
            matrix.setTextSize(1);    // size 1 == 8 pixels high
            matrix.setTextColor(matrix.Color333(7, 7, 0));
            matrix.println("MOYEN");
            matrix.setTextColor(matrix.Color333(7, 7, 7));
            matrix.setCursor(0, 9);
            matrix.setTextSize(2);
            matrix.println(P10, 1);
          }

          delay(7500);
          matrix.fillRect(0, 0, 64, 32, matrix.Color444(0, 0, 0));

          matrix.setCursor(0, 0);   // next line
          matrix.setTextSize(1);    // size 1 == 8 pixels high
          matrix.setTextColor(matrix.Color888(0, 82, 228));
          matrix.println("PM2");
          matrix.setCursor(20, 0);
          matrix.drawPixel(18, 6, matrix.Color888(0, 82, 228));
          matrix.println("5");

          matrix.drawLine(51, 0, 52, 0, matrix.Color888(0, 82, 228));
          matrix.drawLine(59, 0, 59, 0, matrix.Color333(7, 7, 7));
          matrix.drawLine(43, 1, 43, 1, matrix.Color888(0, 82, 228));
          matrix.drawLine(53, 1, 53, 1, matrix.Color888(0, 82, 228));
          matrix.drawLine(58, 1, 58, 1, matrix.Color333(7, 7, 7));
          matrix.drawLine(60, 1, 60, 1, matrix.Color333(7, 7, 7));
          matrix.drawLine(28, 2, 28, 2, matrix.Color888(0, 82, 228));
          matrix.drawLine(32, 2, 32, 2, matrix.Color888(0, 82, 228));
          matrix.drawLine(35, 2, 37, 2, matrix.Color888(0, 82, 228));
          matrix.drawLine(42, 2, 42, 2, matrix.Color888(0, 82, 228));
          matrix.drawLine(52, 2, 53, 2, matrix.Color888(0, 82, 228));
          matrix.drawLine(57, 2, 57, 2, matrix.Color333(7, 7, 7));
          matrix.drawLine(61, 2, 61, 2, matrix.Color333(7, 7, 7));
          matrix.drawLine(28, 3, 28, 3, matrix.Color888(0, 82, 228));
          matrix.drawLine(32, 3, 32, 3, matrix.Color888(0, 82, 228));
          matrix.drawLine(34, 3, 34, 3, matrix.Color888(0, 82, 228));
          matrix.drawLine(37, 3, 38, 3, matrix.Color888(0, 82, 228));
          matrix.drawLine(42, 3, 42, 3, matrix.Color888(0, 82, 228));
          matrix.drawLine(45, 3, 46, 3, matrix.Color888(0, 82, 228));
          matrix.drawLine(48, 3, 48, 3, matrix.Color888(0, 82, 228));
          matrix.drawLine(53, 3, 53, 3, matrix.Color888(0, 82, 228));
          matrix.drawLine(56, 3, 56, 3, matrix.Color333(7, 7, 7));
          matrix.drawLine(62, 3, 62, 3, matrix.Color333(7, 7, 7));
          matrix.drawLine(28, 4, 28, 4, matrix.Color888(0, 82, 228));
          matrix.drawLine(32, 4, 32, 4, matrix.Color888(0, 82, 228));
          matrix.drawLine(34, 4, 34, 4, matrix.Color888(0, 82, 228));
          matrix.drawLine(37, 4, 38, 4, matrix.Color888(0, 82, 228));
          matrix.drawLine(41, 4, 41, 4, matrix.Color888(0, 82, 228));
          matrix.drawLine(45, 4, 45, 4, matrix.Color888(0, 82, 228));
          matrix.drawLine(47, 4, 47, 4, matrix.Color888(0, 82, 228));
          matrix.drawLine(49, 4, 49, 4, matrix.Color888(0, 82, 228));
          matrix.drawLine(51, 4, 52, 4, matrix.Color888(0, 82, 228));
          matrix.drawLine(55, 4, 56, 4, matrix.Color333(7, 7, 7));
          matrix.drawLine(62, 4, 63, 4, matrix.Color333(7, 7, 7));
          matrix.drawLine(28, 5, 28, 5, matrix.Color888(0, 82, 228));
          matrix.drawLine(31, 5, 32, 5, matrix.Color888(0, 82, 228));
          matrix.drawLine(35, 5, 36, 5, matrix.Color888(0, 82, 228));
          matrix.drawLine(38, 5, 38, 5, matrix.Color888(0, 82, 228));
          matrix.drawLine(41, 5, 41, 5, matrix.Color888(0, 82, 228));
          matrix.drawLine(45, 5, 45, 5, matrix.Color888(0, 82, 228));
          matrix.drawLine(47, 5, 47, 5, matrix.Color888(0, 82, 228));
          matrix.drawLine(49, 5, 49, 5, matrix.Color888(0, 82, 228));
          matrix.drawLine(56, 5, 56, 5, matrix.Color333(7, 7, 7));
          matrix.drawLine(62, 5, 62, 5, matrix.Color333(7, 7, 7));
          matrix.drawLine(28, 6, 30, 6, matrix.Color888(0, 82, 228));
          matrix.drawLine(32, 6, 32, 6, matrix.Color888(0, 82, 228));
          matrix.drawLine(38, 6, 38, 6, matrix.Color888(0, 82, 228));
          matrix.drawLine(40, 6, 40, 6, matrix.Color888(0, 82, 228));
          matrix.drawLine(45, 6, 45, 6, matrix.Color888(0, 82, 228));
          matrix.drawLine(47, 6, 47, 6, matrix.Color888(0, 82, 228));
          matrix.drawLine(49, 6, 49, 6, matrix.Color888(0, 82, 228));
          matrix.drawLine(56, 6, 56, 6, matrix.Color333(7, 7, 7));
          matrix.drawLine(62, 6, 62, 6, matrix.Color333(7, 7, 7));
          matrix.drawLine(28, 7, 28, 7, matrix.Color888(0, 82, 228));
          matrix.drawLine(35, 7, 37, 7, matrix.Color888(0, 82, 228));
          matrix.drawLine(45, 7, 45, 7, matrix.Color888(0, 82, 228));
          matrix.drawLine(47, 7, 47, 7, matrix.Color888(0, 82, 228));
          matrix.drawLine(49, 7, 49, 7, matrix.Color888(0, 82, 228));
          matrix.drawLine(56, 7, 62, 7, matrix.Color333(7, 7, 7));

          if (P25 < 10) { // PM2,5 niv IDEAL

            matrix.fillRect(47, 9, 17, 14, matrix.Color444(0, 15, 0));

            matrix.setCursor(18, 25);   // next line
            matrix.setTextSize(1);    // size 1 == 8 pixels high
            matrix.setTextColor(matrix.Color333(0, 7, 0));
            matrix.println("IDEAL");

            if (P25 < 3)
            {
              matrix.setCursor(0, 9);
              matrix.setTextSize(2);
              matrix.setTextColor(matrix.Color333(7, 7, 7));
              matrix.println("< 3"); // affichage borné inf à 3
            }
            else
            {
              matrix.setCursor(6, 9);
              matrix.setTextSize(2);
              matrix.setTextColor(matrix.Color333(7, 7, 7));
              matrix.println(P25, 1);
            }
          } // fin PM2,5 niv bon

          if (P25 > 50) { // PM2,5 niv mauvais
            //matrix.setTextColor(matrix.Color444(15, 0, 0));
            matrix.fillRect(47, 9, 17, 14, matrix.Color444(15, 0, 0));
            matrix.setCursor(3, 25);   // next line
            matrix.setTextSize(1);    // size 1 == 8 pixels high
            matrix.setTextColor(matrix.Color333(7, 0, 0));
            matrix.println("MAUVAIS");
            matrix.setTextColor(matrix.Color333(7, 7, 7));
            matrix.setCursor(0, 9);
            matrix.setTextSize(2);
            if (P25 > 99) {
              matrix.println(P25, 0);
            }
            else {
              matrix.println(P25, 1);
            }
          } // fin PM2,5 niv mauvais

          if (P25 >= 10 && P25 <= 20) { // PM2,5 niv BON
            // matrix.setTextColor(matrix.Color444(15, 15, 0));
            matrix.fillRect(47, 9, 17, 14, matrix.Color444(15, 15, 0));
            matrix.setCursor(22, 25);   // next line
            matrix.setTextSize(1);    // size 1 == 8 pixels high
            matrix.setTextColor(matrix.Color333(7, 7, 0));
            matrix.println("BON");
            matrix.setTextColor(matrix.Color333(7, 7, 7));
            matrix.setCursor(0, 9);
            matrix.setTextSize(2);
            matrix.println(P25, 1);
          }

          if (P25 > 20 && P25 <= 50) { // PM2,5 niv moyen
            // matrix.setTextColor(matrix.Color444(15, 15, 0));
            matrix.fillRect(47, 9, 17, 14, matrix.Color444(15, 15, 0));
            matrix.setCursor(15, 25);   // next line
            matrix.setTextSize(1);    // size 1 == 8 pixels high
            matrix.setTextColor(matrix.Color333(7, 7, 0));
            matrix.println("MOYEN");
            matrix.setTextColor(matrix.Color333(7, 7, 7));
            matrix.setCursor(0, 9);
            matrix.setTextSize(2);
            matrix.println(P25, 1);
          }
          return;
        }
      }
    }
  }
  if (Serial3.available() == 0)
  {
    matrix.setCursor(32, 15);   // next line
    matrix.setTextSize(1);    // size 1 == 8 pixels high
    matrix.setTextColor(matrix.Color333(7, 0, 0));
    matrix.println("Init");
    delay(5000);
    matrix.fillRect(0, 0, 64, 32, matrix.Color444(0, 0, 0));
  }
}

/********************************************ECRAN********************************************/

void LogoModuleAir()
{
  matrix.drawLine(1, 0, 7, 0, matrix.Color888(255, 255, 255));
  matrix.drawLine(0, 1, 6, 1, matrix.Color888(255, 255, 255));
  matrix.drawLine(11, 1, 12, 1, matrix.Color888(255, 255, 255));
  matrix.drawLine(4, 2, 6, 2, matrix.Color888(255, 255, 255));
  matrix.drawLine(10, 2, 13, 2, matrix.Color888(255, 255, 255));
  matrix.drawLine(3, 3, 6, 3, matrix.Color888(255, 255, 255));
  matrix.drawLine(9, 3, 13, 3, matrix.Color888(255, 255, 255));
  matrix.drawLine(17, 3, 18, 3, matrix.Color888(255, 255, 255));
  matrix.drawLine(3, 4, 6, 4, matrix.Color888(255, 255, 255));
  matrix.drawLine(8, 4, 12, 4, matrix.Color888(255, 255, 255));
  matrix.drawLine(16, 4, 19, 4, matrix.Color888(255, 255, 255));
  matrix.drawLine(3, 5, 5, 5, matrix.Color888(255, 255, 255));
  matrix.drawLine(8, 5, 12, 5, matrix.Color888(255, 255, 255));
  matrix.drawLine(15, 5, 19, 5, matrix.Color888(255, 255, 255));
  matrix.drawLine(37, 5, 38, 5, matrix.Color888(255, 255, 255));
  matrix.drawLine(52, 5, 53, 5, matrix.Color888(255, 255, 255));
  matrix.drawLine(2, 6, 5, 6, matrix.Color888(255, 255, 255));
  matrix.drawLine(7, 6, 11, 6, matrix.Color888(255, 255, 255));
  matrix.drawLine(14, 6, 19, 6, matrix.Color888(255, 255, 255));
  matrix.drawLine(36, 6, 38, 6, matrix.Color888(255, 255, 255));
  matrix.drawLine(51, 6, 53, 6, matrix.Color888(255, 255, 255));
  matrix.drawLine(2, 7, 11, 7, matrix.Color888(255, 255, 255));
  matrix.drawLine(13, 7, 18, 7, matrix.Color888(255, 255, 255));
  matrix.drawLine(36, 7, 38, 7, matrix.Color888(255, 255, 255));
  matrix.drawLine(51, 7, 53, 7, matrix.Color888(255, 255, 255));
  matrix.drawLine(2, 8, 18, 8, matrix.Color888(255, 255, 255));
  matrix.drawLine(36, 8, 37, 8, matrix.Color888(255, 255, 255));
  matrix.drawLine(51, 8, 52, 8, matrix.Color888(255, 255, 255));
  matrix.drawLine(1, 9, 6, 9, matrix.Color888(255, 255, 255));
  matrix.drawLine(8, 9, 13, 9, matrix.Color888(255, 255, 255));
  matrix.drawLine(15, 9, 17, 9, matrix.Color888(255, 255, 255));
  matrix.drawLine(23, 9, 37, 9, matrix.Color888(255, 255, 255));
  matrix.drawLine(41, 9, 43, 9, matrix.Color888(255, 255, 255));
  matrix.drawLine(45, 9, 47, 9, matrix.Color888(255, 255, 255));
  matrix.drawLine(50, 9, 52, 9, matrix.Color888(255, 255, 255));
  matrix.drawLine(56, 9, 60, 9, matrix.Color888(255, 255, 255));
  matrix.drawLine(1, 10, 5, 10, matrix.Color888(255, 255, 255));
  matrix.drawLine(8, 10, 12, 10, matrix.Color888(255, 255, 255));
  matrix.drawLine(15, 10, 17, 10, matrix.Color888(255, 255, 255));
  matrix.drawLine(22, 10, 23, 10, matrix.Color888(255, 255, 255));
  matrix.drawLine(26, 10, 27, 10, matrix.Color888(255, 255, 255));
  matrix.drawLine(31, 10, 32, 10, matrix.Color888(255, 255, 255));
  matrix.drawLine(35, 10, 37, 10, matrix.Color888(255, 255, 255));
  matrix.drawLine(41, 10, 42, 10, matrix.Color888(255, 255, 255));
  matrix.drawLine(45, 10, 46, 10, matrix.Color888(255, 255, 255));
  matrix.drawLine(50, 10, 52, 10, matrix.Color888(255, 255, 255));
  matrix.drawLine(55, 10, 57, 10, matrix.Color888(255, 255, 255));
  matrix.drawLine(59, 10, 61, 10, matrix.Color888(255, 255, 255));
  matrix.drawLine(1, 11, 4, 11, matrix.Color888(255, 255, 255));
  matrix.drawLine(8, 11, 11, 11, matrix.Color888(255, 255, 255));
  matrix.drawLine(14, 11, 17, 11, matrix.Color888(255, 255, 255));
  matrix.drawLine(22, 11, 23, 11, matrix.Color888(255, 255, 255));
  matrix.drawLine(26, 11, 27, 11, matrix.Color888(255, 255, 255));
  matrix.drawLine(30, 11, 32, 11, matrix.Color888(255, 255, 255));
  matrix.drawLine(35, 11, 36, 11, matrix.Color888(255, 255, 255));
  matrix.drawLine(40, 11, 42, 11, matrix.Color888(255, 255, 255));
  matrix.drawLine(44, 11, 46, 11, matrix.Color888(255, 255, 255));
  matrix.drawLine(50, 11, 51, 11, matrix.Color888(255, 255, 255));
  matrix.drawLine(55, 11, 60, 11, matrix.Color888(255, 255, 255));
  matrix.drawLine(0, 12, 3, 12, matrix.Color888(255, 255, 255));
  matrix.drawLine(8, 12, 10, 12, matrix.Color888(255, 255, 255));
  matrix.drawLine(14, 12, 23, 12, matrix.Color888(255, 255, 255));
  matrix.drawLine(25, 12, 27, 12, matrix.Color888(255, 255, 255));
  matrix.drawLine(30, 12, 32, 12, matrix.Color888(255, 255, 255));
  matrix.drawLine(34, 12, 36, 12, matrix.Color888(255, 255, 255));
  matrix.drawLine(39, 12, 42, 12, matrix.Color888(255, 255, 255));
  matrix.drawLine(44, 12, 46, 12, matrix.Color888(255, 255, 255));
  matrix.drawLine(49, 12, 51, 12, matrix.Color888(255, 255, 255));
  matrix.drawLine(54, 12, 57, 12, matrix.Color888(255, 255, 255));
  matrix.drawLine(0, 13, 2, 13, matrix.Color888(255, 255, 255));
  matrix.drawLine(8, 13, 9, 13, matrix.Color888(255, 255, 255));
  matrix.drawLine(14, 13, 20, 13, matrix.Color888(255, 255, 255));
  matrix.drawLine(22, 13, 26, 13, matrix.Color888(255, 255, 255));
  matrix.drawLine(31, 13, 38, 13, matrix.Color888(255, 255, 255));
  matrix.drawLine(40, 13, 43, 13, matrix.Color888(255, 255, 255));
  matrix.drawLine(45, 13, 48, 13, matrix.Color888(255, 255, 255));
  matrix.drawLine(50, 13, 53, 13, matrix.Color888(255, 255, 255));
  matrix.drawLine(55, 13, 62, 13, matrix.Color888(255, 255, 255));
  matrix.drawLine(45, 16, 45, 16, matrix.Color888(255, 0, 0));
  matrix.drawLine(15, 17, 18, 17, matrix.Color888(255, 255, 255));
  matrix.drawLine(37, 17, 38, 17, matrix.Color888(0, 255, 82));
  matrix.drawLine(40, 17, 41, 17, matrix.Color888(0, 255, 82));
  matrix.drawLine(43, 17, 47, 17, matrix.Color888(255, 0, 0));
  matrix.drawLine(59, 17, 60, 17, matrix.Color888(0, 106, 178));
  matrix.drawLine(14, 18, 18, 18, matrix.Color888(255, 255, 255));
  matrix.drawLine(22, 18, 23, 18, matrix.Color888(255, 255, 255));
  matrix.drawLine(37, 18, 41, 18, matrix.Color888(0, 255, 82));
  matrix.drawLine(43, 18, 47, 18, matrix.Color888(255, 0, 0));
  matrix.drawLine(60, 18, 61, 18, matrix.Color888(0, 106, 178));
  matrix.drawLine(13, 19, 14, 19, matrix.Color888(255, 255, 255));
  matrix.drawLine(16, 19, 17, 19, matrix.Color888(255, 255, 255));
  matrix.drawLine(21, 19, 24, 19, matrix.Color888(255, 255, 255));
  matrix.drawLine(36, 19, 42, 19, matrix.Color888(0, 255, 82));
  matrix.drawLine(44, 19, 48, 19, matrix.Color888(255, 0, 0));
  matrix.drawLine(57, 19, 58, 19, matrix.Color888(0, 106, 178));
  matrix.drawLine(61, 19, 62, 19, matrix.Color888(0, 106, 178));
  matrix.drawLine(13, 20, 13, 20, matrix.Color888(255, 255, 255));
  matrix.drawLine(15, 20, 17, 20, matrix.Color888(255, 255, 255));
  matrix.drawLine(21, 20, 24, 20, matrix.Color888(255, 255, 255));
  matrix.drawLine(37, 20, 41, 20, matrix.Color888(0, 255, 82));
  matrix.drawLine(43, 20, 47, 20, matrix.Color888(255, 0, 0));
  matrix.drawLine(58, 20, 59, 20, matrix.Color888(0, 106, 178));
  matrix.drawLine(62, 20, 62, 20, matrix.Color888(0, 106, 178));
  matrix.drawLine(12, 21, 13, 21, matrix.Color888(255, 255, 255));
  matrix.drawLine(15, 21, 17, 21, matrix.Color888(255, 255, 255));
  matrix.drawLine(22, 21, 23, 21, matrix.Color888(255, 255, 255));
  matrix.drawLine(27, 21, 28, 21, matrix.Color888(255, 255, 255));
  matrix.drawLine(37, 21, 38, 21, matrix.Color888(0, 255, 82));
  matrix.drawLine(40, 21, 41, 21, matrix.Color888(0, 255, 82));
  matrix.drawLine(43, 21, 44, 21, matrix.Color888(255, 0, 0));
  matrix.drawLine(46, 21, 47, 21, matrix.Color888(255, 0, 0));
  matrix.drawLine(55, 21, 56, 21, matrix.Color888(0, 106, 178));
  matrix.drawLine(59, 21, 59, 21, matrix.Color888(0, 106, 178));
  matrix.drawLine(62, 21, 63, 21, matrix.Color888(0, 106, 178));
  matrix.drawLine(11, 22, 12, 22, matrix.Color888(255, 255, 255));
  matrix.drawLine(15, 22, 16, 22, matrix.Color888(255, 255, 255));
  matrix.drawLine(26, 22, 28, 22, matrix.Color888(255, 255, 255));
  matrix.drawLine(39, 22, 39, 22, matrix.Color888(0, 0, 255));
  matrix.drawLine(52, 22, 53, 22, matrix.Color888(0, 106, 178));
  matrix.drawLine(56, 22, 56, 22, matrix.Color888(0, 106, 178));
  matrix.drawLine(59, 22, 60, 22, matrix.Color888(0, 106, 178));
  matrix.drawLine(63, 22, 63, 22, matrix.Color888(0, 106, 178));
  matrix.drawLine(5, 23, 23, 23, matrix.Color888(255, 255, 255));
  matrix.drawLine(26, 23, 32, 23, matrix.Color888(255, 255, 255));
  matrix.drawLine(37, 23, 41, 23, matrix.Color888(0, 0, 255));
  matrix.drawLine(46, 23, 46, 23, matrix.Color888(255, 255, 255));
  matrix.drawLine(51, 23, 54, 23, matrix.Color888(0, 106, 178));
  matrix.drawLine(56, 23, 57, 23, matrix.Color888(0, 106, 178));
  matrix.drawLine(60, 23, 60, 23, matrix.Color888(0, 106, 178));
  matrix.drawLine(63, 23, 63, 23, matrix.Color888(0, 106, 178));
  matrix.drawLine(2, 24, 22, 24, matrix.Color888(255, 255, 255));
  matrix.drawLine(27, 24, 32, 24, matrix.Color888(255, 255, 255));
  matrix.drawLine(37, 24, 41, 24, matrix.Color888(0, 0, 255));
  matrix.drawLine(44, 24, 48, 24, matrix.Color888(255, 255, 255));
  matrix.drawLine(52, 24, 53, 24, matrix.Color888(0, 106, 178));
  matrix.drawLine(56, 24, 56, 24, matrix.Color888(0, 106, 178));
  matrix.drawLine(59, 24, 60, 24, matrix.Color888(0, 106, 178));
  matrix.drawLine(63, 24, 63, 24, matrix.Color888(0, 106, 178));
  matrix.drawLine(1, 25, 4, 25, matrix.Color888(255, 255, 255));
  matrix.drawLine(8, 25, 9, 25, matrix.Color888(255, 255, 255));
  matrix.drawLine(14, 25, 15, 25, matrix.Color888(255, 255, 255));
  matrix.drawLine(20, 25, 22, 25, matrix.Color888(255, 255, 255));
  matrix.drawLine(27, 25, 27, 25, matrix.Color888(255, 255, 255));
  matrix.drawLine(30, 25, 32, 25, matrix.Color888(255, 255, 255));
  matrix.drawLine(38, 25, 40, 25, matrix.Color888(0, 0, 255));
  matrix.drawLine(44, 25, 48, 25, matrix.Color888(255, 255, 255));
  matrix.drawLine(55, 25, 56, 25, matrix.Color888(0, 106, 178));
  matrix.drawLine(59, 25, 59, 25, matrix.Color888(0, 106, 178));
  matrix.drawLine(62, 25, 63, 25, matrix.Color888(0, 106, 178));
  matrix.drawLine(0, 26, 3, 26, matrix.Color888(255, 255, 255));
  matrix.drawLine(7, 26, 8, 26, matrix.Color888(255, 255, 255));
  matrix.drawLine(13, 26, 15, 26, matrix.Color888(255, 255, 255));
  matrix.drawLine(20, 26, 22, 26, matrix.Color888(255, 255, 255));
  matrix.drawLine(26, 26, 27, 26, matrix.Color888(255, 255, 255));
  matrix.drawLine(30, 26, 31, 26, matrix.Color888(255, 255, 255));
  matrix.drawLine(37, 26, 41, 26, matrix.Color888(0, 0, 255));
  matrix.drawLine(43, 26, 49, 26, matrix.Color888(255, 255, 255));
  matrix.drawLine(58, 26, 59, 26, matrix.Color888(0, 106, 178));
  matrix.drawLine(62, 26, 62, 26, matrix.Color888(0, 106, 178));
  matrix.drawLine(0, 27, 3, 27, matrix.Color888(255, 255, 255));
  matrix.drawLine(6, 27, 7, 27, matrix.Color888(255, 255, 255));
  matrix.drawLine(13, 27, 15, 27, matrix.Color888(255, 255, 255));
  matrix.drawLine(20, 27, 21, 27, matrix.Color888(255, 255, 255));
  matrix.drawLine(25, 27, 26, 27, matrix.Color888(255, 255, 255));
  matrix.drawLine(30, 27, 31, 27, matrix.Color888(255, 255, 255));
  matrix.drawLine(37, 27, 38, 27, matrix.Color888(0, 0, 255));
  matrix.drawLine(40, 27, 41, 27, matrix.Color888(0, 0, 255));
  matrix.drawLine(44, 27, 48, 27, matrix.Color888(255, 255, 255));
  matrix.drawLine(57, 27, 58, 27, matrix.Color888(0, 106, 178));
  matrix.drawLine(61, 27, 62, 27, matrix.Color888(0, 106, 178));
  matrix.drawLine(0, 28, 6, 28, matrix.Color888(255, 255, 255));
  matrix.drawLine(12, 28, 14, 28, matrix.Color888(255, 255, 255));
  matrix.drawLine(20, 28, 25, 28, matrix.Color888(255, 255, 255));
  matrix.drawLine(30, 28, 34, 28, matrix.Color888(255, 255, 255));
  matrix.drawLine(44, 28, 45, 28, matrix.Color888(255, 255, 255));
  matrix.drawLine(47, 28, 48, 28, matrix.Color888(255, 255, 255));
  matrix.drawLine(60, 28, 61, 28, matrix.Color888(0, 106, 178));
  matrix.drawLine(2, 29, 4, 29, matrix.Color888(255, 255, 255));
  matrix.drawLine(59, 29, 60, 29, matrix.Color888(0, 106, 178));
  matrix.drawLine(59, 29, 60, 29, matrix.Color888(0, 106, 178));
}

void LogoAtmoSud()
{
  matrix.drawLine(40, 0, 44, 0, matrix.Color888(41, 179, 232));
  matrix.drawLine(62, 0, 62, 0, matrix.Color888(41, 179, 232));
  matrix.drawLine(5, 1, 5, 1, matrix.Color888(44, 91, 166));
  matrix.drawLine(39, 1, 40, 1, matrix.Color888(41, 179, 232));
  matrix.drawLine(44, 1, 45, 1, matrix.Color888(41, 179, 232));
  matrix.drawLine(62, 1, 62, 1, matrix.Color888(41, 179, 232));
  matrix.drawLine(4, 2, 5, 2, matrix.Color888(44, 91, 166));
  matrix.drawLine(11, 2, 11, 2, matrix.Color888(44, 91, 166));
  matrix.drawLine(39, 2, 39, 2, matrix.Color888(41, 179, 232));
  matrix.drawLine(45, 2, 45, 2, matrix.Color888(41, 179, 232));
  matrix.drawLine(62, 2, 62, 2, matrix.Color888(41, 179, 232));
  matrix.drawLine(4, 3, 6, 3, matrix.Color888(44, 91, 166));
  matrix.drawLine(11, 3, 12, 3, matrix.Color888(44, 91, 166));
  matrix.drawLine(15, 3, 15, 3, matrix.Color888(44, 91, 166));
  matrix.drawLine(17, 3, 18, 3, matrix.Color888(44, 91, 166));
  matrix.drawLine(22, 3, 23, 3, matrix.Color888(44, 91, 166));
  matrix.drawLine(31, 3, 31, 3, matrix.Color888(44, 91, 166));
  matrix.drawLine(39, 3, 39, 3, matrix.Color888(41, 179, 232));
  matrix.drawLine(48, 3, 48, 3, matrix.Color888(41, 179, 232));
  matrix.drawLine(53, 3, 53, 3, matrix.Color888(41, 179, 232));
  matrix.drawLine(58, 3, 58, 3, matrix.Color888(41, 179, 232));
  matrix.drawLine(62, 3, 62, 3, matrix.Color888(41, 179, 232));
  matrix.drawLine(3, 4, 6, 4, matrix.Color888(44, 91, 166));
  matrix.drawLine(11, 4, 13, 4, matrix.Color888(44, 91, 166));
  matrix.drawLine(15, 4, 19, 4, matrix.Color888(44, 91, 166));
  matrix.drawLine(21, 4, 24, 4, matrix.Color888(44, 91, 166));
  matrix.drawLine(29, 4, 33, 4, matrix.Color888(44, 91, 166));
  matrix.drawLine(39, 4, 40, 4, matrix.Color888(41, 179, 232));
  matrix.drawLine(48, 4, 48, 4, matrix.Color888(41, 179, 232));
  matrix.drawLine(53, 4, 53, 4, matrix.Color888(41, 179, 232));
  matrix.drawLine(57, 4, 60, 4, matrix.Color888(41, 179, 232));
  matrix.drawLine(62, 4, 62, 4, matrix.Color888(41, 179, 232));
  matrix.drawLine(3, 5, 4, 5, matrix.Color888(44, 91, 166));
  matrix.drawLine(6, 5, 7, 5, matrix.Color888(44, 91, 166));
  matrix.drawLine(11, 5, 11, 5, matrix.Color888(44, 91, 166));
  matrix.drawLine(15, 5, 16, 5, matrix.Color888(44, 91, 166));
  matrix.drawLine(19, 5, 21, 5, matrix.Color888(44, 91, 166));
  matrix.drawLine(24, 5, 25, 5, matrix.Color888(44, 91, 166));
  matrix.drawLine(28, 5, 29, 5, matrix.Color888(44, 91, 166));
  matrix.drawLine(33, 5, 34, 5, matrix.Color888(44, 91, 166));
  matrix.drawLine(40, 5, 44, 5, matrix.Color888(41, 179, 232));
  matrix.drawLine(48, 5, 48, 5, matrix.Color888(41, 179, 232));
  matrix.drawLine(53, 5, 53, 5, matrix.Color888(41, 179, 232));
  matrix.drawLine(56, 5, 56, 5, matrix.Color888(41, 179, 232));
  matrix.drawLine(61, 5, 62, 5, matrix.Color888(41, 179, 232));
  matrix.drawLine(3, 6, 3, 6, matrix.Color888(44, 91, 166));
  matrix.drawLine(6, 6, 7, 6, matrix.Color888(44, 91, 166));
  matrix.drawLine(11, 6, 11, 6, matrix.Color888(44, 91, 166));
  matrix.drawLine(15, 6, 15, 6, matrix.Color888(44, 91, 166));
  matrix.drawLine(20, 6, 20, 6, matrix.Color888(44, 91, 166));
  matrix.drawLine(25, 6, 25, 6, matrix.Color888(44, 91, 166));
  matrix.drawLine(28, 6, 28, 6, matrix.Color888(44, 91, 166));
  matrix.drawLine(34, 6, 34, 6, matrix.Color888(44, 91, 166));
  matrix.drawLine(44, 6, 45, 6, matrix.Color888(41, 179, 232));
  matrix.drawLine(48, 6, 48, 6, matrix.Color888(41, 179, 232));
  matrix.drawLine(53, 6, 53, 6, matrix.Color888(41, 179, 232));
  matrix.drawLine(55, 6, 56, 6, matrix.Color888(41, 179, 232));
  matrix.drawLine(62, 6, 62, 6, matrix.Color888(41, 179, 232));
  matrix.drawLine(2, 7, 3, 7, matrix.Color888(44, 91, 166));
  matrix.drawLine(6, 7, 7, 7, matrix.Color888(44, 91, 166));
  matrix.drawLine(11, 7, 11, 7, matrix.Color888(44, 91, 166));
  matrix.drawLine(15, 7, 15, 7, matrix.Color888(44, 91, 166));
  matrix.drawLine(20, 7, 20, 7, matrix.Color888(44, 91, 166));
  matrix.drawLine(25, 7, 25, 7, matrix.Color888(44, 91, 166));
  matrix.drawLine(27, 7, 28, 7, matrix.Color888(44, 91, 166));
  matrix.drawLine(34, 7, 35, 7, matrix.Color888(44, 91, 166));
  matrix.drawLine(45, 7, 45, 7, matrix.Color888(41, 179, 232));
  matrix.drawLine(48, 7, 48, 7, matrix.Color888(41, 179, 232));
  matrix.drawLine(53, 7, 53, 7, matrix.Color888(41, 179, 232));
  matrix.drawLine(55, 7, 55, 7, matrix.Color888(41, 179, 232));
  matrix.drawLine(62, 7, 62, 7, matrix.Color888(41, 179, 232));
  matrix.drawLine(2, 8, 7, 8, matrix.Color888(44, 91, 166));
  matrix.drawLine(11, 8, 11, 8, matrix.Color888(44, 91, 166));
  matrix.drawLine(15, 8, 15, 8, matrix.Color888(44, 91, 166));
  matrix.drawLine(20, 8, 20, 8, matrix.Color888(44, 91, 166));
  matrix.drawLine(25, 8, 25, 8, matrix.Color888(44, 91, 166));
  matrix.drawLine(28, 8, 28, 8, matrix.Color888(44, 91, 166));
  matrix.drawLine(34, 8, 34, 8, matrix.Color888(44, 91, 166));
  matrix.drawLine(39, 8, 39, 8, matrix.Color888(41, 179, 232));
  matrix.drawLine(45, 8, 45, 8, matrix.Color888(41, 179, 232));
  matrix.drawLine(48, 8, 48, 8, matrix.Color888(41, 179, 232));
  matrix.drawLine(53, 8, 53, 8, matrix.Color888(41, 179, 232));
  matrix.drawLine(55, 8, 56, 8, matrix.Color888(41, 179, 232));
  matrix.drawLine(62, 8, 62, 8, matrix.Color888(41, 179, 232));
  matrix.drawLine(1, 9, 2, 9, matrix.Color888(44, 91, 166));
  matrix.drawLine(7, 9, 8, 9, matrix.Color888(44, 91, 166));
  matrix.drawLine(11, 9, 12, 9, matrix.Color888(44, 91, 166));
  matrix.drawLine(15, 9, 15, 9, matrix.Color888(44, 91, 166));
  matrix.drawLine(20, 9, 20, 9, matrix.Color888(44, 91, 166));
  matrix.drawLine(25, 9, 25, 9, matrix.Color888(44, 91, 166));
  matrix.drawLine(28, 9, 29, 9, matrix.Color888(44, 91, 166));
  matrix.drawLine(33, 9, 34, 9, matrix.Color888(44, 91, 166));
  matrix.drawLine(39, 9, 40, 9, matrix.Color888(41, 179, 232));
  matrix.drawLine(44, 9, 45, 9, matrix.Color888(41, 179, 232));
  matrix.drawLine(48, 9, 49, 9, matrix.Color888(41, 179, 232));
  matrix.drawLine(53, 9, 53, 9, matrix.Color888(41, 179, 232));
  matrix.drawLine(56, 9, 56, 9, matrix.Color888(41, 179, 232));
  matrix.drawLine(61, 9, 62, 9, matrix.Color888(41, 179, 232));
  matrix.drawLine(1, 10, 2, 10, matrix.Color888(44, 91, 166));
  matrix.drawLine(7, 10, 8, 10, matrix.Color888(44, 91, 166));
  matrix.drawLine(11, 10, 13, 10, matrix.Color888(44, 91, 166));
  matrix.drawLine(15, 10, 15, 10, matrix.Color888(44, 91, 166));
  matrix.drawLine(20, 10, 20, 10, matrix.Color888(44, 91, 166));
  matrix.drawLine(25, 10, 25, 10, matrix.Color888(44, 91, 166));
  matrix.drawLine(29, 10, 33, 10, matrix.Color888(44, 91, 166));
  matrix.drawLine(40, 10, 44, 10, matrix.Color888(41, 179, 232));
  matrix.drawLine(49, 10, 53, 10, matrix.Color888(41, 179, 232));
  matrix.drawLine(57, 10, 60, 10, matrix.Color888(41, 179, 232));
  matrix.drawLine(62, 10, 62, 10, matrix.Color888(41, 179, 232));
  matrix.drawLine(10, 14, 16, 14, matrix.Color888(248, 187, 11));
  matrix.drawLine(7, 15, 24, 15, matrix.Color888(248, 187, 11));
  matrix.drawLine(4, 16, 34, 16, matrix.Color888(248, 187, 11));
  matrix.drawLine(1, 17, 8, 17, matrix.Color888(248, 187, 11));
  matrix.drawLine(22, 17, 47, 17, matrix.Color888(248, 187, 11));
  matrix.drawLine(58, 17, 63, 17, matrix.Color888(248, 187, 11));
  matrix.drawLine(0, 18, 2, 18, matrix.Color888(248, 187, 11));
  matrix.drawLine(26, 18, 59, 18, matrix.Color888(248, 187, 11));
  matrix.drawLine(33, 19, 54, 19, matrix.Color888(248, 187, 11));
  matrix.drawLine(46, 20, 51, 20, matrix.Color888(248, 187, 11));
  matrix.drawLine(38, 22, 38, 22, matrix.Color888(43, 99, 174));
  matrix.drawLine(2, 23, 5, 23, matrix.Color888(44, 91, 166));
  matrix.drawLine(20, 23, 20, 23, matrix.Color888(43, 99, 174));
  matrix.drawLine(22, 23, 22, 23, matrix.Color888(43, 99, 174));
  matrix.drawLine(30, 23, 30, 23, matrix.Color888(43, 99, 174));
  matrix.drawLine(38, 23, 38, 23, matrix.Color888(43, 99, 174));
  matrix.drawLine(51, 23, 51, 23, matrix.Color888(43, 99, 174));
  matrix.drawLine(59, 23, 59, 23, matrix.Color888(43, 99, 174));
  matrix.drawLine(1, 24, 1, 24, matrix.Color888(43, 99, 174));
  matrix.drawLine(6, 24, 6, 24, matrix.Color888(43, 99, 174));
  matrix.drawLine(20, 24, 20, 24, matrix.Color888(43, 99, 174));
  matrix.drawLine(24, 24, 24, 24, matrix.Color888(44, 91, 166));
  matrix.drawLine(29, 24, 29, 24, matrix.Color888(43, 99, 174));
  matrix.drawLine(38, 24, 38, 24, matrix.Color888(43, 99, 174));
  matrix.drawLine(48, 24, 48, 24, matrix.Color888(43, 99, 174));
  matrix.drawLine(50, 24, 50, 24, matrix.Color888(43, 99, 174));
  matrix.drawLine(54, 24, 54, 24, matrix.Color888(43, 99, 174));
  matrix.drawLine(0, 25, 0, 25, matrix.Color888(44, 91, 166));
  matrix.drawLine(7, 25, 7, 25, matrix.Color888(43, 99, 174));
  matrix.drawLine(9, 25, 9, 25, matrix.Color888(44, 91, 166));
  matrix.drawLine(12, 25, 12, 25, matrix.Color888(44, 91, 166));
  matrix.drawLine(15, 25, 18, 25, matrix.Color888(44, 91, 166));
  matrix.drawLine(20, 25, 20, 25, matrix.Color888(43, 99, 174));
  matrix.drawLine(22, 25, 22, 25, matrix.Color888(43, 99, 174));
  matrix.drawLine(24, 25, 25, 25, matrix.Color888(44, 91, 166));
  matrix.drawLine(28, 25, 30, 25, matrix.Color888(43, 99, 174));
  matrix.drawLine(36, 25, 38, 25, matrix.Color888(43, 99, 174));
  matrix.drawLine(41, 25, 43, 25, matrix.Color888(43, 99, 174));
  matrix.drawLine(48, 25, 48, 25, matrix.Color888(43, 99, 174));
  matrix.drawLine(53, 25, 55, 25, matrix.Color888(43, 99, 174));
  matrix.drawLine(59, 25, 59, 25, matrix.Color888(43, 99, 174));
  matrix.drawLine(62, 25, 63, 25, matrix.Color888(43, 99, 174));
  matrix.drawLine(0, 26, 0, 26, matrix.Color888(44, 91, 166));
  matrix.drawLine(7, 26, 7, 26, matrix.Color888(43, 99, 174));
  matrix.drawLine(9, 26, 9, 26, matrix.Color888(44, 91, 166));
  matrix.drawLine(12, 26, 12, 26, matrix.Color888(44, 91, 166));
  matrix.drawLine(14, 26, 14, 26, matrix.Color888(43, 99, 174));
  matrix.drawLine(18, 26, 18, 26, matrix.Color888(44, 91, 166));
  matrix.drawLine(20, 26, 20, 26, matrix.Color888(43, 99, 174));
  matrix.drawLine(22, 26, 22, 26, matrix.Color888(43, 99, 174));
  matrix.drawLine(24, 26, 24, 26, matrix.Color888(44, 91, 166));
  matrix.drawLine(27, 26, 27, 26, matrix.Color888(43, 99, 174));
  matrix.drawLine(30, 26, 30, 26, matrix.Color888(43, 99, 174));
  matrix.drawLine(35, 26, 35, 26, matrix.Color888(43, 99, 174));
  matrix.drawLine(38, 26, 38, 26, matrix.Color888(43, 99, 174));
  matrix.drawLine(40, 26, 40, 26, matrix.Color888(43, 99, 174));
  matrix.drawLine(43, 26, 43, 26, matrix.Color888(43, 99, 174));
  matrix.drawLine(48, 26, 48, 26, matrix.Color888(43, 99, 174));
  matrix.drawLine(53, 26, 53, 26, matrix.Color888(43, 99, 174));
  matrix.drawLine(55, 26, 55, 26, matrix.Color888(43, 99, 174));
  matrix.drawLine(59, 26, 59, 26, matrix.Color888(43, 99, 174));
  matrix.drawLine(61, 26, 61, 26, matrix.Color888(43, 99, 174));
  matrix.drawLine(0, 27, 0, 27, matrix.Color888(44, 91, 166));
  matrix.drawLine(7, 27, 7, 27, matrix.Color888(43, 99, 174));
  matrix.drawLine(9, 27, 9, 27, matrix.Color888(44, 91, 166));
  matrix.drawLine(12, 27, 12, 27, matrix.Color888(44, 91, 166));
  matrix.drawLine(14, 27, 14, 27, matrix.Color888(43, 99, 174));
  matrix.drawLine(18, 27, 18, 27, matrix.Color888(44, 91, 166));
  matrix.drawLine(20, 27, 20, 27, matrix.Color888(43, 99, 174));
  matrix.drawLine(22, 27, 22, 27, matrix.Color888(43, 99, 174));
  matrix.drawLine(24, 27, 24, 27, matrix.Color888(44, 91, 166));
  matrix.drawLine(27, 27, 30, 27, matrix.Color888(43, 99, 174));
  matrix.drawLine(35, 27, 35, 27, matrix.Color888(43, 99, 174));
  matrix.drawLine(38, 27, 38, 27, matrix.Color888(43, 99, 174));
  matrix.drawLine(40, 27, 43, 27, matrix.Color888(43, 99, 174));
  matrix.drawLine(48, 27, 48, 27, matrix.Color888(43, 99, 174));
  matrix.drawLine(52, 27, 52, 27, matrix.Color888(43, 99, 174));
  matrix.drawLine(56, 27, 56, 27, matrix.Color888(43, 99, 174));
  matrix.drawLine(59, 27, 59, 27, matrix.Color888(43, 99, 174));
  matrix.drawLine(61, 27, 61, 27, matrix.Color888(43, 99, 174));
  matrix.drawLine(1, 28, 1, 28, matrix.Color888(43, 99, 174));
  matrix.drawLine(6, 28, 6, 28, matrix.Color888(43, 99, 174));
  matrix.drawLine(9, 28, 9, 28, matrix.Color888(44, 91, 166));
  matrix.drawLine(12, 28, 12, 28, matrix.Color888(44, 91, 166));
  matrix.drawLine(14, 28, 14, 28, matrix.Color888(43, 99, 174));
  matrix.drawLine(18, 28, 18, 28, matrix.Color888(44, 91, 166));
  matrix.drawLine(20, 28, 20, 28, matrix.Color888(43, 99, 174));
  matrix.drawLine(22, 28, 22, 28, matrix.Color888(43, 99, 174));
  matrix.drawLine(24, 28, 24, 28, matrix.Color888(44, 91, 166));
  matrix.drawLine(27, 28, 27, 28, matrix.Color888(43, 99, 174));
  matrix.drawLine(35, 28, 35, 28, matrix.Color888(43, 99, 174));
  matrix.drawLine(38, 28, 38, 28, matrix.Color888(43, 99, 174));
  matrix.drawLine(40, 28, 40, 28, matrix.Color888(43, 99, 174));
  matrix.drawLine(48, 28, 48, 28, matrix.Color888(43, 99, 174));
  matrix.drawLine(52, 28, 56, 28, matrix.Color888(43, 99, 174));
  matrix.drawLine(59, 28, 59, 28, matrix.Color888(43, 99, 174));
  matrix.drawLine(61, 28, 61, 28, matrix.Color888(43, 99, 174));
  matrix.drawLine(2, 29, 5, 29, matrix.Color888(43, 99, 174));
  matrix.drawLine(9, 29, 12, 29, matrix.Color888(44, 91, 166));
  matrix.drawLine(15, 29, 18, 29, matrix.Color888(44, 91, 166));
  matrix.drawLine(20, 29, 20, 29, matrix.Color888(43, 99, 174));
  matrix.drawLine(22, 29, 22, 29, matrix.Color888(43, 99, 174));
  matrix.drawLine(24, 29, 24, 29, matrix.Color888(44, 91, 166));
  matrix.drawLine(27, 29, 30, 29, matrix.Color888(43, 99, 174));
  matrix.drawLine(35, 29, 38, 29, matrix.Color888(43, 99, 174));
  matrix.drawLine(40, 29, 43, 29, matrix.Color888(43, 99, 174));
  matrix.drawLine(48, 29, 48, 29, matrix.Color888(43, 99, 174));
  matrix.drawLine(51, 29, 51, 29, matrix.Color888(43, 99, 174));
  matrix.drawLine(57, 29, 57, 29, matrix.Color888(43, 99, 174));
  matrix.drawLine(59, 29, 59, 29, matrix.Color888(43, 99, 174));
  matrix.drawLine(61, 29, 61, 29, matrix.Color888(43, 99, 174));
  matrix.drawLine(4, 30, 4, 30, matrix.Color888(43, 99, 174));
  matrix.drawLine(5, 31, 5, 31, matrix.Color888(43, 99, 174));
  matrix.drawLine(5, 31, 5, 31, matrix.Color888(43, 99, 174));
}

void RegionSud()
{
  matrix.drawLine(0, 0, 9, 0, matrix.Color888(0, 0, 0));
  matrix.drawLine(10, 0, 10, 0, matrix.Color888(251, 175, 23));
  matrix.drawLine(11, 0, 11, 0, matrix.Color888(252, 175, 23));
  matrix.drawLine(12, 0, 63, 0, matrix.Color888(0, 0, 0));
  matrix.drawLine(0, 1, 7, 1, matrix.Color888(0, 0, 0));
  matrix.drawLine(8, 1, 11, 1, matrix.Color888(252, 175, 23));
  matrix.drawLine(12, 1, 38, 1, matrix.Color888(0, 0, 0));
  matrix.drawLine(39, 1, 39, 1, matrix.Color888(248, 174, 1));
  matrix.drawLine(40, 1, 41, 1, matrix.Color888(230, 0, 5));
  matrix.drawLine(42, 1, 42, 1, matrix.Color888(248, 174, 1));
  matrix.drawLine(43, 1, 44, 1, matrix.Color888(230, 0, 5));
  matrix.drawLine(45, 1, 45, 1, matrix.Color888(248, 174, 1));
  matrix.drawLine(46, 1, 47, 1, matrix.Color888(230, 0, 5));
  matrix.drawLine(48, 1, 48, 1, matrix.Color888(248, 174, 1));
  matrix.drawLine(49, 1, 50, 1, matrix.Color888(230, 0, 5));
  matrix.drawLine(51, 1, 56, 1, matrix.Color888(248, 174, 1));
  matrix.drawLine(57, 1, 57, 1, matrix.Color888(230, 0, 5));
  matrix.drawLine(58, 1, 63, 1, matrix.Color888(248, 174, 1));
  matrix.drawLine(0, 2, 7, 2, matrix.Color888(0, 0, 0));
  matrix.drawLine(13, 2, 38, 2, matrix.Color888(0, 0, 0));
  matrix.drawLine(39, 2, 39, 2, matrix.Color888(248, 174, 1));
  matrix.drawLine(40, 2, 41, 2, matrix.Color888(230, 0, 5));
  matrix.drawLine(42, 2, 42, 2, matrix.Color888(248, 174, 1));
  matrix.drawLine(43, 2, 44, 2, matrix.Color888(230, 0, 5));
  matrix.drawLine(45, 2, 45, 2, matrix.Color888(248, 174, 1));
  matrix.drawLine(46, 2, 47, 2, matrix.Color888(230, 0, 5));
  matrix.drawLine(48, 2, 48, 2, matrix.Color888(248, 174, 1));
  matrix.drawLine(49, 2, 50, 2, matrix.Color888(230, 0, 5));
  matrix.drawLine(51, 2, 54, 2, matrix.Color888(248, 174, 1));
  matrix.drawLine(55, 2, 57, 2, matrix.Color888(0, 71, 129));
  matrix.drawLine(58, 2, 58, 2, matrix.Color888(230, 0, 5));
  matrix.drawLine(59, 2, 63, 2, matrix.Color888(248, 174, 1));
  matrix.drawLine(0, 3, 4, 3, matrix.Color888(255, 255, 255));
  matrix.drawLine(5, 3, 6, 3, matrix.Color888(0, 0, 0));
  matrix.drawLine(7, 3, 11, 3, matrix.Color888(255, 255, 255));
  matrix.drawLine(12, 3, 13, 3, matrix.Color888(0, 0, 0));
  matrix.drawLine(14, 3, 18, 3, matrix.Color888(255, 255, 255));
  matrix.drawLine(19, 3, 19, 3, matrix.Color888(0, 0, 0));
  matrix.drawLine(20, 3, 21, 3, matrix.Color888(255, 255, 255));
  matrix.drawLine(22, 3, 23, 3, matrix.Color888(0, 0, 0));
  matrix.drawLine(25, 3, 27, 3, matrix.Color888(255, 255, 255));
  matrix.drawLine(29, 3, 30, 3, matrix.Color888(0, 0, 0));
  matrix.drawLine(31, 3, 32, 3, matrix.Color888(255, 255, 255));
  matrix.drawLine(33, 3, 35, 3, matrix.Color888(0, 0, 0));
  matrix.drawLine(36, 3, 37, 3, matrix.Color888(255, 255, 255));
  matrix.drawLine(38, 3, 38, 3, matrix.Color888(0, 0, 0));
  matrix.drawLine(39, 3, 39, 3, matrix.Color888(248, 174, 1));
  matrix.drawLine(40, 3, 41, 3, matrix.Color888(230, 0, 5));
  matrix.drawLine(42, 3, 42, 3, matrix.Color888(248, 174, 1));
  matrix.drawLine(43, 3, 44, 3, matrix.Color888(230, 0, 5));
  matrix.drawLine(45, 3, 45, 3, matrix.Color888(248, 174, 1));
  matrix.drawLine(46, 3, 47, 3, matrix.Color888(230, 0, 5));
  matrix.drawLine(48, 3, 48, 3, matrix.Color888(248, 174, 1));
  matrix.drawLine(49, 3, 50, 3, matrix.Color888(230, 0, 5));
  matrix.drawLine(51, 3, 53, 3, matrix.Color888(248, 174, 1));
  matrix.drawLine(54, 3, 58, 3, matrix.Color888(0, 71, 129));
  matrix.drawLine(59, 3, 59, 3, matrix.Color888(230, 0, 5));
  matrix.drawLine(60, 3, 63, 3, matrix.Color888(248, 174, 1));
  matrix.drawLine(0, 4, 5, 4, matrix.Color888(255, 255, 255));
  matrix.drawLine(6, 4, 6, 4, matrix.Color888(0, 0, 0));
  matrix.drawLine(7, 4, 11, 4, matrix.Color888(255, 255, 255));
  matrix.drawLine(12, 4, 12, 4, matrix.Color888(0, 0, 0));
  matrix.drawLine(13, 4, 18, 4, matrix.Color888(255, 255, 255));
  matrix.drawLine(19, 4, 19, 4, matrix.Color888(0, 0, 0));
  matrix.drawLine(20, 4, 21, 4, matrix.Color888(255, 255, 255));
  matrix.drawLine(22, 4, 23, 4, matrix.Color888(0, 0, 0));
  matrix.drawLine(24, 4, 28, 4, matrix.Color888(255, 255, 255));
  matrix.drawLine(29, 4, 30, 4, matrix.Color888(0, 0, 0));
  matrix.drawLine(31, 4, 33, 4, matrix.Color888(255, 255, 255));
  matrix.drawLine(34, 4, 35, 4, matrix.Color888(0, 0, 0));
  matrix.drawLine(36, 4, 37, 4, matrix.Color888(255, 255, 255));
  matrix.drawLine(38, 4, 38, 4, matrix.Color888(0, 0, 0));
  matrix.drawLine(39, 4, 39, 4, matrix.Color888(248, 174, 1));
  matrix.drawLine(40, 4, 41, 4, matrix.Color888(230, 0, 5));
  matrix.drawLine(42, 4, 42, 4, matrix.Color888(248, 174, 1));
  matrix.drawLine(43, 4, 44, 4, matrix.Color888(230, 0, 5));
  matrix.drawLine(45, 4, 45, 4, matrix.Color888(248, 174, 1));
  matrix.drawLine(46, 4, 47, 4, matrix.Color888(230, 0, 5));
  matrix.drawLine(48, 4, 48, 4, matrix.Color888(248, 174, 1));
  matrix.drawLine(49, 4, 50, 4, matrix.Color888(230, 0, 5));
  matrix.drawLine(51, 4, 56, 4, matrix.Color888(248, 174, 1));
  matrix.drawLine(57, 4, 57, 4, matrix.Color888(230, 0, 5));
  matrix.drawLine(58, 4, 59, 4, matrix.Color888(0, 71, 129));
  matrix.drawLine(60, 4, 63, 4, matrix.Color888(248, 174, 1));
  matrix.drawLine(0, 5, 1, 5, matrix.Color888(255, 255, 255));
  matrix.drawLine(2, 5, 3, 5, matrix.Color888(0, 0, 0));
  matrix.drawLine(4, 5, 5, 5, matrix.Color888(255, 255, 255));
  matrix.drawLine(6, 5, 6, 5, matrix.Color888(0, 0, 0));
  matrix.drawLine(7, 5, 8, 5, matrix.Color888(255, 255, 255));
  matrix.drawLine(9, 5, 12, 5, matrix.Color888(0, 0, 0));
  matrix.drawLine(13, 5, 14, 5, matrix.Color888(255, 255, 255));
  matrix.drawLine(15, 5, 19, 5, matrix.Color888(0, 0, 0));
  matrix.drawLine(20, 5, 21, 5, matrix.Color888(255, 255, 255));
  matrix.drawLine(22, 5, 22, 5, matrix.Color888(0, 0, 0));
  matrix.drawLine(23, 5, 24, 5, matrix.Color888(255, 255, 255));
  matrix.drawLine(25, 5, 27, 5, matrix.Color888(0, 0, 0));
  matrix.drawLine(28, 5, 29, 5, matrix.Color888(255, 255, 255));
  matrix.drawLine(30, 5, 30, 5, matrix.Color888(0, 0, 0));
  matrix.drawLine(31, 5, 34, 5, matrix.Color888(255, 255, 255));
  matrix.drawLine(35, 5, 35, 5, matrix.Color888(0, 0, 0));
  matrix.drawLine(36, 5, 37, 5, matrix.Color888(255, 255, 255));
  matrix.drawLine(38, 5, 38, 5, matrix.Color888(0, 0, 0));
  matrix.drawLine(39, 5, 39, 5, matrix.Color888(248, 174, 1));
  matrix.drawLine(40, 5, 41, 5, matrix.Color888(230, 0, 5));
  matrix.drawLine(42, 5, 42, 5, matrix.Color888(248, 174, 1));
  matrix.drawLine(43, 5, 44, 5, matrix.Color888(230, 0, 5));
  matrix.drawLine(45, 5, 45, 5, matrix.Color888(248, 174, 1));
  matrix.drawLine(46, 5, 47, 5, matrix.Color888(230, 0, 5));
  matrix.drawLine(48, 5, 48, 5, matrix.Color888(248, 174, 1));
  matrix.drawLine(49, 5, 50, 5, matrix.Color888(230, 0, 5));
  matrix.drawLine(51, 5, 57, 5, matrix.Color888(248, 174, 1));
  matrix.drawLine(58, 5, 59, 5, matrix.Color888(0, 71, 129));
  matrix.drawLine(60, 5, 63, 5, matrix.Color888(248, 174, 1));
  matrix.drawLine(0, 6, 1, 6, matrix.Color888(255, 255, 255));
  matrix.drawLine(2, 6, 3, 6, matrix.Color888(0, 0, 0));
  matrix.drawLine(4, 6, 5, 6, matrix.Color888(255, 255, 255));
  matrix.drawLine(6, 6, 6, 6, matrix.Color888(0, 0, 0));
  matrix.drawLine(7, 6, 10, 6, matrix.Color888(255, 255, 255));
  matrix.drawLine(11, 6, 12, 6, matrix.Color888(0, 0, 0));
  matrix.drawLine(13, 6, 14, 6, matrix.Color888(255, 255, 255));
  matrix.drawLine(15, 6, 19, 6, matrix.Color888(0, 0, 0));
  matrix.drawLine(20, 6, 21, 6, matrix.Color888(255, 255, 255));
  matrix.drawLine(22, 6, 22, 6, matrix.Color888(0, 0, 0));
  matrix.drawLine(23, 6, 24, 6, matrix.Color888(255, 255, 255));
  matrix.drawLine(25, 6, 27, 6, matrix.Color888(0, 0, 0));
  matrix.drawLine(28, 6, 29, 6, matrix.Color888(255, 255, 255));
  matrix.drawLine(30, 6, 30, 6, matrix.Color888(0, 0, 0));
  matrix.drawLine(31, 6, 37, 6, matrix.Color888(255, 255, 255));
  matrix.drawLine(38, 6, 38, 6, matrix.Color888(0, 0, 0));
  matrix.drawLine(39, 6, 39, 6, matrix.Color888(248, 174, 1));
  matrix.drawLine(40, 6, 41, 6, matrix.Color888(230, 0, 5));
  matrix.drawLine(42, 6, 42, 6, matrix.Color888(248, 174, 1));
  matrix.drawLine(43, 6, 44, 6, matrix.Color888(230, 0, 5));
  matrix.drawLine(45, 6, 45, 6, matrix.Color888(248, 174, 1));
  matrix.drawLine(46, 6, 47, 6, matrix.Color888(230, 0, 5));
  matrix.drawLine(48, 6, 48, 6, matrix.Color888(248, 174, 1));
  matrix.drawLine(49, 6, 50, 6, matrix.Color888(230, 0, 5));
  matrix.drawLine(51, 6, 57, 6, matrix.Color888(248, 174, 1));
  matrix.drawLine(58, 6, 59, 6, matrix.Color888(0, 71, 129));
  matrix.drawLine(60, 6, 63, 6, matrix.Color888(248, 174, 1));
  matrix.drawLine(0, 7, 4, 7, matrix.Color888(255, 255, 255));
  matrix.drawLine(5, 7, 6, 7, matrix.Color888(0, 0, 0));
  matrix.drawLine(7, 7, 10, 7, matrix.Color888(255, 255, 255));
  matrix.drawLine(11, 7, 12, 7, matrix.Color888(0, 0, 0));
  matrix.drawLine(13, 7, 14, 7, matrix.Color888(255, 255, 255));
  matrix.drawLine(15, 7, 16, 7, matrix.Color888(0, 0, 0));
  matrix.drawLine(17, 7, 18, 7, matrix.Color888(255, 255, 255));
  matrix.drawLine(19, 7, 19, 7, matrix.Color888(0, 0, 0));
  matrix.drawLine(20, 7, 21, 7, matrix.Color888(255, 255, 255));
  matrix.drawLine(22, 7, 22, 7, matrix.Color888(0, 0, 0));
  matrix.drawLine(23, 7, 24, 7, matrix.Color888(255, 255, 255));
  matrix.drawLine(25, 7, 27, 7, matrix.Color888(0, 0, 0));
  matrix.drawLine(28, 7, 29, 7, matrix.Color888(255, 255, 255));
  matrix.drawLine(30, 7, 30, 7, matrix.Color888(0, 0, 0));
  matrix.drawLine(31, 7, 37, 7, matrix.Color888(255, 255, 255));
  matrix.drawLine(38, 7, 38, 7, matrix.Color888(0, 0, 0));
  matrix.drawLine(39, 7, 39, 7, matrix.Color888(248, 174, 1));
  matrix.drawLine(40, 7, 41, 7, matrix.Color888(230, 0, 5));
  matrix.drawLine(42, 7, 42, 7, matrix.Color888(248, 174, 1));
  matrix.drawLine(43, 7, 44, 7, matrix.Color888(230, 0, 5));
  matrix.drawLine(45, 7, 45, 7, matrix.Color888(248, 174, 1));
  matrix.drawLine(46, 7, 47, 7, matrix.Color888(230, 0, 5));
  matrix.drawLine(48, 7, 48, 7, matrix.Color888(248, 174, 1));
  matrix.drawLine(49, 7, 50, 7, matrix.Color888(230, 0, 5));
  matrix.drawLine(51, 7, 57, 7, matrix.Color888(248, 174, 1));
  matrix.drawLine(58, 7, 59, 7, matrix.Color888(0, 71, 129));
  matrix.drawLine(60, 7, 60, 7, matrix.Color888(230, 0, 5));
  matrix.drawLine(61, 7, 63, 7, matrix.Color888(248, 174, 1));
  matrix.drawLine(0, 8, 4, 8, matrix.Color888(255, 255, 255));
  matrix.drawLine(5, 8, 6, 8, matrix.Color888(0, 0, 0));
  matrix.drawLine(7, 8, 8, 8, matrix.Color888(255, 255, 255));
  matrix.drawLine(9, 8, 12, 8, matrix.Color888(0, 0, 0));
  matrix.drawLine(13, 8, 14, 8, matrix.Color888(255, 255, 255));
  matrix.drawLine(15, 8, 16, 8, matrix.Color888(0, 0, 0));
  matrix.drawLine(17, 8, 18, 8, matrix.Color888(255, 255, 255));
  matrix.drawLine(19, 8, 19, 8, matrix.Color888(0, 0, 0));
  matrix.drawLine(20, 8, 21, 8, matrix.Color888(255, 255, 255));
  matrix.drawLine(22, 8, 22, 8, matrix.Color888(0, 0, 0));
  matrix.drawLine(23, 8, 24, 8, matrix.Color888(255, 255, 255));
  matrix.drawLine(25, 8, 27, 8, matrix.Color888(0, 0, 0));
  matrix.drawLine(28, 8, 29, 8, matrix.Color888(255, 255, 255));
  matrix.drawLine(30, 8, 30, 8, matrix.Color888(0, 0, 0));
  matrix.drawLine(31, 8, 32, 8, matrix.Color888(255, 255, 255));
  matrix.drawLine(33, 8, 33, 8, matrix.Color888(0, 0, 0));
  matrix.drawLine(34, 8, 37, 8, matrix.Color888(255, 255, 255));
  matrix.drawLine(38, 8, 38, 8, matrix.Color888(0, 0, 0));
  matrix.drawLine(39, 8, 39, 8, matrix.Color888(248, 174, 1));
  matrix.drawLine(40, 8, 41, 8, matrix.Color888(230, 0, 5));
  matrix.drawLine(42, 8, 42, 8, matrix.Color888(248, 174, 1));
  matrix.drawLine(43, 8, 44, 8, matrix.Color888(230, 0, 5));
  matrix.drawLine(45, 8, 45, 8, matrix.Color888(248, 174, 1));
  matrix.drawLine(46, 8, 47, 8, matrix.Color888(230, 0, 5));
  matrix.drawLine(48, 8, 48, 8, matrix.Color888(248, 174, 1));
  matrix.drawLine(49, 8, 50, 8, matrix.Color888(230, 0, 5));
  matrix.drawLine(51, 8, 56, 8, matrix.Color888(248, 174, 1));
  matrix.drawLine(57, 8, 57, 8, matrix.Color888(230, 0, 5));
  matrix.drawLine(58, 8, 58, 8, matrix.Color888(0, 71, 129));
  matrix.drawLine(59, 8, 59, 8, matrix.Color888(230, 0, 5));
  matrix.drawLine(60, 8, 63, 8, matrix.Color888(248, 174, 1));
  matrix.drawLine(0, 9, 1, 9, matrix.Color888(255, 255, 255));
  matrix.drawLine(2, 9, 2, 9, matrix.Color888(0, 0, 0));
  matrix.drawLine(3, 9, 5, 9, matrix.Color888(255, 255, 255));
  matrix.drawLine(6, 9, 6, 9, matrix.Color888(0, 0, 0));
  matrix.drawLine(7, 9, 11, 9, matrix.Color888(255, 255, 255));
  matrix.drawLine(12, 9, 12, 9, matrix.Color888(0, 0, 0));
  matrix.drawLine(13, 9, 15, 9, matrix.Color888(255, 255, 255));
  matrix.drawLine(16, 9, 16, 9, matrix.Color888(0, 0, 0));
  matrix.drawLine(17, 9, 18, 9, matrix.Color888(255, 255, 255));
  matrix.drawLine(19, 9, 19, 9, matrix.Color888(0, 0, 0));
  matrix.drawLine(20, 9, 21, 9, matrix.Color888(255, 255, 255));
  matrix.drawLine(22, 9, 23, 9, matrix.Color888(0, 0, 0));
  matrix.drawLine(24, 9, 28, 9, matrix.Color888(255, 255, 255));
  matrix.drawLine(29, 9, 30, 9, matrix.Color888(0, 0, 0));
  matrix.drawLine(31, 9, 32, 9, matrix.Color888(255, 255, 255));
  matrix.drawLine(33, 9, 34, 9, matrix.Color888(0, 0, 0));
  matrix.drawLine(35, 9, 37, 9, matrix.Color888(255, 255, 255));
  matrix.drawLine(38, 9, 38, 9, matrix.Color888(0, 0, 0));
  matrix.drawLine(39, 9, 39, 9, matrix.Color888(248, 174, 1));
  matrix.drawLine(40, 9, 41, 9, matrix.Color888(230, 0, 5));
  matrix.drawLine(42, 9, 42, 9, matrix.Color888(248, 174, 1));
  matrix.drawLine(43, 9, 44, 9, matrix.Color888(230, 0, 5));
  matrix.drawLine(45, 9, 45, 9, matrix.Color888(248, 174, 1));
  matrix.drawLine(46, 9, 47, 9, matrix.Color888(230, 0, 5));
  matrix.drawLine(48, 9, 48, 9, matrix.Color888(248, 174, 1));
  matrix.drawLine(49, 9, 50, 9, matrix.Color888(230, 0, 5));
  matrix.drawLine(51, 9, 56, 9, matrix.Color888(248, 174, 1));
  matrix.drawLine(57, 9, 59, 9, matrix.Color888(230, 0, 5));
  matrix.drawLine(60, 9, 63, 9, matrix.Color888(248, 174, 1));
  matrix.drawLine(0, 10, 1, 10, matrix.Color888(255, 255, 255));
  matrix.drawLine(2, 10, 3, 10, matrix.Color888(0, 0, 0));
  matrix.drawLine(4, 10, 5, 10, matrix.Color888(255, 255, 255));
  matrix.drawLine(6, 10, 6, 10, matrix.Color888(0, 0, 0));
  matrix.drawLine(7, 10, 11, 10, matrix.Color888(255, 255, 255));
  matrix.drawLine(12, 10, 13, 10, matrix.Color888(0, 0, 0));
  matrix.drawLine(14, 10, 18, 10, matrix.Color888(255, 255, 255));
  matrix.drawLine(19, 10, 19, 10, matrix.Color888(0, 0, 0));
  matrix.drawLine(20, 10, 21, 10, matrix.Color888(255, 255, 255));
  matrix.drawLine(22, 10, 23, 10, matrix.Color888(0, 0, 0));
  matrix.drawLine(25, 10, 27, 10, matrix.Color888(255, 255, 255));
  matrix.drawLine(29, 10, 30, 10, matrix.Color888(0, 0, 0));
  matrix.drawLine(31, 10, 32, 10, matrix.Color888(255, 255, 255));
  matrix.drawLine(33, 10, 35, 10, matrix.Color888(0, 0, 0));
  matrix.drawLine(36, 10, 37, 10, matrix.Color888(255, 255, 255));
  matrix.drawLine(38, 10, 38, 10, matrix.Color888(0, 0, 0));
  matrix.drawLine(39, 10, 39, 10, matrix.Color888(248, 174, 1));
  matrix.drawLine(40, 10, 41, 10, matrix.Color888(230, 0, 5));
  matrix.drawLine(42, 10, 42, 10, matrix.Color888(248, 174, 1));
  matrix.drawLine(43, 10, 44, 10, matrix.Color888(230, 0, 5));
  matrix.drawLine(45, 10, 45, 10, matrix.Color888(248, 174, 1));
  matrix.drawLine(46, 10, 47, 10, matrix.Color888(230, 0, 5));
  matrix.drawLine(48, 10, 48, 10, matrix.Color888(248, 174, 1));
  matrix.drawLine(49, 10, 50, 10, matrix.Color888(230, 0, 5));
  matrix.drawLine(51, 10, 54, 10, matrix.Color888(248, 174, 1));
  matrix.drawLine(55, 10, 55, 10, matrix.Color888(230, 0, 5));
  matrix.drawLine(56, 10, 56, 10, matrix.Color888(248, 174, 1));
  matrix.drawLine(57, 10, 59, 10, matrix.Color888(230, 0, 5));
  matrix.drawLine(60, 10, 63, 10, matrix.Color888(248, 174, 1));
  matrix.drawLine(0, 11, 38, 11, matrix.Color888(0, 0, 0));
  matrix.drawLine(39, 11, 39, 11, matrix.Color888(248, 174, 1));
  matrix.drawLine(40, 11, 41, 11, matrix.Color888(230, 0, 5));
  matrix.drawLine(42, 11, 42, 11, matrix.Color888(248, 174, 1));
  matrix.drawLine(43, 11, 44, 11, matrix.Color888(230, 0, 5));
  matrix.drawLine(45, 11, 45, 11, matrix.Color888(248, 174, 1));
  matrix.drawLine(46, 11, 47, 11, matrix.Color888(230, 0, 5));
  matrix.drawLine(48, 11, 48, 11, matrix.Color888(248, 174, 1));
  matrix.drawLine(49, 11, 50, 11, matrix.Color888(230, 0, 5));
  matrix.drawLine(51, 11, 55, 11, matrix.Color888(248, 174, 1));
  matrix.drawLine(56, 11, 57, 11, matrix.Color888(230, 0, 5));
  matrix.drawLine(58, 11, 63, 11, matrix.Color888(248, 174, 1));
  matrix.drawLine(0, 12, 38, 12, matrix.Color888(0, 0, 0));
  matrix.drawLine(39, 12, 39, 12, matrix.Color888(248, 174, 1));
  matrix.drawLine(40, 12, 41, 12, matrix.Color888(230, 0, 5));
  matrix.drawLine(42, 12, 42, 12, matrix.Color888(248, 174, 1));
  matrix.drawLine(43, 12, 44, 12, matrix.Color888(230, 0, 5));
  matrix.drawLine(45, 12, 45, 12, matrix.Color888(248, 174, 1));
  matrix.drawLine(46, 12, 47, 12, matrix.Color888(230, 0, 5));
  matrix.drawLine(48, 12, 48, 12, matrix.Color888(248, 174, 1));
  matrix.drawLine(49, 12, 50, 12, matrix.Color888(230, 0, 5));
  matrix.drawLine(51, 12, 63, 12, matrix.Color888(248, 174, 1));
  matrix.drawLine(0, 13, 38, 13, matrix.Color888(0, 0, 0));
  matrix.drawLine(39, 13, 39, 13, matrix.Color888(248, 174, 1));
  matrix.drawLine(40, 13, 41, 13, matrix.Color888(230, 0, 5));
  matrix.drawLine(42, 13, 42, 13, matrix.Color888(248, 174, 1));
  matrix.drawLine(43, 13, 44, 13, matrix.Color888(230, 0, 5));
  matrix.drawLine(45, 13, 45, 13, matrix.Color888(248, 174, 1));
  matrix.drawLine(46, 13, 47, 13, matrix.Color888(230, 0, 5));
  matrix.drawLine(48, 13, 48, 13, matrix.Color888(248, 174, 1));
  matrix.drawLine(49, 13, 50, 13, matrix.Color888(230, 0, 5));
  matrix.drawLine(51, 13, 51, 13, matrix.Color888(248, 174, 1));
  matrix.drawLine(52, 13, 57, 13, matrix.Color888(255, 255, 255));
  matrix.drawLine(58, 13, 58, 13, matrix.Color888(230, 0, 5));
  matrix.drawLine(59, 13, 63, 13, matrix.Color888(255, 255, 255));
  matrix.drawLine(0, 14, 16, 14, matrix.Color888(0, 0, 0));
  matrix.drawLine(17, 14, 21, 14, matrix.Color888(255, 255, 255));
  matrix.drawLine(22, 14, 22, 14, matrix.Color888(0, 0, 0));
  matrix.drawLine(23, 14, 24, 14, matrix.Color888(255, 255, 255));
  matrix.drawLine(25, 14, 27, 14, matrix.Color888(0, 0, 0));
  matrix.drawLine(28, 14, 29, 14, matrix.Color888(255, 255, 255));
  matrix.drawLine(30, 14, 30, 14, matrix.Color888(0, 0, 0));
  matrix.drawLine(31, 14, 35, 14, matrix.Color888(255, 255, 255));
  matrix.drawLine(36, 14, 38, 14, matrix.Color888(0, 0, 0));
  matrix.drawLine(39, 14, 39, 14, matrix.Color888(248, 174, 1));
  matrix.drawLine(40, 14, 41, 14, matrix.Color888(230, 0, 5));
  matrix.drawLine(42, 14, 42, 14, matrix.Color888(248, 174, 1));
  matrix.drawLine(43, 14, 44, 14, matrix.Color888(230, 0, 5));
  matrix.drawLine(45, 14, 45, 14, matrix.Color888(248, 174, 1));
  matrix.drawLine(46, 14, 47, 14, matrix.Color888(230, 0, 5));
  matrix.drawLine(48, 14, 48, 14, matrix.Color888(248, 174, 1));
  matrix.drawLine(49, 14, 50, 14, matrix.Color888(230, 0, 5));
  matrix.drawLine(51, 14, 51, 14, matrix.Color888(248, 174, 1));
  matrix.drawLine(52, 14, 56, 14, matrix.Color888(255, 255, 255));
  matrix.drawLine(57, 14, 58, 14, matrix.Color888(230, 0, 5));
  matrix.drawLine(59, 14, 63, 14, matrix.Color888(255, 255, 255));
  matrix.drawLine(0, 15, 15, 15, matrix.Color888(0, 0, 0));
  matrix.drawLine(16, 15, 21, 15, matrix.Color888(255, 255, 255));
  matrix.drawLine(22, 15, 22, 15, matrix.Color888(0, 0, 0));
  matrix.drawLine(23, 15, 24, 15, matrix.Color888(255, 255, 255));
  matrix.drawLine(25, 15, 27, 15, matrix.Color888(0, 0, 0));
  matrix.drawLine(28, 15, 29, 15, matrix.Color888(255, 255, 255));
  matrix.drawLine(30, 15, 30, 15, matrix.Color888(0, 0, 0));
  matrix.drawLine(31, 15, 36, 15, matrix.Color888(255, 255, 255));
  matrix.drawLine(37, 15, 38, 15, matrix.Color888(0, 0, 0));
  matrix.drawLine(39, 15, 39, 15, matrix.Color888(248, 174, 1));
  matrix.drawLine(40, 15, 41, 15, matrix.Color888(230, 0, 5));
  matrix.drawLine(42, 15, 42, 15, matrix.Color888(248, 174, 1));
  matrix.drawLine(43, 15, 44, 15, matrix.Color888(230, 0, 5));
  matrix.drawLine(45, 15, 45, 15, matrix.Color888(248, 174, 1));
  matrix.drawLine(46, 15, 47, 15, matrix.Color888(230, 0, 5));
  matrix.drawLine(48, 15, 48, 15, matrix.Color888(248, 174, 1));
  matrix.drawLine(49, 15, 50, 15, matrix.Color888(230, 0, 5));
  matrix.drawLine(51, 15, 51, 15, matrix.Color888(248, 174, 1));
  matrix.drawLine(52, 15, 52, 15, matrix.Color888(255, 255, 255));
  matrix.drawLine(53, 15, 53, 15, matrix.Color888(230, 0, 5));
  matrix.drawLine(54, 15, 55, 15, matrix.Color888(255, 255, 255));
  matrix.drawLine(56, 15, 58, 15, matrix.Color888(230, 0, 5));
  matrix.drawLine(59, 15, 61, 15, matrix.Color888(255, 255, 255));
  matrix.drawLine(62, 15, 62, 15, matrix.Color888(230, 0, 5));
  matrix.drawLine(63, 15, 63, 15, matrix.Color888(255, 255, 255));
  matrix.drawLine(0, 16, 15, 16, matrix.Color888(0, 0, 0));
  matrix.drawLine(16, 16, 17, 16, matrix.Color888(255, 255, 255));
  matrix.drawLine(18, 16, 22, 16, matrix.Color888(0, 0, 0));
  matrix.drawLine(23, 16, 24, 16, matrix.Color888(255, 255, 255));
  matrix.drawLine(25, 16, 27, 16, matrix.Color888(0, 0, 0));
  matrix.drawLine(28, 16, 29, 16, matrix.Color888(255, 255, 255));
  matrix.drawLine(30, 16, 30, 16, matrix.Color888(0, 0, 0));
  matrix.drawLine(31, 16, 32, 16, matrix.Color888(255, 255, 255));
  matrix.drawLine(33, 16, 34, 16, matrix.Color888(0, 0, 0));
  matrix.drawLine(35, 16, 37, 16, matrix.Color888(255, 255, 255));
  matrix.drawLine(38, 16, 38, 16, matrix.Color888(0, 0, 0));
  matrix.drawLine(39, 16, 39, 16, matrix.Color888(248, 174, 1));
  matrix.drawLine(40, 16, 41, 16, matrix.Color888(230, 0, 5));
  matrix.drawLine(42, 16, 42, 16, matrix.Color888(248, 174, 1));
  matrix.drawLine(43, 16, 44, 16, matrix.Color888(230, 0, 5));
  matrix.drawLine(45, 16, 45, 16, matrix.Color888(248, 174, 1));
  matrix.drawLine(46, 16, 47, 16, matrix.Color888(230, 0, 5));
  matrix.drawLine(48, 16, 48, 16, matrix.Color888(248, 174, 1));
  matrix.drawLine(49, 16, 50, 16, matrix.Color888(230, 0, 5));
  matrix.drawLine(51, 16, 51, 16, matrix.Color888(248, 174, 1));
  matrix.drawLine(52, 16, 52, 16, matrix.Color888(255, 255, 255));
  matrix.drawLine(53, 16, 54, 16, matrix.Color888(230, 0, 5));
  matrix.drawLine(55, 16, 56, 16, matrix.Color888(255, 255, 255));
  matrix.drawLine(57, 16, 58, 16, matrix.Color888(230, 0, 5));
  matrix.drawLine(59, 16, 60, 16, matrix.Color888(255, 255, 255));
  matrix.drawLine(61, 16, 62, 16, matrix.Color888(230, 0, 5));
  matrix.drawLine(63, 16, 63, 16, matrix.Color888(255, 255, 255));
  matrix.drawLine(0, 17, 15, 17, matrix.Color888(0, 0, 0));
  matrix.drawLine(16, 17, 20, 17, matrix.Color888(255, 255, 255));
  matrix.drawLine(21, 17, 22, 17, matrix.Color888(0, 0, 0));
  matrix.drawLine(23, 17, 24, 17, matrix.Color888(255, 255, 255));
  matrix.drawLine(25, 17, 27, 17, matrix.Color888(0, 0, 0));
  matrix.drawLine(28, 17, 29, 17, matrix.Color888(255, 255, 255));
  matrix.drawLine(30, 17, 30, 17, matrix.Color888(0, 0, 0));
  matrix.drawLine(31, 17, 32, 17, matrix.Color888(255, 255, 255));
  matrix.drawLine(33, 17, 35, 17, matrix.Color888(0, 0, 0));
  matrix.drawLine(36, 17, 37, 17, matrix.Color888(255, 255, 255));
  matrix.drawLine(38, 17, 38, 17, matrix.Color888(0, 0, 0));
  matrix.drawLine(39, 17, 39, 17, matrix.Color888(248, 174, 1));
  matrix.drawLine(40, 17, 41, 17, matrix.Color888(230, 0, 5));
  matrix.drawLine(42, 17, 42, 17, matrix.Color888(248, 174, 1));
  matrix.drawLine(43, 17, 44, 17, matrix.Color888(230, 0, 5));
  matrix.drawLine(45, 17, 45, 17, matrix.Color888(248, 174, 1));
  matrix.drawLine(46, 17, 47, 17, matrix.Color888(230, 0, 5));
  matrix.drawLine(48, 17, 48, 17, matrix.Color888(248, 174, 1));
  matrix.drawLine(49, 17, 50, 17, matrix.Color888(230, 0, 5));
  matrix.drawLine(51, 17, 51, 17, matrix.Color888(248, 174, 1));
  matrix.drawLine(52, 17, 52, 17, matrix.Color888(255, 255, 255));
  matrix.drawLine(53, 17, 62, 17, matrix.Color888(230, 0, 5));
  matrix.drawLine(63, 17, 63, 17, matrix.Color888(255, 255, 255));
  matrix.drawLine(0, 18, 17, 18, matrix.Color888(0, 0, 0));
  matrix.drawLine(18, 18, 21, 18, matrix.Color888(255, 255, 255));
  matrix.drawLine(22, 18, 22, 18, matrix.Color888(0, 0, 0));
  matrix.drawLine(23, 18, 24, 18, matrix.Color888(255, 255, 255));
  matrix.drawLine(25, 18, 27, 18, matrix.Color888(0, 0, 0));
  matrix.drawLine(28, 18, 29, 18, matrix.Color888(255, 255, 255));
  matrix.drawLine(30, 18, 30, 18, matrix.Color888(0, 0, 0));
  matrix.drawLine(31, 18, 32, 18, matrix.Color888(255, 255, 255));
  matrix.drawLine(33, 18, 35, 18, matrix.Color888(0, 0, 0));
  matrix.drawLine(36, 18, 37, 18, matrix.Color888(255, 255, 255));
  matrix.drawLine(38, 18, 38, 18, matrix.Color888(0, 0, 0));
  matrix.drawLine(39, 18, 39, 18, matrix.Color888(248, 174, 1));
  matrix.drawLine(40, 18, 41, 18, matrix.Color888(230, 0, 5));
  matrix.drawLine(42, 18, 42, 18, matrix.Color888(248, 174, 1));
  matrix.drawLine(43, 18, 44, 18, matrix.Color888(230, 0, 5));
  matrix.drawLine(45, 18, 45, 18, matrix.Color888(248, 174, 1));
  matrix.drawLine(46, 18, 47, 18, matrix.Color888(230, 0, 5));
  matrix.drawLine(48, 18, 48, 18, matrix.Color888(248, 174, 1));
  matrix.drawLine(49, 18, 50, 18, matrix.Color888(230, 0, 5));
  matrix.drawLine(51, 18, 51, 18, matrix.Color888(248, 174, 1));
  matrix.drawLine(52, 18, 52, 18, matrix.Color888(255, 255, 255));
  matrix.drawLine(53, 18, 55, 18, matrix.Color888(230, 0, 5));
  matrix.drawLine(56, 18, 56, 18, matrix.Color888(255, 255, 255));
  matrix.drawLine(57, 18, 58, 18, matrix.Color888(230, 0, 5));
  matrix.drawLine(59, 18, 59, 18, matrix.Color888(255, 255, 255));
  matrix.drawLine(60, 18, 62, 18, matrix.Color888(230, 0, 5));
  matrix.drawLine(63, 18, 63, 18, matrix.Color888(255, 255, 255));
  matrix.drawLine(0, 19, 19, 19, matrix.Color888(0, 0, 0));
  matrix.drawLine(20, 19, 21, 19, matrix.Color888(255, 255, 255));
  matrix.drawLine(22, 19, 22, 19, matrix.Color888(0, 0, 0));
  matrix.drawLine(23, 19, 24, 19, matrix.Color888(255, 255, 255));
  matrix.drawLine(25, 19, 27, 19, matrix.Color888(0, 0, 0));
  matrix.drawLine(28, 19, 29, 19, matrix.Color888(255, 255, 255));
  matrix.drawLine(30, 19, 30, 19, matrix.Color888(0, 0, 0));
  matrix.drawLine(31, 19, 32, 19, matrix.Color888(255, 255, 255));
  matrix.drawLine(33, 19, 34, 19, matrix.Color888(0, 0, 0));
  matrix.drawLine(35, 19, 37, 19, matrix.Color888(255, 255, 255));
  matrix.drawLine(38, 19, 38, 19, matrix.Color888(0, 0, 0));
  matrix.drawLine(39, 19, 39, 19, matrix.Color888(248, 174, 1));
  matrix.drawLine(40, 19, 41, 19, matrix.Color888(230, 0, 5));
  matrix.drawLine(42, 19, 42, 19, matrix.Color888(248, 174, 1));
  matrix.drawLine(43, 19, 44, 19, matrix.Color888(230, 0, 5));
  matrix.drawLine(45, 19, 45, 19, matrix.Color888(248, 174, 1));
  matrix.drawLine(46, 19, 47, 19, matrix.Color888(230, 0, 5));
  matrix.drawLine(48, 19, 48, 19, matrix.Color888(248, 174, 1));
  matrix.drawLine(49, 19, 50, 19, matrix.Color888(230, 0, 5));
  matrix.drawLine(51, 19, 51, 19, matrix.Color888(248, 174, 1));
  matrix.drawLine(52, 19, 52, 19, matrix.Color888(255, 255, 255));
  matrix.drawLine(53, 19, 53, 19, matrix.Color888(230, 0, 5));
  matrix.drawLine(54, 19, 56, 19, matrix.Color888(255, 255, 255));
  matrix.drawLine(57, 19, 58, 19, matrix.Color888(230, 0, 5));
  matrix.drawLine(59, 19, 61, 19, matrix.Color888(255, 255, 255));
  matrix.drawLine(62, 19, 62, 19, matrix.Color888(230, 0, 5));
  matrix.drawLine(63, 19, 63, 19, matrix.Color888(255, 255, 255));
  matrix.drawLine(0, 20, 15, 20, matrix.Color888(0, 0, 0));
  matrix.drawLine(16, 20, 21, 20, matrix.Color888(255, 255, 255));
  matrix.drawLine(22, 20, 22, 20, matrix.Color888(0, 0, 0));
  matrix.drawLine(23, 20, 29, 20, matrix.Color888(255, 255, 255));
  matrix.drawLine(30, 20, 30, 20, matrix.Color888(0, 0, 0));
  matrix.drawLine(31, 20, 36, 20, matrix.Color888(255, 255, 255));
  matrix.drawLine(37, 20, 38, 20, matrix.Color888(0, 0, 0));
  matrix.drawLine(39, 20, 39, 20, matrix.Color888(248, 174, 1));
  matrix.drawLine(40, 20, 41, 20, matrix.Color888(230, 0, 5));
  matrix.drawLine(42, 20, 42, 20, matrix.Color888(248, 174, 1));
  matrix.drawLine(43, 20, 44, 20, matrix.Color888(230, 0, 5));
  matrix.drawLine(45, 20, 45, 20, matrix.Color888(248, 174, 1));
  matrix.drawLine(46, 20, 47, 20, matrix.Color888(230, 0, 5));
  matrix.drawLine(48, 20, 48, 20, matrix.Color888(248, 174, 1));
  matrix.drawLine(49, 20, 50, 20, matrix.Color888(230, 0, 5));
  matrix.drawLine(51, 20, 51, 20, matrix.Color888(248, 174, 1));
  matrix.drawLine(52, 20, 52, 20, matrix.Color888(255, 255, 255));
  matrix.drawLine(53, 20, 53, 20, matrix.Color888(230, 0, 5));
  matrix.drawLine(54, 20, 56, 20, matrix.Color888(255, 255, 255));
  matrix.drawLine(57, 20, 58, 20, matrix.Color888(230, 0, 5));
  matrix.drawLine(59, 20, 61, 20, matrix.Color888(255, 255, 255));
  matrix.drawLine(62, 20, 62, 20, matrix.Color888(230, 0, 5));
  matrix.drawLine(63, 20, 63, 20, matrix.Color888(255, 255, 255));
  matrix.drawLine(0, 21, 15, 21, matrix.Color888(0, 0, 0));
  matrix.drawLine(16, 21, 20, 21, matrix.Color888(255, 255, 255));
  matrix.drawLine(21, 21, 23, 21, matrix.Color888(0, 0, 0));
  matrix.drawLine(24, 21, 28, 21, matrix.Color888(255, 255, 255));
  matrix.drawLine(29, 21, 30, 21, matrix.Color888(0, 0, 0));
  matrix.drawLine(31, 21, 35, 21, matrix.Color888(255, 255, 255));
  matrix.drawLine(36, 21, 38, 21, matrix.Color888(0, 0, 0));
  matrix.drawLine(39, 21, 39, 21, matrix.Color888(248, 174, 1));
  matrix.drawLine(40, 21, 41, 21, matrix.Color888(230, 0, 5));
  matrix.drawLine(42, 21, 42, 21, matrix.Color888(248, 174, 1));
  matrix.drawLine(43, 21, 44, 21, matrix.Color888(230, 0, 5));
  matrix.drawLine(45, 21, 45, 21, matrix.Color888(248, 174, 1));
  matrix.drawLine(46, 21, 47, 21, matrix.Color888(230, 0, 5));
  matrix.drawLine(48, 21, 48, 21, matrix.Color888(248, 174, 1));
  matrix.drawLine(49, 21, 50, 21, matrix.Color888(230, 0, 5));
  matrix.drawLine(51, 21, 51, 21, matrix.Color888(248, 174, 1));
  matrix.drawLine(52, 21, 53, 21, matrix.Color888(255, 255, 255));
  matrix.drawLine(54, 21, 61, 21, matrix.Color888(230, 0, 5));
  matrix.drawLine(62, 21, 63, 21, matrix.Color888(255, 255, 255));
  matrix.drawLine(0, 22, 38, 22, matrix.Color888(0, 0, 0));
  matrix.drawLine(39, 22, 39, 22, matrix.Color888(248, 174, 1));
  matrix.drawLine(40, 22, 41, 22, matrix.Color888(230, 0, 5));
  matrix.drawLine(42, 22, 42, 22, matrix.Color888(248, 174, 1));
  matrix.drawLine(43, 22, 44, 22, matrix.Color888(230, 0, 5));
  matrix.drawLine(45, 22, 45, 22, matrix.Color888(248, 174, 1));
  matrix.drawLine(46, 22, 47, 22, matrix.Color888(230, 0, 5));
  matrix.drawLine(48, 22, 48, 22, matrix.Color888(248, 174, 1));
  matrix.drawLine(49, 22, 50, 22, matrix.Color888(230, 0, 5));
  matrix.drawLine(51, 22, 51, 22, matrix.Color888(248, 174, 1));
  matrix.drawLine(52, 22, 52, 22, matrix.Color888(255, 255, 255));
  matrix.drawLine(53, 22, 53, 22, matrix.Color888(230, 0, 5));
  matrix.drawLine(54, 22, 54, 22, matrix.Color888(255, 255, 255));
  matrix.drawLine(56, 22, 56, 22, matrix.Color888(255, 255, 255));
  matrix.drawLine(57, 22, 58, 22, matrix.Color888(230, 0, 5));
  matrix.drawLine(59, 22, 59, 22, matrix.Color888(255, 255, 255));
  matrix.drawLine(61, 22, 61, 22, matrix.Color888(255, 255, 255));
  matrix.drawLine(62, 22, 62, 22, matrix.Color888(230, 0, 5));
  matrix.drawLine(63, 22, 63, 22, matrix.Color888(255, 255, 255));
  matrix.drawLine(0, 23, 38, 23, matrix.Color888(0, 0, 0));
  matrix.drawLine(39, 23, 39, 23, matrix.Color888(248, 174, 1));
  matrix.drawLine(40, 23, 41, 23, matrix.Color888(230, 0, 5));
  matrix.drawLine(42, 23, 42, 23, matrix.Color888(248, 174, 1));
  matrix.drawLine(43, 23, 44, 23, matrix.Color888(230, 0, 5));
  matrix.drawLine(45, 23, 45, 23, matrix.Color888(248, 174, 1));
  matrix.drawLine(46, 23, 47, 23, matrix.Color888(230, 0, 5));
  matrix.drawLine(48, 23, 48, 23, matrix.Color888(248, 174, 1));
  matrix.drawLine(49, 23, 50, 23, matrix.Color888(230, 0, 5));
  matrix.drawLine(51, 23, 51, 23, matrix.Color888(248, 174, 1));
  matrix.drawLine(52, 23, 53, 23, matrix.Color888(255, 255, 255));
  matrix.drawLine(56, 23, 56, 23, matrix.Color888(230, 0, 5));
  matrix.drawLine(59, 23, 59, 23, matrix.Color888(230, 0, 5));
  matrix.drawLine(62, 23, 63, 23, matrix.Color888(255, 255, 255));
  matrix.drawLine(0, 24, 38, 24, matrix.Color888(0, 0, 0));
  matrix.drawLine(39, 24, 39, 24, matrix.Color888(248, 174, 1));
  matrix.drawLine(40, 24, 41, 24, matrix.Color888(230, 0, 5));
  matrix.drawLine(42, 24, 42, 24, matrix.Color888(248, 174, 1));
  matrix.drawLine(43, 24, 44, 24, matrix.Color888(230, 0, 5));
  matrix.drawLine(45, 24, 45, 24, matrix.Color888(248, 174, 1));
  matrix.drawLine(46, 24, 47, 24, matrix.Color888(230, 0, 5));
  matrix.drawLine(48, 24, 48, 24, matrix.Color888(248, 174, 1));
  matrix.drawLine(49, 24, 50, 24, matrix.Color888(230, 0, 5));
  matrix.drawLine(51, 24, 51, 24, matrix.Color888(248, 174, 1));
  matrix.drawLine(52, 24, 53, 24, matrix.Color888(255, 255, 255));
  matrix.drawLine(62, 24, 63, 24, matrix.Color888(255, 255, 255));
  matrix.drawLine(0, 25, 38, 25, matrix.Color888(0, 0, 0));
  matrix.drawLine(39, 25, 39, 25, matrix.Color888(248, 174, 1));
  matrix.drawLine(40, 25, 41, 25, matrix.Color888(230, 0, 5));
  matrix.drawLine(42, 25, 42, 25, matrix.Color888(248, 174, 1));
  matrix.drawLine(43, 25, 44, 25, matrix.Color888(230, 0, 5));
  matrix.drawLine(45, 25, 45, 25, matrix.Color888(248, 174, 1));
  matrix.drawLine(46, 25, 47, 25, matrix.Color888(230, 0, 5));
  matrix.drawLine(48, 25, 48, 25, matrix.Color888(248, 174, 1));
  matrix.drawLine(49, 25, 50, 25, matrix.Color888(230, 0, 5));
  matrix.drawLine(51, 25, 51, 25, matrix.Color888(248, 174, 1));
  matrix.drawLine(52, 25, 52, 25, matrix.Color888(255, 255, 255));
  matrix.drawLine(53, 25, 62, 25, matrix.Color888(0, 71, 129));
  matrix.drawLine(63, 25, 63, 25, matrix.Color888(255, 255, 255));
  matrix.drawLine(0, 26, 39, 26, matrix.Color888(0, 0, 0));
  matrix.drawLine(40, 26, 41, 26, matrix.Color888(230, 0, 5));
  matrix.drawLine(42, 26, 42, 26, matrix.Color888(248, 174, 1));
  matrix.drawLine(43, 26, 44, 26, matrix.Color888(230, 0, 5));
  matrix.drawLine(45, 26, 45, 26, matrix.Color888(248, 174, 1));
  matrix.drawLine(46, 26, 47, 26, matrix.Color888(230, 0, 5));
  matrix.drawLine(48, 26, 48, 26, matrix.Color888(248, 174, 1));
  matrix.drawLine(49, 26, 50, 26, matrix.Color888(230, 0, 5));
  matrix.drawLine(51, 26, 51, 26, matrix.Color888(248, 174, 1));
  matrix.drawLine(52, 26, 62, 26, matrix.Color888(255, 255, 255));
  matrix.drawLine(63, 26, 63, 26, matrix.Color888(0, 0, 0));
  matrix.drawLine(0, 27, 41, 27, matrix.Color888(0, 0, 0));
  matrix.drawLine(42, 27, 42, 27, matrix.Color888(248, 174, 1));
  matrix.drawLine(43, 27, 44, 27, matrix.Color888(230, 0, 5));
  matrix.drawLine(45, 27, 45, 27, matrix.Color888(248, 174, 1));
  matrix.drawLine(46, 27, 47, 27, matrix.Color888(230, 0, 5));
  matrix.drawLine(48, 27, 48, 27, matrix.Color888(248, 174, 1));
  matrix.drawLine(49, 27, 50, 27, matrix.Color888(230, 0, 5));
  matrix.drawLine(51, 27, 51, 27, matrix.Color888(248, 174, 1));
  matrix.drawLine(52, 27, 60, 27, matrix.Color888(255, 255, 255));
  matrix.drawLine(61, 27, 63, 27, matrix.Color888(0, 0, 0));
  matrix.drawLine(0, 28, 44, 28, matrix.Color888(0, 0, 0));
  matrix.drawLine(45, 28, 45, 28, matrix.Color888(248, 174, 1));
  matrix.drawLine(46, 28, 47, 28, matrix.Color888(230, 0, 5));
  matrix.drawLine(48, 28, 48, 28, matrix.Color888(248, 174, 1));
  matrix.drawLine(49, 28, 50, 28, matrix.Color888(230, 0, 5));
  matrix.drawLine(51, 28, 51, 28, matrix.Color888(248, 174, 1));
  matrix.drawLine(52, 28, 57, 28, matrix.Color888(255, 255, 255));
  matrix.drawLine(58, 28, 63, 28, matrix.Color888(0, 0, 0));
  matrix.drawLine(0, 29, 46, 29, matrix.Color888(0, 0, 0));
  matrix.drawLine(47, 29, 47, 29, matrix.Color888(230, 0, 5));
  matrix.drawLine(48, 29, 48, 29, matrix.Color888(248, 174, 1));
  matrix.drawLine(49, 29, 50, 29, matrix.Color888(230, 0, 5));
  matrix.drawLine(51, 29, 51, 29, matrix.Color888(248, 174, 1));
  matrix.drawLine(52, 29, 55, 29, matrix.Color888(255, 255, 255));
  matrix.drawLine(56, 29, 63, 29, matrix.Color888(0, 0, 0));
  matrix.drawLine(0, 30, 49, 30, matrix.Color888(0, 0, 0));
  matrix.drawLine(50, 30, 50, 30, matrix.Color888(230, 0, 5));
  matrix.drawLine(51, 30, 51, 30, matrix.Color888(248, 174, 1));
  matrix.drawLine(52, 30, 52, 30, matrix.Color888(255, 255, 255));
  matrix.drawLine(53, 30, 63, 30, matrix.Color888(0, 0, 0));
  matrix.drawLine(0, 31, 63, 31, matrix.Color888(0, 0, 0));
}

void AirInterieur()
{
  matrix.drawLine(10, 0, 10, 0, matrix.Color888(0, 1, 7));
  matrix.drawLine(11, 0, 11, 0, matrix.Color888(2, 9, 59));
  matrix.drawLine(12, 0, 12, 0, matrix.Color888(4, 16, 112));
  matrix.drawLine(13, 0, 13, 0, matrix.Color888(5, 24, 165));
  matrix.drawLine(14, 0, 14, 0, matrix.Color888(7, 32, 222));
  matrix.drawLine(15, 0, 63, 0, matrix.Color888(8, 37, 254));
  matrix.drawLine(15, 1, 15, 1, matrix.Color888(1, 5, 33));
  matrix.drawLine(16, 1, 16, 1, matrix.Color888(3, 13, 93));
  matrix.drawLine(17, 1, 17, 1, matrix.Color888(5, 22, 154));
  matrix.drawLine(18, 1, 18, 1, matrix.Color888(7, 32, 218));
  matrix.drawLine(19, 1, 63, 1, matrix.Color888(8, 37, 254));
  matrix.drawLine(19, 2, 19, 2, matrix.Color888(1, 5, 37));
  matrix.drawLine(20, 2, 20, 2, matrix.Color888(3, 15, 105));
  matrix.drawLine(21, 2, 21, 2, matrix.Color888(5, 25, 173));
  matrix.drawLine(22, 2, 22, 2, matrix.Color888(8, 36, 244));
  matrix.drawLine(23, 2, 63, 2, matrix.Color888(8, 37, 254));
  matrix.drawLine(23, 3, 23, 3, matrix.Color888(2, 11, 73));
  matrix.drawLine(24, 3, 24, 3, matrix.Color888(5, 21, 146));
  matrix.drawLine(25, 3, 25, 3, matrix.Color888(7, 33, 223));
  matrix.drawLine(26, 3, 48, 3, matrix.Color888(8, 37, 254));
  matrix.drawLine(49, 3, 50, 3, matrix.Color888(255, 0, 0));
  matrix.drawLine(51, 3, 63, 3, matrix.Color888(8, 37, 254));
  matrix.drawLine(26, 4, 26, 4, matrix.Color888(2, 9, 59));
  matrix.drawLine(27, 4, 27, 4, matrix.Color888(4, 20, 138));
  matrix.drawLine(28, 4, 28, 4, matrix.Color888(7, 32, 220));
  matrix.drawLine(29, 4, 47, 4, matrix.Color888(8, 37, 254));
  matrix.drawLine(48, 4, 51, 4, matrix.Color888(255, 0, 0));
  matrix.drawLine(52, 4, 53, 4, matrix.Color888(8, 37, 254));
  matrix.drawLine(58, 4, 63, 4, matrix.Color888(8, 37, 254));
  matrix.drawLine(29, 5, 29, 5, matrix.Color888(2, 9, 64));
  matrix.drawLine(30, 5, 30, 5, matrix.Color888(5, 22, 149));
  matrix.drawLine(31, 5, 31, 5, matrix.Color888(7, 35, 237));
  matrix.drawLine(32, 5, 46, 5, matrix.Color888(8, 37, 254));
  matrix.drawLine(47, 5, 48, 5, matrix.Color888(255, 0, 0));
  matrix.drawLine(49, 5, 50, 5, matrix.Color888(180, 180, 180));
  matrix.drawLine(51, 5, 52, 5, matrix.Color888(255, 0, 0));
  matrix.drawLine(53, 5, 53, 5, matrix.Color888(8, 37, 254));
  matrix.drawLine(55, 5, 56, 5, matrix.Color888(180, 180, 180));
  matrix.drawLine(58, 5, 63, 5, matrix.Color888(8, 37, 254));
  matrix.drawLine(16, 6, 16, 6, matrix.Color888(180, 180, 180));
  matrix.drawLine(20, 6, 20, 6, matrix.Color888(180, 180, 180));
  matrix.drawLine(32, 6, 32, 6, matrix.Color888(3, 13, 89));
  matrix.drawLine(33, 6, 33, 6, matrix.Color888(6, 26, 179));
  matrix.drawLine(34, 6, 45, 6, matrix.Color888(8, 37, 254));
  matrix.drawLine(46, 6, 47, 6, matrix.Color888(255, 0, 0));
  matrix.drawLine(48, 6, 51, 6, matrix.Color888(180, 180, 180));
  matrix.drawLine(52, 6, 53, 6, matrix.Color888(255, 0, 0));
  matrix.drawLine(55, 6, 56, 6, matrix.Color888(180, 180, 180));
  matrix.drawLine(58, 6, 63, 6, matrix.Color888(8, 37, 254));
  matrix.drawLine(15, 7, 17, 7, matrix.Color888(180, 180, 180));
  matrix.drawLine(34, 7, 34, 7, matrix.Color888(1, 5, 37));
  matrix.drawLine(35, 7, 35, 7, matrix.Color888(4, 19, 132));
  matrix.drawLine(36, 7, 36, 7, matrix.Color888(7, 33, 229));
  matrix.drawLine(37, 7, 44, 7, matrix.Color888(8, 37, 254));
  matrix.drawLine(45, 7, 46, 7, matrix.Color888(255, 0, 0));
  matrix.drawLine(47, 7, 52, 7, matrix.Color888(180, 180, 180));
  matrix.drawLine(53, 7, 54, 7, matrix.Color888(255, 0, 0));
  matrix.drawLine(55, 7, 56, 7, matrix.Color888(180, 180, 180));
  matrix.drawLine(58, 7, 63, 7, matrix.Color888(8, 37, 254));
  matrix.drawLine(15, 8, 15, 8, matrix.Color888(180, 180, 180));
  matrix.drawLine(17, 8, 17, 8, matrix.Color888(180, 180, 180));
  matrix.drawLine(20, 8, 20, 8, matrix.Color888(180, 180, 180));
  matrix.drawLine(22, 8, 23, 8, matrix.Color888(180, 180, 180));
  matrix.drawLine(37, 8, 37, 8, matrix.Color888(3, 14, 95));
  matrix.drawLine(38, 8, 38, 8, matrix.Color888(6, 29, 196));
  matrix.drawLine(39, 8, 43, 8, matrix.Color888(8, 37, 254));
  matrix.drawLine(44, 8, 45, 8, matrix.Color888(255, 0, 0));
  matrix.drawLine(46, 8, 53, 8, matrix.Color888(180, 180, 180));
  matrix.drawLine(54, 8, 55, 8, matrix.Color888(255, 0, 0));
  matrix.drawLine(56, 8, 56, 8, matrix.Color888(180, 180, 180));
  matrix.drawLine(58, 8, 63, 8, matrix.Color888(8, 37, 254));
  matrix.drawLine(15, 9, 15, 9, matrix.Color888(180, 180, 180));
  matrix.drawLine(17, 9, 17, 9, matrix.Color888(180, 180, 180));
  matrix.drawLine(20, 9, 20, 9, matrix.Color888(180, 180, 180));
  matrix.drawLine(22, 9, 22, 9, matrix.Color888(180, 180, 180));
  matrix.drawLine(39, 9, 39, 9, matrix.Color888(2, 10, 68));
  matrix.drawLine(40, 9, 40, 9, matrix.Color888(5, 25, 173));
  matrix.drawLine(41, 9, 42, 9, matrix.Color888(8, 37, 254));
  matrix.drawLine(43, 9, 44, 9, matrix.Color888(255, 0, 0));
  matrix.drawLine(45, 9, 48, 9, matrix.Color888(180, 180, 180));
  matrix.drawLine(51, 9, 54, 9, matrix.Color888(180, 180, 180));
  matrix.drawLine(55, 9, 56, 9, matrix.Color888(255, 0, 0));
  matrix.drawLine(58, 9, 63, 9, matrix.Color888(8, 37, 254));
  matrix.drawLine(14, 10, 18, 10, matrix.Color888(180, 180, 180));
  matrix.drawLine(20, 10, 20, 10, matrix.Color888(180, 180, 180));
  matrix.drawLine(22, 10, 22, 10, matrix.Color888(180, 180, 180));
  matrix.drawLine(41, 10, 41, 10, matrix.Color888(2, 7, 51));
  matrix.drawLine(42, 10, 43, 10, matrix.Color888(255, 0, 0));
  matrix.drawLine(44, 10, 47, 10, matrix.Color888(180, 180, 180));
  matrix.drawLine(49, 10, 50, 10, matrix.Color888(8, 37, 254));
  matrix.drawLine(52, 10, 55, 10, matrix.Color888(180, 180, 180));
  matrix.drawLine(56, 10, 57, 10, matrix.Color888(255, 0, 0));
  matrix.drawLine(58, 10, 63, 10, matrix.Color888(8, 37, 254));
  matrix.drawLine(14, 11, 14, 11, matrix.Color888(180, 180, 180));
  matrix.drawLine(18, 11, 18, 11, matrix.Color888(180, 180, 180));
  matrix.drawLine(20, 11, 20, 11, matrix.Color888(180, 180, 180));
  matrix.drawLine(22, 11, 22, 11, matrix.Color888(180, 180, 180));
  matrix.drawLine(41, 11, 42, 11, matrix.Color888(255, 0, 0));
  matrix.drawLine(43, 11, 47, 11, matrix.Color888(180, 180, 180));
  matrix.drawLine(49, 11, 50, 11, matrix.Color888(8, 37, 254));
  matrix.drawLine(52, 11, 56, 11, matrix.Color888(180, 180, 180));
  matrix.drawLine(57, 11, 58, 11, matrix.Color888(255, 0, 0));
  matrix.drawLine(59, 11, 63, 11, matrix.Color888(8, 37, 254));
  matrix.drawLine(14, 12, 14, 12, matrix.Color888(180, 180, 180));
  matrix.drawLine(18, 12, 18, 12, matrix.Color888(180, 180, 180));
  matrix.drawLine(20, 12, 20, 12, matrix.Color888(180, 180, 180));
  matrix.drawLine(22, 12, 22, 12, matrix.Color888(180, 180, 180));
  matrix.drawLine(40, 12, 41, 12, matrix.Color888(255, 0, 0));
  matrix.drawLine(42, 12, 48, 12, matrix.Color888(180, 180, 180));
  matrix.drawLine(51, 12, 57, 12, matrix.Color888(180, 180, 180));
  matrix.drawLine(58, 12, 59, 12, matrix.Color888(255, 0, 0));
  matrix.drawLine(60, 12, 63, 12, matrix.Color888(8, 37, 254));
  matrix.drawLine(39, 13, 40, 13, matrix.Color888(255, 0, 0));
  matrix.drawLine(41, 13, 58, 13, matrix.Color888(180, 180, 180));
  matrix.drawLine(59, 13, 60, 13, matrix.Color888(255, 0, 0));
  matrix.drawLine(61, 13, 63, 13, matrix.Color888(8, 37, 254));
  matrix.drawLine(38, 14, 39, 14, matrix.Color888(255, 0, 0));
  matrix.drawLine(40, 14, 59, 14, matrix.Color888(180, 180, 180));
  matrix.drawLine(60, 14, 61, 14, matrix.Color888(255, 0, 0));
  matrix.drawLine(62, 14, 63, 14, matrix.Color888(8, 37, 254));
  matrix.drawLine(37, 15, 38, 15, matrix.Color888(255, 0, 0));
  matrix.drawLine(40, 15, 59, 15, matrix.Color888(180, 180, 180));
  matrix.drawLine(61, 15, 62, 15, matrix.Color888(255, 0, 0));
  matrix.drawLine(63, 15, 63, 15, matrix.Color888(8, 37, 254));
  matrix.drawLine(40, 16, 59, 16, matrix.Color888(180, 180, 180));
  matrix.drawLine(61, 16, 63, 16, matrix.Color888(8, 37, 254));
  matrix.drawLine(15, 17, 15, 17, matrix.Color888(180, 180, 180));
  matrix.drawLine(40, 17, 59, 17, matrix.Color888(180, 180, 180));
  matrix.drawLine(61, 17, 63, 17, matrix.Color888(8, 37, 254));
  matrix.drawLine(3, 18, 3, 18, matrix.Color888(180, 180, 180));
  matrix.drawLine(14, 18, 14, 18, matrix.Color888(180, 180, 180));
  matrix.drawLine(21, 18, 21, 18, matrix.Color888(180, 180, 180));
  matrix.drawLine(40, 18, 59, 18, matrix.Color888(180, 180, 180));
  matrix.drawLine(61, 18, 63, 18, matrix.Color888(8, 37, 254));
  matrix.drawLine(3, 19, 3, 19, matrix.Color888(180, 180, 180));
  matrix.drawLine(10, 19, 10, 19, matrix.Color888(180, 180, 180));
  matrix.drawLine(40, 19, 55, 19, matrix.Color888(180, 180, 180));
  matrix.drawLine(57, 19, 59, 19, matrix.Color888(180, 180, 180));
  matrix.drawLine(61, 19, 63, 19, matrix.Color888(8, 37, 254));
  matrix.drawLine(3, 20, 3, 20, matrix.Color888(180, 180, 180));
  matrix.drawLine(5, 20, 7, 20, matrix.Color888(180, 180, 180));
  matrix.drawLine(10, 20, 10, 20, matrix.Color888(180, 180, 180));
  matrix.drawLine(14, 20, 15, 20, matrix.Color888(180, 180, 180));
  matrix.drawLine(18, 20, 19, 20, matrix.Color888(180, 180, 180));
  matrix.drawLine(21, 20, 21, 20, matrix.Color888(180, 180, 180));
  matrix.drawLine(24, 20, 25, 20, matrix.Color888(180, 180, 180));
  matrix.drawLine(28, 20, 28, 20, matrix.Color888(180, 180, 180));
  matrix.drawLine(31, 20, 31, 20, matrix.Color888(180, 180, 180));
  matrix.drawLine(33, 20, 34, 20, matrix.Color888(180, 180, 180));
  matrix.drawLine(40, 20, 47, 20, matrix.Color888(180, 180, 180));
  matrix.drawLine(52, 20, 54, 20, matrix.Color888(180, 180, 180));
  matrix.drawLine(56, 20, 56, 20, matrix.Color888(180, 180, 180));
  matrix.drawLine(58, 20, 59, 20, matrix.Color888(180, 180, 180));
  matrix.drawLine(61, 20, 63, 20, matrix.Color888(8, 37, 254));
  matrix.drawLine(3, 21, 3, 21, matrix.Color888(180, 180, 180));
  matrix.drawLine(5, 21, 5, 21, matrix.Color888(180, 180, 180));
  matrix.drawLine(8, 21, 8, 21, matrix.Color888(180, 180, 180));
  matrix.drawLine(10, 21, 11, 21, matrix.Color888(180, 180, 180));
  matrix.drawLine(13, 21, 13, 21, matrix.Color888(180, 180, 180));
  matrix.drawLine(16, 21, 16, 21, matrix.Color888(180, 180, 180));
  matrix.drawLine(18, 21, 18, 21, matrix.Color888(180, 180, 180));
  matrix.drawLine(21, 21, 21, 21, matrix.Color888(180, 180, 180));
  matrix.drawLine(23, 21, 23, 21, matrix.Color888(180, 180, 180));
  matrix.drawLine(26, 21, 26, 21, matrix.Color888(180, 180, 180));
  matrix.drawLine(28, 21, 28, 21, matrix.Color888(180, 180, 180));
  matrix.drawLine(31, 21, 31, 21, matrix.Color888(180, 180, 180));
  matrix.drawLine(33, 21, 33, 21, matrix.Color888(180, 180, 180));
  matrix.drawLine(40, 21, 47, 21, matrix.Color888(180, 180, 180));
  matrix.drawLine(49, 21, 50, 21, matrix.Color888(180, 180, 180));
  matrix.drawLine(52, 21, 55, 21, matrix.Color888(180, 180, 180));
  matrix.drawLine(57, 21, 59, 21, matrix.Color888(180, 180, 180));
  matrix.drawLine(61, 21, 63, 21, matrix.Color888(8, 37, 254));
  matrix.drawLine(3, 22, 3, 22, matrix.Color888(180, 180, 180));
  matrix.drawLine(5, 22, 5, 22, matrix.Color888(180, 180, 180));
  matrix.drawLine(8, 22, 8, 22, matrix.Color888(180, 180, 180));
  matrix.drawLine(10, 22, 10, 22, matrix.Color888(180, 180, 180));
  matrix.drawLine(13, 22, 16, 22, matrix.Color888(180, 180, 180));
  matrix.drawLine(18, 22, 18, 22, matrix.Color888(180, 180, 180));
  matrix.drawLine(21, 22, 21, 22, matrix.Color888(180, 180, 180));
  matrix.drawLine(23, 22, 26, 22, matrix.Color888(180, 180, 180));
  matrix.drawLine(28, 22, 28, 22, matrix.Color888(180, 180, 180));
  matrix.drawLine(31, 22, 31, 22, matrix.Color888(180, 180, 180));
  matrix.drawLine(33, 22, 33, 22, matrix.Color888(180, 180, 180));
  matrix.drawLine(40, 22, 47, 22, matrix.Color888(180, 180, 180));
  matrix.drawLine(49, 22, 50, 22, matrix.Color888(180, 180, 180));
  matrix.drawLine(52, 22, 54, 22, matrix.Color888(180, 180, 180));
  matrix.drawLine(58, 22, 59, 22, matrix.Color888(180, 180, 180));
  matrix.drawLine(61, 22, 61, 22, matrix.Color888(3, 15, 101));
  matrix.drawLine(62, 22, 63, 22, matrix.Color888(8, 37, 254));
  matrix.drawLine(3, 23, 3, 23, matrix.Color888(180, 180, 180));
  matrix.drawLine(5, 23, 5, 23, matrix.Color888(180, 180, 180));
  matrix.drawLine(8, 23, 8, 23, matrix.Color888(180, 180, 180));
  matrix.drawLine(10, 23, 10, 23, matrix.Color888(180, 180, 180));
  matrix.drawLine(13, 23, 13, 23, matrix.Color888(180, 180, 180));
  matrix.drawLine(18, 23, 18, 23, matrix.Color888(180, 180, 180));
  matrix.drawLine(21, 23, 21, 23, matrix.Color888(180, 180, 180));
  matrix.drawLine(23, 23, 23, 23, matrix.Color888(180, 180, 180));
  matrix.drawLine(28, 23, 28, 23, matrix.Color888(180, 180, 180));
  matrix.drawLine(30, 23, 31, 23, matrix.Color888(180, 180, 180));
  matrix.drawLine(33, 23, 33, 23, matrix.Color888(180, 180, 180));
  matrix.drawLine(40, 23, 47, 23, matrix.Color888(180, 180, 180));
  matrix.drawLine(49, 23, 50, 23, matrix.Color888(180, 180, 180));
  matrix.drawLine(52, 23, 55, 23, matrix.Color888(180, 180, 180));
  matrix.drawLine(57, 23, 59, 23, matrix.Color888(180, 180, 180));
  matrix.drawLine(62, 23, 62, 23, matrix.Color888(2, 9, 60));
  matrix.drawLine(63, 23, 63, 23, matrix.Color888(7, 32, 220));
  matrix.drawLine(3, 24, 3, 24, matrix.Color888(180, 180, 180));
  matrix.drawLine(5, 24, 5, 24, matrix.Color888(180, 180, 180));
  matrix.drawLine(8, 24, 8, 24, matrix.Color888(180, 180, 180));
  matrix.drawLine(11, 24, 11, 24, matrix.Color888(180, 180, 180));
  matrix.drawLine(14, 24, 16, 24, matrix.Color888(180, 180, 180));
  matrix.drawLine(18, 24, 18, 24, matrix.Color888(180, 180, 180));
  matrix.drawLine(21, 24, 21, 24, matrix.Color888(180, 180, 180));
  matrix.drawLine(24, 24, 26, 24, matrix.Color888(180, 180, 180));
  matrix.drawLine(28, 24, 29, 24, matrix.Color888(180, 180, 180));
  matrix.drawLine(31, 24, 31, 24, matrix.Color888(180, 180, 180));
  matrix.drawLine(33, 24, 33, 24, matrix.Color888(180, 180, 180));
  matrix.drawLine(40, 24, 47, 24, matrix.Color888(180, 180, 180));
  matrix.drawLine(49, 24, 50, 24, matrix.Color888(180, 180, 180));
  matrix.drawLine(52, 24, 54, 24, matrix.Color888(180, 180, 180));
  matrix.drawLine(58, 24, 59, 24, matrix.Color888(180, 180, 180));
  matrix.drawLine(63, 24, 63, 24, matrix.Color888(1, 3, 24));
  matrix.drawLine(40, 25, 47, 25, matrix.Color888(180, 180, 180));
  matrix.drawLine(49, 25, 50, 25, matrix.Color888(180, 180, 180));
  matrix.drawLine(52, 25, 54, 25, matrix.Color888(180, 180, 180));
  matrix.drawLine(56, 25, 56, 25, matrix.Color888(180, 180, 180));
  matrix.drawLine(58, 25, 59, 25, matrix.Color888(180, 180, 180));
  matrix.drawLine(40, 26, 47, 26, matrix.Color888(180, 180, 180));
  matrix.drawLine(49, 26, 50, 26, matrix.Color888(180, 180, 180));
  matrix.drawLine(52, 26, 54, 26, matrix.Color888(180, 180, 180));
  matrix.drawLine(56, 26, 56, 26, matrix.Color888(180, 180, 180));
  matrix.drawLine(58, 26, 59, 26, matrix.Color888(180, 180, 180));
}

void AirExtGPS()
{
  matrix.drawLine(20, 0, 55, 0, matrix.Color888(8, 37, 254));
  matrix.drawLine(56, 0, 56, 0, matrix.Color888(143, 143, 143));
  matrix.drawLine(57, 0, 63, 0, matrix.Color888(8, 37, 254));
  matrix.drawLine(22, 1, 45, 1, matrix.Color888(8, 37, 254));
  matrix.drawLine(46, 1, 46, 1, matrix.Color888(254, 233, 8));
  matrix.drawLine(47, 1, 56, 1, matrix.Color888(8, 37, 254));
  matrix.drawLine(57, 1, 57, 1, matrix.Color888(143, 143, 143));
  matrix.drawLine(58, 1, 63, 1, matrix.Color888(8, 37, 254));
  matrix.drawLine(22, 2, 22, 2, matrix.Color888(0, 1, 5));
  matrix.drawLine(24, 2, 45, 2, matrix.Color888(8, 37, 254));
  matrix.drawLine(46, 2, 46, 2, matrix.Color888(254, 233, 8));
  matrix.drawLine(47, 2, 57, 2, matrix.Color888(8, 37, 254));
  matrix.drawLine(58, 2, 58, 2, matrix.Color888(143, 143, 143));
  matrix.drawLine(59, 2, 60, 2, matrix.Color888(8, 37, 254));
  matrix.drawLine(61, 2, 61, 2, matrix.Color888(143, 143, 143));
  matrix.drawLine(62, 2, 63, 2, matrix.Color888(8, 37, 254));
  matrix.drawLine(24, 3, 24, 3, matrix.Color888(1, 5, 34));
  matrix.drawLine(26, 3, 45, 3, matrix.Color888(8, 37, 254));
  matrix.drawLine(46, 3, 46, 3, matrix.Color888(254, 233, 8));
  matrix.drawLine(47, 3, 58, 3, matrix.Color888(8, 37, 254));
  matrix.drawLine(59, 3, 60, 3, matrix.Color888(143, 143, 143));
  matrix.drawLine(61, 3, 63, 3, matrix.Color888(8, 37, 254));
  matrix.drawLine(27, 4, 45, 4, matrix.Color888(8, 37, 254));
  matrix.drawLine(46, 4, 46, 4, matrix.Color888(254, 233, 8));
  matrix.drawLine(47, 4, 58, 4, matrix.Color888(8, 37, 254));
  matrix.drawLine(59, 4, 60, 4, matrix.Color888(143, 143, 143));
  matrix.drawLine(61, 4, 63, 4, matrix.Color888(8, 37, 254));
  matrix.drawLine(29, 5, 45, 5, matrix.Color888(8, 37, 254));
  matrix.drawLine(46, 5, 46, 5, matrix.Color888(254, 233, 8));
  matrix.drawLine(47, 5, 57, 5, matrix.Color888(8, 37, 254));
  matrix.drawLine(58, 5, 58, 5, matrix.Color888(143, 143, 143));
  matrix.drawLine(59, 5, 60, 5, matrix.Color888(8, 37, 254));
  matrix.drawLine(61, 5, 61, 5, matrix.Color888(143, 143, 143));
  matrix.drawLine(62, 5, 63, 5, matrix.Color888(8, 37, 254));
  matrix.drawLine(14, 6, 14, 6, matrix.Color888(255, 255, 255));
  matrix.drawLine(18, 6, 18, 6, matrix.Color888(255, 255, 255));
  matrix.drawLine(29, 6, 29, 6, matrix.Color888(2, 8, 55));
  matrix.drawLine(30, 6, 45, 6, matrix.Color888(8, 37, 254));
  matrix.drawLine(46, 6, 46, 6, matrix.Color888(254, 233, 8));
  matrix.drawLine(47, 6, 61, 6, matrix.Color888(8, 37, 254));
  matrix.drawLine(62, 6, 62, 6, matrix.Color888(143, 143, 143));
  matrix.drawLine(63, 6, 63, 6, matrix.Color888(8, 37, 254));
  matrix.drawLine(13, 7, 15, 7, matrix.Color888(255, 255, 255));
  matrix.drawLine(32, 7, 45, 7, matrix.Color888(8, 37, 254));
  matrix.drawLine(46, 7, 46, 7, matrix.Color888(254, 233, 8));
  matrix.drawLine(47, 7, 62, 7, matrix.Color888(8, 37, 254));
  matrix.drawLine(63, 7, 63, 7, matrix.Color888(143, 143, 143));
  matrix.drawLine(13, 8, 13, 8, matrix.Color888(255, 255, 255));
  matrix.drawLine(15, 8, 15, 8, matrix.Color888(255, 255, 255));
  matrix.drawLine(18, 8, 18, 8, matrix.Color888(255, 255, 255));
  matrix.drawLine(20, 8, 21, 8, matrix.Color888(255, 255, 255));
  matrix.drawLine(33, 8, 37, 8, matrix.Color888(8, 37, 254));
  matrix.drawLine(38, 8, 38, 8, matrix.Color888(254, 233, 8));
  matrix.drawLine(39, 8, 53, 8, matrix.Color888(8, 37, 254));
  matrix.drawLine(54, 8, 54, 8, matrix.Color888(254, 233, 8));
  matrix.drawLine(55, 8, 63, 8, matrix.Color888(8, 37, 254));
  matrix.drawLine(13, 9, 13, 9, matrix.Color888(255, 255, 255));
  matrix.drawLine(15, 9, 15, 9, matrix.Color888(255, 255, 255));
  matrix.drawLine(18, 9, 18, 9, matrix.Color888(255, 255, 255));
  matrix.drawLine(20, 9, 20, 9, matrix.Color888(255, 255, 255));
  matrix.drawLine(33, 9, 33, 9, matrix.Color888(0, 2, 14));
  matrix.drawLine(35, 9, 38, 9, matrix.Color888(8, 37, 254));
  matrix.drawLine(39, 9, 39, 9, matrix.Color888(254, 233, 8));
  matrix.drawLine(40, 9, 52, 9, matrix.Color888(8, 37, 254));
  matrix.drawLine(53, 9, 53, 9, matrix.Color888(254, 233, 8));
  matrix.drawLine(54, 9, 63, 9, matrix.Color888(8, 37, 254));
  matrix.drawLine(12, 10, 16, 10, matrix.Color888(255, 255, 255));
  matrix.drawLine(18, 10, 18, 10, matrix.Color888(255, 255, 255));
  matrix.drawLine(20, 10, 20, 10, matrix.Color888(255, 255, 255));
  matrix.drawLine(36, 10, 39, 10, matrix.Color888(8, 37, 254));
  matrix.drawLine(40, 10, 40, 10, matrix.Color888(254, 233, 8));
  matrix.drawLine(41, 10, 43, 10, matrix.Color888(8, 37, 254));
  matrix.drawLine(44, 10, 44, 10, matrix.Color888(254, 233, 8));
  matrix.drawLine(45, 10, 45, 10, matrix.Color888(255, 230, 19));
  matrix.drawLine(46, 10, 46, 10, matrix.Color888(255, 232, 10));
  matrix.drawLine(47, 10, 47, 10, matrix.Color888(248, 236, 0));
  matrix.drawLine(48, 10, 48, 10, matrix.Color888(254, 233, 8));
  matrix.drawLine(49, 10, 51, 10, matrix.Color888(8, 37, 254));
  matrix.drawLine(52, 10, 52, 10, matrix.Color888(254, 233, 8));
  matrix.drawLine(53, 10, 63, 10, matrix.Color888(8, 37, 254));
  matrix.drawLine(12, 11, 12, 11, matrix.Color888(255, 255, 255));
  matrix.drawLine(16, 11, 16, 11, matrix.Color888(255, 255, 255));
  matrix.drawLine(18, 11, 18, 11, matrix.Color888(255, 255, 255));
  matrix.drawLine(20, 11, 20, 11, matrix.Color888(255, 255, 255));
  matrix.drawLine(37, 11, 42, 11, matrix.Color888(8, 37, 254));
  matrix.drawLine(43, 11, 48, 11, matrix.Color888(255, 233, 8));
  matrix.drawLine(49, 11, 49, 11, matrix.Color888(255, 232, 8));
  matrix.drawLine(50, 11, 63, 11, matrix.Color888(8, 37, 254));
  matrix.drawLine(12, 12, 12, 12, matrix.Color888(255, 255, 255));
  matrix.drawLine(16, 12, 16, 12, matrix.Color888(255, 255, 255));
  matrix.drawLine(18, 12, 18, 12, matrix.Color888(255, 255, 255));
  matrix.drawLine(20, 12, 20, 12, matrix.Color888(255, 255, 255));
  matrix.drawLine(37, 12, 37, 12, matrix.Color888(1, 6, 40));
  matrix.drawLine(38, 12, 41, 12, matrix.Color888(8, 37, 254));
  matrix.drawLine(42, 12, 42, 12, matrix.Color888(255, 233, 3));
  matrix.drawLine(43, 12, 43, 12, matrix.Color888(255, 233, 6));
  matrix.drawLine(44, 12, 48, 12, matrix.Color888(255, 233, 8));
  matrix.drawLine(49, 12, 49, 12, matrix.Color888(255, 233, 7));
  matrix.drawLine(50, 12, 50, 12, matrix.Color888(252, 233, 11));
  matrix.drawLine(51, 12, 63, 12, matrix.Color888(8, 37, 254));
  matrix.drawLine(38, 13, 38, 13, matrix.Color888(0, 1, 9));
  matrix.drawLine(39, 13, 40, 13, matrix.Color888(8, 37, 254));
  matrix.drawLine(41, 13, 41, 13, matrix.Color888(252, 233, 9));
  matrix.drawLine(42, 13, 48, 13, matrix.Color888(255, 233, 8));
  matrix.drawLine(49, 13, 49, 13, matrix.Color888(255, 233, 0));
  matrix.drawLine(53, 13, 63, 13, matrix.Color888(8, 37, 254));
  matrix.drawLine(40, 14, 40, 14, matrix.Color888(254, 233, 8));
  matrix.drawLine(41, 14, 41, 14, matrix.Color888(252, 235, 12));
  matrix.drawLine(42, 14, 48, 14, matrix.Color888(255, 233, 8));
  matrix.drawLine(50, 14, 52, 14, matrix.Color888(255, 255, 255));
  matrix.drawLine(54, 14, 63, 14, matrix.Color888(8, 37, 254));
  matrix.drawLine(40, 15, 40, 15, matrix.Color888(251, 228, 2));
  matrix.drawLine(41, 15, 41, 15, matrix.Color888(255, 234, 2));
  matrix.drawLine(42, 15, 46, 15, matrix.Color888(255, 233, 8));
  matrix.drawLine(47, 15, 47, 15, matrix.Color888(255, 232, 3));
  matrix.drawLine(49, 15, 53, 15, matrix.Color888(255, 255, 255));
  matrix.drawLine(55, 15, 63, 15, matrix.Color888(8, 37, 254));
  matrix.drawLine(31, 16, 37, 16, matrix.Color888(254, 233, 8));
  matrix.drawLine(39, 16, 39, 16, matrix.Color888(254, 233, 8));
  matrix.drawLine(40, 16, 40, 16, matrix.Color888(255, 228, 23));
  matrix.drawLine(41, 16, 41, 16, matrix.Color888(254, 233, 15));
  matrix.drawLine(42, 16, 42, 16, matrix.Color888(255, 233, 8));
  matrix.drawLine(43, 16, 43, 16, matrix.Color888(254, 233, 8));
  matrix.drawLine(44, 16, 44, 16, matrix.Color888(255, 232, 10));
  matrix.drawLine(45, 16, 45, 16, matrix.Color888(252, 230, 7));
  matrix.drawLine(47, 16, 47, 16, matrix.Color888(249, 228, 37));
  matrix.drawLine(49, 16, 53, 16, matrix.Color888(255, 255, 255));
  matrix.drawLine(55, 16, 60, 16, matrix.Color888(254, 233, 8));
  matrix.drawLine(61, 16, 63, 16, matrix.Color888(8, 37, 254));
  matrix.drawLine(15, 17, 15, 17, matrix.Color888(255, 255, 255));
  matrix.drawLine(40, 17, 40, 17, matrix.Color888(254, 238, 1));
  matrix.drawLine(41, 17, 41, 17, matrix.Color888(255, 232, 10));
  matrix.drawLine(42, 17, 42, 17, matrix.Color888(255, 233, 8));
  matrix.drawLine(43, 17, 43, 17, matrix.Color888(254, 235, 12));
  matrix.drawLine(44, 17, 44, 17, matrix.Color888(246, 240, 41));
  matrix.drawLine(46, 17, 46, 17, matrix.Color888(255, 255, 255));
  matrix.drawLine(48, 17, 48, 17, matrix.Color888(255, 255, 255));
  matrix.drawLine(50, 17, 52, 17, matrix.Color888(255, 255, 255));
  matrix.drawLine(54, 17, 55, 17, matrix.Color888(8, 37, 254));
  matrix.drawLine(57, 17, 63, 17, matrix.Color888(8, 37, 254));
  matrix.drawLine(1, 18, 4, 18, matrix.Color888(255, 255, 255));
  matrix.drawLine(14, 18, 14, 18, matrix.Color888(255, 255, 255));
  matrix.drawLine(21, 18, 21, 18, matrix.Color888(255, 255, 255));
  matrix.drawLine(40, 18, 40, 18, matrix.Color888(247, 233, 43));
  matrix.drawLine(41, 18, 41, 18, matrix.Color888(249, 235, 15));
  matrix.drawLine(42, 18, 42, 18, matrix.Color888(255, 233, 8));
  matrix.drawLine(43, 18, 43, 18, matrix.Color888(247, 228, 0));
  matrix.drawLine(45, 18, 53, 18, matrix.Color888(255, 255, 255));
  matrix.drawLine(56, 18, 56, 18, matrix.Color888(255, 255, 255));
  matrix.drawLine(59, 18, 63, 18, matrix.Color888(8, 37, 254));
  matrix.drawLine(1, 19, 1, 19, matrix.Color888(255, 255, 255));
  matrix.drawLine(10, 19, 10, 19, matrix.Color888(255, 255, 255));
  matrix.drawLine(41, 19, 42, 19, matrix.Color888(253, 231, 6));
  matrix.drawLine(44, 19, 57, 19, matrix.Color888(255, 255, 255));
  matrix.drawLine(60, 19, 63, 19, matrix.Color888(8, 37, 254));
  matrix.drawLine(1, 20, 1, 20, matrix.Color888(255, 255, 255));
  matrix.drawLine(6, 20, 6, 20, matrix.Color888(255, 255, 255));
  matrix.drawLine(8, 20, 8, 20, matrix.Color888(255, 255, 255));
  matrix.drawLine(10, 20, 10, 20, matrix.Color888(255, 255, 255));
  matrix.drawLine(14, 20, 15, 20, matrix.Color888(255, 255, 255));
  matrix.drawLine(18, 20, 19, 20, matrix.Color888(255, 255, 255));
  matrix.drawLine(21, 20, 21, 20, matrix.Color888(255, 255, 255));
  matrix.drawLine(24, 20, 25, 20, matrix.Color888(255, 255, 255));
  matrix.drawLine(28, 20, 28, 20, matrix.Color888(255, 255, 255));
  matrix.drawLine(31, 20, 31, 20, matrix.Color888(255, 255, 255));
  matrix.drawLine(33, 20, 34, 20, matrix.Color888(255, 255, 255));
  matrix.drawLine(41, 20, 41, 20, matrix.Color888(254, 233, 8));
  matrix.drawLine(42, 20, 42, 20, matrix.Color888(255, 229, 3));
  matrix.drawLine(43, 20, 43, 20, matrix.Color888(252, 226, 10));
  matrix.drawLine(45, 20, 58, 20, matrix.Color888(255, 255, 255));
  matrix.drawLine(60, 20, 63, 20, matrix.Color888(8, 37, 254));
  matrix.drawLine(1, 21, 3, 21, matrix.Color888(255, 255, 255));
  matrix.drawLine(6, 21, 8, 21, matrix.Color888(255, 255, 255));
  matrix.drawLine(10, 21, 11, 21, matrix.Color888(255, 255, 255));
  matrix.drawLine(13, 21, 13, 21, matrix.Color888(255, 255, 255));
  matrix.drawLine(16, 21, 16, 21, matrix.Color888(255, 255, 255));
  matrix.drawLine(18, 21, 18, 21, matrix.Color888(255, 255, 255));
  matrix.drawLine(21, 21, 21, 21, matrix.Color888(255, 255, 255));
  matrix.drawLine(23, 21, 23, 21, matrix.Color888(255, 255, 255));
  matrix.drawLine(26, 21, 26, 21, matrix.Color888(255, 255, 255));
  matrix.drawLine(28, 21, 28, 21, matrix.Color888(255, 255, 255));
  matrix.drawLine(31, 21, 31, 21, matrix.Color888(255, 255, 255));
  matrix.drawLine(33, 21, 33, 21, matrix.Color888(255, 255, 255));
  matrix.drawLine(42, 21, 42, 21, matrix.Color888(254, 233, 8));
  matrix.drawLine(43, 21, 43, 21, matrix.Color888(252, 231, 1));
  matrix.drawLine(44, 21, 44, 21, matrix.Color888(254, 233, 8));
  matrix.drawLine(46, 21, 58, 21, matrix.Color888(255, 255, 255));
  matrix.drawLine(60, 21, 63, 21, matrix.Color888(8, 37, 254));
  matrix.drawLine(1, 22, 1, 22, matrix.Color888(255, 255, 255));
  matrix.drawLine(7, 22, 7, 22, matrix.Color888(255, 255, 255));
  matrix.drawLine(10, 22, 10, 22, matrix.Color888(255, 255, 255));
  matrix.drawLine(13, 22, 16, 22, matrix.Color888(255, 255, 255));
  matrix.drawLine(18, 22, 18, 22, matrix.Color888(255, 255, 255));
  matrix.drawLine(21, 22, 21, 22, matrix.Color888(255, 255, 255));
  matrix.drawLine(23, 22, 26, 22, matrix.Color888(255, 255, 255));
  matrix.drawLine(28, 22, 28, 22, matrix.Color888(255, 255, 255));
  matrix.drawLine(31, 22, 31, 22, matrix.Color888(255, 255, 255));
  matrix.drawLine(33, 22, 33, 22, matrix.Color888(255, 255, 255));
  matrix.drawLine(40, 22, 40, 22, matrix.Color888(254, 233, 8));
  matrix.drawLine(45, 22, 57, 22, matrix.Color888(255, 255, 255));
  matrix.drawLine(59, 22, 59, 22, matrix.Color888(255, 255, 255));
  matrix.drawLine(62, 22, 63, 22, matrix.Color888(8, 37, 254));
  matrix.drawLine(1, 23, 1, 23, matrix.Color888(255, 255, 255));
  matrix.drawLine(6, 23, 8, 23, matrix.Color888(255, 255, 255));
  matrix.drawLine(10, 23, 10, 23, matrix.Color888(255, 255, 255));
  matrix.drawLine(13, 23, 13, 23, matrix.Color888(255, 255, 255));
  matrix.drawLine(18, 23, 18, 23, matrix.Color888(255, 255, 255));
  matrix.drawLine(21, 23, 21, 23, matrix.Color888(255, 255, 255));
  matrix.drawLine(23, 23, 23, 23, matrix.Color888(255, 255, 255));
  matrix.drawLine(28, 23, 28, 23, matrix.Color888(255, 255, 255));
  matrix.drawLine(30, 23, 31, 23, matrix.Color888(255, 255, 255));
  matrix.drawLine(33, 23, 33, 23, matrix.Color888(255, 255, 255));
  matrix.drawLine(39, 23, 39, 23, matrix.Color888(254, 233, 8));
  matrix.drawLine(44, 23, 61, 23, matrix.Color888(255, 255, 255));
  matrix.drawLine(63, 23, 63, 23, matrix.Color888(8, 37, 254));
  matrix.drawLine(1, 24, 4, 24, matrix.Color888(255, 255, 255));
  matrix.drawLine(6, 24, 6, 24, matrix.Color888(255, 255, 255));
  matrix.drawLine(8, 24, 8, 24, matrix.Color888(255, 255, 255));
  matrix.drawLine(11, 24, 11, 24, matrix.Color888(255, 255, 255));
  matrix.drawLine(14, 24, 16, 24, matrix.Color888(255, 255, 255));
  matrix.drawLine(18, 24, 18, 24, matrix.Color888(255, 255, 255));
  matrix.drawLine(21, 24, 21, 24, matrix.Color888(255, 255, 255));
  matrix.drawLine(24, 24, 26, 24, matrix.Color888(255, 255, 255));
  matrix.drawLine(28, 24, 29, 24, matrix.Color888(255, 255, 255));
  matrix.drawLine(31, 24, 31, 24, matrix.Color888(255, 255, 255));
  matrix.drawLine(33, 24, 33, 24, matrix.Color888(255, 255, 255));
  matrix.drawLine(38, 24, 38, 24, matrix.Color888(254, 233, 8));
  matrix.drawLine(43, 24, 62, 24, matrix.Color888(255, 255, 255));
  matrix.drawLine(43, 25, 62, 25, matrix.Color888(255, 255, 255));
  matrix.drawLine(44, 26, 61, 26, matrix.Color888(255, 255, 255));
  matrix.drawLine(63, 26, 63, 26, matrix.Color888(8, 37, 254));
  matrix.drawLine(63, 27, 63, 27, matrix.Color888(8, 37, 254));
  matrix.drawLine(46, 28, 46, 28, matrix.Color888(254, 233, 8));
  matrix.drawLine(53, 28, 63, 28, matrix.Color888(8, 37, 254));
  matrix.drawLine(46, 29, 46, 29, matrix.Color888(254, 233, 8));
  matrix.drawLine(54, 29, 63, 29, matrix.Color888(8, 37, 254));
  matrix.drawLine(46, 30, 46, 30, matrix.Color888(254, 233, 8));
  matrix.drawLine(54, 30, 63, 30, matrix.Color888(8, 37, 254));
  matrix.drawLine(55, 31, 63, 31, matrix.Color888(8, 37, 254));
}

void AirExt()
{
  matrix.drawLine(20, 0, 63, 0, matrix.Color888(8, 37, 254));
  matrix.drawLine(22, 1, 45, 1, matrix.Color888(8, 37, 254));
  matrix.drawLine(46, 1, 46, 1, matrix.Color888(254, 233, 8));
  matrix.drawLine(47, 1, 63, 1, matrix.Color888(8, 37, 254));
  matrix.drawLine(22, 2, 22, 2, matrix.Color888(0, 1, 5));
  matrix.drawLine(24, 2, 45, 2, matrix.Color888(8, 37, 254));
  matrix.drawLine(46, 2, 46, 2, matrix.Color888(254, 233, 8));
  matrix.drawLine(47, 2, 63, 2, matrix.Color888(8, 37, 254));
  matrix.drawLine(24, 3, 24, 3, matrix.Color888(1, 5, 34));
  matrix.drawLine(26, 3, 45, 3, matrix.Color888(8, 37, 254));
  matrix.drawLine(46, 3, 46, 3, matrix.Color888(254, 233, 8));
  matrix.drawLine(47, 3, 63, 3, matrix.Color888(8, 37, 254));
  matrix.drawLine(27, 4, 45, 4, matrix.Color888(8, 37, 254));
  matrix.drawLine(46, 4, 46, 4, matrix.Color888(254, 233, 8));
  matrix.drawLine(47, 4, 63, 4, matrix.Color888(8, 37, 254));
  matrix.drawLine(29, 5, 45, 5, matrix.Color888(8, 37, 254));
  matrix.drawLine(46, 5, 46, 5, matrix.Color888(254, 233, 8));
  matrix.drawLine(47, 5, 63, 5, matrix.Color888(8, 37, 254));
  matrix.drawLine(14, 6, 14, 6, matrix.Color888(255, 255, 255));
  matrix.drawLine(18, 6, 18, 6, matrix.Color888(255, 255, 255));
  matrix.drawLine(29, 6, 29, 6, matrix.Color888(2, 8, 55));
  matrix.drawLine(30, 6, 45, 6, matrix.Color888(8, 37, 254));
  matrix.drawLine(46, 6, 46, 6, matrix.Color888(254, 233, 8));
  matrix.drawLine(47, 6, 63, 6, matrix.Color888(8, 37, 254));
  matrix.drawLine(13, 7, 15, 7, matrix.Color888(255, 255, 255));
  matrix.drawLine(32, 7, 45, 7, matrix.Color888(8, 37, 254));
  matrix.drawLine(46, 7, 46, 7, matrix.Color888(254, 233, 8));
  matrix.drawLine(47, 7, 63, 7, matrix.Color888(8, 37, 254));
  matrix.drawLine(13, 8, 13, 8, matrix.Color888(255, 255, 255));
  matrix.drawLine(15, 8, 15, 8, matrix.Color888(255, 255, 255));
  matrix.drawLine(18, 8, 18, 8, matrix.Color888(255, 255, 255));
  matrix.drawLine(20, 8, 21, 8, matrix.Color888(255, 255, 255));
  matrix.drawLine(33, 8, 37, 8, matrix.Color888(8, 37, 254));
  matrix.drawLine(38, 8, 38, 8, matrix.Color888(254, 233, 8));
  matrix.drawLine(39, 8, 53, 8, matrix.Color888(8, 37, 254));
  matrix.drawLine(54, 8, 54, 8, matrix.Color888(254, 233, 8));
  matrix.drawLine(55, 8, 63, 8, matrix.Color888(8, 37, 254));
  matrix.drawLine(13, 9, 13, 9, matrix.Color888(255, 255, 255));
  matrix.drawLine(15, 9, 15, 9, matrix.Color888(255, 255, 255));
  matrix.drawLine(18, 9, 18, 9, matrix.Color888(255, 255, 255));
  matrix.drawLine(20, 9, 20, 9, matrix.Color888(255, 255, 255));
  matrix.drawLine(33, 9, 33, 9, matrix.Color888(0, 2, 14));
  matrix.drawLine(35, 9, 38, 9, matrix.Color888(8, 37, 254));
  matrix.drawLine(39, 9, 39, 9, matrix.Color888(254, 233, 8));
  matrix.drawLine(40, 9, 52, 9, matrix.Color888(8, 37, 254));
  matrix.drawLine(53, 9, 53, 9, matrix.Color888(254, 233, 8));
  matrix.drawLine(54, 9, 63, 9, matrix.Color888(8, 37, 254));
  matrix.drawLine(12, 10, 16, 10, matrix.Color888(255, 255, 255));
  matrix.drawLine(18, 10, 18, 10, matrix.Color888(255, 255, 255));
  matrix.drawLine(20, 10, 20, 10, matrix.Color888(255, 255, 255));
  matrix.drawLine(36, 10, 39, 10, matrix.Color888(8, 37, 254));
  matrix.drawLine(40, 10, 40, 10, matrix.Color888(254, 233, 8));
  matrix.drawLine(41, 10, 43, 10, matrix.Color888(8, 37, 254));
  matrix.drawLine(44, 10, 44, 10, matrix.Color888(254, 233, 8));
  matrix.drawLine(45, 10, 45, 10, matrix.Color888(255, 230, 19));
  matrix.drawLine(46, 10, 46, 10, matrix.Color888(255, 232, 10));
  matrix.drawLine(47, 10, 47, 10, matrix.Color888(248, 236, 0));
  matrix.drawLine(48, 10, 48, 10, matrix.Color888(254, 233, 8));
  matrix.drawLine(49, 10, 51, 10, matrix.Color888(8, 37, 254));
  matrix.drawLine(52, 10, 52, 10, matrix.Color888(254, 233, 8));
  matrix.drawLine(53, 10, 63, 10, matrix.Color888(8, 37, 254));
  matrix.drawLine(12, 11, 12, 11, matrix.Color888(255, 255, 255));
  matrix.drawLine(16, 11, 16, 11, matrix.Color888(255, 255, 255));
  matrix.drawLine(18, 11, 18, 11, matrix.Color888(255, 255, 255));
  matrix.drawLine(20, 11, 20, 11, matrix.Color888(255, 255, 255));
  matrix.drawLine(37, 11, 42, 11, matrix.Color888(8, 37, 254));
  matrix.drawLine(43, 11, 48, 11, matrix.Color888(255, 233, 8));
  matrix.drawLine(49, 11, 49, 11, matrix.Color888(255, 232, 8));
  matrix.drawLine(50, 11, 63, 11, matrix.Color888(8, 37, 254));
  matrix.drawLine(12, 12, 12, 12, matrix.Color888(255, 255, 255));
  matrix.drawLine(16, 12, 16, 12, matrix.Color888(255, 255, 255));
  matrix.drawLine(18, 12, 18, 12, matrix.Color888(255, 255, 255));
  matrix.drawLine(20, 12, 20, 12, matrix.Color888(255, 255, 255));
  matrix.drawLine(37, 12, 37, 12, matrix.Color888(1, 6, 40));
  matrix.drawLine(38, 12, 41, 12, matrix.Color888(8, 37, 254));
  matrix.drawLine(42, 12, 42, 12, matrix.Color888(255, 233, 3));
  matrix.drawLine(43, 12, 43, 12, matrix.Color888(255, 233, 6));
  matrix.drawLine(44, 12, 48, 12, matrix.Color888(255, 233, 8));
  matrix.drawLine(49, 12, 49, 12, matrix.Color888(255, 233, 7));
  matrix.drawLine(50, 12, 50, 12, matrix.Color888(252, 233, 11));
  matrix.drawLine(51, 12, 63, 12, matrix.Color888(8, 37, 254));
  matrix.drawLine(38, 13, 38, 13, matrix.Color888(0, 1, 9));
  matrix.drawLine(39, 13, 40, 13, matrix.Color888(8, 37, 254));
  matrix.drawLine(41, 13, 41, 13, matrix.Color888(252, 233, 9));
  matrix.drawLine(42, 13, 48, 13, matrix.Color888(255, 233, 8));
  matrix.drawLine(49, 13, 49, 13, matrix.Color888(255, 233, 0));
  matrix.drawLine(53, 13, 63, 13, matrix.Color888(8, 37, 254));
  matrix.drawLine(40, 14, 40, 14, matrix.Color888(254, 233, 8));
  matrix.drawLine(41, 14, 41, 14, matrix.Color888(252, 235, 12));
  matrix.drawLine(42, 14, 48, 14, matrix.Color888(255, 233, 8));
  matrix.drawLine(50, 14, 52, 14, matrix.Color888(255, 255, 255));
  matrix.drawLine(54, 14, 63, 14, matrix.Color888(8, 37, 254));
  matrix.drawLine(40, 15, 40, 15, matrix.Color888(251, 228, 2));
  matrix.drawLine(41, 15, 41, 15, matrix.Color888(255, 234, 2));
  matrix.drawLine(42, 15, 46, 15, matrix.Color888(255, 233, 8));
  matrix.drawLine(47, 15, 47, 15, matrix.Color888(255, 232, 3));
  matrix.drawLine(49, 15, 53, 15, matrix.Color888(255, 255, 255));
  matrix.drawLine(55, 15, 63, 15, matrix.Color888(8, 37, 254));
  matrix.drawLine(31, 16, 37, 16, matrix.Color888(254, 233, 8));
  matrix.drawLine(39, 16, 39, 16, matrix.Color888(254, 233, 8));
  matrix.drawLine(40, 16, 40, 16, matrix.Color888(255, 228, 23));
  matrix.drawLine(41, 16, 41, 16, matrix.Color888(254, 233, 15));
  matrix.drawLine(42, 16, 42, 16, matrix.Color888(255, 233, 8));
  matrix.drawLine(43, 16, 43, 16, matrix.Color888(254, 233, 8));
  matrix.drawLine(44, 16, 44, 16, matrix.Color888(255, 232, 10));
  matrix.drawLine(45, 16, 45, 16, matrix.Color888(252, 230, 7));
  matrix.drawLine(47, 16, 47, 16, matrix.Color888(249, 228, 37));
  matrix.drawLine(49, 16, 53, 16, matrix.Color888(255, 255, 255));
  matrix.drawLine(55, 16, 60, 16, matrix.Color888(254, 233, 8));
  matrix.drawLine(61, 16, 63, 16, matrix.Color888(8, 37, 254));
  matrix.drawLine(15, 17, 15, 17, matrix.Color888(255, 255, 255));
  matrix.drawLine(40, 17, 40, 17, matrix.Color888(254, 238, 1));
  matrix.drawLine(41, 17, 41, 17, matrix.Color888(255, 232, 10));
  matrix.drawLine(42, 17, 42, 17, matrix.Color888(255, 233, 8));
  matrix.drawLine(43, 17, 43, 17, matrix.Color888(254, 235, 12));
  matrix.drawLine(44, 17, 44, 17, matrix.Color888(246, 240, 41));
  matrix.drawLine(46, 17, 46, 17, matrix.Color888(255, 255, 255));
  matrix.drawLine(48, 17, 48, 17, matrix.Color888(255, 255, 255));
  matrix.drawLine(50, 17, 52, 17, matrix.Color888(255, 255, 255));
  matrix.drawLine(54, 17, 55, 17, matrix.Color888(8, 37, 254));
  matrix.drawLine(57, 17, 63, 17, matrix.Color888(8, 37, 254));
  matrix.drawLine(1, 18, 4, 18, matrix.Color888(255, 255, 255));
  matrix.drawLine(14, 18, 14, 18, matrix.Color888(255, 255, 255));
  matrix.drawLine(21, 18, 21, 18, matrix.Color888(255, 255, 255));
  matrix.drawLine(40, 18, 40, 18, matrix.Color888(247, 233, 43));
  matrix.drawLine(41, 18, 41, 18, matrix.Color888(249, 235, 15));
  matrix.drawLine(42, 18, 42, 18, matrix.Color888(255, 233, 8));
  matrix.drawLine(43, 18, 43, 18, matrix.Color888(247, 228, 0));
  matrix.drawLine(45, 18, 53, 18, matrix.Color888(255, 255, 255));
  matrix.drawLine(56, 18, 56, 18, matrix.Color888(255, 255, 255));
  matrix.drawLine(59, 18, 63, 18, matrix.Color888(8, 37, 254));
  matrix.drawLine(1, 19, 1, 19, matrix.Color888(255, 255, 255));
  matrix.drawLine(10, 19, 10, 19, matrix.Color888(255, 255, 255));
  matrix.drawLine(41, 19, 42, 19, matrix.Color888(253, 231, 6));
  matrix.drawLine(44, 19, 57, 19, matrix.Color888(255, 255, 255));
  matrix.drawLine(60, 19, 63, 19, matrix.Color888(8, 37, 254));
  matrix.drawLine(1, 20, 1, 20, matrix.Color888(255, 255, 255));
  matrix.drawLine(6, 20, 6, 20, matrix.Color888(255, 255, 255));
  matrix.drawLine(8, 20, 8, 20, matrix.Color888(255, 255, 255));
  matrix.drawLine(10, 20, 10, 20, matrix.Color888(255, 255, 255));
  matrix.drawLine(14, 20, 15, 20, matrix.Color888(255, 255, 255));
  matrix.drawLine(18, 20, 19, 20, matrix.Color888(255, 255, 255));
  matrix.drawLine(21, 20, 21, 20, matrix.Color888(255, 255, 255));
  matrix.drawLine(24, 20, 25, 20, matrix.Color888(255, 255, 255));
  matrix.drawLine(28, 20, 28, 20, matrix.Color888(255, 255, 255));
  matrix.drawLine(31, 20, 31, 20, matrix.Color888(255, 255, 255));
  matrix.drawLine(33, 20, 34, 20, matrix.Color888(255, 255, 255));
  matrix.drawLine(41, 20, 41, 20, matrix.Color888(254, 233, 8));
  matrix.drawLine(42, 20, 42, 20, matrix.Color888(255, 229, 3));
  matrix.drawLine(43, 20, 43, 20, matrix.Color888(252, 226, 10));
  matrix.drawLine(45, 20, 58, 20, matrix.Color888(255, 255, 255));
  matrix.drawLine(60, 20, 63, 20, matrix.Color888(8, 37, 254));
  matrix.drawLine(1, 21, 3, 21, matrix.Color888(255, 255, 255));
  matrix.drawLine(6, 21, 8, 21, matrix.Color888(255, 255, 255));
  matrix.drawLine(10, 21, 11, 21, matrix.Color888(255, 255, 255));
  matrix.drawLine(13, 21, 13, 21, matrix.Color888(255, 255, 255));
  matrix.drawLine(16, 21, 16, 21, matrix.Color888(255, 255, 255));
  matrix.drawLine(18, 21, 18, 21, matrix.Color888(255, 255, 255));
  matrix.drawLine(21, 21, 21, 21, matrix.Color888(255, 255, 255));
  matrix.drawLine(23, 21, 23, 21, matrix.Color888(255, 255, 255));
  matrix.drawLine(26, 21, 26, 21, matrix.Color888(255, 255, 255));
  matrix.drawLine(28, 21, 28, 21, matrix.Color888(255, 255, 255));
  matrix.drawLine(31, 21, 31, 21, matrix.Color888(255, 255, 255));
  matrix.drawLine(33, 21, 33, 21, matrix.Color888(255, 255, 255));
  matrix.drawLine(42, 21, 42, 21, matrix.Color888(254, 233, 8));
  matrix.drawLine(43, 21, 43, 21, matrix.Color888(252, 231, 1));
  matrix.drawLine(44, 21, 44, 21, matrix.Color888(254, 233, 8));
  matrix.drawLine(46, 21, 58, 21, matrix.Color888(255, 255, 255));
  matrix.drawLine(60, 21, 63, 21, matrix.Color888(8, 37, 254));
  matrix.drawLine(1, 22, 1, 22, matrix.Color888(255, 255, 255));
  matrix.drawLine(7, 22, 7, 22, matrix.Color888(255, 255, 255));
  matrix.drawLine(10, 22, 10, 22, matrix.Color888(255, 255, 255));
  matrix.drawLine(13, 22, 16, 22, matrix.Color888(255, 255, 255));
  matrix.drawLine(18, 22, 18, 22, matrix.Color888(255, 255, 255));
  matrix.drawLine(21, 22, 21, 22, matrix.Color888(255, 255, 255));
  matrix.drawLine(23, 22, 26, 22, matrix.Color888(255, 255, 255));
  matrix.drawLine(28, 22, 28, 22, matrix.Color888(255, 255, 255));
  matrix.drawLine(31, 22, 31, 22, matrix.Color888(255, 255, 255));
  matrix.drawLine(33, 22, 33, 22, matrix.Color888(255, 255, 255));
  matrix.drawLine(40, 22, 40, 22, matrix.Color888(254, 233, 8));
  matrix.drawLine(45, 22, 57, 22, matrix.Color888(255, 255, 255));
  matrix.drawLine(59, 22, 59, 22, matrix.Color888(255, 255, 255));
  matrix.drawLine(62, 22, 63, 22, matrix.Color888(8, 37, 254));
  matrix.drawLine(1, 23, 1, 23, matrix.Color888(255, 255, 255));
  matrix.drawLine(6, 23, 8, 23, matrix.Color888(255, 255, 255));
  matrix.drawLine(10, 23, 10, 23, matrix.Color888(255, 255, 255));
  matrix.drawLine(13, 23, 13, 23, matrix.Color888(255, 255, 255));
  matrix.drawLine(18, 23, 18, 23, matrix.Color888(255, 255, 255));
  matrix.drawLine(21, 23, 21, 23, matrix.Color888(255, 255, 255));
  matrix.drawLine(23, 23, 23, 23, matrix.Color888(255, 255, 255));
  matrix.drawLine(28, 23, 28, 23, matrix.Color888(255, 255, 255));
  matrix.drawLine(30, 23, 31, 23, matrix.Color888(255, 255, 255));
  matrix.drawLine(33, 23, 33, 23, matrix.Color888(255, 255, 255));
  matrix.drawLine(39, 23, 39, 23, matrix.Color888(254, 233, 8));
  matrix.drawLine(44, 23, 61, 23, matrix.Color888(255, 255, 255));
  matrix.drawLine(63, 23, 63, 23, matrix.Color888(8, 37, 254));
  matrix.drawLine(1, 24, 4, 24, matrix.Color888(255, 255, 255));
  matrix.drawLine(6, 24, 6, 24, matrix.Color888(255, 255, 255));
  matrix.drawLine(8, 24, 8, 24, matrix.Color888(255, 255, 255));
  matrix.drawLine(11, 24, 11, 24, matrix.Color888(255, 255, 255));
  matrix.drawLine(14, 24, 16, 24, matrix.Color888(255, 255, 255));
  matrix.drawLine(18, 24, 18, 24, matrix.Color888(255, 255, 255));
  matrix.drawLine(21, 24, 21, 24, matrix.Color888(255, 255, 255));
  matrix.drawLine(24, 24, 26, 24, matrix.Color888(255, 255, 255));
  matrix.drawLine(28, 24, 29, 24, matrix.Color888(255, 255, 255));
  matrix.drawLine(31, 24, 31, 24, matrix.Color888(255, 255, 255));
  matrix.drawLine(33, 24, 33, 24, matrix.Color888(255, 255, 255));
  matrix.drawLine(38, 24, 38, 24, matrix.Color888(254, 233, 8));
  matrix.drawLine(43, 24, 62, 24, matrix.Color888(255, 255, 255));
  matrix.drawLine(43, 25, 62, 25, matrix.Color888(255, 255, 255));
  matrix.drawLine(44, 26, 61, 26, matrix.Color888(255, 255, 255));
  matrix.drawLine(63, 26, 63, 26, matrix.Color888(8, 37, 254));
  matrix.drawLine(63, 27, 63, 27, matrix.Color888(8, 37, 254));
  matrix.drawLine(46, 28, 46, 28, matrix.Color888(254, 233, 8));
  matrix.drawLine(53, 28, 63, 28, matrix.Color888(8, 37, 254));
  matrix.drawLine(46, 29, 46, 29, matrix.Color888(254, 233, 8));
  matrix.drawLine(54, 29, 63, 29, matrix.Color888(8, 37, 254));
  matrix.drawLine(46, 30, 46, 30, matrix.Color888(254, 233, 8));
  matrix.drawLine(54, 30, 63, 30, matrix.Color888(8, 37, 254));
  matrix.drawLine(55, 31, 63, 31, matrix.Color888(8, 37, 254));
}

