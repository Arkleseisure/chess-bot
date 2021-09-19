import pygame
import random
import time
import numpy as np
import math
from draw_board import *
from c_interface import *
from Bits_and_pieces import convert_to_text, switch_fen_colours

initial_pos_fen = 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1'

'''
Plays through a full game of chess.
Last Modified: 17/9/2021
Last Modified by: Arkleseisure
'''
def play_game(colour, other_player):
    # Initializes game
    background, buttons, game, zobrist_numbers = initialize_game()
    game_over = False
    exit = False

    # Main game loop
    while not game_over:
        # Draws the board
        draw_board(colour, background, buttons, game.board, game.last_move, current_move=0)

        # Gets legal move from player or computer
        moves = legal_moves(game)
        if other_player == 'human' or (other_player == 'ai' and game.to_play == colour):
            move, exit = get_human_move(game, background, buttons, colour, moves)
        else:
            move, value = get_engine_move(game, zobrist_numbers, 10)

        if exit:
            return

        # Applies the move
        game = apply(game, move, zobrist_numbers)

        # Checks if the game has ended
        result = terminal(game)
        game_over = (result != 3)

    # draws the result then waits for the user to click to exit
    draw_result(result)
    get_square(colour, buttons)

'''
Gets an input human move from clicks on the board
Last Modified: 17/9/2021
Last Modified by: Arkleseisure
'''
def get_human_move(game, background, buttons, colour, legal_moves):
    # After the engine has moved, the pygame window hasn't received anything for a while and says the program isn't responding. 
    # This returns it to its original state.
    for event in pygame.event.get():
        pass

    # draws the board to the screen.
    draw_board(colour, background, buttons, game.board, game.last_move, current_move=0)

    # gets the square/button the human has clicked on
    move_from, button_clicked = get_square(colour, buttons)

    # if the human has clicked on the exit button, this exits the program.
    if button_clicked == 'Exit':
        return [0, 0, 0], True 

    # loops until the human has entered a valid move, at which point the program is quit.
    while True:
        # draws the board
        draw_board(colour, background, buttons, game.board, game.last_move, current_move=move_from)

        # gets the square/button the human has clicked on
        move_to, button_clicked = get_square(colour, buttons)

        # if the button is the exit button, it quits the program
        if button_clicked == 'Exit':
            return [0, 0, 0], True 

        # loops through the legal moves to see if the move entered is legal
        for move in legal_moves:
            # checks if the move is legal
            if move[0] == move_from and move[1] == move_to:
                # if the move is not a promotion, then the starting and end squares uniquely define the move
                if move[2] & 8 == 0:
                    return move, False
                # handles pawn promotion by drawing the potential promotion pieces
                else:
                    promotion_pieces = ['N', 'B', 'R', 'Q']
                    # gives the square number of the square to draw the queen to
                    loc = math.log2(move[1])

                    # finds the positions the pieces need to be drawn to
                    if game.to_play == 0:
                        locs = [loc - 24, loc - 16, loc - 8, loc]
                    else:
                        locs = [loc + 24, loc + 16, loc + 8, loc]

                    # gets the colour of the pieces drawn
                    if game.to_play == 0:
                        piece_col = 'w'
                    else:
                        piece_col = 'b'

                    # draws the pieces so the user can choose from them
                    for i in range(4):
                        draw_piece(promotion_pieces[i] + piece_col, locs[i], colour)

                    # updates the screen
                    pygame.display.flip()

                    # gets the piece the user wants to promote to
                    promotion_piece, button_clicked = get_square(colour, buttons)

                    # if the user tries to exit, then this exits
                    if button_clicked == 'Exit':
                        return [0, 0, 0], True
                    
                    # returns the move with the promotion chosen.
                    for i in range(4):
                        if math.log2(promotion_piece) == locs[i]:
                            return [move[0], move[1], move[2] - (move[2] & 3) + i], False
                    
                    # if the move is invalid, the second square clicked on will become the first square of the next move
                    move_to = promotion_piece
        
        # sets the first part of the move to the second part of the last move as the move is invalid.
        move_from = move_to

# initializes items required for playing the game.
# Last Modified: 15/9/2021
# Last Modified by: Arkleseisure
def initialize_game(fen=initial_pos_fen):
    fen_list = fen.split()

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

    The bitboards are ordered like this:
    0 is white pawns
    1 is white knights
    2 is white bishops
    3 is white rooks
    4 is white queens
    5 is the white king
    Then 6 - 11 are in the same order but black

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
    game = Game()

    get_board(game, fen_list[0])

    
    # The piece_list is an array of 32 piece structures, each containing 3 variables: 
    # pos: position as a bitboard
    # type: number from 0-11 referring to the type of piece as explained above
    # captured: boolean saying whether the piece has been captured yet or not
    initiate_piece_list(game)

    # player to move, 0 if white to play, 1 if black to play
    game.to_play = 0 if fen_list[1] == 'w' else 1

    # initializes the pygame sprite group used to display the background (i.e
    # the board and anything around it)
    background, buttons = initialize_pygame_stuff(game.to_play)
    
    # legality of castling, represented by a four bit number,
    # kingside/queenside for black, then kingside/queenside for white
    game.castling = 0
    if 'Q' in fen_list[2]:
        game.castling += 1
    if 'K' in fen_list[2]:
        game.castling += 2
    if 'q' in fen_list[2]:
        game.castling += 4
    if 'k' in fen_list[2]:
        game.castling += 8

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
    get_last_move(game, fen_list[3])
    
    # initiates the zobrist hashes and zobrist numbers, used to speed up
    # indexing of positions, more info here:
    # https://www.chessprogramming.org/Zobrist_Hashing
    zobrist_numbers = generate_zobrist_stuff(game)

    # half move counter, used to find draws by 50 move rule
    game.ply_counter = int(fen_list[4])

    return background, buttons, game, zobrist_numbers

'''
Function used to test the speed and functionality of the rules. 
Mainly uses a perft function: https://www.chessprogramming.org/Perft
Answers are taken from this article: https://www.chessprogramming.org/Perft_Results
Perft searches are done to depth 4 to save time
The 5 positions looked at are the initial position, kiwipete (position specifically crafted for perft functions), 
a rook endgame position to test the rules closer to the end of the game, and then two positions which are mirrored to check for asymmetry
between the rules for black and white

Last Modified: 30/8/2021
Last Modified by: Arkleseisure
'''
def test_game_mechanics():
    # declares the fens of the positions, as well as the answers which correctly coded rules should give.
    fen_list = [initial_pos_fen, 
                'r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1', 
                '8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 0', 
                'r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1', 
                'r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1']
    fen_names = ['Initial position', 'Kiwipete', 'Rook endgame', 'White to play', 'Black to play']
    answers = {'Nodes': [[20, 400, 8902, 197281, 4865609, 119060324, 3195901860, 84998978956],
                  [48, 2039, 97862, 4085603, 193690690], #, 8031647685],
                  [14, 191, 2812, 43238, 674624, 11030083],
                  [6, 264, 9467, 422333, 15833292, 706045033],
                  [6, 264, 9467, 422333, 15833292, 706045033]],
              'Captures': [[0, 0, 34, 1576, 82719, 2812008, 108329926, 3523740106],
                  [8, 351, 17102, 757163],
                  [1, 14, 209, 3348],
                  [0, 87, 1021, 131393],
                  [0, 87, 1021, 131393]],
              'En passant': [[0, 0, 0, 0, 258, 5248, 319617, 7187977],
                  [0, 1, 45, 1929],
                  [0, 0, 2, 123],
                  [0, 0, 4, 0],
                  [0, 0, 4, 0]],
               'Castling': [[0, 0, 0, 0, 0, 0, 883453, 23605205],
                  [2, 91, 3162, 128013],
                  [0, 0, 0, 0],
                  [0, 6, 0, 7795],
                  [0, 6, 0, 7795]],
               'Promotion': [[0, 0, 0, 0, 0, 0, 0, 0],
                  [0, 0, 0, 15172],
                  [0, 0, 0, 0],
                  [0, 48, 120, 60032],
                  [0, 48, 120, 60032]],
               'Checks': [[0, 0, 12, 469, 27351, 809099, 33103848, 968981593],
                  [0, 3, 993, 25523],
                  [2, 10, 267, 1680],
                  [0, 10, 38, 15492],
                  [0, 10, 38, 15492]],
               'Checkmates': [[0, 0, 0, 8, 347, 10828, 435767, 9852036],
                  [0, 0, 1, 43],
                  [0, 0, 0, 17],
                  [0, 0, 22, 5],
                  [0, 0, 22, 5]]
    }

    all_tests_passed = True
    for i in range(len(fen_list)):
        background, buttons, game, zobrist_numbers = initialize_game(fen_list[i])
        draw_board(game.to_play, background, buttons, game.board, game.last_move, current_move=0)
        print(fen_names[i])
        passed = True
        for j in range(4):
            print('Depth:', j + 1)
            answer_dict, time_taken = perft(game, zobrist_numbers, depth=j + 1, type='all')

            for key in answer_dict:
                print(key)
                if answers[key][i][j] != answer_dict[key]:
                    passed = False
                print('Answer given:', answer_dict[key], 'Actual answer:', answers[key][i][j])
            print('Time taken:', round(time_taken, 5))
            try:
                print('Speed:', round(answer_dict['Nodes']/time_taken), 'nodes per second')
            except ZeroDivisionError:
                print('Speed: Too fast to tell')
            print()

        if passed:
            print('Passed!')
        else:
            all_tests_passed = False
        print()

    print(all_tests_passed)
    # tests the draws, which the perft function doesn't seem to do quite so well.
    draws_work = test_draws()
    if draws_work:
        print('Draw tests succeeded.')
    else:
        print('Draw tests failed.')
        all_tests_passed = False

    if all_tests_passed:
        print('All the tests were passed! You have a working game of chess!')

'''
Checks that the draw legality works properly, as this isn't properly tested by the perft function
Positions used are: 
Initial position

Stalemate:
Stalemate with king and queen vs king
More complex stalemate position

Position with draw by 50 move rule

Draw by lack of material:
Position with king vs king (draw)
Position with king and bishop vs king (draw)
Position with king and knight vs king (draw)
Position with king and 2 bishops vs king (not terminal)
Position with king and pawn vs king (not terminal)
Position with king and 2 knights vs king (not terminal as there are techincally mates which can occur, although these cannot be forced)
Position with king, bishop, knight vs king (not terminal)
Position with king and 3 same colored bishops vs king (draw, as it is impossible to mate with any number of bishops of the same colour)
Position with king knight vs king knight (not terminal as mating positions exist)
Position with king bishop vs king knight (not terminal, as mating positions exist)
Position with king bishop vs king opposite coloured bishop (not terminal as mating positions exist)
Position with king bishop vs king same coloured bishop (draw as no mating positions exist)
Position with king rook vs king (not terminal)
Position with king queen vs king (not terminal)

Draw by repetition:
Initial position is used and then repeated 3 times by moving knights back and forth.

Draw by 50 move rule (follow up):
Position with King, Bishop and Knight vs King is used, and then 100 random moves are made (50 for each side).
The position is then tested for a draw
This is repeated 5 times to ensure that the any fluke captures or checkmates are eliminated.

Last Modified: 2/9/2021
Last Modified by: Arkleseisure
'''
def test_draws():
    draws_work = True
    fen_list = [initial_pos_fen, 
                '7k/5Q2/6K1/8/8/8/8/8 b - - 0 1', 
                '8/2R5/Q4pk1/8/3P2P1/5P2/2PB2P1/6K1 b - - 0 1', 
                '8/2R5/Q4pk1/8/3P2P1/5P2/2PR2P1/6K1 w - - 100 60', 
                '8/8/3k4/8/3K4/8/8/8 w - - 0 1',
                '8/8/3k4/5B2/3K4/8/8/8 w - - 0 1',
                '8/8/3k4/5N2/3K4/8/8/8 b - - 0 1',
                '8/8/3k4/5BB1/3K4/8/8/8 b - - 0 1',
                '8/8/3k4/5P2/3K4/8/8/8 b - - 0 1',
                '8/8/3k4/6N1/3K1N2/8/8/8 b - - 0 1',
                '7k/6N1/5BK1/8/8/8/8/8 b - - 0 1',
                '5k2/8/5BK1/6B1/5B2/8/8/8 b - - 0 1',
                '4nk2/8/6K1/4N3/8/8/8/8 b - - 0 1',
                '4nk2/8/6K1/4B3/8/8/8/8 b - - 0 1',
                '5kb1/8/6K1/4B3/8/8/8/8 b - - 0 1',
                '3b1k2/8/6K1/4B3/8/8/8/8 b - - 0 1',
                '1k6/3R4/8/8/4K3/8/8/8 w - - 0 1',
                '1k6/3Q4/8/8/4K3/8/8/8 w - - 0 1']

    fen_names = ['Initial position',
                 'Stalemate', 
                 'More interesting stalemate', 
                 'Draw by 50 move rule', 
                 'King vs King',
                 'King, Bishop vs King',
                 'King, Knight vs King', 
                 'King, 2 Bishops vs King',
                 'King, Pawn vs King', 
                 'King, 2 Knights vs King',
                 'King, Bishop, Knight vs King', 
                 'King, 3 same colored bishops vs King', 
                 'King, Knight vs King, Knight',
                 'King, Bishop vs King, Knight', 
                 'King, Bishop vs King, opposite coloured Bishop',
                 'King, Bishop vs King, same coloured Bishop',
                 'King, Rook vs King', 
                 'King, Queen vs King']

    # terminal() returns 3 if the game isn't terminal, 1 if it's a draw and 0/2 for wins for black/white
    answer_list = [3, 1, 1, 1, 1, 1, 1, 3, 3, 3, 3, 1, 3, 3, 3, 1, 3, 3]

    for i in range(len(fen_list)):
        background, buttons, game, zobrist_numbers = initialize_game(fen_list[i])
        result = terminal(game)
        if result != answer_list[i]:
            print(fen_names[i], 'for white failed.')
            print('Expected outcome:', answer_list[i], 'Actual outcome:', result)
            draws_work = False

        # verifies that the code works equally well for both colours
        background, buttons, game, zobrist_numbers = initialize_game(switch_fen_colours(fen_list[i]))
        result = terminal(game)
        if result != answer_list[i]:
            print(fen_names[i], 'for black failed')
            print('Expected outcome:', answer_list[i], 'Actual outcome:', result)
            draws_work = False


    # tests draw by repetition by moving the knights to and from the starting position multiple times.
    fen = initial_pos_fen
    background, buttons, game, zobrist_numbers = initialize_game(fen)
    for i in range(2):
        moves = legal_moves(game)
        for move in moves:
            # the move starts on the starting square of the knight on b1
            if move[0] == 2:
                game = apply(game, move, zobrist_numbers)
                break

        moves = legal_moves(game)
        for move in moves:
            # the move starts on the starting square of the knight on b8
            if move[0] == 2**57:
                game = apply(game, move, zobrist_numbers)
                break

        moves = legal_moves(game)
        for move in moves:
            # moves the knight back to b1
            if (move[0] == 2 ** 16 or move[0] == 2 ** 18) and move[1] == 2:
                game = apply(game, move, zobrist_numbers)
                break

        moves = legal_moves(game)
        for move in moves:
            # moves the knight back to b8
            if (move[0] == 2 ** 40 or move[0] == 2 ** 42) and move[1] == 2**57:
                game = apply(game, move, zobrist_numbers)
                break

    if terminal(game) != 1:
        print('Repetition test failed')
        print(game.ply_counter)
        draws_work = False

    # tests the 50 move rule more thoroughly by making the computer apply the moves and then check for a draw
    # takes a King Bishop Knight vs King position and makes the computer play 100 random moves (50 for each side) to reach the 50 move rule.
    # statistically speaking nothing should happen, but it is repeated 5 times so that any accidental checkmates are accounted for
    success = 0
    fails = 0
    captures = 0
    for i in range(5):
        fen = '2k5/8/8/8/8/8/8/5BNK w - - 0 1'
        background, buttons, game, zobrist_numbers = initialize_game(fen)

        move = [0, 0, 4]
        capture_happened = False
        for j in range(100):
            moves = legal_moves(game)
            k = 0
            # prevents captures to make sure that the 50 move rule is reached
            while (move[2] & 4) != 0:
                move = moves[random.randint(0, len(moves) - 1)]

                # breaks out of the loop to avoid infinite loops in the case where taking a piece is the only legal move
                k += 1
                if k >= 50:
                    capture_happened = True
                    break
            game = apply(game, move, zobrist_numbers)

        # tallies up whether the results were successes, fails, or captures (i.e no result because it wasn't able to test properly)
        if capture_happened:
            captures += 1
        elif terminal(game) == 1:
            success += 1
        else:
            fails += 1

    if fails != 0:
        print('Draw by 50 move rule failed')
        draws_work = False
    
    if success != 5:
        print('Draw by 50 move rule results:')
        print('Success:', success, 'Fail:', fails, 'Captures:', captures)

    return draws_work


'''
Tests the speed of the apply/unapply and legal_moves functions, using perft (see c_interface for more info)
Tests the perft function at different depths as it often works at different speeds at different depths due to differing ratios of things it has to do.
Last Modified: 16/9/2021
Last Modified by: Arkleseisure
'''
def speed_test():
    print('Speed test started')
    start_depth = 3
    finish_depth = 5
    num_depths = finish_depth - start_depth + 1
    speeds = [[] for i in range(num_depths)]
    for depth in range(start_depth, finish_depth + 1):
        start_time = time.time()
        trials = 0
        print('Started depth', depth, 'at', time.ctime(time.time()))
        while time.time() - start_time < 600:
            background, buttons, game, zobrist_numbers = initialize_game()
            answer_dict, time_taken = perft(game, zobrist_numbers, depth=depth, type='nodes')
            speeds[depth - start_depth].append(answer_dict['Nodes']/time_taken)
            trials += 1

        if len(speeds[depth - start_depth]) > 1:
            avg_speed = np.average(speeds[depth - start_depth])
            speed_std = np.std(speeds[depth - start_depth], ddof=1)
            error = speed_std/np.sqrt(len(speeds[depth - start_depth]))
            print('Speed estimate for depth', depth, round(avg_speed, -3), '+-', round(error, -3))
            print('Trials:', trials)
    print('Finished')
        


'''
Function used to quickly test out a new feature... stuff in here can largely be disregarded
Last Modified: 17/9/2021
Last Modified by: Arkleseisure
'''
def do_test_stuff(colour, ai):
    speed_test()

