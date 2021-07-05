from ctypes import CDLL
from menu import menu
from play_game import play_game, do_test_stuff

# Main function from which everything else is called
# Last Modified: 28/06/2021
# Last Modified by: Arkleseisure
def main():
    exit = False
    while not exit:
        ai, colour, exit = menu()
        if not exit:
            # play_game(colour, ai)
            do_test_stuff(colour, ai)

main()

'''
Functions to write:

Play Game:
draw_result(result)

C game mechanics:
apply(board, move) (properly)
legal_moves(board, castling, last_move)
terminal(board, last_move)

AI:
get_ai_move(board, castling, last_move, legal_moves)
'''
