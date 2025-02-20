import os
import pygame
import struct
from client.network import NetworkManager
import config

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
    """Return font in desired size"""
    return pygame.font.Font(config.FONT_PATH,size)

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

def fetch_room_list_request():
    """Message type 3: Fetch the list of available rooms from the server."""
    # get message component and convert into byte
    message = "3"
    message = message.encode()

    # Connect to the server
    fetch_room_list_socket = NetworkManager(
        server_addr=config.SERVER_ADDR,
        server_port=config.SERVER_PORT
    )
    fetch_room_list_socket.connect()

    # Send the message to the server
    fetch_room_list_socket.send_tcp_message(message)

    # Get the components
    room_list = []
    message_type = fetch_room_list_socket.receive_tcp_message(buff_size=1)
    print(f"Message type: {message_type}" )

    rooms_num = fetch_room_list_socket.receive_tcp_message(buff_size=1)
    rooms_num = int(rooms_num)
    print(f"Number of room: {rooms_num}")
    while rooms_num:
        room_id = fetch_room_list_socket.receive_tcp_message(buff_size=1)
        room_id = int(room_id)
        print(f"Room ID: {room_id}")

        players_num = fetch_room_list_socket.receive_tcp_message(buff_size=1)
        players_num = int(players_num)
        print(f"Number of players: {players_num}")

        room_list.append({"room_id": room_id, "total_players": players_num})
        rooms_num = rooms_num - 1

    fetch_room_list_socket.close()
    print(room_list)
    return room_list
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

def connect_room_request(user_id: int, host_room_socket: NetworkManager):
    """Message type 6: Send the connect room request to the game room server"""
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


def join_room_request(user_id: int, room_id: int):
    """Message type 5: Send the join room request to the main server"""
    
    # MEssage component
    request_type = "5"
    user_id = user_id
    room_id = room_id

    # Convert components into bytes
    request_type_byte = request_type.encode("ascii")
    user_id_byte = user_id.to_bytes(1, "big")
    room_id_byte = bytes([room_id])

    #Combine
    message = request_type_byte + user_id_byte + room_id_byte
    print(message)

    # Connect the socket to the server
    join_room_socket = NetworkManager(
        server_addr=config.SERVER_ADDR,
        server_port=config.SERVER_PORT
    )
    join_room_socket.connect()
    # Send the message to the server
    join_room_socket.send_tcp_message(message)

    print("Sent the message to server to request join room")

    # Receive the response
    response = join_room_socket.receive_tcp_message(buff_size=5)

    # Parse the response
    response = response.decode()
    status = response[1]
    room_tcp_port = None
    if status == "1":
        room_tcp_port = struct.unpack("!H", response[3:5].encode("utf-8"))[0]
    else:
        room_tcp_port = -1
    return room_tcp_port

def rank_request():
    """Message type 9: Request top 5 players on server"""
    # Connect to the server TCP network
    rank_socket = NetworkManager(
        server_addr=config.SERVER_ADDR,
        server_port=config.SERVER_PORT
    )
    rank_socket.connect()

    # Request message type 9 from server
    message = '9'
    message = message.encode()
    rank_socket.send_tcp_message(message=message)

    # Receive the response
    top5 = []
    
    response = rank_socket.receive_tcp_message(buff_size=1) # Receive the message type (9)
    num_players=5
    while num_players:
        # Get the username length of the player
        username_length = rank_socket.receive_tcp_message(buff_size=1).decode() # 1 byte
        username_length = ord(username_length)

        # Get the username of the player
        username = rank_socket.receive_tcp_message(buff_size=username_length).decode() # max 127 bytes

        # Get the number of games which the player played
        num_game_played= rank_socket.receive_tcp_message(buff_size=1).decode() # 1 byte
        num_game_played = ord(num_game_played)

        # Get the score of the player
        score = rank_socket.receive_tcp_message(buff_size=2) # 2 bytes
        score = struct.unpack("!H", score)[0]
        
        # Add to the list
        top5.append({"username": username, "num_game": num_game_played, "score": score})

        print(f"Username: {username} | Number of games: {num_game_played} | Score: {score}\n")

        num_players = num_players - 1
    
    rank_socket.close()
    return top5

def my_stats_request(user_id: int):
    """Message type 10: Request my stats"""
    # Connect the socket to the server
    my_stats_socket = NetworkManager(
        server_addr=config.SERVER_ADDR,
        server_port=config.SERVER_PORT
    )
    my_stats_socket.connect()

    # Send the request type 10 to the server
    request_type = 58
    user_id = user_id

    # Convert to bytes
    request_type_byte = bytes([request_type])
    user_id = bytes([user_id])
    message = request_type_byte + user_id
    print(f"Message: {message}")
    # Send the message request to the server
    my_stats_socket.send_tcp_message(message=message)
    print("Successfully sent the message")

    # Receive the response
    my_stats = []
    # Get response type
    response_type = my_stats_socket.receive_tcp_message(buff_size=1).decode() # 1 byte (response type)
    
    # Get username length
    username_length = my_stats_socket.receive_tcp_message(buff_size=1).decode() # 1 byte (username length)
    username_length = ord(username_length)
    
    # Get username
    username = my_stats_socket.receive_tcp_message(buff_size=username_length).decode() # Max 127 byte
    
    # Get the number of game played
    num_game = my_stats_socket.receive_tcp_message(buff_size=1).decode() # 1 byte (number of game playerd)
    num_game = ord(num_game)

    # Get the score
    score = my_stats_socket.receive_tcp_message(buff_size=2) # 2 byte (score)
    score = struct.unpack("!H", score)[0]

    my_stats_socket.close()
    my_stats = {"username": username, "num_game": num_game, "score": score}
    
    return my_stats

def update_ready_request(user_id: int, is_ready:int, update_ready_socket: NetworkManager):
    """Message type 7: Update the ready state of each player"""
    # Message components
    request_type = "7"
    user_id = user_id
    is_ready = is_ready

    # Convert to bytes and combine into message
    request_type_byte = request_type.encode("ascii")
    user_id_byte = bytes([user_id])
    is_ready_byte = bytes([is_ready])
    message = request_type_byte + user_id_byte + is_ready_byte

    # Send the message
    update_ready_socket.send_tcp_message(message)

def game_mode_request(user_id: int, game_mode: int, game_mode_socket: NetworkManager):
    """Message type 11: Request the game mode for game play"""
    # Get the message components
    request_type = 59 # request type 11
    user_id = user_id
    game_mode = game_mode

    # Convert to bytes and combine into 1 message
    request_type_byte = bytes([request_type])
    user_id_byte = bytes([user_id])
    game_mode_byte = bytes([game_mode])
    message = request_type_byte + user_id_byte + game_mode_byte

    # Send the request to the server
    game_mode_socket.send_tcp_message(message)

def hero_request(user_id: int, hero_id: int, hero_socket: NetworkManager):
    """Message type 12: Request the hero type"""
    request_type = 60 # request type 12
    user_id = user_id
    hero_id = hero_id

    # Convert to bytes and combine into 1 message
    request_type_byte = bytes([request_type])
    user_id_byte = bytes([user_id])
    hero_id_byte = bytes([hero_id])
    message = request_type_byte + user_id_byte + hero_id_byte


    # Send the request to the server
    hero_socket.send_tcp_message(message)

def logout_request(user_id: int):
    """Message type 13: Requst log out"""
    # Message components
    request_type = 61 # Request type 13
    user_id = user_id

    # Convert to bytes and combine into 1 message
    request_type_byte = bytes([request_type])
    user_id_byte = bytes([user_id])
    message = request_type_byte + user_id_byte

    # Connect to the server
    logout_socket = NetworkManager(
        server_addr=config.SERVER_ADDR,
        server_port=config.SERVER_PORT
    )
    logout_socket.connect()

    # Send the message to the server
    logout_socket.send_tcp_message(message)
    logout_socket.close()


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
            self.frame = min(self.frame+1 , self.img_duration* len(self.images)-1)
            if self.frame >= self.img_duration* len(self.images) -1:
                self.done = True
    def revert_frame(self):
        self.frame -=1
    def get_size(self):
        return self.images[int(self.frame/self.img_duration)].get_size()
    def get_img(self):
        return self.images[int(self.frame/self.img_duration)]


