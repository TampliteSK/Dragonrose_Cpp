# Dragonrose_Cpp Chess Engine

## Overview

**A rewrite of the original [Dragonrose](https://github.com/TampliteSK/dragonrose) in C++.** <br>
Notable differences with older repo: <br>
- Uses magic bitboards to generate attacks, as well as pre-generated attack tables
- Custom movegen 
- Consistent naming convention, data types and parameter ordering
- Taking inspiration from more engines
Through the rewrite, the code quality is expected to increase, and my knowledge of C++ can be improved. <br>
You are free to borrow and modify my code if you so wish, as long as you give credit. <br>
To use the engine, either grab the binary from releases, or build the project locally. In the future I will consider releasing Linux builds. <br>

## Inspirations and Acknowledgments

Dragonrose is greatly inspired by [VICE chess engine](https://github.com/bluefeversoft/vice/tree/main/Vice11/src), developed by by Richard Allbert (Bluefever Software), 
which is a didactic chess engine aimed to introduce beginners to the world of chess programming. Huge credits to him for creating the [YouTube series](https://www.youtube.com/playlist?list=PLZ1QII7yudbc-Ky058TEaOstZHVbT-2hg) explaining the engine in great detail. <br>

Aside from VICE, Dragonrose also draws inspiration from other engines such as 
- [Ethereal](https://github.com/AndyGrant/Ethereal) by Andrew Grant (AndyGrant) et al.
- [Weiss](https://github.com/TerjeKir/weiss) by Terje Kirstihagen (TerjeKir) et al.
- [BBC](https://github.com/maksimKorzh/bbc/tree/master) by Maksim Korzh (Code Monkey King), and
- [Caissa](https://github.com/Witek902/Caissa) by Michal Witanowski (Witek902) et al.

I would also like to thank [Adam Kulju](https://github.com/Adam-Kulju), developer of Willow and Patricia, for helping out with my code.

## How to Use

- Challenge it on Lichess [here](https://lichess.org/@/DragonroseDev)
- To run it locally either download a binary from releases or build it yourself with the makefile. Run `make CXX=<compiler>` and replace compiler with your preferred compiler (g++ / clang++). With it you can pick one of two options:
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
- Negamax alpha-beta search (fail-hard)
  - PV-search
  - Null-move pruning
  - Futility pruning (TODO)
  - Late move pruning (TODO)
- Quiesence search (fail-soft)
  - Delta pruning (TODO)
- Move ordering: MVV/LVA, Killer heuristics, Priority moves (Castling, en passant)
- Iterative deepening
- Transposition table using "age"
- Polyglot opening books

Evaluation (Hand-crafted evaluation, or HCE):
- Tapered eval
  - Material
  - Piece-square table bonuses
- TODO:
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
0.29 (dev): Completely rewritten from scratch. Using fail-soft for Quiescence Search and Negamax Alpha-beta Search

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