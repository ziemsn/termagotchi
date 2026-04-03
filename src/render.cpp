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

void overlay_text(std::string& row, const std::string& text, int x_offset) {
    for (std::size_t i = 0; i < text.size(); ++i){
        const int col = x_offset + static_cast<int>(i);
        if (col < 0 || col >= static_cast<int>(row.size())) {
            continue;
        }
        row[static_cast<std::size_t>(col)] = text[i];
    }
}

std::vector<std::string> build_stage(const BuddyRenderState& buddy) {
    std::vector<std::string> stage(static_cast<std::size_t>(kStageHeight), 
            std::string(static_cast<std::size_t>(kInnerWidth), ' '));

    const int ground_row = kStageHeight - 1;
    stage[static_cast<std::size_t>(ground_row)] = std::string(static_cast<std::size_t>(kInnerWidth), '_');

    const int sprite_left = buddy.x_position;
    const int sprite_top = std::max(0, ground_row - static_cast<int>(buddy.frame.size()));

    for (std::size_t row_index = 0; row_index < buddy.frame.size(); ++row_index) {
        const int stage_row = sprite_top + static_cast<int>(row_index);
        if (stage_row < 0 || stage_row >= ground_row) {
            continue;
        }
        overlay_text(stage[static_cast<std::size_t>(stage_row)], buddy.frame[row_index], sprite_left);
    }

    return stage;
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
    if (buddy.appearance.effect == Effect::Sparkle) {
        return ansi::magenta;
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
        if (row.find_first_not_of('_') == std::string::npos) {
            framed_line_colored(row, ansi::green);
        } else {
            framed_line(row);
        }
    }

    framed_line_colored(divider, ansi::dim);
    framed_line_colored("Commands: feed pet sleep wake quit", ansi::dim);
    framed_line("Input: " + input_buffer);
    std::cout << border << '\n';

    std::cout.flush();
}

} // namespace render 
