
RTC_DATA_ATTR int bootCount = 0;

/*
Метод для печати в мониторе порта причины пробуждения ESP32
*/
void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case 1  : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
                         //  "Пробуждение от внешнего сигнала с помощью RTC_IO"
    case 2  : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
                         //  "Пробуждение от внешнего сигнала с помощью RTC_CNTL"  
    case 3  : Serial.println("Wakeup caused by timer"); break;
                         //  "Пробуждение от таймера"
    case 4  : Serial.println("Wakeup caused by touchpad"); break;
                         //  "Пробуждение от сенсорного контакта"
    case 5  : Serial.println("Wakeup caused by ULP program"); break;
                         //  "Пробуждение от ULP-программы"
    default : Serial.println("Wakeup was not caused by deep sleep"); break;
                         //  "Пробуждение не связано с режимом глубокого сна"

  }
}

                                   


// Set serial for debug console (to the Serial Monitor, default speed 115200)
#define SerialMon Serial

// Set serial for AT commands (to the module)
// Use Hardware Serial on Mega, Leonardo, Micro
#define SerialAT Serial1

#define TINY_GSM_MODEM_SIM7000
#define TINY_GSM_RX_BUFFER 1024 // Set RX buffer to 1Kb
#define SerialAT Serial1

// See all AT commands, if wanted
// #define DUMP_AT_COMMANDS

// set GSM PIN, if any
#define GSM_PIN ""

// Your GPRS credentials, if any
const char apn[]  = "";     //SET TO YOUR APN
const char gprsUser[] = "";
const char gprsPass[] = "";

// включить библиотеку для чтения и записи из/во флэш-память
#include <EEPROM.h>

// определите количество байтов, к которым вы хотите получить доступ
#define EEPROM_SIZE 2


#include <TinyGsmClient.h>
#include <SPI.h>
#include <Ticker.h>

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif

#define uS_TO_S_FACTOR      1000000ULL  // Conversion factor for micro seconds to seconds
#define TIME_TO_SLEEP       580          // Time ESP32 will go to sleep (in seconds)

#define UART_BAUD           9600
#define PIN_DTR             25
#define PIN_TX              27
#define PIN_RX              26
#define PWR_PIN             4

#define SD_MISO             2
#define SD_MOSI             15
#define SD_SCLK             14
#define SD_CS               13
#define LED_PIN             12



void modemPowerOn()
{
    pinMode(PWR_PIN, OUTPUT);
    digitalWrite(PWR_PIN, LOW);
    delay(1000);    //Datasheet Ton mintues = 1S
    digitalWrite(PWR_PIN, HIGH);
}

void modemPowerOff()
{
    pinMode(PWR_PIN, OUTPUT);
    digitalWrite(PWR_PIN, LOW);
    delay(1500);    //Datasheet Ton mintues = 1.2S
    digitalWrite(PWR_PIN, HIGH);
}


void modemRestart()
{
    modemPowerOff();
    delay(1000);
    modemPowerOn();
}
int num;
void setup()
{
    // Set console baud rate
    SerialMon.begin(9600);

    delay(1000);


 // увеличиваем значение в счетчике загрузок
  // и печатаем это значение с каждой загрузкой:
  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));
              // "Количество загрузок: "

  // печатаем причину пробуждения ESP32:
  print_wakeup_reason();








    
EEPROM.begin(EEPROM_SIZE);

    delay(10);

    // Set LED OFF
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);
    num= EEPROM.read(0);
    num+= (EEPROM.read(1))*256;

if (num == 0xFFFF) num = 0;  // первая загрузка;

    
    Serial.print("num=");
    Serial.println(num);

    modemPowerOn();
    SerialAT.begin(UART_BAUD, SERIAL_8N1, PIN_RX, PIN_TX);

    
}

void loop()
{
      

digitalWrite(LED_PIN, HIGH);


    delay(10000);
    String res;

    Serial.println("========INIT========");

    if (!modem.init()) {
        modemRestart();
        delay(2000);
        Serial.println("Failed to restart modem, attempting to continue without restarting");
        return;
    }

    Serial.println("========SIMCOMATI======");
    modem.sendAT("+SIMCOMATI");
    modem.waitResponse(1000L, res);
    res.replace(GSM_NL "OK" GSM_NL, "");
    Serial.println(res);
    res = "";
    Serial.println("=======================");

    Serial.println("=====Preferred mode selection=====");
    modem.sendAT("+CNMP?");
    if (modem.waitResponse(1000L, res) == 1) {
        res.replace(GSM_NL "OK" GSM_NL, "");
        Serial.println(res);
    }
    res = "";
    Serial.println("=======================");


    Serial.println("=====Preferred selection between CAT-M and NB-IoT=====");
    modem.sendAT("+CMNB?");
    if (modem.waitResponse(1000L, res) == 1) {
        res.replace(GSM_NL "OK" GSM_NL, "");
        Serial.println(res);
    }
    res = "";
    Serial.println("=======================");


    String name = modem.getModemName();
    Serial.println("Modem Name: " + name);

    String modemInfo = modem.getModemInfo();
    Serial.println("Modem Info: " + modemInfo);

    // Unlock your SIM card with a PIN if needed
    if ( GSM_PIN && modem.getSimStatus() != 3 ) {
        modem.simUnlock(GSM_PIN);
    }


    for (int i = 0; i < 4; i++) {
        uint8_t network[] = {
            2,  /*Automatic*/
            13, /*GSM only*/
            38, /*LTE only*/
            51  /*GSM and LTE only*/
        };
        Serial.printf("Try %d method\n", network[i]);
        modem.setNetworkMode(network[i]);
        delay(3000);
        bool isConnected = false;
        int tryCount = 60;
        while (tryCount--) {
            int16_t signal =  modem.getSignalQuality();
            Serial.print("Signal: ");
            Serial.print(signal);
            Serial.print(" ");
            Serial.print("isNetworkConnected: ");
            isConnected = modem.isNetworkConnected();
            Serial.println( isConnected ? "CONNECT" : "NO CONNECT");
            if (isConnected) {
                break;
            }
            delay(1000);
            digitalWrite(LED_PIN, !digitalRead(LED_PIN));
        }
        if (isConnected) {
            break;
        }
    }
    digitalWrite(LED_PIN, HIGH);


    Serial.println("Device is connected .");


    Serial.println("=====Inquiring UE system information=====");
    modem.sendAT("+CPSI?");
    if (modem.waitResponse(1000L, res) == 1) {
        res.replace(GSM_NL "OK" GSM_NL, "");
        Serial.println(res);res = "";
    }



    Serial.println("-------------------");
    Serial.println("PDN Auto-activation");
    Serial.println();

    Serial.println("Check SIM card status");
    modem.sendAT("+CPIN?");
    if (modem.waitResponse(1000L, res) == 1) {
        res.replace(GSM_NL "OK" GSM_NL, "");
        Serial.println(res); res = "";
    }

    Serial.println("Check RF signal");
    modem.sendAT("+CSQ");
    if (modem.waitResponse(1000L, res) == 1) {
        res.replace(GSM_NL "OK" GSM_NL, "");
        Serial.println(res); res = "";
    }

    Serial.println("Check PS service");
    modem.sendAT("+CGREG?");
    if (modem.waitResponse(1000L, res) == 1) {
        res.replace(GSM_NL "OK" GSM_NL, "");
        Serial.println(res); res = "";
    }

    Serial.println("Query Network information,operator and network mode 9,NB-IOT network");
    modem.sendAT("+COPS?");
    if (modem.waitResponse(1000L, res) == 1) {
        res.replace(GSM_NL "OK" GSM_NL, "");
        Serial.println(res); res = "";
    }

    Serial.println("Query CAT-M or NB-IOT network after the successful registration of APN");
    modem.sendAT("+CGNAPN");
    if (modem.waitResponse(1000L, res) == 1) {
        res.replace(GSM_NL "OK" GSM_NL, "");
        Serial.println(res); res = "";
    }

 

    Serial.println("-------------");
    Serial.println("MQTT Function");
    Serial.println();


    Serial.println("Open wireless connection parameter CMNET is APN");
    modem.sendAT("+CNACT=1,\"cmnet\"");
for ( int i = 1 ; i <10; i++)
{
 delay(1000);
        while (SerialAT.available()) {
          res += char ( SerialAT.read()) ;
        }

Serial.println(res);
if (res.length()>5) i = 100;  
res = "";

}


    Serial.println("Get local IP");
    modem.sendAT("+CNACT?");
    if (modem.waitResponse(1000L, res) == 1) {
        res.replace(GSM_NL "OK" GSM_NL, "");
        Serial.println(res); res = "";
    }

    modem.sendAT("+SMCONF=\"URL\",86.57.154.232,1883");
    modem.sendAT("+SMCONF=\"KEEPTIME\",60");
    modem.sendAT("+SMCONF=\"USERNAME\",\"sample\"");
    modem.sendAT("+SMCONF=\"PASSWORD\",\"69greystones\"");
    modem.sendAT("+SMCONF=\"CLIENTID\",\"test18\"");

modem.sendAT("+SMCONF=\"CLIENTID\",\"test18\"");

/*
    Serial.println("====");
    modem.sendAT("+SMCONF?");
    if (modem.waitResponse(1000L, res) == 1) {
        res.replace(GSM_NL "OK" GSM_NL, "");
        Serial.println(res); res = "";
    }
*/
    
   Serial.println("SMCONN");
    modem.sendAT("+SMCONN");

for ( int i = 1 ; i <5; i++)
{
 delay(1000);
        while (SerialAT.available()) {
          res += char ( SerialAT.read()) ;
        }

Serial.println(res);
if (res.length()>0) i = 100;  
res = "";

}
 
  Serial.println("подписаться на пакет");
   modem.sendAT("+SMSUB=\"test/t7018\",1");
if (modem.waitResponse(1000L, res) == 1) {
        res.replace(GSM_NL "OK" GSM_NL, "");
        Serial.println(res); res = "";
    }
 

char Spak[256],Stem[256];    


sprintf(Spak,"{\"num\":%d,\"test\":\"pass\"}",num);

sprintf(Stem,"+SMPUB=\"test/t7018\",\"%d\",1,1",strlen(Spak));
/*
Serial.println(Spak);
Serial.println(Stem);
*/


  Serial.println("отправить пакет");
   modem.sendAT(Stem);
if (modem.waitResponse(1000L, res) == 1) {
        Serial.println(res); res = "";
}

SerialAT.print(Spak);

for ( int i = 1 ; i <10; i++)
{
 delay(1000);res = "";
        while (SerialAT.available()) {
          res += char ( SerialAT.read()) ;
        }
        int l=res.indexOf("num");
        Serial.println(res);
        if (l>0) {num++;i=100;res="";} 
  
}



/*
     Serial.println("After the network test is complete, please enter the  ");
     Serial.println("AT command in the serial terminal.");
  
    while (1) {
        while (SerialAT.available()) {
            SerialMon.write(SerialAT.read());
        }
        while (SerialMon.available()) {
            SerialAT.write(SerialMon.read());
        }
    }


*/


  Serial.println("Disconnect MQTT");
    modem.sendAT("+SMDISC");
    if (modem.waitResponse(1000L, res) == 1) {
        res.replace(GSM_NL "OK" GSM_NL, "");
        Serial.println(res); res = "";
    }
    

   Serial.println("Disconnect wireless");
    modem.sendAT("+CNACT=0");
    if (modem.waitResponse(1000L, res) == 1) {
        res.replace(GSM_NL "OK" GSM_NL, "");
        Serial.println(res); res = "";
    }


// пауза 10 минут

    EEPROM.write(0, num%256);
    EEPROM.write(1, num/256);
    
    EEPROM.commit();
digitalWrite(LED_PIN, LOW);

modemPowerOff();


  /*
  Сначала настраиваем инициатор пробуждения.
  Задаем, чтобы ESP32 просыпалась каждые TIME_TO_SLEEP секунд.
  */
//  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
//  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) +
//  " Seconds");
             //  "ESP32 будет просыпаться каждые ... секунд"
             






//Serial.end();

  Serial.println("Going to sleep now");
              // "Переход в режим сна"
              
  delay(5000);

  esp_deep_sleep(TIME_TO_SLEEP * uS_TO_S_FACTOR);
 // esp_deep_sleep_start();


    
}
