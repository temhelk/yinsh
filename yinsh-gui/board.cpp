#include <yinsh-gui/board.hpp>

#include <cassert>

BoardStorage::BoardStorage() : nodes{} {
    for (int32_t x = 0; x < 11; x++) {
        for (int32_t y = BOARD_START_OFFSET[x]; y <= BOARD_END_OFFSET[x]; y++) {
            this->at(x, y) = Node::Empty;
        }
    }
}

Node& BoardStorage::at(int32_t x, int32_t y) {
    assert(x >= 0 && y >= 0 && x < 11 && y < 11);

    return this->nodes[11 * y + x];
}

Node& BoardStorage::at(HVec2 pos) {
    assert(pos.x >= 0 && pos.y >= 0 && pos.x < 11 && pos.y < 11);

    return this->nodes[11 * pos.y + pos.x];
}

Node BoardStorage::get_at(int32_t x, int32_t y) const {
    assert(x >= 0 && y >= 0 && x < 11 && y < 11);

    return this->nodes[11 * y + x];
}

Node BoardStorage::get_at(HVec2 pos) const {
    assert(pos.x >= 0 && pos.y >= 0 && pos.x < 11 && pos.y < 11);

    return this->nodes[11 * pos.y + pos.x];
}

bool BoardState::is_in_game(HVec2 pos) const {
    if (pos.x < 0 || pos.y < 0 || pos.x >= 11 || pos.y >= 11) {
        return false;
    }

    return this->storage.get_at(pos) != Node::NotInGame;
}

Node BoardState::get_at(HVec2 pos) const {
    return this->storage.get_at(pos);
}

void BoardState::place_ring(HVec2 pos) {
    assert(this->white_rings_on_board + this->black_rings_on_board < 10);
    assert(this->storage.at(pos) == Node::Empty);

    this->storage.at(pos) = this->white_moves_next ? Node::WhiteRing : Node::BlackRing;

    if (this->white_moves_next)
        this->white_rings_on_board++;
    else
        this->black_rings_on_board++;

    if (this->black_rings_on_board == 5) {
        this->next_action = NextAction::RingMovement;
    }

    this->white_moves_next = !this->white_moves_next;
}

void BoardState::move_ring(HVec2 from, HVec2 to) {
    assert(this->get_at(from) == Node::WhiteRing || this->get_at(from) == Node::BlackRing);
    assert(this->get_at(to) == Node::Empty);

    const auto mover_marker = this->white_moves_next ? Node::WhiteMarker : Node::BlackMarker;

    assert(
        mover_marker ==
        (this->storage.get_at(from) == Node::WhiteRing ? Node::WhiteMarker : Node::BlackMarker)
    );

    this->storage.at(to) = this->get_at(from);
    this->storage.at(from) = mover_marker;

    auto dir = HVec3{to - from};
    dir /= dir.length();

    auto current_node = from + dir;
    while (current_node != to) {
        const auto current_piece = this->get_at(current_node);

        if (current_piece != Node::Empty) {
            assert(current_piece == Node::WhiteMarker || current_piece == Node::BlackMarker);

            this->storage.at(current_node) =
                current_piece == Node::WhiteMarker ? Node::BlackMarker : Node::WhiteMarker;
        }

        current_node += dir;
    }

    this->last_move_from = from;
    this->last_move_to = to;

    // After we moved the ring we have to check if we created any rows
    this->white_made_last_movement = this->white_moves_next;
    this->check_for_rows_and_change_state(from, to);

    // End the game if all the markers are used and no rows can be removed
    // depends on the check_for_rows_and_change_state to find out whether
    // any rows can be removed
    if (this->next_action != NextAction::RowRemoval &&
        this->number_of_markers_on_the_board() == 51) {
        this->next_action = NextAction::GameOver;
    }
}

BoardState::NextAction BoardState::get_next_action() const {
    return this->next_action;
}

bool BoardState::is_whites_move() const {
    return this->white_moves_next;
}

std::vector<HVec2> BoardState::get_ring_moves(HVec2 pos) const {
    const auto expected_ring_color = this->white_moves_next ?
        Node::WhiteRing : Node::BlackRing;

    assert(this->is_in_game(pos) && this->get_at(pos) == expected_ring_color);

    std::vector<HVec2> result{};

    for (const auto dir : HVec2::Directions) {
        auto path_pos = pos;
        bool went_over_markers = false;

        while (true) {
            path_pos += dir;

            if (!this->is_in_game(path_pos))
                break;

            const auto piece = this->get_at(path_pos);

            if (piece == Node::WhiteRing || piece == Node::BlackRing)
                break;

            if (piece == Node::WhiteMarker || piece == Node::BlackMarker) {
                went_over_markers = true;
                continue;
            }

            result.push_back(path_pos);

            if (went_over_markers)
                break;
        }
    }

    return result;
}

void BoardState::remove_row(HVec2 from, HVec2 to) {
    const auto diff = HVec3{to - from};
    const auto dir = diff / diff.length();

    assert(diff.length() == 4);

    auto current = from;
    while (current != (to + dir)) {
        assert(
            this->storage.get_at(current) == Node::WhiteMarker ||
            this->storage.get_at(current) == Node::BlackMarker
        );
        this->storage.at(current) = Node::Empty;

        current += dir;
    }

    this->next_action = NextAction::RingRemoval;
}

void BoardState::check_for_rows_and_change_state(HVec2 from, HVec2 to) {
    const auto mover_marker = this->white_moves_next ? Node::WhiteMarker : Node::BlackMarker;

    auto dir = HVec3{to - from};
    dir /= dir.length();

    // The row was formed of the same color as the player who originally moved
    bool found_row_of_the_mover = false;
    bool found_rows = false;

    auto node_along_move_line = from;
    while (node_along_move_line != to) {
        if (this->get_at(node_along_move_line) == Node::Empty) {
            node_along_move_line += dir;
            continue;
        }

        const HVec2 dirs[3] = {HVec2{1, 0}, HVec2{0, 1}, HVec2{1, -1}};
        for (const auto dir : dirs) {
            int line_length = 1;

            const auto marker_of_line = this->get_at(node_along_move_line);

            auto node_along_dir = node_along_move_line + dir;
            while (this->is_in_game(node_along_dir) &&
                this->get_at(node_along_dir) == marker_of_line) {
                line_length++;
                node_along_dir += dir;
            }

            const auto anti_dir = -dir;
            auto node_along_anti_dir = node_along_move_line + anti_dir;
            while (this->is_in_game(node_along_anti_dir) &&
                this->get_at(node_along_anti_dir) == marker_of_line) {
                line_length++;
                node_along_anti_dir += anti_dir;
            }

            if (line_length >= 5) {
                found_rows = true;

                if (marker_of_line == mover_marker) {
                    found_row_of_the_mover = true;
                }
            }
        }

        node_along_move_line += dir;
    }

    if (found_rows) {
        this->next_action = NextAction::RowRemoval;

        if (!found_row_of_the_mover) {
            this->white_moves_next = !this->white_moves_next;
        }
    }
    else {
        this->next_action = NextAction::RingMovement;
        this->white_moves_next = !this->white_made_last_movement;
    }
}

int BoardState::number_of_markers_on_the_board() const {
    int result = 0;

    for (int y = 0; y < 11; y++) {
        for (int x = 0; x < 11; x++) {
            if (this->get_at(HVec2{x, y}) == Node::WhiteMarker ||
                this->get_at(HVec2{x, y}) == Node::BlackMarker) {
                result++;
            }
        }
    }

    return result;
}

void BoardState::remove_ring(HVec2 pos) {
    this->storage.at(pos) = Node::Empty;

    if (this->white_moves_next) {
        this->white_rings_on_board--;
    } else {
        this->black_rings_on_board--;
    }

    if (this->white_rings_on_board == 2 ||
        this->black_rings_on_board == 2) {
        this->next_action = NextAction::GameOver;
    } else {
        check_for_rows_and_change_state(this->last_move_from, this->last_move_to);
    }
}
