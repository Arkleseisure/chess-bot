#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include "game_mechanics_v1.c"

float values[12] = { 1, 3, 3, 5, 9, 4, -1, -3, -3, -5, -9, -4 };

float max_np_material = 70;

// Piece-square tables are used to encourage the engine to move the pieces to squares where they will likely be active.
// The squares where pieces are useful are different in the middlegame and the endgame, so different tables are used, and then the 
// actual evaluation is a linear combination depending on the stage of the game.
float piece_square_table_mg[6][8][8] = { {
		// Pawns (note top left is a1, top right is a8, so visually this is equivalent to looking at the piece square table for black from the 
		// white side
		{0, 0, 0, 0, 0, 0, 0, 0},
		{5, 10, 10, -20, -20,  10, 10, 5},
		{5,  5,  0,   0,   0, -15,  5, 5},
		{0,  0,  5,  20,  20,  -5,  0, 0},
		{5,  5, 10,  25,  25,  10,  5, 5},
		{10, 10, 15, 30,  30, 15, 10, 10},
		{50, 50, 50, 50,  50, 50, 50, 50},
		{0, 0, 0, 0, 0, 0, 0, 0}},
		// Knights
		{{-50, -30, -20, -10, -10, -20, -30, -50},
		{-30, -15,  -5,   0,   0,  -5, -15, -30},
		{-20,   5,  10,   0,   0,  10,   5, -20},
		{-10,   0,   5,  20,  20,   5,   0, -10},
		{-10,   0,   5,  20,  20,   5,   0, -10},
		{-20,   0,   5,  15,  15,   5,   0, -20},
		{-30, -15,  -5,   0,   0,  -5, -15, -30},
		{-50, -30, -20, -10, -10, -20, -30, -50}},
		// Bishops
		{{-20, -10, -10, -10, -10, -10, -10, -20},
		{-10,   10,   0,   5,   5,   0,  10, -10},
		{-10,    0,   5,   5,   5,   5,   0, -10},
		{-10,    5,  10,  10,  10,  10,   5, -10},
		{-10,   10,  10,  10,  10,  10,  10, -10},
		{-10,    0,   5,   5,   5,   5,   0, -10},
		{-10,    0,   0,   0,   0,   0,   0, -10},
		{-20, -10, -10, -10, -10, -10, -10, -20}},
		// Rooks
		{{0,  0, 0, 5, 5, 0, 0,   0},
		{-10, 0, 0, 0, 0, 0, 0, -10},
		{-10, 0, 0, 0, 0, 0, 0, -10},
		{-10, 0, 0, 0, 0, 0, 0, -10},
		{-10, 0, 0, 0, 0, 0, 0, -10},
		{-10, 0, 0, 0, 0, 0, 0, -10},
		{10, 20, 20, 20, 20, 20, 20, 10},
		{0, 0, 0, 0, 0, 0, 0, 0}},
		// Queens
		{{-20, -10, -10, -5, -5, -10, -10, -20},
		{-10, 0, 0, 0, 0, 0, 0, -10},
		{-10, 0, 0, 0, 0, 0, 0, -10},
		{-5, 0, 0, 0, 0, 0, 0, -5},
		{-5, 0, 0, 0, 0, 0, 0, -5},
		{-10, 0, 0, 0, 0, 0, 0, -10},
		{-10, 0, 0, 0, 0, 0, 0, -10},
		{-20, -10, -10, -5, -5, -10, -10, -20}},
		// King
		{{20, 30, 25, -20, -20, -10, 30, 25},
		{  5,  5,  0, -10, -10,   0,  5,  5},
		{-20, -20, -20, -20, -20, -20, -20, -20},
		{-30, -30, -30, -30, -30, -30, -30, -30},
		{-40, -40, -40, -40, -40, -40, -40, -40},
		{-50, -50, -50, -50, -50, -50, -50, -50},
		{-60, -60, -60, -60, -60, -60, -60, -60},
		{-70, -70, -70, -70, -70, -70, -70, -70}} };

float piece_square_table_eg[6][8][8] = {
	// Pawns
	{{0, 0, 0, 0, 0, 0, 0, 0},
	{10,  5,  0,  0,  0,  0,  5, 10},
	{15, 10,  5,  5,  5,  5, 10, 15},
	{20, 15, 10, 10, 10, 10, 15, 20},
	{40, 35, 30, 30, 30, 30, 35, 40},
	{60, 55, 50, 50, 50, 50, 55, 60},
	{80, 75, 70, 70, 70, 70, 75, 80},
	{0, 0, 0, 0, 0, 0, 0, 0} },
	// Knights
	{ {-50, -30, -20, -10, -10, -20, -30, -50},
	{-30, -15,  -5,   0,   0,  -5, -15, -30},
	{-20,  -5,  10,  15,  15,  10,   5, -20},
	{-10,   0,  15,  20,  20,  15,   0, -10},
	{-10,   0,  15,  20,  20,  15,   0, -10},
	{-20,  -5,  10,  15,  15,  10,  -5, -20},
	{-30, -15,  -5,   0,   0,  -5, -15, -30},
	{-50, -30, -20, -10, -10, -20, -30, -50} },
	// Bishops
	{ {-20, -10, -10, -10, -10, -10, -10, -20},
	{-10,    0,   0,   0,   0,   0,   0, -10},
	{-10,    0,   5,   5,   5,   5,   0, -10},
	{-10,    0,   5,  10,  10,   5,   0, -10},
	{-10,    0,   5,  10,  10,   5,   0, -10},
	{-10,    0,   5,   5,   5,   5,   0, -10},
	{-10,    0,   0,   0,   0,   0,   0, -10},
	{-20, -10, -10, -10, -10, -10, -10, -20} },
	// Rooks
	{ {0,  0, 0, 5, 5, 0, 0,   0},
	{-10, 0, 0, 0, 0, 0, 0, -10},
	{-10, 0, 0, 0, 0, 0, 0, -10},
	{-10, 0, 0, 0, 0, 0, 0, -10},
	{-10, 0, 0, 0, 0, 0, 0, -10},
	{-10, 0, 0, 0, 0, 0, 0, -10},
	{10, 20, 20, 20, 20, 20, 20, 10},
	{0, 0, 0, 0, 0, 0, 0, 0} },
	// Queens
	{ {-20, -10, -10, -5, -5, -10, -10, -20},
	{-10, 0, 0, 0, 0, 0, 0, -10},
	{-10, 0, 0, 0, 0, 0, 0, -10},
	{-5, 0, 0, 0, 0, 0, 0, -5},
	{-5, 0, 0, 0, 0, 0, 0, -5},
	{-10, 0, 0, 0, 0, 0, 0, -10},
	{-10, 0, 0, 0, 0, 0, 0, -10},
	{-20, -10, -10, -5, -5, -10, -10, -20} },
	// King
	{ {-40, -20, -20, -20, -20, -20, -20, -40},
	{-20, -10,  -5,  -5,  -5,  -5, -10, -20},
	{-20,  -5,   0,  10,  10,   0,  -5, -20},
	{-20,  -5,  10,  30,  30,  10,  -5, -20},
	{-20,  -5,  10,  30,  30,  10,  -5, -20},
	{-20,  -5,   0,  10,  10,   0,  -5, -20},
	{-20, -10,  -5,  -5,  -5,  -5, -10, -20},
	{-40, -20, -20, -20, -20, -20, -20, -40} } };

/*
Used to verify at the start of the program that the code being used is indeed the latest version, and that it has imported the correct version of game_mechanics
Last Modified: 17/9/2021
Last Modified by: Arkleseisure
*/
void check_it_works() {
	confirm_it_works();
	printf(" This is the engine code version 144.\n");
	printf(" This is engine v5.\n");
}

/*
Function used to evaluate the value of a given position
Last Modified: 24/12/2021
Last Modified by: Arkleseisure
*/
float evaluate(struct Game* game) {
	// returns the value with a small random element so that the engine plays differently each time.
	return game->value + ((float)rand() / RAND_MAX) / 5 - 0.1;
}

/*
Gets the value of a piece on a square by interpolating between the middlegame and endgame piece_square tables according to
the current amount of non-pawn material (if there are 8 pawns on the board and no pieces, it is still very much considered an endgame).
Last Modified: 01/01/2022
Last Modified by: Arkleseisure

*/
float get_psqt_value(int type, int rank, int file, float current_np_material) {
	if (current_np_material > max_np_material) {
		current_np_material = max_np_material;
	}
	return (current_np_material * piece_square_table_mg[type][rank][file] +
		(max_np_material - current_np_material) * piece_square_table_eg[type][rank][file]) / (100 * max_np_material);
}

/*
Function used to get the initial value for the game state at the start of each move.
Last Modified: 24/12/2021
Last Modified by: Arkleseisure
*/
void fully_evaluate(struct Game* game) {
	int i;
	game->value = 0;
	game->current_np_material = 0;
	int piece_rank;
	int piece_file;
	struct Piece piece;
	// loops through the piece_list, adding the values of each piece in the position.
	for (i = 0; i < 32; i++) {
		if (!(game->piece_list[i].captured)) {
			piece = game->piece_list[i];
			game->value += values[piece.type];
			if ((piece.type % 6) != 0) {
				game->current_np_material += values[piece.type % 6];
			}
		}
	}
	for (i = 0; i < 16; i++) {
		if (!(game->piece_list[i].captured)) {
			piece = game->piece_list[i];
			piece_rank = rank(piece.loc);
			piece_file = file(piece.loc);
			game->value += get_psqt_value(piece.type, piece_rank, piece_file, game->current_np_material);
		}
		if (!(game->piece_list[i + 16].captured)) {
			piece = game->piece_list[i + 16];
			piece_rank = rank(piece.loc);
			piece_file = file(piece.loc);
			game->value -= get_psqt_value(piece.type - 6, 7 - piece_rank, piece_file, game->current_np_material);
		}
	}
}

/*
Updates the parts of the evaluation which are incrementally updated for efficiency reasons.
Last Modified: 24/12/2021
Last Modified by: Arkleseisure
*/
void update_value(struct Game* game, unsigned long long move[3], int captured_piece) {
	// handles promotions
	if ((move[2] & 8) != 0) {
		struct Piece piece;
		// gets the piece which is to be promoted to (last 2 bits encode promotion type... 0=N, 1=B, 2=R, 3=Q).
		// Note we use 1 - game->to_play because the apply function has just happened.
		int prom_piece = (move[2] & 3) + 1 + 6 * (1 - game->to_play);
		// adds the value for the promotion piece, and subtracts the value of the pawn
		game->value += values[prom_piece];
		game->value -= values[6 * (1 - game->to_play)];

		if (captured_piece != 32) {
			piece = game->piece_list[captured_piece];
			game->value -= values[piece.type];
		}

		int file1 = file(move[0]);
		int file2 = file(move[1]);
		if (game->to_play == 1) {
			game->value -= get_psqt_value(0, 6, file1, game->current_np_material);
			if (captured_piece != 32) {
				game->value += get_psqt_value(piece.type - 6, 7, file(piece.loc), game->current_np_material);
				if (piece.type % 6 != 0) {
					game->current_np_material -= values[piece.type % 6];
				}
			}
			game->current_np_material += values[prom_piece];
			game->value += get_psqt_value(prom_piece, 7, file2, game->current_np_material);
		}
		else {
			game->value += get_psqt_value(0, 1, file1, game->current_np_material);
			if (captured_piece != 32) {
				game->value += get_psqt_value(piece.type, 0, file(piece.loc), game->current_np_material);
				if (piece.type % 6 != 0) {
					game->current_np_material -= values[piece.type % 6];
				}
			}
			game->current_np_material -= values[prom_piece];
			game->value -= get_psqt_value(prom_piece % 6, 0, file2, game->current_np_material);
		}

	}
	// handles captures
	else if (captured_piece != 32) {
		struct Piece piece = game->piece_list[captured_piece];
		game->value -= values[piece.type];
		if (game->to_play == 1) {
			game->value += get_psqt_value(piece.type - 6, 7 - rank(piece.loc), file(piece.loc), game->current_np_material);
		}
		else {
			game->value -= get_psqt_value(piece.type, rank(piece.loc), file(piece.loc), game->current_np_material);
		}
		if (piece.type % 6 != 0) {
			game->current_np_material -= values[piece.type % 6];
		}
	}
	else {
		int piece_type = (move[2] >> 4) & 15;
		piece_type %= 6;
		int rank1 = rank(move[0]);
		int file1 = file(move[0]);
		int rank2 = rank(move[1]);
		int file2 = file(move[1]);
		if (game->to_play == 1) {
			game->value -= get_psqt_value(piece_type, rank1, file1, game->current_np_material);
			game->value += get_psqt_value(piece_type, rank2, file2, game->current_np_material);
		}
		else {
			game->value += get_psqt_value(piece_type, 7 - rank1, file1, game->current_np_material);
			game->value -= get_psqt_value(piece_type, 7 - rank2, file2, game->current_np_material);
		}
	}
}

/*
Function which performs a minimax search on the position.
More info here: https://www.chessprogramming.org/Minimax

INPUTS:
game: Game struct holding the position to calculate from
depth: depth to calculate the minimax value to from this position
zobrist_numbers: numbers used to calculate the values of the hashes of positions, used for draw by repetition.
move_number: index of the best move in the position in the output array of the legal_moves function. This is then changed within the function so as for it to be returned automatically.
start_time: time at which the calculation was started (given in terms of clock tick since the start of some local era, often the start of the program).
time_allowed: time in seconds which the engine has to make its move.
max_depth: total depth to which the computer is calculating its result to. Different from depth as the function is recursive, and so max_depth is constant throughout the layers, while
		depth just holds the depth yet to calculate.
alpha: highest value that white can guarantee themselves higher up the tree... Used to help efficiency of search. See https://www.chessprogramming.org/Alpha-Beta
beta: equivalent of alpha, but for black: lowest value black can guarantee themselves higher up the tree.

OUTPUT:
value: value of the position as a float. This value is, as with most traditional engines in terms of pawns of advantage, with + being good for white, - good for black.

Last Modified: 22/9/2021
Last Modified by: Arkleseisure
*/
float minimax(struct Game* game, int depth, unsigned long long* zobrist_numbers, int move_number[1], clock_t start_time, double time_allowed, int max_depth, float alpha, float beta, unsigned long long nodes[1]) {
	nodes[0]++;
	if ((((double)clock() - (double)start_time) / CLOCKS_PER_SEC) > time_allowed) {
		return 0;
	}
	// once the depth hits 0, the tree is exited
	if (depth == 0) {
		return evaluate(game);
	}

	int value = terminal(game);
	// terminal gives 0 for a win for black, 1 for a draw, 2 for a win for white and 3 for no result.
	if (value == 1) {
		return 0;
	}
	// if the position is mate, then it returns a very high value, which is slightly higher if it is at lower depth  
	// (as the depth variable holds the depth yet to search as opposed to the current depth, the sign is the same as that of the returned value) to reward shorter mates
	else if (value == 0) {
		return (float)(-1000 - depth);
	}
	else if (value == 2) {
		return (float)(1000 + depth);
	}

	int i;
	unsigned long long moves[220][3];
	int num_moves = legal_moves(game, moves);
	unsigned long long removed_hash[1];
	int child_move_number[1] = { 0 };

	// initializes the value of this node to a really bad value so that it will be immediately replaced
	float node_value;
	if (game->to_play == 0) {
		node_value = -2000;
	}
	else {
		node_value = 2000;
	}

	// initializes variables which allow the moves to be undone.
	float child_value;
	float prev_value = game->value;
	float prev_material = game->current_np_material;
	int captured_piece;
	int previous_castling = game->castling;
	int previous_ply_counter = game->ply_counter;
	unsigned long long previous_last_move[3];
	for (i = 0; i < 3; i++) {
		previous_last_move[i] = game->last_move[i];
	}
	// loops through the legal moves, calculating the value for each move.
	for (i = 0; i < num_moves; i++) {
		captured_piece = apply(game, moves[i], zobrist_numbers, removed_hash);
		update_value(game, moves[i], captured_piece);
		child_value = minimax(game, depth - 1, zobrist_numbers, child_move_number, start_time, time_allowed, max_depth, alpha, beta, nodes);
		game->value = prev_value;
		game->current_np_material = prev_material;
		unapply(game, moves[i], removed_hash[0], captured_piece, previous_castling, previous_ply_counter, previous_last_move);
		// quits the function if the time allowed has been passed
		if (child_value == 0 && ((double)clock() - (double)start_time) / CLOCKS_PER_SEC > time_allowed) {
			return 0;
		}
		// if next person to play is white, they will try to maximize their score
		else if (game->to_play == 0 && child_value > node_value) {
			node_value = child_value;
			move_number[0] = i;
			// alpha beta pruning: if the other player can already guarantee themselves a better score higher up the tree, they won't need to search this path.
			if (node_value > beta) {
				return node_value;
			}
			else if (node_value > alpha) {
				alpha = node_value;
			}
		}
		// if the next person to play is black, they will try to minimize their score
		else if (game->to_play == 1 && child_value < node_value) {
			node_value = child_value;
			move_number[0] = i;
			if (node_value < alpha) {
				return node_value;
			}
			else if (node_value < beta) {
				beta = node_value;
			}
		}
	}
	return node_value;
}

/*
Function to get the move of the engine in a particular position, using the minimax funtion.
It works by looping through, starting at depth 1 until it reaches the allocated time.
This is actually an efficient way to do it, as when using alpha-beta pruning, the order in which the moves are search heavily
affects the number of nodes which can be pruned, and hence we can use the results from previous depths to optimise the move ordering
so that the overall search is more efficient.

INPUTS:
game: Game struct holding the position from which to get the engine move.
zobrist_numbers: numbers used to calculate the hash of the position, allowing us to find draws by repetition
current_move_number: index of the current best move in the moves array, used to return the best move to the main program
time_allowed: time allowed for the program to make its move.

Last Modified: 22/9/2021
Last Modified by: Arkleseisure
*/
void get_engine_move(struct Game* game, unsigned long long* zobrist_numbers, int* current_move_number, double time_allowed, float current_value[1], int depth[1], unsigned long long nodes[1]) {
	srand((unsigned)time(NULL));
	clock_t start_time;
	start_time = clock();
	float value;
	fully_evaluate(game);
	unsigned long long moves[220][3];
	int move_number[1] = { 0 };

	int num_moves = legal_moves(game, moves);

	clock_t current_time;
	double time_taken;
	current_time = clock();
	time_taken = ((double)current_time - (double)start_time) / CLOCKS_PER_SEC;

	// as long as the time hasn't elapsed, the program will keep on calculating the minimax value at greater and greater depth
	while (time_taken < time_allowed) {
		value = minimax(game, depth[0], zobrist_numbers, move_number, start_time, time_allowed, depth[0], (float)-2000, (float)2000, nodes);

		// if the time hasn't ellapsed, then the values are updated (otherwise they will be updated erroneously)
		current_time = clock();
		time_taken = ((double)current_time - (double)start_time) / CLOCKS_PER_SEC;
		if (time_taken < time_allowed) {
			current_value[0] = value;
			current_move_number[0] = move_number[0];

			// if it has found a mate, plays the move which leads to mate.
			if ((current_value[0] > 1000 && game->to_play == 0) || (current_value[0] < -1000 && game->to_play == 1)) {
				return;
			}
		}
		depth[0]++;
	}
}
