# Gotham Chess bot
Chess engine coded in c, with UI using the python pygame library
If you want to download a version to play with, this will work on Windows:
https://drive.google.com/drive/folders/1vCnzwUARBz3m_jzsfWfNYJDO8Gg9NO6R?usp=sharing
Download the entire "Gotham_bot" folder and run "main.exe" to test the engine out.

Welcome to the Gotham Discord Chess bot project. 
If you're looking to help out, here is most of the information you will need (it's all the information I could think of and be bothered to write down, but I'm sure to have forgotten something). Also, you may need to ask me to help out with setting things up at first... I had quite a few troubles, so hopefully I can spare you some of those.

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

## Some key definitions
### Bitboard
Bitboards are 64 bit unsigned integers used to store a board state. Each bit stores whether or not a particular piece is on that square, so for instance a piece on a1 would be stored as the number 1, on b1 as 2, c1 4, and so on. a2 would be 2 ^ 8, a3 2^16 or in other words in binary it would be stored as all 0s except for the 17th from the right, which would be a 1. The board state and moves are the main things which are stored this way.

### Piece types
Each piece type has its own associated index, from 0 to 11. The first 6 are the white pieces, the last 6 black. The order for both is P, N, B, R, Q, K.

### Hash
64 bit zobrist hash generated from the current position on the board. This is used to detect draw by repetition, but may also come in useful for indexing things in the future.

### Move storage
Moves are stored as 3 64 bit numbers in an array. The first is the bitboard of the location it has come from, the second the bitboard of the location it is going to and the 3rd is a flag containing other important information about the move, such as promotions and captures. Full documentation for this is in the initialize_game function of the play_game file.

### Transpositions/Transposition table
Transpositions are positions which occur multiple times in the search through different move orders... e.g you get to the same position by playing 1.e4 e6 2.d4 as you do when you play 1.d4 e6 2.e4. A transposition table holds positions which have already been searched so that if they are hit through a transposition later on the engine can reuse the evaluation that has already been calculated.

## Main data structures
### Game
This data structure essentially holds all the information you need about the current position in the game. Its variables are:

**Piece list**:  
Array holding all 32 pieces as Piece structs. The first 16 are the white pieces, ordered by value (i.e P, N, B, R, Q, K), and the last 16 the black pieces.  
Each piece struct has 3 variables:  
- captured: boolean holding whether or not the piece has been captured.  
- loc: location of the piece as a bitboard  
- type: type of piece as an int  

**Board**:  
Array holding the current board state as a series of bitboards for each piece, 0 being white pawns, 1 white knights, ... 11 black kings

**Hash**:  
Zobrist hash of the current position. Currently used to detect draw by repetition, but can potentially be used in the future to index transposition tables for instance. 

**Past Hash list**:  
List of all previous hashes back to the last irreversible move. This is used to detect draw by repetition. 

**Last Move**:  
Last move made. Used to see if en-passant is possible. 

**To play**:  
Player to move next. 0 if white, 1 if black.

**Ply counter**:  
Number of ply (one move for one player) since the last irreversible move, used for looking for the draw by 50 move rule. 

**Castling**:  
Although it is stored as an integer, only the last 4 bits are used, k/q side for black then k/q side for white.

**Value**:  
Current minimax value of the position. This is updated after each move as it is more efficient than recalculating every time the position is evaluated.

**Current np material**:  
Total amount of non-pawn material currently on the board. Used for interpolating between values used in the endgame and values used in the middlegame.

### Zobrist Numbers
List of numbers used to generate 64 bit hashes for each position on the chess board. These hashes are not unique for each chess position but might as well be (64 bits is on the order of 10^19 so the likelihood of collisions is very low). They are used to detect draw by repetition, as well as index the transposition table, the opening book and the endgame tablebase (in development at time of writing).

### Engine
Here are some of the data structures used by the engine:  
**Values**:   
Values of the pieces, by index. Black pieces are negative, as a black advantage is represented by a negative number.

**Piece Square table mg**:  
Values for each piece being positioned on each square in the middlegame. This, along with the endgame piece-square table is indexed by piece number (i.e 0 for pawn, 1 for knight, ...)

**Piece Square table eg**:  
Values for each piece being poisitioned on each square in the endgame. 

**Max np material**:  
Amount of non-pawn material at the start of the game. This is used to help with the interpolation between the middlegame and endgame.

**Transposition table**
Array holding the evaluation, type of node (i.e whether it was fully searched or is an upper/lower bound), depth searched and best move for positions that have been searched, indexed by part of their zobrist hash. Another part of the hash is stored to prevent collisions (i.e a transposition is incorrectly identified and the results stored are used in the wrong context.)

**Nodes**
The node class is used to store information from previous searches. For each position searched, the evaluation is stored so that it can then be used to order the moves of later searches well, hence improving their efficiency. Equally it holds the locations of its child nodes to help build up the tree and a couple of other bits and pieces to help the smooth running of the code.

## Basic functioning
Full documentation for all functions regarding inputs and outputs should be in their docstrings.

### Game mechanics
There are essentially 4 main useful functions which are called from game_mechanics (the other functions are generally called by these ones). These are:  

**Apply**:  
Applies an input move to the game struct.

**Unapply**:  
Undoes the changes made by the apply function.

**Legal_moves**:  
Returns the legal moves in a given position.

**Terminal**:  
Figures out if the game has ended. Returns 0 for a black win, 1 for a draw, 2 for a white win and 3 for an unfinished game.

### Engine
There are a few functions to be aware of that the engine uses:

**Get engine move**:  
This is the function called by the python. It runs a minimax at iteratively increasing depth until the allocated time is up. Then, it returns the result of the deepest search to date. 

**Minimax**:  
Performs a minimax search with alpha-beta pruning.

**Evaluate**:  
Returns an evaluation of the current position. At the time of writing, this evaluation includes material imbalance and piece-square tables.

**Fully evaluate**:  
Initialises various variables to the values for the current position. This means that they only have to be updated after each move, which is generally more efficient.

**Update value**:  
Updates the value according to the previous move.

**Get psqt value**:  
Returns the value of a particular piece on a particular square.

### Testing
These functions are all in the play_game.py file.

**Test game mechanics**:  
Verifies that the game mechanics work correctly. This would generally be used if you were trying to improve the efficiency of the game mechanics, and wished to verify that they still work as expected.

**Speed test**:  
Tests the speed of the game mechanics using a perft function. This would be used after you've verified the game mechanics function to check that they are indeed faster than the original version. 

**Test engines**:  
Performs round robin tournaments between a group of engines and outputs statistics such as estimated elos (note that for accurate elo estimations you will need to input the elo of at least one of the engines). 

**Test engine**:  
Tests one engine against a bunch of opponents of known strength to quickly determine strength. Note that it is generally better to make it play a head-to-head with the previous version using test_engines as this is a better indicator of head-to-head strength.

**Test engine on pos**:  
Tests an engine on a bunch of positions. This is to be used while printing stuff from the engine, in order to test its outputs vs a previous version, normally in debugging cases. 

**Do test stuff**:  
This is what is called directly from main, so muck around with the stuff in here to choose what you actually want to do with the test.


Copyright: Honestly don't have a copyright cos who the hell would want to copy this thing? I mean, go use Stockfish's code for crying out loud.
