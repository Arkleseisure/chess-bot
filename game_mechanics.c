#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

// Function to confirm that the file is being imported properly
// Last Modified: 7/9/2021
// Last Modified by: Arkleseisure
void confirm_it_works() {
	printf("\n It's working!!!\n");
	printf(" This is game mechanics version 308.\n");
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

struct Game {
	struct Piece piece_list[32];
	unsigned long long board[12];
	unsigned long long hash;
	unsigned long long past_hash_list[100];
	unsigned long long last_move[3];
	int to_play;
	int ply_counter;
	int castling;
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
void add_move(unsigned long long* moves, unsigned long long piece_loc, unsigned long long piece_dest, unsigned long long flag,
	unsigned long long piece, unsigned long long index) {
	moves[0] = piece_loc;
	moves[1] = piece_dest;
	moves[2] = flag + (piece << 4) + (index << 8);
}

/*
Prints an input bitboard to the screen.

Last Modified: 12/9/2021
Last Modified by: Arkleseisure
*/
void print_bitboard(unsigned long long bitboard) {
	int j;
	int k;
	printf("%llx\n", bitboard);
	for (j = 0; j < 8; j++) {
		for (k = 0; k < 8; k++) {
			printf("%d", (bitboard & ((unsigned long long)1 << (8 * (7 - j) + k))) != 0);
		}
		printf("\n");
	}
	printf("\n");
}

/*
Function to print the contents of the board in the form of bitboards, mainly for debugging purposes.

Last Modified: 6/9/2021
Last Modified by: Arkleseisure
*/
void print_board(unsigned long long* board) {
	int i;

	for (i = 0; i < 12; i++) {
		print_bitboard(board[i]);
	}
}

/*
Given a move in bitboard form, returns the equivalent move in format a1a2

Last Modified: 11/9/2021
Last Modified by: Arkleseisure
*/
void get_move_string(unsigned long long* move, char* output_move) {
	output_move[0] = file(move[0]) + 'a';
	output_move[1] = rank(move[0]) + '1';
	output_move[2] = file(move[1]) + 'a';
	output_move[3] = rank(move[1]) + '1';
}

void print_move(unsigned long long* move) {
	char move_string[4];
	char promotions[4] = { 'N', 'B', 'R', 'Q' };
	get_move_string(move, move_string);
	for (int i = 0; i < 4; i++) {
		printf("%c", move_string[i]);
	}
	if ((move[2] & 8) != 0) {
		printf("%c", promotions[move[2] & 3]);
	}
	printf("\n");
}

/*
Checks whether the square held by loc is attacked by the player who isn't to play

Last Modified: 05/9/2021
Last Modified by: Arkleseisure
*/
bool is_attacked(unsigned long long* board, int player_attacked, unsigned long long loc) {
	int i;
	int j;

	// holds 0 if the current player is black, and 6 if white.
	// This can then be added to the number of the piece looked for (pawn = 0, knight = 1, ...) to find the bitboard representing that piece for the other player
	int colour_shift = 6 * (1 - player_attacked);

	// rank of the target square
	int loc_rank = rank(loc);

	// encodes the directions the queen can move in.
	int queen_shifts[] = { 1, 7, 8, 9 };
	int directions[] = { -1, 1 };
	int shift;
	int direction;

	// bitboard holding the positions of all the pieces
	unsigned long long all_pieces = 0;
	for (i = 0; i < 12; i++) {
		all_pieces ^= board[i];
	}

	// first bitboard encodes pieces of the opposite colour which can move horizontally/vertically, and the second diagonally
	unsigned long long movers[2] = { board[3 + colour_shift] ^ board[4 + colour_shift], board[2 + colour_shift] ^ board[4 + colour_shift] };
	unsigned long long new_position;
	int new_rank;

	// loops through each of the eight directions in which it can move
	for (i = 0; i < 8; i++) {
		direction = directions[i / 4];
		shift = queen_shifts[i % 4];

		// loops though moving one square at a time
		for (j = 1; j < 8; j++) {
			new_position = shift_bitboard(loc, shift * j, direction);
			new_rank = rank(new_position);
			// checks whether the loop should be stopped due to going off the side of the board
			// first checks it hasn't gone off the top or bottom of the board
			// second checks it hasn't moved horizontally off the side of the board (when i % 4 == 0, the shift is 1, so we are looking horizontally)
			// third checks it hasn't moved diagonally off the side of the board (the diagonal moves occupy the odd positions in the list)
			if ((new_position == 0) || ((i % 4 == 0) && (new_rank != loc_rank)) || ((i % 2 == 1) && (abs(new_rank - loc_rank) != j))) {
				break;
			}
			// movers holds the bitboard with pieces of the opposite colour able to move in the relevant direction, so the first statement looks for checks from bishops/rooks/queens. 
			// (some_bitboard & new_position) != 0 returns true if the new position has landed on some square present in the bitboard
			else if ((movers[i % 2] & new_position) != 0) {
				return true;
			}
			// Handles attacks from the king
			else if (j == 1 && ((board[5 + colour_shift] & new_position) != 0)) {
				return true;
			}
			// Handles attacks from pawns (if i % 2 == 1, then the direction is diagonal, and if i / 4 == (1 - player_attacked), the direction up/down is correct as well)
			else if (j == 1 && (i % 2 == 1 && i / 4 == (1 - player_attacked) && ((board[colour_shift] & new_position) != 0))) {
				return true;
			}
			// if there is a piece in the way, it breaks
			else if ((new_position & all_pieces) != 0) {
				break;
			}
		}
	}

	// The next section checks for attacks from knights

	// array holding the amount a bit has to be shifted to make each knight move in the positive direction (this is then reflected for the negative)
	int knight_shifts[] = { 6, 10, 15, 17 };
	int rank_changes[] = { 1, 1, 2, 2, 1, 1, 2, 2 };

	// loops through the possible shifts
	for (i = 0; i < 8; i++) {
		new_position = shift_bitboard(loc, knight_shifts[i % 4], directions[i / 4]);

		// if it hasn't gone off the side and there is a knight on that square, then that knight attacks the square
		if (abs(rank(new_position) - loc_rank) == rank_changes[i] && (new_position & board[1 + colour_shift]) != 0) {
			return true;
		}
	}
	return false;
}

/*
Gets all the pseudolegal moves for a pawn at a given location
same_pieces: bitboard containing the pieces of the same colour as the pawn
other_pieces: bitboard containing the pieces of the opposite colour as the pawn
loc: bitboard with the location of the pawn
moves: array to put the moves found into
index: index of the piece in the piece_list
to_play: player to move (0: white, 1: black)
last_move: last move to be played as 2 bitboards, 1 for the starting position, 1 for the finish, 
			and 1 flag containing extra information about the move (for more info see play_game)

Last Modified: 10/8/2021
Last Modified by: Arkleseisure
*/
int get_pawn_moves(unsigned long long same_pieces, unsigned long long other_pieces, unsigned long long loc, unsigned long long moves[28][3], 
				unsigned long long index, int to_play, unsigned long long* last_move) {
	int i;
	int j;
	int num_moves = 0;
	int direction = 1 - 2 * to_play;
	// used for updating the flag when promotion occurs
	unsigned long long original_flag;

	unsigned long long piece_type = 6 * (unsigned long long)to_play;

	// variable holding the rank the pawn is on for ease of calculation for double moves at the start and promotion
	int piece_rank = rank(loc);
	// variable holding the file the pawn is on so that it doesn't skip across the side of the board	
	int piece_file = file(loc);

	// checks if the square in front of the pawn is occupied
	if ((shift_bitboard(loc, 8, direction) & (same_pieces | other_pieces)) == 0) {
		add_move(moves[0], loc, shift_bitboard(loc, 8, direction), 0, piece_type, index);
		num_moves = 1;

		// checks for double moves (first term checks if the rank of the piece is 1 for white or 6 for black (due to indexing from 0), second for a piece on the next square)
		if (piece_rank == to_play * 5 + 1 && ((shift_bitboard(loc, 16, direction) & (same_pieces | other_pieces)) == 0)) {
			add_move(moves[1], loc, shift_bitboard(loc, 16, direction), 1, piece_type, index);
			num_moves = 2;
		}
	}

	// captures... first checks whether there is an opponent's piece on the relevant square, then if it is on a file for which that capture is possible 
	// (as we are using single numbers as bitboards, there is no inbuilt idea of edges of the board)
	if ((shift_bitboard(loc, 7, direction) & other_pieces) != 0 && piece_file != 7 * to_play) {
		add_move(moves[num_moves], loc, shift_bitboard(loc, 7, direction), 4, piece_type, index);
		num_moves++;
	}
	if ((shift_bitboard(loc, 9, direction) & other_pieces) != 0 && piece_file != 7 * (1 - to_play)) {
		add_move(moves[num_moves], loc, shift_bitboard(loc, 9, direction), 4, piece_type, index);
		num_moves++;
	}

	// en passant (if statement translates to: if (last move was a double pawn push) and then the next two check 
	// whether it is to the left or right of the current pawn)
	if ((last_move[2] & 15) == 1) {
		if (last_move[1] >> 1 == loc && piece_file != 7) {
			add_move(moves[num_moves], loc, shift_bitboard(loc, 9 - 2 * to_play, direction), 5, piece_type, index);
			num_moves++;
		}
		else if (last_move[1] << 1 == loc && piece_file != 0) {
			add_move(moves[num_moves], loc, shift_bitboard(loc, 7 + 2 * to_play, direction), 5, piece_type, index);
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
index: index of the piece in the piece_list
to_play: player to move (0: white, 1: black)

Last Modified: 11/8/2021
Last Modified by: Arkleseisure
*/
int get_knight_moves(unsigned long long same_pieces, unsigned long long other_pieces, unsigned long long loc, unsigned long long moves[28][3], 
	unsigned long long index, int to_play) {
	int i;
	int num_moves = 0;
	// array holding the amount a bit has to be shifted to make each knight move in the positive direction (this is then reflected for the negative)
	int knight_shifts[] = { 6, 10, 15, 17 };
	int directions[] = { -1, 1 };
	int rank_changes[] = { 1, 1, 2, 2, 1, 1, 2, 2 };
	int piece_rank = rank(loc);
	
	int piece_type = 1 + 6 * (unsigned long long)to_play;


	unsigned long long new_pos;
	// loops through the possible shifts
	for (i = 0; i < 8; i++) {
		new_pos = shift_bitboard(loc, knight_shifts[i % 4], directions[i / 4]);
		// checks if the move takes it off the side of the board
		if (new_pos != 0 && abs(rank(new_pos) - piece_rank) == rank_changes[i]) {
			// captures
			if ((new_pos & other_pieces) != 0) {
				add_move(moves[num_moves], loc, new_pos, 4, piece_type, index);
				num_moves++;
			}
			// normal moves
			else if ((new_pos & same_pieces) == 0) {
				add_move(moves[num_moves], loc, new_pos, 0, piece_type, index);
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
index: index of the piece in the piece_list
to_play: player to move (0: white, 1: black)

Last Modified: 12/8/2021
Last Modified by: Arkleseisure
*/
int get_bishop_moves(unsigned long long same_pieces, unsigned long long other_pieces, unsigned long long loc, unsigned long long moves[28][3], 
	unsigned long long index, int to_play) {
	int i;
	int j;
	int num_moves = 0;
	int piece_rank = rank(loc);
	int piece_shifts[] = { 7, 9 };
	int directions[] = { -1, 1 };
	unsigned long long new_position;
	unsigned long long piece_type = 2 + 6 * (unsigned long long)to_play;

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
				add_move(moves[num_moves], loc, new_position, 4, piece_type, index);
				num_moves++;
				break;
			}
			add_move(moves[num_moves], loc, new_position, 0, piece_type, index);
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
index: index of the piece in the piece_list
to_play: player to move (0: white, 1: black)

Last Modified: 12/8/2021
Last Modified by: Arkleseisure
*/
int get_rook_moves(unsigned long long same_pieces, unsigned long long other_pieces, unsigned long long loc, unsigned long long moves[28][3], 
	unsigned long long index, int to_play) {
	int i;
	int j;
	int num_moves = 0;
	int piece_rank = rank(loc);
	int piece_shifts[] = { 1, 8 };
	int directions[] = { -1, 1 };
	unsigned long long new_position;
	unsigned long long piece_type = 3 + 6 * (unsigned long long)to_play;

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
				add_move(moves[num_moves], loc, new_position, 4, piece_type, index);
				num_moves++;
				break;
			}
			add_move(moves[num_moves], loc, new_position, 0, piece_type, index);
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
index: index of the piece in the piece_list
to_play: player to move (0: white, 1: black)

Last Modified: 12/8/2021
Last Modified by: Arkleseisure
*/
int get_queen_moves(unsigned long long same_pieces, unsigned long long other_pieces, unsigned long long loc, unsigned long long moves[28][3], 
	unsigned long long index, int to_play) {
	int i;
	int j;
	int num_moves = 0;
	int piece_rank = rank(loc);
	int piece_shifts[] = { 1, 7, 8, 9 };
	int directions[] = { -1, 1 };
	unsigned long long new_position;
	unsigned long long piece_type = 4 + 6 * (unsigned long long)to_play;

	// loops through each of the eight directions in which it can move
	for (i = 0; i < 8; i++) {
		// loops though moving one square at a time
		for (j = 1; j < 8; j++) {
			new_position = shift_bitboard(loc, piece_shifts[i % 4] * j, directions[i / 4]);
			// checks the move doesn't take the piece off the side of the board or onto one of their own pieces
			// first checks it hasn't gone off the top or bottom of the board
			// second checks it hasn't landed on a friendly piece
			// third checks it hasn't moved horizontally off the side of the board (horizontal moves occur when the shift is 1, so i % 4 = 0)
			// fourth checks it hasn't moved diagonally off the side of the board (diagonal moves take up the odd positions in the list)
			if (new_position == 0 || (new_position & same_pieces) != 0 || ((i % 4 == 0) && rank(new_position) != piece_rank) ||
					((i % 2 == 1) && abs(rank(new_position) - piece_rank) != j)) {
				break;
			}
			// if the move is a capture, it adds that move and then breaks to the next loop
			else if ((new_position & other_pieces) != 0) {
				add_move(moves[num_moves], loc, new_position, 4, piece_type, index);
				num_moves++;
				break;
			}
			add_move(moves[num_moves], loc, new_position, 0, piece_type, index);
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
index: index of the piece in the piece_list
to_play: player to move (0: white, 1: black)
board: board position as 12 bitboards (see play_game)

Last Modified: 13/8/2021
Last Modified by: Arkleseisure
*/
int get_king_moves(unsigned long long same_pieces, unsigned long long other_pieces, unsigned long long loc, unsigned long long moves[28][3], 
	unsigned long long index, int to_play, int castling, unsigned long long* board) {
	int i;
	int num_moves = 0;
	int piece_rank = rank(loc);
	int piece_file = file(loc);
	int piece_shifts[] = { 1, 7, 8, 9 };
	int directions[] = { -1, 1 };
	unsigned long long new_position;
	unsigned long long piece_type = 5 + 6 * (unsigned long long)to_play;

	// loops through each of the eight directions in which it can move
	for (i = 0; i < 8; i++) {
		new_position = shift_bitboard(loc, piece_shifts[i % 4], directions[i / 4]);
		// checks the move doesn't take the piece off the side of the board or onto one of their own pieces
		// first checks it hasn't gone off the top or bottom of the board
		// second checks it hasn't landed on a friendly piece
		// third checks it hasn't moved horizontally off the side of the board (horizontal moves occur when the shift is 1, so i % 4 = 0)
		// fourth checks it hasn't moved diagonally off the side of the board (diagonal moves take up the odd positions in the list)
		if (new_position == 0 || (new_position & same_pieces) != 0 || ((i % 4 == 0) && rank(new_position) != piece_rank) ||
			((i % 2 == 1) && abs(rank(new_position) - piece_rank) != 1)) {
		}
		// if the move is a capture, it adds that move and then breaks to the next loop
		else if ((new_position & other_pieces) != 0) {
			add_move(moves[num_moves], loc, new_position, 4, piece_type, index);
			num_moves++;
		}
		else {
			add_move(moves[num_moves], loc, new_position, 0, piece_type, index);
			num_moves++;
		}
	}

	// if the king is in check, then castling isn't legal
	if (castling != 0 && !(is_attacked(board, to_play, loc))) {
		// castling kingside (checks whether the king or rook have moved (held in the castling variable), then verifies the castle would not be moving through check, 
		// then that it would not be moving through pieces... 96 is 01100000, so represents the squares which must be free for the move to be made)
		if (((castling & (2 + 6 * to_play)) != 0) && !(is_attacked(board, to_play, loc << 1)) && (((other_pieces ^ same_pieces) & ((unsigned long long)96 << (56 * to_play))) == 0)) {
			add_move(moves[num_moves], loc, loc << 2, 2, piece_type, index);
			num_moves++;
		}
		// castling queenside
		// generally the same as kingside, but with 00001110 being 14 representing the squares which need to be free
		if (((castling & (1 + 3 * to_play)) != 0) && !(is_attacked(board, to_play, loc >> 1)) && (((other_pieces ^ same_pieces) & ((unsigned long long)14 << (56 * to_play))) == 0)) {
			add_move(moves[num_moves], loc, loc >> 2, 3, piece_type, index);
			num_moves++;
		}
	}
	return num_moves;
}

/*
Function to apply a move to the position
board: board position, holding 12 bitboards, one for each piece type
move: move to be applied to the postion, held in 2 bitboards, one with the start of the move and one with the end, and a flag giving details about the move
extras: [castling, to_play, ply_counter], where castling is a 4 bit int with each bit representing the legality of castling for white or black, kingside or queenside,
			,to_play = 0 (white to play) or 1 (black to play) and ply_counter is the number of ply (half-moves) since the last irreversible move (pawn move/capture/castling),
			and is used for the 50 move rule and to help with the efficiency of draw by repetition
hash: zobrist hash of the position (more explanations found in the c_interface file where most of this stuff is done). Passed as an array as this allows it to be returned automatically.
last_move: last move to be played. This is not used in the function, but it does get changed and having it as an input means that its value gets
			returned automatically
piece_list: list of 32 piece structs (1 for each piece), holding the piece type, location and whether or not that piece has been captured.
past_hash_list: list of all the hashes of past positions back to the last time that there was an irreversible move, such as a capture or pawn move. 
zobrist_numbers: random numbers used to define the hashes, see https://www.chessprogramming.org/Zobrist_Hashing
removed_hash: used to return the hash which was removed from the past_hash_list, so that it can be undone later.

Last Modified: 8/9/2021
Last Modified by: Arkleseisure
*/
int apply(struct Game* game, unsigned long long* move, unsigned long long* zobrist_numbers, unsigned long long* removed_hash) {
	// the piece which moves is encoded within the flag of the move
	int i;
	int index = (int)(move[2] >> 8);
	int piece = (move[2] >> 4) & 15;
	int flag = move[2] & 15;
	int capture_index = 32;

	// whether a move can be reversed or not (used for 50 move rule and to help the efficiency of draw by repetition searches)
	// initialised to whether or not the move is a pawn move
	bool irreversible = (piece % 6 == 0);

	int first_square_rank = rank(move[0]);
	int first_square_file = file(move[0]);
	int second_square_rank = rank(move[1]);
	int second_square_file = file(move[1]);

	// if there was the possibility of en passant this move, then that must be removed from the hash
	// the first statement checks for a double pawn move last move, the second for a pawn of the opposite colour either side of the arrival square
	int last_move_file = file(game->last_move[1]);
	if ((game->last_move[2] & 15) == 1 && 
		(((((game->last_move[1] >> 1) & game->board[6 * game->to_play]) != 0) && last_move_file != 0) || 
			((((game->last_move[1] << 1) & game->board[6 * game->to_play]) != 0) && last_move_file != 7))) {
		game->hash ^= zobrist_numbers[784 + last_move_file];
	}

	// applies the move to the bitboard for that piece
	game->board[piece] ^= move[0] | move[1];
	game->piece_list[index].loc ^= move[0] | move[1];

	// applies the movement of the piece to the hash of the position
	game->hash ^= zobrist_numbers[64 * piece + 8 * first_square_rank + first_square_file];
	game->hash ^= zobrist_numbers[64 * piece + 8 * second_square_rank + second_square_file];

	// if the move is a double pawn move and there is a pawn on one of the sides then the possibility of en passant must be added to the hash
	if (flag == 1 && 
			(((((move[1] >> 1) & game->board[6 * (1 - game->to_play)]) != 0) && first_square_file != 0) ||
			((((move[1] << 1) & game->board[6 * (1 - game->to_play)]) != 0) && first_square_file != 7))) {
		// 784 = 12 * 64 + 16 is the initial index of the en passant files.
		game->hash ^= zobrist_numbers[784 + first_square_file];
	}
	// castling kingside
	else if (flag == 2) {
		// xors the board corresponding to the correct rook (the king will have already been moved) with the binary number 10100000, which will flip
		// the bits in the position that the rook currently is and will move to (as 1 refers to a1). This is then shifted for castling as black by 
		// 7 ranks * 8 squares = 56 squares
		game->board[3 + 6 * game->to_play] ^= ((unsigned long long)(0xA0)) << 56 * game->to_play;
		game->piece_list[13 + 16 * game->to_play].loc ^= ((unsigned long long)(0xA0)) << 56 * game->to_play;
		irreversible = true;

		// applies the movement of the rook to the hash (199 is the index for a white rook on h1, 197 on f1, 440 is difference in index between these and
		// the equivalent black rook positions on f8 and h8.)
		game->hash ^= zobrist_numbers[199 + 440 * game->to_play];
		game->hash ^= zobrist_numbers[197 + 440 * game->to_play];
	}
	// castling queenside
	else if (flag == 3) {
		// same as for kingside castling, but with the binary number 1001 representing the queenside castling transformation
		game->board[3 + 6 * game->to_play] ^= ((unsigned long long)(0x9)) << 56 * game->to_play;
		game->piece_list[12 + 16 * game->to_play].loc ^= ((unsigned long long)(0x9)) << 56 * game->to_play;
		irreversible = true;

		// applies the movement of the rook to the hash (192 is the index for a white rook on a1, 195 on d1, 440 is difference in index between these and
		// the equivalent black rook positions on a8 and c8.)
		game->hash ^= zobrist_numbers[195 + 440 * game->to_play];
		game->hash ^= zobrist_numbers[192 + 440 * game->to_play];
	}
	// en passant
	else if (flag == 5) {
		unsigned long long target_pos;
		if (game->to_play == 0) {
			target_pos = move[1] >> 8;
		}
		else {
			target_pos = move[1] << 8;
		}
		// loops through the pawns in the piece_list to find the one which has been captured
		for (i = (1 - game->to_play) * 16; i < 16 * (1 - game->to_play) + 8; i++) {
			if (!game->piece_list[i].captured && (game->piece_list[i].loc & target_pos) != 0) {
				game->piece_list[i].captured = true;
				game->board[game->piece_list[i].type] ^= target_pos;
				capture_index = i;
				break;
			}
		}

		// updates the hash... the piece taken will be on the same rank as the first part of the move and the same file as the second.
		game->hash ^= zobrist_numbers[64 * game->piece_list[capture_index].type + 8 * first_square_rank + second_square_file];
	}
	// other captures
	else if ((flag & 4) != 0) {
		// loops through the piece_list to find the piece which has been captured.
		for (i = (1 - game->to_play) * 16; i < 16 * (1 - game->to_play) + 16; i++) {
			if (!game->piece_list[i].captured && (game->piece_list[i].loc & move[1]) != 0) {
				game->piece_list[i].captured = true;
				game->board[game->piece_list[i].type] ^= move[1];
				capture_index = i;
				irreversible = true;

				// removes queenside castling for white if the rook on a1 is taken
				if (game->piece_list[i].type == 3 && move[1] == (unsigned long long)1) {
					game->castling &= 14;
				}
				// removes kingside castling for white if the rook on h1 is taken
				else if (game->piece_list[i].type == 3 && move[1] == (unsigned long long)1 << 7) {
					game->castling &= 13;
				}
				// removes queenside castling for black if the rook on a8 is taken
				else if (game->piece_list[i].type == 9 && move[1] == (unsigned long long)1 << 56) {
					game->castling &= 11;
				}
				// removes kingside castling for black if the rook on h8 is taken
				else if (game->piece_list[i].type == 9 && move[1] == (unsigned long long)1 << 63) {
					game->castling &= 7;
				}

				// updates the hash
				game->hash ^= zobrist_numbers[64 * game->piece_list[i].type + 8 * second_square_rank + second_square_file];
				break;
			}
		}
	}

	// handles pawn promotion
	if ((flag & 8) != 0) {
		// flag & 3 gives 0 for knight, 1 for bishop, 2 for rook, 3 for queen... + 1 + 6 * to_play transforms that to the actual type of piece. 
		int promotion_type = (flag & 3) + 1 + 6 * game->to_play;

		// removes the pawn
		game->board[piece] ^= move[1];
		// adds the new piece
		game->board[promotion_type] ^= move[1];
		// changes the type of the piece in the piece_list
		game->piece_list[index].type = promotion_type;

		// updates the hash
		game->hash ^= zobrist_numbers[64 * piece + 8 * second_square_rank + second_square_file];
		game->hash ^= zobrist_numbers[64 * promotion_type + 8 * second_square_rank + second_square_file];
	}

	// changes the value of the castling variable (as it is extras[0], to return it the value of extras needs to be changed)
	if (game->castling != 0) {
		// 768 = 12 * 64 is the starting index of the zobrist numbers relating to castling
		game->hash ^= zobrist_numbers[768 + game->castling];

		// removes castling king/queenside for white if the white king has moved
		if (piece == 5) {
			game->castling &= 12;
		}
		// removes castling king/queenside for black if the black king has moved
		else if (piece == 11) {
			game->castling &= 3;
		}
		// removes queenside castling for white if the rook on a1 moves
		else if (piece == 3 && move[0] == (unsigned long long)1) {
			game->castling &= 14;
		}
		// removes kingside castling for white if the rook on h1 moves
		else if (piece == 3 && move[0] == (unsigned long long)1 << 7) {
			game->castling &= 13;
		}
		// removes queenside castling for black if the rook on a8 moves
		else if (piece == 9 && move[0] == (unsigned long long)1 << 56) {
			game->castling &= 11;
		}
		// removes kingside castling for black if the rook on h8 moves
		else if (piece == 9 && move[0] == (unsigned long long)1 << 63) {
			game->castling &= 7;
		}
		game->hash ^= zobrist_numbers[768 + game->castling];

	}

	// changes the player
	game->to_play = 1 - game->to_play;
	game->hash ^= zobrist_numbers[792];

	// sets the last move to the move which has just been applied
	for (int i = 0; i < 3; i++) {
		game->last_move[i] = move[i];
	}

	// changes the value of the ply_counter, which accounts for the 50 move rule
	if (irreversible) {
		game->ply_counter = 0;
	}
	else {
		game->ply_counter++;
	}

	// records the removed hash so that the past_hash_list can be returned to its original state if the move is unapplied.
	removed_hash[0] = game->past_hash_list[game->ply_counter];
	// record of the hashes of past games, used for draw by repetition
	game->past_hash_list[game->ply_counter] = game->hash;

	return capture_index;
}

/*
board: board position, holding 12 bitboards, one for each piece type
move: move to be unapplied to the postion, held in 2 bitboards, one with the start of the move and one with the end, and a flag giving details about the move
extras: [castling, to_play, ply_counter], where castling is a 4 bit int with each bit representing the legality of castling for white or black, kingside or queenside,
			,to_play = 0 (white to play) or 1 (black to play) and ply_counter is the number of ply (half-moves) since the last irreversible move (pawn move/capture/castling),
			and is used for the 50 move rule and to help with the efficiency of draw by repetition
hash: zobrist hash of the position... This is passed in so that it can be changed and returned automatically (more explanations about zobrist stuff found in the c_interface file where most of the zobrist things are done).
removed_hash: hash which was replaced at the start of the past_hash_list if the previous move wasn't reversible. This has to be put back to return the past_hash_list to its original state. 
piece_list: list of 32 piece structs (1 for each piece), holding the piece type, location and whether or not that piece has been captured.
captured: number of the piece which was captured (0 for white pawn, 1 for white knight, 2 for white bishop, ... documented in play_game)
previous_castling: value of the castling variable before the move was applied.
previous_ply_counter: value of the ply_counter before the move was applied.
past_hash_list: list of all the hashes of past positions back to the last time that there was an irreversible move, such as a capture or pawn move.
zobrist_numbers: random numbers used to define the hashes, see https://www.chessprogramming.org/Zobrist_Hashing
last_move: last_move to be played, same format as move.
previous_last_move: the last_move from before move was applied, meaning that the last move variable can be returned to its previous state.

Last Modified: 9/9/2021
Last Modified by: Arkleseisure
*/
void unapply(struct Game* game, unsigned long long* move, unsigned long long removed_hash, int capture_index, int previous_castling, 
	int previous_ply_counter, unsigned long long* previous_last_move) {
	int i;
	int index = (int)(move[2] >> 8);
	// the piece which moves is encoded within the flag of the move
	int piece = (move[2] >> 4) & 15;
	int flag = move[2] & 15;
	// as the move has already been applied, the to_play variable now holds the opponent instead of the player who's move we're undoing.
	game->to_play = 1 - game->to_play;

	// unapplies the move to the bitboard for that piece
	game->board[piece] ^= move[0] | move[1];
	game->piece_list[index].loc ^= move[0] | move[1];

	// castling kingside
	if (flag == 2) {
		// xors the board corresponding to the correct rook (the king will have already been moved) with the binary number 10100000, which will flip
		// the bits in the position that the rook currently is and was originally (as 1 refers to a1). This is then shifted for castling as black by 
		// 7 ranks * 8 squares = 56 squares
		game->board[3 + 6 * game->to_play] ^= ((unsigned long long)(0xA0)) << 56 * game->to_play;
		game->piece_list[13 + 16 * game->to_play].loc ^= ((unsigned long long)(0xA0)) << 56 * game->to_play;
	}
	// castling queenside
	else if (flag == 3) {
		// same as for kingside castling, but with the binary number 1001 representing the queenside castling transformation
		game->board[3 + 6 * game->to_play] ^= ((unsigned long long)(0x9)) << 56 * game->to_play;
		game->piece_list[12 + 16 * game->to_play].loc ^= ((unsigned long long)(0x9)) << 56 * game->to_play;
	}
	// replaces captured pieces
	else if ((flag & 4) != 0) {
		game->piece_list[capture_index].captured = false;
		game->board[game->piece_list[capture_index].type] ^= game->piece_list[capture_index].loc;
	}

	// handles pawn promotion
	if ((flag & 8) != 0) {
		int promotion_type = (flag & 3) + 1 + 6 * game->to_play;

		// the pawn will have reappeared on both its starting and finishing square as it is undone, 
		// so it needs to be removed from its finishing square again.
		game->board[piece] ^= move[1];
		// removes the promoted piece
		game->board[promotion_type] ^= move[1];
		// changes the type of the piece in the piece_list back to a pawn
		game->piece_list[index].type = piece;
	}

	for (i = 0; i < 3; i++) {
		game->last_move[i] = previous_last_move[i];
	}
	
	// undoes the changes to the castling, ply_counter, hash and past_hash_list variables.
	game->castling = previous_castling;
	game->past_hash_list[game->ply_counter] = removed_hash;
	game->ply_counter = previous_ply_counter;
	game->hash = game->past_hash_list[game->ply_counter];
}

/*
Function which applies the critical changes to the position such that they can be undone quickly, making doing and undoing single moves more efficient
board and move take the same form as they do in apply. Note: does not include pawn promotion
Last Modified: 11/08/2021
Last Modified by: Arkleseisure
*/
int quick_apply(unsigned long long* board, unsigned long long* move, int to_play, struct Piece* piece_list) {
	// breaks the last part of the move down into the bit describing the piece, and the bit describing the type of move, or flag
	int piece = (move[2] >> 4) & 15;
	int flag = move[2] & 15;
	int captured = -1;

	// applies the move to the bitboard for that piece
	board[piece] ^= move[0] | move[1];

	// if the king moves, updates his position as this is the only part of the piece_list which may be used after a quick_apply.
	if (piece % 6 == 5) {
		piece_list[15 + 16 * to_play].loc ^= move[0] | move[1];
	}

	// applies captures
	// en passant for white
	if (flag == 5 && to_play == 0) {
		board[6] ^= (move[1] >> 8);
		captured = 6;
	}
	// en passant for black
	else if (flag == 5 && to_play == 1) {
		board[0] ^= (move[1] << 8);
		captured = 0;
	}
	// other captures
	else if ((flag & 4) != 0) {
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
		board[3 + 6 * to_play] ^= ((unsigned long long)(0xA0)) << 56 * to_play;
	}
	// castling queenside
	else if (flag == 3) {
		// same as for kingside castling, but with the binary number 1001 representing the queenside castling transformation
		board[3 + 6 * to_play] ^= ((unsigned long long)(0x9)) << 56 * to_play;
	}

	// returns the type of piece which has been captured, allowing this capture to be undone if required
	return captured;
}

/*
Function which undoes the changes made by the quick_apply function.
Last Modified: 10/08/2021
Last Modified by: Arkleseisure
*/
void quick_undo(unsigned long long* board, unsigned long long* move, int to_play, int captured, struct Piece* piece_list) {
	// breaks the last part of the move down into the index (of the piece moving in the piece_list), the piece type and the flag (all documented in play_game)
	int index = (int)move[2] >> 8;
	int piece = (move[2] >> 4) & 15;
	int flag = move[2] & 15;

	// undoes the move to the bitboard for that piece
	board[piece] ^= move[0] | move[1];

	// if the king moves, updates his position as this is the only part of the piece_list which is changed by quick_apply.
	if (piece % 6 == 5) {
		piece_list[15 + 16 * to_play].loc ^= move[0] | move[1];
	}

	// undoes captures
	// en passant for white
	if (flag == 5 && to_play == 0) {
		board[6] ^= (move[1] >> 8);
	}
	// en passant for black
	else if (flag == 5 && to_play == 1) {
		board[0] ^= (move[1] << 8);
	}
	// other captures
	else if ((flag & 4) != 0) {
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
int legal_moves(struct Game* game, unsigned long long legal_moves[220][3]) {
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
	int p = 6 * game->to_play;
	int other_p = 6 * (1 - game->to_play);

	// holds the pseudolegal moves for one piece
	unsigned long long piece_legal_moves[28][3];

	// precalculated bitboards with the locations of each of the pieces for each colour
	unsigned long long other_pieces = 0;
	unsigned long long same_pieces = 0;

	for (i = 0; i < 6; i++) {
		other_pieces ^= game->board[i + other_p];
		same_pieces ^= game->board[i + p];
	}


	// loops through each of the pieces for the side to play, to generate the legal moves for each of them.
	for (i = game->to_play * 16; i < 16 + game->to_play * 16; i++) {
		if (!game->piece_list[i].captured) {
			// generates the legal moves for that piece depending on its piece type.
			switch (game->piece_list[i].type % 6) {
			case 0:
				piece_moves = get_pawn_moves(same_pieces, other_pieces, game->piece_list[i].loc, piece_legal_moves, i, game->to_play, game->last_move);
				break;
			case 1:
				piece_moves = get_knight_moves(same_pieces, other_pieces, game->piece_list[i].loc, piece_legal_moves, i, game->to_play);
				break;
			case 2:
				piece_moves = get_bishop_moves(same_pieces, other_pieces, game->piece_list[i].loc, piece_legal_moves, i, game->to_play);
				break;
			case 3:
				piece_moves = get_rook_moves(same_pieces, other_pieces, game->piece_list[i].loc, piece_legal_moves, i, game->to_play);
				break;
			case 4:
				piece_moves = get_queen_moves(same_pieces, other_pieces, game->piece_list[i].loc, piece_legal_moves, i, game->to_play);
				break;
			case 5:
				piece_moves = get_king_moves(same_pieces, other_pieces, game->piece_list[i].loc, piece_legal_moves, i, game->to_play, game->castling, game->board);
				break;
			}

			// loops through each of the moves generated to see if it's in check.
			for (j = 0; j < piece_moves; j++) {
				// move is applied to the position efficiently (factors which don't change whether the resulting position is check aren't applied)
				captured = quick_apply(game->board, piece_legal_moves[j], game->to_play, game->piece_list);

				// if the resulting position is not check, the move is added to the array of legal moves. (the position passed in is that of the king of the relevant colour)
				if (!(is_attacked(game->board, game->to_play, game->piece_list[15 + 16 * game->to_play].loc))) {
					// the index and piece are already included within the flag so we don't need to add them.
					add_move(legal_moves[num_moves], piece_legal_moves[j][0], piece_legal_moves[j][1], piece_legal_moves[j][2], 0, 0);
					num_moves++;
				}

				// the move is undone from the position
				quick_undo(game->board, piece_legal_moves[j], game->to_play, captured, game->piece_list);
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
	int num_minor_pieces = 0;
	int num_bishops[2] = { 0, 0 };
	int bishop_rank;
	int bishop_file;
	int bishop_type;

	for (i = 0; i < 32; i++) {
		if (!piece_list[i].captured) {
			switch (piece_list[i].type % 6) {
			// a pawn is always enough material to win
			case 0:
				return false;
			// a knight will be enough to win if there is at least another piece on the board
			case 1:
				num_minor_pieces++;
				if (num_minor_pieces >= 2) {
					return false;
				}
				break;
			// a bishop will be enough to win if there is at least another piece on the board which is not a bishop of the same colour
			case 2:
				num_minor_pieces++;
				bishop_rank = rank(piece_list[i].loc);
				bishop_file = file(piece_list[i].loc);
				if (bishop_rank % 2 == bishop_file % 2) {
					num_bishops[0]++;
					bishop_type = 0;
				}
				else {
					num_bishops[1]++;
					bishop_type = 1;
				}

				if (num_minor_pieces >= 2 && num_minor_pieces != num_bishops[bishop_type]) {
					return false;
				}
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
				captured = quick_apply(board, piece_legal_moves[j], to_play, piece_list);

				// if the resulting position is not check, then it is neither checkmate nor stalemate
				if (!(is_attacked(board, to_play, piece_list[15 + 16 * to_play].loc))) {
					quick_undo(board, piece_legal_moves[j], to_play, captured, piece_list);
					return 0;
				}

				// the move is undone from the position
				quick_undo(board, piece_legal_moves[j], to_play, captured, piece_list);
			}
		}
	}

	// if there are no moves and it is check, then it is checkmate, if not then it is stalemate
	if (is_attacked(board, to_play, piece_list[15 + 16 * to_play].loc)) {
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
int terminal(struct Game* game) {
	int i;
	int repetitions = 0;
	int mates;

	// checks if the 50 move rule has been reached
	if (game->ply_counter >= 100) {
		return 1;
	}
	// looks for repetitions... After the 50 move rule has been reset, no repetitions of positions before this point can occur, 
	// and threefold repetition can only occur at least 3 moves after the reset, so we check that the move counter is at least 4 for efficiency purposes
	else if (game->ply_counter >= 8) {
		// it is only a repetition if it is the same player to play, and so we only look at the moves where it is this player to play
		for (i = game->ply_counter % 2; i < game->ply_counter; i += 2) {
			// increments the repetition counter if this position is a repetition of the past board
			if (game->hash == game->past_hash_list[i]) {
				repetitions++;
				// if this position has occured 2 times in the past then this is the third repetition and so it is a draw
				if (repetitions == 2) {
					return 1;
				}
			}
		}
	}

	// gets draw by lack of material
	if (draw_by_lack_of_material(game->piece_list)) {
		return 1;
	}

	// look for mates function returns 0 by default, 1 if checkmate and 2 if stalemate
	mates = look_for_mates(game->board, game->to_play, game->last_move, game->piece_list);
	switch (mates) {
	case 0:
		return 3;
	case 1:
		return 2 * game->to_play;
	case 2:
		return 1;
	}

	return 3;
}

/*
Perft function: calculates the number of nodes, captures, en passant captures, castles, promotions, checks and checkmates at a certain depth. 
These values are then compared to generally accepted values in order to check the functioning of the game mechanics code.

Last Modified: 9/9/2021
Last Modified by: Arkleseisure
*/
void perft_all(struct Game* game, unsigned long long* answers, unsigned long long* zobrist_numbers, int depth) {
	// checks if the position is at the final depth. if so, it adds the values to the answers, the order of these being: 
	// Nodes, Captures, En passant, Castling, Promotion, Checks, Checkmates
	if (depth == 0) {
		answers[0]++;
		// captures are noted in the 3rd bit of the flag
		if ((game->last_move[2] & 4) != 0) {
			answers[1]++;
			// denotes the previous move being en passant
			if ((game->last_move[2] & 15) == 5) {
				answers[2]++;
			}
		}

		// checks for kingside or queenside castling
		if (((game->last_move[2] & 15) == 2) || ((game->last_move[2] & 15) == 3)) {
			answers[3]++;
		}

		// looks for promotions
		if ((game->last_move[2] & 8) != 0) {
			answers[4]++;
		}

		// looks for checks
		if (is_attacked(game->board, game->to_play, game->piece_list[15 + 16 * game->to_play].loc)) {
			answers[5]++;

			if (look_for_mates(game->board, game->to_play, game->last_move, game->piece_list) != 0) {
				answers[6]++;
			}
		}
		return;
	}

	unsigned long long moves[220][3];
	unsigned long long previous_last_move[3];
	int previous_ply_counter = game->ply_counter;
	int previous_castling = game->castling;
	int num_moves;
	int captured;
	int i;
	// single variables are passed as arrays so that their values are returned automatically.
	unsigned long long removed_hash[1] = { 0 };

	// this is required in order to be able to undo the move properly, as otherwise the last move is changed and there is no way to get it back.
	for (i = 0; i < 3; i++) {
		previous_last_move[i] = game->last_move[i];
	}

	num_moves = legal_moves(game, moves);

	for (i = 0; i < num_moves; i++) {
		captured = apply(game, moves[i], zobrist_numbers, removed_hash);
		perft_all(game, answers, zobrist_numbers, depth - 1);
		unapply(game, moves[i], removed_hash[0], captured, previous_castling, previous_ply_counter, previous_last_move);
	}
}

/* 
Same as perft_all, but doesn't return all the debugging statistics and is instead more streamlined and used for benchmarking

Last Modified: 9/9/2021
Last Modified by: Arkleseisure
*/
void perft_nodes(struct Game* game, int depth, unsigned long long* answers, unsigned long long* zobrist_numbers) {
	unsigned long long moves[220][3];
	unsigned long long previous_last_move[3];
	int previous_ply_counter = game->ply_counter;
	int previous_castling = game->castling;
	int num_moves;
	int captured;
	int i;
	// single variables are passed as arrays so that their values are returned automatically.
	unsigned long long removed_hash[1];

	// this is required in order to be able to undo the move properly, as otherwise the last move is changed and there is no way to get it back.
	for (i = 0; i < 3; i++) {
		previous_last_move[i] = game->last_move[i];
	}


	num_moves = legal_moves(game, moves);

	if (depth == 1) {
		answers[0] += num_moves;
		return;
	}

	for (i = 0; i < num_moves; i++) {
		captured = apply(game, moves[i], zobrist_numbers, removed_hash);
		perft_nodes(game, depth - 1, answers, zobrist_numbers);
		unapply(game, moves[i], removed_hash[0], captured, previous_castling, previous_ply_counter, previous_last_move);
	}
}
