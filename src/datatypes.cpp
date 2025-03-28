// datatypes.cpp

#include "datatypes.hpp"

//                   EMPTY,    wP,    wN,    wB,    wR,    wQ,    wK,    bP,    bN,    bB,    bR,    bQ,    bK
int piece_type[13] = { NONE, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING,  PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING };
int piece_col[13] = { BOTH, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK };
bool piece_big[13] = { false, false, true, true, true, true, true, false, true, true, true, true, true };
bool piece_maj[13] = { false, false, false, false, true, true, true, false, false, false, true, true, true };
bool piece_min[13] = { false, false, true, true, false, false, false, false, true, true, false, false, false };

// evaluate.c
int Mirror64[64] = {
56	,	57	,	58	,	59	,	60	,	61	,	62	,	63	,
48	,	49	,	50	,	51	,	52	,	53	,	54	,	55	,
40	,	41	,	42	,	43	,	44	,	45	,	46	,	47	,
32	,	33	,	34	,	35	,	36	,	37	,	38	,	39	,
24	,	25	,	26	,	27	,	28	,	29	,	30	,	31	,
16	,	17	,	18	,	19	,	20	,	21	,	22	,	23	,
8	,	9	,	10	,	11	,	12	,	13	,	14	,	15	,
0	,	1	,	2	,	3	,	4	,	5	,	6	,	7
};

