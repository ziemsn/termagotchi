#include "render.hpp"
#include "buddy.hpp"

#include <array>
#include <chrono>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

constexpr int kInnerWidth = 37;
constexpr int kStageHeight = 14;

namespace ansi {
constexpr const char* reset   = "\033[0m";
constexpr const char* dim     = "\033[2m";
constexpr const char* yellow  = "\033[33m";
constexpr const char* red     = "\033[31m";
constexpr const char* green   = "\033[32m";
constexpr const char* cyan    = "\033[36m";
constexpr const char* magenta = "\033[35m";
constexpr const char* blue    = "\033[34m";
constexpr const char* bright_white = "\033[97m";
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

struct SkyProp {
    int row = 0;
    int col = 0;
    char glyph = ' ';
    SpriteLayerRole role = SpriteLayerRole::None;
};

struct StarPoint {
    int row = 0;
    int col = 0;
    char glyph = '.';
};

struct CloudSprite {
    int row = 0;
    int col = 0;
    std::vector<std::string> rows;
};

struct SkySnapshot {
    bool daytime = true;
    SkyProp prop;
    std::vector<StarPoint> stars;
    std::vector<CloudSprite> clouds;
};

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
    if (cell.role == SpriteLayerRole::Prop) {
        if (cell.glyph == 'O') return ansi::yellow;
        if (cell.glyph == 'C') return ansi::cyan;
        if (cell.glyph == '*' || cell.glyph == '+' || cell.glyph == '.') {
            return ansi::bright_white;
        }
        return ansi::reset;
    }
    if (cell.role == SpriteLayerRole::Effect) {
        if (buddy.appearance.activity == Activity::Sleeping) {
            return ansi::cyan;
        }
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

void put_stage_cell(std::vector<StageRow>& stage, int row, int col, char glyph, SpriteLayerRole role) {
    if (glyph == ' ') return;

    if (row < 0 || row >= static_cast<int>(stage.size())) return;

    if (col < 0 || col >= static_cast<int>(stage[static_cast<std::size_t>(row)].size())) return;

    stage[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)] = SpriteCell{glyph, role};
}

void overlay_stage_text(std::vector<StageRow>& stage, int row, int col, const std::string& text, SpriteLayerRole role) {
    for (std::size_t i = 0; i < text.size(); ++i) {
        put_stage_cell(stage, row, col + static_cast<int>(i), text[i], role);
    }
}

int cloud_width(const CloudSprite& cloud) {
    int width = 0;
    for (const auto& row : cloud.rows){
        const int row_width = static_cast<int>(row.size());
        if (row_width > width) {
            width = row_width;
        }
    }
    return width;
}

std::tm current_local_tm() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    const std::tm* local_tm = std::localtime(&now_time);

    if (local_tm == nullptr) {
        return std::tm{};
    }

    return *local_tm;
}

double current_local_hour(const std::tm& local_tm) {
    return static_cast<double>(local_tm.tm_hour) + 
        static_cast<double>(local_tm.tm_min) / 60.0 +
        static_cast<double>(local_tm.tm_sec) / 3600.0;
}

double seconds_since_midnight(const std::tm& local_tm) {
    return static_cast<double>(local_tm.tm_hour) * 3600.0 +
        static_cast<double>(local_tm.tm_min) * 60.0 + 
        static_cast<double>(local_tm.tm_sec);
}

bool is_daytime(double hour) {
    return hour >= 6.0 && hour < 18.0;
}

double sky_progress_for_hour(double hour) {
    if (is_daytime(hour)) {
        return (hour - 6.0) / 12.0;
    }

    if (hour >= 18.0) {
        return (hour - 18.0) / 12.0;
    }

    return (hour + 6.0) / 12.0;
}

SkyProp current_sky_prop(double hour) {
    const bool daytime = is_daytime(hour);
    const double progress = sky_progress_for_hour(hour);
    const double centered_progress = 2.0 * progress - 1.0;
    const double arc_height = 1.0 - centered_progress * centered_progress;

    constexpr int kSkyMinCol = 1;
    constexpr int kSkyMaxCol = kInnerWidth - 2;
    constexpr int kHorizonRow = 4;
    constexpr int kZenithRow = 0;

    SkyProp prop;
    prop.col = kSkyMinCol + static_cast<int>(std::round(progress * (kSkyMaxCol - kSkyMinCol)));
    prop.row = kHorizonRow - static_cast<int>(std::round(arc_height * (kHorizonRow - kZenithRow)));
    prop.glyph = daytime ? 'O' : 'C';
    prop.role = SpriteLayerRole::Prop;
    return prop;
}

std::vector<StarPoint> current_stars() {
    return {
        {0,  2, '*'},
        {0, 10, '.'},
        {0, 18, '*'},
        {0, 31, '.'},
        {1,  6, '.'},
        {1, 14, '*'},
        {1, 24, '.'},
        {1, 34, '*'},
        {2,  4, '.'},
        {2, 12, '*'},
        {2, 21, '.'},
        {2, 29, '*'},
        {3,  8, '.'},
        {3, 17, '*'},
        {3, 27, '.'}
    };
}

CloudSprite make_cloud(int row, double seconds, double speed_chars_per_second, double phase_offset, std::vector<std::string> rows) {
    CloudSprite cloud;
    cloud.row = row;
    cloud.rows = std::move(rows);

    const int width = cloud_width(cloud);
    const int travel_width = kInnerWidth + width + 10;
    const double wrapped = std::fmod( seconds * speed_chars_per_second + phase_offset  * static_cast<double>(travel_width), static_cast<double>(travel_width));

    cloud.col = static_cast<int>(std::floor(wrapped)) - width;
    return cloud;
}

std::vector<CloudSprite> current_clouds(double seconds) { 
    std::vector<CloudSprite> clouds;
    clouds.push_back(make_cloud(0, seconds, 0.28, 0.00, {" _--_ ", "(___ )"}));
    clouds.push_back(make_cloud(1, seconds, 0.18, 0.38, {"  _--_  ", " (____) "}));
    clouds.push_back(make_cloud(0, seconds, 0.23, 0.74, {" _-_ ", "(___)"}));
    return clouds;
}

SkySnapshot current_sky_snapshot() {
    const std::tm local_tm = current_local_tm();
    const double hour = current_local_hour(local_tm);
    const double seconds = seconds_since_midnight(local_tm);

    SkySnapshot sky;
    sky.daytime = is_daytime(hour);
    sky.prop = current_sky_prop(hour);

    if (sky.daytime) {
        sky.clouds = current_clouds(seconds);
    } else {
        sky.stars = current_stars();
    }

    return sky;
}

void overlay_sky_prop(std::vector<StageRow>& stage, const SkyProp& prop) {
    put_stage_cell(stage, prop.row, prop.col, prop.glyph, prop.role);
}

void overlay_stars(std::vector<StageRow>& stage, const std::vector<StarPoint>& stars) {
    for (const auto& star : stars) {
        put_stage_cell(stage, star.row, star.col, star.glyph, SpriteLayerRole::Prop);
    }
}

void overlay_clouds(std::vector<StageRow>& stage, const std::vector<CloudSprite>& clouds) {
    for (const auto& cloud : clouds) {
        for (std::size_t row_index = 0; row_index < cloud.rows.size(); ++row_index) {
            overlay_stage_text(stage, cloud.row + static_cast<int>(row_index), cloud.col, cloud.rows[row_index], SpriteLayerRole::Prop);
        }
    }
}

void overlay_sky_scene(std::vector<StageRow>& stage) {
    const SkySnapshot sky = current_sky_snapshot();

    if (sky.daytime) {
        overlay_sky_prop(stage, sky.prop);
        overlay_clouds(stage, sky.clouds);
    } else {
        overlay_stars(stage, sky.stars);
        overlay_sky_prop(stage, sky.prop);
    }
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

    overlay_sky_scene(stage);

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
