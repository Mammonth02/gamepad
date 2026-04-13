// Защита от повторного подключения заголовка.
#ifndef UI_H
// Маркер того, что этот заголовок уже был включён.
#define UI_H

// Перечисление возможных экранов интерфейса.
enum Screen {
  WIFI_SCAN,
  // Стартовый экран, пока идёт подключение.
  STARTUP_SCREEN,
  // Основной экран после запуска.
  MAIN_SCREEN,
  // Экран меню.
  MENU_SCREEN
};

extern bool menuMode;

// Инициализация дисплея и интерфейса.
void initUI();
// Отрисовка текущего состояния интерфейса.
void drawUI();
// Смена активного экрана.
void setScreen(Screen s);

// Конец include guard.
#endif
