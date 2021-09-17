#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include "game_mechanics.c"

int values[12] = { 1, 3, 3, 5, 9, 0, -1, -3, -3, -5, -9, 0 };

void check_it_works() {
	confirm_it_works();
	printf(" This is the engine code version 16.\n");
	printf(" This is engine v1.\n");
}

float evaluate(struct Game* game) {
	int i;
	float value = 0;
	for (i = 0; i < 32; i++) {
		if (!(game->piece_list[i].captured)) {
			value += values[game->piece_list[i].type];
		}
	}
	return value;
}

float minimax(struct Game* game, int depth, unsigned long long* zobrist_numbers, int move_number[1], clock_t start_time, double time_allowed, int max_depth) {
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

	// initializes the value of this node to a really bad value so that 
	float node_value;
	if (game->to_play == 0) {
		node_value = -2000;
	}
	else {
		node_value = 2000;
	}

	float child_value;
	int captured_piece;
	int previous_castling = game->castling;
	int previous_ply_counter = game->ply_counter;
	unsigned long long previous_last_move[3];
	for (i = 0; i < 3; i++) {
		previous_last_move[i] = game->last_move[i];
	}

	for (i = 0; i < num_moves; i++) {
		captured_piece = apply(game, moves[i], zobrist_numbers, removed_hash);
		child_value = minimax(game, depth - 1, zobrist_numbers, child_move_number, start_time, time_allowed, max_depth);
		unapply(game, moves[i], removed_hash[0], captured_piece, previous_castling, previous_ply_counter, previous_last_move);
		if (child_value == 0 && ((double)clock() - (double)start_time) / CLOCKS_PER_SEC > time_allowed) {
			return 0;
		}
		else if (game->to_play == 0 && child_value > node_value) {
			node_value = child_value;
			move_number[0] = i;
		}
		else if (game->to_play == 1 && child_value < node_value) {
			node_value = child_value;
			move_number[0] = i;
		}
	}
	return node_value;
}

float get_engine_move(struct Game* game, unsigned long long* zobrist_numbers, int* current_move_number, double time_allowed) {
	clock_t start_time;
	start_time = clock();
	int depth = 1;
	float value;
	float current_value = 0;
	unsigned long long moves[220][3];
	int move_number[1] = { 0 };

	int num_moves = legal_moves(game, moves);

	clock_t current_time;
	current_time = clock();

	for (int i = 0; i < num_moves; i++) {
		print_move(moves[i]);
	}

	while (((double)current_time - (double)start_time) / CLOCKS_PER_SEC < time_allowed) {
		value = minimax(game, depth, zobrist_numbers, move_number, start_time, time_allowed, depth);
		current_time = clock();
		if (((double)current_time - (double)start_time) / CLOCKS_PER_SEC < time_allowed) {
			current_value = value;
			current_move_number[0] = move_number[0];
			printf("Depth: %d\n", depth);
			printf("Move: ");
			print_move(moves[current_move_number[0]]);
			printf("Value: %f\n", current_value);
		}
		depth++;
	}
	return current_value;
}
