
#include <EEPROM.h>

// ID of the settings block
#define CONFIG_VERSION "Ph4"
#define CONFIG_START 32

/*
  Analog Input
 */

#define MQ7PIN A0 
#define MQ8PIN A1 
#define MQ3PIN A2 
#define MQ135PIN A3 
#define PHPIN A5

#define NUMSENSORS 4

typedef struct
 {
     char* name;
     char* gas;
     char* unit;
     int  pin;
     float ro;
     float rl;        //load resistor
     float cleanairf;
     int   outputdec;
     float coefa;
     float coefb;
 }  sensordata;

 typedef struct
 {
    int  average;    //how many readings to average for one measurement
    long interval;   //milliseconds between readings
    int  startup;    //seconds before running
    float vRef;      //reference voltage
    float ph4read;
    float ph7read;
    sensordata sensor[NUMSENSORS]; 
 } configdata;
 
/************************
  log-log plot paa datasheet =>
  y = a * pow(x,b) 
  
  b = log(y1/y0) / log(x1/x0)
  a = y0 / pow(x0, b)
  
  mq7  : https://www.sparkfun.com/datasheets/Sensors/Biometric/MQ-7.pdf //25
  mq8  : https://dlnmh9ip6v2uc.cloudfront.net/datasheets/Sensors/Biometric/MQ-8.pdf //70
  mq3  : https://www.sparkfun.com/datasheets/Sensors/MQ-3.pdf // 60
  mq135: http://www.futurlec.com/Air_Quality_Control_Gas_Sensor.shtml  //3.5
 ************************/

configdata conf;

void loadDefaults () {
   conf.average =  250;     //number of sampples per reading
   conf.interval =5000;     //ms between readings
   conf.startup =   60;     //Seconds
   conf.vRef =    5000.0;   //mV
   conf.ph4read = 3440.0;
   conf.ph7read = 2875.0;
   
                     //name    gas        unit    pin        ro    rl  freshair dec acoef  bcoef
   conf.sensor[0] =  {"MQ7",   "CO",      "ppm",  MQ7PIN,    15.0, 10.0, 25.0 , 3, 23.4  , -0.67 };
   conf.sensor[1] =  {"MQ8",   "H",       "ppm",  MQ8PIN,     5.0, 10.0, 70.0 , 3, 17.5  , -1.44 };
   conf.sensor[2] =  {"MQ3",   "Alcohol", "ng/l", MQ3PIN,    10.0, 10.0, 60.0 , 6, 0.504 , -0.62 };
   conf.sensor[3] =  {"MQ135", "NH3",     "ppm",  MQ135PIN, 100.0, 10.0,  3.5 , 3, 6.00  , -0.38 };
}

void displayConfig () {
   Serial.print("average     = ");
   Serial.println(conf.average);
   Serial.print("interval    = ");
   Serial.println(conf.interval);
   Serial.print("wait        = ");
   Serial.println(conf.startup);
   Serial.print("ph4         = ");
   Serial.println(conf.ph4read);
   Serial.print("ph7         = ");
   Serial.println(conf.ph7read);

  for (int i=0; i<NUMSENSORS; i++) {
    showsensor(i,i==0);
  }

}


void setup() {
  // declare the ledPin as an OUTPUT:
  Serial.begin(9600);
  loadDefaults(); 
  loadConfig();
  Serial.println("Setup complete.");
  Serial.println("  ");
}


void loop()
{
  static char buffer[80];
  int running = 1;
  //wait  before starting
  long nextpoll = millis() + conf.startup * 1000L;
  
  while (1) {
    if (Serial.available() &&  readline(Serial.read(), buffer, 80) > 0) {
      int addr,iter;
      long value;
      switch (buffer[0]) {
        case 'c': //calibrate
          value = getnumargument(buffer);
          iter = 1;
          if ( value > 0 ) {
            iter = (int)value;
          }
          for (int i=0; i < iter; i++) {
            Serial.print(F("Calibrating #"));
            Serial.println(i+1);
            calibrate();
          }
          break;
        case 's': //save
          saveConfig();
          Serial.println(F("Saved"));
          break;
        case 'l':  //load
          loadConfig();
          break;
        case 'd':  //display
          displayConfig();
          //
          break;
        case '4':  //set ph4
          conf.ph4read = GetAvgmV(PHPIN, conf.average); 
          break;
        case '7':  //set ph7
          conf.ph7read = GetAvgmV(PHPIN, conf.average); 
          break;
        case 'r': //run
          running = 1;
          nextpoll = millis()-1;
          Serial.println(F("Running"));
          break;
        case 'w': //wait
          value = getnumargument(buffer);
          if ( value > 0 ) {
            conf.startup = value;
            Serial.print(F("Interval updated to "));
            Serial.println(value);
          }
          break;
        case 'i': //set interval
          value = getnumargument(buffer);
          if ( value > 0 ) {
            conf.interval = value;
            Serial.print(F("Interval updated to "));
            Serial.println(value);
          }
          break;
        case 'a': //set average
          value = getnumargument(buffer);
          if ( value > 0 ) {
            conf.average = value;
            Serial.print(F("average updated to "));
            Serial.println(value);
          }
          break;
        case 'p': //pause
          running = 0;          
          Serial.println(F("Paused"));
          break;
        default:
          Serial.println(F("Usage:"));
          Serial.println(F(" calibrate n: run n gascalibrations in clean air"));
          Serial.println(F(" interval ms: set measurement interval to ms milliseconds"));
          Serial.println(F(" wait s:      set initial waiting time to s seconds"));
          Serial.println(F(" average n:   each measurement i an average of n samples"));
          Serial.println(F(" 4phbuffer:   calibrate phmeter to ph4 now"));
          Serial.println(F(" 7phbuffer:   calibrate phmeter to ph7 now"));
          Serial.println(F(" load config"));
          Serial.println(F(" save config"));
          Serial.println(F(" display config"));
          Serial.println(F(" run"));
          Serial.println(F(" pause"));
      }
    }
    //Serial.println(millis() - nextpoll);
    if (running == 1) {
      if (millis() > nextpoll) {
        nextpoll = millis() + conf.interval;
        pollsensors(conf.average);
      }
    }
  }
}

long getnumargument(char buffer[80]) {
  char dummy[16];
  long value = -1;
  if (sscanf(buffer, "%s %ld", &dummy, &value)) {
            if ( value < 0 ) {
              value = -1;
            }
  }
  return value;
}

int readline(int readch, char *buffer, int len)
{
  static int pos = 0;
  int rpos;

  if (readch > 0) {
    switch (readch) {
      case '\n': // Ignore new-lines
        break;
      case '\r': // Return on CR
        rpos = pos;
        pos = 0;  // Reset position index ready for next time
        return rpos;
      default:
        if (pos < len-1) {
          buffer[pos++] = readch;
          buffer[pos] = 0;
        }
    }
  }
  // No end of line has been found, so return -1.
  return -1;
}

void saveConfig() {
  EEPROM.write(CONFIG_START + 0,CONFIG_VERSION[0]);
  EEPROM.write(CONFIG_START + 1,CONFIG_VERSION[1]);
  EEPROM.write(CONFIG_START + 2,CONFIG_VERSION[2]);
  for (unsigned int t=0; t<sizeof(conf); t++)
    EEPROM.write(CONFIG_START + 3 + t, *((char*)&conf + t));
}


int loadConfig() {
  // To make sure there are settings, and they are YOURS!
  // If nothing is found it will use the default settings.
  if (EEPROM.read(CONFIG_START + 0) == CONFIG_VERSION[0] &&
      EEPROM.read(CONFIG_START + 1) == CONFIG_VERSION[1] &&
      EEPROM.read(CONFIG_START + 2) == CONFIG_VERSION[2]) {
        
    for (unsigned int t=0; t<sizeof(conf); t++) {
      *((char*)&conf + t) = EEPROM.read(CONFIG_START + 3 + t);
    }
    Serial.println(F("Loaded config"));
    return 1;
  } else {
    Serial.println(F("Not my config - nothing loaded"));
    return 0;
  }      
}

void pollsensors(int readings) {

  int i;
  float mV;
  float Rs;
  float measure;
  char buffer[50];
  
  for (i=0; i<NUMSENSORS; i++) {

    mV = GetAvgmV(conf.sensor[i].pin, readings);
    Rs = CalcRsFromVo(mV, conf.vRef, conf.sensor[i].rl); 
    measure = CalcMeasure(Rs/conf.sensor[i].ro, conf.sensor[i].coefa, conf.sensor[i].coefb); 


    sprintf(buffer, "#%s %s ", conf.sensor[i].gas, conf.sensor[i].unit);
    Serial.print(buffer);
    Serial.println(measure,conf.sensor[i].outputdec);
    sprintf(buffer, "#%sRAW mV ", conf.sensor[i].name);
    Serial.print(buffer);
    Serial.println(mV,0);

    delay(10);
  }
  
  // add pH reading
  mV = GetAvgmV(PHPIN,readings);
  measure = (conf.ph7read - mV) / ((conf.ph4read - conf.ph7read)/2.99) + 7.00;

  Serial.print("#PH mH/l ");
  Serial.println(measure,2);
  Serial.print("#PHRAW mV ");
  Serial.println(mV,0);
}

void calibrate () {
  float mV = 0.0;
  char buffer[50];  
  int i;
  Serial.println("Sensor id Pin Measures Unit  Ro");

  for (i=0; i<NUMSENSORS; i++) {
    //  Conv output to Ro
    //  Ro = calibration factor for measurement in clean air.
    //  Ro = ((vRef - mV) * RL) / (mV * Ro_clean_air_factor);
    //  Hereafter, measure the sensor output, convert to Rs, and
    //  then calculate Rs/Ro using: Rs = ((Vc-Vo)*RL) / Vo

    mV = GetAvgmV(conf.sensor[i].pin, conf.average); 
    conf.sensor[i].ro = CalcRsFromVo(mV, conf.vRef, conf.sensor[i].rl) / conf.sensor[i].cleanairf;
    showsensor(i,i==0);    
  }
  Serial.println("");
}

void showsensor (int sid, int header) {
  char buffer[50];  
  if (header == 1) {
    Serial.println("Sensor id Pin Measures Unit  Ro");
  }
  
  sprintf(buffer, "%-6s %2i %3i %-8s %-4s  ",
                          conf.sensor[sid].name, sid,
                          conf.sensor[sid].pin,
                          conf.sensor[sid].gas,
                          conf.sensor[sid].unit);
  Serial.print(buffer);
  Serial.println(conf.sensor[sid].ro);
}

float Get_mVfromADC(byte AnalogPin) {
    // read the value from the sensor:
    int v = analogRead(AnalogPin);  
    // It takes about 100 microseconds (0.0001 s) to read an analog input
    delay(1);
    //  Voltage at pin in milliVolts = (reading from ADC) * (5000/1024) 
    float mV = v * (conf.vRef / 1024.0);
    return mV;
}

float GetAvgmV(byte AnalogPin, int samples) {
  float mV = 0.0;
  int s = 0;
  //  take a reading..
  for(int i = samples; i>0; i--){
    mV += Get_mVfromADC(AnalogPin);
    s += 1;
  }
  mV = mV / (float) s;
  return mV;
}


float CalcRsFromVo(float Vo, float vRef, float RL) {
  //  Vo = sensor output voltage in mV.
  //  VRef = supply voltage, 5000 mV
  //  RL = load resistor in k ohms
  //  The equation Rs = (Vc - Vo)*(RL/Vo)
  //  is derived from the voltage divider
  //  principle:  Vo = RL * Vc (Rs + RL)
  //
  //  Note.  Alternatively you could calc
  //         Rs from ADC value using
  //         Rs = RL * (1024 - ADC) / ADC
  float Rs = (vRef - Vo) * (RL / Vo);  
  return Rs;
}

float CalcMeasure(float RsRo_ratio, float a_coeficient, float b_coeficient) {
  //  If you extract the data points from the CO concentration
  //  versus Rs/Ro chart in the datasheet, plot the points,
  //  fit a polynomial curve to the points, you come up with the equation
  //  for the curve of:  Rs/Ro = 22.073 * (CO ppm) ^ -0.66659
  //  This equation is valid for ambient conditions of 20 C and 65% RH.
  //  Solving for the concentration of CO you get:
  //    CO ppm = [(Rs/Ro)/22.073]^(1/-0.66666)
  float measure;
  measure = pow((RsRo_ratio/a_coeficient), (1/b_coeficient));
  return measure;
}


