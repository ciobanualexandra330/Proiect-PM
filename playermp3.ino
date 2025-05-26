#include <SoftwareSerial.h>  
#include <DFRobotDFPlayerMini.h>     

// ====== DFPlayer ======
SoftwareSerial mySoftwareSerial(10, 11); // tx = 10, rx = 11
DFRobotDFPlayerMini myDFPlayer;

int lastVolume = -1;
int currentTrack = 1;

// ====== ADC - Volum cu potențiometru ======
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

// ====== I2C folosind TWI ======
#define SLA 0x27 

void TWI_init() {
  TWSR = 0;
  TWBR = 32;
  TWCR = (1 << TWEN);
}

void TWI_start() {
  TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
  while (!(TWCR & (1 << TWINT)));
}

void TWI_stop() {
  TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);
}

void TWI_write(uint8_t data) {
  TWDR = data;
  TWCR = (1 << TWINT) | (1 << TWEN);
  while (!(TWCR & (1 << TWINT)));
}

// ====== Control LCD 16x2 prin PCF8574 ======
#define LCD_BACKLIGHT 0x08
#define ENABLE 0x04
#define RS 0x01

void lcd_pulse_enable(uint8_t data) {
  TWI_write(data | ENABLE);
  delayMicroseconds(1);
  TWI_write(data & ~ENABLE);
  delayMicroseconds(50);
}

void lcd_send_nibble(uint8_t nibble, uint8_t mode) {
  uint8_t data = (nibble << 4) | LCD_BACKLIGHT;
  if (mode) data |= RS;
  TWI_write(data);
  lcd_pulse_enable(data);
}

void lcd_send_byte(uint8_t value, uint8_t mode) {
  lcd_send_nibble(value >> 4, mode);
  lcd_send_nibble(value & 0x0F, mode);
  delayMicroseconds(50);
}

void lcd_command(uint8_t cmd) {
  TWI_start();
  TWI_write(SLA << 1);
  lcd_send_byte(cmd, 0);
  TWI_stop();
}

void lcd_data(uint8_t data) {
  TWI_start();
  TWI_write(SLA << 1);
  lcd_send_byte(data, 1);
  TWI_stop();
}

void lcd_init() {
  delay(50);
  TWI_start();
  TWI_write(SLA << 1);
  lcd_send_nibble(0x03, 0); delay(5);
  lcd_send_nibble(0x03, 0); delay(5);
  lcd_send_nibble(0x03, 0); delay(1);
  lcd_send_nibble(0x02, 0); // 4-bit mode
  TWI_stop();

  lcd_command(0x28); // 4-bit, 2 lines
  lcd_command(0x0C); // Display ON
  lcd_command(0x06); // Increment
  lcd_command(0x01); // Clear
  delay(5);
}

void lcd_setCursor(uint8_t col, uint8_t row) {
  const uint8_t row_offsets[] = {0x00, 0x40};
  lcd_command(0x80 | (col + row_offsets[row]));
}

void lcd_clear() {
  lcd_command(0x01);
  delay(2);
}

void lcd_print(const char* str) {
  while (*str) lcd_data(*str++);
}

// ====== I2C - Afișare melodie pe LCD ======
void displaySong(int song) {
  lcd_clear();
  lcd_setCursor(0, 0);
  switch (song) {
    case 1: lcd_print("The Smiths Bigmouth Strikes Again"); break;
    case 2: lcd_print("Enjambre Impacto"); break;
    case 3: lcd_print("Surf Curse Disco"); break;
    case 4: lcd_print("Steve Lacy Some"); break;
    case 5: lcd_print("Abandoned Pools Armed To The Teeth"); break;
  }
  lcd_setCursor(0, 1);
  lcd_print("Volum: ");
  char buf[5];
  itoa(lastVolume >= 0 ? lastVolume : 0, buf, 10);
  lcd_print(buf);
  lcd_print("%");
}

// ====== Setup ======
void setup() {
  TWI_init();
  lcd_init();
  adc_init();
  mySoftwareSerial.begin(9600);
  Serial.begin(9600);

  // GPIO - Inițializare butoane (pini 4-7 ca input cu pull-up)
  DDRD &= ~((1 << DDD4) | (1 << DDD5) | (1 << DDD6) | (1 << DDD7));
  PORTD |= (1 << PORTD4) | (1 << PORTD5) | (1 << PORTD6) | (1 << PORTD7);

  lcd_setCursor(0, 0);
  lcd_print("Volum (ADC):");

  if (!myDFPlayer.begin(mySoftwareSerial)) {
    lcd_setCursor(0, 1);
    lcd_print("DFPlayer ERR");
    while (true);
  }

  myDFPlayer.setTimeOut(500);
  myDFPlayer.volume(20);
  myDFPlayer.EQ(DFPLAYER_EQ_NORMAL);

  currentTrack = 1;
  myDFPlayer.play(currentTrack);
  displaySong(currentTrack);
}

// ====== Loop principal ======
void loop() {
  uint16_t potValue = adc_read();
  int volumePercent = map(potValue, 0, 1023, 0, 100);

  if (volumePercent != lastVolume) {
    lastVolume = volumePercent;
    lcd_setCursor(0, 1);
    lcd_print("Volum:     ");
    lcd_setCursor(7, 1);
    char buf[5];
    itoa(volumePercent, buf, 10);
    lcd_print(buf);
    lcd_print("%");

    int dfVol = map(volumePercent, 0, 100, 0, 30);
    myDFPlayer.volume(dfVol);
  }

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

  if ((PIND & (1 << PIND4)) == 0) {
    myDFPlayer.play(currentTrack);
    displaySong(currentTrack);
    delay(300);
  }

  if ((PIND & (1 << PIND5)) == 0) {
    myDFPlayer.stop();
    lcd_setCursor(0, 0);
    lcd_print("STOP              ");
    delay(300);
  }

  if ((PIND & (1 << PIND6)) == 0) {
    currentTrack++;
    if (currentTrack > 5) currentTrack = 1;
    myDFPlayer.play(currentTrack);
    displaySong(currentTrack);
    delay(300);
  }

  if ((PIND & (1 << PIND7)) == 0) {
    currentTrack--;
    if (currentTrack < 1) currentTrack = 5;
    myDFPlayer.play(currentTrack);
    displaySong(currentTrack);
    delay(300);
  }
}
