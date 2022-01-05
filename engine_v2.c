#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include "game_mechanics_v1.c"

int values[12] = { 1, 3, 3, 5, 9, 0, -1, -3, -3, -5, -9, 0 };

/*
Used to verify at the start of the program that the code being used is indeed the latest version, and that it has imported the correct version of game_mechanics
Last Modified: 17/9/2021
Last Modified by: Arkleseisure
*/
void check_it_works() {
	confirm_it_works();
	printf(" This is the engine code version 24.\n");
	printf(" This is engine v2.\n");
}

/*
Function used to evaluate the value of a given position
Last Modified: 17/9/2021
Last Modified by: Arkleseisure
*/
float evaluate(struct Game* game) {
	int i;
	float value = 0;
	// loops through the piece_list, adding the values of each piece in the position.
	for (i = 0; i < 32; i++) {
		if (!(game->piece_list[i].captured)) {
			value += values[game->piece_list[i].type];
		}
	}
	// adds a random value between -0.1 and 0.1 so that the engine gives different results each time
	float value_before = value;
	value += ((float)rand() / RAND_MAX) / 5;
	value -= (float)0.1;
	return value;
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
	// once the depths hits 0, the tree is exited
	else if (depth == 0) {
		return evaluate(game);
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
		child_value = minimax(game, depth - 1, zobrist_numbers, child_move_number, start_time, time_allowed, max_depth, alpha, beta, nodes);
		unapply(game, moves[i], removed_hash[0], captured_piece, previous_castling, previous_ply_counter, previous_last_move);
		// quits the function if the time allowed has been passed
		if (child_value == 0 && ((double)clock() - (double)start_time) / CLOCKS_PER_SEC > time_allowed) {
			return 0;
		}
		// if next person to play is white, they will try to maximize their score
		else if (game->to_play == 0 && child_value > node_value) {
			node_value = child_value;
			move_number[0] = i;
		}
		// if the next person to play is black, they will try to minimize their score
		else if (game->to_play == 1 && child_value < node_value) {
			node_value = child_value;
			move_number[0] = i;
		}

		// alpha beta pruning: if the other player can already guarantee themselves a better score higher up the tree, they won't need to search this path.
		if (game->to_play == 0 && node_value > beta) {
			return node_value;
		}
		else if (game->to_play == 0 && node_value > alpha) {
			alpha = node_value;
		}
		else if (game->to_play == 1 && node_value < alpha) {
			return node_value;
		}
		else if (game->to_play == 1 && node_value < beta) {
			beta = node_value;
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
	unsigned long long moves[220][3];
	int move_number[1] = { 0 };

	int num_moves = legal_moves(game, moves);

	clock_t current_time;
	current_time = clock();

	// as long as the time hasn't elapsed, the program will keep on calculating the minimax value at greater and greater depth
	while (((double)current_time - (double)start_time) / CLOCKS_PER_SEC < time_allowed) {
		value = minimax(game, depth[0], zobrist_numbers, move_number, start_time, time_allowed, depth[0], (float)-2000, (float)2000, nodes);

		// if the time hasn't ellapsed, then the values are updated (otherwise they will be updated erroneously)
		current_time = clock();
		if (((double)current_time - (double)start_time) / CLOCKS_PER_SEC < time_allowed) {
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
