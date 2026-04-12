// Базовые возможности Arduino.
#include <Arduino.h>
// Работа с Wi‑Fi нужна, чтобы показывать статус подключения.
#include <ESP8266WiFi.h>
// I2C-шина, через которую подключён OLED-дисплей.
#include <Wire.h>
// Базовая графическая библиотека Adafruit.
#include <Adafruit_GFX.h>
// Драйвер конкретно для SSD1306 OLED-дисплея.
#include <Adafruit_SSD1306.h>

// Объявления функций и типов интерфейса.
#include "ui.h"
// Доступ к значениям кнопок и осей.
#include "input.h"
// Доступ к флагу connected.
#include "wifi.h"

// Создаём объект дисплея:
// 128x64 - разрешение,
// &Wire - используем I2C,
// -1 - отдельный пин Reset не используется.
Adafruit_SSD1306 display(128, 64, &Wire, -1);

// Текущий активный экран интерфейса.
Screen currentScreen = WIFI_SCAN;
// Индекс выбранного пункта меню.
int menuIndex = 0;

int selected = 0;

// Первичная настройка дисплея и I2C.
void initUI() {
  // Запускаем I2C на пинах D2 (SDA) и D1 (SCL).
  Wire.begin(D2, D1);
  // Инициализируем дисплей по адресу 0x3C.
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); 
  // Очищаем внутренний буфер экрана.
  display.clearDisplay();
  // Выбираем белый цвет текста.
  display.setTextColor(SSD1306_WHITE);
  // Отправляем буфер на дисплей, чтобы применить изменения.
  display.display();
}

// Вспомогательная функция для смены активного экрана.
void setScreen(Screen s) {
  // Просто записываем новый экран в глобальное состояние.
  currentScreen = s;
}

void drawWiFiList() {
  display.clearDisplay();

  for (int i = 0; i < networksCount && i < 20; i++) {
    if (i == selected) {
      display.setTextColor(BLACK, WHITE); // выделение
    } 
    else {
      display.setTextColor(WHITE);
    }

    display.setCursor(0, i * 10);
    display.println(ssids[i]);
  }

  display.display();
}

// Рисуем стартовый экран подключения.
void drawStartup() {
  // Очищаем буфер перед новой отрисовкой.
  display.clearDisplay();
  // Ставим курсор в левый верхний угол.
  display.setCursor(0, 0);

  // Проверка на подключение
  if (WiFi.status() != WL_CONNECTED) {
  // Этап 1: Нет даже интернета
  display.println(F("WiFi: Connecting..."));
  } 
  else if (!connected) {
    // Этап 2: Wi-Fi есть, но ПК нас еще не "увидел"
    display.println(F("WiFi: OK!"));
    display.println(F("Waiting for PC..."));
  } 
  else {
    // Этап 3: Полный контакт
    display.println(F("System: Online"));
    display.println(F("Connected to PC!"));
  }

  // Выводим подготовленный кадр на экран.
  display.display();
}

// Рисуем главный экран после успешного подключения.
void  drawMain() {
  // Очищаем экран перед новой картинкой.
  display.clearDisplay();

  // Пишем заголовок в левом верхнем углу.
  display.setCursor(0, 0);
  display.println(F("GAMEPAD"));

  // Показываем состояние сетевого соединения.
  display.print(F("WiFi: "));
  display.println(connected ? F("OK") : F("..."));

  // Подсказка по управлению.
  display.println(F("\nBTN -> Menu"));

  // Обновляем реальный дисплей.
  display.display();
}

// Рисуем экран меню.
void drawMenu() {
  // Очищаем буфер дисплея.
  display.clearDisplay();

  // Список пунктов меню.
  const char* items[2] = {
    "WiFi",
    "Back"
  };

  // Выводим строку состояния сверху.
  display.setCursor(0, 0);
  display.print(F("Status: "));
  display.println(connected ? F("OK") : F("..."));
  // Пустая строка для отступа.
  display.println();

  // Проходим по всем пунктам меню.
  for (int i = 0; i < 2; i++) {
    // Для выбранного пункта рисуем маркер ">".
    if (i == menuIndex) display.print("> ");
    // Для остальных рисуем обычный отступ.
    else display.print("  ");

    // Печатаем текст текущего пункта.
    display.println(items[i]);
  }

  // Показываем результат на экране.
  display.display();
}

// Главная функция обновления интерфейса и обработки управления меню.

void drawUI() {
  // Время последнего перемещения по меню.
  static unsigned long lastMove = 0;
  // Момент, когда соединение стало активным.
  static unsigned long connectedAt = 0;

  if (currentScreen == WIFI_SCAN) {
    if (btn2.current) selected--;
    if (btn3.current) selected++;

    if (selected < 0) selected = 0;
    if (selected >= networksCount) selected = networksCount - 1;

    
    if (wasPressed(btn4)) {
      strcpy(ssid, ssids[selected].c_str());
      setScreen(STARTUP_SCREEN);
      WiFi.disconnect();
      wifiStarted = true;
    }

    delay(100);
  }

  // Логика стартового экрана.
  if (currentScreen == STARTUP_SCREEN) {
    // Если ПК уже найден, начинаем отсчёт до перехода дальше.
    if (connected) {
      // Запоминаем момент подключения только один раз.
      if (connectedAt == 0) connectedAt = millis();
      // Через секунду после подключения переходим в меню.
      if (millis() - connectedAt > 1000) {
        setScreen(MENU_SCREEN);
      }
    } 
    else {
      // Если соединение пропало, сбрасываем таймер ожидания.
      connectedAt = 0;
    }
  }

  // Управление пунктами меню работает только на экране меню.
  if (currentScreen == MENU_SCREEN) {
    // Ограничиваем частоту перемещения, чтобы меню не "улетало" слишком быстро.
    if (millis() - lastMove > 200) {
      // Большое значение Y считаем движением вниз.
      if (y > 800) {
        // Переходим к следующему пункту.
        menuIndex++;
        // Если вышли за последний пункт, возвращаемся к первому.
        if (menuIndex > 1) menuIndex = 0;
        // Запоминаем время этого перемещения.
        lastMove = millis();
      }

      // Малое значение Y считаем движением вверх.
      if (y < 200) {
        // Переходим к предыдущему пункту.
        menuIndex--;
        // Если ушли выше первого пункта, переходим на последний.
        if (menuIndex < 0) menuIndex = 1;
        // Запоминаем время этого перемещения.
        lastMove = millis();
      }
    }
  }
  


  // Вызываем функцию рисования того экрана, который активен сейчас.
  switch (currentScreen) {
    case WIFI_SCAN: drawWiFiList(); break;
    // Рисуем стартовый экран.
    case STARTUP_SCREEN: drawStartup(); break;
    // Рисуем главный экран.
    case MAIN_SCREEN: drawMain(); break;
    // Рисуем меню.
    case MENU_SCREEN: drawMenu(); break;
  }
}
