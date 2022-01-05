# chess-bot
Chess engine coded in c, with UI using the python pygame library

## Files:  
__Bits_and_pieces.py__: Used for various functions which may be useful in multiple places  
__c_interface.py__: Used to connect the C-code to the python. Python code calling things from a c library should go via functions in this file first.  
__display_menu.py__: Code for displaying the menu.  
__draw_board.py__: Code for drawing the board  
__engine_v-.c__: The -th iteration of the engine. Generally used for testing against current version.  
**engine_v-_win32.so**: Compiled Windows binary of engine_v-.c  
__game_mechanics_v1.c__: All the code related to how the game of chess works.  
__main.py__: Function from which everything else is called... Also holds comments relating to version history of the engines.  
__menu.py__: Holds code relating to the menu.  
__play_game.py__: Holds all the code relating to initializing the python elements of the game, playing the game and any sort of engine testing. Also the initialize_game function in this file contains a  lot of useful information about how the actual processing of moves works.  
__anything.png__: Image used for the display. P, N, B, R, Q, K all refer to their respective pieces and w/b refers to white or black. There are also a couple of arrows.

### Key files:  
__engine.c__ - final version will have the most up-to-date code. This is where most of the efforts to improve the engine will be made.  
__play_game.py__ - file where code should be changed when you want to change up a test... do_test_stuff at the bottom of the code is what will generally be called.  
__main.py__ - run your code from here.  
__game_mechanics_v1.c__ - If you want to improve the efficiency of how the engine finds its legal moves and applies/unapplies moves, this is the file for that.  
__c_interface.py__ - Occasionally you may want to change the Game or Piece struct (for instance, storing information to help with efficiency). When this happens, the python declaration in this file will also have to be changed.
