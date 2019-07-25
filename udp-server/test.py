'''
TEST
'''
import socket

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

server_address = ('127.0.0.1', 3333)
message = b'\xca\xfe\x01\x01'+ b'\x00'*800 + b'\xcd'

try:
    print('TX: {!r}'.format(message))
    sent = sock.sendto(message, server_address)

finally:
    print('closing socket')
    sock.close()
