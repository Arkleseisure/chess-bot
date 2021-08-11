from ctypes import CDLL, c_ulonglong, c_int, c_char, Structure, c_bool
import sys
import Bits_and_pieces as BaP

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
	# extras is created so that the object passed is mutable, making it easier to return stuff
	extras = (c_int * 2)(*[castling, to_play])


	# applies the move with the new types
	game_mech.apply(c_board, c_move, extras, c_last_move, piece_list)
	extras = extras[:]
	castling = extras[0]
	to_play = extras[1]

	# returns the c arrays as python lists ([:] will automatically cast an iterable into a list)
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

Last Modified: 11/8/2021
Last Modified by: Arkleseisure
'''
def legal_moves(board, castling, to_play, last_move, piece_list):
	# casts the variables into equivalent types in c
	c_board = (c_ulonglong * 12)(*board)
	c_last_move = (c_ulonglong * 3)(*last_move)
	c_to_play = c_int(to_play)
	c_castling = c_int(castling)
	# I think the maximum possible number of legal moves from a single position is about this (https://lichess.org/analysis/fromPosition/R6R/3Q4/1Q4Q1/4Q3/2Q4Q/Q4Q2/pp1Q4/kBNN1KB1_w_-_-)
	c_legal_moves = ((c_ulonglong * 3) * 220)()

	num_moves = game_mech.legal_moves(c_board, c_legal_moves, c_castling, c_to_play, c_last_move, piece_list)
	return c_legal_moves[:num_moves]

# initiates the piece list, an array containing a struct for each piece
def initiate_piece_list():
	piece_list = (Piece * 32)()

	# adds the pawns to the piece list
	for i in range(8):
		white_pawn = Piece()
		white_pawn.loc = 2 ** (i + 8)
		white_pawn.type = 0
		piece_list[i] = white_pawn

	# adds the remaining white pieces to the piece_list
	rook = Piece()
	rook.loc = 2 ** 0
	rook.type = 3
	piece_list[8] = rook
	knight = Piece()
	knight.loc = 2 ** 1
	knight.type = 1
	piece_list[9] = knight
	bishop = Piece()
	bishop.loc = 2 ** 2
	bishop.type = 2
	piece_list[10] = bishop
	queen = Piece()
	queen.loc = 2 ** 3
	queen.type = 4
	piece_list[11] = queen
	king = Piece()
	king.loc = 2 ** 4
	king.type = 5
	piece_list[12] = king
	bishop = Piece()
	bishop.loc = 2 ** 5
	bishop.type = 2
	piece_list[13] = bishop
	knight = Piece()
	knight.loc = 2 ** 6
	knight.type = 1
	piece_list[14] = knight
	rook = Piece()
	rook.loc = 2 ** 7
	rook.type = 3
	piece_list[15] = rook


	for i in range(16):
		black_piece = Piece()
		if i < 8:
			black_piece.loc = piece_list[i].loc * (2 ** 40)
		else:
			black_piece.loc = piece_list[i].loc * (2 ** 56)
		black_piece.type = piece_list[i].type + 6
		piece_list[16 + i] = black_piece


	for piece in piece_list:
		piece.captured = False

	return piece_list
