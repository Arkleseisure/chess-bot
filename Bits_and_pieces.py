import math

# function to convert an input bitboard to a readable square in text such as 'a1' or 'f3'
def convert_to_text(pos):
	# finds the square number of the position (see explanation in play_game)
	try:
		square_number = int(math.log2(pos))
	except ValueError:
		print(pos)
	
	# finds the row and column of the position
	col = square_number % 8
	row = square_number // 8

	# returns the letter for the column and the number of the row as a string
	return chr(col + 97) + str(row + 1)


# takes in the board part of the fen and returns the bitboard for a specific piece type
def get_bitboard_from_fen(fen, piece):
    square_number = 56
    bitboard = 0
    for item in fen:
        if item == '/':
            square_number -= 16
        elif item == piece:
            bitboard += 2 ** square_number
            square_number += 1
        elif item.isdigit():
            square_number += int(item)
        else:
            square_number += 1
    return bitboard

'''
Takes a fen string and changes it such that it represents the same position, but with switched colours
More info on fen strings here: https://en.wikipedia.org/wiki/Forsyth%E2%80%93Edwards_Notation

Last Modified: 3/9/2021
Last Modified by: Arkleseisure
'''
def switch_fen_colours(fen):
    fen_list = fen.split()
    new_fen = ''

    # flips the board's perspective by reversing the board string
    fen_list[0] = fen_list[0][::-1]

    # flips the colours
    for i in range(len(fen_list[0])):
        if fen_list[0][i].isupper():
            new_fen += fen_list[0][i].lower()
        elif fen_list[0][i].islower():
            new_fen += fen_list[0][i].upper()
        else:
            new_fen += fen_list[0][i]
    
    # switches the player to move
    new_fen += ' b' if fen_list[1] == 'w' else ' w'

    # switches castling rights
    if fen_list[2] == '-':
        new_fen += ' -'
    else:
        white_castling = ''
        black_castling = ''
        for item in fen_list[2]:
            if item.isupper():
                black_castling += item.lower()
            elif item.islower():
                white_castling += item.upper()
        new_fen += ' ' + white_castling + black_castling


    # switches en passant legality
    if fen_list[3] == '-':
        new_fen += ' -'
    else:
        new_fen += ' ' + fen_list[3][0] + str(9 - int(fen_list[3][1]))

    # adds the ply counter for the 50 move rule and the move number of the game
    new_fen += ' ' + fen_list[4] + ' ' + fen_list[5]

    return new_fen
