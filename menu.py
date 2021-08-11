import random
from display_menu import display_menu, get_option

'''
Displays a menu for the user to choose their opponent and colour.
Last Modified: 28/06/2021
Last Modified by: Arkleseisure
'''
def menu():
    # Opponent choice
    options = ['Play human', 'Play computer', 'Exit']
    display_menu(options)
    option_chosen = get_option(options)

    # Exits if 'Exit' is chosen
    if option_chosen == len(options) - 1:
        return 0, 0, True

    # Colour choice
    colour_options = ['White', 'Black', 'Random']
    display_menu(colour_options)
    colour_chosen = get_option(colour_options)

    # Chooses random colour if 'Random' is chosen
    if colour_chosen == 2:
        colour_chosen = random.randint(0, 1)

    return option_chosen, colour_chosen, False
