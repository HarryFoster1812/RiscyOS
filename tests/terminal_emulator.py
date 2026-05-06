import os
import pty
import threading
import sys

from board_comms import BoardComms, CMD


def handle_master(master_fd, board: BoardComms):
    """
    This is where we translate between:
    - screen (raw bytes)
    - your structured protocol
    """

    while True:
        try:
            data = os.read(master_fd, 1024)
            if not data:
                continue

            # For now: simple byte-wise FIFO write
            for b in data:
                board._w8(CMD.PeripheralWrite)
                board._w8(0)  # FIFO
                board._w8(b)
                board._r8()   # consume ACK

        except Exception as e:
            print(f"[ERR write] {e}")
            break


def poll_board(master_fd, board: BoardComms):
    """
    Continuously read FIFO from board and push to PTY
    """
    while True:
        try:
            board._w8(CMD.PeripheralRead)
            board._w8(0)

            while True:
                flag = board._r8()
                if flag != 0xFF:
                    break
                byte = board._r8()
                os.write(master_fd, bytes([byte]))

        except Exception as e:
            print(f"[ERR read] {e}")
            break


def main():
    master_fd, slave_fd = pty.openpty()
    slave_name = os.ttyname(slave_fd)

    print(f"\n[+] Virtual serial port created: {slave_name}\n")
    print(f"Use it with: screen {slave_name} 115200\n")

    with BoardComms("/dev/ttyUSB1") as board:
        t1 = threading.Thread(target=handle_master, args=(master_fd, board), daemon=True)
        t2 = threading.Thread(target=poll_board, args=(master_fd, board), daemon=True)

        t1.start()
        t2.start()

        t1.join()
        t2.join()


if __name__ == "__main__":
    main()
