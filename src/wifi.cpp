// Базовые функции Arduino: Serial, millis(), delay() и т.д.
#include <Arduino.h>
// Подключение к Wi‑Fi на ESP8266.
#include <ESP8266WiFi.h>
// Работа с UDP-пакетами.
#include <WiFiUdp.h>
// Объявления функций и переменных Wi‑Fi модуля.
#include "wifi.h"
// Нужен доступ к x, y и кнопкам для отправки состояния геймпада.
#include "input.h"

// Имя Wi‑Fi сети, к которой подключается устройство.
const char* ssid = "фиви";
// Пароль от Wi‑Fi сети.
const char* pass = "12345678";

// Объект для отправки и приёма UDP-пакетов.
WiFiUDP udp;
// Порт, на котором происходит discovery и отправка данных.
const int PORT = 4210;

// Широковещательный адрес сети, нужен для поиска ПК.
IPAddress broadcastIP;
// IP-адрес найденного компьютера.
IPAddress pcIP;
// Флаг: компьютер уже найден и подтвердил соединение.
bool connected = false;
// Флаг: Wi‑Fi уже подключён, UDP уже запущен.
bool wifiReady = false;

// Время последней отправки состояния на ПК.
unsigned long lastSend = 0;

// Запускаем подключение к беспроводной сети.
void initWiFi() {
  // Режим станции: устройство подключается к роутеру, а не создаёт свою сеть.
  WiFi.mode(WIFI_STA);
  // Начинаем подключение по имени сети и паролю.
  WiFi.begin(ssid, pass);
  // Пишем статус в Serial для отладки.
  Serial.println("\nConnecting...");
}

// Обрабатываем подключение, поиск ПК и отправку состояния геймпада.
void handleWiFi() {
  // Этот блок выполняется до тех пор, пока не будет готово само Wi‑Fi соединение.
  if (!wifiReady) {
    // Если к роутеру ещё не подключились, пока ничего не делаем.
    if (WiFi.status() != WL_CONNECTED) {
      return;
    }

    // Сообщаем, что подключение к Wi‑Fi успешно.
    Serial.println("Connected!");

    // Получаем локальный IP устройства.
    IPAddress ip = WiFi.localIP();
    // Получаем маску подсети.
    IPAddress mask = WiFi.subnetMask();

    // Вычисляем broadcast-адрес для текущей сети.
    for (int i = 0; i < 4; i++) {
      broadcastIP[i] = ip[i] | (~mask[i]);
    }

    // Начинаем слушать UDP-порт.
    udp.begin(PORT);
    // Помечаем, что Wi‑Fi-часть полностью готова.
    wifiReady = true;
  }

  // Пока компьютер ещё не найден, выполняем discovery.
  if (!connected) {
    // Отправляем широковещательный пакет "HELLO" всем в локальной сети.
    udp.beginPacket(broadcastIP, PORT);
    udp.write("HELLO");
    udp.endPacket();

    // Даём сети немного времени на ответ.
    delay(200);

    // Проверяем, не пришёл ли входящий UDP-пакет.
    int packetSize = udp.parsePacket();
    // Если пакет есть, читаем его содержимое.
    if (packetSize) {
      // Буфер для короткого ответа, например "OK".
      char buf[16];
      // Читаем не больше 15 символов, чтобы остался 1 байт под конец строки.
      int len = udp.read(buf, 15);
      // Завершаем строку нулевым символом.
      buf[len] = 0;

      // Если компьютер ответил "OK", считаем соединение установленным.
      if (strcmp(buf, "OK") == 0) {
        // Запоминаем IP-адрес отправителя, чтобы дальше слать данные уже ему.
        pcIP = udp.remoteIP();
        // Подтверждаем, что discovery завершён.
        connected = true;
        // Пишем сообщение в Serial Monitor.
        Serial.println("Connected to PC!");
      }
    }
    // Пока discovery не завершён, выходим и не отправляем данные управления.
    return;
  }

  // После подключения отправляем состояние геймпада примерно каждые 30 мс.
  if (millis() - lastSend > 30) {

    // Буфер для строки вида "x,y,btn,btn2,btn3,btn4".
    char msg[64];
    // Собираем текущие значения в текстовый пакет.
    sprintf(msg, "%d,%d,%d,%d,%d,%d", x, y, btn, btn2, btn3, btn4);

    // Отправляем пакет на IP компьютера и заданный UDP-порт.
    udp.beginPacket(pcIP, PORT);
    udp.write(msg);
    udp.endPacket();

    // Обновляем время последней отправки.
    lastSend = millis();
  }
}
