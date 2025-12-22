// evaluate.hpp

#ifndef EVALUATE_HPP
#define EVALUATE_HPP

#include "../chess/Board.hpp"
#include "../datatypes.hpp"
#include "ScorePair.hpp"

// Functions
int evaluate_pos(const Board& pos);
int count_material(const Board& pos, const uint8_t phase);

/*
    Evaluation constants
*/

// indicies:  0  1  2  3  4  5  6  7
// ranks:    r8 r7 r6 r5 r4 r3 r2 r1
// 7-ranks:  r1 r2 r3 r4 r5 r6 r7 r8
const int passer_bonus[8] = {0, 5, 10, 20, 35, 60, 100, 200};
const uint8_t connected_passers = 50;
const uint8_t isolated_pawn = 10;         // Base penalty of isolated pawns
const uint8_t isolated_centre_pawn = 10;  // Additional penalty for isolated pawns on d/e files
const uint8_t stacked_pawn = 5;           // Multiplied by n-1, where n is no. of stacked pawns
const uint8_t backwards_pawn = 15;

// Rescaled from Clockwork 40552d7
const ScorePair PAWN_PHALANX[8] = {
    S(0, 0), S(0, -5), S(3, 2), S(23, 16), S(59, 35), S(103, 209), S(325, 383), S(0, 0)
};

// inline ScorePair bishop_pair = S(28, 72);
// inline tempo = S(9, 2);

const uint8_t bishop_pair = 20;
const uint8_t bishop_blocks_ctrpawn = 20;
const uint8_t bishop_attacks_piece = 5;
const uint8_t rook_open_file = 10;
const uint8_t rook_semiopen_file = 5;
const uint8_t rook_attacks_piece = 5;
const uint8_t queen_open_file = 5;
const uint8_t queen_semiopen_file = 3;
const uint8_t queen_attacks_piece = 3;
const uint8_t battery = 10;  // B+Q, R+R, Q+R

const Bitboard DEVELOPMENT_MASK = 0x7E7E7E7E7E7E00ULL;  // B2-G7 set

// clang-format off
// ======================================================================
// MOBILITY BONUS TABLES
// Values source: Sirius (mcthouacbb / A_randomnoob) | Explanation inspired by Weak chess engine (Dragjon)
// ======================================================================
//
// Mobility is a measure of piece activity based on the number of squares a 
// piece can attack. Each piece can attack up to 28 squares.
// In general, the more squares a piece attacks the better. However, queens
// are given penalties for having high mobility in the opening, to discourage
// early queen movement.
// 
// Table format:
//   MOBILITY_VALUES[PieceType][AttackCount] → ScorePair(mg, eg)
//
//   PieceType:  0 = Knight  | max  8 attacks
//               1 = Bishop  | max 13 attacks
//               2 = Rook    | max 14 attacks
//               3 = Queen   | max 27 attacks
//
// Unused entries beyond max attacks per piece are zeroed.
const ScorePair MOBILITY_VALUES[4][28] = {
    // ── Knights (0 – 8 attacks) ────────────────────────────────────────
    {
        S(  -7,  -22), S( -36,  -55), S( -21,  -18), S( -10,   -2),
        S(   0,    8), S(   6,   18), S(  14,   22), S(  22,   26),
        S(  32,   16)
        // 9–27: unused (0,0)
    },
    // ── Bishops (0 – 13 attacks) ───────────────────────────────────────
    {
        S( -12,  -31), S( -42,  -62), S( -22,  -32), S( -11,  -11),
        S(  -4,   -1), S(   0,    9), S(   1,   16), S(   5,   19),
        S(   4,   21), S(   8,   21), S(   6,   22), S(  12,   15),
        S(  17,   13), S(  40,  -11)
        // 14–27: unused
    },
    // ── Rooks (0 – 14 attacks) ─────────────────────────────────────────
    {
        S( -25,  -62), S( -49,  -63), S( -18,  -43), S( -10,  -26),
        S(  -2,  -15), S(   3,   -5), S(   2,    6), S(   4,   11),
        S(   5,   14), S(   7,   21), S(   9,   27), S(   9,   34),
        S(  10,   38), S(  14,   39), S(  40,   18)
        // 15–27: unused
    },
    // ── Queens (0 – 27 attacks) ────────────────────────────────────────
    {
        S( -14,   41), S( -41,    7), S( -60,  -13), S( -41, -125),
        S( -26, -105), S( -12,  -68), S(  -4,  -51), S(   0,  -33),
        S(   0,  -15), S(   1,   -1), S(   4,    6), S(   3,   16),
        S(   6,   22), S(   6,   30), S(   7,   34), S(   9,   34),
        S(   7,   40), S(   8,   39), S(  10,   40), S(  12,   34),
        S(  16,   31), S(  20,   17), S(  20,   17), S(  28,    4),
        S(  22,    9), S(  23,   -9), S(  22,  -37), S( -38,   11)
    }
};
// clang-format on

// King safety
// Attack units table from Stockfish, rescaled to centipawns
const int safety_table[100] = {
    0,   0,   1,   2,   3,   5,   7,   9,   12,  15,  18,  22,  26,  30,  35,  39,  44,
    50,  56,  62,  68,  75,  82,  85,  89,  97,  105, 113, 122, 131, 140, 150, 169, 180,
    191, 202, 213, 225, 237, 248, 260, 272, 283, 295, 307, 319, 330, 342, 354, 366, 377,
    389, 401, 412, 424, 436, 448, 459, 471, 483, 494, 500, 500, 500, 500, 500, 500, 500,
    500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
    500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500};

/*
    PesTO Material and PSQT Evaluation
*/

// Material
inline ScorePair piece_values[13] = {
    S(0, 0),  // EMPTY
    S(82, 94),   S(337, 281),  S(365, 297),
    S(477, 512), S(1025, 936), S(0, 0),  // wP, wN, wB, wR, wQ, wK
    S(82, 94),   S(337, 281),  S(365, 297),
    S(477, 512), S(1025, 936), S(0, 0)  // bP, bN, bB, bR, bQ, bK
};

// Piece-square Tables
inline ScorePair PSQT[6][64] = {
    {// Pawns
     S(0, 0),    S(0, 0),     S(0, 0),     S(0, 0),    S(0, 0),    S(0, 0),    S(0, 0),
     S(0, 0),    S(98, 178),  S(134, 173), S(61, 158), S(95, 134), S(68, 147), S(126, 132),
     S(34, 165), S(-11, 187), S(-6, 94),   S(7, 100),  S(26, 85),  S(31, 67),  S(65, 56),
     S(56, 53),  S(25, 82),   S(-20, 84),  S(-14, 32), S(13, 24),  S(6, 13),   S(21, 5),
     S(23, -2),  S(12, 4),    S(17, 17),   S(-23, 17), S(-27, 13), S(-2, 9),   S(-5, -3),
     S(12, -7),  S(17, -7),   S(6, -8),    S(10, 3),   S(-25, -1), S(-26, 4),  S(-4, 7),
     S(-4, -6),  S(-10, 1),   S(3, 0),     S(3, -5),   S(33, -1),  S(-12, -8), S(-35, 13),
     S(-1, 8),   S(-20, 8),   S(-23, 10),  S(-15, 13), S(24, 0),   S(38, 2),   S(-22, -7),
     S(0, 0),    S(0, 0),     S(0, 0),     S(0, 0),    S(0, 0),    S(0, 0),    S(0, 0),
     S(0, 0)},
    {// Knights
     S(-167, -58), S(-89, -38), S(-34, -13), S(-49, -28), S(61, -31),  S(-97, -27), S(-15, -63),
     S(-107, -99), S(-73, -25), S(-41, -8),  S(72, -25),  S(36, -2),   S(23, -9),   S(62, -25),
     S(7, -24),    S(-17, -52), S(-47, -24), S(60, -20),  S(37, 10),   S(65, 9),    S(84, -1),
     S(129, -9),   S(73, -19),  S(44, -41),  S(-9, -17),  S(17, 3),    S(19, 22),   S(53, 22),
     S(37, 22),    S(69, 11),   S(18, 8),    S(22, -18),  S(-13, -18), S(4, -6),    S(16, 16),
     S(13, 25),    S(28, 16),   S(19, 17),   S(21, 4),    S(-8, -18),  S(-23, -23), S(-9, -3),
     S(12, -1),    S(10, 15),   S(19, 10),   S(17, -3),   S(25, -20),  S(-16, -22), S(-29, -42),
     S(-53, -20),  S(-12, -10), S(-3, -5),   S(-1, -2),   S(18, -20),  S(-14, -23), S(-19, -44),
     S(-105, -29), S(-21, -51), S(-58, -23), S(-33, -15), S(-17, -22), S(-28, -18), S(-19, -50),
     S(-23, -64)},
    {// Bishops
     S(-29, -14), S(4, -21),   S(-82, -11), S(-37, -8), S(-25, -7),  S(-42, -9),  S(7, -17),
     S(-8, -24),  S(-26, -8),  S(16, -4),   S(-18, 7),  S(-13, -12), S(30, -3),   S(59, -13),
     S(18, -4),   S(-47, -14), S(-16, 2),   S(37, -8),  S(43, 0),    S(40, -1),   S(35, -2),
     S(50, 6),    S(37, 0),    S(-2, 4),    S(-4, -3),  S(5, 9),     S(19, 12),   S(50, 9),
     S(37, 14),   S(37, 10),   S(7, 3),     S(-2, 2),   S(-6, -6),   S(13, 3),    S(13, 13),
     S(26, 19),   S(34, 7),    S(12, 10),   S(10, -3),  S(4, -9),    S(0, -12),   S(15, -3),
     S(15, 8),    S(15, 10),   S(14, 13),   S(27, 3),   S(18, -7),   S(10, -15),  S(4, -14),
     S(15, -18),  S(16, -7),   S(0, -1),    S(7, 4),    S(21, -9),   S(33, -15),  S(1, -27),
     S(-33, -23), S(-3, -9),   S(-14, -23), S(-21, -5), S(-13, -9),  S(-12, -16), S(-39, -5),
     S(-21, -17)},
    {// Rooks
     S(32, 13),  S(42, 10),  S(32, 18),  S(51, 15),  S(63, 12), S(9, 12),  S(31, 8),   S(43, 5),
     S(27, 11),  S(32, 13),  S(58, 13),  S(62, 11),  S(80, -3), S(67, 3),  S(26, 8),   S(44, 3),
     S(-5, 7),   S(19, 7),   S(26, 7),   S(36, 5),   S(17, 4),  S(45, -3), S(61, -5),  S(16, -3),
     S(-24, 4),  S(-11, 3),  S(7, 13),   S(26, 1),   S(24, 2),  S(35, 1),  S(-8, -1),  S(-20, 2),
     S(-36, 3),  S(-26, 5),  S(-12, 8),  S(-1, 4),   S(9, -5),  S(-7, -6), S(6, -8),   S(-23, -11),
     S(-45, -4), S(-25, 0),  S(-16, -5), S(-17, -1), S(3, -7),  S(0, -12), S(-5, -8),  S(-33, -16),
     S(-44, -6), S(-16, -6), S(-20, 0),  S(-9, 2),   S(-1, -9), S(11, -9), S(-6, -11), S(-71, -3),
     S(-19, -9), S(-13, 2),  S(1, 3),    S(17, -1),  S(16, -5), S(7, -13), S(-37, 4),  S(-26, -20)},
    {// Queens
     S(-28, -9), S(0, 22),    S(29, 22),   S(12, 27),  S(59, 27),  S(44, 19),   S(43, 10),
     S(45, 20),  S(-24, -17), S(-39, 20),  S(-5, 32),  S(1, 41),   S(-16, 58),  S(57, 25),
     S(28, 30),  S(54, 0),    S(-13, -20), S(-17, 6),  S(7, 9),    S(8, 49),    S(29, 47),
     S(56, 35),  S(47, 19),   S(57, 9),    S(-27, 3),  S(-27, 22), S(-16, 24),  S(-16, 45),
     S(-1, 57),  S(17, 40),   S(-2, 57),   S(1, 36),   S(-9, -18), S(-26, 28),  S(-9, 19),
     S(-10, 47), S(-2, 31),   S(-4, 34),   S(3, 39),   S(-3, 23),  S(-14, -16), S(2, -27),
     S(-11, 15), S(-2, 6),    S(-5, 9),    S(2, 17),   S(14, 10),  S(5, 5),     S(-35, -22),
     S(-8, -23), S(11, -30),  S(2, -16),   S(8, -16),  S(15, -23), S(-3, -36),  S(1, -32),
     S(-1, -33), S(-18, -28), S(-9, -22),  S(10, -43), S(-15, -5), S(-25, -32), S(-31, -20),
     S(-50, -41)},
    {// Kings
     S(-65, -74), S(23, -35), S(16, -18), S(-15, -18), S(-56, -11), S(-34, 15),  S(2, 4),
     S(13, -17),  S(29, -12), S(-1, 17),  S(-20, 14),  S(-7, 17),   S(-8, 17),   S(-4, 38),
     S(-38, 23),  S(-29, 11), S(-9, 10),  S(24, 17),   S(2, 23),    S(-16, 15),  S(-20, 20),
     S(6, 45),    S(22, 44),  S(-22, 13), S(-17, -8),  S(-20, 22),  S(-12, 24),  S(-27, 27),
     S(-30, 26),  S(-25, 33), S(-14, 26), S(-36, 3),   S(-49, -18), S(-1, -4),   S(-27, 21),
     S(-39, 24),  S(-46, 27), S(-44, 23), S(-33, 9),   S(-51, -11), S(-14, -19), S(-14, -3),
     S(-22, 11),  S(-46, 21), S(-44, 23), S(-30, 16),  S(-15, 7),   S(-27, -9),  S(1, -27),
     S(7, -11),   S(-8, 4),   S(-64, 13), S(-43, 14),  S(-16, 4),   S(9, -5),    S(8, -17),
     S(-15, -53), S(36, -34), S(12, -21), S(-54, -11), S(8, -28),   S(-28, -14), S(24, -24),
     S(14, -43)}};

#endif  // EVALUATE_HPP