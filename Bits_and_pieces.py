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

def print_move(move):
    print(convert_to_text(move[0]) + convert_to_text(move[1]))


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

'''
Takes in a square in the form 'a1', 'g4', ... and returns the relevant bitboard
Last Modified: 20/2/2022
Last Modified by: Arkleseisure
'''
def get_bitboard_from_square(squ):
    x = ord(squ[0]) - 97
    y = int(squ[1]) - 1

    return 2**(x + 8 * y)

'''
Takes in a move in the form 'a1a2', 'h5f7', ... and returns the relevant bitboard move, as detailed in play_game
Last Modified: 20/2/2022
Last Modified by: Arkleseisure
'''
def convert_text_to_bitboard_move(move, game):
    # first two bitboards detail the locations that the piece moves from and to.
    new_move = [get_bitboard_from_square(move[:2]), get_bitboard_from_square(move[2:4])]
    flag = 0
    piece_captured = False
    for i in range(len(game.piece_list)):
        if (game.piece_list[i].loc & new_move[0]) != 0 and not game.piece_list[i].captured:
            piece = game.piece_list[i]
            # the flag has 15 important bits: first 7 are the index of the piece in the piece list
            # then there are 4 detailing the type of piece
            # the final 4 detail the type of move
            flag += i << 8
            flag += piece.type << 4

            # detecting special moves to add to the flag
            # castling
            if (piece.type % 6) == 5:
                # kingside
                if new_move[1] == (new_move[0] << 2):
                    flag += 2
                # queenside
                elif new_move[1] == (new_move[0] >> 2):
                    flag += 3
            # pawn stuff
            elif (piece.type % 6) == 0:
                # double pawn push
                if abs(int(move[3]) - int(move[1])) == 2:
                    flag += 1
                # promotion
                elif len(move) == 5:
                    flag += 8
                    if move[-1] == 'B':
                        flag += 1
                    elif move[-1] == 'R':
                        flag += 2
                    elif move[-1] == 'Q':
                        flag += 3
                # en passant
                # first checks whether the previous move was a pawn move
                elif (game.last_move[1] & (game.board[0] | game.board[6])) != 0:
                    # then checks whether the move ends up immediately behind the previous pawn move, only possible in en-passant
                    if (game.last_move[1] << 8 == new_move[1]) and game.to_play == 0:
                        flag += 5
                    elif (game.last_move[1] >> 8 == new_move[1]) and game.to_play == 1:
                        flag += 5
        elif (game.piece_list[i].loc & new_move[1]) != 0:
            piece_captured = True

    if piece_captured:
        flag += 4
    
    new_move.append(flag)
    return new_move

# function to print out the individual bitboards, mainly for debuggin purposes
# Last Modified: 11/8/2021
# Last Modified by: Arkleseisure
def print_board(board):
    for i in range(len(board)):
        for j in range(8):
            print(bin(board[i] + 2 ** 64)[8 * (j + 1) + 2: 8 * j + 2: -1])
        print()
