// UciHandler.hpp

/*
        Forked from VICE uci.c by Richard Allbert (Bluefever Software)
*/

#ifndef UCIHANDLER_HPP
#define UCIHANDLER_HPP

#include "Board.hpp"
#include "search.hpp"

#define ENGINE_NAME "Dragonrose 0.29"

/*
|------------|-----------------------------------------------------------------------|
|  Commands  | Response. * denotes that the command blocks until no longer searching |
|------------|-----------------------------------------------------------------------|
|        uci |           Outputs the engine name, authors, and all available options |
|    isready | *           Responds with readyok when no longer searching a position |
| ucinewgame | *  Resets the TT and any Hueristics to ensure determinism in searches |
|  setoption | *     Sets a given option and reports that the option was set if done |
|   position | *  Sets the board position via an optional FEN and optional move list |
|         go | *       Searches the current position with the provided time controls |
|       stop |            Signals the search threads to finish and report a bestmove |
|       quit |             Exits the engine and any searches by killing the UCI loop |
|      perft |            Custom command to compute PERFT(N) of the current position |
|      print |         Custom command to print an ASCII view of the current position |
|------------|-----------------------------------------------------------------------|
*/

// UCI options struct
typedef struct {
    uint16_t hash_size;  // type spin
    uint16_t threads;    // type spin
                         // bool use_book; // type check
} UciOptions;

class UciHandler {
   public:
    UciHandler();  // Blank constructor

    void parse_go(Board *pos, HashTable *table, SearchInfo *info, const std::string &line);
    void parse_position(Board *pos, const std::string &line);
    void uci_loop(Board *pos, HashTable *table, SearchInfo *info, UciOptions *options);

   private:
    int get_value_from_line(const std::string &line, const std::string &key);
};

#endif  // UCIHANDLER_HPP