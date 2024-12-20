import pygame
import sys
import time
from scripts.utils import *
from scripts.button import Button
from scripts.entities import Player
import config
from .network import NetworkManager

class GameManager:
    def __init__(self, network_manager: NetworkManager):
        self.network = network_manager
        self.screen = pygame.display.set_mode((config.SCREEN_WIDTH, config.SCREEN_HEIGHT))
        pygame.display.set_caption('Combat Game')
        # self.ui_manager = ui_manager

        # Load background
        self.background = pygame.image.load(config.BACKGROUND_PATH)

        # Fonts
        self.title_font = pygame.font.Font(config.FONT_PATH, 75)
        self.button_font = pygame.font.Font(config.FONT_PATH, 50)

        # Game display surfaces
        self.display = pygame.Surface((640, 480), pygame.SRCALPHA)
        self.display_2 = pygame.Surface((640, 480))
        
        # Game state
        self.horizontal_movement = [False, False]
        self.clock = pygame.time.Clock()
        
        # Load game assets
        # self.load_assets()
        
        # Players
        self.player = None
        self.player2 = None
        self.entities = []
    
    def load_assets(self):
        """Load game assets"""
        self.assets = {
            'player': load_image('entities/Samurai/Idle/Idle_1.png'),
            'background': load_images('background'),
            'player/idle': Animation(load_images('entities/Samurai/Idle'), img_dur=5, loop=True),
            'player/attack1': Animation(load_images('entities/Samurai/Attack1'), img_dur=5, loop=False),
            'player/attack2': Animation(load_images('entities/Samurai/Attack2'), img_dur=5, loop=False),
            'player/death': Animation(load_images('entities/Samurai/Death'), img_dur=5, loop=False),
            'player/run': Animation(load_images('entities/Samurai/Run'), img_dur=5, loop=True),
            'player/jump': Animation(load_images('entities/Samurai/Jump'), img_dur=5, loop=False)
        }
    
    def setup_background(self):
        """Prepare game background"""
        self.background = pygame.Surface(self.assets['background'][0].get_size())
        count = 0
        for each in self.assets['background']:
            count += 1
            y_offset = -80 if count >= 3 else 0
            self.background.blit(each, (0, y_offset))
    
    def handle_input(self, input_data):
        """Process received input data"""
        if input_data == "d|l":
            self.horizontal_movement[0] = True
        elif input_data == "d|r":
            self.horizontal_movement[1] = True
        elif input_data == "d|q":
            self.player.ground_attack("attack1", attack_cooldown=1)
        elif input_data == "d|e":
            self.player.ground_attack("attack2", attack_cooldown=1)
        elif input_data == "u|l":
            self.horizontal_movement[0] = False
        elif input_data == "u|r":
            self.horizontal_movement[1] = False

    def login_screen(self):
        """Render login/registration screen"""
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
                            response = login_register_request(username=username_text, 
                                                              password=password_text, 
                                                              mode=mode)
                            print(response)
                            if response[1] == "1": 
                                self.main_menu()
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
                            response = login_register_request(username=username_text, 
                                                              password=password_text, 
                                                              mode=mode)
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
            clock.tick(config.FPS)

    def main_menu(self):
        """Main menu screen"""
        self.screen = pygame.display.set_mode((config.SCREEN_WIDTH, config.SCREEN_HEIGHT))
        while True:
            self.screen.blit(self.background, (0, 0))
            mouse_pos = pygame.mouse.get_pos()
            
            # Menu Title
            menu_text = self.title_font.render("MAIN MENU", True, config.COLORS['MENU_TEXT'])
            menu_rect = menu_text.get_rect(center=(config.SCREEN_WIDTH//2, 100))
            
            # Buttons
            play_button = Button(
                image=pygame.image.load("data/images/menuAssets/Play Rect.png"), 
                pos=(config.SCREEN_WIDTH//2, 250),
                text_input="PLAY", 
                font=self.button_font, 
                base_color=config.COLORS['BUTTON_BASE'], 
                hovering_color="White"
            )
            
            quit_button = Button(
                image=pygame.image.load("data/images/menuAssets/Quit Rect.png"), 
                pos=(config.SCREEN_WIDTH//2, 400),
                text_input="QUIT", 
                font=self.button_font, 
                base_color=config.COLORS['BUTTON_BASE'], 
                hovering_color="White"
            )
            
            self.screen.blit(menu_text, menu_rect)
            
            for button in [play_button, quit_button]:
                button.changeColor(mouse_pos)
                button.update(self.screen)
            
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    pygame.quit()
                    sys.exit()
                
                if event.type == pygame.MOUSEBUTTONDOWN:
                    if play_button.checkForInput(mouse_pos):
                        self.game_mode_screen()
                    
                    if quit_button.checkForInput(mouse_pos):
                        pygame.quit()
                        sys.exit()
            
            pygame.display.update()

    def game_mode_screen(self):
        """Game mode selection screen"""
        while True:
            self.screen.blit(self.background, (0, 0))
            mouse_pos = pygame.mouse.get_pos()
            
            # Title
            title_text = self.title_font.render("GAME MODE", True, config.COLORS['MENU_TEXT'])
            title_rect = title_text.get_rect(center=(config.SCREEN_WIDTH//2, 100))
            
            # Buttons
            join_button = Button(
                image=pygame.image.load("data/images/menuAssets/Play Rect.png"), 
                pos=(config.SCREEN_WIDTH//2, 250),
                text_input="JOIN ROOM", 
                font=get_font(30), 
                base_color=config.COLORS['BUTTON_BASE'], 
                hovering_color="White"
            )
            
            host_button = Button(
                image=pygame.image.load("data/images/menuAssets/Play Rect.png"), 
                pos=(config.SCREEN_WIDTH//2, 400),
                text_input="HOST ROOM", 
                font=get_font(30), 
                base_color=config.COLORS['BUTTON_BASE'], 
                hovering_color="White"
            )
            
            back_button = Button(
                image=pygame.image.load("data/images/menuAssets/Quit Rect.png"), 
                pos=(config.SCREEN_WIDTH//2, 550),
                text_input="BACK", 
                font=get_font(30), 
                base_color=config.COLORS['BUTTON_BASE'], 
                hovering_color="White"
            )
            
            self.screen.blit(title_text, title_rect)
            
            for button in [join_button, host_button, back_button]:
                button.changeColor(mouse_pos)
                button.update(self.screen)
            
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    pygame.quit()
                    sys.exit()
                
                if event.type == pygame.MOUSEBUTTONDOWN:
                    if join_button.checkForInput(mouse_pos):
                        # Implement join room logic
                        self.list_of_room_screen()
                        return
                    
                    if host_button.checkForInput(mouse_pos):
                        # Implement host room logic
                        self.host_room_screen(1)
                        return
                    
                    if back_button.checkForInput(mouse_pos):
                        self.main_menu()
            
            pygame.display.update()
    
    def host_room_screen(self, user_id):
        """Waiting room screen for host"""
        # Send the request to the server to get the room information
        status, room_id, tcp_port = host_room_request(user_id=user_id)

        # Connect to the room TCP network
        host_room_tcp_socket = NetworkManager(
            server_addr=config.SERVER_ADDR,
            server_port=tcp_port
        )
        time.sleep(1)
        host_room_tcp_socket.connect()
        
        # Join the room (send message type 6)
        response = join_room_request(user_id=user_id, 
                                     host_room_socket=host_room_tcp_socket)
        print("Join room response: " + response)

        print(1)
        if status == '1':
            while True:
                self.screen.blit(self.background, (0, 0))
                mouse_pos = pygame.mouse.get_pos()
                
                # Title
                text = f"ROOM {room_id}"
                title_text = self.title_font.render(text, True, config.COLORS['MENU_TEXT'])
                title_rect = title_text.get_rect(center=(config.SCREEN_WIDTH//2, 100))
                
                # Buttons
                start_button = Button(
                    image=pygame.image.load("data/images/menuAssets/Options Rect.png"), 
                    pos=(config.SCREEN_WIDTH//2, 500),
                    text_input="START_GAME", 
                    font=get_font(30), 
                    base_color=config.COLORS['BUTTON_BASE'], 
                    hovering_color="White"
                )            
                
                self.screen.blit(title_text, title_rect)
                
                for button in [start_button]:
                    button.changeColor(mouse_pos)
                    button.update(self.screen)
                
                for event in pygame.event.get():
                    if event.type == pygame.QUIT:
                        host_room_tcp_socket.close()
                        pygame.quit()
                        sys.exit()
                    
                    if event.type == pygame.MOUSEBUTTONDOWN:                  
                        if start_button.checkForInput(mouse_pos):
                            self.run()
                pygame.display.update()

    def list_of_room_screen(self):
        """Display the room selection screen with pagination and a refresh option."""

        def fetch_room_list():
            """Fetch the list of available rooms from the server."""
            # Connect to the server to request room information
            room_network = NetworkManager(
                server_addr=config.SERVER_ADDR,
                server_port=config.SERVER_PORT
            )
            room_network.connect()

            # Request room data
            message = "3"
            room_network.send_tcp_message(message.encode())  # Send request code for room data
            response = room_network.receive_tcp_message()  # Receive room data
            room_network.close()
            print(response)
            return parse_room_data(response)


        def parse_room_data(response):
            """Parse serialized room data from the server."""
            room_list = []
            total_rooms = int(response[1])-48  # First byte indicates total rooms
            print(total_rooms)
            index = 2  # Start reading after the first byte
            for _ in range(total_rooms):
                room_id = int(response[index])-48
                total_players = int(response[index+1])-48
                room_list.append({"room_id": room_id, "total_players": total_players})
                index += 2

            return room_list


        # Fetch initial room list
        room_list = fetch_room_list()

        # Pagination variables
        rooms_per_page = 4
        current_page = 0

        while True:
            self.screen.blit(self.background, (0, 0))
            mouse_pos = pygame.mouse.get_pos()

            # Title
            title_text = self.title_font.render("SELECT ROOM", True, config.COLORS['MENU_TEXT'])
            title_rect = title_text.get_rect(center=(config.SCREEN_WIDTH // 2, 100))
            self.screen.blit(title_text, title_rect)

            # Pagination Buttons
            left_button = Button(
                image=pygame.image.load("data/images/menuAssets/Buttons Rect.png"),
                pos=(config.SCREEN_WIDTH // 10, config.SCREEN_HEIGHT // 2),
                text_input="<-",
                font=get_font(30),
                base_color=config.COLORS['BUTTON_BASE'],
                hovering_color="White"
            )
            right_button = Button(
                image=pygame.image.load("data/images/menuAssets/Buttons Rect.png"),
                pos=(config.SCREEN_WIDTH - config.SCREEN_WIDTH // 10, config.SCREEN_HEIGHT // 2),
                text_input="->",
                font=get_font(30),
                base_color=config.COLORS['BUTTON_BASE'],
                hovering_color="White"
            )

            refresh_button = Button(
                image=pygame.image.load("data/images/menuAssets/Refresh Rect.png"),
                pos=(config.SCREEN_WIDTH-config.SCREEN_WIDTH // 5, config.SCREEN_HEIGHT - 100),
                text_input="REFRESH",
                font=get_font(20),
                base_color=config.COLORS['BUTTON_BASE'],
                hovering_color="White"
            )

            back_button = Button(
                image=pygame.image.load("data/images/menuAssets/Refresh Rect.png"),
                pos=(config.SCREEN_WIDTH // 5, config.SCREEN_HEIGHT - 100),
                text_input="BACK",
                font=get_font(20),
                base_color=config.COLORS['BUTTON_BASE'],
                hovering_color="White"
            )

            # Draw all control buttons
            control_buttons = [left_button, right_button, back_button, refresh_button]
            for button in control_buttons:
                button.changeColor(mouse_pos)
                button.update(self.screen)

            # Calculate rooms to display on the current page
            start_index = current_page * rooms_per_page
            end_index = start_index + rooms_per_page
            current_rooms = room_list[start_index:end_index]

            # Room Buttons
            buttons = []
            y_offset = 200
            for room in current_rooms:
                button_text = f"Room {room['room_id']} ({room['total_players']} players)"
                button = Button(
                    image=pygame.image.load("data/images/menuAssets/Options Rect.png"),
                    pos=(config.SCREEN_WIDTH // 2, y_offset),
                    text_input=button_text,
                    font=get_font(30),
                    base_color=config.COLORS['BUTTON_BASE'],
                    hovering_color="White"
                )
                buttons.append((button, room['room_id']))
                y_offset += 100  # Adjust button spacing

            # Draw room buttons
            for button, _ in buttons:
                button.changeColor(mouse_pos)
                button.update(self.screen)

            # Event handling
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    pygame.quit()
                    sys.exit()

                if event.type == pygame.MOUSEBUTTONDOWN:
                    # Pagination button logic
                    if left_button.checkForInput(mouse_pos) and current_page > 0:
                        current_page -= 1
                    if right_button.checkForInput(mouse_pos) and end_index < len(room_list):
                        current_page += 1

                    # Refresh button logic
                    if refresh_button.checkForInput(mouse_pos):
                        room_list = fetch_room_list()  # Refresh room data
                        current_page = 0  # Reset to the first page
                    
                    if back_button.checkForInput(mouse_pos):
                        return # Return to the menu

                    # Room button logic
                    for button, room_id in buttons:
                        if button.checkForInput(mouse_pos):
                            print(f"Connect to the game room ID: {room_id}")
                            return room_id  # Return selected room ID

            pygame.display.update()




    def run(self):
        """Main game loop"""
        # Setup game
        self.load_assets()
        self.setup_background()
        
        # Create players
        self.player = Player(id=1, game=self, pos=(0, 280), size=(128, 128))
        self.player2 = Player(id=2, game=self, pos=(30, 280), size=(128, 128))
        self.entities = [self.player, self.player2]
        
        # Start network input listener
        self.network.start_input_listener(self.handle_input)
        
        while True:
            self.display.fill(color=(0, 0, 0, 0))
            self.display_2.blit(pygame.transform.scale(self.background, self.display.get_size()), (0, 0))
            
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    pygame.quit()
                    sys.exit()
                
                if event.type == pygame.KEYDOWN:
                    msg = "d|"
                    if event.key == pygame.K_a:
                        msg += "l"
                    elif event.key == pygame.K_d:
                        msg += "r"
                    elif event.key == pygame.K_q:
                        msg += "q"
                    elif event.key == pygame.K_e:
                        msg += "e"
                    self.network.send_udp_input(msg, (config.SERVER_ADDR, config.INPUT_PORT))
                
                if event.type == pygame.KEYUP:
                    msg = "u|"
                    if event.key == pygame.K_a:
                        msg += "l"
                    if event.key == pygame.K_d:
                        msg += "r"
                    self.network.send_udp_input(msg, (config.SERVER_ADDR, config.INPUT_PORT))
            
            for entity in self.entities:
                entity.render(self.display)
                if entity is self.player:
                    entity.update((self.horizontal_movement[1] - self.horizontal_movement[0], 0))
            
            self.display_2.blit(self.display, (0, 0))
            self.screen.blit(pygame.transform.scale(self.display_2, self.screen.get_size()), dest=(0, 0))
            
            pygame.display.update()
            self.clock.tick(config.FPS)