import socket
import time
from pynput.mouse import Controller, Button

def main():
    mouse = Controller()

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(("0.0.0.0", 4210))
    sock.setblocking(False)

    esp_ip = None
    PORT = 4210

    mode = False

    print("Listening...")

    deadzone = 0.01
    speedzone = 0.98
    max_speed = 7

    last_data = "512,512,0"

    last_btn1 = 0
    last_btn2 = 0

    accum_x = 0
    accum_y = 0

    target_dt = 1 / 120 

    def process_axis(n):
        n = (n - 512) / 512

        abs_n = abs(n)

        if abs_n < deadzone:
            return 0

        if abs_n < speedzone:
            return n * 0.5 * max_speed
        else:
            return n * 1 * max_speed
        
    while True:
        start = time.time()
        try:
            data, addr = sock.recvfrom(1024)
            last_data = data.decode().strip()
        except:
            pass

        if mode == 0:
            if last_data == "HELLO":
                esp_ip = addr[0]
                print("ESP found:", esp_ip)

                sock.sendto(b"OK", (esp_ip, PORT))

                mode = 1  # <-- переключение режима
                print("Switched to normal mode")

        try:
            x, y, btn0, btn1, btn2, btn3 = map(int, last_data.split(","))
        except:
            continue

        dx = process_axis(x)
        dy = process_axis(y)

        accum_x += dx
        accum_y += dy

        move_x = int(accum_x)
        move_y = int(accum_y)

        accum_x -= move_x
        accum_y -= move_y

        mouse.move(move_x, move_y)

        # --- кнопка1 ---
        if btn1 == 1 and last_btn1 == 0:
            mouse.press(Button.left)   # нажали
        if btn1 == 0 and last_btn1 == 1:
            mouse.release(Button.left) # отпустили
        last_btn1 = btn1

            # --- кнопка2 ---
        if btn2 == 1 and last_btn2 == 0:
            mouse.press(Button.right)   # нажали
        if btn2 == 0 and last_btn2 == 1:
            mouse.release(Button.right) # отпустили
        last_btn2 = btn2

        # --- ограничение FPS ---
        elapsed = time.time() - start
        if elapsed < target_dt:
            time.sleep(target_dt - elapsed)

if __name__ == "__main__":
    main()