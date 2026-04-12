// Подключаем базовые возможности Arduino.
#include <Arduino.h>
// Библиотека для создания дополнительного последовательного порта на обычных пинах.
#include <SoftwareSerial.h>
// Заголовок с объявлениями переменных и функций этого модуля.
#include "input.h"

// Создаём программный UART:
// D6 - RX, сюда приходят данные;
// D7 - TX, отсюда при необходимости можно отправлять данные.
SoftwareSerial mySerial(D6, D7);

// Текущее значение оси X.
int x = 0;
// Текущее значение оси Y.
int y = 0;
// Состояние первой кнопки.
Button btn1 = {0, 0, 0};
// Состояние второй кнопки.
Button btn2 = {0, 0, 0};
// Состояние третьей кнопки.
Button btn3 = {0, 0, 0};
// Состояние четвёртой кнопки.
Button btn4 = {0, 0, 0};

// Буфер для накопления входящей строки вида "123,456,1,0,1,0".
char buffer[32];
// Индекс текущей позиции в буфере.
int bufIndex = 0;

// Объявляем функцию заранее, потому что она описана ниже по файлу.
void parseData(char* data);

// Обновляем состояние кнопки и отмечаем момент изменения.
void updateButton(Button& button, bool state) {
  if (button.current != state) {
    button.last = button.current;
    button.current = state;
    button.lastChangeTime = millis();
  }
}

bool isPressed(Button &b) {
  return b.current == 1;
}

bool wasPressed(Button &b) {
  if (b.current == 1 && b.last == 0) {
    if (millis() - b.lastChangeTime > 50) {
      return true;
    }
  }
  return false;
}

bool wasReleased(Button &b) {
  return b.current == 0 && b.last == 1;
}

// Инициализация входного последовательного соединения.
void initInput() {
  // Скорость должна совпадать с устройством, которое отправляет данные.
  mySerial.begin(9600);
}

// Читаем всё, что успело прийти в программный UART.
void handleInput() {

  // Пока во входном буфере есть байты, обрабатываем их по одному.
  while (mySerial.available()) {
    // Считываем очередной символ.
    char c = mySerial.read();

    // Символ новой строки означает конец одного пакета данных.
    if (c == '\n') {
      // Ставим нулевой символ конца строки, чтобы buffer стал корректной C-строкой.
      buffer[bufIndex] = 0;
      // Разбираем готовую строку и обновляем x, y и кнопки.
      parseData(buffer);
      // Сбрасываем индекс, чтобы начать приём следующей строки с начала буфера.
      bufIndex = 0;
    } else {
      // Защита от переполнения: оставляем место под завершающий ноль.
      if (bufIndex < 31) {
        // Сохраняем символ и сдвигаем индекс на следующую свободную позицию.
        buffer[bufIndex++] = c;
      }
    }
  }
}

// Разбираем строку без использования класса String,
// чтобы снизить расход памяти и избежать фрагментации кучи на микроконтроллере.
void parseData(char* data) {
  // Из строки формата "x,y,btn,btn2,btn3,btn4"
  // читаем шесть целых чисел во временные переменные.
  int rawBtn1 = 0;
  int rawBtn2 = 0;
  int rawBtn3 = 0;
  int rawBtn4 = 0;
  sscanf(data, "%d,%d,%d,%d,%d,%d",
         &x, &y, &rawBtn1, &rawBtn2, &rawBtn3, &rawBtn4);

  // Инвертируем значения кнопок.
  // Это полезно, если кнопки подключены по схеме, где нажатие даёт 0, а отпускание 1.
  updateButton(btn1, !rawBtn1);
  // То же самое для второй кнопки.
  updateButton(btn2, !rawBtn2);
  // То же самое для третьей кнопки.
  updateButton(btn3, !rawBtn3);
  // То же самое для четвёртой кнопки.
  updateButton(btn4, !rawBtn4);
}
