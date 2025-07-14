#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>

// -------- OLED --------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_DC     8
#define OLED_CS     12
#define OLED_RESET  9
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OLED_DC, OLED_RESET, OLED_CS);

// -------- Motor Pins --------
#define STEP_R 5
#define DIR_R 6
#define ENABLE_R 7
#define STEP_Z 2
#define DIR_Z 3
#define ENABLE_Z 4

// -------- Sensor y Botones --------
#define SENSOR_PIN A0
#define BTN_INICIAR 22
#define BTN_RESET   24
#define BTN_GUARDAR 26
#define LIM_INF 30
#define LIM_SUP 28

// -------- Parámetros --------
const int pasos_por_vuelta_rapido = 200;
const int pasos_por_vuelta_preciso = 400;
const int pasos_z_por_cm_rapido = 125;
const int pasos_z_por_cm_preciso = 62;
const int delay_paso_us_r = 700;
const int delay_paso_us_z = 500;
const int altura_maxima_cm = 21;

int vueltas = 0;
bool escaneo_terminado = false;
bool escaneo_iniciado = false;
bool modo_preciso = true;

float x = 0.0, y = 0.0, z = 0.0;
float angle = 0.0;
float distance_to_center = 8.5;

enum Estado {
  ESPERANDO,
  RESETEANDO,
  ESCANEANDO,
  SUBIENDO,
  TERMINADO
};
Estado estado_actual = ESPERANDO;

void setup() {
  Serial.begin(9600);
  inicializarPins();
  inicializarOLED();
  Inicio();
  mostrarModo();
  Serial.println("X;Y;Z");
}

void loop() {
  manejarEstado();
}

void inicializarPins() {
  pinMode(STEP_R, OUTPUT);
  pinMode(DIR_R, OUTPUT);
  pinMode(ENABLE_R, OUTPUT);
  pinMode(STEP_Z, OUTPUT);
  pinMode(DIR_Z, OUTPUT);
  pinMode(ENABLE_Z, OUTPUT);
  pinMode(SENSOR_PIN, INPUT);
  pinMode(BTN_INICIAR, INPUT_PULLUP);
  pinMode(BTN_RESET, INPUT_PULLUP);
  pinMode(BTN_GUARDAR, INPUT_PULLUP);
  pinMode(LIM_INF, INPUT_PULLUP);
  pinMode(LIM_SUP, INPUT_PULLUP);
  digitalWrite(ENABLE_R, LOW);
  digitalWrite(ENABLE_Z, LOW);
}

void inicializarOLED() {
  if (!display.begin(SSD1306_SWITCHCAPVCC)) {
    Serial.println("Error OLED");
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(15, 30);
  display.println("Bienvenido...");
  display.display();
  delay(2000);
}

void manejarEstado() {
  switch (estado_actual) {
    case ESPERANDO:
      if (digitalRead(BTN_RESET) == LOW) {
        estado_actual = RESETEANDO;
      } else if (!escaneo_iniciado && digitalRead(BTN_INICIAR) == LOW) {
        escaneo_iniciado = true;
        estado_actual = ESCANEANDO;
      } else if (digitalRead(BTN_GUARDAR) == LOW) {
        cambiarModo();
        delay(500);
      }
      break;

    case RESETEANDO:
      Reset();
      estado_actual = ESPERANDO;
      break;

    case ESCANEANDO:
      ejecutarEscaneo();
      break;

    case SUBIENDO:
      digitalWrite(DIR_Z, LOW);
      subirZ_un_cm();
      vueltas++;
      estado_actual = ESCANEANDO;
      break;

    case TERMINADO:
      mostrarFinal();
      break;
  }
}

void ejecutarEscaneo() {
  if (digitalRead(LIM_SUP) == HIGH || vueltas >= altura_maxima_cm) {
  estado_actual = TERMINADO;
  return;
  }

  int pasos_por_vuelta = modo_preciso ? pasos_por_vuelta_preciso : pasos_por_vuelta_rapido;
  for (int paso = 0; paso < pasos_por_vuelta; paso++) {
    rotarPlataforma(paso, pasos_por_vuelta);
  }
  estado_actual = SUBIENDO;
}

void rotarPlataforma(int paso, int total_pasos) {
  digitalWrite(DIR_R, LOW);
  digitalWrite(STEP_R, HIGH);
  delayMicroseconds(delay_paso_us_r);
  digitalWrite(STEP_R, LOW);
  delayMicroseconds(delay_paso_us_r);

  angle = (float(paso) / total_pasos) * 2 * PI;
  getDistance();

  mostrarDatosOLED();
  mostrarProgreso(paso, total_pasos);

  Serial.print(x, 2); Serial.print(";");
  Serial.print(y, 2); Serial.print(";");
  Serial.println(z, 2);
  delay(125);
}

void mostrarDatosOLED() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("X: "); display.println(x, 1);
  display.print("Y: "); display.println(y, 1);
  display.print("Z: "); display.println(z, 1);
  display.display();
}

void mostrarProgreso(int paso, int total) {
  int progreso_px = map(paso, 0, total, 0, SCREEN_WIDTH);
  int porcentaje = map(paso, 0, total, 0, 100);

  // Barra de fondo (más gruesa)
  for (int i = 0; i < 3; i++) {
    display.drawRect(0, 55 + i, SCREEN_WIDTH, 1, SSD1306_WHITE); // borde
  }

  // Barra de progreso rellena
  for (int i = 0; i < 3; i++) {
    display.fillRect(0, 55 + i, progreso_px, 1, SSD1306_WHITE);
  }

  // Porcentaje
  display.setTextSize(1);
  display.setCursor(SCREEN_WIDTH - 30, 45);
  display.print(porcentaje); display.println("%");

  display.display();
}

void mostrarFinal() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("ESCANEO TERMINADO");
  display.setCursor(0, 20);
  display.println("Presiona RESET");
  display.setCursor(0, 30);
  display.println("para reiniciar");
  display.display();
}

void Reset() {
  escaneo_iniciado = false;
  escaneo_terminado = false;
  z = 0;
  vueltas = 0;
  Inicio();
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("RESET COMPLETO");
  display.display();
  delay(1000);
  mostrarModo();
}

void subirZ_un_cm() {
  int pasos_z = modo_preciso ? pasos_z_por_cm_preciso : pasos_z_por_cm_rapido;
  for (int i = 0; i < pasos_z; i++) {
    digitalWrite(STEP_Z, HIGH);
    delayMicroseconds(delay_paso_us_z);
    digitalWrite(STEP_Z, LOW);
    delayMicroseconds(delay_paso_us_z);
  }
  z += modo_preciso ? 0.25 : 0.5;
}

float leerDistancia() {
  int raw = analogRead(SENSOR_PIN);
  if (raw < 400) return 80.0;  
  if (raw > 700) return 3.0;  

  float distancia_cm = 12345.0 * pow(raw, -1.15);  

  return constrain(distancia_cm, 3.0, 80.0); 
}

void getDistance() {
  float distancia_sensor_objeto = leerDistancia(); // en cm
  float distancia_desde_centro = distance_to_center - distancia_sensor_objeto; // eje al objeto
  distancia_desde_centro = constrain(distancia_desde_centro, -20.0, 20.0); // limita valores locos

  y = cos(angle) * distancia_desde_centro;
  x = sin(angle) * distancia_desde_centro;
}


void Inicio() {
  display.clearDisplay();
  display.setCursor(0, 10);
  display.println("Cargando...");
  display.display();
  digitalWrite(DIR_Z, HIGH);
  while (digitalRead(LIM_INF) == LOW) {
    digitalWrite(STEP_Z, HIGH);
    delayMicroseconds(delay_paso_us_z);
    digitalWrite(STEP_Z, LOW);
    delayMicroseconds(delay_paso_us_z);
  }
}

void cambiarModo() {
  modo_preciso = !modo_preciso;
  mostrarModo();
  Serial.print("Modo cambiado a: ");
  Serial.println(modo_preciso ? "Preciso" : "Rápido");
}

void mostrarModo() {
  display.clearDisplay();
  display.setCursor(0, 10);
  display.print("Modo actual: ");
  display.println(modo_preciso ? "Preciso" : "Rapido");
  display.setCursor(0, 25);
  display.println("Presiona INICIAR");
  display.setCursor(0, 40);
  display.println("para comenzar escaneo...");
  display.display();
}