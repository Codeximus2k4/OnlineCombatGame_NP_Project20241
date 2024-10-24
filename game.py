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

    def serializePlayerInfo(self):
        """
        Concatenates all relevant information of a Player object into a string.

        Args:
            player: A Player object representing the player's data.

        Returns:
            A string containing player information in a well-formatted way.
        """

        # Extract relevant information from the Player object
        id = self.player.id
        entity_type = self.player.entity_type
        pos = self.player.pos

        if(self.player.flip is True): flip = 1
        else: flip = 0

        action = self.player.action
        frame =  self.player.animation.frame

        # Format the information into a string with clear labels
        info_string = f"{id}|{entity_type}|{pos[0]}|{pos[1]}|{flip}|{action}"    # send the existed info according to format message of server

        return info_string

    def get_font(self, size):  # Returns Press-Start-2P in the desired size
        return pygame.font.Font(FONT_PATH, size)

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
                    print("Received from server: ", data.decode())
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
                    msg = msg + "u"
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
