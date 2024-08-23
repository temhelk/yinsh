#ifndef YINSH_GUI_BOARD_HPP
#define YINSH_GUI_BOARD_HPP

#include <yinsh-gui/coords.hpp>

#include <yngine/moves.hpp>

#include <array>
#include <cstdint>
#include <vector>

enum class Node {
    NotInGame = 0,

    Empty,

    WhiteRing,
    BlackRing,

    WhiteMarker,
    BlackMarker,
};

class BoardStorage {
public:
    // Initialzes an empty board with correct NotInGame nodes
    BoardStorage();

    Node& at(int32_t x, int32_t y);
    Node& at(HVec2 pos);

    Node get_at(int32_t x, int32_t y) const;
    Node get_at(HVec2 pos) const;

private:
    std::array<Node, 11*11> nodes;
};

// These offsets tell us where the board begins and ends relative to
// a parallelogram that encloses the board
inline const int32_t BOARD_START_OFFSET[11] = {
    6, 4, 3, 2, 1, 1, 0, 0, 0, 0, 1
};

inline const int32_t BOARD_END_OFFSET[11] = {
    9, 10, 10, 10, 10, 9, 9, 8, 7, 6, 4
};

class BoardState {
public:
    enum class NextAction {
        RingPlacement,
        RingMovement,
        RowRemoval,
        RingRemoval,
        GameOver,
    };

    NextAction get_next_action() const;
    bool is_in_game(HVec2 pos) const;
    Node get_at(HVec2 pos) const;
    bool is_whites_move() const;

    bool is_move_legal(Yngine::Move move) const;
    void apply_move(Yngine::Move move);

    std::vector<HVec2> get_ring_moves(HVec2 pos) const;

private:
    void place_ring(HVec2 pos);
    void move_ring(HVec2 from, HVec2 to);
    void remove_row(HVec2 from, HVec2 to);
    void remove_ring(HVec2 pos);

    // Accepts last move from and to coordinates because that's the place where
    // rows might have had formed
    void check_for_rows_and_change_state(HVec2 from, HVec2 to);
    int number_of_markers_on_the_board() const;

    BoardStorage storage;

    NextAction next_action = NextAction::RingPlacement;
    bool white_moves_next = true;

    // Used when removing rings, to find the correct player to move next
    bool white_made_last_movement = false;

    int white_rings_on_board = 0;
    int black_rings_on_board = 0;

    HVec2 last_move_from;
    HVec2 last_move_to;
};

#endif // YINSH_GUI_BOARD_HPP
