import os
import sys
import pygame
import config
import struct
from client.network import NetworkManager

def load_image(path):
    img = pygame.image.load(config.BASE_IMG_PATH+path).convert_alpha()
    img.set_colorkey((0,0,0))
    return img
def load_images(path):
    images = []
    for img in sorted(os.listdir(config.BASE_IMG_PATH +path)):
        images.append(load_image(path + '/'+img))
    return images

def render_text(text, font, color):
    """
    Render text in desired font and color
    """
    return font.render(text, True, color)

def get_font(size):
        """Returns font in desired size"""
        return pygame.font.Font(config.FONT_PATH, size)

def login_register_request(username, password, mode):
    """
    Sending login/register request and information to server
    Handling response from server

    :param username: The username of client
    :param password: The password of client
    :param mode: login or register
    """
    # message components
    request_type = ""
    user_name = username
    pass_word = password

    # define request type
    if mode == "register":
        request_type = "1"
    else:
        request_type = "2"

    # Convert components to bytes
    request_type_byte = request_type.encode("ascii")
    username_length_byte = len(username).to_bytes(1, "big")
    username_data = user_name.encode("ascii")
    password_length_byte = len(password).to_bytes(1, "big")
    password_data = password.encode("ascii")

    # Combine all components into a single message
    message = request_type_byte + username_length_byte + username_data + \
        password_length_byte + password_data
    
    # Connect the socket to the server
    login_register_socket = NetworkManager(
        server_addr=config.SERVER_ADDR, 
        server_port=config.SERVER_PORT)
    login_register_socket.connect()
    
    # Send the message to the server
    login_register_socket.send_tcp_message(message)

    # Receive the response
    response = login_register_socket.receive_tcp_message()

    # Close the socket
    login_register_socket.close()

    return response.decode()

def host_room_request(user_id: int):
    """
    Send the host room request to server
    Handle the response ad TCP address of the game room

    user_id: the host's user id
    """

    # Message components
    request_type = "4"
    user_id = user_id

    # Convert components to bytes
    request_type_byte = request_type.encode("ascii")
    user_id_byte = user_id.to_bytes(1, "big")

    # Combine all components into a single message
    message = request_type_byte + user_id_byte

    # connect the socket to the server
    host_room_socket = NetworkManager(
        server_addr=config.SERVER_ADDR,
        server_port=config.SERVER_PORT
    )
    host_room_socket.connect()

    # Send the message to the server
    host_room_socket.send_tcp_message(message) 

    # Receive the response 
    response = host_room_socket.receive_tcp_message(buff_size=6)

    # Close the socket
    host_room_socket.close()

    # Parse the response
    response = response.decode()
    status = response[1]  # Second byte: status (ASCII character)
    room_id = ord(response[2]) # Third byte: room_id 
    tcp_port = struct.unpack("!H", response[3:5].encode("utf-8"))[0] # Last 2 bytes: TCP port (network byte order)
    print(status, room_id, tcp_port)
    return status, room_id, tcp_port

def join_room_request(user_id: int, host_room_socket: NetworkManager):
    """Send the join room request to the game room server"""
    # Message component
    request_type = "6"
    user_id = user_id

    # Convert components into bytes
    request_type_byte = request_type.encode("ascii")
    user_id_byte = user_id.to_bytes(1, "big")

    # Combine
    message = request_type_byte + user_id_byte

    # Send the message to the room server
    host_room_socket.send_tcp_message(message)

    # Receive the response
    response = host_room_socket.receive_tcp_message(buff_size=2)
    return response.decode()
    


class Animation:
    def __init__(self, images ,img_dur = 5, loop =True):
        self.images=  images
        self.img_duration =  img_dur
        self.loop =loop
        self.done = False
        self.frame  =0
    def copy(self):
        return Animation(self.images, self.img_duration, self.loop)
    
    def update(self):
        if self.loop:
            self.frame =  (self.frame+1)%(self.img_duration* len(self.images))
        else :
            self.frame = min(self.frame+1 , self.img_duration* len(self.images) )
            if self.frame >= self.img_duration* len(self.images) -1:
                self.done = True
        
    def get_img(self):
        return self.images[int(self.frame/self.img_duration)]


