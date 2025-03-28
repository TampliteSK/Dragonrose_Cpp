# Dragonrose Cpp Chess Engine

## Overview

**A rewrite of the original [Dragonrose](https://github.com/TampliteSK/dragonrose) in C++.** <br>
Notable differences with older repo: <br>
- Inherent use of magic bitboards and pre-generated attack tables
- More addition of original code (e.g. movegen)
- Taking inspiration from more engines
- Consistent naming convention, data types and parameter ordering
Through the rewrite, the code quality is expected to increase, and my knowledge of C++ can be improved. <br>
You are free to borrow and modify my code if you so wish, as long as you give credit. <br>
To use the engine, either grab the binary from releases, or build the project locally. In the future I will consider releasing Linux builds. <br>

## Inspirations

Dragonrose is greatly inspired by [VICE chess engine](https://github.com/bluefeversoft/vice/tree/main/Vice11/src), developed by by Richard Allbert (Bluefever Software), 
which is a didactic chess engine aimed to introduce beginners to the world of chess programming. Huge credits to him for creating the [YouTube series](https://www.youtube.com/playlist?list=PLZ1QII7yudbc-Ky058TEaOstZHVbT-2hg) explaining the engine in great detail. <br>

Aside from VICE, Dragonrose also draws inspiration from other engines such as 
- [Ethereal](https://github.com/AndyGrant/Ethereal) by Andrew Grant (AndyGrant) et al.
- [Weiss](https://github.com/TerjeKir/weiss) by Terje Kirstihagen (TerjeKir) et al.
- [BBC](https://github.com/maksimKorzh/bbc/tree/master) by Maksim Korzh (Code Monkey King), and
- [Caissa](https://github.com/Witek902/Caissa) by Michal Witanowski (Witek902) et al.

## How to Use

- Challenge it on Lichess [here](https://lichess.org/@/DragonroseDev)
- To run it locally either download a binary from releases or build it yourself with the makefile. Run `make CC=<compiler>` and replace compiler with your preferred compiler (gcc / clang). With it you can pick one of two options:
  - Plug it into a chess GUI such as Arena or Cutechess
  - Directly run the executable (usually for testing). You can run it normally with ./Dragonrose or run a benchmark with ./Dragonrose bench

## UCI options

| Name  |      Type       | Default |  Valid values  | Description                                                                                             |
|:-----:|:---------------:|:-------:|:--------------:|:-------------------------------------------------------------------------------------------------------:|
| Hash  | integer (spin)  |    64   |   [1, 65536]   | Size of the transposition table in megabytes. 16 - 512 MB is recommended.                               |
| Book  | boolean (check) |  TRUE   |  TRUE / FALSE  | Whether to use the internal book (VICEbook.bin). The book and the binary must be in the same directory. |
| Bench |  CLI Argument   |    -    |        -       | Run `./Dragonrose bench` from a CLI to check nodes and NPS based on a 50-position suite (from [Heimdall](https://git.nocturn9x.space/nocturn9x/heimdall)). As it stands the node count is inconsistent even with the same version.|

## Main Features

Search:
- Negamax Alpha-beta search
  - PV-search
  - Null-move pruning
  - Futility pruning
  - Late move pruning
- Quiesence search
  - Delta pruning
- Move ordering: MVV/LVA, Killer heuristics, Priority moves (Castling, en passant)
- Iterative deepening
- Transposition table using "age"
- Polyglot opening books

Evaluation (Hand-crafted evaluation, or HCE):
- Tapered eval
  - Material
  - Piece-square table bonuses
- King safety: customised king tropism, pawn shield, king open files penalty, king in centre penalty
- Piece bonuses: Rook/queen open-file bonuses
- Pawn bonuses / penalties: Passed pawns, isolated pawns, stacked pawns
- Endgame knowledge: Drawn endgame detection (7-man equivalent), 50-move rule adjustment

## Playing Strength

- Latest version is about 2350 CCRL in strength. At the moment it is quite inconsistent in tests, so the estimate may not be accurate.
- The Chesscom rating is estimated based on its games against human players (1800 - 2500). However it suffers greatly from small sample size, so take it with a grain of salt.

| Metric | Rapid | Blitz | Bullet |
| --- | --- | --- | --- |
| CCRL | 2350? | N/A | 2350? |
| Lichess (BOT) | 2221 ± 57 | 2074 ± 51 | 2104 ± 54 |
| Chesscom (est.) | 2591 ± 239 | 2760 ± 178 | 2659 ± 227 |

## Useful Reesources
- VICE video playlist (as linked above)
- [Chess Programming Wiki](https://www.chessprogramming.org/Main_Page). Great resource in general to learn concepts.
- Analog Hors for [this guide](https://analog-hors.github.io/site/magic-bitboards/) explaining magic bitboards in a understandable format. By far much better than any other resource I have found.
- nemequ, mbitsnbites, zhuyie et al. for [TinyCThread](https://github.com/tinycthread/tinycthread/tree/master).

## Changelogs <br>
### 0.x: <br>
0.28 (dev): Rework king safety. Added extended futility pruning. Improved endgame knowledge. | Elo gain: ~5. <br>
0.27c: Added doubled pawns. Improved time management. Minor code restructuring. <br>
0.27b: Improved aspiration windows. Improved move ordering. Added OpenBench support. | Elo gain: ~30. <br>
0.27: Added late move pruning. Improved drawn endgame detection. Optimised king tropism. | Elo gain: ~50. <br>
0.26d: Added aspiration windows. <br>
0.26: Added king tropism. Improved time management. <br>
0.25: Added futility pruning. | Elo gain: ~20. <br> 
0.24c: Enabled O3 optimisation. Added mate ouptut. <br>
0.24b: Fixed delta pruning. Optimised memory (variable sizes). <br>
0.24: Added delta pruning. Added punishments for minor pieces in front of pawns. | Elo gain: ~40. <br>
0.23: Added king safety to evaluation (pawn shield and punish open files near king). <br>
0.22: Improved time management (tested on Lichess). Slight speed boost. <br>
0.21: Slight speed boost. <br>
0.2: Fixed crash. Improved tapered eval. | Elo gain: ~240. <br>
0.11: Added tapered eval to material (point values). <br>
0.1: Added tapered eval to PSQT. <br>

## To-do list
- Optimise attackgen (magic bitboard)
- Add draw score variation
- Add seldepth
- Tune LMR, Add RFP
- Improve king safety
- Rename all poorly-named functions / variables for consistency
- Add TT move ordering
- Add SEE
- Pawn / bishop interaction
- Search thread / LazySMP

## Bugs to fix:
- May blunder threefold in a winning position due to how threefold is implemented
- Fix perft command freeze
- ID loop needs to only exit when it has a legal move (i.e done depth 1 at least)
- Obscure illegal move bug that occurs once every 100-200 games. Not replicable just with FEN.