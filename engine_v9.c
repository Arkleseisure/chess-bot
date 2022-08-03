#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include "game_mechanics_v1.c"

float values[12] = { 1, 3, (float)3.3, 5, 9, 4, -1, -3, (float)-3.3, -5, -9, -4 };

float max_np_material = (float)71.2;

// Once there is less material than this, only the endgame piece square tables are used.
float min_np_material = 30;

// Variables defining the number of bits used to index the hash table and the total size of the hash table.
int hash_table_index_bits = 27;
int hash_table_size = 1 << 26;



/*
Used to verify at the start of the program that the code being used is indeed the latest version, and that it has imported the correct version of game_mechanics
Last Modified: 17/9/2021
Last Modified by: Arkleseisure
*/
void check_it_works() {
	confirm_it_works();
	printf(" This is the engine code version 280.\n");
	printf(" This is engine v9.\n");
}


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
hash: hash of the position. As the last 24 bits are used to index the position in the hash table, it is not guaranteed that the hashes are the same,
		and so the other 40 bits are used to check that the information stored is the same as that we are looking for.
value: value of the position as found previously.
node_type: 0 (PV: perfect value node, where the tree has been searched well enough to determine the value of the node perfectly)
		   1 (Cut: The search has been cut off by the alpha-beta pruning. This one is a lower bound for the value of the node.)
		   2 (All: The search has been cut off by the alpha-beta pruning. This one is an upper bound for the value of the node.)
depth: Depth to which the node has been searched.
best_move: best move in the position last time it was searched. Can be useful for move ordering when the depth isn't high enough.

Last Modified: 17/7/2022
Last Modified by: Arkleseisure
*/
struct HashTableEntry {
	unsigned int hash : 32;
	float value;
	int node_type : 2;
	int depth : 6;
	int move_number : 8;
};


/*
The Node struct is used to hold the information about a node, and save it for future use.
Last Modified: 20/9/2021
Last Modified by: Arkleseisure
*/
struct Node {
	float value;
	bool evaluated;
	struct Node* children;
	int num_moves;
};

/*
Adds the results for a particular position to the hash table

Last Modified: 17/7/2022
Last Modified by: Arkleseisure
*/
void add_hash_table_entry(struct HashTableEntry* hash_table, unsigned long long hash, int depth, int node_type, float node_value, int move_number) {
	struct HashTableEntry* new_hash_entry = &hash_table[hash & ((unsigned long long)hash_table_size - 1)];
	new_hash_entry->hash = (unsigned int)(hash >> hash_table_index_bits);
	new_hash_entry->depth = depth;
	new_hash_entry->node_type = node_type;
	new_hash_entry->value = node_value;
	new_hash_entry->move_number = move_number;
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
	// minimum non pawn material is set so that after a certain point in the endgame only endgame tablebases are used.
	else if (current_np_material < min_np_material) {
		current_np_material = min_np_material;
	}
	return ((current_np_material - min_np_material) * piece_square_table_mg[type][rank][file] +
		(max_np_material - current_np_material) * piece_square_table_eg[type][rank][file]) / (100 * (max_np_material - min_np_material));
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
Function used to evaluate the value of a given position
Last Modified: 24/12/2021
Last Modified by: Arkleseisure
*/
float evaluate(struct Game* game) {
	// returns the value with a small random element so that the engine plays differently each time.
	return game->value + ((float)rand() / RAND_MAX) / 5 - (float)0.1;
}


/*
Orders the moves so that they can be more efficiently evaluated with the alpha-beta pruning.
This ordering is largely based off the results of the previous search.

Inputs:
move_order: array which stores the order of the indexes of the moves, i.e if move_order[0] = 2, then the move to be searched first
			is to be found at index 2 in moves
to_play: 0 if white is next player, 1 if black
node: Node struct holding the current node and potentially children.
num_moves: number of moves in the current position.
ht_move: hash table move. If the transposition table is hit, but the entry isn't strong enough to warrant an immediate return (e.g depth isn't high enough),
		the best move stored can still be used to improve the move ordering. If the transposition table has not been hit, ht_move = -1.

Last Modified: 21/7/2022
Last Modified by: Arkleseisure
*/
void order_moves(int* move_order, int to_play, struct Node* node, int num_moves, int ht_move) {
	// initializes the variables used in the function
	int i;
	int j;
	int swap;
	float current_best;
	float next_value;
	struct Node next_node;
	int current_index = 0;
	int multiplier;
	if (to_play == 0) {
		multiplier = 1;
	}
	else {
		multiplier = -1;
	}

	// if there is a transposition table hit, the move which was found to be the best the previous time will be tried first this time
	int start_index = 0;
	if (ht_move != -1) {
		move_order[0] = ht_move;
		start_index = 1;
	}

	// initializes the array to the ordered state, so that a selection sort can be passed over it.
	j = 0;
	for (i = start_index; i < num_moves; i++) {
		if (j == ht_move) {
			j++;
		}
		move_order[i] = j;
		j++;
	}

	if (node->children != 0 && node->children[0].evaluated == true) {
		// creates a list ordered by the previous evaluation of each of the moves.
		for (i = start_index; i < num_moves - 1; i++) {
			if (node->children[move_order[i]].evaluated) {
				// current_best is the best move found so far, starting from i (the list before that will be ordered)
				// current_index is the index of that move within the move_order array
				current_best = node->children[move_order[i]].value * multiplier;
				current_index = i;
			}
			else {
				current_best = -2000;
				current_index = i;
			}
			// loops through the other moves from i to find the best one, which will be swapped with the one at index i.
			for (j = i + 1; j < num_moves; j++) {
				next_node = node->children[move_order[j]];
				if (next_node.evaluated) {
					next_value = next_node.value * multiplier;
					if (next_value > current_best) {
						current_best = next_value;
						current_index = j;
					}
				}
			}

			// swaps the best move found after move i and swaps it with move i
			if (current_index != i) {
				swap = move_order[i];
				move_order[i] = move_order[current_index];
				move_order[current_index] = swap;
			}
		}
	}
}

float quiescence(struct Game* game, unsigned long long* zobrist_numbers, int move_number[1], clock_t start_time, double time_allowed,
	int max_depth, float alpha, float beta, unsigned long long nodes[1], struct HashTableEntry* transposition_table, int table_hits[1],
	struct Node* node);

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
float minimax(struct Game* game, int depth, unsigned long long* zobrist_numbers, int move_number[1], clock_t start_time, double time_allowed,
	int max_depth, float alpha, float beta, unsigned long long nodes[1], struct HashTableEntry* transposition_table, int table_hits[1],
	struct Node* node) {
	nodes[0]++;
	// once the depth hits 0, the tree is exited
	if (depth == 0) {
		float value = quiescence(game, zobrist_numbers, move_number, start_time, time_allowed, max_depth, 
			alpha, beta, nodes, transposition_table, table_hits, node);
		node->value = value;
		node->evaluated = true;
		return value;
	}

	int i;
	struct HashTableEntry* entry = &transposition_table[game->hash & ((unsigned long long)hash_table_size - 1)];
	bool hash_table_entry = false;

	if ((((double)clock() - (double)start_time) / CLOCKS_PER_SEC) > time_allowed) {
		return 0;
	}
	// Checks whether the part of the hash stored in the entry matches the equivalent part of the current hash, in which case the table is looked at.
	// The hash is xored with 32 bits of 1s in case hash_table_index_bits has been decreased, in which case the values would otherwise never match due
	// the automatic truncation when the value was entered into the table.
	else if (entry->hash == (unsigned int)((game->hash >> hash_table_index_bits) & (((unsigned long long)1 << 32) - 1))) {
		hash_table_entry = true;
		// if the value is exact, or provides enough information for an alpha-beta cutoff, the value can be returned immediately.
		if ((entry->node_type == 0 || (entry->node_type == 1 && entry->value > beta) || (entry->node_type == 2 && entry->value < alpha)) && entry->depth >= depth) {
			move_number[0] = entry->move_number;
			table_hits[0]++;
			node->value = entry->value;
			node->evaluated = true;
			return entry->value;
		}
	}
	// Checks whether the engine has already found mate for this position in an earlier search
	if (node->value >= 1000 || node->value <= -1000) {
		return node->value;
	}

	int value = terminal(game);
	// terminal gives 0 for a win for black, 1 for a draw, 2 for a win for white and 3 for no result.
	if (value == 1) {
		node->value = 0;
		node->evaluated = true;
		return 0;
	}
	// if the position is mate, then it returns a very high value, which is slightly higher if it is at lower depth  
	// (as the depth variable holds the depth yet to search as opposed to the current depth, the sign is the same as that of the returned value) to reward shorter mates
	else if (value == 0) {
		node->value = (float)(-1000 - depth);
		node->evaluated = true;
		return (float)(-1000 - depth);
	}
	else if (value == 2) {
		node->value = (float)(1000 + depth);
		node->evaluated = true;
		return (float)(1000 + depth);
	}

	unsigned long long moves[220][3];
	int num_moves = legal_moves(game, moves);
	unsigned long long removed_hash[1];
	int child_move_number[1] = { 0 };

	// if the node.children array hasn't been filled with the child nodes, this has to now be completed.
	if (node->children == 0) {
		node->children = (struct Node*)calloc(num_moves, sizeof(struct Node));
		if (!node->children) {
			return 0;
		}
		node->num_moves = num_moves;
	}

	// initializes the value of this node to a really bad value so that it will be immediately replaced
	float node_value;
	if (game->to_play == 0) {
		node_value = -2000;
	}
	else {
		node_value = 2000;
	}

	// if there is a hash table (transposition table) move, this sets ht_move to it so that it can be searched first.
	int ht_move = -1;
	if (hash_table_entry && entry->move_number < num_moves) {
		ht_move = entry->move_number;
	}

	// produces a move order array containing the order in which the moves are to be evaluated
	int* move_order = malloc(220 * sizeof(int));
	if (!move_order) {
		return 0;
	}
	order_moves(move_order, game->to_play, node, num_moves, ht_move);

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
	int current_index;
	// Loops through the legal moves, calculating the value for each move.
	for (i = 0; i < num_moves; i++) {
		current_index = move_order[i];
		if (current_index > 100) {
			printf("got index %d\n", current_index);
		}
		captured_piece = apply(game, moves[current_index], zobrist_numbers, removed_hash);
		update_value(game, moves[current_index], captured_piece);
		child_value = minimax(game, depth - 1, zobrist_numbers, child_move_number, start_time, time_allowed, max_depth, alpha, beta, nodes, transposition_table, table_hits, &(node->children[current_index]));
		game->value = prev_value;
		game->current_np_material = prev_material;
		unapply(game, moves[current_index], removed_hash[0], captured_piece, previous_castling, previous_ply_counter, previous_last_move);

		// quits the function if the time allowed has been passed
		if (child_value == 0 && ((double)clock() - (double)start_time) / CLOCKS_PER_SEC > time_allowed) {
			free(move_order);
			return 0;
		}
		// if next person to play is white, they will try to maximize their score
		else if (game->to_play == 0 && child_value > node_value) {
			node_value = child_value;
			move_number[0] = current_index;
			// alpha beta pruning: if the other player can already guarantee themselves a better score higher up the tree, they won't need to search this path.
			if (node_value > beta) {
				add_hash_table_entry(transposition_table, game->hash, depth, 1, node_value, current_index);
				node->value = node_value;
				node->evaluated = true;
				free(move_order);
				return node_value;
			}
			else if (node_value > alpha) {
				alpha = node_value;
			}
		}
		// if the next person to play is black, they will try to minimize their score
		else if (game->to_play == 1 && child_value < node_value) {
			node_value = child_value;
			move_number[0] = current_index;
			if (node_value < alpha) {
				add_hash_table_entry(transposition_table, game->hash, depth, 2, node_value, current_index);
				node->value = node_value;
				node->evaluated = true;
				free(move_order);
				return node_value;
			}
			else if (node_value < beta) {
				beta = node_value;
			}
		}
	}
	add_hash_table_entry(transposition_table, game->hash, depth, 0, node_value, move_number[0]);
	node->value = node_value;
	node->evaluated = true;
	free(move_order);
	return node_value;
}

float run_quiescence(struct Game* game, unsigned long long* zobrist_numbers, int move_number[1], clock_t start_time, double time_allowed,
	int max_depth, float alpha, float beta, unsigned long long nodes[1], struct HashTableEntry* transposition_table, int table_hits[1],
	struct Node* node, unsigned long long* move) {
	
	int i;
	float value = 0;
	// in order to return the game to its original state after applying a move, certain values have to be stored so that they can be quickly regained.
	float prev_value = game->value;
	float prev_material = game->current_np_material;
	int previous_castling = game->castling;
	int previous_ply_counter = game->ply_counter;
	unsigned long long previous_last_move[3];
	for (i = 0; i < 3; i++) {
		previous_last_move[i] = game->last_move[i];
	}

	int captured_piece;
	unsigned long long removed_hash[1];

	nodes[0]++;
	captured_piece = apply(game, move, zobrist_numbers, removed_hash);
	update_value(game, move, captured_piece);
	value = quiescence(game, zobrist_numbers, move_number, start_time, time_allowed, max_depth, alpha, beta, nodes, transposition_table, table_hits, node);
	game->value = prev_value;
	game->current_np_material = prev_material;
	unapply(game, move, removed_hash[0], captured_piece, previous_castling, previous_ply_counter, previous_last_move);

	return value;
}
/*
Function used to perform a quiescence search (i.e verify that the position is quiet before being evaluated)

Last Modified: 25/7/2022
Last Modified by: Arkleseisure
*/
float quiescence(struct Game* game, unsigned long long* zobrist_numbers, int move_number[1], clock_t start_time, double time_allowed,
	int max_depth, float alpha, float beta, unsigned long long nodes[1], struct HashTableEntry* transposition_table, int table_hits[1],
	struct Node* node) {


	// if in check, increases the depth by 1.
	if (is_attacked(game->board, game->to_play, game->piece_list[15 + 16 * game->to_play].loc)) {
		return minimax(game, 1, zobrist_numbers, move_number, start_time, time_allowed, max_depth, alpha, beta, nodes, transposition_table, table_hits, node);
	}
	/*
	Gets current evaluation: we assume that the current position is not zugzwang and so there will be a move that will improve the evaluation.
	If this score is higher than beta (best score that black can ensure themselves higher up the tree) and it's white to play, 
	then we can just do a beta cutoff here as it is unlikely things are going to improve for black. 
	The reverse of course happens when it's black to play.
	*/
	float stand_pat = evaluate(game);
	if ((game->to_play == 0 && stand_pat > beta)
		|| (game->to_play == 1 && stand_pat < alpha)) {
		return stand_pat;
	}

	if ((((double)clock() - (double)start_time) / CLOCKS_PER_SEC) > time_allowed) {
		return 0;
	}

	// gets the legal moves for the position
	int i;
	int j;
	int k;
	unsigned long long moves[220][3];
	int num_moves = legal_moves(game, moves);

	// if the node.children array hasn't been filled with the child nodes, this has to now be completed.
	if (node->children == 0) {
		node->children = (struct Node*)calloc(num_moves, sizeof(struct Node));
		if (!node->children) {
			return stand_pat;
		}
		node->num_moves = num_moves;
	}

	int shift = 6 * (1 - game->to_play);

	int flag;
	int piece;

	unsigned long long higher_value_pieces;

	float safety_margin = 2;

	float new_value = stand_pat;
	float best_value = stand_pat;

	for (i = 0; i < num_moves; i++) {
		// the flag indicates the type of move
		flag = (int)(moves[i][2] & 15);

		/*
		The position will be expanded if:
		1. A pawn is being promoted to a queen.
		2. A piece is attacking another with higher value, high enough to compete with the best move searched higher up.
		*/
		if ((flag & 11) == 11) {
			new_value = run_quiescence(game, zobrist_numbers, move_number, start_time, time_allowed, max_depth, alpha, beta, nodes, 
				transposition_table, table_hits, &(node->children[i]), moves[i]);
		}
		// if the move is a capture, it looks to see if the capture is of a piece more valuable than itself.
		else if ((flag & 4) != 0){
			// type of piece captured, 0 for pawn through to 5 for king in order of value
			piece = (int)((moves[i][2] >> 4) & 15) % 6;
			// finds pieces which are higher value than the piece doing the capturing, and verifies that the capture is going to provide a value 
			// which might compare with the best score than it can guarantee itself higher up (otherwise there is no use in searching)
			for (j = piece + 1; j < 5; j++) {
				if (((stand_pat + safety_margin + values[j] > alpha) && (game->to_play == 0)) ||
					((stand_pat - safety_margin - values[j] < beta) && (game->to_play == 1))) {
					// finds the locations of the other player's pieces which are of high enough value to be considered.
					higher_value_pieces = 0;
					for (k = j; k < 5; k++) {
						higher_value_pieces ^= game->board[k + shift];
					}
					// if any of the higher value pieces is the one captured by this move, the move is expanded.
					if ((higher_value_pieces & moves[i][1]) != 0) {
						new_value = run_quiescence(game, zobrist_numbers, move_number, start_time, time_allowed, max_depth, alpha, beta, nodes, transposition_table, table_hits, &(node->children[i]), moves[i]);
					}
				}
			}
		}

		if ((((double)clock() - (double)start_time) / CLOCKS_PER_SEC) > time_allowed) {
			return 0;
		}

		// Improves alpha and best_value depending on how good new_value is.
		// Performs a beta cutoff if black can already guarantee themselves a better value higher up
		if ((game->to_play == 0) && (new_value > best_value)) {
			best_value = new_value;
			if (best_value > beta) {
				return best_value;
			}
			else if (best_value > alpha) {
				alpha = best_value;
			}
		}
		// Improves beta and best_value depending on how good new_value is.
		// Performs an alpha cutoff if white can already guarantee themselves a better value higher up
		else if ((game->to_play == 1) && (new_value < best_value)) {
			best_value = new_value;
			if (best_value < alpha) {
				return best_value;
			}
			else if (best_value < beta) {
				beta = best_value;
			}
		}
	}
	return best_value;
}

/*
Frees the memory allocated to the nodes by recursively looking into the children of the parent node

Last Modified: 24/7/2022
Last Modified by: Arkleseisure
*/
void free_node(struct Node* root_node) {
	for (int i = 0; i < root_node->num_moves; i++) {
		free_node(&(root_node->children[i]));
	}
	free(root_node->children);
}

/*
Frees all the memory which has been allocated during the search.

Last Modified: 24/7/2022
Last Modified by: Arkleseisure
*/
void free_stuff(struct HashTableEntry* transposition_table, struct Node* root_node) {
	free(transposition_table);
	free_node(root_node);
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
	int table_hits[1] = { 0 };
	struct HashTableEntry* transposition_table = (struct HashTableEntry*)malloc(hash_table_size * sizeof(struct HashTableEntry));

	int num_moves = legal_moves(game, moves);


	bool hash_table_generated = false;
	while (!hash_table_generated) {
		if (transposition_table == NULL) {
			hash_table_size >>= 1;
			hash_table_index_bits -= 1;
			printf("Transposition table generation failed, now trying with size %d.\n", hash_table_index_bits);
			transposition_table = (struct HashTableEntry*)malloc(hash_table_size * sizeof(struct HashTableEntry));
		}
		else {
			hash_table_generated = true;
		}
	}

	clock_t current_time;
	double time_taken;
	current_time = clock();
	time_taken = ((double)current_time - (double)start_time) / CLOCKS_PER_SEC;

	struct Node* root_node = &(struct Node) {
		.value = 0,
			.evaluated = false,
			.children = 0,
			.num_moves = num_moves
	};

	// as long as the time hasn't elapsed, the program will keep on calculating the minimax value at greater and greater depth
	while (time_taken < time_allowed) {
		value = minimax(game, depth[0], zobrist_numbers, move_number, start_time, time_allowed, depth[0], (float)-2000, (float)2000, nodes, transposition_table, table_hits, root_node);

		// if the time hasn't ellapsed, then the values are updated (otherwise they will be updated erroneously)
		current_time = clock();
		time_taken = ((double)current_time - (double)start_time) / CLOCKS_PER_SEC;
		if (time_taken < time_allowed) {
			current_value[0] = value;
			current_move_number[0] = move_number[0];

			// if it has found a mate, plays the move which leads to mate.
			if ((current_value[0] > 1000 && game->to_play == 0) || (current_value[0] < -1000 && game->to_play == 1)) {
				free_stuff(transposition_table, root_node);
				return;
			}
		}
		depth[0]++;
	}
	free_stuff(transposition_table, root_node);
	if (move_number[0] >= num_moves) {
		printf("Move out of range\n");
		printf("Board:\n");
		print_board(game->board);
		printf("Move number: %d\n", move_number[0]);
		printf("Moves: \n");
		for (int i = 0; i < num_moves; i++) {
			print_move(moves[i]);
		}
	}
}
