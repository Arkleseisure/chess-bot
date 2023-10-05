from ctypes import CDLL, c_ulonglong, c_int, c_char, Structure, c_bool, c_double, c_float
import sys
import os
import random
import time
import math
from Bits_and_pieces import get_bitboard_from_fen, convert_to_text, print_move, print_board, convert_text_to_bitboard_move

# imports the c libraries
game_mech_lib_path = os.path.dirname(os.path.abspath(__file__)) + "\\theories\game_mechanics_v1_%s.so" % (sys.platform)
print(game_mech_lib_path)
game_mech = CDLL(game_mech_lib_path)
game_mech.confirm_it_works()
engine_lib_path =  os.path.dirname(os.path.abspath(__file__)) + "\\theories/engine_v13_%s.so" % (sys.platform)
engine = CDLL(engine_lib_path)
engine.check_it_works()

'''
The equivalent to the piece class in the game_mechanics file, but in python
Last Modified: 19/9/21
Last Modified by: Arkleseisure
'''
class Piece(Structure):
	_fields_ = [('loc', c_ulonglong), ('type', c_int), ('captured', c_bool)]

	def __str__(self):
		types = ['pawn', 'knight', 'bishop', 'rook', 'queen', 'king']
		if self.type < 6:
			colour = 'white '
		else:
			colour = 'black '

		location = convert_to_text(self.loc)

		return ('captured ' if self.captured else '') + colour + types[self.type % 6] + ' on ' + location


'''
The equivalent to the Game structure in the game_mechanics file, but in python
Last Modified: 19/9/21
Last Modified by: Arkleseisure
'''
class Game(Structure):
	_fields_ = [('piece_list', (Piece * 32)), ('board', (c_ulonglong * 12)), ('hash', c_ulonglong), ('past_hash_list', (c_ulonglong * 100)), ('last_move', (c_ulonglong * 3)), 
			 ('to_play', c_int), ('ply_counter', c_int), ('castling', c_int), ('value', c_float), ('current_np_material', c_float), ('num_pawns', (c_int * 2))]


'''
Interface for the apply function
INPUTS:
game: Game struct, holding all variables relating to that point in the game.
move = 2 bitboards, one for the square being left and one for the square being landed on, 
	also a 7 bit flag containing extra info about the position
zobrist_numbers: list holding all the zobrist numbers used to calculate the zobrist hash for the next position. 

OUTPUTS:
new game struct with updated values.

Last Modified: 17/9/2021
Last Modified by: Arkleseisure
'''
def apply(game, move, zobrist_numbers):
	# casts the python variables into the equivalent types in c
	c_zobrist_numbers = (c_ulonglong * 793)(*zobrist_numbers)
	c_removed_hash = (c_ulonglong * 1)()
	c_move = (c_ulonglong * 3)(*move)
	c_game = (Game * 1)(*[game])
	piece_taken = game_mech.apply(c_game, c_move, c_zobrist_numbers, c_removed_hash)

	return c_game[0]
	

'''
Interface for the legal_moves function
INPUTS:
game: Game struct, holding all variables relating to that point in the game.

OUTPUTS:
legal_moves list containing all the legal moves in the position as c arrays of 2 bitboards and a flag (documented in play_game)

Last Modified: 16/9/2021
Last Modified by: Arkleseisure
'''
def legal_moves(game):
	# I think the maximum possible number of legal moves from a single position is about this
	# (https://lichess.org/analysis/fromPosition/R6R/3Q4/1Q4Q1/4Q3/2Q4Q/Q4Q2/pp1Q4/kBNN1KB1_w_-_-)
	c_legal_moves = ((c_ulonglong * 3) * 220)()
	c_game = (Game * 1)(*[game])

	num_moves = game_mech.legal_moves(c_game, c_legal_moves)
	return c_legal_moves[:num_moves]

'''
Gets the move from the ai, given the position
INPUTS:
game: Game struct, holding all variables relating to that point in the game.
zobrist_numbers: list holding all the zobrist numbers used to calculate the zobrist hash for the next position. 
time_allowed: float holding the amount of time the engine has to choose its move.

OUTPUTS:
move: move it thinks is the best in the position
current_value: evaluation of the current position

Last Modified: 17/9/2021
Last Modified by: Arkleseisure
'''
def get_engine_move(game, zobrist_numbers, time_allowed, book, engine_code=engine, value_output='int'):
	c_game = (Game * 1)(*[game])
	c_zobrist_numbers = (c_ulonglong * 793)(*zobrist_numbers)
	depth = 1
	move_number = (c_int * 1)()
	c_time_allowed = c_double(time_allowed)
	move = [0, 0, 0]
	moves = legal_moves(game)
	c_value = (c_float * 1)(*[0])
	c_depth = (c_int * 1)(*[1])
	c_nodes = (c_ulonglong * 1)(*[0])

	if len(moves) == 1:
		return 0.01, 0, 0, moves[0]

	# If the position is in the opening book, it chooses a random move according to how high it has scored and how much it was played in the human database
	if game.hash in book:
		# the things stored about each move are 1. number of times it occured, N, and 2. total score from that position, s.
		# these are weighted according to the formula: s * s/N, which gives a weight to the fractional score, meaning it avoids common blunders, 
		# and to the frequency of occurence meaning it plays mainline moves.
		total_score = sum(book[game.hash][key][1] ** 2 / book[game.hash][key][0] for key in book[game.hash])
		# generates a random number between 0 and the total score, which is then used to pick the move
		pick = random.uniform(0, total_score)
		# adds the weights of each move until it reaches the one in the pick
		current_pick = 0
		for key in book[game.hash]:
			current_pick += book[game.hash][key][1] ** 2 / book[game.hash][key][0]
			if current_pick >= pick:
				move_choice = key
				break;

		book_move = convert_text_to_bitboard_move(move_choice, game)
		for move in moves:
			if (move[0] == book_move[0] and move[1] == book_move[1] and move[2] == book_move[2]):
				return 0.01, 0, 0, move
	value = engine_code.get_engine_move(c_game, c_zobrist_numbers, move_number, c_time_allowed, c_value, c_depth, c_nodes)
	try:
		move = moves[int(move_number[0])]
	except IndexError:
		print('Index Error')
		print('Board was:')
		print_board(game.board)
		print('Moves were:')
		print(moves)
		print('Move given was index:')
		print(int(move_number[0]))
		move = moves[0]
	return float(c_value[0]), int(c_depth[0]) - 2, int(c_nodes[0]), move

'''
Generates pseudorandom numbers used for zobrist hashes (an efficient way to hash a board position), 
as well as the hash of the initial position and the list of past hashes, with the initial position included
More info on Zobrist Hashing here: https://www.chessprogramming.org/Zobrist_Hashing

Last Modified: 19/8/2021
Last Modified by: Arkleseisure
'''
def generate_zobrist_stuff(game):
	# ensures that the numbers are the same each time the program is run
	random.seed(1)

	# adds a 32 bit hash number for each piece on each square (12 * 64), castling rights (16), en-passant file (8) and black to move (1)
	# the values are ordered by piece as in the board variable, then by square number 
	# (e.g zobrist_numbers[0] is for a white pawn on a1, 1 is on a2, ...  64 is a white knight on a1, ...)
	# castling rights are ordered by binary value as held in the castling variable
	# (4 bits: kingside/queenside for black (8/4 in terms of int value) then kingside/queenside for white (2/1))
	zobrist_numbers = []
	for i in range(793):
		zobrist_numbers.append(random.getrandbits(64))

	game.hash = 0
	# the zobrist hash is made by xoring the zobrist numbers corresponding to each
	# item of the game
	# adds the pieces from the board
	for i in range(len(game.board)):
		binary_representation = bin(game.board[i])
		index = 0
		while (index != -1):
			index = binary_representation.find('1', index + 1)
			if index != -1:
				game.hash ^= zobrist_numbers[64 * i + len(binary_representation) - index - 1]

	# adds the castling legality
	game.hash ^= zobrist_numbers[64 * 12 + game.castling]

	# adds en-passant
	# checks if the last move was a double pawn move
	if game.last_move[-1] ^ 15 == 1:
		last_move_file = game_mech.file(game.last_move[1])
		
		# checks if there is a pawn on the squares next to the last move played
		if ((game.last_move[1] >> 1) & game.board[6 * game.to_play] != 0 and last_move_file > 0) or \
			((game.last_move[1] << 1) & game.board[6 * game.to_play] != 0 and last_move_file < 7):
			game.hash ^= zobrist_numbers[64 * 12 + 16 + last_move_file]

	# adds person to move
	if game.to_play == 1:
		game.hash ^= zobrist_numbers[64 * 12 + 16 + 8]

	game.past_hash_list[0] = c_ulonglong(game.hash)

	# ensures that numbers after this point are truly random
	random.seed(time.time())
	return zobrist_numbers

'''
Gets the board array from an input fen (just the part holding the board, the rest of the string must be discarded first)

INPUTS: 
game: Game struct holding information about the game. As this is mutable, it is changed but doesn't need to be returned.
board_fen: board part of a fen string (aka, fen string excluding castling rights, en passant square, ...)

Last Modified: 17/9/2021
Last Modified by: Arkleseisure
'''
def get_board(game, board_fen):
	white_pawns = get_bitboard_from_fen(board_fen, 'P')
	white_knights = get_bitboard_from_fen(board_fen, 'N')
	white_bishops = get_bitboard_from_fen(board_fen, 'B')
	white_rooks = get_bitboard_from_fen(board_fen, 'R')
	white_queens = get_bitboard_from_fen(board_fen, 'Q')
	white_king = get_bitboard_from_fen(board_fen, 'K')

	black_pawns = get_bitboard_from_fen(board_fen, 'p')
	black_knights = get_bitboard_from_fen(board_fen, 'n')
	black_bishops = get_bitboard_from_fen(board_fen, 'b')
	black_rooks = get_bitboard_from_fen(board_fen, 'r')
	black_queens = get_bitboard_from_fen(board_fen, 'q')
	black_king = get_bitboard_from_fen(board_fen, 'k')

	game.board = (c_ulonglong * 12)(*[white_pawns, white_knights, white_bishops, white_rooks, white_queens, white_king, 
                black_pawns, black_knights, black_bishops, black_rooks, black_queens, black_king])


'''
Initiates the piece list, an array containing a struct for each piece
INPUTS: 
game: struct containing all the information about a game

Last Modified: 17/9/2021
Last Modified by: Arkleseisure
'''
def initiate_piece_list(game):
	piece_types = {0: 'white pawn', 1: 'white knight', 2: 'white bishop', 3: 'white rook', 4: 'white queen', 5: 'white king', 
				6: 'black pawn', 7: 'black knight', 8: 'black bishop', 9: 'black rook', 10: 'black queen', 11: 'black king'}

	piece_num = 0
	try:
		for i in range(12):
			for j in range(64):
				if ((2 ** j) & game.board[i]) != 0:
					new_piece = Piece()
					new_piece.loc = 2 ** j
					new_piece.type = i
					new_piece.captured = False
					game.piece_list[piece_num] = new_piece
					piece_num += 1
		
			# ensures that the kings are always the 16th and 32nd in the piece_list as this fact is used in game_mechanics
			if i % 6 == 4 and piece_num % 16 != 15:
				for j in range(15 - (piece_num % 16)):
					new_piece = Piece()
					new_piece.loc = 0
					new_piece.type = 0
					new_piece.captured = True
					game.piece_list[piece_num] = new_piece
					piece_num += 1
			# does the same for the rooks, as this is used for quick location of the rooks when castling
			elif i % 6 == 2 and piece_num % 16 != 12:
				for j in range(12 - (piece_num % 16)):
					new_piece = Piece()
					new_piece.loc = 0
					new_piece.type = 0
					new_piece.captured = True
					game.piece_list[piece_num] = new_piece
					piece_num += 1
	except IndexError:
		print(piece_num)
		print_board(game.board)

'''
Gets the last_move variable (used for detecting legality of en-passant) when the game is initialized.
This is only done when the fen string specifies that the last move was a double pawn move.
INPUTS:
game: Game struct holding all the information about the game
en_passant_target: string holding the square which a pawn could potentially land on after an en-passant capture next turn.

Last Modified: 17/9/2021
Last Modified by : Arkleseisure
'''
def get_last_move(game, en_passant_target):
    last_from = 0
    last_to = 0
    last_flag = 0
    if en_passant_target != '-':
        last_flag = 2
        col = ord(en_passant_target[0]) - ord('a')
        row = int(en_passant_target[1]) - 1

        square_num = 8 * row + col
        target_square_bitboard = 2 ** square_num

        if to_play == 0:
            last_from = target_square_bitboard << 8
            last_to = target - square_bitboard >> 8
        else:
            last_from = target_square_bitboard >> 8
            last_to = target_square_bitboard << 8

    game.last_move = (c_ulonglong * 3)(*[last_from, last_to, last_flag])

'''
The perft function (PERFormace Test) is used for testing the efficiency of the game mechanics and for debugging in the case of faulty rules.
It works by calculating all the moves down to a certain depth and counting the number of nodes at that depth which are captures/castles/promotions/..., allowing us to compare
our results to generally accepted values.
It has two modes in which it can be run: 'all', which counts the number of nodes in each of the categories in the answer_type list, and
										 'nodes', which just counts the number of nodes, and is used for efficiency testing.

More info: https://www.chessprogramming.org/Perft

INPUTS:
game: Game struct, holding all the information about the position from which the perft function must be run
zobrist_numbers: list holding the zobrist numbers, used for updating the hashes for each position.
depth: int holding the depth to which the perft function should calculate
type: mode in which the perft function will be run, 'all' or 'nodes'

OUTPUTS:
answer_dict: dictionary with the types of answer as keys (as held in answer_type) and the values calculated as values.
time_taken: time taken to find these values.

Last Modified: 15/9/2021
Last Modified by: Arkleseisure
'''
def perft(game, zobrist_numbers, depth, type='all'):
	c_depth = c_int(depth)
	c_zobrist_numbers = (c_ulonglong * 793)(*zobrist_numbers)
	c_game = (Game * 1)(*[game])
	answer_type = ['Nodes', 'Captures', 'En passant', 'Castling', 'Promotion', 'Checks', 'Checkmates']
	answer_dict = {}

	# perft_all is used for testing the rules of the game, and returns statistics for various different things, 
	# while perft_nodes is used for speed benchmarking and only counts nodes.
	if type == 'all':
		answers = (c_ulonglong * 7)()
		start_time = time.time()
		game_mech.perft_all(c_game, answers, c_zobrist_numbers, c_depth)
		time_taken = time.time() - start_time
		game = c_game[0]
	else:
		answers = (c_ulonglong * 1)()
		start_time = time.time()
		game_mech.perft_nodes(c_game, c_depth, answers, c_zobrist_numbers)
		time_taken = time.time() - start_time

	for i in range(len(answers)):
		answer_dict[answer_type[i]] = answers[i]

	return answer_dict, time_taken


'''
Given a position in a game, returns whether or not the game is over.
Returns 0 if a win for black, 1 if a draw, 2 if a win for white and 3 if the game isn't over.
Last Modified: 17/9/2021
Last Modified by: Arkleseisure
'''
def terminal(game):
	c_game = (Game * 1)(*[game])
	return int(game_mech.terminal(c_game))

'''
Gets the code for a particular engine given the name of the engine
Last Modified: 19/9/2021
Last Modified by: Arkleseisure
'''
def get_engine_code(engine_name):
	if engine_name != '':
		return CDLL('theories/engine_%s.so' % (engine_name + '_' + sys.platform))
	return CDLL('theories/engine_%s.so' % sys.platform)
