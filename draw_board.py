import pygame
import pygame.freetype
import os

# defining the global pygame variables which are used in many places in this file
screen_width = 1400
screen_height = 700
square_size = screen_height // 10
board_x = screen_width // 10
board_y = screen_height // 10

# colours
white = [255, 255, 255]
black = [0, 0, 0]
dark_square_colour = [150, 100, 0]
light_square_colour = [255, 255, 255]
border_colour = [0, 100, 0]
location_highlight_colour = [50, 200, 50]
destination_highlight_colour = [60, 130, 70]
background_colour = [0, 0, 0]

# creates a pygame window in the center of the screen
os.environ['SDL_VIDEO_CENTERED'] = '1'
pygame.init()
screen = pygame.display.set_mode([screen_width, screen_height])
pygame.display.set_caption("Chess")

'''
Parent class for many different objects which are rectangular and may have text in them

width: width of the box
height: height of the box
x: x coordinate of the top left of the box (using pygame's grid... top left of screen is (0, 0), and each unit is 1 pixel across)
y: y coordinate of the top left of the box
colour: RGB value of the background of the box, 0-255 for each colour
text: text which appears in the center of the box
font: font the text appears in
font_size: size of the font
font_colour: RGB value, 0-255 for each like the background
bold: boolean determining whether the text is bold
italic: boolean determining whether the text is italic

Last Modified: 03/07/2021
Last Modified by: Arkleseisure
'''
class RectangleSprite(pygame.sprite.Sprite):
    def __init__(self, width, height, x=0, y=0, colour=[0,0,0], text='', font='Calibri', font_size=40, font_colour=[0,0,0], bold=False, italic=False):
            super().__init__()

            # creates pygame surface object onto which the label can be placed
            self.image = pygame.Surface([width, height])

            # sets the position of the sprite
            self.rect = self.image.get_rect()
            self.rect.x = x
            self.rect.y = y

            # draws the background colour onto the image
            pygame.draw.rect(self.image, colour, [0, 0, width, height])

            if text:
                # creates a surface with the text on it
                font = pygame.freetype.SysFont(font, font_size, bold=bold, italic=italic)
                print_image, rect = font.render(text, font_colour)

                # sticks this surface to the image such that it is centralized
                text_width, text_height = print_image.get_size()
                self.image.blit(print_image, ((width - text_width)//2, (height - text_height)//2))


# sprite class for the sprite which covers the background of the board
# Last Modified: 03/07/2021
# Last Modified by: Arkleseisure
class BackgroundColour(RectangleSprite):
    def __init__(self):
        super().__init__(width=screen_width, height=screen_height, colour=background_colour)

'''
Class for sprites of the squares of the board

colour: colour of the square, RGB 0-255
x: x position relative to board (when looking from white's perspective, x=0 is the a file)
y: y position relative to board (when looking from white's perspective, y=0 is the 8th rank)

Last Modified: 03/07/2021
Last Modified by: Arkleseisure
'''
class Square(RectangleSprite):
    def __init__(self, colour, x, y):
        super().__init__(width=square_size, height=square_size, x=board_x + x * square_size, y=board_y + y * square_size, colour=colour)

'''
Class for the row and column labels on the side of the board
x, y: see description for square
text: text in the label

Last Modified: 03/07/2021
Last Modified by: Arkleseisure
'''
class Label(RectangleSprite):
        def __init__(self, x, y, text):
            super().__init__(width=square_size, height=square_size, x = board_x + x * square_size, y = board_y + y * square_size, 
                             text=text, font='Calibri', font_size=40, font_colour=[0, 10, 0], bold=True)
            self.image.set_colorkey(black)


# class for the sprites of the border of the board
# Last Modified: 03/07/2021
# Last Modified by: Arkleseisure
class Border(RectangleSprite):
    def __init__(self):
        super().__init__(width = 10 * square_size, height = 10 * square_size, x = board_x - square_size, colour=border_colour)


'''
Class for any buttons beside the board
text: text in the button
x, y: same as for the square class
width, height: width and height of the button
colour: colour of the background of the button, RGB 0-255

Last Modified: 03/07/2021
Last Modified by: Arkleseisure
'''
class Button(RectangleSprite):
    def __init__(self, text, x, y, width=3*square_size//2, height=3*square_size//4, colour=[100, 100, 100]):
        super().__init__(width=width, height=height, x=board_x + x * square_size, y=board_y + y * square_size, colour=colour, text=text)
        self.width = width
        self.height = height
        self.name = text

    # returns True if the given x, y coordinates are within the button, False if not
    # Last Modified 03/07/2021
    # Last Modified by: Arkleseisure
    def is_clicked(self, x, y):
        return self.rect.x < x < self.rect.x + self.width and self.rect.y < y < self.rect.y + self.height


'''
initializes the background group, which is used to display the board, its borders and the column and row labels beside the board.
col: colour from whose perspective the user is looking, 0 = white, 1 = black

Last Modified: 01/07/2021
Last Modified by: Arkleseisure
'''
def initialize_pygame_stuff(col):
    # initializes the group
    background = pygame.sprite.Group()

    # adds the background colour to the background group
    background_colour = BackgroundColour()
    background.add(background_colour)

    # adds the border to the background group
    border = Border()
    background.add(border)

    # adds all the squares to the background group
    for i in range(8):
        for j in range(8):
            if (i + j) % 2 == 0:
                colour = light_square_colour
            else:
                colour = dark_square_colour

            new_square = Square(colour, i, j)
            background.add(new_square)


    # adds the labels to the background group
    for i in range(8):
        # labels for if the board is viewed from white's perspective (col=0), or black's perspective (col=1)
        if col == 0:
            new_col_label = Label(x=i, y=8, text=chr(ord('A') + i))
            new_row_label = Label(x=-1, y=7-i, text=str(i + 1))
        else:
            new_col_label = Label(x=7-i, y=8, text=chr(ord('A') + i))
            new_row_label = Label(x=-1, y=i, text=str(i + 1))

        background.add(new_col_label, new_row_label)

    buttons = pygame.sprite.Group()
    exit_button = Button('Exit', x=9.5, y=4.125)
    buttons.add(exit_button)
    return background, buttons


'''
Draws the current position to the screen 
colour: draw the board from white's perspective (0) or black's perspective (1)
background: pygame sprite group holding the sprites for the background (i.e, the board, borders, 
            background colour and labels beside the board)
buttons: pygame sprite group holding the sprites for the buttons beside the board
last_move: the last move that was played, encoded as 2 bitboards, one for the starting position 
           and one for the end, and a flag, which encodes additional information such as the piece moved
current_move: bitboard containing the square encoded by any move which is currently being played, 
              for instance when the player has clicked on a piece but hasn't moved it yet.

Last Modified: 03/07/2021
Last Modified by: Arkleseisure
'''
def draw_board(colour, background, buttons, board, last_move, current_move):
    background.draw(screen)
    draw_highlights(last_move, current_move, colour)
    draw_pieces(board, colour)
    buttons.draw(screen)

    pygame.display.flip()
        

'''
Waits for the human player to click on a square or button and then returns the result
colour: perspective of board, 0 (white) or 1 (black)
buttons: sprite group containing all buttons beside the board.

Last Modified: 02/07/2021
Last Modified by: Arkleseisure
'''
def get_square(colour, buttons):
    # loops while the player hasn't clicked on a valid square
    correct_square = False
    while not correct_square:
        # detects where the person clicks
        for event in pygame.event.get():
            if event.type == pygame.MOUSEBUTTONUP:
                mouse_x, mouse_y = pygame.mouse.get_pos()

                # calculates the x-y position with respect to the board
                x = (mouse_x - board_x) // square_size
                y = (mouse_y - board_y) // square_size

                # if a button is clicked on, this returns which one
                for button in buttons:
                    if button.is_clicked(mouse_x, mouse_y):
                        return 0, button.name

                # if the square is within the board, the loop is exited
                if 0 <= x <= 7 and 0 <= y <= 7:
                    correct_square = True

    # returns the square which was clicked on as a bitboard number
    if colour == 0:
        return 2 ** (x + 8 * (7 - y)), ''
    return 2 ** (7 - x + 8 * y ), ''

'''
Draws the images of the pieces to the screen
board: 12 * bitboards, one for each piece type
colour: perspective from which to draw the pieces, white (0) or black (1)

Last Modified: 01/07/2021
Last Modified by: Arkleseisure
'''
def draw_pieces(board, colour):
    image_names = ['Pw', 'Nw', 'Bw', 'Rw', 'Qw', 'Kw', 'Pb', 'Nb', 'Bb', 'Rb', 'Qb', 'Kb']

    # loops through each piece type
    for i in range(len(board)):
        # gets the image for this piece, then scales it to the size of the squares
        piece_type_image = pygame.image.load(image_names[i] + '.png')
        piece_type_image = pygame.transform.scale(piece_type_image, [square_size, square_size])

        # creates string of the bitboard which can be looped through to find the pieces
        piece_type_bitboard = bin(board[i])[1:]

        # finds the positions of the pieces in the bitboard
        piece_positions = [len(piece_type_bitboard) - j - 1 for j, k in enumerate(piece_type_bitboard) if k == '1']

        # gets the x and y positions to draw the pieces to and draws them there
        for pos in piece_positions:
            # due to the way the bitboards are set up, the first bit refers to a1, and the last to h8, going row by row
            x_coord = pos % 8
            y_coord = pos // 8

            # pygame indexes from the top left of the screen, so all the y coordinates are flipped
            if colour == 0:
                x = board_x + x_coord * square_size
                y = board_y + (7 - y_coord) * square_size
            else:
                x = board_x + (7 - x_coord) * square_size
                y = board_y + y_coord * square_size

            # does the actual drawing of pieces
            screen.blit(piece_type_image, (x, y))

'''
Draws the highlights onto the board
last_move: 2 * bitboard and 1 flag, refers to the last move to be played
current_move: bitboard encoding the position of the current click, for instance just after the human player has just clicked to select a piece
colour: 0 (white) or 1 (black), refers to the perspective from which the board is being viewed

Last Modified: 03/07/2021
Last Modified by: Arkleseisure
'''
def draw_highlights(last_move, current_move, colour):
    highlights = pygame.sprite.Group()

    # adds the highlights for the last move
    if last_move != [0, 0, 0]:
        squ1 = last_move[0]
        squ2 = last_move[1]

        # finds the number of the squares to highlight (the -3 is because we need to index the squares from 0 (-1) and bin always returns a string starting with 0b (-2))
        num_1 = len(bin(squ1)) - 3
        num_2 = len(bin(squ2)) - 3

        # finds the x and y coordinates of the square to highlight
        x_1 = num_1 % 8
        y_1 = num_1 // 8
        x_2 = num_2 % 8 
        y_2 = num_2 // 8


        # creates the sprites which will appear on the screen
        if colour == 0:
            highlight_1 = Square(location_highlight_colour, x_1, 7 - y_1)
            highlight_2 = Square(destination_highlight_colour, x_2, 7 - y_2)
        else:
            highlight_1 = Square(location_highlight_colour, 7 - x_1, y_1)
            highlight_2 = Square(destination_highlight_colour, 7 - x_2, y_2)

        # draws the sprites to the screen
        highlights.add(highlight_1, highlight_2)

    # adds highlight for current click if halfway through human move
    if current_move != 0:
        x = (len(bin(current_move)) - 3) % 8
        y = (len(bin(current_move)) - 3) // 8
        if colour == 0:
            new_highlight = Square(location_highlight_colour, x, 7 - y)
        else:
            new_highlight = Square(location_highlight_colour, 7 - x, y)
        highlights.add(new_highlight)

    # draws the highlights to the screen
    highlights.draw(screen)

# function to print out the individual bitboards, mainly for debuggin purposes
# Last Modified: 11/8/2021
# Last Modified by: Arkleseisure
def print_board(board):
    for i in range(len(board)):
        for j in range(8):
            print(bin(board[i] + 2 ** 64)[8 * (j + 1) + 2: 8 * j + 2: -1])
        print()
