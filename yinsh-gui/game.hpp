#ifndef YINSH_GUI_GAME_HPP
#define YINSH_GUI_GAME_HPP

#include <yinsh-gui/board.hpp>

#include <yngine/mcts.hpp>

#include <raylib-cpp.hpp>
#include <optional>
#include <string>
#include <vector>
#include <filesystem>

class Game {
public:
    Game();
    Game(Game &&) = delete;
    Game &operator=(Game &&) = delete;
    Game(const Game &) = delete;
    Game &operator=(const Game &) = delete;

    void run();

    friend void update_draw_frame(void* game_voidptr);

private:
    enum class State {
        ChoosingMode,
        ChoosingAISettings,
        Playing,
        Reviewing,
    };

    static void handle_upload_file(std::string const &filename, std::string const &mime_type, std::string_view buffer, void* game_ptr);

    void update();
    std::optional<Yngine::Move> get_player_move();

    void render();
    void draw_board(const BoardState& board);
    void draw_review_bar();
    void draw_engine_analysis();

    // Update the camera parameters to get the correct view when window size changes
    void update_camera();

    // Rebuild replay_board by replaying move_history[0..review_cursor)
    void rebuild_replay_board();

    // Generate engine replay board similar to rebuild_replay_board, but for engine's type
    Yngine::BoardState build_engine_replay_board(bool is_blitz);

    // Save move_history to a file in the native save format
    void save_game(const std::string& filename);

    // Write game to a stream in the native save format
    void save_game_stream(std::ostream& stream);

    // Load a native save file into move_history and enter Reviewing at move 0
    // Returns false and prints to stderr on parse/validation error
    bool load_game(const std::filesystem::path& path);

    // Load a native save from a stream, see load_game
    bool load_game_stream(std::istream& stream);

    // Reset all game state and return to the mode-selection screen
    void reset_game();

    HVec2 get_mouse_hex_pos();

    raylib::Window window;
    raylib::Camera2D camera;

    State state;
    bool white_is_ai;
    bool black_is_ai;
    float ai_move_time;
    // When true, the human places *all* rings during the initial placement phase
    // (including the AI's rings) so that custom openings can be tested.
    bool place_ai_rings;

    BoardState board_state;

    std::optional<HVec2> selected_ring; // Ring that the player wants to move
    std::vector<HVec2> ring_moves;

    // First node of the row that the player selects to remove
    std::optional<HVec2> row_remove_from;
    // Used to draw the line player selected
    HVec2 row_remove_to;

    // Move history and review state
    bool review_only = false; // if we loaded a game we don't want to be able to play
    bool reviewing_blitz = false; // @TODO: remove this and use save file, set it on load
    std::vector<Yngine::Move> move_history;
    std::size_t review_cursor = 0; // == move_history.size() when at live position
    BoardState replay_board;       // re-derived board for review mode
    bool auto_saved = false;       // true once we've auto-saved this game

    // Not null if we play against AI
    std::optional<Yngine::MCTS> engine;
    int engine_thread_count; // The thread count is configured per search, not in engine
                             // constructor, so we store it here until we use it
    std::chrono::high_resolution_clock::time_point engine_search_start_time;

    std::size_t total_system_memory;
    int system_max_threads;
};

// @TODO: deduplicate that code and move somewhere?
std::string move_to_string(Yngine::Move move);

#endif // YINSH_GUI_GAME_HPP
