#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <time.h>
#include <Preferences.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Preferences prefs;

#define BUTTON_PIN 13
#define BUZZER_PIN 27

const char* ssid = "Nombre del internet";
const char* password = "Contraseña del internet";

int screen = 0;
bool format24h = true;
bool alarmOn = false;

int alarmHour = 7;
int alarmMinute = 0;

bool inMenu = false;
int menuStep = 0;

bool alarmRinging = false;

unsigned long buttonPressTime = 0;
bool buttonHeld = false;
bool lastButtonState = HIGH;

void beep(int f, int d) {
  tone(BUZZER_PIN, f, d);
}

void saveSettings() {
  prefs.begin("reloj", false);
  prefs.putInt("ah", alarmHour);
  prefs.putInt("am", alarmMinute);
  prefs.putBool("aon", alarmOn);
  prefs.putBool("f24", format24h);
  prefs.end();
}

void loadSettings() {
  prefs.begin("reloj", true);
  alarmHour = prefs.getInt("ah", 7);
  alarmMinute = prefs.getInt("am", 0);
  alarmOn = prefs.getBool("aon", false);
  format24h = prefs.getBool("f24", true);
  prefs.end();
}

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);

  Serial.begin(115200);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    while (true);
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  loadSettings();

  WiFi.begin(ssid, password);
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.println("Conectando WiFi...");
  display.display();

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  configTime(-5 * 3600, 0, "pool.ntp.org");

  beep(1500, 150);
}

void loop() {
  handleButton();

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return;

  checkAlarm(timeinfo);

  display.clearDisplay();

  if (alarmRinging) {
    showRinging();
  } else if (inMenu) {
    showMenu();
  } else {
    switch (screen) {
      case 0: showTime(timeinfo); break;
      case 1: showDate(timeinfo); break;
      case 2: showAlarm(); break;
      case 3: showFormat(); break;
    }
  }

  display.display();
  delay(120);
}

void handleButton() {
  bool currentState = digitalRead(BUTTON_PIN);

  if (lastButtonState == HIGH && currentState == LOW) {
    buttonPressTime = millis();
    buttonHeld = false;
  }

  if (currentState == LOW && !buttonHeld && millis() - buttonPressTime > 1200) {
    buttonHeld = true;
    longPress();
  }

  if (lastButtonState == LOW && currentState == HIGH) {
    if (!buttonHeld) {
      shortPress();
    }
  }

  lastButtonState = currentState;
}

void shortPress() {
  beep(2000, 80);

  if (alarmRinging) {
    alarmRinging = false;
    noTone(BUZZER_PIN);
    return;
  }

  if (inMenu) {
    if (menuStep == 0) alarmHour = (alarmHour + 1) % 24;
    else if (menuStep == 1) alarmMinute = (alarmMinute + 1) % 60;
    else if (menuStep == 2) {
      inMenu = false;
      menuStep = 0;
      saveSettings();
    }
    return;
  }

  if (screen == 2) {
    alarmOn = !alarmOn;
    saveSettings();
  }
  if (screen == 3) {
    format24h = !format24h;
    saveSettings();
  }

  screen++;
  if (screen > 3) screen = 0;
}

void longPress() {
  beep(800, 200);

  if (!inMenu) {
    inMenu = true;
    menuStep = 0;
  } else {
    menuStep++;
    if (menuStep > 2) {
      inMenu = false;
      menuStep = 0;
      saveSettings();
    }
  }
}

void checkAlarm(struct tm timeinfo) {
  if (!alarmOn || alarmRinging) return;

  if (timeinfo.tm_hour == alarmHour && timeinfo.tm_min == alarmMinute && timeinfo.tm_sec == 0) {
    alarmRinging = true;
  }

  if (alarmRinging) {
    tone(BUZZER_PIN, 1500);
  }
}

void showRinging() {
  display.setTextSize(2);
  display.setCursor(0, 20);
  display.println("ALARMA!");
  display.setTextSize(1);
  display.setCursor(0, 50);
  display.println("Presiona para parar");
}

void showTime(struct tm timeinfo) {
  char timeStr[20];
  if (format24h)
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
  else
    strftime(timeStr, sizeof(timeStr), "%I:%M:%S %p", &timeinfo);

  display.setTextSize(2);
  display.setCursor(0, 20);
  display.println(timeStr);
}

void showDate(struct tm timeinfo) {
  char dateStr[20];
  strftime(dateStr, sizeof(dateStr), "%d/%m/%Y", &timeinfo);

  display.setTextSize(2);
  display.setCursor(0, 20);
  display.println(dateStr);
}

void showAlarm() {
  display.setTextSize(2);
  display.setCursor(0, 10);
  display.print("Alarma");
  display.setCursor(0, 30);
  display.println(alarmOn ? "ON" : "OFF");
}

void showFormat() {
  display.setTextSize(2);
  display.setCursor(0, 10);
  display.print("Formato");
  display.setCursor(0, 30);
  display.println(format24h ? "24h" : "12h");
}

void showMenu() {
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("CONFIG ALARMA");

  display.setTextSize(2);
  display.setCursor(0, 20);

  if (menuStep == 0) {
    display.print("Hora: ");
    if (alarmHour < 10) display.print("0");
    display.println(alarmHour);
  } else if (menuStep == 1) {
    display.print("Min: ");
    if (alarmMinute < 10) display.print("0");
    display.println(alarmMinute);
  } else if (menuStep == 2) {
    display.println("Guardar");
  }
}
