import os
import sys
import math
import pygame
import socket
import threading
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
                                self.menu()
                                response_flag = True
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
                            print(response)
                            if response[1] == "1":
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



    def menu(self):
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




    def run(self):
        self.screen = pygame.display.set_mode((960, 720))
        self.assets = {
                'player': load_image('entities/Samurai/Idle/Idle_1.png'),
                'background': load_images('background'),
                'player/idle': Animation(load_images('entities/Samurai/Idle'), img_dur=5, loop = True),
                'player/attack1': Animation(load_images('entities/Samurai/Attack1'), img_dur = 5, loop =False),
                'player/attack2': Animation(load_images('entities/Samurai/Attack2'),img_dur = 5, loop = False),
                'player/death':Animation(load_images('entities/Samurai/Death'),img_dur = 5, loop = False),
                'player/run':Animation(load_images('entities/Samurai/Run'),img_dur=5, loop = True),
                'player/jump':Animation(load_images('entities/Samurai/Jump'),img_dur= 5, loop=False)
        }
        self.background =  pygame.Surface(self.assets['background'][0].get_size())
        count = 0
        for each in self.assets['background']:
            count+=1
            if count>=3:
                self.background.blit(each,(0,self.background_offset_y))
            else :
                self.background.blit(each, (0,0))
        
        self.player = Player(id=1, game  = self, pos = (0,280),size = (128,128))
        self.player2 =  Player(id = 2, game = self, pos = (30,280),size = (128,128))
        self.entities = []
        self.entities.append(self.player)
        self.entities.append(self.player2)

        message = "Start game"
        self.client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.client_socket.connect(("127.0.0.1", 5500))
        self.client_socket.send(message.encode("utf-8"))
        bufferSize = 1024

        # create socket to send input/receive input to/from server
        input_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        

        # ---------- MULTI-THREAD HANDLING---------
        # target function for multi-thread handling
        def receive_messages():
            while True:
                try:
                    data, _ = input_socket.recvfrom(bufferSize)
                    data = data.decode()
                    if data == "d|l":
                        self.horizontal_movement[0] = True
                    elif data == "d|r":
                        self.horizontal_movement[1] = True
                    elif data == "d|q":
                        self.player.ground_attack("attack1", attack_cooldown=1)
                    elif data == "d|e":
                        self.player.ground_attack("attack2", attack_cooldown=1)
                    elif data == "u|l":
                        self.horizontal_movement[0] = False
                    elif data == "u|r":
                        self.horizontal_movement[1] = False
                except OSError:
                    break # Exit loop if socket is closed
        
        # Start a thread to handle incoming messages
        thread = threading.Thread(target=receive_messages)
        thread.daemon = True # Allow thread to exit when main program exits
        thread.start()

        while True:
            self.display.fill(color = (0,0,0,0))
            self.display_2.blit(pygame.transform.scale(self.background, self.display.get_size()), (0,0))

            # ------- Send/Receive inputs from client to server -----
            # r - move left
            # l - move right
            # q - attack 1
            # e - attack 2
            # u - key up
            # d - key down
            # MSG FORMAT: {key_up/key_down}|{button}
            #---------------------------------------------------------
         
            for event in pygame.event.get():
                msg = ""
                if event.type ==pygame.QUIT:
                     pygame.quit()
                     sys.exit()
                if event.type == pygame.KEYDOWN:
                    msg = msg + "d|"
                    if event.key == pygame.K_a:
                        # self.horizontal_movement[0]= True
                        msg = msg + "l"
                    elif event.key == pygame.K_d:
                        # self.horizontal_movement[1]= True
                        msg = msg + "r"
                    elif event.key == pygame.K_q:
                        # self.player.ground_attack("attack1", attack_cooldown=1)
                        msg = msg +"q"
                    elif event.key == pygame.K_e:
                        # self.player.ground_attack("attack2", attack_cooldown=1)
                        msg = msg + "e"
                    input_socket.sendto(msg.encode(), (ADDRESS, PORT))
                    print(msg)
                if event.type == pygame.KEYUP:
                    msg = msg + "u|"
                    if event.key == pygame.K_a:
                        # self.horizontal_movement[0]= False
                        msg = msg + "l"
                    if event.key == pygame.K_d:
                        # self.horizontal_movement[1]= False
                        msg = msg + "r"
                    input_socket.sendto(msg.encode(), (ADDRESS, PORT))
                    print(msg)

            
        
            for each in self.entities:
                each.render(self.display)
                if each is self.player:
                    each.update((self.horizontal_movement[1]-self.horizontal_movement[0],0))

            self.display_2.blit(self.display, (0,0))
            self.screen.blit(pygame.transform.scale(self.display_2, self.screen.get_size()), dest = (0,0))
            pygame.display.update()
            self.clock.tick(60)

if __name__ == "__main__":
    Game().menu()
