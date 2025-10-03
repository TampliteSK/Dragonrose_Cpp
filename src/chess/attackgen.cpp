// attackgen.cpp

#include "attackgen.hpp"

#include "../datatypes.hpp"
#include "bitboard.hpp"

// Init globals
Bitboard pawn_attacks[2][64] = {{0}};      // pawn attacks table [side][square]
Bitboard knight_attacks[64] = {0};         // knight attacks table [square]
Bitboard king_attacks[64] = {0};           // king attacks table [square]
Bitboard bishop_masks[64] = {0};           // bishop attack masks
Bitboard rook_masks[64] = {0};             // rook attack masks
Bitboard bishop_attacks[64][512] = {{0}};  // bishop attacks table [square][occupancies]
Bitboard rook_attacks[64][4096] = {{0}};   // rook attacks table [square][occupancies]

/*
    Sliders attackgen
*/

// generate pawn attacks
Bitboard mask_pawn_attacks(int side, int sq) {
    Bitboard attacks = 0ULL;
    Bitboard bitboard = 0ULL;
    SET_BIT(bitboard, sq);

    // White pawns
    if (!side) {
        if ((bitboard >> 7) & not_a_file) attacks |= (bitboard >> 7);
        if ((bitboard >> 9) & not_h_file) attacks |= (bitboard >> 9);
    }
    // Black pawns
    else {
        if ((bitboard << 7) & not_h_file) attacks |= (bitboard << 7);
        if ((bitboard << 9) & not_a_file) attacks |= (bitboard << 9);
    }

    return attacks;
}

// generate knight attacks
Bitboard mask_knight_attacks(int sq) {
    Bitboard attacks = 0ULL;
    Bitboard bitboard = 0ULL;
    SET_BIT(bitboard, sq);

    if ((bitboard >> 17) & not_h_file) attacks |= (bitboard >> 17);
    if ((bitboard >> 15) & not_a_file) attacks |= (bitboard >> 15);
    if ((bitboard >> 10) & not_hg_file) attacks |= (bitboard >> 10);
    if ((bitboard >> 6) & not_ab_file) attacks |= (bitboard >> 6);
    if ((bitboard << 17) & not_a_file) attacks |= (bitboard << 17);
    if ((bitboard << 15) & not_h_file) attacks |= (bitboard << 15);
    if ((bitboard << 10) & not_ab_file) attacks |= (bitboard << 10);
    if ((bitboard << 6) & not_hg_file) attacks |= (bitboard << 6);

    return attacks;
}

// generate king attacks
Bitboard mask_king_attacks(int sq) {
    Bitboard attacks = 0ULL;
    Bitboard bitboard = 0ULL;
    SET_BIT(bitboard, sq);

    if (bitboard >> 8) attacks |= (bitboard >> 8);
    if ((bitboard >> 9) & not_h_file) attacks |= (bitboard >> 9);
    if ((bitboard >> 7) & not_a_file) attacks |= (bitboard >> 7);
    if ((bitboard >> 1) & not_h_file) attacks |= (bitboard >> 1);
    if (bitboard << 8) attacks |= (bitboard << 8);
    if ((bitboard << 9) & not_a_file) attacks |= (bitboard << 9);
    if ((bitboard << 7) & not_h_file) attacks |= (bitboard << 7);
    if ((bitboard << 1) & not_a_file) attacks |= (bitboard << 1);

    return attacks;
}

/*
    Leapers attackgen
*/

// mask bishop attacks
Bitboard mask_bishop_attacks(int sq) {
    Bitboard attacks = 0ULL;

    int r, f;
    // init target rank & files
    int tr = sq / 8;
    int tf = sq % 8;

    // mask relevant bishop occupancy bits
    for (r = tr + 1, f = tf + 1; r <= 6 && f <= 6; r++, f++) attacks |= (1ULL << (r * 8 + f));
    for (r = tr - 1, f = tf + 1; r >= 1 && f <= 6; r--, f++) attacks |= (1ULL << (r * 8 + f));
    for (r = tr + 1, f = tf - 1; r <= 6 && f >= 1; r++, f--) attacks |= (1ULL << (r * 8 + f));
    for (r = tr - 1, f = tf - 1; r >= 1 && f >= 1; r--, f--) attacks |= (1ULL << (r * 8 + f));

    return attacks;
}

// mask rook attacks
Bitboard mask_rook_attacks(int sq) {
    Bitboard attacks = 0ULL;

    int r, f;
    int tr = sq / 8;
    int tf = sq % 8;

    // mask relevant rook occupancy bits
    for (r = tr + 1; r <= 6; r++) attacks |= (1ULL << (r * 8 + tf));
    for (r = tr - 1; r >= 1; r--) attacks |= (1ULL << (r * 8 + tf));
    for (f = tf + 1; f <= 6; f++) attacks |= (1ULL << (tr * 8 + f));
    for (f = tf - 1; f >= 1; f--) attacks |= (1ULL << (tr * 8 + f));

    return attacks;
}

// generate bishop attacks on the fly
Bitboard bishop_attacks_on_the_fly(int sq, Bitboard blockers) {
    Bitboard attacks = 0ULL;

    int r, f;
    int tr = sq / 8;
    int tf = sq % 8;

    for (r = tr + 1, f = tf + 1; r <= 7 && f <= 7; r++, f++) {
        attacks |= (1ULL << (r * 8 + f));
        if ((1ULL << (r * 8 + f)) & blockers) break;
    }

    for (r = tr - 1, f = tf + 1; r >= 0 && f <= 7; r--, f++) {
        attacks |= (1ULL << (r * 8 + f));
        if ((1ULL << (r * 8 + f)) & blockers) break;
    }

    for (r = tr + 1, f = tf - 1; r <= 7 && f >= 0; r++, f--) {
        attacks |= (1ULL << (r * 8 + f));
        if ((1ULL << (r * 8 + f)) & blockers) break;
    }

    for (r = tr - 1, f = tf - 1; r >= 0 && f >= 0; r--, f--) {
        attacks |= (1ULL << (r * 8 + f));
        if ((1ULL << (r * 8 + f)) & blockers) break;
    }

    return attacks;
}

// generate rook attacks on the fly
Bitboard rook_attacks_on_the_fly(int sq, Bitboard blockers) {
    Bitboard attacks = 0ULL;

    int r, f;
    int tr = sq / 8;
    int tf = sq % 8;

    for (r = tr + 1; r <= 7; r++) {
        attacks |= (1ULL << (r * 8 + tf));
        if ((1ULL << (r * 8 + tf)) & blockers) break;
    }

    for (r = tr - 1; r >= 0; r--) {
        attacks |= (1ULL << (r * 8 + tf));
        if ((1ULL << (r * 8 + tf)) & blockers) break;
    }

    for (f = tf + 1; f <= 7; f++) {
        attacks |= (1ULL << (tr * 8 + f));
        if ((1ULL << (tr * 8 + f)) & blockers) break;
    }

    for (f = tf - 1; f >= 0; f--) {
        attacks |= (1ULL << (tr * 8 + f));
        if ((1ULL << (tr * 8 + f)) & blockers) break;
    }

    return attacks;
}

// get bishop attacks
Bitboard get_bishop_attacks(int sq, Bitboard occupancy) {
    occupancy &= bishop_masks[sq];
    occupancy *= bishop_magic_numbers[sq];
    occupancy >>= 64 - bishop_relevant_bits[sq];
    return bishop_attacks[sq][occupancy];
}

// get rook attacks
Bitboard get_rook_attacks(int sq, Bitboard occupancy) {
    occupancy &= rook_masks[sq];
    occupancy *= rook_magic_numbers[sq];
    occupancy >>= 64 - rook_relevant_bits[sq];
    return rook_attacks[sq][occupancy];
}

// get queen attacks
Bitboard get_queen_attacks(int sq, Bitboard occupancy) {
    return get_bishop_attacks(sq, occupancy) | get_rook_attacks(sq, occupancy);
}

/*
    Master functions
*/

// init leaper pieces attacks
void init_leapers_attacks() {
    for (int square = 0; square < 64; square++) {
        // init pawn attacks
        pawn_attacks[WHITE][square] = mask_pawn_attacks(WHITE, square);
        pawn_attacks[BLACK][square] = mask_pawn_attacks(BLACK, square);

        // init knight attacks
        knight_attacks[square] = mask_knight_attacks(square);

        // init king attacks
        king_attacks[square] = mask_king_attacks(square);
    }
}

// set occupancies
Bitboard set_occupancy(int index, int bits_in_mask, Bitboard attack_mask) {
    Bitboard copy = attack_mask;
    Bitboard occupancy = 0ULL;

    for (int count = 0; count < bits_in_mask; count++) {
        int square = pop_ls1b(copy);

        // make sure occupancy is on board
        if (index & (1 << count)) {
            occupancy |= (1ULL << square);
        }
    }

    return occupancy;
}

// init slider piece's attack tables
void init_sliders_attacks(int bishop) {
    for (int square = 0; square < 64; square++) {
        bishop_masks[square] = mask_bishop_attacks(square);
        rook_masks[square] = mask_rook_attacks(square);

        Bitboard attack_mask = bishop ? bishop_masks[square] : rook_masks[square];
        int relevant_bits_count = count_bits(attack_mask);
        int occupancy_indicies = (1 << relevant_bits_count);

        for (int index = 0; index < occupancy_indicies; index++) {
            // Bishop
            if (bishop) {
                Bitboard occupancy =
                    set_occupancy(index, relevant_bits_count,
                                  attack_mask);  // init current occupancy variation
                int magic_index = (occupancy * bishop_magic_numbers[square]) >>
                                  (64 - bishop_relevant_bits[square]);
                bishop_attacks[square][magic_index] = bishop_attacks_on_the_fly(square, occupancy);
            }
            // Rook
            else {
                Bitboard occupancy = set_occupancy(index, relevant_bits_count, attack_mask);
                int magic_index =
                    (occupancy * rook_magic_numbers[square]) >> (64 - rook_relevant_bits[square]);
                rook_attacks[square][magic_index] = rook_attacks_on_the_fly(square, occupancy);
            }
        }
    }
}