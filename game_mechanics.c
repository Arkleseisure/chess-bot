#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

// Function to confirm that the file is being imported properly
// Last Modified: 04/07/2021
// Last Modified by: Arkleseisure
void confirm_it_works() {
	printf("\n It's working!!!\n");
	printf(" This is version 31.\n");
}


// functions for which the code hasn't yet been written

bool in_check(unsigned long long* board, int to_play) {
	return false;
}

void remove_illegal_moves(unsigned long long** legal_moves, int* illegal_moves, int num_moves, int num_illegal_moves) {
}

int get_pseudolegal_moves(unsigned long long* board, unsigned long long** legal_moves, int to_play, int castling, unsigned long long* last_move) {
	return 0;
}

/*
Function to apply a move to the position
board: board position, holding 12 bitboards, one for each piece type
move: move to be applied to the postion, held in 2 bitboards, one with the start of the move and one with the end, and a flag giving details about the move
extras: [castling, to_play], where castling is a 4 bit int with each bit representing the legality of castling for white or black, kingside or queenside,
			and to_play = 0 (white to play) or 1 (black to play)
last_move: last move to be played. This is not used in the function, but having it as an input means that its value gets returned automatically
Last Modified: 04/07/2021
Last Modified by: Arkleseisure

INCOMPLETE
*/
void apply(unsigned long long* board, unsigned long long* move, int* extras, unsigned long long* last_move) {
	// the piece which moves is encoded within the flag of the move
	unsigned long long piece = *(move + 2) >> 4;
	int castling = *extras;
	int to_play = *(extras + 1);

	// applies the move to the bitboard for that piece
	*(board + piece) ^= *move;
	*(board + piece) ^= *(move + 1);

	// removes any piece of the opposing colour on the square the piece lands on
	for (int i = (1 - to_play) * 6; i < 6 * (1 - to_play) + 6; i++) {
		*(board + i) ^= (*(board + i) & *(move + 1));
	}

	// changes the player
	*(extras + 1) = 1 - *(extras + 1);

	// sets the last move to the move which has just been applied
	for (int i = 0; i < 3; i++) {
		*(last_move + i) = *(move + i);
	}
}

/*
Function which applies the critical changes to the position such that they can be undone quickly, making doing and undoing single moves more efficient
board and move take the same form as they do in apply. Note: does not include pawn promotion

Last Modified: 11/07/2021
Last Modified by: Arkleseisure
UNTESTED
*/
int quick_apply(unsigned long long* board, unsigned long long* move, int to_play) {
	// breaks the last part of the move down into the bit describing the piece, and the bit describing the type of move, or flag
	unsigned long long piece = *(move + 2) >> 4;
	int flag = *(move + 2) & 15;
	int captured = -1;

	// applies the move to the bitboard for that piece
	*(board + piece) ^= *(move);
	*(board + piece) ^= *(move + 1);

	// applies captures
	// en passant for white
	if (flag == 5 && to_play == 0) {
		*(board + 6) ^= (*move << 8);
	}
	// en passant for black
	else if (flag == 5 && to_play == 1) {
		*(board) ^= (*move >> 8);
	}
	// other captures
	else if (4 == (flag & 4)) {
		for (int i = (1 - to_play) * 6; i < 6 * (1 - to_play) + 6; i++) {
			if ((*(board + i) & *(move + 1)) != 0) {
				*(board + i) ^= (*(board + i) & *(move + 1));
				captured = i;
			}
		}
	}

	// applies castling
	// castling kingside
	if (flag == 2) {
		// xors the board corresponding to the correct rook (the king will have already been moved) with the binary number 10100000, which will flip
		// the bits in the position that the rook currently is and will move to (as 1 refers to a1). This is then shifted for castling as black by 
		// 7 ranks * 8 squares = 56 squares
		*(board + 6 * to_play) ^= ((unsigned long long)(0xA0)) << 56 * to_play;
	}
	// castling queenside
	else if (flag == 3) {
		// same as for kingside castling, but with the binary number 1001 representing the queenside castling transformation
		*(board + 6 * to_play) ^= ((unsigned long long)(0x9)) << 56 * to_play;
	}

	// returns the type of piece which has been captured, allowing this capture to be undone if required
	return captured;
}


/*
Function which undoes the changes made by the quick_apply function.

Last Modified: 11/07/2021
Last Modified by: Arkleseisure
UNTESTED
*/
void quick_undo(unsigned long long* board, unsigned long long* move, int to_play, int captured) {
	// breaks the last part of the move down into the bit describing the piece, and the bit describing the type of move, or flag
	unsigned long long piece = *(move + 2) >> 4;
	unsigned int flag = *(move + 2) & 15;

	// undoes the move to the bitboard for that piece
	*(board + piece) ^= *(move);
	*(board + piece) ^= *(move + 1);

	// undoes captures
	// en passant for white
	if (flag == 5 && to_play == 0) {
		*(board + 6) ^= (*move << 8);
	}
	// en passant for black
	else if (flag == 5 && to_play == 1) {
		*(board) ^= (*move >> 8);
	}
	// other captures
	else if (4 == (flag & 4)) {
		*(board + captured) ^= *(move + 1);
	}

	// undoes castling
	// castling kingside
	if (flag == 2) {
		// xors the board corresponding to the correct rook (the king will have already been moved) with the binary number 10100000, which will flip
		// the bits in the position that the rook currently is and will move to (as 1 refers to a1). This is then shifted for castling as black by 
		// 7 ranks * 8 squares = 56 squares
		*(board + 6 * to_play) ^= ((unsigned long long)(0xA0)) << 56 * to_play;
	}
	// castling queenside
	else if (flag == 3) {
		// same as for kingside castling, but with the binary number 1001 representing the queenside castling transformation
		*(board + 6 * to_play) ^= ((unsigned long long)(0x9)) << 56 * to_play;
	}


}

/*
Function which returns the legal moves in a position.
All inputs are the same as for the apply function, except that the castling and to_play variables don't have to be modified and so can be passed normally

Last Modified: 11/07/2021
Last Modified by: Arkleseisure
UNTESTED... will probably have bugs, but basic ideas should be correct
*/
int legal_moves(unsigned long long* board, unsigned long long** legal_moves, int castling, int to_play, unsigned long long* last_move) {
	int i;
	int num_moves;

	// gets the pseudo-legal moves (moves which are possible without the condition that the king is not in check) for each piece type
	num_moves = get_pseudolegal_moves(board, legal_moves, to_play, castling, last_move);

	// creates a list which will hold all the positions of the moves which are removed due to the resulting position being check
	int* illegal_moves = (int*)malloc(num_moves * sizeof(int));
	int num_illegal_moves = 0;
	int captured;

	// checks if after any of the moves the player is in check
	for (i = 0; i < num_moves; i++) {
		// move is applied to the position efficiently (factors which don't change whether the resulting position is check aren't applied)
		captured = quick_apply(board, *(legal_moves + i), to_play);

		// if the resulting position is check, the location of the move in the list is added to the list of illegal moves
		if (in_check(board, 1 - to_play)) {
			*(illegal_moves + num_illegal_moves) = i;
			num_illegal_moves++;
		}

		// the move is undone from the position
		quick_undo(board, *(legal_moves + i), to_play, captured);
	}

	// removes the illegal moves from the resulting array
	remove_illegal_moves(legal_moves, illegal_moves, num_moves, num_illegal_moves);

	// returns the final number of legal moves, so that the code only uses the part of the legal_moves list which holds actual moves (as it is declared 
	// in advance, it by default will have a size significantly larger than the number of moves it will usually return)
	return num_moves - num_illegal_moves;
}
