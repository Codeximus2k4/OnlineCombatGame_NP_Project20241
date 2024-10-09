import os
import sys
import math
import pygame
from scripts.utils import load_image, load_images, Animation
from scripts.entities import PhysicsEntity, Player
class Game:
    def __init__(self):
        pygame.init()

        pygame.display.set_caption('combat game')
        
        self.clock  = pygame.time.Clock()
        self.screen = pygame.display.set_mode((960,720))
        self.display =  pygame.Surface((640,480), pygame.SRCALPHA)
        self.display_2 =  pygame.Surface((640,480))
        self.horizontal_movement = [False,False]
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
        self.background_offset_y = -80

        self.background =  pygame.Surface(self.assets['background'][0].get_size())
        count = 0
        for each in self.assets['background']:
            count+=1
            if count>=3:
                self.background.blit(each,(0,self.background_offset_y))
            else :
                self.background.blit(each, (0,0))
        
        self.player = Player(game  = self, pos = (0,280),size = (128,128))
    def run(self):
        while True:
            self.display.fill(color = (0,0,0,0))
            self.display_2.blit(pygame.transform.scale(self.background, self.display.get_size()), (0,0))

            for event in pygame.event.get():
                if event.type ==pygame.QUIT:
                     pygame.quit()
                     sys.exit()
                if event.type == pygame.KEYDOWN:
                    if event.key == pygame.K_LEFT:
                        self.horizontal_movement[0]= True
                    if event.key == pygame.K_RIGHT:
                        self.horizontal_movement[1]= True
                if event.type == pygame.KEYUP:
                    if event.key == pygame.K_LEFT:
                        self.horizontal_movement[0]= False
                    if event.key == pygame.K_RIGHT:
                        self.horizontal_movement[1]= False
            
            if self.player.velocity[0]!=0:
                self.player.set_action('run')
            self.player.update((self.horizontal_movement[1]-self.horizontal_movement[0],0))
            self.player.render(self.display)

            self.display_2.blit(self.display, (0,0))
            self.screen.blit(pygame.transform.scale(self.display_2, self.screen.get_size()), dest = (0,0))
            pygame.display.update()
            self.clock.tick(60)

Game().run()
