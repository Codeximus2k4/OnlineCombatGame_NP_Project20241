import math
import random
import pygame
BASE_IMG_PATH =  'data/images'

class PhysicsEntity:
    def __init__(self, game, id, entity_type, entity_class,  pos,flip):
        self.game =  game
        self.id =  id
        self.entity_type= entity_type
        self.entity_class = entity_class
        self.pos = list(pos) 
        self.flip=flip

    def convert_entity_class(self):
        raise NotImplementedError("Have not implemented this method")
    def convert_action_type(self):
        raise NotImplementedError("Have not implemented this method")
    
    def set_action(self,action,fresh_start):
        if action!= self.action_type or fresh_start:
            fresh_start=False
            self.action_type= action
            self.animation =  self.game.assets[self.convert_entity_class()+'/'+self.convert_action_type()].copy()
    
    def render(self,surf):
            surf.blit(pygame.transform.flip(self.animation.get_img(),self.flip, False),dest = self.pos)
class Player(PhysicsEntity):
    def __init__(self, game, id, entity_class, posx, posy,health, stamina ):
        super().__init__(game, id, 0, entity_class, (posx, posy),flip=False)
        self.health =  health
        self.stamina = stamina
        self.action_type  = 0
    
    def update(self):
        #self.set_action(self.action_type)
        #If not in the animation of attacking then the character can move
        # if not self.isAttacking :
        #     if movement[0]>0:
        #         self.flip = False
        #         self.set_action('run')
        #     elif movement[0]<0:
        #         self.flip = True
        #         self.set_action('run')
        #     else:
        #         self.set_action('idle')
        
        #     self.velocity[0]= self.speed * movement[0]
        #     frame_movement=  (self.velocity[0],self.velocity[1])
        #     self.pos[0]+= frame_movement[0]

        #     # update on whether the player can attack or not with the cooldown time
        #     self.timeSinceAttack = min (self.timeSinceAttack+1, self.attack_cooldown+1)
        # else:
        
        if (self.action_type ==1 or self.action_type ==2) and self.animation.done:
            self.set_action(0,False)
        self.animation.update()

    def convert_entity_class(self):
        ent_class = {0:"Samurai",1:"Evil Mage"}
        return ent_class[self.entity_class]
        
    def convert_action_type(self):
        action = {0:"idle",1:"attack1",2:"attack2",3:"run",4:"jump",5:"crouch",6:"fall",
                  7:"special ability",8:"block",9:"dash",10:"take hit",11:"death"}
        return action[self.action_type]
    def set_action(self, action, fresh_start):
        super().set_action(action, fresh_start)
    def render(self,surf):
        super().render(surf)

    # def ground_attack(self, attack_animation,attack_cooldown = 3):
    #     if self.timeSinceAttack > self.attack_cooldown:
    #         self.attack_cooldown = attack_cooldown
    #         self.isAttacking = True
    #         self.set_action(attack_animation)
    #         self.timeSinceAttack = 0
    # def serialize_data(self):
    #     return "player|" + str(self.id) +"|"+str(self.pos[0])+"|"+str(self.pos[1])+"|"+str(int(self.flip))+"|"+self.action+"|"+str(self.animation.frame)

    
    
    
