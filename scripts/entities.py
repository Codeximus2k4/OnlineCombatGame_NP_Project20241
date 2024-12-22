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
        self.velocity_y = 3
        self.collisions = {'up':False, 'down':False,'right' : False, 'left':False}
    def convert_entity_class(self):
        raise NotImplementedError("Have not implemented this method")
    def convert_action_type(self):
        raise NotImplementedError("Have not implemented this method")
    
    def set_action(self,action,fresh_start):
        if action!= self.action_type or fresh_start:
            fresh_start=False
            self.action_type= action
            self.animation =  self.game.assets[self.convert_entity_class()+'/'+self.convert_action_type()].copy()
    
    def update(self, tilemap, movement = (0,0)):
        self.collisions = {'up':False, 'down':False,'right' : False, 'left':False}
        

        predicted_pos = [self.pos[0]+movement[0],self.pos[1]]
        entity_rect =  self.rect(topleft=predicted_pos,size= (200,200))
        for rect in tilemap.physics_rects_around(predicted_pos):
            if entity_rect.colliderect(rect):
                if (movement[0]>0):
                    entity_rect.right =  rect.left
                    self.collisions['right'] = True
                if movement[0]<0:
                    entity_rect.left =  rect.right
                    self.collisions['left'] = True
                self.pos[0]= entity_rect.x
                predicted_pos[0]= self.pos[0]
        
        predicted_pos = [predicted_pos[0],self.pos[1]+ self.velocity_y]
        entity_rect =  self.rect(topleft=predicted_pos, size = (200,200))
        for rect in tilemap.physics_rects_around(predicted_pos):
            if entity_rect.colliderect(rect):
                if (movement[1] >0):
                    entity_rect.bottom = rect.top
                    self.collisions['down'] = True
                if (movement[1]<0):
                    entity_rect.top = rect.bottom
                    self.collisions['up'] =  True
                self.pos[1] =  entity_rect.y
        self.velocity_y =  min(5, self.velocity_y+0.1)
    def rect(self,size, topleft = [0,0]):
        return pygame.Rect(topleft[0],topleft[1], size[0],size[1])
    
    def render(self,surf, offset = (0,0)):
            surf.blit(pygame.transform.flip(self.animation.get_img(),self.flip, False),dest = (self.pos[0]-offset[0],self.pos[1]-offset[1]))
class Player(PhysicsEntity):
    def __init__(self, game, id, entity_class, posx, posy,health, stamina ):
        super().__init__(game, id, 0, entity_class, (posx, posy),flip=False)
        self.health =  health
        self.stamina = stamina
        self.action_type  = 0
    
    def update(self,tilemap, movement= (0,0)):
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
        super().update(tilemap=tilemap,movement=movement)
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
    def render(self,surf,offset = (0,0)):
        super().render(surf,offset=offset)
    def rect(self, size=(0,0), topleft = [0,0]):
        return super().rect(size =size,topleft=topleft)

   
    
