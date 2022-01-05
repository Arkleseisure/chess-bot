# chess-bot
Chess engine coded in c, with UI using the python pygame library
Files:
Bits_and_pieces.py: Used for various functions which may be useful in multiple places
c_interface.py: Used to connect the C-code to the python. Python code calling things from a c library should go via functions in this file first. 
display_menu.py: Code for displaying the menu.
draw_board.py: Code for drawing the board
engine_v-.c: The -th iteration of the engine. Generally used for testing against current version.
engine_v-_win32.so: Compiled Windows binary of engine_v-.c
game_mechanics_v1.c: All the code related to how the game of chess works.
main.py: Function from which everything else is called... Also holds comments relating to version history of the engines.
menu.py: Holds code relating to the menu.
play_game.py: Holds all the code relating to initializing the python elements of the game, playing the game and any sort of engine testing. 
              Also the initialize_game function in this file contains a  lot of useful information about how the actual processing of moves works.

Key files:
engine.c - final version will have most up-to-date code. This is where most of the efforts to improve the engine will be made.
play_game.py - file where code should be changed when you want to change up a test... do_test_stuff at the bottom of the code is what will generally be called.
main.py - run your code from here.
game_mechanics_v1.c: If you want to improve the efficiency of how the engine finds its legal moves and applies/unapplies moves, this is the file for that.
