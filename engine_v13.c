#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>
#include "game_mechanics_v1.c"

float values[6] = { 1, (float)2.9, (float)3.35, (float)4.9, 10, (float)4.5 };

float max_np_material = (float)73.6;

// Once there is less material than this, only the endgame piece square tables are used.
float min_np_material = 21;

// Value of a tempo
float tempo_value = 0.6;

// Late move reduction threshold: The depth searched is reduced by 1 after this number of moves searched
int late_move_reduction_threshold = 1;

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
	printf(" This is the engine code version 445.\n");
	printf(" This is engine v13.\n");
}


// Piece-square tables are used to encourage the engine to move the pieces to squares where they will likely be active.
// The squares where pieces are useful are different in the middlegame and the endgame, so different tables are used, and then the 
// actual evaluation is a linear combination depending on the stage of the game.
float piece_square_table_mg[6][8][8] = {
	// Pawns (note top left is a1, top right is a8, so visually this is equivalent to looking at the piece square table for black from the 
	// white side
	{{-0.727912, 0.328852, -1.486663, -2.447386, -2.813639, -0.058794, -0.523194, 0.664647},
	{2.629826, 8.910352, 7.753578, -18.970261, -18.295618, 7.909894, 8.888805, 1.814493},
	{2.003281, 5.379421, 0.403104, 0.439146, -0.179311, -12.725715, 4.417905, 6.075884},
	{-0.470824, 3.306055, 4.809977, 17.560762, 19.722115, -3.108753, 1.239189, 0.684942},
	{2.213187, 3.528443, 10.893231, 25.336481, 23.486450, 10.821390, 5.292444, 2.459655},
	{9.691961, 7.867565, 13.999008, 30.302668, 26.780434, 14.230887, 10.574587, 6.920118},
	{50.869457, 49.216881, 45.629826, 49.400143, 50.435730, 50.669407, 49.308434, 48.785347},
	{-0.571291, 0.007584, 1.051653, -1.843822, 0.896374, 2.721869, 1.984970, 0.844005}},
	// Knights
	{{-47.781075, -30.952223, -19.227348, -8.755409, -8.455932, -17.963789, -29.718330, -51.799690},
	{-30.483459, -15.261803, -5.099811, -2.581698, -0.102893, -6.274560, -13.144734, -31.104389},
	{-21.837107, 2.661229, 7.370235, 0.818461, -1.315333, 7.145253, 7.009903, -18.095661},
	{-7.425352, -0.453703, 5.053209, 18.657566, 19.795877, 7.046190, 4.240013, -13.495422},
	{-7.368221, -0.390561, 5.734504, 20.278984, 18.104206, 3.101093, -0.351588, -10.974959},
	{-16.561434, 1.268273, 7.969985, 13.589175, 16.166281, 6.807199, -1.760811, -18.034502},
	{-28.039598, -13.768319, -4.183737, -1.132405, -3.203146, -2.236015, -11.914716, -28.506958},
	{-48.464081, -31.035021, -18.563601, -11.050493, -9.482726, -23.675177, -27.627781, -50.686832}},
	// Bishops
	{{-20.737007, -8.470550, -10.419981, -11.215110, -9.119709, -6.374966, -3.684484, -18.083849},
	{-8.738838, 9.797159, -0.060686, 6.371731, 5.668737, -0.016160, 11.953719, -8.802286},
	{-7.192160, -1.958754, 5.378994, 6.065050, 8.970168, 6.681524, 1.816660, -6.544496},
	{-6.649510, 3.900128, 9.041856, 11.260216, 10.125751, 8.112232, 1.468230, -7.240409},
	{-10.607333, 5.717719, 9.329677, 11.451536, 8.554934, 11.311152, 8.924025, -7.300561},
	{-8.743233, -1.741524, 3.758248, 4.666051, 5.454161, 4.856609, -0.055132, -5.495758},
	{-8.280541, -1.203360, -2.312159, 1.271783, -1.628605, -0.852184, 1.204855, -10.460540},
	{-19.475281, -11.418241, -8.071093, -9.838267, -9.344783, -10.943281, -11.666661, -15.822367}},
	// Rooks
	{{-0.230338, -1.322047, 0.222465, 6.312067, 3.779183, 0.955947, -1.165914, -0.955428},
	{-9.833781, -0.820811, -2.404538, -2.283868, 1.540773, 0.120777, -4.122425, -8.359249},
	{-7.821360, -3.201437, -2.408628, 1.692663, -0.863964, -1.493805, 1.969161, -8.663304},
	{-7.965621, 0.834147, 1.375668, -1.430265, 1.108295, 0.900159, -0.508057, -7.255150},
	{-12.388668, -1.492340, -2.283929, -2.257012, -0.827189, -1.550050, -0.875317, -6.534913},
	{-7.800607, 1.680059, 0.838420, -1.890393, 2.915693, -3.435087, 0.135334, -7.793588},
	{8.137074, 18.941328, 21.156757, 17.876293, 15.368313, 20.194204, 17.547976, 6.282311},
	{-2.676824, 1.074816, -1.121143, 1.743812, 1.456511, -1.521821, -0.813181, -2.517701}},
	// Queens
	{{-19.395931, -6.102222, -12.512939, -5.535096, -1.675420, -10.223014, -5.406552, -19.681890},
	{-9.965346, 1.475921, -2.906720, -0.222343, 0.534669, 2.071795, 0.820292, -9.355556},
	{-10.984604, -3.287317, -0.330317, 0.715003, 0.141255, -3.257591, 1.098254, -8.344630},
	{-7.695868, 0.315271, 0.154073, 1.287378, 2.639927, -2.279656, -0.620335, -2.744820},
	{-7.097064, 0.133503, -1.569308, 0.541963, -2.208395, 0.015183, 1.756661, -3.195456},
	{-9.180777, -0.125141, 2.185080, 0.995743, 2.322352, -0.382504, -2.246391, -12.758156},
	{-10.296808, 0.194632, 2.634983, 0.444853, -0.503296, -1.224052, 2.495697, -11.421201},
	{-18.641148, -11.839183, -6.864208, -0.852794, -5.432402, -13.350215, -11.249077, -23.227898}},
	// Kings
	{{22.131672, 28.093189, 23.660925, -19.617954, -16.899548, -10.663854, 32.849438, 25.969162},
	{4.169088, 3.779122, -0.842631, -12.617405, -11.828074, 4.272302, 2.475188, 6.129017},
	{-17.441557, -20.937208, -20.224083, -19.354366, -16.555056, -17.986923, -20.665258, -22.980972},
	{-30.662390, -31.189718, -31.023087, -29.385738, -29.882793, -29.616520, -29.300226, -28.414366},
	{-38.617313, -38.750557, -40.489410, -40.263054, -38.254417, -37.994766, -39.174458, -42.299858},
	{-51.831005, -51.315517, -48.727242, -50.099049, -47.054428, -49.800026, -47.756691, -53.719582},
	{-61.591831, -59.413784, -58.965286, -58.758949, -58.034561, -60.733650, -61.071430, -60.213554},
	{-69.440460, -70.943527, -68.950699, -69.677040, -67.864845, -67.868019, -70.655861, -69.907974}} };



float piece_square_table_eg[6][8][8] = {
	// Pawns
	{ {-0.782571, 0.668401, 0.531220, -1.297815, -0.809183, 0.381466, 0.357753, -1.270104},
	{ 13.372372, 3.680700, -0.987625, 2.857189, 1.474181, -1.342067, 8.035539, 6.514100 },
	{ 13.547792, 10.628208, 4.572207, 8.498444, 7.006088, 6.441435, 10.309473, 13.535127 },
	{ 18.812357, 14.286004, 10.943617, 10.251640, 11.436552, 8.199850, 12.520600, 14.531037 },
	{ 40.665836, 35.009995, 31.493134, 29.716833, 26.868420, 28.917799, 33.837414, 37.634434 },
	{ 59.801399, 57.450958, 51.076130, 49.901318, 47.556915, 48.997818, 54.547729, 59.492157 },
	{ 78.742134, 74.645241, 68.768105, 70.028610, 69.483612, 70.538208, 76.227837, 80.762733 },
	{ -0.647099, 1.478820, -0.530885, -1.277734, -0.194571, -1.014664, -2.215262, 0.986831 }},
	// Knights
	{ {-48.773201, -32.472229, -18.614933, -8.436125, -6.820261, -21.839060, -27.625492, -51.166920},
	{-28.052874, -14.727576, -5.617100, -0.972732, 2.962569, -3.335536, -14.805246, -30.280083},
	{-16.775948, -6.311884, 9.220634, 14.786721, 16.283442, 5.650212, -6.846751, -20.189016},
	{-12.093768, 0.209220, 15.653844, 16.400631, 20.994522, 10.980330, -1.589389, -10.613468},
	{-12.132404, 3.069262, 15.415006, 17.370205, 19.494385, 16.751259, 0.797952, -7.410428},
	{-20.230827, -3.984024, 11.069811, 14.572786, 14.427916, 9.196646, -3.889874, -19.453154},
	{-31.304560, -15.599002, -7.252922, -0.928785, 0.260002, -6.035264, -16.725868, -29.134998},
	{-52.333370, -29.355434, -20.255363, -11.703833, -9.274896, -17.229515, -30.608891, -48.498413} },
	// Bishops
	{ {-17.975174, -11.924482, -7.990463, -7.394009, -10.848948, -7.577303, -9.344691, -19.989029},
	{-6.303980, -1.853893, -2.506592, 0.219352, -2.341182, 0.227073, 1.146199, -8.833812},
	{-11.672857, 1.021928, 6.383327, 6.524446, 5.236259, 5.762734, -0.502808, -7.045946},
	{-8.703131, -2.453429, 7.207205, 6.762307, 9.687445, 3.035203, -0.562044, -7.053239},
	{-7.396695, -2.418424, 0.813669, 9.085589, 9.457244, 4.003403, -0.787027, -10.951704},
	{-10.374905, -2.278710, 4.228477, 4.991897, 4.329859, 1.481933, 0.873974, -12.385403},
	{-10.241600, 3.382839, 1.384060, 1.132588, -1.996200, 5.419004, -1.551759, -9.049273},
	{-20.071428, -6.729530, -11.274316, -9.503967, -9.459472, -7.452178, -8.018388, -15.818949} },
	// Rooks
	{ {-0.023118, 2.216086, 1.691656, 5.967086, 3.688910, -0.585269, 1.456084, 0.496063},
	{-10.077959, 1.393613, 2.068682, -0.740547, 3.084430, -3.218558, -0.416715, -8.227134},
	{-11.902600, -1.090991, -0.777535, 1.225272, 2.323390, -0.598086, 1.471923, -8.428343},
	{-10.837962, -2.234886, 0.203269, 1.538057, 1.911969, 2.175161, 0.171194, -10.493225},
	{-8.012009, -1.798685, 1.726173, -2.248070, -2.909009, -0.327662, -0.911481, -12.007248},
	{-5.578097, 0.408780, -1.973434, -0.265404, -2.465545, -1.960616, 1.282525, -9.634800},
	{9.921095, 18.619877, 20.590883, 20.476227, 18.923964, 18.273949, 18.631260, 11.377254},
	{-1.933576, -2.426755, -2.238243, 0.273003, 1.398282, -1.122028, -3.125446, -1.929701} },
	// Queens
	{ {-20.051805, -7.646397, -7.781320, -0.714148, -3.122761, -7.114826, -6.467193, -17.979294},
	{-6.572145, -2.393521, 0.333369, 2.473815, 2.260766, -0.736793, -1.300806, -6.040147},
	{-8.724982, -0.871502, -0.599918, -2.097278, 1.503723, -2.309198, -1.291711, -12.228568},
	{-5.820444, -0.168600, 0.741737, -0.570956, -1.435484, -4.289330, -0.321436, -3.138325},
	{-2.362819, -2.917188, -0.310572, 1.757851, 2.229759, 1.256218, -1.069689, -3.094409},
	{-9.287805, 0.420194, 2.438841, -1.122852, 1.812998, -1.445708, 0.279351, -7.031419},
	{-10.326166, -2.358364, -1.220298, -2.134144, -0.270592, 2.402555, -2.796213, -10.997940},
	{-20.492065, -7.730079, -11.297510, -3.921918, -1.055193, -9.178488, -11.111622, -18.285639} },
	// Kings
	{ {-40.696144, -20.607487, -20.122364, -17.299067, -19.700171, -15.917981, -21.858776, -42.182697},
	{-20.484100, -9.123158, -4.988204, -2.311548, -4.290582, -4.892132, -7.812387, -22.352810},
	{-20.899670, -5.040239, -0.921735, 13.070635, 9.236382, 0.252586, -5.662724, -16.839304},
	{-20.473083, -4.787606, 11.422178, 29.636112, 29.041643, 7.452422, -4.471648, -23.694036},
	{-19.334284, -6.119831, 9.740852, 27.991745, 29.207357, 8.752968, -3.255608, -20.230858},
	{-19.451200, -5.402677, 0.939741, 6.995499, 11.437101, 2.212546, -2.692480, -20.905193},
	{-19.993210, -9.522676, -3.137196, -2.434660, -2.370907, -3.821879, -7.953078, -16.817423},
	{-40.819195, -19.422972, -20.102528, -15.137318, -17.673407, -21.061022, -19.658606, -39.842144} } };


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
			game->value += values[piece.type];
		}
		if (!(game->piece_list[i + 16].captured)) {
			piece = game->piece_list[i + 16];
			piece_rank = 7 - rank(piece.loc);
			piece_file = file(piece.loc);
			game->value -= get_psqt_value(piece.type - 6, piece_rank, piece_file, game->current_np_material);
			game->value -= values[piece.type - 6];
		}
	}

	// Using NegaMax, so we flip the score if it is black to play
	if (game->to_play == 1) {
		game->value *= -1;
	}
}

/*
Updates the parts of the evaluation which are incrementally updated for efficiency reasons.
Last Modified: 24/12/2021
Last Modified by: Arkleseisure
*/
void update_value(struct Game* game, unsigned long long move[3], int captured_piece) {
	//printf("Original value: %f\n", game->value);
	// handles promotions
	if ((move[2] & 8) != 0) {
		int file1 = file(move[0]);
		int file2 = file(move[1]);

		// gets the piece which is to be promoted to (last 2 bits encode promotion type... 0=N, 1=B, 2=R, 3=Q).
		// Note we use 1 - game->to_play because the apply function has just happened.
		int prom_piece = (int)((move[2] & 3) + 1);

		// subtracts the value of the pawn
		game->value -= values[0];
		game->value -= get_psqt_value(0, 6, file1, game->current_np_material);

		if (captured_piece != 32) {
			int piece_capt = game->piece_list[captured_piece].type % 6;
			game->value += values[piece_capt];
			game->value += get_psqt_value(piece_capt, 0, file2, game->current_np_material);

			if (piece_capt != 0) {
				game->current_np_material -= values[piece_capt];
			}
		}

		// adds the value of the promotion piece
		game->current_np_material += values[prom_piece];
		game->value += values[prom_piece];
		game->value += get_psqt_value(prom_piece, 7, file2, game->current_np_material);
	}
	else {
		int piece_type = (int)((move[2] >> 4) & (unsigned long long)15);
		piece_type %= 6;
		int rank1 = rank(move[0]);
		int file1 = file(move[0]);
		int rank2 = rank(move[1]);
		int file2 = file(move[1]);

		if (game->to_play == 0) {
			rank1 = 7 - rank1;
			rank2 = 7 - rank2;
		}

		int flag = (int)(move[2] & (unsigned long long)15);

		game->value -= get_psqt_value(piece_type, rank1, file1, game->current_np_material);
		game->value += get_psqt_value(piece_type, rank2, file2, game->current_np_material);


		// adds captures
		if (flag == 4) {
			int piece_capt = game->piece_list[captured_piece].type % 6;
			game->value += values[piece_capt];
			game->value += get_psqt_value(piece_capt, 7 - rank2, file2, game->current_np_material);
			if (piece_capt != 0) {
				game->current_np_material -= values[piece_capt];
			}
		}
		// adds en passant
		else if (flag == 5) {
			game->value += values[0];
			game->value += get_psqt_value(0, 7 - (rank2 - 1), file2, game->current_np_material);
		}
		// adds kingside castling
		else if (flag == 2) {
			game->value -= get_psqt_value(3, 0, 7, game->current_np_material);
			game->value += get_psqt_value(3, 0, 5, game->current_np_material);
		}
		// adds queenside castling
		else if (flag == 3) {
			game->value -= get_psqt_value(3, 0, 0, game->current_np_material);
			game->value += get_psqt_value(3, 0, 3, game->current_np_material);
		}
	}
	//printf("Final value: %f\n", game->value);
	game->value *= -1;
}

/*
Function used to evaluate the value of a given position
Last Modified: 24/12/2021
Last Modified by: Arkleseisure
*/
float evaluate(struct Game* game) {
	// returns the value, subtracting the value of the fact the other player has the move.
	return game->value + tempo_value; // +((float)rand() / RAND_MAX) / 5 - (float)0.1;
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
				current_best = node->children[move_order[i]].value;
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
					next_value = next_node.value;
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
		// The value of the node is the value when looked at from the opponent's perspective, as that's where it will be accessed from next
		node->value = -value;
		node->evaluated = true;
		return value;
	}

	int i;
	struct HashTableEntry* entry = &transposition_table[game->hash & ((unsigned long long)hash_table_size - 1)];
	bool hash_table_entry = false;

	if ((((double)clock() - (double)start_time) / CLOCKS_PER_SEC) > time_allowed) {
		return -2000;
	}
	// Checks whether the part of the hash stored in the entry matches the equivalent part of the current hash, in which case the table is looked at.
	// The hash is xored with 32 bits of 1s in case hash_table_index_bits has been decreased, in which case the values would otherwise never match due
	// the automatic truncation when the value was entered into the table.
	else if (entry->hash == (unsigned int)((game->hash >> hash_table_index_bits) & (((unsigned long long)1 << 32) - 1))) {
		hash_table_entry = true;
		// if the value is exact, or provides enough information for an alpha-beta cutoff, the value can be returned immediately.
		if ((entry->node_type == 0 || (entry->node_type == 1 && entry->value > beta)) && entry->depth >= depth) {
			move_number[0] = entry->move_number;
			table_hits[0]++;
			node->value = -entry->value;
			node->evaluated = true;
			return entry->value;
		}
	}
	// Checks whether the engine has already found mate for this position in an earlier search
	if (node->value >= 1000 || node->value <= -1000) {
		return -node->value;
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
	else if (value == 2 * game->to_play) {
		node->value = (float)(1000 + depth);
		node->evaluated = true;
		return -node->value;
	}
	else if (value == 2 * (1 - game->to_play)) {
		node->value = (float)(-1000 - depth);
		node->evaluated = true;
		return -node->value;
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
	node_value = -2000;

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
	int depth_searched = depth - 1;
	bool needs_full_search = true;
	float reduced_search_value;

	// Loops through the legal moves, calculating the value for each move.
	for (i = 0; i < num_moves; i++) {
		current_index = move_order[i];
		// applies the move and updates the value held by game
		captured_piece = apply(game, moves[current_index], zobrist_numbers, removed_hash);
		update_value(game, moves[current_index], captured_piece);

		// the depth searched is reduced for later moves as they are less likely to produce a good move
		if ((i >= late_move_reduction_threshold) && (depth >= 2)) {
			needs_full_search = false;
			depth_searched = depth - 2;
			reduced_search_value = -minimax(game, depth_searched, zobrist_numbers, child_move_number, start_time, time_allowed,
				max_depth, -beta, -alpha, nodes, transposition_table, table_hits, &(node->children[current_index]));
			// good moves are searched to the full depth
			if (reduced_search_value > node_value) {
				needs_full_search = true;
				depth_searched = depth - 1;
			}
		}
		// The first move is fully searched, as are later moves which the reduced search has shown to be strong
		if (needs_full_search) {
			// We are using NegaMax (same as minimax, but the maximiser is always the player to play),
			// so the value flips at every depth, along with alpha and beta, see https://www.chessprogramming.org/Negamax
			child_value = -minimax(game, depth_searched, zobrist_numbers, child_move_number, start_time, time_allowed,
				max_depth, -beta, -alpha, nodes, transposition_table, table_hits, &(node->children[current_index]));
		}

		// returns values and position of board to what they were previously
		game->value = prev_value;
		game->current_np_material = prev_material;
		unapply(game, moves[current_index], removed_hash[0], captured_piece, previous_castling,
			previous_ply_counter, previous_last_move);

		// quits the function if the time allowed has been passed
		if (((double)clock() - (double)start_time) / CLOCKS_PER_SEC > time_allowed) {
			free(move_order);
			return node_value;
		}
		if (child_value > node_value) {
			node_value = child_value;
			move_number[0] = current_index;
			// alpha beta pruning: if the other player can already guarantee themselves a better score higher up the tree, they won't need to search this path.
			if (node_value > beta) {
				add_hash_table_entry(transposition_table, game->hash, depth, 1, node_value, current_index);
				node->value = -node_value;
				node->evaluated = true;
				free(move_order);
				return node_value;
			}
			else if (node_value > alpha) {
				alpha = node_value;
			}
		}
		/*
		if (depth == max_depth) {
			print_move(moves[current_index]);
			printf("Value: %f\n", child_value);
		}
		*/
	}
	add_hash_table_entry(transposition_table, game->hash, depth, 0, node_value, move_number[0]);
	node->value = -node_value;
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
	//printf("Quiescence move: ");
	//print_move(move);
	update_value(game, move, captured_piece);
	value = -quiescence(game, zobrist_numbers, move_number, start_time, time_allowed, max_depth, -beta, -alpha, nodes, transposition_table, table_hits, node);
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
	if (stand_pat > beta) {
		return stand_pat;
	}

	if ((((double)clock() - (double)start_time) / CLOCKS_PER_SEC) > time_allowed) {
		return 0;
	}

	// gets the legal moves for the position
	int i;
	int j;
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
		else if ((flag & 4) != 0) {
			// type of piece captured, 0 for pawn through to 5 for king in order of value
			piece = (int)((moves[i][2] >> 4) & 15) % 6;
			// finds pieces which are higher value than the piece doing the capturing, and verifies that the capture is going to provide a value 
			// which might compare with the best score than it can guarantee itself higher up (otherwise there is no use in searching)
			for (j = piece + 1; j < 5; j++) {
				if (stand_pat + safety_margin + values[j] > alpha) {
					// if any of the higher value pieces is the one captured by this move, the move is expanded.
					if ((game->board[j + shift] & moves[i][1]) != 0) {
						new_value = run_quiescence(game, zobrist_numbers, move_number, start_time, time_allowed, max_depth, alpha, beta, nodes, transposition_table, table_hits, &(node->children[i]), moves[i]);
						break;
					}
				}
			}
		}

		if ((((double)clock() - (double)start_time) / CLOCKS_PER_SEC) > time_allowed) {
			return 0;
		}

		// Improves alpha and best_value depending on how good new_value is.
		// Performs a beta cutoff if black can already guarantee themselves a better value higher up
		if (new_value > best_value) {
			best_value = new_value;
			if (best_value > beta) {
				return best_value;
			}
			else if (best_value > alpha) {
				alpha = best_value;
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
	double time_change;
	clock_t previous_time;
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
		//printf("Depth: %d\n", depth[0]);
		value = minimax(game, depth[0], zobrist_numbers, move_number, start_time, time_allowed, depth[0], (float)-2000, (float)2000, nodes, transposition_table, table_hits, root_node);
		//printf("Value: %f\n", value);
		previous_time = current_time;
		current_time = clock();
		time_change = ((double)current_time - (double)previous_time) / CLOCKS_PER_SEC;
		time_taken = ((double)current_time - (double)start_time) / CLOCKS_PER_SEC;
		depth[0]++;

		// if the evaluation of the first node was cut short, it will return -2000 as a value and this search should be disregarded.
		if (value != -2000) {
			if (game->to_play == 0) {
				current_value[0] = value;
			}
			else {
				current_value[0] = -value;
			}
			current_move_number[0] = move_number[0];

			// if it has found a mate, plays the move which leads to mate.
			// equally if the time remaining is less than the time taken for the previous depth, 
			// it is assumed that it will not make it to the next depth and so returns
			if (value > 1000 || value < -1000) {
				free_stuff(transposition_table, root_node);
				return;
			}
		}
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

/*
Adds noise to the piece square tables, mode = 0 for middlegame, 1 for endgame.
Noise is added with a uniform distribution, width 2 * randomiser_amount (i.e it increases/decreases by a max of randomiser amount)
*/
void create_new_piece_square_table(int mode, float randomiser_amount, bool seed) {
	if (seed) {
		srand((unsigned)time(NULL));
	}
	int i;
	int j;
	int k;
	for (i = 0; i < 6; i++) {
		for (j = 0; j < 8; j++) {
			for (k = 0; k < 8; k++) {
				if (mode == 0) {
					piece_square_table_mg[i][j][k] += (float)(((double)rand() / RAND_MAX) * 2 * randomiser_amount - randomiser_amount);
				}
				else if (mode == 1) {
					piece_square_table_eg[i][j][k] += (float)(((double)rand() / RAND_MAX) * 2 * randomiser_amount - randomiser_amount);
				}
			}
		}
	}
}

/*
Sets a piece square table to the one specified in psqt. Mode = 0 for middlegame, 1 for endgame.
*/
void set_piece_square_table(float psqt[6][8][8], int mode) {
	int i;
	int j;
	int k;
	for (i = 0; i < 6; i++) {
		for (j = 0; j < 8; j++) {
			for (k = 0; k < 8; k++) {
				if (mode == 0) {
					piece_square_table_mg[i][j][k] = psqt[i][j][k];
				}
				else if (mode == 1) {
					piece_square_table_eg[i][j][k] = psqt[i][j][k];
				}
			}
		}
	}
}

/*
A genetic algorithm is used to improve the piece-square tables. This is used to breed two of them (by averaging their values).
*/
void breed(float psqt[6][8][8], int mode) {
	int i;
	int j;
	int k;
	for (i = 0; i < 6; i++) {
		for (j = 0; j < 8; j++) {
			for (k = 0; k < 8; k++) {
				if (mode == 0) {
					piece_square_table_mg[i][j][k] -= (piece_square_table_mg[i][j][k] - psqt[i][j][k]) / 2;
				}
				else if (mode == 1) {
					piece_square_table_eg[i][j][k] -= (piece_square_table_eg[i][j][k] - psqt[i][j][k]) / 2;
				}
			}
		}
	}
}

// prints out a piece-square table in a format where it can be copied and pasted into the code
void print_piece_square_table(float psqt[6][8][8]) {
	int i;
	int j;
	int k;
	printf("{");
	for (i = 0; i < 6; i++) {
		printf("{");
		for (j = 0; j < 8; j++) {
			printf("{");
			for (k = 0; k < 8; k++) {
				printf("%f", psqt[i][j][k]);
				if (k != 7) {
					printf(", ");
				}
			}
			if (j != 7) {
				printf("},\n");
			}
			else {
				printf("}");
			}
		}
		if (i != 5) {
			printf("},\n\n");
		}
		else {
			printf("}");
		}
	}
	printf("}\n\n");
}

void print_piece_square_tables() {
	print_piece_square_table(piece_square_table_mg);
	print_piece_square_table(piece_square_table_eg);
}

// returns a pointer to a piece square table, mg if type = 0, eg if type = 1
void* get_psqt(int type) {
	if (type == 0) {
		return &(piece_square_table_mg);
	}
	else {
		return &(piece_square_table_eg);
	}
}