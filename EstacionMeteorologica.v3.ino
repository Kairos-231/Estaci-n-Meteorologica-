#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP085_U.h>
#include <DHT.h>
#include <MQUnifiedsensor.h>

#define NUM_SWITCH      4
#define DHTPIN          2
#define MQ135_PIN       A0
#define DHTTYPE         DHT22
#define ANCHO_PANTALLA  128
#define ALTO_PANTALLA   64
#define DIREC_SCREEN    0x3C
#define OLED_RST        -1
#define REFRESH_MS      2000
#define DEBOUNCE_MS     80
#define BUZZER_PIN      7

const uint8_t switch_pin[NUM_SWITCH] = {3, 4, 5, 6};

uint32_t lastDebounceTime[NUM_SWITCH] = {0};
bool     lastReading[NUM_SWITCH]     = {HIGH, HIGH, HIGH, HIGH};
bool     buttonState[NUM_SWITCH]     = {HIGH, HIGH, HIGH, HIGH};

uint8_t displayMode = 1;
bool    showError   = false;  // Nueva bandera para alternar modo error

uint32_t lastUpdate = 0;

DHT dht(DHTPIN, DHTTYPE);
Adafruit_SSD1306 display(ANCHO_PANTALLA, ALTO_PANTALLA, &Wire, OLED_RST);
Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(10085);
bool bmp_ok = false;
const __FlashStringHelper* calidad_aire = nullptr;

void setup() {
  pinMode(BUZZER_PIN, OUTPUT);
  noTone(BUZZER_PIN);

  for (uint8_t i = 0; i < NUM_SWITCH; i++) {
    pinMode(switch_pin[i], INPUT_PULLUP);
    lastReading[i] = digitalRead(switch_pin[i]);
    buttonState[i] = lastReading[i];
  }

  dht.begin();

  if (bmp.begin()) {
    bmp_ok = true;
  } else {
    bmp_ok = false;
  }

  display.begin(SSD1306_SWITCHCAPVCC, DIREC_SCREEN);
  display.clearDisplay();
  display.display();
}

void loop() {
  uint32_t now = millis();

  // Lectura de switches con debounce y toggling de showError
  for (uint8_t i = 0; i < NUM_SWITCH; i++) {
    bool reading = digitalRead(switch_pin[i]);
    if (reading != lastReading[i]) {
      lastDebounceTime[i] = now;
    }
    if ((now - lastDebounceTime[i]) > DEBOUNCE_MS) {
      if (reading != buttonState[i]) {
        buttonState[i] = reading;
        if (buttonState[i] == LOW) {
          uint8_t modo = i + 1;
          if (displayMode == modo) {
            // Si es el mismo botón, alterno mostrar error
            showError = !showError;
          } else {
            // Nuevo modo: vuelvo al modo normal sin error
            displayMode = modo;
            showError   = false;
          }
        }
      }
    }
    lastReading[i] = reading;
  }

  // Actualización de pantalla
  if (now - lastUpdate >= REFRESH_MS) {
    lastUpdate = now;

    uint16_t valor_MQ = analogRead(MQ135_PIN);
    float t_dht = dht.readTemperature();
    float h_dht = dht.readHumidity();
    float t_bmp = NAN, p_bmp = NAN;
    if (bmp_ok) {
      sensors_event_t event;
      bmp.getEvent(&event);
      if (event.pressure) p_bmp = event.pressure * 100.0;
      bmp.getTemperature(&t_bmp);
    }

    // Calidad de aire y buzzer
    if (valor_MQ <= 300) {
      calidad_aire = F("Buena");
      noTone(BUZZER_PIN);
    } else if (valor_MQ <= 600) {
      calidad_aire = F("Regular");
      tone(BUZZER_PIN, 1000, 200);
    } else {
      calidad_aire = F("Mala");
      tone(BUZZER_PIN, 2000, 500);
    }

    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);

    switch (displayMode) {
      case 1: // Temperatura DHT22
        display.setCursor(0, 24);
        if (!isnan(t_dht)) {
          display.print(F("Temp:"));
          display.print(t_dht, 1);
          display.write(247);
          display.print(F("C"));
          if (showError) {
            // ±0.5 °C
            display.setCursor(0, 48);
            display.print(F("+/-0.5C"));
          }
        } else {
          display.print(F("Temp Err"));
        }
        break;

      case 2: // Humedad DHT22
        display.setCursor(0, 24);
        if (!isnan(h_dht)) {
          display.print(F("Hum: "));
          display.print(h_dht, 1);
          display.print(F("%"));
          if (showError) {
            // ±4 %
            display.setCursor(0, 48);
            display.print(F("+/-4%"));
          }
        } else {
          display.print(F("Hum Err"));
        }
        break;

      case 3: // PPM + calidad aire
        display.setCursor(0, 0);
        display.print(F("PPM: "));
        display.print(valor_MQ);
        display.setCursor(0, 32);
        display.print(F("Aire: "));
        display.print(calidad_aire);
        break;

      case 4: // Presión BMP180
        display.setCursor(0, 24);
        if (bmp_ok && !isnan(p_bmp)) {
          display.print(F("Pres: "));
          display.print(p_bmp / 100.0, 1);
          display.print(F("hPa"));
          if (showError) {
            // ±0.12 hPa
            display.setCursor(0, 48);
            display.print(F("+/-0.12"));
            display.print(F("hPa"));
          }
        } else {
          display.print(F("BMP Err"));
        }
        break;

      default:
        displayMode = 1;
        showError   = false;
        break;
    }

    display.display();
  }
}
