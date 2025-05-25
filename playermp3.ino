#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>      
#include <DFRobotDFPlayerMini.h> 
 
 LiquidCrystal_I2C lcd(0x27, 16, 2);  // Adresa 0x27, 16 coloane, 2 rânduri
 
 SoftwareSerial mySoftwareSerial(10, 11); // RX, TX
 DFRobotDFPlayerMini myDFPlayer;
 
 const int BUTTON_PLAY  = 4;
 const int BUTTON_STOP  = 5;
 const int BUTTON_NEXT  = 6;
 const int BUTTON_PREV  = 7;
 
 int lastVolume = -1;
 int currentTrack = 1;
 
 void adc_init() {
   ADMUX = (1 << REFS0); // Referință AVcc, canal ADC0
   ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // Activare + prescaler 128
 }
 
 uint16_t adc_read() {
   ADMUX = (ADMUX & 0xF0) | 0; // Selectează ADC0
   ADCSRA |= (1 << ADSC);      // Start conversie
   while (ADCSRA & (1 << ADSC));
   return ADC;
 }
 
 void displaySong(int song) {
   lcd.clear();
   switch (song) {
     case 1:
       lcd.setCursor(0, 0); lcd.print("Coldplay Charlie");
       break;
     case 2:
       lcd.setCursor(0, 0); lcd.print("Enjambre Impacto");
       break;
     case 3:
       lcd.setCursor(0, 0); lcd.print("Radiohead All I");
       break;
     case 4:
       lcd.setCursor(0, 0); lcd.print("Steve Lacy Some");
       break;
     case 5:
       lcd.setCursor(0, 0); lcd.print("White Stripes Fell in love with a girl");
       break;
   }
   lcd.setCursor(0, 1);
   lcd.print("Volum: ");
   lcd.print(lastVolume >= 0 ? lastVolume : 0);
   lcd.print("%");
 }
 
 // ====== Setup ======
 void setup() {
   // I2C - Inițializare LCD
   lcd.init();
   lcd.backlight();
 
   // ADC - Inițializare
   adc_init();
 
   // UART - Inițializare DFPlayer
   mySoftwareSerial.begin(9600);
   Serial.begin(9600);
 
   // GPIO - Butoane
   pinMode(BUTTON_PLAY, INPUT_PULLUP);
   pinMode(BUTTON_STOP, INPUT_PULLUP);
   pinMode(BUTTON_NEXT, INPUT_PULLUP);
   pinMode(BUTTON_PREV, INPUT_PULLUP);
 
   lcd.setCursor(0, 0);
   lcd.print("Volum (ADC):");
 
   // UART - DFPlayer init
   if (!myDFPlayer.begin(mySoftwareSerial)) {
     lcd.setCursor(0, 1);
     lcd.print("DFPlayer ERR");
     while (true); // Blochează dacă eșuează
   }
 
   myDFPlayer.setTimeOut(500);
   myDFPlayer.volume(20);               // Setare volum inițial
   myDFPlayer.EQ(DFPLAYER_EQ_NORMAL);   // Egalizator
 
   // Redare piesă inițială
   currentTrack = 1;
   myDFPlayer.play(currentTrack);
   displaySong(currentTrack);
 }
 
 // ====== Loop principal ======
 void loop() {
   // ADC - Volum cu potențiometru
   uint16_t potValue = adc_read();
   int volumePercent = map(potValue, 0, 1023, 0, 100);
 
   if (volumePercent != lastVolume) {
     lastVolume = volumePercent;
     lcd.setCursor(0, 1);
     lcd.print("                "); // Curățare linie
     lcd.setCursor(0, 1);
     lcd.print("Volum: ");
     lcd.print(volumePercent);
     lcd.print("%");
 
     int dfVol = map(volumePercent, 0, 100, 0, 30);
     myDFPlayer.volume(dfVol);
   }
 
   // UART - Verificare sfârșit melodie
   if (myDFPlayer.available()) {
     uint8_t type = myDFPlayer.readType();
     uint16_t value = myDFPlayer.read();
 
     if (type == DFPlayerPlayFinished) {
       currentTrack++;
       if (currentTrack > 5) currentTrack = 1;
       myDFPlayer.play(currentTrack);
       displaySong(currentTrack);
     }
   }
 
   // GPIO - Butoane control
   if (digitalRead(BUTTON_PLAY) == LOW) {
     myDFPlayer.play(currentTrack);
     displaySong(currentTrack);
     delay(300);
   }
 
   if (digitalRead(BUTTON_STOP) == LOW) {
     myDFPlayer.stop();
     lcd.setCursor(0, 0);
     lcd.print("STOP              ");
     delay(300);
   }
 
   if (digitalRead(BUTTON_NEXT) == LOW) {
     currentTrack++;
     if (currentTrack > 5) currentTrack = 1;
     myDFPlayer.play(currentTrack);
     displaySong(currentTrack);
     delay(300);
   }
 
   if (digitalRead(BUTTON_PREV) == LOW) {
     currentTrack--;
     if (currentTrack < 1) currentTrack = 5;
     myDFPlayer.play(currentTrack);
     displaySong(currentTrack);
     delay(300);
   }
 }
 