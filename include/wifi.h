// Защита от повторного включения заголовочного файла.
#ifndef WIFI_H
// Определяем символ, показывающий, что файл уже подключался.
#define WIFI_H

// Глобальный флаг успешного соединения с ПК.
extern bool connected;
extern bool wifiReady;

extern bool wifiStarted;


extern int networksCount;

extern String ssids[20];

extern char ssid[64];

extern char pass[64];

// Обработка сети: подключение, поиск ПК и отправка пакетов.
void handleWiFi();

void scanWIFI();

// Конец защитного блока.
#endif
