#include "TinyGPS.h"
///////////////////////////////////////////////////////////////////////////////////////
TinyGPS gps;

//Variables de configuracion para el arduino mega y el gps shield
#define GPS_TX_DIGITAL_OUT_PIN 5
#define GPS_RX_DIGITAL_OUT_PIN 6
// El ajuste al reloj mundial deacuerdo a Chile
#define UTC_OFFSET -4 
///////////////////////////////////////////////////////////////////////////////////////
// Estructura de datos que almacena los ajustes al reloj mundial, deacuerdo a Chile
struct dateAjust
{
  byte day;
  byte hour;
  byte month;
  int year;
};
typedef struct dateAjust DateAjust;
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
// Variables Globales
DateAjust temp; // Estrcutura definida anteriormente
long startMillis;
long secondsToFirstLocation = 0;
#define DEBUG

float latitude = 0.0;
float longitude = 0.0;
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
// Funciones
void setup()
{
  // Aqui se define la salida hacia el puerto serial
#ifdef DEBUG
  Serial.begin(19200);
#endif

  // Serial1 es el GPS  
  Serial1.begin(9600);

  // Se configuran los pines que se comunican con el GPS
  pinMode(GPS_TX_DIGITAL_OUT_PIN, INPUT);
  pinMode(GPS_RX_DIGITAL_OUT_PIN, INPUT);
  // Esto es para contabilizar cuanto se demora en hacer la conexin con el GPS
  startMillis = millis();
  Serial.println("Iniciado Conexion con GPS");
}

void loop()
{
  readLocation();
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Funcion que contabiliza los dias que tiene el mes y año que se le indica 
int numbersOfDays(byte month, int year){  
  int numberOfDays;  
  if (month == 4 || month == 6 || month == 9 || month == 11)  
    numberOfDays = 30;  
  else if (month == 2)  
  { 
    bool isLeapYear = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);  
    if (isLeapYear)  
      numberOfDays = 29;  
    else  
      numberOfDays = 28;  
  }  
  else  
    numberOfDays = 31;

  return numberOfDays;  
}
////////////////////////////////////////////////////////////////////////////////////////////////
// Funcion que ajusta la fecha y hora global, a la hora local definida por UTC_OFFSET
void ajustToLocalTime(byte day , byte hour, byte month, int year){
  int monthTemp = month -1;
  if(monthTemp <= 0){
    monthTemp = 12;
  }
  int numbersOfDaysTemp = numbersOfDays(monthTemp,year);
  int temp_hour = hour + UTC_OFFSET;
  if(temp_hour < 0){
    int dayTemp = day-1;
    hour = 24 + temp_hour;
    if(dayTemp <= 0){
      day = numbersOfDaysTemp - dayTemp;
      int monthTemp = month - 1;
      month = monthTemp;
      if(monthTemp <=0){
        month = 12;
        year = year - 1;
      }
    }
    else{
      day = dayTemp;
    }
  }
  else{
    hour = temp_hour;
  }
  temp.month = month;
  temp.day = day;
  temp.year = year;
  temp.hour = hour;
}
/////////////////////////////////////////////////////////////////////////////////////////////
/// Funcion que se encarga de obtener y parsear la informacion obtenida desde el GPS y llamar
/// los ajustes necesarios
void readLocation(){
  bool newData = false;
  unsigned long chars = 0;
  unsigned short sentences, failed;
  // Se hace un total de 1000 lecturas desde el GPS
  for (unsigned long start = millis(); millis() - start < 1000;)
  {
    while (Serial1.available())
    {
      int c = Serial1.read();
      //Serial.print((char)c); //Descomentar para ver los datos que vienen desde las tramas del GPS 
      ++chars;
      if (gps.encode(c)) // Si viene algun dato desde el GPS
        newData = true;
    }
  }

  if (newData)
  {
    // Se contabiliza cuando demora en hacer la contexin
    if(secondsToFirstLocation == 0){
      secondsToFirstLocation = (millis() - startMillis) / 1000;
      Serial.print("Acquired in:");
      Serial.print(secondsToFirstLocation);
      Serial.println("s");
    }
    
    //// Obtencion de los datos del GPS
    unsigned long age;
    gps.f_get_position(&latitude, &longitude, &age); // Posicion
    //
    int year;
    byte month, day, hour, minute, second, hundredths;
    gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &age);// Fecha y hora
    // Ajuste = byte day , byte hour, byte month, int year
    ajustToLocalTime(day, hour, month, year);
    month=temp.month;
    day=temp.day;
    year=temp.year;
    hour=temp.hour;
    //
    char sz[32];
    sprintf(sz, "%02d/%02d/%02d %02d:%02d:%02d ",
    month, day, year, hour, minute, second);
    latitude == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : latitude;
    longitude == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : longitude;
    //
    // Los siguientes if verifican la validez de la informacion recibida por el GPS (A para valido, V para invalido)
    if (age == TinyGPS::GPS_INVALID_AGE)
      Serial.println("No fix detected");
    else if (age > 5000)
      Serial.println("Warning: possible stale data!");
    else{
      Serial.println("Data is current.");
      Serial.print("Location: ");
      Serial.print(latitude, 6);
      Serial.print(" , ");
      Serial.print(longitude, 6);
      Serial.println("");
      Serial.print(sz);
      Serial.println("");
      //INSTRUCCIONES
      // El controlador envia un prompt para que verifique la conexion.
      // prompt:
      // <Siemens
      // <
      // <
      // Luego de eso se envian los comandos, por el puerto serial.
      // PME=249/CR
      // TOD=DDMMAA/CR
      // TOD=HH:MM:SS/CR
    }
  }

  if (chars == 0){
    // Envia un aviso si es que no se esta recibiendo informacion
    Serial.println("Check wiring");
  }
  else if(secondsToFirstLocation == 0){
    // still working
  }
}


