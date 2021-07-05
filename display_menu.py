from draw_board import screen, screen_width, screen_height, RectangleSprite
import pygame

# class for the buttons used to display the menu
# num = number of buttons being drawn
# y = some int from 0 to num - 1, and states that this button is the yth counting from the top
# text = text displayed on the button
# Last Modified: 03/07/2021
class Button(RectangleSprite):
    def __init__(self, y, num, text):
        # font details
        font = 'Calibri'
        font_size = 100
        font_colour = [255, 255, 255]

        # defines button size
        button_width = screen_width
        button_height = screen_height//num

        super().__init__(width=button_width, height=button_height, x=0, y=y*button_height, 
                         text=text, font=font, font_size=font_size, font_colour=font_colour, bold=True)


# displays a menu page, given a list of options
# Last Modified: 02/07/2021
def display_menu(options):
    buttons = pygame.sprite.Group()

    i = 0
    for option in options:
        new_button = Button(i, len(options), option)
        buttons.add(new_button)
        i += 1

    buttons.draw(screen)
    pygame.display.flip()


# takes in a list of options and returns the option which the user has clicked on
# Last Modified: 02/07/2021
def get_option(options):
    n = len(options)
    option_chosen = False

    # loops while the user hasn't chosen an option
    while not option_chosen:
        for event in pygame.event.get():
            if event.type == pygame.MOUSEBUTTONUP:
                mouse_x, mouse_y = pygame.mouse.get_pos()

                y = mouse_y // (screen_height//n)
                option_chosen = True

    return y

    