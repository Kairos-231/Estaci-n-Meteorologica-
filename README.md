# Estación Meteorológica

Este repositorio contiene el código fuente de la **Estación Meteorológica** desarrollado en Arduino. El propósito de este proyecto es medir y visualizar, de forma local, las principales variables ambientales (temperatura, humedad, presión atmosférica, calidad de aire, etc.) utilizando distintos sensores y una pantalla OLED.

> **Proyecto realizado para la materia**  
> “Laboratorio de Mediciones Mecánicas Eléctricas y Electrónicas”  
> **Carrera:** Ingeniería Mecatrónica – 3er Año

---

## Tecnologías y Lenguaje de Programación

- **Lenguaje:** C/C++  
- **Entorno de desarrollo:** Arduino IDE (versión 1.8.x o superior)  
- **Plataforma de hardware:** Arduino Uno (o compatible ATmega328P)  
- **Librerías principales:**
  - `Adafruit_SSD1306` (pantalla OLED 128×64 vía I²C)  
  - `Adafruit_GFX` (soporte gráfico para la OLED)  
  - `DHT` (sensor DHT22 para temperatura y humedad)  
  - `Adafruit_BMP085_U` (sensor BMP180 para presión atmosférica y temperatura interna)  
  - `MQUnifiedsensor` (sensor MQ-135 para calidad de aire/PPM)  

---


## Conexiones de Hardware

1. **DHT22 (AM2302)**
   - VCC → 5 V  
   - GND → GND  
   - DATA → Pin digital 2  

2. **BMP180 (I²C)**
   - VCC → 3.3 V (o 5 V si el módulo lo permite)  
   - GND → GND  
   - SCL → A5 (SCL en Arduino Uno)  
   - SDA → A4 (SDA en Arduino Uno)  

3. **MQ-135**
   - VCC → 5 V  
   - GND → GND  
   - AOUT (salida analógica) → A0 (se recomienda RL de 10 kΩ entre AOUT y GND)  

4. **Pantalla OLED 128×64 (I²C)**
   - VCC → 5 V  
   - GND → GND  
   - SCL → A5  
   - SDA → A4  

5. **Switches (4 unidades)**
   - Switch 1 → Pin digital 3 → GND  
   - Switch 2 → Pin digital 4 → GND  
   - Switch 3 → Pin digital 5 → GND  
   - Switch 4 → Pin digital 6 → GND  
   (Todos configurados en INPUT_PULLUP; al presionar, leen LOW)

---

## Funcionamiento Básico

1. Al compilar y subir el sketch en **Arduino IDE**, el microcontrolador inicia lecturas periódicas (cada 2 s) de:
   - **DHT22**: temperatura (°C) y humedad (%)  
   - **MQ-135**: valor analógico (0–1023) y cálculo de “calidad de aire” (Buena/Regular/Mala)  
   - **BMP180**: presión atmosférica (hPa) y temperatura interna para compensación  

2. La pantalla OLED muestra únicamente la variable seleccionada mediante los 4 switches:
   1. **Switch 1 (pin 3):** Temperatura (DHT22)  
   2. **Switch 2 (pin 4):** Humedad (DHT22)  
   3. **Switch 3 (pin 5):** PPM y calidad de aire (MQ-135)  
   4. **Switch 4 (pin 6):** Presión atmosférica (BMP180)  

3. Cada switch implementa debounce para asegurar lecturas limpias y, en caso de falla en algún sensor, la pantalla muestra “Err”.

---

## Instalación y Uso

1. **Clonar el repositorio**  
   ```bash
   git clone https://github.com/<tu_usuario>/Estacion-Meteorologica-.git
   cd Estacion-Meteorologica-
