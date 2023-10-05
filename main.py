from menu import menu
from play_game import play_game, do_test_stuff
import time

# Main function from which everything else is called
# Last Modified: 28/06/2021
# Last Modified by: Arkleseisure
def main():
    exit = False
    while not exit:
        option, colour, exit = menu()
        if option == 0:
            other_player = 'human'
        elif option == 1:
            other_player = 'ai'
        elif option == 2:
            do_test_stuff()
            exit = True
        elif option == 3:
            exit = True
        if not exit:
            play_game(colour, other_player)

if __name__ == "__main__":
    main()

'''
Things to do:
-Tablebase
-Parallel search
-Magic bitboards
-Stop clearing transposition table after each move
-Pawn structure
-Killer move    
-Null move pruning

------------------------------------
Perft results: Previous ai/new ai
depth 3: 35900 +- 100 / 6300000 +- 10000
depth 4: 34300 +- 400 / 4737000 +- 1000
depth 5: 34600 +- 300 / 5697000 +- 4000


v1: basic minimax
v2: added alpha-beta pruning
v3: improved efficiency
v4: added piece-square tables, started updating the value after each move rather than calculating it at the end of the search
v5: added endgame piece-square tables and interpolation
v6: added min_np_material and changed bishop value to 3.3
v7: added transposition table
v8: added nodes for move ordering
v9: added quiescence search
v10: added NegaMax and minor efficiency improvements
v11: optimised psqt and piece values
v12: allowed minimax to use full search, even at interrupted depth.
v13: introduced late move reduction and tempo bonus

Elo:
    0.1     0.2     0.5
v1  770     800     895     +-15
v2  1040    1150    1300    +-5
v3  1090    1195    1340    +-10
v4  1140    1215    1350    +-10
v5  1180    1280    1400    +-10
v6  1195    1310    1435    +-10
v7  1235    1340    1490    +-10
v8  1325    1450    1600    +-15
v9  1415    1525    1675    +-15
v10 1455    1585    1720    +-15
v11 1460    1585    1735    +-10
v12 1530    1635    1770    +-10
v13 1745    1870    2025    +-15

Speeds /kn/s:
v1: 2143 +- 21
v2: 1362 +- 9
v3: 1919 +- 12
v4: 1881 +- 3
v5: 1909 +- 17
v6: 1846 +- 23
v7: 1547 +- 2
v8: 1460 +- 15
v9:  533 +- 13
v10: 506 +- 14
v11: 468 +- 15
v12: 431 +- 10
v13: 582 +- 10

Depth:
    0.1     0.2     0.5
v1  3.35    3.54    3.93
v2  4.38    4.74    5.06
v3  4.57    4.91    5.24
v4  4.62    4.90    5.28
v5  4.71    5.08    5.48
v6  4.55    4.90    5.25
v7  4.86    5.18    5.57
v8  5.16    5.59    6.01
v9  4.57    5.12    5.72
v10 4.23    4.92    5.57
v11 4.33    4.85    5.41
v12 4.57    5.03    5.59
v13 6.50    7.19    7.95
'''
