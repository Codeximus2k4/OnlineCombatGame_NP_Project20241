import pygame
import traceback
import sys
from client.game import GameManager
from scripts.utils import logout_request

def main():
    try:
        # Initialize Pygame
        pygame.init()



        # Create Game Manager
        game_manager = GameManager()
        game_manager.login_screen()



    except Exception as e:
        print(f"An error occurred: {e}")
        traceback.print_exc() # better for debugging
    finally:
        if game_manager.user_id:
            logout_request(game_manager.user_id)
        pygame.quit()
        sys.exit()

if __name__ == "__main__":
    main()