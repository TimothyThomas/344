import socket
import sys

handle = 'hostA> '
def main():
    host, port = '', int(sys.argv[1])

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind((host, port))
        s.listen(1)
        conn, addr = s.accept()
        with conn:
            print('Connected by', addr)

            while True:
                data = conn.recv(500).decode('utf-8')
                if not data:
                    break
                print(data)

                message = input("{}".format(handle))
                if message == r'\quit':
                    break
                else:
                    conn.sendall(''.join([handle, message]).encode())

if __name__ == '__main__':
    main()
