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
        self.dashing = 0
        self.jump_cooldown = 3
        self.speed = 3
        self.dashing = 0
        self.isAttacking = False
        self.attack_cooldown = 5
        self.timeSinceAttack = 0
        
    
    def update(self, movement = [0,0]):

        #If not in the animation of attacking then the character can move
        if not self.isAttacking :
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

            # update on whether the player can attack or not with the cooldown time
            self.timeSinceAttack = min (self.timeSinceAttack+1, self.attack_cooldown+1)
        else:
            if self.animation.done:
                self.isAttacking = False
                self.set_action('idle')
        self.animation.update()


    def render(self,surf):
        super().render(surf)

    def ground_attack(self, attack_animation,attack_cooldown = 3):
        if self.timeSinceAttack > self.attack_cooldown:
            self.attack_cooldown = attack_cooldown
            self.isAttacking = True
            self.set_action(attack_animation)
            self.timeSinceAttack = 0
    def serialize_data(self):
        return "player|" + str(self.id) +"|"+str(self.pos[0])+"|"+str(self.pos[1])+"|"+str(int(self.flip))+"|"+self.action+"|"+str(self.animation.frame)

    
    def render(self,surf):
        super().render(surf)
    
    
