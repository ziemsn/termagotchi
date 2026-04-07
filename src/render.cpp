#include "render.hpp"
#include "buddy.hpp"

#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

constexpr int kInnerWidth = 70;
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

struct CloudTrack { 
    int row = 0;
    double speed_chars_per_seconds = 0.0;
    double phase_offset = 0.0;
    std::vector<std::string> rows;
};

struct SkySnapshot {
    bool daytime = true;
    SkyProp prop;
    std::vector<StarPoint> stars;
    std::vector<CloudSprite> clouds;
};

std::string cap_crown_display_text(char glyph) {
    switch (glyph) {
    case '-':
        return u8"─";
    case 'o':
        return u8"○";
    case 'O':
        return u8"●";
    default:
        return std::string(1, glyph);
    }
}

std::string cap_brim_display_text(char glyph) {
    switch (glyph) {
    case '_':
        return u8"─";
    default:
        return std::string(1, glyph);
    }
}

std::string body_display_text(char glyph) {
    switch (glyph) {
    case '|':
        return u8"│";
    case '_':
        return u8"─";
    default:
        return std::string(1, glyph);
    }
}

std::string blush_display_text(char glyph) {
    switch (glyph) {
    case '.':
        return u8"•";
    default:
        return std::string(1, glyph);
    }
}

std::string feet_display_text(char glyph) {
    switch (glyph) {
    case '/':
        return u8"╱";
    case '\\':
        return u8"╲";
    default:
        return std::string(1, glyph);
    }
}

std::string effect_display_text(char glyph) {
    switch (glyph) {
    case 'Z':
        return u8"ᴢ";
    case 'z':
        return u8"ᶻ";
    case '*':
        return u8"✦";
    case '+':
        return u8"✧";
    case '.':
        return u8"·";
    default:
        return std::string(1, glyph);
    }
}

std::uint32_t mix_bits(std::uint32_t value) {
    value ^= value >> 16;
    value *= 0x7feb352du;
    value ^= value >> 15;
    value *= 0x846ca68bu;
    value ^= value >> 16;
    return value;
}

double unit_random(std::uint32_t seed) {
    return static_cast<double>(mix_bits(seed) & 0x00ffffffu) / static_cast<double>(0x01000000u);
}

char sun_prop_glyph() {
    return 'O';
}

char moon_prop_glyph() {
    return 'C';
}

std::string eyes_display_text(char glyph) {
    switch(glyph) {
    case 'o':
        return u8"◉";
    case 'O':
        return u8"◍";
    case '-':
        return u8"─";
    case 'v':
        return u8"⌄";
    case 'L':
        return u8"◜";
    case 'R':
        return u8"◝";
    case 'l':
        return u8"◟";
    case 'r':
        return u8"◞";
    default:
        return std::string(1, glyph);
    }
}

std::string display_text_for_cell(const SpriteCell& cell) {
    if (cell.role == SpriteLayerRole::Prop && cell.glyph == sun_prop_glyph()) return u8"☀";
    if (cell.role == SpriteLayerRole::Prop && cell.glyph == moon_prop_glyph()) return u8"☾";
    switch (cell.role) {
    case SpriteLayerRole::CapCrown: return cap_crown_display_text(cell.glyph);
    case SpriteLayerRole::CapBrim:  return cap_brim_display_text(cell.glyph);
    case SpriteLayerRole::Eyes:     return eyes_display_text(cell.glyph);
    case SpriteLayerRole::Blush:    return blush_display_text(cell.glyph);
    case SpriteLayerRole::Body:     return body_display_text(cell.glyph);
    case SpriteLayerRole::Feet:     return feet_display_text(cell.glyph);
    case SpriteLayerRole::Effect:   return effect_display_text(cell.glyph);
    case SpriteLayerRole::Prop:
    case SpriteLayerRole::None:
    default:                        return std::string(1, cell.glyph);
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

int cloud_travel_width(const CloudTrack& track) {
    CloudSprite cloud;
    cloud.row = track.row;
    cloud.col = 0;
    cloud.rows = track.rows;
    return kInnerWidth + cloud_width(cloud) + 10;
}

std::uint32_t cloud_cycle_index(const CloudTrack& track, double seconds) {
    const int travel_width = cloud_travel_width(track);
    const double travel = seconds * track.speed_chars_per_seconds + track.phase_offset * static_cast<double>(travel_width);

    return static_cast<std::uint32_t>(
        std::floor(travel / static_cast<double>(travel_width)));
}

bool cloud_track_is_active(const CloudTrack& track, std::size_t track_index, double seconds, bool daytime) {
    const std::uint32_t cycle = cloud_cycle_index(track, seconds);
    const std::uint32_t seed = 
        0x51f15e5du ^
        static_cast<std::uint32_t>(track_index * 977u) ^
        (cycle * 131u) ^
        (daytime ? 0x13579bdfu : 0x2468ace0u);

    const double active_threshold = daytime ? 0.62 : 0.38;
    return unit_random(seed) < active_threshold;
}

std::size_t fallback_cloud_track_index(double seconds, bool daytime, std::size_t track_count) {
    const std::uint32_t window = static_cast<std::uint32_t>(seconds / 28.0);
    const std::uint32_t seed =
        0x9e3779b9u ^
        (window * 313u) ^
        (daytime ? 0x10203040u : 0x55667788u);

    return static_cast<std::size_t>(mix_bits(seed) % static_cast<std::uint32_t>(track_count));
}



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
    if (buddy.appearance.activity == Activity::Comforting) {
        return ansi::magenta;
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

double star_visibility_for_hour(double hour) {
    if (hour >= 18.0 && hour < 20.0) {
        return (hour - 18.0) / 2.0;
    }

    if (hour >= 20.0 || hour < 4.0) {
        return 1.0;
    }

    if (hour >= 4.0 && hour < 6.0) {
        return (6.0 - hour) / 2.0;
    }

    return 0.0;
}

std::uint32_t nightly_star_field_seed(std::tm local_tm) {
    if (local_tm.tm_hour < 6) {
        local_tm.tm_mday -= 1;
        std::mktime(&local_tm);
    }

    const std::uint32_t year = static_cast<std::uint32_t>(local_tm.tm_year + 1900);
    const std::uint32_t yday = static_cast<std::uint32_t>(local_tm.tm_yday);

    return mix_bits(0x6d2b79f5u ^ (year * 977u) ^ (yday * 1315423911u));
}

const char* sprite_cell_color(const BuddyRenderState& buddy, const SpriteCell& cell) {
    if (cell.glyph == ' ') {
        return ansi::reset;
    }
    if (cell.role == SpriteLayerRole::Prop) {
        if (cell.glyph == sun_prop_glyph()) return ansi::yellow;
        if (cell.glyph == moon_prop_glyph()) return ansi::cyan;
        if (cell.glyph == '*') return ansi::bright_white;
        if (cell.glyph == '+') return ansi::reset;
        if (cell.glyph == '.') return ansi::dim;
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
    prop.glyph = daytime ? sun_prop_glyph() : moon_prop_glyph();
    prop.role = SpriteLayerRole::Prop;
    return prop;
}

bool star_overlaps_moon(int row, int col, const SkyProp& prop) {
    if (prop.glyph != 'C') {
        return false;
    }

    return std::abs(row - prop.row) <= 1 && std::abs(col - prop.col) <= 3;
}

bool has_star_at(const std::vector<StarPoint>& stars, int row, int col) {
    for (const auto& star : stars) {
        if (star.row == row && star.col == col) {
            return true;
        }
    }
    return false;
}

char twinkle_glyph(double phase) {
    if (phase < 0.16) return ' ';
    if (phase < 0.42) return '.';
    if (phase < 0.70) return '+';
    if (phase < 0.90) return '*';
    return '+';
}

char attenuated_star_glyph(char glyph, double visibility) {
    if (visibility <= 0.05) return ' ';

    if (glyph == '*') {
        if (visibility < 0.35) return '.';
        if (visibility < 0.75) return '+';
        return '*';
    }

    if (glyph == '+') {
        return (visibility < 0.45) ? '.' : '+';
    }

    if (glyph == '.') {
        return (visibility < 0.22) ? ' ' : '.';
    }

    return glyph;
}

std::vector<StarPoint> current_stars(double seconds, const SkyProp& prop, std::uint32_t field_seed, double visibility) {
    constexpr int kStarFieldMinRow = 0;
    constexpr int kStarFieldMaxRow = 4;
    constexpr int kStarCandidateCount = 40;

    std::vector<StarPoint> stars;
    stars.reserve(kStarCandidateCount);

    for (std::uint32_t i = 0; i < static_cast<std::uint32_t>(kStarCandidateCount); ++i) {
        const std::uint32_t base_seed = mix_bits(field_seed ^ 0x7a4d3c21u ^ (i * 2654435761u));
        const int row = kStarFieldMinRow + static_cast<int>(mix_bits(base_seed ^ 0x13579bdfu) %
                static_cast<std::uint32_t>(kStarFieldMaxRow - kStarFieldMinRow + 1));
        const int col = 1 + static_cast<int>(mix_bits(base_seed ^ 0x2468ace0u) %
                static_cast<std::uint32_t>(kInnerWidth - 2));

        if (star_overlaps_moon(row, col, prop) || has_star_at(stars, row, col)) {
            continue;
        }

        const double twinkle_rate = 0.18 + 4.2 * unit_random(base_seed ^ 0xabcdef01u);
        const double phase_offset = unit_random(base_seed ^ 0x31415926u);
        const double phase = std::fmod(seconds * twinkle_rate + phase_offset, 1.0);
        const char glyph = attenuated_star_glyph(twinkle_glyph(phase), visibility);

        if (glyph == ' ') {
            continue;
        }

        stars.push_back(StarPoint{row, col, glyph});

    }

    return stars;
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

std::vector<CloudSprite> current_clouds(double seconds, bool daytime) { 
    const std::array<CloudTrack, 6> tracks{{
        {0, 0.28, 0.00, {" _--_ ",     "(___ )"}},
        {1, 0.18, 0.38, {"  _--_  ",   " (____) "}},
        {0, 0.23, 0.74, {" _-_ ",      "(___)"}},
        {2, 0.15, 0.19, {"   _---_ ",  " _(____)_"}},
        {1, 0.21, 0.57, {"  _-_  ",    " (___) "}},
        {0, 0.31, 0.86, {" __--__ ",   "(______)"}} 
    }};

    std::vector<CloudSprite> clouds;
    const std::size_t fallback_index = fallback_cloud_track_index(seconds, daytime, tracks.size());
    bool any_active = false;
    
    for (std::size_t i = 0; i < tracks.size(); ++i) {
        const bool active = cloud_track_is_active(tracks[i], i, seconds, daytime);
        if (!active) {
            continue;
        }

        const CloudTrack& track = tracks[i];
        clouds.push_back(make_cloud(track.row, seconds, track.speed_chars_per_seconds, track.phase_offset, track.rows));
        any_active = true;
    }

    if (!any_active) {
        const CloudTrack& track = tracks[fallback_index];
        clouds.push_back(make_cloud(track.row, seconds, track.speed_chars_per_seconds, track.phase_offset, track.rows));
    }

    return clouds;
}

SkySnapshot current_sky_snapshot() {
    const std::tm local_tm = current_local_tm();
    const double hour = current_local_hour(local_tm);
    const double seconds = seconds_since_midnight(local_tm);

    SkySnapshot sky;
    sky.daytime = is_daytime(hour);
    sky.prop = current_sky_prop(hour);
    sky.clouds = current_clouds(seconds, sky.daytime);

    if (!sky.daytime) {
        const std::uint32_t field_seed = nightly_star_field_seed(local_tm);
        const double visibility = star_visibility_for_hour(hour);
        sky.stars = current_stars(seconds, sky.prop, field_seed, visibility);
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
    } else {
        overlay_stars(stage, sky.stars);
        overlay_sky_prop(stage, sky.prop);
    }
    overlay_clouds(stage, sky.clouds);
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
        std::cout << display_text_for_cell(cell);
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
