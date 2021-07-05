#include <stdlib.h>
#include <stdio.h>

// Function to confirm that the file is being imported properly
// Last Modified: 04/07/2021
void confirm_it_works() {
	printf("\n It's working!!!\n");
	printf(" This is version 30.\n");
}


// Function to apply a move to the position
// board = board position, holding 12 bitboards, one for each piece type
// move = move to be applied to the postion, held in 2 bitboards, one with the start of the move and one with the end, and a flag giving details about the move
// extras = [castling, to_play], where castling is a 4 bit int with each bit representing the legality of castling for white or black, kingside or queenside,
//			and to_play = 0 (white to play) or 1 (black to play)
// last_move = last move to be played. This is not used in the function, but having it as an input means that its value gets returned automatically
// Last Modified: 04/07/2021
void apply(unsigned long long* board, unsigned long long* move, unsigned int* extras, unsigned long long* last_move) {
	// the piece which moves is encoded within the flag of the move
	unsigned int piece = *(move + 2) >> 4;
	unsigned int castling = *extras;
	unsigned int to_play = *(extras + 1);

	// applies the move to the bitboard for that piece
	*(board + piece) = *(board + piece) ^ *(move) ^ *(move + 1);

	// removes any piece of the opposing colour on the square the piece lands on
	for (unsigned int i = (1 - to_play) * 6; i < 6 * (1 - to_play) + 6; i++) {
		*(board + i) = (*(board + i) & *(move + 1)) ^ *(board + i);
	}

	// changes the player
	*(extras + 1) = 1 - *(extras + 1);

	// sets the last move to the move which has just been applied
	for (int i = 0; i < 3; i++) {
		*(last_move + i) = *(move + i);
	}
}