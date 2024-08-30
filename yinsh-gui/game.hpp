#ifndef YINSH_GUI_GAME_HPP
#define YINSH_GUI_GAME_HPP

#include <yinsh-gui/board.hpp>

#include <yngine/mcts.hpp>

#include <raylib-cpp.hpp>
#include <optional>

class Game {
public:
    Game() = default;
    Game(const Game &) = delete;
    Game &operator=(const Game &) = delete;

    void run();

private:
    void update();
    std::optional<Yngine::Move> get_player_move();

    void render();
    void draw_board();

    // Update the camera parameters to get the correct view when window size changes
    void update_camera();

    HVec2 get_mouse_hex_pos();

    raylib::Window window;
    raylib::Camera2D camera;

    BoardState board_state;

    std::optional<HVec2> selected_ring; // Ring that the player wants to move
    std::vector<HVec2> ring_moves;

    // First node of the row that the player selects to remove
    std::optional<HVec2> row_remove_from;
    // Used to draw the line player selected
    HVec2 row_remove_to;

    // @temp
    Yngine::MCTS mcts;
};

#endif // YINSH_GUI_GAME_HPP
