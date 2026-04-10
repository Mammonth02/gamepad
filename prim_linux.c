#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <linux/uinput.h>

#define PORT 4210
#define TARGET_FPS 120
#define DEADZONE   0.01
#define SPEEDZONE  0.98
#define MAX_SPEED  7.0

static double accum_x = 0, accum_y = 0;

double process_axis(int raw) {
    double n = (raw - 512.0) / 512.0;
    double abs_n = fabs(n);
    if (abs_n < DEADZONE)  return 0;
    if (abs_n < SPEEDZONE) return n * 0.5 * MAX_SPEED;
    return n * 1.0 * MAX_SPEED;
}

void emit(int fd, int type, int code, int val) {
    struct input_event ev = {0};
    ev.type  = type;
    ev.code  = code;
    ev.value = val;
    write(fd, &ev, sizeof(ev));
}

int uinput_init() {
    int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd < 0) { perror("open /dev/uinput"); exit(1); }

    ioctl(fd, UI_SET_EVBIT,  EV_KEY);
    ioctl(fd, UI_SET_KEYBIT, BTN_LEFT);
    ioctl(fd, UI_SET_KEYBIT, BTN_RIGHT);
    ioctl(fd, UI_SET_EVBIT,  EV_REL);
    ioctl(fd, UI_SET_RELBIT, REL_X);
    ioctl(fd, UI_SET_RELBIT, REL_Y);

    struct uinput_setup usetup = {0};
    usetup.id.bustype = BUS_USB;
    usetup.id.vendor  = 0x1234;
    usetup.id.product = 0x5678;
    strcpy(usetup.name, "esp-mouse");
    ioctl(fd, UI_DEV_SETUP, &usetup);
    ioctl(fd, UI_DEV_CREATE);
    sleep(1);
    return fd;
}

int main() {
    int ufd = uinput_init();

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    fcntl(sock, F_SETFL, O_NONBLOCK);

    struct sockaddr_in addr = {0};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;
    bind(sock, (struct sockaddr*)&addr, sizeof(addr));

    struct sockaddr_in esp_addr = {0};
    int mode = 0;
    int last_btn1 = 0, last_btn2 = 0;
    char last_data[256] = "512,512,0,0,0,0";
    long target_ns = 1000000000 / TARGET_FPS;

    printf("Listening on port %d...\n", PORT);

    while (1) {
        struct timespec t0, t1;
        clock_gettime(CLOCK_MONOTONIC, &t0);

        char buf[256];
        struct sockaddr_in from = {0};
        socklen_t fromlen = sizeof(from);
        int n = recvfrom(sock, buf, sizeof(buf)-1, 0, (struct sockaddr*)&from, &fromlen);
        if (n > 0) {
            buf[n] = '\0';
            for (int i = n-1; i >= 0 && (buf[i]=='\r'||buf[i]=='\n'); i--) buf[i]='\0';
            snprintf(last_data, sizeof(last_data), "%s", buf);
            if (mode == 0 && strcmp(buf, "HELLO") == 0) {
                esp_addr = from;
                esp_addr.sin_port = htons(PORT);
                sendto(sock, "OK", 2, 0, (struct sockaddr*)&esp_addr, sizeof(esp_addr));
                mode = 1;
                printf("ESP found: %s\nSwitched to normal mode\n", inet_ntoa(from.sin_addr));
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

            if (move_x || move_y) {
                emit(ufd, EV_REL, REL_X, move_x);
                emit(ufd, EV_REL, REL_Y, move_y);
                emit(ufd, EV_SYN, SYN_REPORT, 0);
            }

            if (btn1 != last_btn1) { emit(ufd, EV_KEY, BTN_LEFT,  btn1); emit(ufd, EV_SYN, SYN_REPORT, 0); }
            if (btn2 != last_btn2) { emit(ufd, EV_KEY, BTN_RIGHT, btn2); emit(ufd, EV_SYN, SYN_REPORT, 0); }
            last_btn1 = btn1;
            last_btn2 = btn2;
        }

throttle:
        clock_gettime(CLOCK_MONOTONIC, &t1);
        long elapsed_ns = (t1.tv_sec - t0.tv_sec) * 1000000000L + (t1.tv_nsec - t0.tv_nsec);
        if (elapsed_ns < target_ns) {
            struct timespec ts = { 0, target_ns - elapsed_ns };
            nanosleep(&ts, NULL);
        }
    }

    ioctl(ufd, UI_DEV_DESTROY);
    close(ufd);
    close(sock);
    return 0;
}
