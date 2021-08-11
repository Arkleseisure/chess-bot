import pygame
from draw_board import initialize_pygame_stuff, draw_board, get_square, print_board
from c_interface import apply, initiate_piece_list, legal_moves
import Bits_and_pieces as BaP

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
# Last Modified: 11/8/2021
# Last Modified by: Arkleseisure
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

    This means that the value added by each square is equal to 2 ^ square number, where the square number is given by 
    this grid (as viewed from white's perspective):
    56 57 58 59 60 61 62 63
    48 49 50 51 52 53 54 55
    40 41 42 43 44 45 46 47
    32 33 34 35 36 37 38 39
    24 25 26 27 28 29 30 31
    16 17 18 19 20 21 22 23
    8  9  10 11 12 13 14 15
    0  1  2  3  4  5  6  7

    This can be calculated as 8 * row + file and hence the bitboard for each piece can be easily calculated from its coordinates on the board.
    At the time of writing I haven't used this yet, but I may and you may find it useful.
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

    Added to the front of this are 4 bits referring to the piece moved:
    0 - 5 = white, 6 - 11 = black
    0 = pawn
    1 = knight
    2 = bishop
    3 = rook
    4 = queen
    5 = king

    In front of this are 7 bits referring to the index of the piece in the piece list.

    This also helps make moves easier to reverse, as all the information about the move is easily retrievable.
    '''
    last_from = 0x0000000000000000
    last_to = 0x0000000000000000
    last_flag = 0x00
    last_move = [last_from, last_to, last_flag]

    # legality of castling, represented by a four bit number 
    castling = 0xF

    piece_list = initiate_piece_list()

    return background, buttons, board, last_move, castling, piece_list


# function used until all the functions in play_game are coded, allows us to test what we've done so far
# Last Modified: 04/07/2021
def do_test_stuff(colour, ai):
    background, buttons, board, last_move, castling, piece_list = initialize_game(colour)
    to_play = 0

    quit = False
    square_selected = False
    while not quit:
        draw_board(colour, background, buttons, board, last_move, current_move=0)

        print_board(board)

        print('printing legal moves')
        poss_moves = legal_moves(board, castling, to_play, last_move, piece_list)
        # prints the legal moves as [starting square][finishing square] [flag] [piece type] [piece index]
        for move in poss_moves:
            print(BaP.convert_to_text(move[0]) + BaP.convert_to_text(move[1]) + ' ' + str(move[2] % 16) + ' ' + str((move[2] % 256) // 16) + ' ' + str(move[2] // 256))

        if not square_selected:
            move_from, button_clicked = get_square(colour, buttons)

        if button_clicked == 'Exit':
            quit = True
        else:
            draw_board(colour, background, buttons, board, last_move, move_from)
            move_to, button_clicked = get_square(colour, buttons)

            # checks if the move is legal
            move = [0,0,0]
            for i in range(len(poss_moves)):
                if poss_moves[i][0] == move_from and poss_moves[i][1] == move_to:
                    move = poss_moves[i]

            # exits if the exit button is clicked, otherwise applies the move to the board
            if button_clicked == 'Exit':
                quit = True
            elif move != [0, 0, 0]:
                board, castling, to_play, last_move = apply(board, move, castling, to_play, piece_list)
                square_selected = False
            else:
                move_from = move_to
                square_selected = True

