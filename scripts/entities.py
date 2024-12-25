import math
import random
import pygame
BASE_IMG_PATH =  'data/images'

class PhysicsEntity:
    def __init__(self, game, id, entity_type, entity_class,  pos,flip, team =0 ):
        self.game =  game
        self.team = 0
        self.score =0
        self.id =  id
        self.entity_type= entity_type
        self.entity_class = entity_class
        self.velocity_y=3
        self.pos = list(pos) 
        self.flip=flip
        self.collisions = {'up':False, 'down':False,'right' : False, 'left':False}
        self.size = (0,0)
        self.prev_size = (0,0)
        self.collision_mesh =  None
    def generate_collision_mesh(self):
        self.collision_mesh =  {'left':[], 'right': [],'up':[],'down':[]}
        coord_x = [i for i in range(self.pos[0],self.pos[0]+self.size[0],self.game.tilemap.tile_size)]
        coord_x.append(self.size[0]+self.pos[0])
        coord_x.append(self.pos[0])
        coord_y = [i for i in range(self.pos[1],self.pos[1]+self.size[1],self.game.tilemap.tile_size)]
        coord_y.append(self.size[1]+self.pos[1])
        coord_y.append(self.pos[1])
        for each in coord_y:
            self.collision_mesh['left'].append([self.pos[0]-2,each])
        for each in coord_y:
            self.collision_mesh['right'].append([self.pos[0]+ self.size[0]+2,each])
        for each in coord_x:
            self.collision_mesh['up'].append([each,self.pos[1]-2])
        for each in coord_x:
            self.collision_mesh['down'].append([each,self.pos[1]+self.size[1] + 2])
    def convert_entity_class(self):
        raise NotImplementedError("Have not implemented this method")
    def convert_action_type(self):
        raise NotImplementedError("Have not implemented this method")
    def set_action(self,action,fresh_start):
        if action!= self.action_type or fresh_start:
            self.action_type= action
            self.animation =  self.game.assets[self.convert_entity_class()+'/'+self.convert_action_type()].copy()
            self.size = self.animation.get_size()
        else :
            self.update()                       
    def update(self):
        self.velocity_y = min(self.velocity_y+0.1, 3)
        self.animation.update()
        self.size =  self.animation.get_size()
    def predict_next_frame_size(self, action):
        if (action!= self.action_type):
            animation =  self.game.assets[self.convert_entity_class()+'/'+self.convert_action_type()].copy()
            return animation.get_size()
        else :
            self.animation.update()
            temp  = self.animation.get_size()
            self.animation.revert_frame()
            return temp

    #self.collisions = self.check_collision(tilemap, movement)
    def rect(self,topleft = [0,0]):
        return pygame.Rect(topleft[0],topleft[1], self.size[0],self.size[1])
    
    def render(self,surf, offset = (0,0)):
            surf.blit(pygame.transform.flip(self.animation.get_img(),self.flip, False),dest = (self.pos[0]-offset[0],self.pos[1]-offset[1]))
class Player(PhysicsEntity):
    def __init__(self, game, id, entity_class, posx, posy,health, stamina ,team):
        super().__init__(game, id, 0, entity_class, (posx, posy),flip=False,team=team)
        self.health =  health
        self.stamina = stamina
        self.action_type=0
    
    def update(self):
        super().update()

        # if (self.action_type ==1 or self.action_type ==2) and self.animation.done:
        #handle jump end:
    
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

   
    
