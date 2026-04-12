// Защита от повторного включения этого заголовка в одном файле.
#ifndef INPUT_H
// Определяем маркер, что заголовок уже подключён.
#define INPUT_H

// Состояние одной кнопки.
struct Button {
  bool current;
  bool last;
  unsigned long lastChangeTime;
};

// Внешняя переменная с координатой по оси X.
extern int x, y;
// Внешние переменные состояний кнопок.
extern Button btn1, btn2, btn3, btn4;

// Инициализация источника входных данных.
void initInput();
// Обработка чтения новых данных.
void handleInput();

// Вспомогательные проверки состояния кнопки.
bool isPressed(Button& b);
bool wasPressed(Button& b);
bool wasReleased(Button& b);

// Конец защитного блока include guard.
#endif
