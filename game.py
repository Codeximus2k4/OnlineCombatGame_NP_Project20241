import os
import sys
import math
import pygame
import socket
import threading
import time
import queue
import select
from pygame import SCRAP_TEXT

FONT_PATH = "data/images/menuAssets/font.ttf"
ADDRESS = "127.0.0.1"
PORT = 7070

from scripts.utils import load_image, load_images, Animation
from scripts.entities import PhysicsEntity, Player
from scripts.button import Button
class Game:
    def __init__(self):
        pygame.init()

        pygame.display.set_caption('combat game')
        
        self.clock  = pygame.time.Clock()
        self.display =  pygame.Surface((640,480), pygame.SRCALPHA)
        self.display_2 =  pygame.Surface((640,480))
        self.horizontal_movement = [False,False]
        self.game_state = "menu"
        self.assets = {}
        self.background_offset_y = -80
        self.client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    def get_font(self, size):  # Returns Press-Start-2P in the desired size
        return pygame.font.Font(FONT_PATH, size)
    
    def login_register(self):
        # Constants
        BG_PATH = "data/images/menuAssets/Background.png"
        FONT_PATH = "data/images/menuAssets/font.ttf"
        SCREEN_WIDTH, SCREEN_HEIGHT = 400, 300
        WHITE = (255, 255, 255)
        RED = (255, 0, 0)
        BLACK = (0, 0, 0)
        GREEN = (0, 128, 0)
        BROWN = (182,143,64)
        COLOR_ACTIVE = pygame.Color('lightskyblue3')
        COLOR_PASSIVE = pygame.Color('white')
        DISPLAY_LIMIT = 10  # Display only the last 10 characters
        BUFF_SIZE = 1024
        SERVER_ADDR = "127.0.0.1"
        SERVER_PORT = 5500

        # Initialize Pygame and Font
        pygame.init()
        clock = pygame.time.Clock()
        screen = pygame.display.set_mode([SCREEN_WIDTH, SCREEN_HEIGHT])
        base_font = pygame.font.Font(FONT_PATH, 15)
        base_bg = pygame.image.load(BG_PATH)
        response_font = pygame.font.Font(FONT_PATH, 12)

        # Interface Mode (login or register)
        mode = "login"

        # Input Field Setup
        username_text = ""
        password_text = ""
        active_input = None  # Track which input is active

        # Rectangles for Input Fields and Buttons
        username_rect = pygame.Rect(120, 100, 170, 32)
        password_rect = pygame.Rect(120, 150, 170, 32)
        login_button_rect = pygame.Rect(50, 250, 135, 32)
        register_button_rect = pygame.Rect(230, 250, 135, 32)

        # Cursor settings
        cursor_visible = True
        cursor_timer = 0
        cursor_flash_rate = 500  # Milliseconds

        
        
        def render_text(text, font, color):
            return font.render(text, True, color)
        
        def login_register_request(username, password, mode=mode):
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
            self.client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.client_socket.connect((SERVER_ADDR, SERVER_PORT))
            
            # Send the message to the server
            self.client_socket.send(message)

            # Receive the response
            response = self.client_socket.recv(BUFF_SIZE)

            return response.decode()
            
        response_text = ""
        response_flag = True # flag to to distinguish successful response and successful response
        while True:
            pygame.init()
            screen.blit(base_bg, (0,0))
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    pygame.quit()
                    sys.exit()

                # Handle mouse click
                if event.type == pygame.MOUSEBUTTONDOWN:
                    if username_rect.collidepoint(event.pos):
                        active_input = "username"
                    elif password_rect.collidepoint(event.pos):
                        active_input = "password"
                    elif login_button_rect.collidepoint(event.pos):
                        if mode == "login":
                            print("Attempting to log in with:", username_text, password_text)
                            
                            # get response of login request
                            response = login_register_request(username=username_text, password=password_text, mode=mode)
                            print(response)
                            if response[1] == "1": 
                                pygame.quit()
                                response_flag = True
                                self.handle_connection_to_room(response)
                                continue
                            else:
                                response_text = "Unsuccessfully login"
                                response_flag = False
                        else:
                            mode = "login"
                    elif register_button_rect.collidepoint(event.pos):
                        if mode == "register":
                            print("Attempting to register with:", username_text, password_text)
                            
                            # get response of register request
                            response = login_register_request(username=username_text, password=password_text, mode=mode)
                            if response[1] == "1":
                                pygame.quit()
                                mode = "login"
                                response_flag = True
                            else:
                                response_text = "Unsuccessfully register"
                                response_flag = False
                        else:
                            mode = "register"
                    else:
                        active_input = None

                # Handle keyboard input
                if event.type == pygame.KEYDOWN:
                    if active_input == "username":
                        if event.key == pygame.K_BACKSPACE:
                            username_text = username_text[:-1]
                        else:
                            username_text += event.unicode
                    elif active_input == "password":
                        if event.key == pygame.K_BACKSPACE:
                            password_text = password_text[:-1]
                        else:
                            password_text += event.unicode
           
            # Blinking cursor logic
            cursor_timer += clock.get_time()
            if cursor_timer >= cursor_flash_rate:
                cursor_timer = 0
                cursor_visible = not cursor_visible

            # Input boxes and their colors
            username_color = COLOR_ACTIVE if active_input == "username" else COLOR_PASSIVE
            password_color = COLOR_ACTIVE if active_input == "password" else COLOR_PASSIVE
            pygame.draw.rect(screen, username_color, username_rect)
            pygame.draw.rect(screen, password_color, password_rect)

            # Render text for input fields with DISPLAY_LIMIT
            username_display_text = username_text[-DISPLAY_LIMIT:]  # Display only the last 10 characters
            password_display_text = "*" * len(password_text[-DISPLAY_LIMIT:])
            username_surface = base_font.render(username_display_text, True, BLACK)
            password_surface = base_font.render(password_display_text, True, BLACK)
            screen.blit(username_surface, (username_rect.x + 5, username_rect.y + 10))
            screen.blit(password_surface, (password_rect.x + 5, password_rect.y + 10))

            # Draw the blinking cursor in active input box
            if cursor_visible:
                if active_input == "username":
                    cursor_x = username_rect.x + 5 + username_surface.get_width() + 2
                    pygame.draw.line(screen, WHITE, (cursor_x, username_rect.y + 5), (cursor_x, username_rect.y + 27), 2)
                elif active_input == "password":
                    cursor_x = password_rect.x + 5 + password_surface.get_width() + 2
                    pygame.draw.line(screen, WHITE, (cursor_x, password_rect.y + 5), (cursor_x, password_rect.y + 27), 2)

            # Draw buttons and labels
            # Create semi-transparent surfaces for the buttons
            login_button_surface = pygame.Surface((login_button_rect.width, login_button_rect.height), pygame.SRCALPHA)
            register_button_surface = pygame.Surface((register_button_rect.width, register_button_rect.height), pygame.SRCALPHA)

            # Set the alpha value (0 is fully transparent, 255 is fully opaque)
            login_button_surface.set_alpha(150)  # Adjust the value for desired transparency
            register_button_surface.set_alpha(150)

            # Fill the button surfaces with a color (with an alpha value already set)
            login_button_surface.fill(COLOR_PASSIVE)
            register_button_surface.fill(COLOR_PASSIVE)

            # Blit the semi-transparent button surfaces onto the main screen
            screen.blit(login_button_surface, (login_button_rect.x, login_button_rect.y))
            screen.blit(register_button_surface, (register_button_rect.x, register_button_rect.y))
            login_label = render_text("Login", base_font, WHITE)
            register_label = render_text("Register", base_font, WHITE)
            screen.blit(login_label, (login_button_rect.x + 30, login_button_rect.y + 10))
            screen.blit(register_label, (register_button_rect.x + 10, register_button_rect.y + 10))

            # Display login or register text
            title = mode
            screen.blit(render_text(title.upper(), base_font, BROWN), (SCREEN_WIDTH // 2 - 30, 50))

            # Display response after login and register
            if response_flag:
                screen.blit(render_text(response_text, response_font, GREEN), (120, 200))
            else:
                screen.blit(render_text(response_text, response_font, RED), (120, 200))
            
            # Update screen and set frame rate
            pygame.display.update()
            clock.tick(60)


    def handle_connection_to_room(self,login_response):
        BUFF_SIZE = 1024
        SERVER_ADDR = "127.0.0.1"
        SERVER_PORT = 5500
        status =  login_response[1]
        if (status=="1"):
            print("Login: Successfully")
        else:
            print("Login:failed")
            self.login_register()
        temp =  login_response[2]
        temp =  bytes(temp, 'utf-8')
        temp =  int.from_bytes(temp)
        self.client_id =  temp
        print(self.client_id)

       # old version so here we still have to close connection and then restablish it again
        self.client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.client_socket.connect((SERVER_ADDR,SERVER_PORT))
        #  send a host room request
        choice = int(input())
        if (choice ==1):
            print("Send create room request to server...")
            request_type = "4"
            request_type_byte = request_type.encode("ascii")
            user_id_byte =  self.client_id.to_bytes(1,"big")
            message = request_type_byte + user_id_byte
            #print(message)
            self.client_socket.send(message)
        elif choice==2:
            print("Send join room request to server...")
            request_type = "5"
            request_type_byte = request_type.encode("ascii")
            user_id_byte =  self.client_id.to_bytes(1,"big")
            print("input the room number:")
            room =  int(input())
            room_byte = room.to_bytes(1, "big")
            message = request_type_byte + user_id_byte+ room_byte
            self.client_socket.send(message)
        
        # listen for the tcp port
        response =  self.client_socket.recv(BUFF_SIZE)
        status =  response[1]

        if (status ==48):
            print("Create/join room request: failed")
        elif (status ==49):
            print("Create/join room request: successfully")
            self.client_socket.close()
            self.room_id = response[2]
            #print(self.room_id)
            #print(type(self.room_id))
            t1 =  response[3]
            t2 =  response[4]
            self.tcp_port =  t1*256+t2
            print(self.tcp_port)
            #connect to game room with tcp
            time.sleep(1)
            self.tcp_room_socket =  socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.tcp_room_socket.connect((SERVER_ADDR, self.tcp_port))
        
            # send initial message type 6 to connect the room
            message_type= "6"
            message_type_byte =  message_type.encode()
            message=  message_type_byte+ user_id_byte
            self.tcp_room_socket.send(message)
            #receive the confirmation of successful connection from game room
            response =  self.tcp_room_socket.recv(BUFF_SIZE)
            #print(response)
            status = response[1]
            if status == "0":
                print("Game Room : Connection Failed")
            else:
                print("Game Room : Connected")
            #     print("Type 1 for ready, 0 to ignore")
                while True:
                    choice=  int(input())
                    if (choice ==1): #ready
                        #send ready message
                        message_type =  "7"
                        message_type_byte =  message_type.encode("ascii")
                        user_id_byte =  self.client_id.to_bytes(1,"big")
                        ready =  1
                        ready_byte =  ready.to_bytes(1,"big")
                        message =  message_type_byte + user_id_byte+ ready_byte
                        #print(len(message))
                        self.tcp_room_socket.send(message)
                        print("Ready state sent")
                        #receive the udp port
                        should_exit =0
                        while not should_exit:
                            read_sockets, _, _ = select.select([self.tcp_room_socket,0],[],[],1)
                            for sock in read_sockets:
                                if sock== self.tcp_room_socket:
                                    response =  self.tcp_room_socket.recv(BUFF_SIZE)
                                    print(response)
                                    message_type =  response[0]
                                    t1 = response[1]
                                    t2 =  response[2]
                                    self.udp_room_port =  t1*256+t2
                                    print("Game room UDP port is "+ str(self.udp_room_port))
                                    self.udp_room_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
                                    print("Game starting...")
                                    should_exit=1
                                    break
                        if should_exit:
                            self.menu()
                    elif (choice ==0): # not ready
                        message_type =  "7"
                        message_type_byte =  message_type.encode("ascii")
                        user_id_byte =  self.client_id.to_bytes(1,"big")
                        ready =  0
                        ready_byte =  ready.to_bytes(1,"big")
                        message =  message_type_byte + user_id_byte+ ready_byte
                        self.tcp_room_socket.send(message)
                    





            
        
    def test_udp(self):
        SERVER_ADDR = "127.0.0.1"
        while True:
            choice  =  int(input())
            if (choice ==0):
                break
            elif choice ==1:
                message =  "HELLO"
                

                #non-blocking send using select
                _ , writefds, _ =  select.select([],[self.udp_room_socket,0],[],1)
                for socket in writefds:
                    if socket == self.udp_room_socket:
                        self.udp_room_socket.sendto(message.encode(), (SERVER_ADDR, self.udp_room_port))
                
                readfds, _, _ =  select.select([self.udp_room_socket,0],[],[],1)
                for socket in readfds:
                    data, address = self.udp_room_socket.recvfrom(1024)
                    print(f"Received from server: {data}")
                if len(readfds)==0:
                    print("Nothing to read yet")
        

    def menu(self):
        pygame.init()
        self.screen = pygame.display.set_mode((960,720))
        # self.screen = pygame.display.set_mode((1280, 720))
        BG = pygame.image.load('data/images/menuAssets/Background.png')
        while True:
            self.screen.blit(BG, (0, 0))
            MENU_MOUSE_POS = pygame.mouse.get_pos()
            MENU_TEXT = self.get_font(75).render("MAIN MENU", True, "#b68f40")
            MENU_RECT = MENU_TEXT.get_rect(center=(480, 100))

            PLAY_BUTTON = Button(image=pygame.image.load("data/images/menuAssets/Play Rect.png"), pos=(480, 250),
                                 text_input="PLAY", font=self.get_font(50), base_color="#d7fcd4", hovering_color="White")
            QUIT_BUTTON = Button(image=pygame.image.load("data/images/menuAssets/Quit Rect.png"), pos=(480, 400),
                                 text_input="QUIT", font=self.get_font(50), base_color="#d7fcd4", hovering_color="White")
            self.screen.blit(MENU_TEXT, MENU_RECT)

            for button in [PLAY_BUTTON, QUIT_BUTTON]:
                button.changeColor(MENU_MOUSE_POS)
                button.update(self.screen)

            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    pygame.quit()
                    sys.exit()
                if event.type == pygame.MOUSEBUTTONDOWN:
                    if PLAY_BUTTON.checkForInput(MENU_MOUSE_POS):
                        self.run()
                    if QUIT_BUTTON.checkForInput(MENU_MOUSE_POS):
                        pygame.quit()
                        sys.exit()

            pygame.display.update()


    def de_serialize_entities(self, data):
        index = 0
        length =  len(data)
    #print(f"Analyzing payload of length {length}")
        while (1):

            if index >= length: 
                #print("Payload finishes reading")
                break
            else:
                id = data[index]
                index+=1
            
            if index >=length:
                print("Payload is missing entity_type")
                break
            else:
                entity_type =  data[index]
                index+=1
            
            if entity_type==0:
                if index >= length:
                    print("Payload is missing entity_class")
                    break
                else:
                    entity_class = data[index]
                    index+=1

                if index>=length-1:
                    print("Payload is missing posx")
                    break
                else:
                    posx =  data[index]*256 + data[index+1]
                    index+=2

                if index>= length-1:
                    print("Payload is missing posy")
                    break
                else:
                    posy = data[index]*256 + data[index+1]
                    index+=2
                
                if index>=length:
                    print("Payload is missing flip")
                    break
                else:
                    flip =  data[index]
                    index+=1

                if index>=length:
                    print("Payload is missing action type")
                    break
                else:
                    action_type = data[index]
                    index+=1

                if index>=length:
                    print("Payload is missing health")
                    break
                else:   
                    health =  data[index]
                    index+=1

                if index>=length:
                    print("Payload is missing stamina")
                    break
                else:   
                    stamina =  data[index]
                    index+=1
            
            found_entity = False
            for each in self.entities:
                if each.id == id and each.entity_type==entity_type:
                    found_entity=True
                    each.pos = [posx,posy]
                    if entity_type==0:
                        #each.flip = flip
                        #each.action_type =  action_type -> will implement this specifically later
                        each.health = health
                        each.stamina = stamina  
            if not found_entity:
                if entity_type==0:
                    new_player =  Player(game= self, id = id, entity_class=entity_class,posx = posx
                                         , posy=posy, health=health, stamina=stamina)
                    new_player.set_action(0, True)
                    self.entities.append(new_player)
        print(f"There are {len(self.entities)} in game")
         

    def run(self):
        SERVER_ADDR = "127.0.0.1"
        self.screen = pygame.display.set_mode((960, 720))
        self.assets = {
                'Samurai': load_image('entities/Samurai/Idle/Idle_1.png'),
                'background': load_images('background'),
                'Samurai/idle': Animation(load_images('entities/Samurai/Idle'), img_dur=5, loop = True),
                'Samurai/attack1': Animation(load_images('entities/Samurai/Attack1'), img_dur = 5, loop =False),
                'Samurai/attack2': Animation(load_images('entities/Samurai/Attack2'),img_dur = 5, loop = False),
                'Samurai/death':Animation(load_images('entities/Samurai/Death'),img_dur = 5, loop = False),
                'Samurai/run':Animation(load_images('entities/Samurai/Run'),img_dur=5, loop = True),
                'Samurai/jump':Animation(load_images('entities/Samurai/Jump'),img_dur= 5, loop=False)
        }
        self.background =  pygame.Surface(self.assets['background'][0].get_size())
        count = 0
        for each in self.assets['background']:
            count+=1
            if count>=3:
                self.background.blit(each,(0,self.background_offset_y))
            else :
                self.background.blit(each, (0,0))
        
        # self.player = Player(id=1, game  = self, pos = (0,280),size = (128,128))
        # self.player2 =  Player(id = 2, game = self, pos = (30,280),size = (128,128))
        self.entities = []
        # self.entities.append(self.player)
        # self.entities.append(self.player2)

        # message = "Start game"
        # self.client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        # self.client_socket.connect(("127.0.0.1", 5500))
        # self.client_socket.send(message.encode("utf-8"))
        bufferSize = 1024

        # create socket to send input/receive input to/from server
        input_socket = self.udp_room_socket
        self.udp_room_socket.setblocking(False)
        q = queue.Queue(maxsize= 10)

        # 
        # ---------- MULTI-THREAD HANDLING---------
        # target function for multi-thread handling
        def receive_messages(sock, queue):
            while True:
                try:
                    data, _ = sock.recvfrom(1024)
                    if (len(data)!=0):
                        queue.put(data)
                except BlockingIOError as e:
                    continue
                    
        
        # # # Start a thread to handle incoming messages
        thread = threading.Thread(target=receive_messages, args = (self.udp_room_socket, q),daemon=True)
        # #thread.daemon = True # Allow thread to exit when main program exits
        thread.start()
        self.player = None 
        self.horizontal_movement = [0,0]
        while True:
            self.display.fill(color = (0,0,0,0))
            self.display_2.blit(pygame.transform.scale(self.background, self.display.get_size()), (0,0))
            #  Receive message from server stored queue:
            #data , address =  input_socket.recvfrom()

        # ------- Read inputs from player and send client to server -----
        # MSG FORMAT: {key_up/key_down}|{button}
        #---------------------------------------------------------
            if self.player is None:
                for each in self.entities:
                    print(f"{each.id} - {self.client_id}")
                    if each.entity_type ==0 and each.id == self.client_id:
                        self.player = each
                        break
            
            for event in pygame.event.get():
                msg = self.client_id.to_bytes(1,"big")
                if event.type ==pygame.QUIT:
                    pygame.quit()
                    sys.exit()
                movement_y = 0
                action  = 0
                interaction = 0
                if event.type == pygame.KEYDOWN:
                    if event.key == pygame.K_w:
                        movement_y+=1
                    if event.key == pygame.K_s:
                        movement_y-=1

                    if event.key == pygame.K_a:
                        self.horizontal_movement[0]=1
                        # self.horizontal_movement[0]= True
                    if event.key == pygame.K_d:
                        # self.horizontal_movement[1]= True
                        self.horizontal_movement[1]=1
                    
                    if event.key == pygame.K_q:
                        # self.player.ground_attack("attack1", attack_cooldown=1)
                        action +=1
                    
                    if event.key == pygame.K_e:
                        # self.player.ground_attack("attack2", attack_cooldown=1)
                        action -=1

                    if event.key == pygame.K_SPACE: #blocking is prioritize above the other two attacks
                        action = 3
                    
                    if event.key == pygame.K_k: #dashing is even more prioritize
                        action= 4 
                    
                    if event.key == pygame.K_f: #activate trap or use item on the map
                        interaction = 1
                    if event.key == pygame.K_g: #capture or submit the flag
                        interaction = 2
                elif event.type ==  pygame.KEYUP:
                        if event.key == pygame.K_a:
                            self.horizontal_movement[0]=0
                        if event.key == pygame.K_d:
                            self.horizontal_movement[1]=0
                
                # correct the movement_x and movement_y and attack
                
                if self.horizontal_movement[0]-self.horizontal_movement[1]==-1:
                    movement_x=1
                elif self.horizontal_movement[0]-self.horizontal_movement[1]==1:
                    movement_x=2
                else:
                    movement_x=0
                if movement_y==-1:
                    movement_y = 2
                if action ==-1:
                    action=2

                msg += movement_x.to_bytes(1,"big") + movement_y.to_bytes(1,"big")+action.to_bytes(1,"big")+ interaction.to_bytes(1,"big")
                byteSent = self.udp_room_socket.sendto(msg, (SERVER_ADDR, self.udp_room_port))

                if (byteSent<=0):
                    print("Send payload: failed")
                else:
                    print(f"Send {byteSent} bytes to server")
                #print(msg)
            
        
            data =  ""
            
            try:
                data = q.get(block=False)
            except Exception as e:
                pass

            if len(data)!=0:
                print(data)
                self.de_serialize_entities(data)
        
            for each in self.entities:
                each.render(self.display)
                if each is self.player:
                    direction = self.horizontal_movement[0]-self.horizontal_movement[1]
                    if direction==0:
                        each.set_action(0,False)
                    elif direction ==1:
                        each.set_action(3,False)
                        each.flip=1
                    elif direction ==-1:
                        each.set_action(3,False)
                        each.flip=0
                    each.update()
                else:
                    each.update()
            self.display_2.blit(self.display, (0,0))
            self.screen.blit(pygame.transform.scale(self.display_2, self.screen.get_size()), dest = (0,0))
            pygame.display.update()
            self.clock.tick(60)

if __name__ == "__main__":
    Game().menu()
