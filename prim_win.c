#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 4210
#define TARGET_FPS 120
#define DEADZONE   0.01
#define SPEEDZONE  0.98
#define MAX_SPEED  7.0

static double accum_x = 0, accum_y = 0;

double process_axis(int raw) {
    double n = (raw - 512.0) / 512.0;
    double abs_n = fabs(n);
    if (abs_n < DEADZONE)   return 0;
    if (abs_n < SPEEDZONE)  return n * 0.5 * MAX_SPEED;
    return n * 1.0 * MAX_SPEED;
}

void mouse_move(int dx, int dy) {
    INPUT input = {0};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_MOVE;
    input.mi.dx = dx;
    input.mi.dy = dy;
    SendInput(1, &input, sizeof(INPUT));
}

void mouse_button(int button, int down) {
    INPUT input = {0};
    input.type = INPUT_MOUSE;
    if (button == 0) input.mi.dwFlags = down ? MOUSEEVENTF_LEFTDOWN  : MOUSEEVENTF_LEFTUP;
    else             input.mi.dwFlags = down ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
    SendInput(1, &input, sizeof(INPUT));
}

int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
    u_long nb = 1;
    ioctlsocket(sock, FIONBIO, &nb);

    struct sockaddr_in addr = {0};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;
    bind(sock, (struct sockaddr*)&addr, sizeof(addr));

    struct sockaddr_in esp_addr = {0};
    int esp_known = 0;
    int mode = 0;

    int last_btn1 = 0, last_btn2 = 0;
    char last_data[64] = "512,512,0,0,0,0";

    printf("Listening on port %d...\n", PORT);

    long target_us = 1000000 / TARGET_FPS;

    while (1) {
        LARGE_INTEGER t0, t1, freq;
        QueryPerformanceFrequency(&freq);
        QueryPerformanceCounter(&t0);

        char buf[256];
        struct sockaddr_in from = {0};
        int fromlen = sizeof(from);
        int n = recvfrom(sock, buf, sizeof(buf)-1, 0, (struct sockaddr*)&from, &fromlen);
        if (n > 0) {
            buf[n] = '\0';
            // trim \r\n
            for (int i = n-1; i >= 0 && (buf[i]=='\r'||buf[i]=='\n'); i--) buf[i]='\0';
            strncpy(last_data, buf, sizeof(last_data)-1);

            if (mode == 0 && strcmp(buf, "HELLO") == 0) {
                esp_addr = from;
                esp_addr.sin_port = htons(PORT);
                esp_known = 1;
                sendto(sock, "OK", 2, 0, (struct sockaddr*)&esp_addr, sizeof(esp_addr));
                mode = 1;
                printf("ESP found: %s\n", inet_ntoa(from.sin_addr));
                printf("Switched to normal mode\n");
            }
        }

        if (mode == 0) goto throttle;

        {
            int x, y, btn0, btn1, btn2, btn3;
            if (sscanf(last_data, "%d,%d,%d,%d,%d,%d", &x, &y, &btn0, &btn1, &btn2, &btn3) != 6)
                goto throttle;

            double dx = process_axis(x);
            double dy = process_axis(y);
            accum_x += dx;
            accum_y += dy;
            int move_x = (int)accum_x;
            int move_y = (int)accum_y;
            accum_x -= move_x;
            accum_y -= move_y;
            if (move_x || move_y) mouse_move(move_x, move_y);

            if (btn1 == 1 && last_btn1 == 0) mouse_button(0, 1);
            if (btn1 == 0 && last_btn1 == 1) mouse_button(0, 0);
            last_btn1 = btn1;

            if (btn2 == 1 && last_btn2 == 0) mouse_button(1, 1);
            if (btn2 == 0 && last_btn2 == 1) mouse_button(1, 0);
            last_btn2 = btn2;
        }

throttle:
        QueryPerformanceCounter(&t1);
        long elapsed_us = (long)((t1.QuadPart - t0.QuadPart) * 1000000 / freq.QuadPart);
        if (elapsed_us < target_us)
            Sleep((target_us - elapsed_us) / 1000);
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}
