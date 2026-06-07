#include <yinsh-gui/game.hpp>
#include <yinsh-gui/coords.hpp>
#include <yinsh-gui/board.hpp>
#include <yinsh-gui/utils.hpp>
#include <yinsh-gui/system.hpp>

#include <raylib-cpp.hpp>
#define RAYGUI_IMPLEMENTATION
#include <raygui.h>

#include <rlImGui.h>
#include <imgui.h>

#if defined(EMSCRIPTEN)
    #include <emscripten/emscripten.h>
#endif

#include <cassert>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

static std::tm localtime_safe(std::time_t timer)
{
    std::tm result{};

#if defined(__unix__)
    localtime_r(&timer, &result);
#elif defined(_WIN32) || defined(_MSC_VER)
    localtime_s(&bt, &timer);
#else
    static std::mutex m;
    std::lock_guard<std::mutex> lock(m);
    bt = *std::localtime(&timer);
#endif

    return result;
}

static std::string get_local_time_string() {
    const auto now = std::chrono::floor<std::chrono::seconds>(
        std::chrono::system_clock::now()
    );

    const auto now_time_t = std::chrono::system_clock::to_time_t(now);

    std::tm local_time = localtime_safe(now_time_t);

    std::string file_name = std::format(
        "{:04}{:02}{:02}_{:02}{:02}{:02}",
        local_time.tm_year + 1900,
        local_time.tm_mon + 1,
        local_time.tm_mday,
        local_time.tm_hour,
        local_time.tm_min,
        local_time.tm_sec
    );

    return file_name;
}

Game::Game()
    : window{}
    , camera{}
    , state{Game::State::ChoosingMode}
    , white_is_ai{false}
    , black_is_ai{false}
    , ai_move_time{1.f}
    , place_ai_rings{false}
    , board_state{}
    , selected_ring{}
    , ring_moves{}
    , row_remove_from{}
    , row_remove_to{}
    , engine{} {
    this->total_system_memory = get_system_memory();
    this->system_max_threads = get_system_threads();
}

void update_draw_frame(void* game_voidptr) {
    auto game = static_cast<Game*>(game_voidptr);

#if defined(EMSCRIPTEN)
    // Keep the raylib window in sync with the browser canvas each frame
    const int cw = EM_ASM_INT({ return window.innerWidth; });
    const int ch = EM_ASM_INT({ return window.innerHeight; });
    if (cw > 0 && ch > 0 &&
        (cw != game->window.GetWidth() || ch != game->window.GetHeight())) {
        SetWindowSize(cw, ch);
    }
#endif

    game->update();
    game->render();
}

void Game::run() {
#if defined(EMSCRIPTEN)
    // On web, match the canvas to the browser window from the start
    const int initial_w = EM_ASM_INT({ return window.innerWidth; });
    const int initial_h = EM_ASM_INT({ return window.innerHeight; });
    const auto initial_window_size = raylib::Vector2{
        static_cast<float>(initial_w > 0 ? initial_w : 800),
        static_cast<float>(initial_h > 0 ? initial_h : 600)
    };
#else
    const auto initial_window_size = raylib::Vector2{1280, 720};
#endif

    SetConfigFlags(FLAG_MSAA_4X_HINT);

    this->window.Init(
        static_cast<int>(initial_window_size.x),
        static_cast<int>(initial_window_size.y),
        "Yinsh", FLAG_WINDOW_RESIZABLE
    );

    this->camera = raylib::Camera2D{
        initial_window_size / 2.f,
        to_vector2(HVec2{5, 5}.to_world())
    };
    this->update_camera();

    GuiSetStyle(DEFAULT, TEXT_SIZE, 20);

    rlImGuiSetup(true);
    IMGUI_CHECKVERSION();

#if defined(EMSCRIPTEN)
    emscripten_set_main_loop_arg(
        update_draw_frame,
        this,
        0,
        1
    );
#else
    SetTargetFPS(60);

    while (!this->window.ShouldClose()) {
        this->update();
        this->render();
    }
#endif

    std::cout << "Shutting down" << std::endl;
    rlImGuiShutdown();
}

void Game::update() {
    switch (this->state) {
    case State::ChoosingMode:
    case State::ChoosingAISettings: {
    } break;
    case State::Reviewing: {
        // AI search continues in background; we just don't apply input or moves.
    } break;
    case State::Playing: {
        if (this->board_state.get_next_action() == BoardState::NextAction::GameOver) {
            // Auto-save once when the game ends
            if (!this->auto_saved) {
                const auto time_str = get_local_time_string();
                std::string file_name = std::format(
                    "{}.txt",
                    time_str
                );
                this->save_game(file_name);
                this->auto_saved = true;
            }
            return;
        }

        const bool in_placement = this->board_state.get_next_action() == BoardState::NextAction::RingPlacement;
        const bool ai_turn =
            ( this->board_state.is_whites_move() && this->white_is_ai) ||
            (!this->board_state.is_whites_move() && this->black_is_ai);

        if (ai_turn && !(this->place_ai_rings && in_placement)) {
            assert(this->engine);

            if (!this->engine_move) {
                this->engine_move = this->engine->search(this->ai_move_time, this->engine_thread_count);
            } else {
                const auto move_status = this->engine_move->wait_for(std::chrono::seconds(0));

                if (move_status == std::future_status::ready) {
                    const auto move = this->engine_move->get();

                    this->board_state.apply_move(move);
                    engine->apply_move(move);

                    this->move_history.push_back(move);
                    this->review_cursor = this->move_history.size();

                    this->engine_move = std::nullopt;
                }
            }

        } else {
            const auto move = this->get_player_move();

            if (move) {
                if (this->board_state.is_move_legal(*move)) {
                    this->board_state.apply_move(*move);

                    if (this->engine) {
                        this->engine->apply_move(*move);
                    }

                    this->move_history.push_back(*move);
                    this->review_cursor = this->move_history.size();
                }
            }
        }
    } break;
    }
}

void Game::rebuild_replay_board() {
    this->replay_board = BoardState{};
    for (std::size_t i = 0; i < this->review_cursor; i++) {
        this->replay_board.apply_move(this->move_history[i]);
    }
}

// Convert an engine board index to "Letter+Number" notation (e.g. index -> "E4")
static std::string index_to_notation(uint8_t index) {
    const auto coords = Yngine::Bitboard::index_to_coords(index);
    const int x = coords.first;
    const int y = coords.second;
    const char letter = static_cast<char>('A' + x + y - 5);
    const int  number = y + 1;
    return std::string(1, letter) + std::to_string(number);
}

// Convert a RemoveRowMove endpoint (from + 4 steps in direction) to notation
static std::string row_end_notation(uint8_t from, Yngine::Direction dir) {
    const uint8_t end_index = Yngine::Bitboard::index_move_direction(from, dir, 4);
    return index_to_notation(end_index);
}

void Game::save_game(const std::string& path) {
    std::ofstream f(path);
    if (!f) {
        std::cerr << "Failed to save the game" << std::endl;
        return;
    }

    // Timestamp header
    const auto time_str = get_local_time_string();
    f << "# DATE " << time_str << "\n";

    // Player colour (human side); for PvP both sides are human
    if (!this->white_is_ai && !this->black_is_ai) {
        f << "# PLAYER_COLOR Both\n";
    } else {
        f << "# PLAYER_COLOR " << (this->white_is_ai ? "Black" : "White") << "\n";
    }

    // Result — count rings on board to determine winner
    const bool game_over = (this->board_state.get_next_action() == BoardState::NextAction::GameOver);
    if (!game_over) {
        f << "# RESULT Unfinished\n";
    } else {
        switch (this->board_state.game_result()) {
        case Yngine::GameResult::WhiteWon: {
            f << "# RESULT White\n";
        } break;
        case Yngine::GameResult::BlackWon: {
            f << "# RESULT Black\n";
        } break;
        case Yngine::GameResult::Draw: {
            f << "# RESULT Draw\n";
        } break;
        }
    }

    // Serialise each move
    for (const auto& move : this->move_history) {
        std::visit(Yngine::variant_overloaded{
            [&](const Yngine::PlaceRingMove& m) {
                f << "PLACE " << index_to_notation(m.index) << "\n";
            },
            [&](const Yngine::RingMove& m) {
                f << "MOVE " << index_to_notation(m.from)
                  << " "     << index_to_notation(m.to) << "\n";
            },
            [&](const Yngine::RemoveRowMove& m) {
                f << "REMOVE_ROW " << index_to_notation(m.from)
                  << " "           << row_end_notation(m.from, m.direction) << "\n";
            },
            [&](const Yngine::RemoveRingMove& m) {
                f << "REMOVE_RING " << index_to_notation(m.index) << "\n";
            },
            [&](const Yngine::PassMove&) {
                // No PASS line in format
            },
        }, move);
    }
}

// Parse "Letter+Number" notation (e.g. "E4") into a board index.
// Returns false if the string is malformed or out of range.
static std::optional<uint8_t> notation_to_index(const std::string& s) {
    if (s.size() < 2)
        return std::nullopt;

    const char letter = s[0];

    if (letter < 'A' || letter > 'K')
        return std::nullopt;

    int number = 0;
    std::from_chars(s.data() + 1, s.data() + s.size(), number, 10);
    if (number < 1 || number > 11)
        return std::nullopt;

    const int letter_index = letter - 'A';
    const int y = number - 1;
    const int x = (letter_index + 5) - y;

    if (x < 0 || x > 10 || y < 0 || y > 10)
        return std::nullopt;

    if (!Yngine::Bitboard::are_coords_in_game(x, y))
        return std::nullopt;

    return Yngine::Bitboard::coords_to_index(x, y);
}

// Derive direction from two board indices (must be on a straight line).
static std::optional<Yngine::Direction> indices_to_direction(uint8_t from_idx, uint8_t to_idx) {
    const auto from = Yngine::Bitboard::index_to_coords(from_idx);
    const auto to = Yngine::Bitboard::index_to_coords(to_idx);

    const auto from_3 = HVec3{HVec2{from.first, from.second}};
    const auto to_3 = HVec3{HVec2{to.first, to.second}};

    if (from_3 == to_3)
        return std::nullopt;

    const auto dir = from_3.direction_to(to_3);

    // Check if they're on the same line by trying to compute 'to' backwards
    const auto length = (to_3 - from_3).length();
    if ((from_3 + HVec3{HVec2::from_direction(dir)} * length) != to_3)
        return std::nullopt;

    return from_3.direction_to(to_3);
}

bool Game::load_game(const std::string& path) {
    std::ifstream f(path);
    if (!f) {
        std::cerr << "load_game: cannot open '" << path << "'\n";
        return false;
    }

    std::vector<Yngine::Move> loaded;
    BoardState validator;
    std::string line;
    int line_num = 0;

    while (std::getline(f, line)) {
        line_num++;
        // Strip trailing whitespace
        while (!line.empty() && (line.back() == '\r' || line.back() == ' '))
            line.pop_back();
        // Skip blank lines and comments
        if (line.empty() || line[0] == '#') continue;

        // Tokenise
        std::vector<std::string> tokens;
        std::istringstream ss(line);
        std::string tok;
        while (ss >> tok) tokens.push_back(tok);
        if (tokens.empty()) continue;

        Yngine::Move move{Yngine::PassMove{}};
        bool parsed = false;

        if (tokens[0] == "PLACE" && tokens.size() == 2) {
            const auto idx = notation_to_index(tokens[1]);
            if (!idx) goto parse_error;

            move = Yngine::PlaceRingMove{*idx};
            parsed = true;
        } else if (tokens[0] == "MOVE" && tokens.size() == 3) {
            const auto from_idx = notation_to_index(tokens[1]);
            const auto to_idx = notation_to_index(tokens[2]);
            if (!from_idx || !to_idx) goto parse_error;

            const auto dir = indices_to_direction(*from_idx, *to_idx);
            if (!dir) goto parse_error;

            move = Yngine::RingMove{*from_idx, *to_idx, *dir};
            parsed = true;
        } else if (tokens[0] == "REMOVE_ROW" && tokens.size() == 3) {
            const auto from_idx = notation_to_index(tokens[1]);
            const auto to_idx = notation_to_index(tokens[2]);
            if (!from_idx || !to_idx) goto parse_error;

            const auto dir = indices_to_direction(*from_idx, *to_idx);
            if (!dir) goto parse_error;

            move = Yngine::RemoveRowMove{*from_idx, *dir};
            parsed = true;
        } else if (tokens[0] == "REMOVE_RING" && tokens.size() == 2) {
            const auto idx = notation_to_index(tokens[1]);
            if (!idx) goto parse_error;

            move = Yngine::RemoveRingMove{*idx};
            parsed = true;
        } else {
            std::cerr << "load_game: unknown token '" << tokens[0] << "' at line " << line_num << "\n";
            return false;
        }

        if (parsed) {
            if (!validator.is_move_legal(move)) {
                std::cerr << "load_game: illegal move at line " << line_num << ": " << line << "\n";
                return false;
            }
            validator.apply_move(move);
            loaded.push_back(move);
        }
        continue;

parse_error:
        std::cerr << "load_game: parse error at line " << line_num << ": " << line << "\n";
        return false;
    }

    // Success — replace game state
    this->move_history  = std::move(loaded);
    this->review_cursor = 0;
    this->auto_saved    = true; // don't auto-save a loaded game
    this->board_state   = BoardState{};
    this->engine        = std::nullopt;
    this->engine_move   = std::nullopt;
    this->selected_ring = std::nullopt;
    this->ring_moves.clear();
    this->row_remove_from = std::nullopt;
    this->rebuild_replay_board();
    this->state = State::Reviewing;
    return true;
}

void Game::reset_game() {
    this->board_state     = BoardState{};
    this->replay_board    = BoardState{};
    this->move_history.clear();
    this->review_cursor   = 0;
    this->auto_saved      = false;
    this->selected_ring   = std::nullopt;
    this->ring_moves.clear();
    this->row_remove_from = std::nullopt;
    this->engine          = std::nullopt;
    this->engine_move     = std::nullopt;
    this->white_is_ai     = false;
    this->black_is_ai     = false;
    this->place_ai_rings  = false;
    this->state           = State::ChoosingMode;
}

std::optional<Yngine::Move> Game::get_player_move() {
    switch (this->board_state.get_next_action()) {
    case BoardState::NextAction::RingPlacement: {
        if (raylib::Mouse::IsButtonReleased(MOUSE_BUTTON_LEFT)) {
            const auto clicked_pos = this->get_mouse_hex_pos();

            if (this->board_state.is_in_game(clicked_pos)) {
                return Yngine::PlaceRingMove{
                    Yngine::Bitboard::coords_to_index(clicked_pos.x, clicked_pos.y)
                };
            }
        }
    } break;
    case BoardState::NextAction::RingMovement: {
        if (!this->board_state.ring_moves_available()) {
            return Yngine::PassMove{};
        }

        if (raylib::Mouse::IsButtonReleased(MOUSE_BUTTON_LEFT)) {
            const auto clicked_pos = this->get_mouse_hex_pos();

            if (this->selected_ring) {
                const bool clicked_on_possible_move_node =
                    std::find(
                        this->ring_moves.begin(),
                        this->ring_moves.end(),
                        clicked_pos) != this->ring_moves.end();

                if (clicked_on_possible_move_node) {
                    auto selected_ring_copy = this->selected_ring;
                    this->selected_ring = std::nullopt;

                    return Yngine::RingMove{
                        Yngine::Bitboard::coords_to_index(
                            (*selected_ring_copy).x,
                            (*selected_ring_copy).y
                        ),
                        Yngine::Bitboard::coords_to_index(
                            clicked_pos.x,
                            clicked_pos.y
                        ),
                        HVec3{*selected_ring_copy}.direction_to(clicked_pos)
                    };
                }
            }

            const auto is_whites_move = this->board_state.is_whites_move();
            const auto expected_ring = is_whites_move ? Node::WhiteRing : Node::BlackRing;

            if (clicked_pos == this->selected_ring) {
                this->selected_ring = std::nullopt;
            } else if (this->board_state.is_in_game(clicked_pos) &&
                this->board_state.get_at(clicked_pos) == expected_ring) {
                this->selected_ring = clicked_pos;

                // Fill possible moves when we select the ring
                this->ring_moves = this->board_state.get_ring_moves(clicked_pos);
            } else {
                this->selected_ring = std::nullopt;
            }
        }
    } break;
    case BoardState::NextAction::RowRemoval: {
        if (raylib::Mouse::IsButtonPressed(MOUSE_BUTTON_LEFT)) {
            const auto selected_node = this->get_mouse_hex_pos();

            if (this->board_state.is_in_game(selected_node)) {
                this->row_remove_from = selected_node;
            }
        }

        if (this->row_remove_from) {
            const auto from = *this->row_remove_from;
            const auto hovered = this->get_mouse_hex_pos();

            const auto diff = HVec3{hovered - from};
            const auto straight_diff = diff.closest_straight_line();

            const auto row_remove_to = from + straight_diff;

            if (this->board_state.is_in_game(row_remove_to)) {
                this->row_remove_to = row_remove_to;

                if (raylib::Mouse::IsButtonReleased(MOUSE_BUTTON_LEFT)) {
                    this->row_remove_from = std::nullopt;

                    const auto diff = HVec3{row_remove_to - from};

                    if (diff.length() == 4) {
                        return Yngine::RemoveRowMove{
                            Yngine::Bitboard::coords_to_index(from.x, from.y),
                            HVec3{from}.direction_to(row_remove_to)
                        };
                    }
                }
            } else {
                if (raylib::Mouse::IsButtonReleased(MOUSE_BUTTON_LEFT)) {
                    this->row_remove_from = std::nullopt;
                }
            }
        }
    } break;
    case BoardState::NextAction::RingRemoval: {
        if (raylib::Mouse::IsButtonReleased(MOUSE_BUTTON_LEFT)) {
            const auto clicked_pos = this->get_mouse_hex_pos();

            if (this->board_state.is_in_game(clicked_pos)) {
                return Yngine::RemoveRingMove{
                    Yngine::Bitboard::coords_to_index(clicked_pos.x, clicked_pos.y)
                };
            }
        }
    } break;
    case BoardState::NextAction::GameOver: {
        assert(false);
    } break;
    }

    return std::nullopt;
}

void Game::render() {
    if (this->window.IsResized()) {
        this->update_camera();
    }

    BeginDrawing();
    this->window.ClearBackground(raylib::Color(0xB7B3AFFF));

    const auto window_size = window.GetSize();

    switch (this->state) {
    case State::ChoosingMode: {
        static bool pvp_blitz_checked = false;

        if (GuiButton(
            Rectangle{window_size.x / 2 - 100, window_size.y / 2 - 70, 200, 40},
            "Player vs Player"
        )) {
            this->white_is_ai = false;
            this->black_is_ai = false;
            this->board_state.set_blitz_mode(pvp_blitz_checked);
            this->state = Game::State::Playing;
        }

        GuiCheckBox(
            Rectangle{window_size.x / 2 - 100, window_size.y / 2 - 20, 20, 20},
            "Blitz mode (1 row to win)",
            &pvp_blitz_checked
        );

        if (GuiButton(
            Rectangle{window_size.x / 2 - 100, window_size.y / 2 + 10, 200, 40},
            "Player vs AI"
        )) {
            this->state = Game::State::ChoosingAISettings;
        }

        static bool  show_load_input = false;
        static char  load_path[512]  = "";
        static bool  load_error      = false;

        if (!show_load_input) {
            if (GuiButton(
                Rectangle{window_size.x / 2 - 100, window_size.y / 2 + 60, 200, 30},
                "Load Game"
            )) {
                show_load_input = true;
                load_error = false;
                load_path[0] = '\0';
            }
        } else {
            const float box_y = window_size.y / 2 + 60;
            GuiTextBox(
                Rectangle{window_size.x / 2 - 150, box_y, 260, 30},
                load_path, sizeof(load_path), true
            );
            if (GuiButton(Rectangle{window_size.x / 2 + 120, box_y, 60, 30}, "Load")) {
                if (this->load_game(std::string(load_path))) {
                    show_load_input = false;
                    load_error = false;
                } else {
                    load_error = true;
                }
            }
            if (GuiButton(Rectangle{window_size.x / 2 + 190, box_y, 60, 30}, "Cancel")) {
                show_load_input = false;
                load_error = false;
            }
            if (load_error) {
                DrawText("Failed to load file.", static_cast<int>(window_size.x / 2 - 100),
                         static_cast<int>(box_y + 36), 16, RED);
            }
        }
    } break;
    case State::ChoosingAISettings: {
        static int color_selected = 0;
        GuiToggleGroup(
            Rectangle{window_size.x / 2 - 101, window_size.y / 2 - 60, 100, 30},
            "White;Black",
            &color_selected
        );

        static float move_time = 1;
        GuiSlider(
            Rectangle{window_size.x / 2, window_size.y / 2 - 20, 100, 30},
            "Move time",
            TextFormat("%.1fs", move_time),
            &move_time,
            1.f, 30.f
        );

        static std::size_t memory_limit_mb = 0;

        const int total_system_memory_mb = this->total_system_memory / 1024 / 1024;
        if (memory_limit_mb == 0) {
            if (this->total_system_memory >= 2 * 1024) {
                memory_limit_mb = 2048;
            } else {
                memory_limit_mb = memory_limit_mb;
            }
        }

        float memory_limit_mb_float = memory_limit_mb;
        GuiSlider(
            Rectangle{window_size.x / 2, window_size.y / 2 + 20, 100, 30},
            "Memory limit",
            TextFormat("%i MB", memory_limit_mb),
            &memory_limit_mb_float,
            1.f, total_system_memory_mb
        );
        memory_limit_mb = static_cast<std::size_t>(memory_limit_mb_float);

        static std::size_t thread_count = this->system_max_threads;
        float thread_count_float = thread_count;
        GuiSlider(
            Rectangle{window_size.x / 2, window_size.y / 2 + 60, 100, 30},
            "Threads",
            TextFormat("%i", thread_count),
            &thread_count_float,
            1.f, this->system_max_threads
        );
        thread_count = static_cast<std::size_t>(thread_count_float);
        this->engine_thread_count = thread_count;

        static bool place_ai_rings_checked = false;
        GuiCheckBox(
            Rectangle{window_size.x / 2 - 100, window_size.y / 2 + 100, 20, 20},
            "Place AI rings manually",
            &place_ai_rings_checked
        );

        static bool blitz_checked = false;
        GuiCheckBox(
            Rectangle{window_size.x / 2 - 100, window_size.y / 2 + 130, 20, 20},
            "Blitz mode (1 row to win)",
            &blitz_checked
        );

        if (GuiButton(
            Rectangle{window_size.x / 2 - 100, window_size.y / 2 + 170, 200, 30},
            "Play"
        )) {
            if (color_selected == 0) {
                this->white_is_ai = false;
                this->black_is_ai = true;
            } else {
                this->white_is_ai = true;
                this->black_is_ai = false;
            }

            this->ai_move_time = move_time;
            this->place_ai_rings = place_ai_rings_checked;

            this->board_state.set_blitz_mode(blitz_checked);
            this->engine.emplace(blitz_checked, memory_limit_mb * 1024 * 1024);

            this->state = Game::State::Playing;
        }

        static bool  show_load_input = false;
        static char  load_path[512]  = "";
        static bool  load_error      = false;

        if (!show_load_input) {
            if (GuiButton(
                Rectangle{window_size.x / 2 - 100, window_size.y / 2 + 210, 200, 30},
                "Load Game"
            )) {
                show_load_input = true;
                load_error = false;
                load_path[0] = '\0';
            }
        } else {
            const float box_y = window_size.y / 2 + 210;
            GuiTextBox(
                Rectangle{window_size.x / 2 - 150, box_y, 260, 30},
                load_path, sizeof(load_path), true
            );
            if (GuiButton(Rectangle{window_size.x / 2 + 120, box_y, 60, 30}, "Load")) {
                if (this->load_game(std::string(load_path))) {
                    show_load_input = false;
                    load_error = false;
                } else {
                    load_error = true;
                }
            }
            if (GuiButton(Rectangle{window_size.x / 2 + 190, box_y, 60, 30}, "Cancel")) {
                show_load_input = false;
                load_error = false;
            }
            if (load_error) {
                DrawText("Failed to load file.", static_cast<int>(window_size.x / 2 - 100),
                         static_cast<int>(box_y + 36), 16, RED);
            }
        }
    } break;
    case State::Playing: {
        this->camera.BeginMode();
        this->draw_board(this->board_state);
        this->camera.EndMode();
        this->draw_review_bar();
    } break;
    case State::Reviewing: {
        this->camera.BeginMode();
        this->draw_board(this->replay_board);
        this->camera.EndMode();
        this->draw_review_bar();
    } break;
    }

    EndDrawing();
}

void Game::draw_review_bar() {
    const std::size_t total = this->move_history.size();
    const bool at_live      = (this->review_cursor == total);
    const bool at_start     = (this->review_cursor == 0);

    const float margin  = 8.f;
    const float padding = 6.f;
    const float btn_w   = 36.f;
    const float btn_h   = 24.f;
    const float rl_w    = 100.f;
    const float label_w = 110.f;
    const int   font_sz = 16;

    // Four rows:
    // Row 1: |< < [Move N/M] > >|
    // Row 2 (centred): [Resume Live]
    // Row 3 (centred): [Save]
    // Row 4 (centred): [New Game]
    const float row_gap  = 4.f;
    const float save_w   = 60.f;
    const float ng_w     = 90.f;
    const float panel_w  = margin + btn_w + padding + btn_w + padding
                         + label_w + padding + btn_w + padding + btn_w
                         + margin;
    const float panel_h  = margin + btn_h + row_gap + btn_h + row_gap + btn_h + row_gap + btn_h + margin;
    const float panel_x  = margin;
    const float panel_y  = margin;

    DrawRectangleRounded(
        Rectangle{panel_x, panel_y, panel_w, panel_h},
        0.25f, 8, ColorAlpha(BLACK, 0.60f));

    // --- Row 1: nav buttons + centred label ---
    float bx      = panel_x + margin;
    const float by = panel_y + margin;

    // |<
    if (at_start) GuiSetState(STATE_DISABLED);
    if (GuiButton(Rectangle{bx, by, btn_w, btn_h}, "|<") && !at_start) {
        this->review_cursor = 0;
        this->rebuild_replay_board();
        this->state = State::Reviewing;
    }
    GuiSetState(STATE_NORMAL);
    bx += btn_w + padding;

    // <
    if (at_start) GuiSetState(STATE_DISABLED);
    if (GuiButton(Rectangle{bx, by, btn_w, btn_h}, "<") && !at_start) {
        this->review_cursor--;
        this->rebuild_replay_board();
        this->state = State::Reviewing;
    }
    GuiSetState(STATE_NORMAL);
    bx += btn_w + padding;

    // Label — centred within label_w slot
    const char* counter = TextFormat("Move %zu / %zu", this->review_cursor, total);
    const int text_px   = MeasureText(counter, font_sz);
    DrawText(counter,
             static_cast<int>(bx + (label_w - text_px) / 2.f),
             static_cast<int>(by + (btn_h - font_sz) / 2.f),
             font_sz, WHITE);
    bx += label_w + padding;

    // >
    if (at_live) GuiSetState(STATE_DISABLED);
    if (GuiButton(Rectangle{bx, by, btn_w, btn_h}, ">") && !at_live) {
        this->review_cursor++;
        this->rebuild_replay_board();
        if (this->review_cursor == total)
            this->state = State::Playing;
        else
            this->state = State::Reviewing;
    }
    GuiSetState(STATE_NORMAL);
    bx += btn_w + padding;

    // >|
    if (at_live) GuiSetState(STATE_DISABLED);
    if (GuiButton(Rectangle{bx, by, btn_w, btn_h}, ">|") && !at_live) {
        this->review_cursor = total;
        this->rebuild_replay_board();
        this->state = State::Playing;
    }
    GuiSetState(STATE_NORMAL);

    // --- Row 2: Resume Live, centred in panel ---
    const float rl_x = panel_x + (panel_w - rl_w) / 2.f;
    const float rl_y = by + btn_h + row_gap;
    if (at_live) GuiSetState(STATE_DISABLED);
    if (GuiButton(Rectangle{rl_x, rl_y, rl_w, btn_h}, "Resume Live") && !at_live) {
        this->review_cursor = total;
        this->rebuild_replay_board();
        this->state = State::Playing;
    }
    GuiSetState(STATE_NORMAL);

    // --- Row 3: Save, centred in panel ---
    const float save_x = panel_x + (panel_w - save_w) / 2.f;
    const float save_y = rl_y + btn_h + row_gap;
    if (this->move_history.empty()) GuiSetState(STATE_DISABLED);
    if (GuiButton(Rectangle{save_x, save_y, save_w, btn_h}, "Save") && !this->move_history.empty()) {
        const auto time_str = get_local_time_string();
        std::string file_name = std::format(
            "{}.txt",
            time_str
        );
        this->save_game(file_name);
    }
    GuiSetState(STATE_NORMAL);

    // --- Row 4: New Game, centred in panel ---
    const float ng_x = panel_x + (panel_w - ng_w) / 2.f;
    const float ng_y = save_y + btn_h + row_gap;
    if (GuiButton(Rectangle{ng_x, ng_y, ng_w, btn_h}, "New Game")) {
        this->reset_game();
    }
}

void Game::draw_board(const BoardState& board) {
    const float line_thickness = 0.04f;
    const auto line_color = raylib::Color(0x383838FF);
    const auto label_color = raylib::Color::White();

    // Font size in world units: target ~18px on screen regardless of zoom
    const float label_px = 18.f;
    const float label_size = label_px / this->camera.zoom;
    const float label_spacing = 0.f;
    const auto font = GetFontDefault();

    // Draw lines
    for (int32_t x = 0; x < 11; x++) {
        const auto start_world = to_vector2(HVec2{x, BOARD_START_OFFSET[x]}.to_world());
        const auto end_world = to_vector2(HVec2{x, BOARD_END_OFFSET[x]}.to_world());
        start_world.DrawLine(end_world, line_thickness, line_color);
    }

    for (int32_t y = 0; y < 11; y++) {
        const auto start_world = to_vector2(HVec2{BOARD_START_OFFSET[y], y}.to_world());
        const auto end_world = to_vector2(HVec2{BOARD_END_OFFSET[y], y}.to_world());
        start_world.DrawLine(end_world, line_thickness, line_color);
    }

    for (int32_t y = -5; y <= 5; y++) {
        const auto index = y + 5;
        const auto start = HVec2{10, y} + HVec2::up() * BOARD_START_OFFSET[index];
        const auto end = start + HVec2::up() * (BOARD_END_OFFSET[index] - BOARD_START_OFFSET[index]);

        to_vector2(start.to_world()).DrawLine(to_vector2(end.to_world()), line_thickness, line_color);
    }

    // Draw column letters A-K below the bottom node of each diagonal (x+y = const)
    // The diagonal with index i has letter chr('A' + i), i = 0..10
    // Its bottom node (largest world_y) is: start = HVec2{10,y} + up*BOARD_START_OFFSET[i]
    //   where y = i - 5
    for (int32_t i = 0; i < 11; i++) {
        const int32_t y = i - 5;
        const auto bottom_node = HVec2{10, y} + HVec2::up() * BOARD_START_OFFSET[i];
        const auto world_pos = to_vector2(bottom_node.to_world());

        const char letter[2] = { static_cast<char>('A' + i), '\0' };
        const auto text_size = MeasureTextEx(font, letter, label_size, label_spacing);

        // Place centred horizontally, just below the bottom node
        const auto draw_pos = Vector2{
            world_pos.x - text_size.x / 2.f,
            world_pos.y + 0.15f
        };
        DrawTextEx(font, letter, draw_pos, label_size, label_spacing, label_color);
    }

    // Draw row numbers 1-11 to the left of the leftmost node of each y=const row
    for (int32_t row_y = 0; row_y < 11; row_y++) {
        const auto left_node = HVec2{BOARD_START_OFFSET[row_y], row_y};
        const auto world_pos = to_vector2(left_node.to_world());

        const char* num_str = TextFormat("%i", row_y + 1);
        const auto text_size = MeasureTextEx(font, num_str, label_size, label_spacing);

        // Place centred vertically, just to the left of the leftmost node
        const auto draw_pos = Vector2{
            world_pos.x - text_size.x - 0.15f,
            world_pos.y - text_size.y / 2.f
        };
        DrawTextEx(font, num_str, draw_pos, label_size, label_spacing, label_color);
    }

    // Draw possible moves if a ring is selected
    if (this->selected_ring) {
        for (const auto move_pos : this->ring_moves) {
            DrawRing(
                to_vector2(move_pos.to_world()),
                0.3f, 0.43f, 0.f, 360.f, 40,
                raylib::Color::DarkGreen().Fade(0.5)
            );
        }
    }

    // Draw selected row for removal if needed
    if (this->row_remove_from) {
        if (*this->row_remove_from != this->row_remove_to) {
            const auto straight_diff = HVec3{this->row_remove_to - *this->row_remove_from};
            const auto dir = straight_diff / straight_diff.length();

            auto current = *this->row_remove_from;
            while (current != (this->row_remove_to + dir)) {
                to_vector2(current.to_world()).DrawCircle(0.4, raylib::Color::Red());

                current += dir;
            }
        } else {
            to_vector2(this->row_remove_from->to_world()).DrawCircle(0.4, raylib::Color::Red());
        }
    }

    // Draw contents of the nodes
    for (int32_t x = 0; x < 11; x++) {
        for (int32_t y = 0; y < 11; y++) {
            const auto pos = HVec2{x, y};

            if (board.is_in_game(pos)) {
                const auto pos_world = to_vector2(pos.to_world());
                const auto piece = board.get_at(pos);

                switch (piece) {
                case Node::WhiteRing: [[fallthrough]];
                case Node::BlackRing: {
                    if (this->selected_ring == pos) {
                        // Draw selection outline
                        // DrawRing(center, 0.27f, 0.46f, 0.f, 360.f, 40, raylib::Color::Red());
                        // Draw red marker inside the selection
                        pos_world.DrawCircle(0.24f, raylib::Color::Red().Fade(0.8));
                    }

                    if (piece == Node::WhiteRing) {
                        DrawRing(pos_world, 0.3f, 0.43f, 0.f, 360.f, 40, raylib::Color::White());
                        // DrawRing(pos_world, 0.33f, 0.4f, 0.f, 360.f, 40, raylib::Color::White());
                    } else {
                        DrawRing(pos_world, 0.3f, 0.43f, 0.f, 360.f, 40, raylib::Color::Black());
                    }
                } break;

                case Node::WhiteMarker: [[fallthrough]];
                case Node::BlackMarker: {
                    if (piece == Node::WhiteMarker) {
                        pos_world.DrawCircle(0.27f, raylib::Color::White());
                    } else {
                        pos_world.DrawCircle(0.27f, raylib::Color::Black());
                    }
                } break;

                default: {}
                }
            }
        }
    }
}

void Game::update_camera() {
    this->camera.offset = window.GetSize() / 2;

    const auto window_size = window.GetSize();

    if (window_size.x < window_size.y) {
        this->camera.zoom = (window_size.x / 10.f);
    } else {
        this->camera.zoom = (window_size.y / 10.f);
    }
}

HVec2 Game::get_mouse_hex_pos() {
    const auto mouse_pos = raylib::Mouse::GetPosition();
    const auto world_pos = this->camera.GetScreenToWorld(mouse_pos);
    const auto hex_pos = from_vector2(world_pos).from_world();

    return hex_pos;
}
