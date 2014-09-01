#include "key.h" // Should contains #define API_KEY "/api/post?apikey=YOUR_API_KEY&json="
#include <LiquidCrystal.h>

#include <VirtualWire.h>

#include <EtherCard.h>
#include <dht11.h>

#define REQUEST_RATE_CMS 10000 // milliseconds
#define REQUEST_RATE_DISPLAY 5000 // milliseconds

#define DHT11_PIN 7
#define RX_PIN 8

#define LCD_RS_PIN A0
#define LCD_ENABLE_PIN A1
#define LCD_D4_PIN A2
#define LCD_D5_PIN A3
#define LCD_D6_PIN A4
#define LCD_D7_PIN A5

static byte mymac[] = { 
  0x74,0x42,0x69,0x2D,0x30,0x31 };


byte Ethernet::buffer[400];
long timerCms;
long timerDisplay;
char website[] PROGMEM = "emoncms.org";



volatile int count = 0;

int inTemp;
int inHum;
int extTemp;
int extHum;

int timeHour;
int timeMinute;

char displayMode;

LiquidCrystal lcd(LCD_RS_PIN, LCD_ENABLE_PIN, LCD_D4_PIN, LCD_D5_PIN, LCD_D6_PIN, LCD_D7_PIN);

void setup () {

  Serial.begin(9600);
  Serial.println(F("DHCP Demo"));

  initLcd();
  lcd.print(F("Weather Station"));
  lcd.setCursor(0,1);
  lcd.print(F("is starting..."));

  Serial.print( F("Init ethernet controller : "));
  if (ether.begin(sizeof Ethernet::buffer, mymac, 10))
     Serial.println("OK");
  else 
     Serial.println("KO");

Serial.print(F("DHCP configuration : "));
  if (ether.dhcpSetup())
    Serial.println("OK");
  else
    Serial.println("KO");

  ether.printIp("IP :\t", ether.myip);
  ether.printIp("Mask:\t", ether.mymask);
  ether.printIp("Gateway:\t", ether.gwip);
  ether.printIp("DNS : ", ether.dnsip);

  Serial.print(" DNS"); 
  if (ether.dnsLookup(website)){
    Serial.println("OK");  
    ether.printIp("SRV IP:\t", ether.hisip);
    Serial.println();
  } 
  else {
    Serial.println("KO");    
  }

  timerCms = - REQUEST_RATE_CMS; // start timing out right away
  timerDisplay = - REQUEST_RATE_DISPLAY; // start timing out right away

  attachInterrupt(1, trig, RISING); // D3

  initVirtualWire();
}

static void response_callback (byte status, word off, word len) {
  Serial.println(F("Update send to Emon"));
}

void loop() {


  ether.packetLoop(ether.packetReceive());

  receivedAvailiableMessage();

  if (millis() > timerDisplay + REQUEST_RATE_DISPLAY) {
    updateIndoorTemperature();
    
    timerDisplay = millis();
    updateDisplay();
  }

  if (millis() > timerCms + REQUEST_RATE_CMS) {

         timerCms = millis();

   

     int tmpCount;

      noInterrupts();
      tmpCount =count;
      count=0;
      interrupts();

      char   temp[60] ;// 30 avant
   
      sprintf(temp, "{T:%d,H:%d,E:%d,Te:%d,He:%d}", inTemp,inHum,tmpCount,extTemp,extHum);

      Serial.print("formated :");
      Serial.println(temp);

       ether.browseUrl(PSTR(API_KEY), temp, website, response_callback);   


    }
  

}

void updateIndoorTemperature(){
   dht11 DHT11;
    DHT11.attach(DHT11_PIN);   
      Serial.print("Read DHT11: ");
    if (DHT11.read() == 0)
    {
      Serial.println("OK");   
      inTemp=DHT11.temperature;
      inHum=DHT11.humidity;
    }
    else{ 
      Serial.println("KO"); 
    }
  
  
  }

void updateDisplay(){

  char line1[17];
  char line2[17];



  switch(displayMode){
  case 0:

    sprintf(line1, "Temp %din/%dex", inTemp, extTemp);
    sprintf(line2, "Hum  %din/%dex",inHum , extHum);
    break;
  case 1:
    sprintf(line1, "il est %02d:%02d", timeHour, timeMinute);  
    sprintf(line2, "Bon");   
    switch(timeHour){
    case 0:
    case 23:
    case 1:
    case 2:                  
    case 3:                  
    case 4:                  
    case 5:                  
    case 6:                
      sprintf(line2+3, "ne nuit !");    
      break;
    case 7:                  
    case 8:                  
    case 9:                  
    case 10:                  
    case 11:                  
      sprintf(line2+3, "ne journée");  
      break;
    case 12:                  
    case 13:                  
      sprintf(line2+3, " appétit");  
      break;
    case 14:                  
    case 15:                  
    case 16:                  
    case 17:                  
    case 18:                  
      sprintf(line2+3, " après-midi");  
      break;
    case 19:                  
    case 20:                  
    case 21:
    case 22:
      sprintf(line2+3, "ne soirée");  


    }




  }





  lcd.clear();
  lcd.setCursor(0,0);  
  lcd.print(line1);
  lcd.setCursor(0,1);
  lcd.print(line2);

  displayMode = (displayMode + 1)%2;
}

void initVirtualWire(){
  // Initialise the IO and ISR
  vw_set_rx_pin(RX_PIN);
  vw_set_ptt_inverted(true); // Required for DR3100
  vw_setup(1000);	 // Bits per sec
  vw_rx_start();       // Start the receiver PLL running 
}

void  initLcd(){
  lcd.begin(16, 2);
}



void trig()
{
  count++; 
}


void receivedAvailiableMessage(){
  uint8_t buf[VW_MAX_MESSAGE_LEN];
  uint8_t buflen = VW_MAX_MESSAGE_LEN;
  if (vw_get_message(buf, &buflen)) // Non-blocking
  {
     *(buf+buflen) = '\0';
    Serial.print("Received ");
    Serial.println((char*)buf);

    sscanf((char*)buf,"N%02dM%02dT%dH%02d",&timeHour,&timeMinute,&extTemp,&extHum);
    
  }

}









