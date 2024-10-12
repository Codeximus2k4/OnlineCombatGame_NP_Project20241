import socket

def start_client():
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    message = "Hello, Server!"
    
    # Send the message to the server
    client_socket.sendto(message.encode('utf-8'), ('localhost', 9999))

    # Receive response from server
    response, server_address = client_socket.recvfrom(1024)
    print(f"Received from server: {response}")

    client_socket.close()

if __name__ == "__main__":
    start_client()
