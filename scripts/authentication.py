import socket

class Authentication:
    def __init__(self, username, password):
        self.username = username
        self.password = password
        self.is_logged_in = False  # Add this to track login status
    
    def login(self, ipv4, port):
        BUFFSIZE = 1024
        authen_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server_address = (ipv4, port)
        print(f"Connecting to {server_address} port")
        print('Login...')
        authen_socket.connect(server_address)

        try:
            message = f"LOGIN\n{self.username}\n{self.password}"
            authen_socket.sendall(message.encode("utf-8"))
            data = authen_socket.recv(BUFFSIZE).decode("utf-8")
            print(data)
            if "Login Successfully" in data:
                self.is_logged_in = True  # Mark as logged in
            else:
                self.is_logged_in = False

        finally:
            authen_socket.close()
        return data

    def register(self, ipv4, port):
        BUFFSIZE = 1024
        authen_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server_address = (ipv4, port)
        print(f"Connecting to {server_address} port")
        print('Register...')
        authen_socket.connect(server_address)

        try:
            message = f"REGISTER\n{self.username}\n{self.password}"
            authen_socket.sendall(message.encode("utf-8"))
            data = authen_socket.recv(BUFFSIZE).decode("utf-8")
            print(data)
            if "Register Successfully" in data:
                self.is_logged_in = True
            else:
                self.is_logged_in = False

        finally:
            authen_socket.close()
        return data
