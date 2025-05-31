#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP085_U.h>
#include <DHT.h>
#include <MQUnifiedsensor.h>

#define NUM_SWITCH      4      // Cantidad de switches que tenemos conectados
#define DHTPIN          2      // Pin al que está conectado el sensor DHT22
#define MQ135_PIN       A0     // Pin analógico para el MQ-135
#define DHTTYPE         DHT22  // Tipo de sensor DHT
#define ANCHO_PANTALLA  128
#define ALTO_PANTALLA   64
#define DIREC_SCREEN    0x3C
#define OLED_RST        -1
#define REFRESH_MS      2000   // Intervalo de actualización de pantalla (ms)
#define DEBOUNCE_MS     80     // Tiempo de debounce en ms
#define BUZZER_PIN      7

// Pines de los switches (entradas)
const uint8_t switch_pin[NUM_SWITCH] = {3, 4, 5, 6};

// Variables para debounce (una por cada switch)
uint32_t lastDebounceTime[NUM_SWITCH] = {0};
bool     lastReading[NUM_SWITCH]     = {HIGH, HIGH, HIGH, HIGH};
bool     buttonState[NUM_SWITCH]     = {HIGH, HIGH, HIGH, HIGH};

// En esta variable guardamos qué “modo” está seleccionado:
// 1 = Temperatura (DHT22), 
// 2 = Humedad (DHT22),
// 3 = PPM + Calidad de aire (MQ-135), 
// 4 = Presión atmosférica (BMP180).
uint8_t displayMode = 1;

// Variable de temporización para refrescar la pantalla
uint32_t lastUpdate = 0;

// Objetos de sensores y pantalla
DHT dht(DHTPIN, DHTTYPE);
Adafruit_SSD1306 display(ANCHO_PANTALLA, ALTO_PANTALLA, &Wire, OLED_RST);

// Sensor BMP180 (Adafruit_BMP085_Unified)
Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(10085);
bool bmp_ok = false;  // Bandera que indica si el BMP180 inicializó correctamente

// Texto para calidad de aire (se almacena en FLASH para no consumir SRAM)
const __FlashStringHelper* calidad_aire = nullptr;

void setup() {
  pinMode(BUZZER_PIN, OUTPUT);
  noTone(BUZZER_PIN);  // Asegura que no esté sonando al inicio

  // --- 1) Configuro switches como INPUT_PULLUP y seteo los estados iniciales ---
  for (uint8_t i = 0; i < NUM_SWITCH; i++) {
    pinMode(switch_pin[i], INPUT_PULLUP);
    lastReading[i] = digitalRead(switch_pin[i]);
    buttonState[i] = lastReading[i];
  }

  // --- 2) Inicializo sensor DHT22 ---
  dht.begin();

  // --- 3) Inicializo sensor BMP180 sin bloquear si falla ---
  if (bmp.begin()) {
    bmp_ok = true;
  } else {
    bmp_ok = false;
    // Si quieres, puedes mostrar un mensaje breve en la OLED indicando el error.
    // Pero recuerda que la OLED aún no está inicializada. Marcamos bmp_ok=false.
  }

  // --- 4) Inicializo pantalla OLED ---
  display.begin(SSD1306_SWITCHCAPVCC, DIREC_SCREEN);
  display.clearDisplay();
  display.display();
}

void loop() {
  uint32_t now = millis();

  // --- 1) Lectura de switches con debounce ---
  for (uint8_t i = 0; i < NUM_SWITCH; i++) {
    bool reading = digitalRead(switch_pin[i]);
    if (reading != lastReading[i]) {
      // Si cambió la señal, reinicio el temporizador de debounce
      lastDebounceTime[i] = now;
    }
    if ((now - lastDebounceTime[i]) > DEBOUNCE_MS) {
      // Si ya pasó el tiempo de debounce
      if (reading != buttonState[i]) {
        buttonState[i] = reading;
        // Con INPUT_PULLUP, estado LOW significa que se presionó el switch
        if (buttonState[i] == LOW) {
          // Cuando se presiona el switch i, asigno el modo correspondiente (1..4)
          displayMode = i + 1;
        }
      }
    }
    lastReading[i] = reading;
  }

  // --- 2) Si corresponde, actualizo pantalla cada REFRESH_MS milisegundos ---
  if (now - lastUpdate >= REFRESH_MS) {
    lastUpdate = now;

    // --- 2.1) Lectura MQ-135 (PPM) ---
    uint16_t valor_MQ = analogRead(MQ135_PIN);

    // --- 2.2) Lectura DHT22 (temperatura + humedad) ---
    float t_dht = dht.readTemperature();
    float h_dht = dht.readHumidity();

    // --- 2.3) Lectura BMP180 (temperatura interna + presión) solo si inicializó ---
    float t_bmp = NAN;
    float p_bmp = NAN;
    if (bmp_ok) {
      sensors_event_t event;
      bmp.getEvent(&event);
      if (event.pressure) {
        // event.pressure viene en hPa, lo convierto a Pa multiplicando por 100
        p_bmp = event.pressure * 100.0;
      }
      bmp.getTemperature(&t_bmp);
    }

    // --- 2.4) Determino calidad de aire según valor_MQ y manejo del buzzer ---
    if (valor_MQ <= 300) {
      calidad_aire = F("Buena");
      noTone(BUZZER_PIN); // No suena
    } else if (valor_MQ <= 600) {
      calidad_aire = F("Regular");
      tone(BUZZER_PIN, 1000, 200); // Tono medio, 200 ms
    } else {
      calidad_aire = F("Mala");
      tone(BUZZER_PIN, 2000, 500); // Tono más alto, 500 ms
    }

    // --- 3) Pinto únicamente lo que indique displayMode ---
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);

    switch (displayMode) {
      case 1:
        // Mostrar Temperatura del DHT22
        display.setCursor(0, 24);
        if (!isnan(t_dht)) {
          display.print(F("Temp:"));
          display.print(t_dht, 1);
          display.write(247); // símbolo °
          display.print(F("C"));
        } else {
          display.print(F("Temp Err"));
        }
        break;

      case 2:
        // Mostrar Humedad del DHT22
        display.setCursor(0, 24);
        if (!isnan(h_dht)) {
          display.print(F("Hum: "));
          display.print(h_dht, 1);
          display.print(F("%"));
        } else {
          display.print(F("Hum Err"));
        }
        break;

      case 3:
        // Mostrar PPM y Calidad de aire (dos líneas)
        display.setCursor(0, 0);
        display.print(F("PPM: "));
        display.print(valor_MQ);
        display.setCursor(0, 32);
        display.print(F("Aire: "));
        display.print(calidad_aire);
        break;

      case 4:
        // Mostrar Presión atmosférica (BMP180) o error si bmp_ok == false
        display.setCursor(0, 24);
        if (bmp_ok && !isnan(p_bmp)) {
          display.print(F("Pres: "));
          display.print(p_bmp / 100.0, 1);
          display.print(F("hPa"));
        } else {
          display.print(F("BMP Err"));
        }
        break;

      default:
        // Si por alguna razón displayMode no está en 1..4, vuelvo a 1
        displayMode = 1;
        break;
    }

    display.display();
  }
}
