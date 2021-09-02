from ctypes import CDLL, c_ulonglong, c_int, c_char, Structure, c_bool
import sys
import Bits_and_pieces as BaP
import random
import time

# imports the c libraries
game_mechanics_lib_path = 'theories/game_mechanics_%s.so' % (sys.platform)
game_mech = CDLL(game_mechanics_lib_path)
game_mech.confirm_it_works()


class Piece(Structure):
	_fields_ = [('loc', c_ulonglong), ('type', c_int), ('captured', c_bool)]

	def __str__(self):
		types = ['pawn', 'knight', 'bishop', 'rook', 'queen', 'king']
		if self.type < 6:
			colour = 'white '
		else:
			colour = 'black '

		location = BaP.convert_to_text(self.loc)

		return ('captured ' if self.captured else '') + colour + types[self.type % 6] + ' on ' + location

'''
Interface for the apply function
INPUTS:
board = board position as a list of 12 bitboards, one for each piece, each represented by an integer
move = 2 bitboards, one for the square being left and one for the square being landed on, 
	also a 7 bit flag containing extra info about the position
castling = 4 bit int representing current castling legality (based on the movement of the king and rooks... other factors still have to be checked)
to_play = 0 (white to play) or 1 (black to play)
piece_list = length 32 array containing structs with all the pieces and their locations for quicker move generation
OUTPUTS:
Same as inputs, but with updated values. Also, move is now replaced by last_move, as this needs to be changed when a move is applied.
but the move applied is already known to the program calling this function.
Last Modified: 04/07/2021
Last Modified by: Arkleseisure
'''
def apply(board, move, castling, to_play, piece_list):
	# casts the python variables into the equivalent types in c
	c_board = (c_ulonglong * 12)(*board)
	c_move = (c_ulonglong * 3)(*move)
	c_last_move = (c_ulonglong * 3)()
	# extras is created so that the object passed is mutable, making it easier to
	# return stuff
	extras = (c_int * 2)(*[castling, to_play])


	# applies the move with the new types
	game_mech.apply(c_board, c_move, extras, c_last_move, piece_list)
	extras = extras[:]
	castling = extras[0]
	to_play = extras[1]

	# returns the c arrays as python lists ([:] will automatically cast an
	# iterable into a list)
	return c_board[:], castling, to_play, c_last_move[:]

'''
Interface for the legal_moves function
INPUTS:
board = board position as a list of 12 bitboards, one for each piece, each represented by an integer
castling = 4 bit int representing current castling legality (based on the movement of the king and rooks... other factors still have to be checked)
to_play = 0 (white to play) or 1 (black to play)
last_move = last move to be played as 2 bitboards and a flag
piece_list = length 32 array containing structs with all the pieces and their locations for quicker move generation
OUTPUTS:
legal_moves list containing all the legal moves in the position as c arrays of 2 bitboards and a flag (documented in play_game)
Last Modified: 2/9/2021
Last Modified by: Arkleseisure
'''
def legal_moves(board, castling, to_play, last_move, piece_list):
	# casts the variables into equivalent types in c
	c_board = (c_ulonglong * 12)(*board)
	c_last_move = (c_ulonglong * 3)(*last_move)
	c_to_play = c_int(to_play)
	c_castling = c_int(castling)
	# I think the maximum possible number of legal moves from a single position is about this
	# (https://lichess.org/analysis/fromPosition/R6R/3Q4/1Q4Q1/4Q3/2Q4Q/Q4Q2/pp1Q4/kBNN1KB1_w_-_-)
	c_legal_moves = ((c_ulonglong * 3) * 220)()

	num_moves = game_mech.legal_moves(c_board, c_legal_moves, c_castling, c_to_play, c_last_move, piece_list)
	return c_legal_moves[:num_moves]

# initiates the piece list, an array containing a struct for each piece
def initiate_piece_list(board):
	piece_list = (Piece * 32)()
	piece_num = 0
	for i in range(12):
		for j in range(64):
			if ((2 ** j) & board[i]) != 0:
				new_piece = Piece()
				new_piece.loc = 2 ** j
				new_piece.type = i
				new_piece.captured = False
				piece_list[piece_num] = new_piece
				piece_num += 1
		
		# ensures that the kings are always the 16th and 32nd in the piece_list as this fact is used in game_mechanics
		if i % 6 == 4 and piece_num % 16 != 15:
			for i in range(15 - (piece_num % 16)):
				new_piece = Piece()
				new_piece.loc = 0
				new_piece.type = 0
				new_piece.captured = True
				piece_list[piece_num] = new_piece
				piece_num += 1

	print('Number of pieces in piece list:', piece_num)
	return piece_list

'''
Generates pseudorandom numbers used for zobrist hashes (an efficient way to hash a board position), 
as well as the hash of the initial position and the list of past hashes, with the initial position included
Last Modified: 19/8/2021
Last Modified by: Arkleseisure
'''
def generate_zobrist_stuff(board, castling, to_play, last_move):
	random.seed(1)

	# adds a 32 bit hash number for each piece on each square (12 * 64), castling
	# rights (16), en-passant file (8) and black to move (1)
	# the values are ordered by piece as in the board, then by square number (e.g
	# zobrist_numbers[0] is for a white pawn on a1, 1 is on a2, ...  64 is a white
	# knight on a1, ...)
	# castling rights are ordered by binary value as held in the castling variable
	# (4 bits: kingside/queenside for black (8/4 in terms of int value) then
	# kingside/queenside for white (2/1))
	zobrist_numbers = []
	for i in range(793):
		zobrist_numbers.append(random.getrandbits(32))

	initial_hash = 0
	# the zobrist hash is made by xoring the zobrist numbers corresponding to each
	# item of the game
	# adds the pieces from the board
	for i in range(len(board)):
		binary_representation = bin(board[i])
		index = 0
		while (index != -1):
			index = binary_representation.find('1', index + 1)
			if index != -1:
				initial_hash ^= zobrist_numbers[64 * i + len(binary_representation) - index - 1]

	# adds the castling legality
	initial_hash ^= zobrist_numbers[64 * 12 + castling]

	# adds en-passant
	# checks if the last move was a double pawn move
	if last_move[-1] ^ 15 == 1:
		last_move_file = game_mech.file(last_move[1])
		
		# checks if there is a pawn on the squares next to the last move played
		if ((last_move[1] >> 1) & board[6 * to_play] != 0 and last_move_file > 0) or \
			((last_move[1] << 1) & board[6 * to_play] != 0 and last_move_file < 7):
			initial_hash ^= zobrist_numbers[64 * 12 + 16 + last_move_file]

	# adds person to move
	if to_play == 1:
		initial_hash ^= zobrist_numbers[64 * 12 + 16 + 8]

	past_hash_list = (c_int * 100)()
	past_hash_list[0] = initial_hash

	return initial_hash, zobrist_numbers, past_hash_list


def perft(board, last_move, castling, piece_list, to_play, initial_hash, past_hash_list, ply_counter, depth, type='all'):
	c_board = (c_ulonglong * 12)(*board)
	c_last_move = (c_ulonglong * 3)(*last_move)
	c_castling = c_int(castling)
	c_to_play = c_int(to_play)
	c_initial_hash = c_int(initial_hash)
	c_past_hash_list = (c_int * 100)(*past_hash_list)
	c_move_counter = c_int(ply_counter)
	c_depth = c_int(depth)
	answer_type = ['Nodes', 'Captures', 'En passant', 'Castling', 'Promotion', 'Checks', 'Checkmates']
	answer_dict = {}

	# perft_all is used for testing the rules of the game, and returns statistics for various different things, 
	# while perft_nodes is used for speed benchmarking and only counts nodes.
	if type == 'all':
		answers = (c_int * 7)()
		perft_all(c_board, c_last_move, c_castling, piece_list, c_to_play, c_initial_hash, c_past_hash_list, c_move_counter, c_depth, answers)
	else:
		answers = (c_int * 1)()
		perft_nodes(c_board, c_last_move, c_castling, piece_list, c_to_play, c_initial_hash, c_past_hash_list, c_move_counter, c_depth, answers)

	for i in range(len(answers)):
		answer_dict[answer_type[i]] = answers[i]

	return answer_dict
