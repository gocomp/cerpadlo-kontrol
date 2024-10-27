#include <UIPEthernet.h>   // ENC28J60 Ethernet könyvtár
#include <SPI.h>
#include <avr/pgmspace.h>  // PROGMEM makró használatához
#include <avr/wdt.h>       // Watchdog Timer könyvtár
#include <Arduino.h>
#include <EEPROM.h>  // EEPROM könyvtár


// Kapcsoló pin konfiguráció
const int switchPin = A6;  // Kapcsoló az A6-os pin-en


const int eepromAddress = 0; // EEPROM cím, ahol az adatokat tároljuk
const int eepromAddress1 = 10; // EEPROM cím, ahol az adatokat tároljuk
const int eepromAddress2 = 20;

const unsigned long interval = 86400000; 
const int ledPin = 13; 

// Ethernet beállítások (csak MAC címre van szükség DHCP esetén)
byte mac[] = { 0xDE, 0xAF, 0xBA, 0xEF, 0xFC, 0xED }; // MAC cím

// SMTP szerver beállítások
const char smtpServer[] PROGMEM = "localhost";
const char smtpUser[] PROGMEM = "user"; // md5
const char smtpPass[] PROGMEM = "heslo";// md5
const char recipient[] PROGMEM = "user@gmail.com"; // Címzett e-mail cím
const int smtpPort = 587;

// SMTP üzenetek PROGMEM-ben
const char smtpHello[] PROGMEM = "EHLO mail.email.sk";
const char smtpAuth[] PROGMEM = "AUTH LOGIN";
const char smtpFrom[] PROGMEM = "MAIL FROM:<email>";
const char smtpRcpt[] PROGMEM = "RCPT TO:<email>";
const char smtpRcpt2[] PROGMEM = "RCPT TO:<email>";
const char smtpRcpt3[] PROGMEM = "RCPT TO:<email>";
const char smtpData[] PROGMEM = "DATA";

// Üzenetek különböző állapotokhoz
const char smtpSubject[] PROGMEM = "Subject: ALARM: Čerpadlo nefunguje.";
const char smtpMessage[] PROGMEM = "ALARM: Čerpadlo nefunguje.";
const char smtpSubjectv[] PROGMEM = "Subject: ALARM: Čerpadlo - elektrina bola zapnutá späť";
const char smtpMessagev[] PROGMEM = "ALARM: Čerpadlo - elektrina bola zapnutá späť.";
const char smtpSubjectf[] PROGMEM = "Subject: ALARM: Čerpadlo funguje.";
const char smtpMessagef[] PROGMEM = "ALARM: Čerpadlo funguje.";
const char smtpSubjectk[] PROGMEM = "Subject: Denná kontrola: Čerpadlo je v prevádzke.";
const char smtpMessagek[] PROGMEM = "Denná kontrola: Čerpadlo je v prevádzke.";

const char smtpQuit[] PROGMEM = "QUIT";

// Kapcsoló korábbi állapotát tároló változó
bool emailSent = false;
bool villanyvissza = true;
bool emailSentfu = false;
bool cerpfunkc = true;
bool restartad = false;



EthernetClient client;

bool sendCommand(const char *cmd, const char *additionalData = nullptr, const char *additionalData2 = nullptr); 
bool sendEmail();


void setup() {
  int attempts = 0;
  wdt_disable();
    pinMode(ledPin, OUTPUT);
//     Serial.begin(9600);
     
villanyvissza = (EEPROM.read(eepromAddress) != (byte)0);  
cerpfunkc = (EEPROM.read(eepromAddress1) != (byte)0);
restartad = (EEPROM.read(eepromAddress2) != (byte)0);

   //  Serial.println("indul");
  // Ethernet inicializálása DHCP-vel


  while (Ethernet.begin(mac) == 0) {
        delay(2000);  // Várj,  majd próbáld újra
      attempts++;
      if(attempts==10){
             wdt_enable(WDTO_15MS);   // Watchdog Timer indítása
             while (true);  // Végtelen ciklus az újraindításhoz
        }
       
  }
  
  // Serial.println(Ethernet.localIP());
      
 
   if (restartad){
    sendEmail();
     EEPROM.put(eepromAddress2, false);
   }
   
   if (villanyvissza){
      sendEmail();
   }
     if (!villanyvissza){
          EEPROM.put(eepromAddress, true);
     
     }
    villanyvissza = false;
    if(!cerpfunkc){
      sendEmail();
      }
}

void loop() {

    digitalWrite(ledPin, HIGH);

  // Aktuális kapcsoló állapot olvasása
  int currentSwitchState = analogRead(switchPin);  // Olvassuk az A6 pin-t

  // Ellenőrizzük, hogy az analóg érték elérte-e a küszöbértéket (512 a határ)
  if (currentSwitchState > 512 && !emailSent) {
      cerpfunkc = false;
     emailSentfu = false;
    // E-mail küldése
    if (sendEmail()) {
      emailSent = true;
      
    }
  }

  // Ha a kapcsoló kikapcsolódik, e-mail küldés a normál állapot visszaállásáról
  if (currentSwitchState <= 512 && !emailSentfu) {
     cerpfunkc = true;
     emailSent = false;
    if (sendEmail()) {
      emailSentfu = true;
    }
   }


 //   unsigned long currentMillis = millis();
  if (millis() >= interval) {
    
     if (EEPROM.read(eepromAddress) != (byte)false) {
    EEPROM.put(eepromAddress, false); //villanyvissza
     }
     if (EEPROM.read(eepromAddress1) != cerpfunkc) {
    EEPROM.put(eepromAddress1, cerpfunkc);
     }
    
    if (EEPROM.read(eepromAddress2) != (byte)true) {
    EEPROM.put(eepromAddress2, true);//    restartad
    }
    wdt_enable(WDTO_15MS);   // Watchdog Timer indítása
    while (true);  // Végtelen ciklus az újraindításhoz
  }
 delay(500);
   digitalWrite(ledPin, LOW);
 delay(500);
}

bool sendEmail() {
  char serverBuffer[30];
  strcpy_P(serverBuffer, smtpServer); 
  const int maxAttempts = 20;
  int attempts = 0;
  while (true) {
    if (client.connect(serverBuffer, smtpPort)) {

 //     Serial.println(F("van SMTP"));
      
      // SMTP parancsok küldése
      sendCommand(smtpHello);
      sendCommand(smtpAuth);
      sendCommand(smtpUser);
      sendCommand(smtpPass);
      sendCommand(smtpFrom);
      sendCommand(smtpRcpt);
      sendCommand(smtpRcpt2);
      sendCommand(smtpRcpt3);

      // Villany visszakapcsolás üzenet
      if (villanyvissza) {
        sendCommand(smtpData, smtpSubjectv, smtpMessagev);
      } 

       if (restartad) {
    // E-mail küldése, napi kontrol
    sendCommand(smtpData, smtpSubjectk, smtpMessagek);
  }
  
     if (!villanyvissza) {
       if (!restartad) {
  if (cerpfunkc) {
    // E-mail küldése, ha a szivattyú működik
    sendCommand(smtpData, smtpSubjectf, smtpMessagef);
  } else {
    // Hibaüzenet küldése, ha a szivattyú nem működik
    sendCommand(smtpData, smtpSubject, smtpMessage);
  }
}
     }



      sendCommand(smtpQuit);
      client.stop();
      return true;
    } else {
        delay(2000); 
        attempts++;
      if(attempts==10){
               wdt_enable(WDTO_15MS);   // Watchdog Timer indítása
    while (true);  // Végtelen ciklus az újraindításhoz
        }
      
    }
  }

  // Watchdog Timer beállítása az újraindításhoz, ha nem sikerül kapcsolódni
 // wdt_enable(WDTO_15MS); 
 // while (true);  // Végtelen ciklus az újraindításig
// Serial.println(F("hiba SMTP"));
 return false;
}

bool sendCommand(const char *cmd, const char *additionalData, const char *additionalData2) {
  char buffer[128];
  int attempts = 0;
  strcpy_P(buffer, cmd);
  client.println(buffer);
//Serial.println(buffer);
  // Várakozás és válaszkódok ellenőrzése
  while (true) {
    if (client.available()) {
      String response = client.readString();
      //Serial.println(response);
      if (response.startsWith("250") || response.startsWith("354") || response.startsWith("235") || response.startsWith("220") || response.startsWith("334")) {
        if (response.startsWith("354")) {
          // Üzenet törzs küldése
          if (additionalData) {
            strcpy_P(buffer, additionalData);
            client.println(buffer);
            delay(250);
            if (additionalData2) {
              client.println(" ");
              strcpy_P(buffer, additionalData2);
              client.println(buffer);
            }
          }
          delay(50);
          client.println(" ");
          client.println(".");  // Üzenet vége
          return true;
        }
        return true;
      } else {
        return false;
      }
    }
    delay(250);
        attempts++;
      if(attempts==100){
               wdt_enable(WDTO_15MS);   // Watchdog Timer indítása
              while (true);  // Végtelen ciklus az újraindításhoz
        }
  }
  return false;
}
