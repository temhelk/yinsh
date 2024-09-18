#include <yinsh-gui/game.hpp>
#include <yinsh-gui/coords.hpp>
#include <yinsh-gui/board.hpp>
#include <yinsh-gui/utils.hpp>
#include <yinsh-gui/system.hpp>

#include <raylib-cpp.hpp>
#define RAYGUI_IMPLEMENTATION
#include <raygui.h>

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

#include <cassert>
#include <algorithm>

Game::Game()
    : window{}
    , camera{}
    , state{Game::State::ChoosingMode}
    , white_is_ai{false}
    , black_is_ai{false}
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

    game->update();
    game->render();
}

void Game::run() {
    const auto initial_window_size = raylib::Vector2{1280, 720};

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

#if defined(PLATFORM_WEB)
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
}

void Game::update() {
    switch (this->state) {
    case State::ChoosingMode:
    case State::ChoosingAISettings: {
    } break;
    case State::Playing: {
        if (this->board_state.get_next_action() == BoardState::NextAction::GameOver)
            return;

        if ( this->board_state.is_whites_move() && this->white_is_ai ||
            !this->board_state.is_whites_move() && this->black_is_ai) {
            assert(this->engine);

            if (!this->engine_move) {
                this->engine_move = this->engine->search(this->ai_move_time, this->engine_thread_count);
            } else {
                const auto move_status = this->engine_move->wait_for(std::chrono::seconds(0));

                if (move_status == std::future_status::ready) {
                    const auto move = this->engine_move->get();

                    this->board_state.apply_move(move);
                    engine->apply_move(move);

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
                }
            }
        }
    } break;
    }
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
                    this->selected_ring = std::nullopt;

                    return Yngine::RingMove{
                        Yngine::Bitboard::coords_to_index(
                            (*this->selected_ring).x,
                            (*this->selected_ring).y
                        ),
                        Yngine::Bitboard::coords_to_index(
                            clicked_pos.x,
                            clicked_pos.y
                        ),
                        HVec3{*this->selected_ring}.direction_to(clicked_pos)
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
        if (GuiButton(
            Rectangle{window_size.x / 2 - 100, window_size.y / 2 - 50, 200, 40},
            "Player vs Player"
        )) {
            this->white_is_ai = false;
            this->black_is_ai = false;
            this->state = Game::State::Playing;
        }

        if (GuiButton(
            Rectangle{window_size.x / 2 - 100, window_size.y / 2 + 10, 200, 40},
            "Player vs AI"
        )) {
            this->state = Game::State::ChoosingAISettings;
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

        if (GuiButton(
            Rectangle{window_size.x / 2 - 100, window_size.y / 2 + 100, 200, 30},
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

            this->engine.emplace(memory_limit_mb * 1024 * 1024);

            this->state = Game::State::Playing;
        }
    } break;
    case State::Playing: {
        this->camera.BeginMode();
        this->draw_board();
        this->camera.EndMode();
    } break;
    }

    EndDrawing();
}

void Game::draw_board() {
    const float line_thickness = 0.04f;
    const auto line_color = raylib::Color(0x383838FF);

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

            if (this->board_state.is_in_game(pos)) {
                const auto pos_world = to_vector2(pos.to_world());
                const auto piece = this->board_state.get_at(pos);

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
