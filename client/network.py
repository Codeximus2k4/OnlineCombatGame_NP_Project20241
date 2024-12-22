import socket
import threading

class NetworkManager:
    def __init__(self, server_addr, server_port):
        self.server_addr = server_addr
        self.server_port = server_port
        
        # TCP Socket for main communication
        self.tcp_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        
        # UDP Socket for input
        self.udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        
        # Message buffer
        self.buffer_size = 1024
        
        # Input callback
        self.input_callback = None
    
    def connect(self):
        """Establish connection to the server"""
        try:
            self.tcp_socket.connect((self.server_addr, self.server_port))
            print("Connected to server")
        except Exception as e:
            print(f"Connection error: {e}")
            raise
    
    def send_tcp_message(self, message):
        """Send a message via TCP"""
        try:
            self.tcp_socket.send(message)
        except Exception as e:
            print(f"TCP send error: {e}")
    
    def send_udp_input(self, message, addr):
        """Send input via UDP"""
        try:
            self.udp_socket.sendto(message, addr)
        except Exception as e:
            print(f"UDP send error: {e}")

    def receive_tcp_message(self, buff_size=1024):
        """Receive message via TCP"""
        message = self.tcp_socket.recv(buff_size)
        return message
        
    
    def start_input_listener(self, callback):
        """Start listening for UDP inputs"""
        self.input_callback = callback
        
        def receive_messages():
            while True:
                try:
                    data, _ = self.udp_socket.recvfrom(self.buffer_size)
                    if self.input_callback:
                        self.input_callback(data)
                except Exception as e:
                    print(f"Receive error: {e}")
                    break
        
        # Start a daemon thread for receiving messages
        thread = threading.Thread(target=receive_messages)
        thread.daemon = True
        thread.start()
    
    def close(self):
        """Close all socket connections"""
        self.tcp_socket.close()
        self.udp_socket.close()