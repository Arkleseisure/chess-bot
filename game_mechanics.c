#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

// Function to confirm that the file is being imported properly
// Last Modified: 1/9/2021
// Last Modified by: Arkleseisure
void confirm_it_works() {
	printf("\n It's working!!!\n");
	printf(" This is version 106.\n");
}


// structure defining the key elements for each piece
// Last Modified: 10/8/2021
// Last Modified by: Arkleseisure
struct Piece {
	// bitboard representing the position of the piece
	unsigned long long loc;
	// number from 0 to 11 representing the piece type (white pawn, black pawn, white bishop, ... )
	int type;
	// whether this piece has been captured, true or false
	bool captured;
};

// returns the rank given by a single bit on a bitboard (or if there is more than one bit, the rank of the bit with the highest rank)
// Last Modified: 10/8/2021
// Last Modified by: Arkleseisure
int rank(unsigned long long loc) {
	int i = 0;
	int rank_no = 0;
	unsigned long long loc_shift = loc >> 8;

	while (loc_shift != 0 && loc_shift < loc) {
		rank_no++;
		loc_shift >>= 8;
	}
	return rank_no;
}

// returns the file given by a single bit on a bitboard (or if there is more than one bit, the file of the bit with the lowest file)
// Last Modified: 10/8/2021
// Last Modified by: Arkleseisure
int file(unsigned long long loc) {
	int i = 0;
	int file_no = 0;
	unsigned long long mask = 0x0101010101010101;

	// returns an error value if there is no bit there
	if (loc == 0) {
		return 8;
	}

	while ((mask & loc) == 0) {
		file_no++;
		mask <<= 1;
	}
	return file_no;
}


// functions for which the code hasn't yet been written
// checks whether the square held by loc is attacked
bool in_check(unsigned long long* board, int to_play, unsigned long long loc) {
	return false;
}

void unapply(unsigned long long* board, unsigned long long* move, int* extras, unsigned long long* previous_last_move, struct Piece* piece_list, int captured) {

}

/*
Function to add a move to an array
moves: array the move should be added to
piece_loc: current location of the piece as a bitboard
piece_dest: destination of the piece as a bitboard
flag: 4 bits telling us the type of move (documented in play_game)
piece: 4 bits referring to the type of piece being moved (also documented in play_game)
index: part of the flag, refers to the index of the piece in the piece_list. This is often more convenient to keep separate, which is why this
		is calculated within the function. 7 bits. Should be set to 0 if the index is already included within the flag.
Last Modified: 10/8/2021
Last Modified by: Arkleseisure
*/
void add_move(unsigned long long* moves, unsigned long long piece_loc, unsigned long long piece_dest, unsigned long long flag, unsigned long long piece, unsigned long long index) {
	moves[0] = piece_loc;
	moves[1] = piece_dest;
	moves[2] = flag + (piece << 4) + (index << 8);
}

/*
Performs a bitshift of a bitboard in a particular direction, used to avoid copious if statements in the code
bitboard: unsigned long long bitboard to be shifted
amount: number of bits to shift bitboard by
direction: 1 (<<) or -1 (>>)
Last Modified: 11/8/2021
Last Modified by: Arkleseisure
*/
unsigned long long shift_bitboard(unsigned long long bitboard, int amount, int direction) {
	if (direction == 1) {
		return bitboard << amount;
	}
	else if (direction == -1) {
		return bitboard >> amount;
	}
	return 0;
}

/*
Gets all the pseudolegal moves for a pawn at a given location
same_pieces: bitboard containing the pieces of the same colour as the pawn
other_pieces: bitboard containing the pieces of the opposite colour as the pawn
to_play: 0 (white) or 1 (black) to play
loc: bitboard with the location of the pawn
moves: array to put the moves found into
last_move: last move to have been played, used to check for possibility of en passant
index:
Last Modified: 10/8/2021
Last Modified by: Arkleseisure
*/
int get_pawn_moves(unsigned long long same_pieces, unsigned long long other_pieces, unsigned long long loc, unsigned long long moves[28][3], unsigned long long index,
	int to_play, unsigned long long* last_move) {
	int i;
	int j;
	int num_moves = 0;
	int direction = 1 - 2 * to_play;
	// used for updating the flag when promotion occurs
	unsigned long long original_flag;

	// variable holding the rank the pawn is on for ease of calculation for double moves at the start and promotion
	int piece_rank = rank(loc);
	// variable holding the file the pawn is on so that it doesn't skip across the side of the board	
	int piece_file = file(loc);

	// checks if the square in front of the pawn is occupied
	if ((shift_bitboard(loc, 8, direction) & (same_pieces | other_pieces)) == 0) {
		add_move(moves[0], loc, shift_bitboard(loc, 8, direction), 0, 6 * (unsigned long long)to_play, index);
		num_moves = 1;

		// checks for double moves (first term checks if the rank of the piece is 1 for white or 6 for black (due to indexing from 0), second for a piece on the next square)
		if (piece_rank == to_play * 5 + 1 && ((shift_bitboard(loc, 16, direction) & (same_pieces | other_pieces)) == 0)) {
			add_move(moves[1], loc, shift_bitboard(loc, 16, direction), 1, 6 * (unsigned long long)to_play, index);
			num_moves = 2;
		}
	}

	// captures... first checks whether there is an opponent's piece on the relevant square, then if it is on a file for which that capture is possible 
	// (as we are using single numbers as bitboards, there is no inbuilt idea of edges of the board)
	if ((shift_bitboard(loc, 7, direction) & other_pieces) != 0 && piece_file != 7 * to_play) {
		add_move(moves[num_moves], loc, shift_bitboard(loc, 7, direction), 4, 6 * (unsigned long long)to_play, index);
		num_moves++;
	}
	if ((shift_bitboard(loc, 9, direction) & other_pieces) != 0 && piece_file != 7 * (1 - to_play)) {
		add_move(moves[num_moves], loc, shift_bitboard(loc, 9, direction), 4, 6 * (unsigned long long)to_play, index);
		num_moves++;
	}

	// en passant (if statement translates to: if (last move was a double pawn push) and then the next two check 
	// whether it is to the left or right of the current pawn)
	if ((last_move[2] & 15) == 1) {
		if (last_move[1] >> 1 == loc) {
			add_move(moves[num_moves], loc, shift_bitboard(loc, 9 - 2 * to_play, direction), 5, 6 * (unsigned long long)to_play, index);
			num_moves++;
		}
		else if (last_move[1] << 1 == loc) {
			add_move(moves[num_moves], loc, shift_bitboard(loc, 7 + 2 * to_play, direction), 5, 6 * (unsigned long long)to_play, index);
			num_moves++;
		}
	}

	// pawn promotion (checks if the pawn is on one of the ranks and then changes the moves accordingly)
	if (piece_rank == 6 - (to_play * 5)) {
		// loops through each move found for the pawn
		for (i = 0; i < num_moves; i++) {
			original_flag = moves[i][2];
			// 4 possible promotions for each move
			for (j = 0; j < 4; j++) {
				// edits the move to add promotion to the flag
				add_move(moves[i + j * num_moves], moves[i][0], moves[i][1], original_flag + 8 + j, 0, 0);
			}
		}
		num_moves *= 4;
	}
	return num_moves;
}

/*
Gets all the pseudolegal moves for a knight at a given location
same_pieces: bitboard containing the pieces of the same colour as the knight
other_pieces: bitboard containing the pieces of the opposite colour as the knight
loc: bitboard with the location of the knight
moves: array to put the moves found into
Last Modified: 11/8/2021
Last Modified by: Arkleseisure
*/
int get_knight_moves(unsigned long long same_pieces, unsigned long long other_pieces, unsigned long long loc, unsigned long long moves[28][3], unsigned long long index, int to_play) {
	int i;
	int num_moves = 0;
	// array holding the amount a bit has to be shifted to make each knight move in the positive direction (this is then reflected for the negative)
	int knight_shifts[] = { 6, 10, 15, 17 };
	int piece_file = file(loc);

	// loops through the possible shifts
	for (i = 0; i < 4; i++) {
		// adds moves going up the board (first if statement checks the move doesn't go off the sides)
		if (loc << knight_shifts[i] != 0 && abs((piece_file + knight_shifts[i]) % 8 - piece_file) <= 2) {
			// captures
			if (((loc << knight_shifts[i]) & other_pieces) != 0) {
				add_move(moves[num_moves], loc, loc << knight_shifts[i], 4, 1 + 6 * (unsigned long long)to_play, index);
				num_moves++;
			}
			// normal moves
			else if (((loc << knight_shifts[i]) & same_pieces) == 0) {
				add_move(moves[num_moves], loc, loc << knight_shifts[i], 0, 1 + 6 * (unsigned long long)to_play, index);
				num_moves++;
			}
		}
		// adds moves going down the board (by symmetry the calculation which checks the side above works here, 
		// but with flipped coordinates (as the coordinates are now decreasing instead of increasing))
		if (loc >> knight_shifts[i] != 0 && abs((knight_shifts[i] + (7 - piece_file)) % 8 - (7 - piece_file)) <= 2) {
			// captures
			if (((loc >> knight_shifts[i]) & other_pieces) != 0) {
				add_move(moves[num_moves], loc, loc >> knight_shifts[i], 4, 1 + 6 * (unsigned long long)to_play, index);
				num_moves++;
			}
			// normal moves
			else if (((loc >> knight_shifts[i]) & same_pieces) == 0) {
				add_move(moves[num_moves], loc, loc >> knight_shifts[i], 0, 1 + 6 * (unsigned long long)to_play, index);
				num_moves++;
			}
		}
	}
	return num_moves;
}

/*
Gets all the pseudolegal moves for a bishop at a given location
same_pieces: bitboard containing the pieces of the same colour as the bishop
other_pieces: bitboard containing the pieces of the opposite colour as the bishop
loc: bitboard with the location of the bishop
moves: array to put the moves found into
Last Modified: 12/8/2021
Last Modified by: Arkleseisure
*/
int get_bishop_moves(unsigned long long same_pieces, unsigned long long other_pieces, unsigned long long loc, unsigned long long moves[28][3], unsigned long long index, int to_play) {
	int i;
	int j;
	int num_moves = 0;
	int piece_rank = rank(loc);
	int piece_shifts[] = { 7, 9 };
	int directions[] = { -1, 1 };
	unsigned long long new_position;

	// loops through each of the four directions in which it can move
	for (i = 0; i < 4; i++) {
		for (j = 1; j < 8; j++) {
			new_position = shift_bitboard(loc, piece_shifts[i % 2] * j, directions[i / 2]);
			// checks the move doesn't take the piece off the side of the board or onto one of their own pieces
			if (new_position == 0 || (new_position & same_pieces) != 0 || abs(rank(new_position) - piece_rank) != j) {
				break;
			}
			// if the move is a capture, it adds that move and then breaks to the next loop
			else if ((new_position & other_pieces) != 0) {
				add_move(moves[num_moves], loc, new_position, 4, 2 + 6 * (unsigned long long)to_play, index);
				num_moves++;
				break;
			}
			add_move(moves[num_moves], loc, new_position, 0, 2 + 6 * (unsigned long long)to_play, index);
			num_moves++;
		}
	}
	return num_moves;
}

/*
Gets all the pseudolegal moves for a rook at a given location
same_pieces: bitboard containing the pieces of the same colour as the rook
other_pieces: bitboard containing the pieces of the opposite colour as the rook
loc: bitboard with the location of the rook
moves: array to put the moves found into
Last Modified: 12/8/2021
Last Modified by: Arkleseisure
*/
int get_rook_moves(unsigned long long same_pieces, unsigned long long other_pieces, unsigned long long loc, unsigned long long moves[28][3], unsigned long long index, int to_play) {
	int i;
	int j;
	int num_moves = 0;
	int piece_rank = rank(loc);
	int piece_shifts[] = { 1, 8 };
	int directions[] = { -1, 1 };
	unsigned long long new_position;

	// loops through each of the four directions in which it can move
	for (i = 0; i < 4; i++) {
		for (j = 1; j < 8; j++) {
			new_position = shift_bitboard(loc, piece_shifts[i % 2] * j, directions[i / 2]);
			// checks the move doesn't take the piece off the side of the board or onto one of their own pieces
			if (new_position == 0 || (new_position & same_pieces) != 0 || (piece_shifts[i % 2] == 1 && rank(new_position) != piece_rank)) {
				break;
			}
			// if the move is a capture, it adds that move and then breaks to the next loop
			else if ((new_position & other_pieces) != 0) {
				add_move(moves[num_moves], loc, new_position, 4, 3 + 6 * (unsigned long long)to_play, index);
				num_moves++;
				break;
			}
			add_move(moves[num_moves], loc, new_position, 0, 3 + 6 * (unsigned long long)to_play, index);
			num_moves++;
		}
	}
	return num_moves;
}

/*
Gets all the pseudolegal moves for a queen at a given location
same_pieces: bitboard containing the pieces of the same colour as the queen
other_pieces: bitboard containing the pieces of the opposite colour as the queen
loc: bitboard with the location of the queen
moves: array to put the moves found into
Last Modified: 12/8/2021
Last Modified by: Arkleseisure
*/
int get_queen_moves(unsigned long long same_pieces, unsigned long long other_pieces, unsigned long long loc, unsigned long long moves[28][3], unsigned long long index, int to_play) {
	int i;
	int j;
	int num_moves = 0;
	int piece_rank = rank(loc);
	int piece_shifts[] = { 1, 7, 8, 9 };
	int directions[] = { -1, 1 };
	unsigned long long new_position;

	// loops through each of the eight directions in which it can move
	for (i = 0; i < 8; i++) {
		// loops though moving one square at a time
		for (j = 1; j < 8; j++) {
			new_position = shift_bitboard(loc, piece_shifts[i % 4] * j, directions[i / 4]);
			// checks the move doesn't take the piece off the side of the board or onto one of their own pieces
			// first checks it hasn't gone off the top or bottom of the board
			// second checks it hasn't landed on a friendly piece
			// third checks it hasn't moved horizontally off the side of the board
			// fourth checks it hasn't moved diagonally off the side of the board (7 and 9 are the shifts for diagonal moves)
			if (new_position == 0 || (new_position & same_pieces) != 0 || (piece_shifts[i % 4] == 1 && rank(new_position) != piece_rank) ||
				((piece_shifts[i % 4] == 7 || piece_shifts[i % 4] == 9) && abs(rank(new_position) - piece_rank) != j)) {
				break;
			}
			// if the move is a capture, it adds that move and then breaks to the next loop
			else if ((new_position & other_pieces) != 0) {
				add_move(moves[num_moves], loc, new_position, 4, 4 + 6 * (unsigned long long)to_play, index);
				num_moves++;
				break;
			}
			add_move(moves[num_moves], loc, new_position, 0, 4 + 6 * (unsigned long long)to_play, index);
			num_moves++;
		}
	}
	return num_moves;
}

/*
Gets all the pseudolegal moves for a king at a given location
same_pieces: bitboard containing the pieces of the same colour as the king
other_pieces: bitboard containing the pieces of the opposite colour as the king
loc: bitboard with the location of the king
moves: array to put the moves found into
castling: 4 bits representing legality of castling for each side, first two kingside/queenside for black, second two kingside/queenside for white
Last Modified: 13/8/2021
Last Modified by: Arkleseisure
*/
int get_king_moves(unsigned long long same_pieces, unsigned long long other_pieces, unsigned long long loc, unsigned long long moves[28][3], unsigned long long index, int to_play, int castling, unsigned long long* board) {
	int i;
	int num_moves = 0;
	int piece_rank = rank(loc);
	int piece_file = file(loc);
	int piece_shifts[] = { 1, 7, 8, 9 };
	int directions[] = { -1, 1 };
	unsigned long long new_position;

	// loops through each of the eight directions in which it can move
	for (i = 0; i < 8; i++) {
		new_position = shift_bitboard(loc, piece_shifts[i % 4], directions[i / 4]);
		// checks the move doesn't take the piece off the side of the board or onto one of their own pieces
		// first checks it hasn't gone off the top or bottom of the board
		// second checks it hasn't landed on a friendly piece
		// third checks it hasn't moved horizontally off the side of the board
		// fourth checks it hasn't moved diagonally off the side of the board (7 and 9 are the shifts for diagonal moves)
		if (new_position == 0 || (new_position & same_pieces) != 0 || (piece_shifts[i % 4] == 1 && rank(new_position) != piece_rank) ||
			((piece_shifts[i % 4] == 7 || piece_shifts[i % 4] == 9) && abs(rank(new_position) - piece_rank) != 1)) {
		}
		// if the move is a capture, it adds that move and then breaks to the next loop
		else if ((new_position & other_pieces) != 0) {
			add_move(moves[num_moves], loc, new_position, 4, 5 + 6 * (unsigned long long)to_play, index);
			num_moves++;
		}
		else {
			add_move(moves[num_moves], loc, new_position, 0, 5 + 6 * (unsigned long long)to_play, index);
			num_moves++;
		}
	}

	// castling kingside (checks whether the king or rook have moved (held in the castling variable), then verifies the castle would not be moving through check, 
	// then that it would not be moving through pieces... 96 is 01100000, so represents the squares which must be free for the move to be made)
	if (((castling & (2 + 6 * to_play)) != 0) && !(in_check(board, to_play, loc << 1)) && (((other_pieces ^ same_pieces) & ((unsigned long long)96 << (56 * (unsigned long long)to_play))) == 0)) {
		add_move(moves[num_moves], loc, loc << 2, 2, 5 + 6 * (unsigned long long)to_play, index);
		num_moves++;
	}
	// castling queenside
	// generally the same as kingside, but with 00001110 being 14 representing the squares which need to be free
	if ((castling & (1 + 3 * to_play)) != 0 && !(in_check(board, to_play, loc >> 1)) && ((other_pieces ^ same_pieces) & ((unsigned long long)14 << (56 * (unsigned long long)to_play))) == 0) {
		add_move(moves[num_moves], loc, loc >> 2, 3, 5 + 6 * (unsigned long long)to_play, index);
		num_moves++;
	}
	return num_moves;
}

/*
Function to apply a move to the position
board: board position, holding 12 bitboards, one for each piece type
move: move to be applied to the postion, held in 2 bitboards, one with the start of the move and one with the end, and a flag giving details about the move
extras: [castling, to_play], where castling is a 4 bit int with each bit representing the legality of castling for white or black, kingside or queenside,
			and to_play = 0 (white to play) or 1 (black to play)
last_move: last move to be played. This is not used in the function, but it does get changed and having it as an input means that its value gets
			returned automatically
Last Modified: 11/8/2021
Last Modified by: Arkleseisure
INCOMPLETE
*/
int apply(unsigned long long* board, unsigned long long* move, int* extras, unsigned long long* last_move, struct Piece* piece_list, int* past_hash_list) {
	// the piece which moves is encoded within the flag of the move
	int i;
	int index = (int)move[2] >> 8;
	int piece = (move[2] >> 4) & 15;
	int flag = move[2] & 15;
	int castling = extras[0];
	int to_play = extras[1];
	int hash = extras[2];
	int ply_counter = extras[3];
	int capture_type = 12;

	// applies the move to the bitboard for that piece
	board[piece] ^= move[0] | move[1];
	piece_list[index].loc ^= move[0] | move[1];

	// removes any piece of the opposing colour on the square the piece lands on
	for (i = (1 - to_play) * 6; i < 6 * (1 - to_play) + 6; i++) {
		board[i] ^= board[i] & move[1];
	}
	for (i = (1 - to_play) * 16; i < 16 * (1 - to_play) + 16; i++) {
		if ((piece_list[i].loc & move[1]) != 0) {
			piece_list[i].captured = true;
			capture_type = piece_list[i].type;
		}
	}

	// changes the player
	extras[1] = 1 - extras[1];

	// sets the last move to the move which has just been applied
	for (int i = 0; i < 3; i++) {
		last_move[i] = move[i];
	}
	return capture_type;
}

/*
Function which applies the critical changes to the position such that they can be undone quickly, making doing and undoing single moves more efficient
board and move take the same form as they do in apply. Note: does not include pawn promotion
Last Modified: 11/08/2021
Last Modified by: Arkleseisure
*/
int quick_apply(unsigned long long* board, unsigned long long* move, int to_play) {
	// breaks the last part of the move down into the bit describing the piece, and the bit describing the type of move, or flag
	int piece = (move[2] >> 4) & 15;
	int flag = move[2] & 15;
	int captured = -1;

	// applies the move to the bitboard for that piece
	board[piece] ^= move[0];
	board[piece] ^= move[1];

	// applies captures
	// en passant for white
	if (flag == 5 && to_play == 0) {
		board[6] ^= (move[0] << 8);
	}
	// en passant for black
	else if (flag == 5 && to_play == 1) {
		board[0] ^= (move[0] >> 8);
	}
	// other captures
	else if (4 == (flag & 4)) {
		for (int i = (1 - to_play) * 6; i < 6 * (1 - to_play) + 6; i++) {
			if ((board[i] & move[1]) != 0) {
				board[i] ^= (board[i] & move[1]);
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
		board[6 * to_play + 3] ^= ((unsigned long long)(0xA0)) << 56 * to_play;
	}
	// castling queenside
	else if (flag == 3) {
		// same as for kingside castling, but with the binary number 1001 representing the queenside castling transformation
		board[6 * to_play + 3] ^= ((unsigned long long)(0x9)) << 56 * to_play;
	}

	// returns the type of piece which has been captured, allowing this capture to be undone if required
	return captured;
}

/*
Function which undoes the changes made by the quick_apply function.
Last Modified: 10/08/2021
Last Modified by: Arkleseisure
*/
void quick_undo(unsigned long long* board, unsigned long long* move, int to_play, int captured) {
	// breaks the last part of the move down into the index (of the piece moving in the piece_list), the piece type and the flag (all documented in play_game)
	int index = (int)move[2] >> 8;
	int piece = (move[2] >> 4) & 15;
	int flag = move[2] & 15;

	// undoes the move to the bitboard for that piece
	board[piece] ^= move[0] & move[1];

	// undoes captures
	// en passant for white
	if (flag == 5 && to_play == 0) {
		board[6] ^= (move[0] << 8);
	}
	// en passant for black
	else if (flag == 5 && to_play == 1) {
		board[0] ^= (move[0] >> 8);
	}
	// other captures
	else if (4 == (flag & 4)) {
		board[captured] ^= move[1];
	}

	// undoes castling
	// castling kingside
	if (flag == 2) {
		// xors the board corresponding to the correct rook (the king will have already been moved) with the binary number 10100000, which will flip
		// the bits in the position that the rook currently is and will move to (as 1 refers to a1). This is then shifted for castling as black by 
		// 7 ranks * 8 squares = 56 squares
		board[6 * to_play + 3] ^= ((unsigned long long)(0xA0)) << 56 * to_play;
	}
	// castling queenside
	else if (flag == 3) {
		// same as for kingside castling, but with the binary number 1001 representing the queenside castling transformation
		board[6 * to_play + 3] ^= ((unsigned long long)(0x9)) << 56 * to_play;
	}

}

/*
Function which returns the legal moves in a position.
All inputs are the same as for the apply function, except that the castling and to_play variables don't have to be modified and so can be passed normally, and
the added legal_moves array to put the moves in.
Last Modified: 10/08/2021
Last Modified by: Arkleseisure
*/
int legal_moves(unsigned long long* board, unsigned long long legal_moves[220][3], int castling, int to_play, unsigned long long* last_move, struct Piece* piece_list) {
	// integers used in for loops
	int i;
	int j;

	// total number of legal moves generated
	int num_moves = 0;
	// number of pseudolegal moves found for that piece
	int piece_moves = 0;
	// holds the piece which has been captured when the move is applied to look for check
	int captured;

	// the first 6 bitboards in board hold the white pieces, the next 6 black... These store this shift for white and for black so it doesn't have to be calculated every time.
	int p = 6 * to_play;
	int other_p = 6 * (1 - to_play);

	// holds the pseudolegal moves for one piece
	unsigned long long piece_legal_moves[28][3];

	// precalculated bitboards with the locations of each of the pieces for each colour
	unsigned long long other_pieces = 0;
	unsigned long long same_pieces = 0;

	for (i = 0; i < 6; i++) {
		other_pieces ^= board[i + other_p];
		same_pieces ^= board[i + p];
	}

	// loops through each of the pieces for the side to play, to generate the legal moves for each of them.
	for (i = to_play * 16; i < 16 + to_play * 16; i++) {
		if (!piece_list[i].captured) {
			// generates the legal moves for that piece depending on its piece type.
			switch (piece_list[i].type % 6) {
			case 0:
				piece_moves = get_pawn_moves(same_pieces, other_pieces, piece_list[i].loc, piece_legal_moves, i, to_play, last_move);
				break;
			case 1:
				piece_moves = get_knight_moves(same_pieces, other_pieces, piece_list[i].loc, piece_legal_moves, i, to_play);
				break;
			case 2:
				piece_moves = get_bishop_moves(same_pieces, other_pieces, piece_list[i].loc, piece_legal_moves, i, to_play);
				break;
			case 3:
				piece_moves = get_rook_moves(same_pieces, other_pieces, piece_list[i].loc, piece_legal_moves, i, to_play);
				break;
			case 4:
				piece_moves = get_queen_moves(same_pieces, other_pieces, piece_list[i].loc, piece_legal_moves, i, to_play);
				break;
			case 5:
				piece_moves = get_king_moves(same_pieces, other_pieces, piece_list[i].loc, piece_legal_moves, i, to_play, castling, board);
				break;
			}

			// loops through each of the moves generated to see if it's in check.
			for (j = 0; j < piece_moves; j++) {
				// move is applied to the position efficiently (factors which don't change whether the resulting position is check aren't applied)
				captured = quick_apply(board, piece_legal_moves[j], to_play);

				// if the resulting position is not check, the move is added to the array of legal moves. (the position passed in is that of the king of the relevant colour)
				if (!(in_check(board, 1 - to_play, piece_list[15 + 16 * to_play].loc))) {
					// the index and piece are already included within the flag so we don't need to add them.
					add_move(legal_moves[num_moves], piece_legal_moves[j][0], piece_legal_moves[j][1], piece_legal_moves[j][2], 0, 0);
					num_moves++;
				}

				// the move is undone from the position
				quick_undo(board, piece_legal_moves[j], to_play, captured);
			}
		}
	}

	// returns the final number of legal moves, so that the code only uses the part of the legal_moves list which holds actual moves (as it is declared 
	// in advance, it by default will have a size significantly larger than the number of moves it will usually return)
	return num_moves;
}

/*
Looks for a draw by lack of material, by looking through the piece list and checking whether there are enough pieces to carry on playing
inputs: piece_list: List of structs of each piece, holding piece location, type and whether it has been captured.
Last Modified: 17/8/2021
Last Modified by: Arkleseisure
UNTESTED
*/
bool draw_by_lack_of_material(struct Piece* piece_list) {
	int i;
	int num_minor_pieces[] = { 0, 0 };

	for (i = 0; i < 32; i++) {
		if (!piece_list[i].captured) {
			switch (piece_list[i].type % 6) {
				// a pawn is always enough material to win
			case 0:
				return false;
				// a knight will be enough to win if paired with a bishop
			case 1:
				num_minor_pieces[piece_list[i].type / 6]++;
				break;
				// a bishop will be enough to win if it is paired either with a knight or a bishop
			case 2:
				if (num_minor_pieces[piece_list[i].type / 6] > 0) {
					return false;
				}
				num_minor_pieces[piece_list[i].type / 6]++;
				break;
				// a rook is always enough to win
			case 3:
				return false;
				// a queen is always enough to win
			case 4:
				return false;
			default:
				break;
			}
		}
	}
	return true;
}

/*
Looks to see if there is checkmate or stalemate... First checks that there are no legal moves, then checks whether it is check or not
INPUTS:
board: same as usual, 12 x bitboards documented in play_game
to_play: 0 (white is the one being mated) or 1 (black is the one being mated)
last_move: last move to be played, using the standard notation used in play_game
piece_list: array of 32 pieces, each one containing the piece type, their location and whether or not they have been captured yet.
Last Modified: 17/8/2021
Last Modified by: Arkleseisure
UNTESTED
*/
int look_for_mates(unsigned long long* board, int to_play, unsigned long long* last_move, struct Piece* piece_list) {
	int i;
	int j;

	// number of pseudolegal moves found for each piece when its moves are generated
	int piece_moves = 0;
	// holds the piece which has been captured when the move is applied to look for check
	int captured;

	// holds the pseudolegal moves for one piece
	unsigned long long piece_legal_moves[28][3];

	// precalculated bitboards with the locations of each of the pieces for each colour
	unsigned long long other_pieces = 0;
	unsigned long long same_pieces = 0;


	// the first 6 bitboards in board hold the white pieces, the next 6 black... These store this shift for white and for black so it doesn't have to be calculated every time.
	int p = 6 * to_play;
	int other_p = 6 * (1 - to_play);

	for (i = 0; i < 6; i++) {
		other_pieces ^= board[i + other_p];
		same_pieces ^= board[i + p];
	}

	// tries to find a single legal move for the player. If this is found, it is neither checkmate nor stalemate
	for (i = 16 * to_play; i < 16 * to_play + 16; i++) {
		if (!piece_list[i].captured) {
			// generates the legal moves for that piece depending on its piece type.
			switch (piece_list[i].type % 6) {
			case 0:
				piece_moves = get_pawn_moves(same_pieces, other_pieces, piece_list[i].loc, piece_legal_moves, i, to_play, last_move);
				break;
			case 1:
				piece_moves = get_knight_moves(same_pieces, other_pieces, piece_list[i].loc, piece_legal_moves, i, to_play);
				break;
			case 2:
				piece_moves = get_bishop_moves(same_pieces, other_pieces, piece_list[i].loc, piece_legal_moves, i, to_play);
				break;
			case 3:
				piece_moves = get_rook_moves(same_pieces, other_pieces, piece_list[i].loc, piece_legal_moves, i, to_play);
				break;
			case 4:
				piece_moves = get_queen_moves(same_pieces, other_pieces, piece_list[i].loc, piece_legal_moves, i, to_play);
				break;
			case 5:
				piece_moves = get_king_moves(same_pieces, other_pieces, piece_list[i].loc, piece_legal_moves, i, to_play, 0, board);
				break;
			}


			// loops through each of the moves generated to see if it's in check.
			for (j = 0; j < piece_moves; j++) {
				// move is applied to the position efficiently (factors which don't change whether the resulting position is check aren't applied)
				captured = quick_apply(board, piece_legal_moves[j], to_play);

				// if the resulting position is not check, then it is neither checkmate nor stalemate
				if (!(in_check(board, 1 - to_play, piece_list[15 + 16 * to_play].loc))) {
					quick_undo(board, piece_legal_moves[j], to_play, captured);
					return 0;
				}

				// the move is undone from the position
				quick_undo(board, piece_legal_moves[j], to_play, captured);
			}
		}
	}

	// if there are no moves and it is check, then it is checkmate, if not then it is stalemate
	if (in_check(board, to_play, piece_list[15 + 16 * to_play].loc)) {
		return 1;
	}
	return 2;
}

/*
Function which determines whether or not the game is in a terminal state... Returns 0 if a win for black, 1 if draw, 2 if win for white and 3 if the position is not terminal
INPUTS:
board: the usual 12 * bitboards described in play_game
to_play: 0 (black played last and we are checking if white is mated) or 1 (white played last and we're checking if black is being mated)
piece_list: array of 32 pieces, each one containing the piece type, their location and whether or not they have been captured yet.
ply_counter: number of ply since the 50 move rule has been reset (1 ply = 1 move for 1 player)
past_hash_list: array of the hashes of past positions, to check for repetition
hash: zobrist hash of current position
last_move: last move to be played, held as 2 bitboards and a flag, as explained in play_game
Last Modified: 17/8/2021
Last Modified by: Arkleseisure
UNTESTED
*/
int terminal(unsigned long long* board, int to_play, struct Piece* piece_list, int ply_counter, int* past_hash_list, int hash, unsigned long long* last_move) {
	int i;
	int repetitions = 0;
	int mates;

	// checks if the 50 move rule has been reached
	if (ply_counter >= 100) {
		return 1;
	}
	// looks for repetitions... After the 50 move rule has been reset, no repetitions of positions before this point can occur, 
	// and threefold repetition can only occur at least 3 moves after the reset, so we check that the move counter is at least 4 for efficiency purposes
	else if (ply_counter > 4) {
		// it is only a repetition if it is the same player to play, and so we only look at the moves where it is this player to play
		for (i = ply_counter % 2; i < ply_counter; i += 2) {
			// increments the repetition counter if this position is a repetition of the past board
			if (hash == past_hash_list[i]) {
				repetitions++;
				// if this position has occured 2 times in the past then this is the third repetition and so it is a draw
				if (repetitions == 2) {
					return 1;
				}
			}
		}
	}

	// gets draw by lack of material
	if (draw_by_lack_of_material(piece_list)) {
		return 1;
	}

	// look for mates function returns 0 by default, 1 if checkmate and 2 if stalemate
	mates = look_for_mates(board, to_play, last_move, piece_list);
	switch (mates) {
	case 0:
		return 3;
	case 1:
		return 2 * to_play;
	case 2:
		return 1;
	}

	return 3;
}

/*
Perft function: calculates the number of nodes, captures, en passant captures, castles, promotions, checks and checkmates at a certain depth. 
These values are then compared to generally accepted values in order to check the functioning of the game mechanics code.

Last Modified: 1/9/2021
Last Modified by: Arkleseisure
*/
void perft_all(unsigned long long* board, unsigned long long* last_move, int castling, struct Piece* piece_list, int to_play, int hash, int* past_hash_list, 
		int ply_counter, int depth, int* answers) {
	// checks if the position is at the final depth. if so, it adds the values to the answers, the order of these being: 
	// Nodes, Captures, En passant, Castling, Promotion, Checks, Checkmates
	if (depth == 0) {
		answers[0]++;
		// captures are noted in the 3rd bit of the flag
		if ((last_move[2] & 4) != 0) {
			answers[1]++;
			// denotes the previous move being en passant
			if ((last_move[2] & 15) == 5) {
				answers[2]++;
			}
		}

		// checks for kingside or queenside castling
		if (((last_move[2] & 15) == 2) || ((last_move[2] & 15) == 3)) {
			answers[3]++;
		}

		// looks for promotions
		if ((last_move[2] & 8) != 0){
			answers[4]++;
		}

		// looks for checks
		if (in_check(board, to_play, piece_list[15 + 16 * to_play].loc)) {
			answers[5]++;
			if (look_for_mates(board, to_play, last_move, piece_list) != 0)  {
				answers[6]++;
			}
		}
		return;
	}
	unsigned long long moves[220][3];
	unsigned long long previous_last_move[3];
	int num_moves;
	int captured;
	int i;
	// single variables are passed as arrays so that their values are returned automatically.
	int extras[4] = { castling, to_play, hash, ply_counter };

	// this is required in order to be able to undo the move properly, as otherwise the last move is changed and there is no way to get it back.
	for (i = 0; i < 3; i++) {
		previous_last_move[i] = last_move[i];
	}


	num_moves = legal_moves(board, moves, castling, to_play, last_move, piece_list);

	for (i = 0; i < num_moves; i++){
		captured = apply(board, moves[i], extras, last_move, piece_list, past_hash_list);
		perft_all(board, last_move, extras[0], piece_list, extras[1], extras[2], past_hash_list, extras[3], depth - 1, answers);
		unapply(board, moves[i], extras, previous_last_move, piece_list, captured);
	}

}

/* 
Same as perft_all, but doesn't return all the debugging statistics and is instead more streamlined and used for benchmarking

Last Modified: 1/9/2021
Last Modified by: Arkleseisure
*/
void perft_nodes(unsigned long long* board, unsigned long long* last_move, int castling, struct Piece* piece_list, int to_play, int hash, int* past_hash_list,
	int ply_counter, int depth, int* answers) {
	unsigned long long moves[220][3];
	unsigned long long previous_last_move[3];
	int num_moves;
	int captured;
	int i;
	// single variables are passed as arrays so that their values are returned automatically.
	int extras[4] = { castling, to_play, hash, ply_counter };

	// this is required in order to be able to undo the move properly, as otherwise the last move is changed and there is no way to get it back.
	for (i = 0; i < 3; i++) {
		previous_last_move[i] = last_move[i];
	}


	num_moves = legal_moves(board, moves, castling, to_play, last_move, piece_list);

	if (depth == 1) {
		answers[0] += num_moves;
		return;
	}

	for (i = 0; i < num_moves; i++) {
		captured = apply(board, moves[i], extras, last_move, piece_list, past_hash_list);
		perft_nodes(board, last_move, extras[0], piece_list, extras[1], extras[2], past_hash_list, extras[3], depth - 1, answers);
		unapply(board, moves[i], extras, previous_last_move, piece_list, captured);
	}
}
