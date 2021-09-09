from ctypes import CDLL, c_ulonglong, c_int, c_char, Structure, c_bool
import sys
import random
import time
import Bits_and_pieces as BaP
from draw_board import print_board


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
def apply(board, move, castling, to_play, piece_list, past_hash_list, current_hash, ply_counter, zobrist_numbers):
	# casts the python variables into the equivalent types in c
	c_board = (c_ulonglong * 12)(*board)
	c_move = (c_ulonglong * 3)(*move)
	c_last_move = (c_ulonglong * 3)()
	# extras is created so that the object passed is mutable, making it easier to
	# return stuff
	extras = (c_int * 3)(*[castling, to_play, ply_counter])
	c_zobrist_numbers = (c_ulonglong * 793)(*zobrist_numbers)
	c_hash = (c_ulonglong * 1)(*[current_hash])
	c_removed_hash = (c_ulonglong * 1)()

	piece_taken = game_mech.apply(c_board, c_move, extras, c_hash, c_last_move, piece_list, past_hash_list, c_zobrist_numbers, c_removed_hash)
	extras = extras[:]
	castling = extras[0]
	to_play = extras[1]
	ply_counter = extras[2]
	hash = c_hash[0]

	# returns the c arrays as python lists ([:] will automatically cast an
	# iterable into a list)
	return c_board[:], castling, to_play, c_last_move[:], hash, ply_counter

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
	piece_types = {0: 'white pawn', 1: 'white knight', 2: 'white bishop', 3: 'white rook', 4: 'white queen', 5: 'white king', 
			   6: 'black pawn', 7: 'black knight', 8: 'black bishop', 9: 'black rook', 10: 'black queen', 11: 'black king'}
	piece_list = (Piece * 32)()
	piece_num = 0
	try:
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
				for j in range(15 - (piece_num % 16)):
					new_piece = Piece()
					new_piece.loc = 0
					new_piece.type = 0
					new_piece.captured = True
					piece_list[piece_num] = new_piece
					piece_num += 1
			# does the same for the rooks, as this is used for quick location of the rooks when castling
			elif i % 6 == 2 and piece_num % 16 != 12:
				for j in range(12 - (piece_num % 16)):
					new_piece = Piece()
					new_piece.loc = 0
					new_piece.type = 0
					new_piece.captured = True
					piece_list[piece_num] = new_piece
					piece_num += 1
	except IndexError:
		print(piece_num)
		print_board(board)

	return piece_list

'''
Generates pseudorandom numbers used for zobrist hashes (an efficient way to hash a board position), 
as well as the hash of the initial position and the list of past hashes, with the initial position included
More info on Zobrist Hashing here: https://www.chessprogramming.org/Zobrist_Hashing

Last Modified: 19/8/2021
Last Modified by: Arkleseisure
'''
def generate_zobrist_stuff(board, castling, to_play, last_move):
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

	past_hash_list = (c_ulonglong * 100)()
	past_hash_list[0] = c_ulonglong(initial_hash)

	return initial_hash, zobrist_numbers, past_hash_list


def perft(board, last_move, castling, piece_list, to_play, initial_hash, past_hash_list, ply_counter, zobrist_numbers, depth,  type='all'):
	c_board = (c_ulonglong * 12)(*board)
	c_last_move = (c_ulonglong * 3)(*last_move)
	c_castling = c_int(castling)
	c_to_play = c_int(to_play)
	c_initial_hash = c_ulonglong(initial_hash)
	c_ply_counter = c_int(ply_counter)
	c_depth = c_int(depth)
	c_zobrist_numbers = (c_ulonglong * 793)(*zobrist_numbers)
	c_past_hash_list = (c_ulonglong * 100)(*past_hash_list)
	answer_type = ['Nodes', 'Captures', 'En passant', 'Castling', 'Promotion', 'Checks', 'Checkmates']
	answer_dict = {}

	# perft_all is used for testing the rules of the game, and returns statistics for various different things, 
	# while perft_nodes is used for speed benchmarking and only counts nodes.
	if type == 'all':
		moves = legal_moves(board, castling, to_play, last_move, piece_list)
		for i in range(len(moves)):
			print(BaP.convert_to_text(moves[i][0]) + BaP.convert_to_text(moves[i][1]))
		for i in range(len(piece_list)):
			if not piece_list[i].captured:
				print(piece_list[i].type)
				print_board([piece_list[i].loc])

		answers = (c_int * 7)()
		game_mech.perft_all(c_board, c_last_move, c_past_hash_list, c_castling, piece_list, c_to_play, c_initial_hash, c_ply_counter, c_depth, answers, c_zobrist_numbers)
	else:
		answers = (c_int * 1)()
		game_mech.perft_nodes(c_board, c_last_move, c_castling, piece_list, c_to_play, c_initial_hash, c_past_hash_list, c_ply_counter, c_depth, answers, c_zobrist_numbers)

	for i in range(len(answers)):
		answer_dict[answer_type[i]] = answers[i]

	return answer_dict


def terminal(board, to_play, piece_list, ply_counter, past_hash_list, current_hash, last_move):
	c_board = (c_ulonglong * 12)(*board)
	c_to_play = c_int(to_play)
	c_ply_counter = c_int(ply_counter)
	c_hash = c_ulonglong(current_hash)
	c_last_move = (c_ulonglong * 3)(*last_move)

	return int(game_mech.terminal(c_board, c_to_play, piece_list, c_ply_counter, past_hash_list, c_hash, c_last_move))
