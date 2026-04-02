#include "render.hpp"

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

namespace {

constexpr int kInnerWidth = 37;

std::string bar(double value, int width = 10) {
    int filled = static_cast<int>((value / 100) * width);
    if (filled < 0) filled = 0;
    if (filled > width) filled = width;

    std::string out = "[";
    out += std::string(filled, '#');
    out += std::string(width - filled, '-');
    out += "]";
    return out;
}

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

void draw(const Buddy& buddy,const std::string& input_buffer) {
    clear_screen();

    const auto& s = buddy.stats();
    const auto frame = buddy.current_frame();
    const std::string border = "+" + std::string(kInnerWidth + 2, '-') + "+";

    std::cout << border << '\n';
    framed_line("Buddy: " + buddy.name());
    framed_line("Mood: " + buddy.mood_text());
    /* framed_line("X: " + std::to_string(buddy.x_position())); */
    framed_line("Hunger:    " + bar(s.hunger));
    framed_line("Energy:    " + bar(s.energy));
    framed_line("Happiness: " + bar(s.happiness));
    framed_line();

    for (const auto& row : frame) {
        const std::string padded_row = std::string(static_cast<std::size_t>(buddy.x_position()), ' ') + row; 
        framed_line(padded_row);
    }

    framed_line();
    framed_line("Commands: feed pet sleep wake quit");
    framed_line("Input: " + input_buffer);
    std::cout << border << '\n';

    std::cout.flush();
}

} // namespace render 
