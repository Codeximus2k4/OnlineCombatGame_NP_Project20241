import sys
import pygame
import socket
import threading
import time
import queue
import config
from client.network import NetworkManager
from pygame import SCRAP_TEXT
from scripts.utils import *
from scripts.entities import PhysicsEntity, Player, Base, Flag
from scripts.button import Button
from scripts.tilemap  import Tilemap

class GameManager:
    def __init__(self,network_manager: NetworkManager):
        self.network = network_manager
        self.screen = pygame.display.set_mode((config.SCREEN_WIDTH, config.SCREEN_HEIGHT))
        pygame.display.set_caption('combat game')
        
        self.menu_background = pygame.image.load(config.BACKGROUND_PATH)

        self.title_font =  pygame.font.Font(config.FONT_PATH,75)
        self.button_font = pygame.font.Font(config.FONT_PATH, 50)

        self.display =  pygame.Surface((640,480), pygame.SRCALPHA)
        self.display_2 =  pygame.Surface((640,480))

        self.horizontal_movement = [False,False]
        self.clock = pygame.time.Clock()

        self.assets = {}
        self.load_assets()
        self.background_offset_y = -80

        self.player = None
        self.entities = []

        self.user_id = None
        self.udp_room_socket =None
        self.udp_room_port =None

        self.id_to_username = {}

    def load_assets(self):
        """Load game assets"""
        self.assets = {
                'decor': load_images('tiles/decor'),
                'grass': load_images('tiles/grass'),
                'large_decor': load_images('tiles/large_decor'),
                'stone':  load_images('tiles/stone'),
                'Samurai': load_image('entities/Samurai/Idle/idle1.png'),
                'background': load_images('background'),
                'Samurai/idle': Animation(load_images('entities/Samurai/Idle'), img_dur=5, loop = True),
                'Samurai/attack1': Animation(load_images('entities/Samurai/Attack1'), img_dur = 5, loop =False),
                'Samurai/attack2': Animation(load_images('entities/Samurai/Attack2'),img_dur = 5, loop = False),
                'Samurai/death':Animation(load_images('entities/Samurai/Death'),img_dur = 5, loop = False),
                'Samurai/run':Animation(load_images('entities/Samurai/Run'),img_dur=5, loop = True),
                'Samurai/jump':Animation(load_images('entities/Samurai/Jump'),img_dur= 5, loop=False),
                'Samurai/fall':Animation(load_images('entities/Samurai/Fall'),img_dur= 5, loop=False),
                'Samurai/take hit':Animation(load_images('entities/Samurai/Take Hit'),img_dur= 5, loop=False),
                'base/team1': Animation(load_images('team_home/teama'), img_dur=5,loop= True),
                'base/team2': Animation(load_images('team_home/teamb'), img_dur=5,loop= True),
                'flag/flag1': Animation(load_images('flags/flagA'), img_dur=5,loop= True),
                'flag/flag2': Animation(load_images('flags/flagB'), img_dur=5,loop= True),
        }
    
    def setup_background(self):
        """Prepare game background"""
        self.game_background =  pygame.Surface(self.assets['background'][0].get_size())
        count = 0
        for each in self.assets['background']:
            count+=1
            y_offset = -80 if count >=3 else 0
            self.game_background.blit(each, (0,y_offset))
    
    
    def login_screen(self):
        """Render login/registration screen"""
        # Constants
        SCREEN_WIDTH, SCREEN_HEIGHT = 400, 300
        COLOR_ACTIVE = pygame.Color('lightskyblue3')
        COLOR_PASSIVE = pygame.Color('white')
        DISPLAY_LIMIT = 10  # Display only the last 10 characters

        # Initialize Pygame and Font
        pygame.init()
        clock = pygame.time.Clock()
        screen = pygame.display.set_mode([SCREEN_WIDTH, SCREEN_HEIGHT])
        base_font = pygame.font.Font(config.FONT_PATH, 15)
        base_bg = pygame.image.load(config.BACKGROUND_PATH)
        response_font = pygame.font.Font(config.FONT_PATH, 12)

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
                            if response[1] == "1": 
                                self.user_id = ord(response[2])
                                print(f"Login successfully with user_id: {self.user_id}")
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
                            if response[1] == "1":
                                print("Register successfully")
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
            username_surface = base_font.render(username_display_text, True, config.COLORS["BLACK"])
            password_surface = base_font.render(password_display_text, True, config.COLORS["BLACK"])
            screen.blit(username_surface, (username_rect.x + 5, username_rect.y + 10))
            screen.blit(password_surface, (password_rect.x + 5, password_rect.y + 10))

            # Draw the blinking cursor in active input box
            if cursor_visible:
                if active_input == "username":
                    cursor_x = username_rect.x + 5 + username_surface.get_width() + 2
                    pygame.draw.line(screen, config.COLORS["WHITE"], (cursor_x, username_rect.y + 5), (cursor_x, username_rect.y + 27), 2)
                elif active_input == "password":
                    cursor_x = password_rect.x + 5 + password_surface.get_width() + 2
                    pygame.draw.line(screen, config.COLORS["WHITE"], (cursor_x, password_rect.y + 5), (cursor_x, password_rect.y + 27), 2)

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
            login_label = render_text("Login", base_font, config.COLORS["WHITE"])
            register_label = render_text("Register", base_font, config.COLORS["WHITE"])
            screen.blit(login_label, (login_button_rect.x + 30, login_button_rect.y + 10))
            screen.blit(register_label, (register_button_rect.x + 10, register_button_rect.y + 10))

            # Display login or register text
            title = mode
            screen.blit(render_text(title.upper(), base_font, config.COLORS["BROWN"]), (SCREEN_WIDTH // 2 - 30, 50))

            # Display response after login and register
            if response_flag:
                screen.blit(render_text(response_text, response_font, config.COLORS["GREEN"]), (120, 200))
            else:
                screen.blit(render_text(response_text, response_font, config.COLORS["RED"]), (120, 200))
            
            # Update screen and set frame rate
            pygame.display.update()
            clock.tick(config.FPS)

    def main_menu(self):
        """Main menu screen"""
        self.screen = pygame.display.set_mode((config.SCREEN_WIDTH, config.SCREEN_HEIGHT))
        while True:
            self.screen.blit(self.menu_background, (0, 0))
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

            rank_button = Button(
                image=pygame.image.load("data/images/menuAssets/Play Rect.png"), 
                pos=(config.SCREEN_WIDTH//2, 400),
                text_input="RANK", 
                font=self.button_font, 
                base_color=config.COLORS['BUTTON_BASE'], 
                hovering_color="White"
            )
            
            quit_button = Button(
                image=pygame.image.load("data/images/menuAssets/Quit Rect.png"), 
                pos=(config.SCREEN_WIDTH//2, 550),
                text_input="QUIT", 
                font=self.button_font, 
                base_color=config.COLORS['BUTTON_BASE'], 
                hovering_color="White"
            )
            
            self.screen.blit(menu_text, menu_rect)
            
            for button in [play_button, rank_button, quit_button]:
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

                    if rank_button.checkForInput(mouse_pos):
                        self.rank_screen()
            
            pygame.display.update()

    def game_mode_screen(self):
        """Game mode selection screen"""
        while True:
            self.screen.blit(self.menu_background, (0, 0))
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
                        # Send the request to the server to get the room information
                        status, room_id, tcp_port = host_room_request(user_id=self.user_id)
                        if status=="1":
                            self.waiting_room_screen(user_id=self.user_id,
                                                        room_id=room_id,
                                                        room_tcp_port=tcp_port)
                        return
                    
                    if back_button.checkForInput(mouse_pos):
                        self.main_menu()
            
            pygame.display.update()
    
    def rank_screen(self):
        """Top 5 player ranking screen with Refresh and My Stats buttons."""

        def fetch_top5():
            """Fetch the top 5 player rankings."""
            # return []
            return rank_request()  # Replace with the actual implementation

        def fetch_my_stats(user_id):
            """Fetch the stats for the current user."""
            # return None
            return my_stats_request(self.user_id)  # Replace with actual implementation

        # Fetch the initial top 5 rankings
        top5 = fetch_top5()

        my_stats = None  # Placeholder for the user's stats

        title_text = self.title_font.render("TOP 5 PLAYERS", True, config.COLORS['MENU_TEXT'])
        title_rect = title_text.get_rect(center=(config.SCREEN_WIDTH // 2, 50))

        while True:
            self.screen.blit(self.menu_background, (0, 0))
            mouse_pos = pygame.mouse.get_pos()

            # Title
            self.screen.blit(title_text, title_rect)

            # Header Row
            header_font = get_font(25)
            headers = ["Username", "Games Played", "Score"]
            header_positions = [config.SCREEN_WIDTH // 5, config.SCREEN_WIDTH // 2, 4 * config.SCREEN_WIDTH // 5]

            for i, header in enumerate(headers):
                header_text = header_font.render(header, True, config.COLORS['MENU_TEXT'])
                header_rect = header_text.get_rect(center=(header_positions[i], 150))
                self.screen.blit(header_text, header_rect)

            # Player Stats Rows
            y_offset = 200
            row_font = get_font(20)

            for player in top5:
                row_data = [
                    player["username"],
                    str(player["num_game"]),
                    str(player["score"]),
                ]

                for i, data in enumerate(row_data):
                    data_text = row_font.render(data, True, config.COLORS['MENU_TEXT'])
                    data_rect = data_text.get_rect(center=(header_positions[i], y_offset))
                    self.screen.blit(data_text, data_rect)

                y_offset += 50  # Adjust row spacing

            # Display My Stats (if fetched)
            if my_stats:
                y_offset = 200  # Add some spacing
                title_text = self.title_font.render("MY STATS", True, config.COLORS['MENU_TEXT'])
                title_rect = title_text.get_rect(center=(config.SCREEN_WIDTH // 2, 50))
                y_offset += 50

                row_data = [
                    my_stats["username"],
                    str(my_stats["num_game"]),
                    str(my_stats["score"]),
                ]
                for i, data in enumerate(row_data):
                    data_text = row_font.render(data, True, config.COLORS['MENU_TEXT'])
                    data_rect = data_text.get_rect(center=(header_positions[i], y_offset))
                    self.screen.blit(data_text, data_rect)

            # Buttons
            back_button = Button(
                image=pygame.image.load("data/images/menuAssets/Refresh Rect.png"),
                pos=(config.SCREEN_WIDTH // 5, config.SCREEN_HEIGHT - 100),
                text_input="BACK",
                font=get_font(20),
                base_color=config.COLORS['BUTTON_BASE'],
                hovering_color="White"
            )
            refresh_button = Button(
                image=pygame.image.load("data/images/menuAssets/Refresh Rect.png"),
                pos=(config.SCREEN_WIDTH // 2, config.SCREEN_HEIGHT - 100),
                text_input="REFRESH",
                font=get_font(20),
                base_color=config.COLORS['GREEN'],
                hovering_color="White"
            )
            my_stats_button = Button(
                image=pygame.image.load("data/images/menuAssets/Refresh Rect.png"),
                pos=(4 * config.SCREEN_WIDTH // 5, config.SCREEN_HEIGHT - 100),
                text_input="MY STATS",
                font=get_font(20),
                base_color=config.COLORS['BLUE'],
                hovering_color="White"
            )

            for button in [back_button, refresh_button, my_stats_button]:
                button.changeColor(mouse_pos)
                button.update(self.screen)

            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    pygame.quit()
                    sys.exit()

                if event.type == pygame.MOUSEBUTTONDOWN:
                    if back_button.checkForInput(mouse_pos):
                        return  # Go back to the previous screen
                    if refresh_button.checkForInput(mouse_pos):
                        my_stats = None
                        title_text = self.title_font.render("TOP 5 PLAYERS", True, config.COLORS['MENU_TEXT'])
                        title_rect = title_text.get_rect(center=(config.SCREEN_WIDTH // 2, 50))
                        top5 = fetch_top5()  # Refresh the top 5 data
                    if my_stats_button.checkForInput(mouse_pos):
                        top5 = []
                        my_stats = fetch_my_stats(self.user_id)  # Fetch the current user's stats

            pygame.display.update()

    def waiting_room_screen(self, user_id, room_id, room_tcp_port):
        """Waiting room screen for host"""

        # Connect to the room TCP network
        room_tcp_socket = NetworkManager(
            server_addr=config.SERVER_ADDR,
            server_port=room_tcp_port
        )
        time.sleep(1)
        room_tcp_socket.connect()

        # Join the room (send message type 6)
        response = connect_room_request(user_id=user_id, 
                                        host_room_socket=room_tcp_socket)
        print("Create/Join room response: " + response)

        players = []  # Shared variable to store the current player list
        stop_thread = threading.Event()  # Event to signal the update thread to stop
        is_ready = 0 # Store current ready state of the player
        udp_room_port: int = None # udp room port for game play
        

        def waiting_room_process():
            """Fetch the list of players in the room/Receive the tcp port"""
            
            new_players = [] # New players list
            try:
                room_tcp_socket.tcp_socket.setblocking(False) # set non blocking for room socket
                message_type = room_tcp_socket.receive_tcp_message(buff_size=1).decode() # Receive the message type
                print(f"Message type: {message_type}")
                if message_type == "7":
                    nonlocal udp_room_port
                    udp_room_port = room_tcp_socket.receive_tcp_message(buff_size=2).decode()
                    print(udp_room_port)
                    udp_room_port = struct.unpack("!H", udp_room_port.encode("utf-8"))[0]
                    print(f"Room UDP port: {udp_room_port}")
                elif message_type == "8":
                    num_players = room_tcp_socket.receive_tcp_message(buff_size=1).decode()
                    num_players = ord(num_players)
                    while num_players:
                        # Get the username length of the player
                        username_length = room_tcp_socket.receive_tcp_message(buff_size=1).decode() 
                        username_length = ord(username_length)

                        # Get the username of the player
                        username = room_tcp_socket.receive_tcp_message(buff_size=username_length).decode()

                        # Get the player id of the player
                        player_id = room_tcp_socket.receive_tcp_message(buff_size=1)
                        player_id = ord(player_id)

                        # Get the ready status of the player
                        ready_status = room_tcp_socket.receive_tcp_message(buff_size=1).decode()
                        ready_status = ord(ready_status)

                        # get the list of players
                        new_players.append({"player_id": player_id,"username": username, "ready": ready_status})
                        num_players = num_players - 1
                return new_players
            except Exception as e:
                print(f"Error when receiving message type 7/8: {e}")
            finally:
                room_tcp_socket.tcp_socket.setblocking(True) # Set blocking for room socket
                return new_players
            
        def update_players():
            """Background thread to update the player list"""
            while not stop_thread.is_set():
                new_players = waiting_room_process()
                if new_players:
                    nonlocal players
                    players = new_players

        # Start the update thread
        update_thread = threading.Thread(target=update_players,
                                         daemon=True)
        update_thread.start()

        # Buttons
        ready = False
        ready_button = Button(
            image=pygame.image.load("data/images/menuAssets/Refresh Rect.png"), 
            pos=(config.SCREEN_WIDTH - config.SCREEN_WIDTH // 5, config.SCREEN_HEIGHT - 100),
            text_input="READY", 
            font=get_font(20), 
            base_color=config.COLORS['GREEN'], 
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

        try:
            while True:
                self.screen.blit(self.menu_background, (0, 0))
                mouse_pos = pygame.mouse.get_pos()

                # Title
                text = f"ROOM {room_id}"
                title_text = self.title_font.render(text, True, config.COLORS['MENU_TEXT'])
                title_rect = title_text.get_rect(center=(config.SCREEN_WIDTH // 2, 100))
                self.screen.blit(title_text, title_rect)

                # Display player list
                y_offset = 200
                for player in players:
                    player_text = f"{player['username']} - {'Ready' if player['ready'] else 'Not Ready'}"
                    player_text_rendered = get_font(25).render(player_text, True, config.COLORS['MENU_TEXT'])
                    player_rect = player_text_rendered.get_rect(center=(config.SCREEN_WIDTH // 2, y_offset))
                    self.screen.blit(player_text_rendered, player_rect)
                    y_offset += 50  # Adjust spacing between players

                # Buttons
                for button in [ready_button, back_button]:
                    button.changeColor(mouse_pos)
                    button.update(self.screen)

                for event in pygame.event.get():
                    if event.type == pygame.QUIT:
                        stop_thread.set()  # Signal the update thread to stop
                        room_tcp_socket.close()
                        pygame.quit()
                        sys.exit()

                    if event.type == pygame.MOUSEBUTTONDOWN:
                        if ready_button.checkForInput(mouse_pos):
                            if is_ready==0:
                                is_ready = 1
                                ready_button.text_input = "CANCEL"
                                ready_button.base_color = config.COLORS["RED"]
                                ready_button.text = ready_button.font.render(ready_button.text_input, True, ready_button.base_color)
                                update_ready_request(user_id=self.user_id,
                                                        is_ready=is_ready,
                                                        update_ready_socket=room_tcp_socket)
                            else:
                                ready_button.text_input = "READY"
                                ready_button.base_color = config.COLORS["GREEN"]
                                ready_button.text = ready_button.font.render(ready_button.text_input, True, ready_button.base_color)
                                is_ready = 0
                                update_ready_request(user_id=self.user_id,
                                                        is_ready=is_ready,
                                                        update_ready_socket=room_tcp_socket)

                        if back_button.checkForInput(mouse_pos):
                            stop_thread.set()  # Signal the update thread to stop
                            return
                        
                pygame.display.update()
                if udp_room_port:
                    break
        finally:
            stop_thread.set() # Ensure the thread stops when leaving the function
            update_thread.join() # Wait for the thread to finish
            room_tcp_socket.close()
            if udp_room_port:
                for player in players:
                    self.id_to_username[player["player_id"]] = player["username"]
                self.run(room_udp_port=udp_room_port)


    def list_of_room_screen(self):
        """Display the room selection screen with pagination and a refresh option."""

        # Fetch initial room list
        room_list = fetch_room_list_request()

        # Pagination variables
        rooms_per_page = 4
        current_page = 0

        while True:
            self.screen.blit(self.menu_background, (0, 0))
            mouse_pos = pygame.mouse.get_pos()

            # Title
            title_text = self.title_font.render("SELECT ROOM", True, config.COLORS['MENU_TEXT'])
            title_rect = title_text.get_rect(center=(config.SCREEN_WIDTH // 2, 100))
            self.screen.blit(title_text, title_rect)

            # Pagination Buttons
            left_button = Button(
                image=pygame.image.load("data/images/menuAssets/Buttons Rect.png"),
                pos=(config.SCREEN_WIDTH // 10, config.SCREEN_HEIGHT // 2),
                text_input="<",
                font=get_font(30),
                base_color=config.COLORS['BUTTON_BASE'],
                hovering_color="White"
            )
            right_button = Button(
                image=pygame.image.load("data/images/menuAssets/Buttons Rect.png"),
                pos=(config.SCREEN_WIDTH - config.SCREEN_WIDTH // 10, config.SCREEN_HEIGHT // 2),
                text_input=">",
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
                        room_list = fetch_room_list_request()  # Refresh room data
                        current_page = 0  # Reset to the first page
                    
                    if back_button.checkForInput(mouse_pos):
                        return # Return to the menu

                    # Room button logic
                    for button, room_id in buttons:
                        if button.checkForInput(mouse_pos):
                            print(f"Connect to the game room ID: {room_id}")
                            room_tcp_port = join_room_request(user_id=self.user_id,
                                                              room_id=room_id)
                            print(f"Room TCP port: {room_tcp_port}")
                            if room_tcp_port >= 10000:
                                self.waiting_room_screen(user_id=self.user_id,
                                                         room_id=room_id,
                                                         room_tcp_port=room_tcp_port)
                            

            pygame.display.update()
    
    def pick_hero_screen(self):
        hero_picked = " " 
        while True:
            self.screen.blit(self.menu_background, (0, 0))
            mouse_pos = pygame.mouse.get_pos()

            # Title
            title_text = self.title_font.render("PICK A HERO", True, config.COLORS['MENU_TEXT'])
            title_rect = title_text.get_rect(center=(config.SCREEN_WIDTH // 2, 100))
            self.screen.blit(title_text, title_rect)
            
            #Button
            lock_button = Button(
                image=pygame.image.load("data/images/menuAssets/Refresh Rect.png"),
                pos=(config.SCREEN_WIDTH-config.SCREEN_WIDTH // 5, config.SCREEN_HEIGHT - 100),
                text_input="Lock",
                font=get_font(20),
                base_color=config.COLORS['BUTTON_BASE'],
                hovering_color="White"
            )
            
            samurai_button = Button(
                image=pygame.image.load("data/images/pickHero/Samurai.png"),
                pos=(390, 270),
                text_input="Samurai",
                font=get_font(16),
                base_color=config.COLORS['BUTTON_BASE'],
                hovering_color="White"
            )
            
            wizard_button = Button(
                image=pygame.image.load("data/images/pickHero/Wizard.png"),
                pos=(390, 450),
                text_input="Wizard",
                font=get_font(16),
                base_color=config.COLORS['BUTTON_BASE'],
                hovering_color="White"
            )
            
            king_button = Button(
                image=pygame.image.load("data/images/pickHero/King.png"),
                pos=(570, 270),
                text_input="King",
                font=get_font(16),
                base_color=config.COLORS['BUTTON_BASE'],
                hovering_color="White"
            )
            
            witch_button = Button(
                image=pygame.image.load("data/images/pickHero/Witch.png"),
                pos=(570, 450),
                text_input="Witch",
                font=get_font(16),
                base_color=config.COLORS['BUTTON_BASE'],
                hovering_color="White"
            )
            
            # Draw all buttons
            buttons = [lock_button, samurai_button, wizard_button, king_button, witch_button]
            for button in buttons:
                button.changeColor(mouse_pos)
                button.update(self.screen)
                
            # Event handling
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    pygame.quit()
                    sys.exit()
                    
                if event.type == pygame.MOUSEBUTTONDOWN:
                    if lock_button.checkForInput(mouse_pos):
                        break
                    if samurai_button.checkForInput(mouse_pos):
                        hero_picked = "Samurai"
                    if wizard_button.checkForInput(mouse_pos):
                        hero_picked = "Wizard"
                    if king_button.checkForInput(mouse_pos):
                        hero_picked = "King"
                    if witch_button.checkForInput(mouse_pos):
                        hero_picked = "Witch"
                        
            pick_text = f"You pick: {hero_picked}"
            pick_text_rendered = get_font(22).render(pick_text, True, config.COLORS['MENU_TEXT'])
            player_rect = pick_text_rendered.get_rect(center=(config.SCREEN_WIDTH // 5, config.SCREEN_HEIGHT - 100))
            self.screen.blit(pick_text_rendered, player_rect)
            
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

                team=0
                if self.game_mode==1:
                    if index>= length-1:
                        print("Payload is missing score")
                        break
                    else:
                        score = data[index]*256 + data[index+1]
                        index+=2
                elif self.game_mode==2:
                    if index>=length:
                        print("Payload is missing team")
                        break
                    else:   
                        team =  data[index]
                        index+=1
                    
                    if index>=length:
                        print("Payload is missing score")
                        break
                    else:   
                        score =  data[index]
                        index+=1
            elif entity_type==3:
                if index >=length:
                    print("Payload is missing entity_class")
                    break
                else:
                    index+=1

                if index>= length-1:
                    print("Payload is missing posx flag")
                    break
                else:
                    posx = data[index]*256 + data[index+1]
                    index+=2
                
                if index>= length-1:
                    print("Payload is missing posy flag")
                    break
                else:
                    posy = data[index]*256 + data[index+1]
                    index+=2

            found_entity = False
            for each in self.entities:
                if each.id == id and each.entity_type==entity_type:
                    found_entity=True
                    # update the position
                    each.pos[0] = posx
                    each.pos[1] = posy
                    if entity_type==0:
                        each.flip = flip
                        each.set_action(action_type,False)
                        each.health = health
                        each.stamina = stamina  
                        each.score=score
                        each.team =team
                    if entity_type==3:
                        pass # no implementation yet
            if not found_entity:
                if entity_type==0:
                    new_player =  Player(game = self, id = id, entity_class=entity_class,posx = posx
                                         , posy=posy, health=health, stamina=stamina, team =team)
                    new_player.set_action(0, True)
                    self.entities.append(new_player)
                if entity_type == 3:
                    new_flag =  Flag(team_id =  id, pos =  [posx,posy], game=self)
                    self.entities.append(new_flag)
            #print(f"There are {len(self.entities)} in game")
         
    def load_level(self, map_id):
        self.tilemap.load('data/maps/' +str(map_id)+'.json')
        self.scroll= [0,0]
    
    def run(self,room_udp_port):
        """Main game loop"""
        self.game_mode = 1 # the game mode is capture the flag
        if (self.game_mode ==1):
            self.base_sign = []

        self.display =  pygame.Surface((960,720), pygame.SRCALPHA)
        self.display_2 =  pygame.Surface((960,720))

        # Set up the NetworkManager for UDP communication
        print(f"Room UDP port: {room_udp_port}")
        room_udp_socket = NetworkManager(
            server_addr=config.SERVER_ADDR,
            server_port=room_udp_port
        )
        self.udp_room_socket =  room_udp_socket.udp_socket
        print("Game is starting...")

        # Load the level and initialize entities
        self.tilemap = Tilemap(self, tile_size=16)
        self.map = 4
        self.load_level(self.map)
        self.setup_background()
        self.entities = []
        self.scroll = [0, 0]

        # Set up the player
        self.player = None
        self.horizontal_movement = [0, 0]
        self.player_default_size = [50,60]
        # Set up queue for shared data
        q = queue.Queue(maxsize=1)
        def receive_messages(sock, q):
            while True:
                try:
                    data, _ = sock.recvfrom(1024)
                    if (len(data)!=0):
                        try :
                            q.get(block=False)
                        except Exception as e:
                            pass
                        q.put(data)
                except BlockingIOError as e:
                    continue
        thread = threading.Thread(target=receive_messages, args = (self.udp_room_socket, q),daemon=True)
        thread.start()

        while True:
            self.display.fill(color = (0,0,0,0))
            self.display_2.blit(pygame.transform.scale(self.game_background, self.display.get_size()), (0,0))

        # --------Find the player in the list of entities ------------
            if self.player is None:
                for each in self.entities:
                    if each.entity_type ==0 and each.id == self.user_id:
                        self.player = each
                        print("found player")
                        break
            
        # ------- Read inputs from player and send client to server --------
            msg= self.user_id.to_bytes(1, "big")
            movement_x=0
            movement_y=0
            action = 0
            interaction= 0 
            for event in pygame.event.get():
                if event.type ==pygame.QUIT:
                    pygame.quit()
                    sys.exit()
                if event.type == pygame.KEYDOWN:
                    if event.key == pygame.K_w:
                        movement_y+=1
                    if event.key == pygame.K_s:
                        movement_y-=1
                    if event.key == pygame.K_a:
                        self.horizontal_movement[0]=1
                    if event.key == pygame.K_d:
                        self.horizontal_movement[1]=1
                    if event.key == pygame.K_q:
                        action +=1
                    if event.key == pygame.K_e:
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

            if self.player is None:
                movement_y = 0

            msg += movement_x.to_bytes(1,"big") + movement_y.to_bytes(1,"big")+action.to_bytes(1,"big")+ interaction.to_bytes(1,"big")
            
        # get player size
            if self.player is not None:
                if (action==1 or action==2):
                    size = self.player.predict_next_frame_size(action)
                    msg +=size[0].to_bytes(1,"big") +size[1].to_bytes(1,"big")
                else:
                    msg += self.player.size[0].to_bytes(1,"big") + self.player.size[1].to_bytes(1,"big")
            else :
                msg += self.player_default_size[0].to_bytes(1,"big")+self.player_default_size[1].to_bytes(1,"big")
        # Send message to server
            byteSent = self.udp_room_socket.sendto(msg, (config.SERVER_ADDR, room_udp_port))
            if (byteSent<=0):
                pass
                print("Send payload: failed")
            else:
                pass
                #print(f"Sent {byteSent} to server")
        #Parse data
            data = ""
            try:
                data = q.get(block=False)
            except Exception as e:
                pass
            if len(data)!=0:
                self.de_serialize_entities(data)
        # Camera offset to get player to the center of the screen
            if self.player is not None:
                        self.scroll[0] +=  (self.player.rect(topleft =  self.player.pos).centerx -  self.display.get_width()/2 - self.scroll[0])/30
                        self.scroll[1] +=  (self.player.rect(topleft =  self.player.pos).centery -  self.display.get_height()/2 - self.scroll[1])/30
            render_scroll =  (int(self.scroll[0]),int (self.scroll[1]))

            if (self.game_mode ==1):
                self.display_base_sign(render_scroll)        
            self.tilemap.render(self.display, offset =  render_scroll)
            
            #update and render each entity
            if self.player is not None:
                # pygame.draw.rect(self.display, (255,0,0),
                #                  pygame.Rect(self.player.pos[0]-render_scroll[0], self.player.pos[1]-render_scroll[1],self.player.size[0],self.player.size[1]),
                #                  1)
                self.display_username(self.player, render_scroll, config.COLORS["LIGHT_GREEN"])
                # self.display_score(self.player, self.player.score, render_scroll, config.COLORS["WHITE"])
                self.display_healthbar_staminabar(self.player, render_scroll)
                self.player.render(self.display,offset = render_scroll)
            
            # render the flag 
            for each in self.entities:
                if each is not self.player and isinstance(each, Flag):
                    each.update()
                    each.render(self.display,offset = render_scroll)

            # render the player after the flags,items,traps to make them appear on top 
            for each in self.entities:
                if each is not self.player and isinstance(each, Player):
                    each.render(self.display,offset = render_scroll)
                    self.display_username(each, render_scroll, config.COLORS["BLUE"])
                    # self.display_score(each, each.score, render_scroll, config.COLORS["WHITE"])
                    self.display_healthbar_staminabar(each, render_scroll)

            self.display_2.blit(self.display, (0,0))
            self.screen.blit(pygame.transform.scale(self.display_2, self.screen.get_size()), dest = (0,0))
            pygame.display.update()
            self.clock.tick(40)

    def display_healthbar_staminabar(self, player: Player, render_scroll):
        """
        Display healthbar and stamina bar of players\n
        Args:
        - player: the player who need to display his/her health and stamina status
        - render_scroll: To modify the position of the bars
        """
        # Display health loss
        pygame.draw.rect(self.display, config.COLORS['RED'], (player.pos[0]-render_scroll[0], player.pos[1]-render_scroll[1]-17, 50, 7))
        
        # Display current health
        pygame.draw.rect(self.display, config.COLORS['LIGHT_GREEN'], (player.pos[0]-render_scroll[0], player.pos[1]-render_scroll[1]-17, (50/100) * (player.health), 7))
        
        # Display stamina
        pygame.draw.rect(self.display, config.COLORS['YELLOW'], (player.pos[0]-render_scroll[0], player.pos[1]-render_scroll[1]-7, (50/100) * (player.stamina), 3))

    def display_username(self, player: Player, render_scroll, color):
        """
        Display the username above the health bar of the player.
        
        Args:
        - player: The player whose username needs to be displayed.
        - render_scroll: To modify the position of the username.
        - color: The color of the username text.
        """
        username = self.id_to_username.get(player.id, "")

        # Load the font and render the username text
        username_font = pygame.font.Font(config.FONT_PATH, 10)
        username_text = username_font.render(username, True, color)
        
        # Calculate the position of the username (above the health bar)
        username_rect = username_text.get_rect(center=(
            player.pos[0] - render_scroll[0] + 25,  # Center above the health bar
            player.pos[1] - render_scroll[1] - 25  # Positioned slightly above the health bar
        ))
        
        # Blit the username onto the display
        self.display.blit(username_text, username_rect)

    def display_score(self, player: Player, score, render_scroll, color):
        """
        Display the player's score next to the right of the health bar.
        
        Args:
        - score: The score to display.
        - player: The player whose score needs to be displayed.
        - render_scroll: To modify the position of the score.
        - color: The color of the score text.
        """
        # Load the font and render the score text
        score_font = pygame.font.Font(config.FONT_PATH, 10)
        score_text = score_font.render(f"Score: {score}", True, color)
        
        # Calculate the position of the score (next to the right of the health bar)
        score_rect = score_text.get_rect(topleft=(
            player.pos[0] - render_scroll[0] + 55,  # Positioned to the right of the health bar
            player.pos[1] - render_scroll[1] - 17  # Align vertically with the health bar
        ))
        
        # Blit the score onto the display
        self.display.blit(score_text, score_rect)
    # display the base sign for two teams A and B
    def display_base_sign(self, offset):
        if len(self.base_sign)==0:
            self.base_sign = [Base(1, [224, 384],self), Base(2, [1648, 384],self)]
        
        for each in self.base_sign:
            each.update()
            each.render(self.display,offset)

if __name__ == "__main__":
    GameManager().menu()
