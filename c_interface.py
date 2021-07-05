from ctypes import CDLL, c_ulonglong, c_uint, c_char

# imports the c libraries
game_mechanics_lib_path = 'theories/game_mechanics.so'
game_mech = CDLL(game_mechanics_lib_path)
game_mech.confirm_it_works()

# interface for the apply function
# Last Modified: 04/07/2021
def apply(board, move, castling, to_play):
	c_board = (c_ulonglong * 12)(*board)
	c_move = (c_ulonglong * 3)(*move)
	extras = (c_uint * 2)(*[castling, to_play])
	c_last_move = (c_ulonglong * 3)()

	game_mech.apply(c_board, c_move, extras, c_last_move)

	return c_board[:], extras[:], c_last_move[:]

