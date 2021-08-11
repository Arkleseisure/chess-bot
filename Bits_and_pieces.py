import math

# function to convert an input bitboard to a readable square in text such as 'a1' or 'f3'
def convert_to_text(pos):
	# finds the square number of the position (see explanation in play_game)
	try:
		square_number = int(math.log2(pos))
	except ValueError:
		print(pos)
	
	# finds the row and column of the position
	col = square_number % 8
	row = square_number // 8

	# returns the letter for the column and the number of the row as a string
	return chr(col + 97) + str(row + 1)