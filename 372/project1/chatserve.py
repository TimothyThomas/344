"""
    Timothy Thomas
    CS 372, Winter 2017
    Project 1

    A simple python TCP server that allows sending/receiving messages to a client.

    usage:  python chatserve.py port_number

    sources:
        1.  https://docs.python.org/3.5/library/socket.html
        2.  Computer Networking A Top-Down Approach (7th ed.), Kurose and Ross
"""
import socket
import sys

SERVER_HANDLE = 'server> '
MAX_MSG_SIZE = 500
HOST = ''  # symbolic name meaning all available interfaces

def accept_connection(socket):
    while True:
        print('Waiting for new connection...', flush=True)
        conn, addr = socket.accept()
        print('Connected established with {}'.format(addr), flush=True)
        chat(conn, addr)
        print('Connection terminated.')


def chat(connection, address):
    with connection:
        while True:
            data = connection.recv(MAX_MSG_SIZE).decode()

            if not data:
                break
            print(data, flush=True)

            message = input("{}".format(SERVER_HANDLE))
            connection.sendall(''.join([SERVER_HANDLE, message]).encode())

            if message == r'\quit':
                break


def main():
    port = int(sys.argv[1])

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind((HOST, port))
        s.listen(1)

        print("Socket created and bound to port {}".format(port))

        # Continue accepting connections until SIGINT received
        while True:
            try:
                accept_connection(s)
            except KeyboardInterrupt:
                print(" Program terminated. Exiting.")
                sys.exit(0)

if __name__ == '__main__':
    main()
