# Dragonrose_Cpp Chess Engine

![Build Status](https://github.com/TampliteSK/Dragonrose_Cpp/actions/workflows/build.yml/badge.svg)

## Overview
Dragonrose is a [UCI](https://en.wikipedia.org/wiki/Universal_Chess_Interface)-compliant chess engine written in C++. <br>
Dragonrose is a command-line interface (CLI). It is highly recommended to use Dragonrose via a graphic-user interface (GUI)
that supports UCI, such as [En Croissant](https://encroissant.org/), [Arena](http://www.playwitharena.de/) or [Banksia](https://banksiagui.com/), in order to have a better user experience.
This repository is a **rewrite of the original [C Dragonrose](https://github.com/TampliteSK/dragonrose)**. <br>
Notable differences with older repo: <br>
- Uses magic bitboards to generate attacks, storing them in pre-generated attack tables for lookups later
- No longer uses the built-in opening book that comes with VICE
- Consistent naming convention, data types and parameter ordering
- Takes inspiration from more engines

You are free to borrow and/or modify my code if you so wish, as long as you give credit. <br>
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
| Hash  | integer (spin)  |    64   |   [1, 65536]   | Size of the transposition table in megabytes (MB). 16 - 512 MB is recommended.                               |
| Bench |  CLI Argument   |    -    |        -       | Run `./Dragonrose_Cpp bench` (or whatever you named the binary) from a CLI to check nodes and NPS, based on a 50-position suite (from [Heimdall](https://git.nocturn9x.space/nocturn9x/heimdall)).|

## Main Features

Search:
- Negamax alpha-beta search (fail-hard)
  - PV-search (TODO)
  - Null-move pruning
  - Futility pruning (extended to depth 3)
  - Late move pruning
- Quiesence search (fail-soft)
- Move ordering: MVV/LVA, Killer heuristics, Priority moves (Castling, en passant)
- Transposition table using "age"
- Iterative deepening + Aspiration windows

Evaluation (Hand-crafted evaluation, or HCE):
- Tapered eval
  - Material
  - Piece-square table bonuses
  - Piece activity	
  - Tempi
- King safety
  - Attack units
  - Pawn shield
  - King mobility (in endgames)
- Pawn structure
  - Passed pawns bonus
  - Isolated pawns penalty (extra penalty for isolated d/e pawns)
  - Stacked pawns penalty
  - Backwards pawns penalty
- Piece bounses / penalties
  - Penalty for bishops blocking centre pawns
  - Bonus for semi-open and open files for rooks and queens
  - Bonus for sliders to target enemy pieces
  - Bonus for batteries (bishop + queen, rook + queen, rook + rook)
- Basic endgaeme knowledge	 

## Playing Strength
- The current C++ dev version, as well as the strongest C version (0.25) are about 2100 CCRL, benched against Stash v17 (2295 CCRL 2+1).
- Note that the estimated ratings in the C repo are not accurate, as the engine used in gauntlets are bugged.

- Below are some other statistics about the engine:
| Metric | Rapid | Blitz | Bullet |
| --- | --- | --- | --- |
| CCRL | ? | ? | 2116? |
| Lichess (BOT) | 2208 ± 66 | 2019 ± 52 | 2102 ± 60 |
| Chesscom* (est.) | 2591 ± 239 | 2760 ± 178 | 2659 ± 227 |
*: These Chesscom ratings are estimated based on its games against human players (rated 1800 - 2500), though the sample size is fairly small.

## Useful Reesources
- VICE video playlist (as linked above)
- [Chess Programming Wiki](https://www.chessprogramming.org/Main_Page). Great resource in general to learn concepts.
- Analog Hors for [this guide](https://analog-hors.github.io/site/magic-bitboards/) explaining magic bitboards in a understandable format. By far much better than any other resource I have found.
- nemequ, mbitsnbites, zhuyie et al. for [TinyCThread](https://github.com/tinycthread/tinycthread/tree/master).

## Changelogs <br>
### 0.x: <br>
0.29 (dev): Completely rewritten from scratch, on par with 0.28 and 0.25. Using more aggressive FP/EFP, as well as more evaluation terms.

## To-do list
- Other search / eval enhancements
- Add mate distance pruning
- Release
- ...
- Search thread / LazySMP
- Add Chess960 support

## Bugs to fix:
- May blunder threefold in a winning position due to how threefold is implemented