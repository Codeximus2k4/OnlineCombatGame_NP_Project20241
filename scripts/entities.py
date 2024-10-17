import math
import random
import pygame
BASE_IMG_PATH =  'data/images'

class PhysicsEntity:
    def __init__(self, game, entity_type, pos, size):
        self.game =  game
        self.entity_type =  entity_type
        self.pos = list(pos)
        self.size = size
        self.velocity = [0,0]
        self.flip =  False
        self.action = ''
        self.set_action('idle')
    def set_action(self,action):
        if action!= self.action:
            self.action  = action
            self.animation =  self.game.assets[self.entity_type+'/'+self.action].copy()
    def render(self,surf):
        if self.flip:
            surf.blit(pygame.transform.flip(self.animation.get_img(),self.flip, False),dest = self.pos)
        else:
            surf.blit(self.animation.get_img(),dest = self.pos)
class Player(PhysicsEntity):
    def __init__(self, id, game,pos,size):
        super().__init__(game, 'player', pos, size)
        self.id = id
        self.air_time = 0
        self.speed = 3
        self.dashing = 0
    def update(self, movement = [0,0]):
        if movement[0]>0:
            self.flip = False
            self.set_action('run')
        elif movement[0]<0:
            self.flip = True
            self.set_action('run')
        else:
            self.set_action('idle')
        
        self.velocity[0]= self.speed * movement[0]
        frame_movement=  (self.velocity[0],self.velocity[1])
        self.pos[0]+= frame_movement[0]
        self.animation.update()
    def render(self,surf):
        super().render(surf)
