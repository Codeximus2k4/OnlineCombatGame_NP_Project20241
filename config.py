import os
import pygame

# Network Configuration
SERVER_ADDR = "192.168.39.82"
SERVER_PORT = 5500

# Display Configuration
SCREEN_WIDTH = 960
SCREEN_HEIGHT = 720
FPS = 60

# Paths
BASE_PATH = os.path.dirname(os.path.abspath(__file__))
FONT_PATH = os.path.join(BASE_PATH, 'data/images/menuAssets/font.ttf')
BACKGROUND_PATH = os.path.join(BASE_PATH, 'data/images/menuAssets/Background.png')
BASE_IMG_PATH = os.path.join(BASE_PATH, 'data/images/')

# Colors
COLORS = {
    'WHITE': (255, 255, 255),
    'BLACK': (0, 0, 0),
    'RED': (255, 0, 0),
    'GREEN': (0, 128, 0),
    'BLUE': (0, 0, 128),
    'BROWN': (182, 143, 64),
    'LIGHT_GREEN': (0, 255, 0),
    'LIGHT_BLUE': (0, 0, 255),
    'YELLOW': (255, 255, 0),
    'MENU_TEXT': "#b68f40",
    'BUTTON_BASE': "#d7fcd4"
}

# Initialize Pygame
pygame.init()