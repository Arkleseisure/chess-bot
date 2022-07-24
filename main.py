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

main()

'''
Things to do:
-Tablebase
-Quiescence search

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


Elo: based off v2
    0.1     0.2     0.5
v1  635     650     745     +-20
v2  840     960     1100    +-5
v3  860     995     1145    +-10
v4  925     1040    1115    +-15
v5  975     1080    1175    +-15
v6  990     1075    1210    +-15
v7  1020    1140    1250    +-10
v8

Elo: based off previous
    0.1     0.2     0.5
v1  635     650     745     +-20
v2  840     960     1100    +-5
v3  860     995     1145    +-10
v4  930     1015    1150    +-15
v5  965     1075    1175    +-15
v6  980     1095    1190    +-10
v7  1015    1115    1245    +-15
v8  1115    1225    1350    +-15

Speeds /kn/s:
v1: 1354 +- 9
v2: 868 +- 6
v3: 1243 +- 12
v4: 1241 +- 1
v5: 1190 +- 12
v6: 1174 +- 13
v7: 1094 +- 8
v8: 1031 +- 4

Depth:
    0.1     0.2     0.5
v1  3.32    3.46    3.73
v2  4.11    4.53    4.87
v3  4.38    4.72    5.06
v4  4.39    4.78    5.15
v5  4.44    4.76    5.15
v6  4.36    4.74    5.11
v7  4.63    4.97    5.45
v8  4.96    5.43    5.89
'''
