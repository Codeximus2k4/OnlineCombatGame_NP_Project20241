import pygame
import sys
import config
from client.network import NetworkManager
from client.game import GameManager

def main():
    try:
        # Initialize Pygame
        pygame.init()

        # Create Network Manager
        network_manager = NetworkManager(
            server_addr=config.SERVER_ADDR, 
            server_port=config.SERVER_PORT
        )


        # Create Game Manager
        game_manager = GameManager(network_manager)
        game_manager.login_screen()



    except Exception as e:
        print(f"An error occurred: {e}")
    finally:
        # Ensure clean shutdown
        network_manager.close()
        pygame.quit()
        sys.exit()

if __name__ == "__main__":
    main()