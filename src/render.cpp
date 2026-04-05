#include "render.hpp"
#include "buddy.hpp"

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

namespace {

constexpr int kInnerWidth = 37;
constexpr int kStageHeight = 10;

namespace ansi {
constexpr const char* reset   = "\033[0m";
constexpr const char* dim     = "\033[2m";
constexpr const char* yellow  = "\033[33m";
constexpr const char* red     = "\033[31m";
constexpr const char* green   = "\033[32m";
constexpr const char* cyan    = "\033[36m";
constexpr const char* magenta = "\033[35m";
constexpr const char* blue    = "\033[34m";
} // namespace ansi

std::string fit_text(const std::string& text, int width = kInnerWidth) {
    if (static_cast<int>(text.size()) <= width) {
        return text + std::string(width - static_cast<int>(text.size()), ' ');
    }

    if (width <= 3) {
        return text.substr(0, static_cast<std::size_t>(width));
    }

    return text.substr(0, static_cast<std::size_t>(width - 3)) + "...";
}

void framed_line(const std::string& text = "") {
    std::cout << "| " << fit_text(text) << " |\n";
}

void framed_line_colored(const std::string& text, const char* color_code) {
    std::cout << "| " << color_code << fit_text(text) << ansi::reset << " |\n";
}

using StageRow = std::vector<SpriteCell>;

const char* state_accent_color(const BuddyRenderState& buddy) {
    if (buddy.appearance.effect == Effect::Sparkle) {
        return ansi::magenta;
    }
    if (buddy.appearance.body_pose == BodyPose::BreathingIn) {
        return ansi::blue;
    }
    if (buddy.appearance.body_pose == BodyPose::BreathingOut) {
        return ansi::cyan;
    }
    if (buddy.appearance.movement_phase == MovementPhase::WallSquishPause) {
        return ansi::magenta;
    }
    if (buddy.appearance.movement_phase == MovementPhase::TurningPause) {
        return ansi::cyan;
    }
    if (buddy.appearance.activity == Activity::Sleeping) {
        return ansi::cyan;
    }
    if (buddy.appearance.activity == Activity::Eating) {
        return ansi::green;
    }
    if (buddy.appearance.expression == Expression::Sad) {
        return ansi::red;
    }
    if (buddy.appearance.expression == Expression::Blinking) {
        return ansi::cyan;
    }
    if (buddy.appearance.activity == Activity::Walking) {
        return ansi::green;
    }
    return ansi::reset;
}

const char* sprite_cell_color(const BuddyRenderState& buddy, const SpriteCell& cell) {
    if (cell.glyph == ' ') {
        return ansi::reset;
    }
    if (cell.role == SpriteLayerRole::Effect) {
        return ansi::magenta;
    }
    if (cell.role == SpriteLayerRole::Blush) {
        return ansi::red;
    }
    if (cell.role == SpriteLayerRole::CapCrown || cell.role == SpriteLayerRole::CapBrim) {
        return state_accent_color(buddy);
    }
    if (cell.role == SpriteLayerRole::Eyes) {
        return ansi::reset;
    }
    if (cell.role == SpriteLayerRole::Body) {
        return ansi::yellow;
    }
    if (cell.role == SpriteLayerRole::None && cell.glyph == '_') {
        return ansi::green;
    }

    return ansi::yellow;
}

void overlay_sprite(StageRow& stage_row, const SpriteRow& sprite_row, int x_offset) {
    for (std::size_t i = 0; i < sprite_row.size(); ++i) {
        const int col = x_offset + static_cast<int>(i);
        if (col < 0 || col >= static_cast<int>(stage_row.size())) {
            continue;
        }
        const SpriteCell& sprite_cell = sprite_row[i];
        if (sprite_cell.glyph == ' ') {
            continue;
        }

        stage_row[static_cast<std::size_t>(col)] = sprite_cell;
    }
}

std::vector<StageRow> build_stage(const BuddyRenderState& buddy) {
    std::vector<StageRow> stage(
            static_cast<std::size_t>(kStageHeight),
            StageRow(static_cast<std::size_t>(kInnerWidth), SpriteCell{}));

    const int ground_row = kStageHeight - 1;
    for (auto& cell : stage[static_cast<std::size_t>(ground_row)]) {
        cell.glyph = '_';
        cell.role = SpriteLayerRole::None;
    }

    const int sprite_left = buddy.x_position;
    const int sprite_top = std::max(0, ground_row - static_cast<int>(buddy.sprite.rows.size()));

    for (std::size_t row_index = 0; row_index < buddy.sprite.rows.size(); ++row_index) {
        const int stage_row = sprite_top + static_cast<int>(row_index);
        if (stage_row < 0 || stage_row >= ground_row) {
            continue;
        }
        overlay_sprite(stage[static_cast<std::size_t>(stage_row)], buddy.sprite.rows[row_index], sprite_left);
    }

    return stage;
}

void framed_stage_row(const StageRow& row, const BuddyRenderState& buddy) {
    std::cout << "| ";

    const char* active_color = ansi::reset;
    std::cout << active_color;

    for (const auto& cell : row) {
        const char* next_color = sprite_cell_color(buddy, cell);
        if (next_color != active_color){
            std::cout << next_color;
            active_color = next_color;
        }
        std::cout << cell.glyph;
    }

    if (active_color != ansi::reset) {
        std::cout << ansi::reset;
    }

    std::cout << " |\n";
}

const char* stat_color_hunger(double hunger) {
    if (hunger >= 85.0) return ansi::red;
    if (hunger >= 70.0) return ansi::yellow;
    return ansi::reset;
}

const char* stat_color_energy(double energy) {
    if (energy <= 15.0) return ansi::red;
    if (energy <= 30.0) return ansi::yellow;
    return ansi::reset;
}

const char* stat_color_happiness(double happiness) {
    if (happiness < 20.0) return ansi::red;
    if (happiness < 40.0) return ansi::yellow;
    return ansi::reset;
}

const char* mood_color(const BuddyRenderState& buddy) {
    return state_accent_color(buddy);
}

} // namespace

namespace render {

void clear_screen() {
    std::cout << "\033[2J\033[H";
}

void hide_cursor() {
    std::cout << "\033[?25l";
}

void show_cursor() {
    std::cout << "\033[?25h";
}

void draw(const BuddyRenderState& buddy,const std::string& input_buffer) {
    clear_screen();

    const auto stage = build_stage(buddy);
    const std::string border = "+" + std::string(kInnerWidth + 2, '-') + "+";
    const std::string divider(kInnerWidth, '-');

    std::cout << border << '\n';
    framed_line(buddy.title_line);
    framed_line_colored(buddy.mood_line, mood_color(buddy));
    framed_line_colored(buddy.hunger_line, stat_color_hunger(buddy.stats.hunger));
    framed_line_colored(buddy.energy_line, stat_color_energy(buddy.stats.energy));
    framed_line_colored(buddy.happiness_line, stat_color_happiness(buddy.stats.happiness));
    framed_line(divider);

    for (const auto& row : stage) {
        framed_stage_row(row, buddy);
    }

    framed_line_colored(divider, ansi::dim);
    framed_line_colored("Commands: feed pet sleep wake quit", ansi::dim);
    framed_line("Input: " + input_buffer);
    std::cout << border << '\n';

    std::cout.flush();
}

} // namespace render 
