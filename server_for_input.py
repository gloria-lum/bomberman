import socket
import time

s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)

s.setblocking(False)

while True:
    command = input("Enter command (u, d, l, r-): ")
    s.sendto(bytes(command, 'utf-8'), ('127.0.0.1', 9999))

s.close()
