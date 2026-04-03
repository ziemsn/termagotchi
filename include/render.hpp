#pragma once

#include "buddy.hpp"

namespace render {

void clear_screen();
void hide_cursor();
void show_cursor();

void draw(const BuddyRenderState& buddy, const std::string& input_buffer);

} // namespace render
