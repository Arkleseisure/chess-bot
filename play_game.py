import pygame
from draw_board import initialize_pygame_stuff, draw_board, get_square
from c_interface import apply

# Plays through a full game of chess.
# Last Modified: 29/06/2021
def play_game(colour, ai):
    # Initializes game
    background, buttons, board, last_move, castling = initialize_game()
    game_over = False
    to_play = 0

    # Main game loop
    while not game_over:
        # Draws the board
        draw_board(colour, background, board, last_move)

        # Gets legal move from player or computer
        moves = legal_moves(board, last_move, castling, to_play)
        move_is_legal = False
        while not move_is_legal:
            if ai == 0 or colour == to_play:
                move = get_human_move(colour)
            else:
                move = get_ai_move(ai, board, last_move, moves)

            if move in legal_moves:
                move_is_legal = True

        # Applies the move
        apply(board, move, castling, last_move, to_play)

        # Checks if the game has ended
        result = terminal(board, last_move)
        game_over = (result != 3)

    draw_result(result)


# initializes items required for playing the game.
# Last Modified: 01/07/2021
def initialize_game(colour):
    '''
    Creates bitboards for each piece type... The positions of the pieces are each represented by a single unsigned 8 byte integer
    which in binary would spell out the board were we to remove any other piece type, e.g white pawns in the starting position are represented by:
    00000000
    00000000
    00000000
    00000000
    00000000
    00000000
    11111111
    00000000

    It is generally easier to use hex
    '''

    white_pawns     = 0x000000000000FF00
    white_knights   = 0x0000000000000042
    white_bishops   = 0x0000000000000024
    white_rooks     = 0x0000000000000081
    white_queens    = 0x0000000000000008
    white_king      = 0x0000000000000010
    
    black_pawns     = 0x00FF000000000000
    black_knights   = 0x4200000000000000
    black_bishops   = 0x2400000000000000
    black_rooks     = 0x8100000000000000
    black_queens    = 0x0800000000000000
    black_king      = 0x1000000000000000

    board = [white_pawns, white_knights, white_bishops, white_rooks, white_queens, white_king, 
             black_pawns, black_knights, black_bishops, black_rooks, black_queens, black_king]

    # initializes the pygame sprite group used to display the background (i.e the board and anything around it)
    background, buttons = initialize_pygame_stuff(colour)

    '''
    Moves are represented by two bitboards and a flag. One bitboard represents the original location of the piece, 
    one the final location of the piece and the flag represents special moves:
    0 = nothing special
    1 = double pawn push
    2 = castle kingside
    3 = castle queenside
    4 = capture
    5 = ep capture
    8 = knight promotion
    9 = bishop promotion
    10 = rook promotion
    11 = queen promotion
    12 = knight promotion with capture
    13 = bishop promotion with capture
    14 = rook promotion with capture
    15 = queen promotion with capture

    Added to the front of this are 3 bits referring to the piece moved:
    0 = pawn
    1 = knight
    2 = bishop
    3 = rook
    4 = queen
    5 = king

    This also helps make moves easier to reverse, as all the information about the move is easily retrievable.
    '''
    last_from = 0x0000000000000000
    last_to = 0x0000000000000000
    last_flag = 0x00
    last_move = [last_from, last_to, last_flag]

    # legality of castling, represented by a four bit number 
    castling = 0xF

    return background, buttons, board, last_move, castling


# function used until all the functions in play_game are coded, allows us to test what we've done so far
# Last Modified: 04/07/2021
def do_test_stuff(colour, ai):
    background, buttons, board, last_move, castling = initialize_game(colour)
    to_play = 0

    quit = False
    square_selected = False
    while not quit:
        draw_board(colour, background, buttons, board, last_move, current_move=0)
        if not square_selected:
            move_from, button_clicked = get_square(colour, buttons)

        if button_clicked == 'Exit':
            quit = True
        else:
            draw_board(colour, background, buttons, board, last_move, move_from)
            move_to, button_clicked = get_square(colour, buttons)

            # Figures out which piece has been moved
            move_from_no = len(bin(move_from)) - 2
            move_to_no = len(bin(move_to)) - 2
            flag = -1
            for i in range(len(board)):
                bit_board = bin(board[i])
                if len(bit_board) >= move_from_no and bit_board[-move_from_no] == '1':
                    flag = i * 16

                # removes moves where they take their own piece
                if len(bit_board) >= move_to_no and bit_board[-move_to_no] == '1' and (i > 5) == (to_play == 1):
                    flag = -2
                    break

            # exits if the exit button is clicked, otherwise applies the move to the board
            if button_clicked == 'Exit':
                quit = True
            elif (0 <= flag < 6 * 16 and to_play == 0) or (6 * 16 <= flag < 12 * 16 and to_play == 1):
                board, [castling, to_play], last_move = apply(board, [move_from, move_to, flag], castling, to_play)
                square_selected = False
            else:
                move_from = move_to
                square_selected = True
