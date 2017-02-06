import socket
import sys

def main():
    host, port = sys.argv[1], int(sys.argv[2])
    handle = input('Enter your handle (10 characters max): ') + '> '

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((host, port))
        while True:
            message = input("{}".format(handle))
            if message == r'\quit':
                break
            else:
                s.sendall(''.join([handle, message]).encode())

            data = s.recv(500).decode('utf-8')
            if not data:
                break
            print(data)

if __name__ == '__main__':
    main()

