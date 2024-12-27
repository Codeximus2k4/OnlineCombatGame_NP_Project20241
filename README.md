Based on the uploaded game document, here is a draft README file for your project:

---

# Multiplayer Online Combat Game

## Project Overview
This project is a **Multiplayer Online Combat Game** inspired by popular fighting games like Tekken and Street Fighter, with arena mechanics from Strike Force Heroes. The game features pixel art, dynamic gameplay, and multiple modes for engaging multiplayer combat.

---

## Features
- **Game Modes**:
  - **Classic**: Players battle in a trap-filled arena to score points by defeating opponents.
  - **Capture the Flag**: Teams compete to capture the enemy's flag and return it to their base.

- **Characters**:
  - Samurai: Fast attacks, high mobility.
  - Wizard: Powerful magic attacks with slow effects.
  - King: High health and regeneration, with a large attack range.
  - Witch: Drains opponent's stamina and mana.

- **Dynamic Elements (To be implemented)**:
  - Traps (fire and slow).
  - Power-ups and abilities (e.g., speed boost, immunity, health restoration).
  - Environmental interactions.

- **Pixel Art**: Unique character animations and immersive environment design.

---

## Gameplay Mechanics
- **Player Attributes**:
  - Health and stamina management.
  - Diverse attack options: melee, and special abilities.
  - Interactive movement: run, jump and attack.

- **Combat Scoring**:
  - Points for attacks, kills, flag captures.

- **Respawn System**:
  - Players can rejoin the match after being eliminated after 10s.

---

## Technical Details
- **Server-Side**:
  - Game logic and player data processing.
  - Real-time synchronization of game states.

- **Client-Side**:
  - Player controls and game rendering.
  - Collision detection and interaction with items/traps.

- **Object-Oriented Programming (OOP)**:
  - Modular design with clear definitions for items, traps, and characters.

---

## Team Members
1. Nguyễn Hữu Nam Hòa
2. Hoàng Trung Hiếu
3. Nguyễn Huy Hoàng
4. Ngô Minh Đức

---

## Getting Started
### Prerequisites
- Programming languages: Python (Client), C (Server).
- Game assets: Pixel art sprites, animations, tile sets, visual effects.

### Installation
1. Clone the repository.
2. Install required dependencies for the server and client.
3. Run SQL file to create database using __server/db-data/db-init.sql__
4. Compile __server/server.cpp__ 
5. Start the server and launch the __main.py__ to begin playing.


---

## Roadmap
- Add more characters and abilities.
- Enhance visual effects and animations.
- Introduce new game modes and maps.
