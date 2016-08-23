#include "TinyGPS.h"
#include <SoftwareSerial.h>
#include <TimerOne.h>
///////////////////////////////////////////////////////////////////////////////////////
TinyGPS gps;
SoftwareSerial rs232(2, 3); // RX, TX // Pines de conexion rs232
//Variables de configuracion para el arduino mega y el gps shield
#define GPS_TX_DIGITAL_OUT_PIN 5
#define GPS_RX_DIGITAL_OUT_PIN 6
// El ajuste al reloj mundial deacuerdo a Chile
#define UTC_OFFSET -4 
// Variables 
#define BAUDARDUINO 9600
#define BAUDSERIAL 1200
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
long secondsToFirstLocation = 0;
//#define DEBUG
#define PROD

float latitude = 0.0;
float longitude = 0.0;

String gps_global_prev = "null";
String gps_global_new = "null";
int count = 0;
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
// Funciones
void setup()
{
  // Aqui se define la salida hacia el puerto serial
#ifdef DEBUG
  Serial.begin(BAUDSERIAL);
#endif

#ifdef PROD
  rs232.begin(BAUDSERIAL);
#endif

  // Serial1 es el GPS  
  Serial1.begin(BAUDARDUINO);
  Serial.begin(BAUDSERIAL);

  // Se configuran los pines que se comunican con el GPS
  pinMode(GPS_TX_DIGITAL_OUT_PIN, INPUT);
  pinMode(GPS_RX_DIGITAL_OUT_PIN, OUTPUT);

  Timer1.initialize(5000000);         // Dispara cada 5 seg
  Timer1.attachInterrupt(interruption); // Activa la interrupcion y la asocia a interruption

  //rs232.println("Iniciado Conexion con GPS");
}

void loop()
{
  readLocation();
  //delay(5000);
}

//////////////////////////////////////////////////////////////////////////////////////////////

void interruption(){
  if(count>=5){
    if(gps_global_new!="null"){
      rs232.println(gps_global_new);
      }
    else{
      Serial.println("No GPS Signal");
      }
    count=0;
    }
  else{
    //rs232.print((String)count+", ");
    count++;
    }
  }

////////////////////////////////////////////////////////////////////////////////////////////////
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
/// Funcion que se encarga de formatear los valores numericos de HH:MM:SS
/// 
String formatNumber(int num){
  String formato;
  if(num>=0 && num<=9){
    formato = "0"+(String)num;
  }
  else{
    formato = (String)num;
  }
  return formato;
}
/////////////////////////////////////////////////////////////////////////////////////////////
/// Funcion que se encarga de formatear los meses
/// 
String formatMonth(int num){
  String mes;
  switch (num) {
      case 1:
        mes = "JAN";
        break;
      case 2:
        mes = "FEB";
        break;
      case 3:
        mes = "MAR";
        break;
      case 4:
        mes = "APR";
        break;
      case 5:
        mes = "MAY";
        break;
      case 6:
        mes = "JUN";
        break;
      case 7:
        mes = "JUL";
        break;
      case 8:
        mes = "AUG";
        break;
      case 9:
        mes = "SEP";
        break;
      case 10:
        mes = "OCT";
        break;
      case 11:
        mes = "NOV";
        break;
      case 12:
        mes = "DEC";
        break;
  }
  return mes;
}
/////////////////////////////////////////////////////////////////////////////////////////////
/// Funcion que se encarga de obtener y parsear la informacion obtenida desde el GPS y llamar
/// los ajustes necesarios
void readLocation(){
  bool newData = false, conectado = false,  tramaValida = false;
  unsigned long chars = 0;
  unsigned short sentences, failed;
  // Se hace un total de 1000 lecturas desde el GPS
  // comentar el ciclo for y 
  while(!conectado)
  {
    while (Serial1.available())
    {
      int c = Serial1.read();
      //Serial.print((char)c); //Descomentar para ver los datos que vienen desde las tramas del GPS 
      ++chars;
      if (gps.encode(c)){ // Si viene algun dato desde el GPS
        newData = true;
        conectado = true;
      }
    }
  }
  
  if (newData)
  {
    //// Obtencion de los datos del GPS
    unsigned long age;
    int year;
    byte month, day, hour, minute, second, hundredths;
    gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &age);// Fecha y hora
    // Ajuste = byte day , byte hour, byte month, int year
    ajustToLocalTime(day, hour, month, year);
    month=temp.month;
    day=temp.day;
    year=temp.year;
    hour=temp.hour;
    // Los siguientes if verifican la validez de la informacion recibida por el GPS (A para valido, V para invalido)
    if (age == TinyGPS::GPS_INVALID_AGE || age > 5000){
      //rs232.println("No fix detected OR Warning: possible stale data!");
    }
    else{
      tramaValida = true;    
      String dd = formatNumber((int)day);
      String hh = formatNumber((int)hour);
      String mm = formatNumber((int)minute);
      String ss = formatNumber((int)second);
      String mes = formatMonth((int)month);
      String year_new = (String)year;
      year_new.remove(0,2);
      String hora = hh+":"+mm+":"+ss;
      String dia = dd+mes+year_new;
      //rs232.println("PME\n249\nxxc\nPME=249\nTOD="+hora+"\nTOD="+dia);
      gps_global_prev = gps_global_new;
      gps_global_new = "PME\n249\nxxc\nPME=249\nTOD="+hora+"\nTOD="+dia;
    }
  }

  if (chars == 0){
    // Envia un aviso si es que no se esta recibiendo informacion
    //rs232.println("Check wiring");
  }
  else if(secondsToFirstLocation == 0){
    // still working
  }
}


