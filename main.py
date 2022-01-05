from menu import menu
from play_game import play_game, do_test_stuff

# Main function from which everything else is called
# Last Modified: 28/06/2021
# Last Modified by: Arkleseisure
def main():
    exit = False
    while not exit:
        other_player, colour, exit = menu()
        if other_player == 0:
            other_player = 'human'
        else:
            other_player = 'ai'
        if not exit:
            # play_game(colour, other_player)
            do_test_stuff(colour, other_player)

main()

'''
Things to do:
-Node functionality
-Transposition table

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


Elo:
    0.1     0.2     0.5
v1  550     600     700
v2  850     1000    1100
v3  900     1050    1200
v4  1025    1125    1275
v5  1075    1180    1310

Speeds /kn/s:
v1: 1380
v2: 880
v3: 1260
v4: 1310
v5: 1300
'''
