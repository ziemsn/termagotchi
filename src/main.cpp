#include "buddy.hpp"
#include "render.hpp"

#include <atomic>
#include <cctype>
#include <chrono>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <sys/types.h>
#include <thread>
#include <termios.h>
#include <unistd.h>

namespace {

Command parse_command(const std::string& input) {
    if (input == "feed") return Command::Feed;
    if (input == "pet") return Command::Pet;
    if (input == "sleep") return Command::Sleep;
    if (input == "wake") return Command::Wake;
    if (input == "q") return Command::Quit;
    if (input == "quit") return Command::Quit;
    return Command::Invalid;
}

struct TerminalModeGuard {
    termios original{};
    bool active = false;
    
    TerminalModeGuard() {
        if (::tcgetattr(STDIN_FILENO, &original) != 0) {
            return;
        }

        termios raw = original;
        raw.c_lflag &= static_cast<tcflag_t>(~(ICANON | ECHO));
        raw.c_cc[VMIN] = 0;
        raw.c_cc[VTIME] = 1;

        if (::tcsetattr(STDIN_FILENO, TCSANOW, &raw) == 0) {
            active = true;
        }
    }

    ~TerminalModeGuard() {
        if (active) {
            ::tcsetattr(STDIN_FILENO, TCSANOW, &original);
        }
    }
};

} // namespace
int main() {
    Buddy buddy("Mochi");
    TerminalModeGuard terminal_mode;

    render::hide_cursor();

    std::mutex io_mutex;
    std::queue<Command> command_queue;
    std::string current_input;
    std::string status_line = "Type a command and press Enter.";
    std::atomic<bool> stop_requested{false};

    std::thread input_thread([&]() {
        while (!stop_requested.load()) {
            char ch = '\0';
            const ssize_t nread = ::read(STDIN_FILENO, &ch, 1);

            if (nread <= 0) {
                continue;
            }

            std::lock_guard<std::mutex> lock(io_mutex);

            if (ch == '\n' || ch == '\r') {
                const std::string submitted = current_input;
                current_input.clear();

                if (submitted.empty()) {
                    status_line = "Enter a command.";
                    continue;
                }

                const Command cmd = parse_command(submitted);
                if (cmd == Command::Invalid) {
                    status_line = "Unknown command: " + submitted;
                    continue;
                }

                command_queue.push(cmd);
                status_line = "Queued: " + submitted;

                if (cmd == Command::Quit) {
                    stop_requested = true;
                }
                continue;
            }

            if (ch == 127 || ch == '\b') {
                if (!current_input.empty()) {
                    current_input.pop_back();
                }
                continue;
            }

            if (ch == 3) {
                command_queue.push(Command::Quit);
                status_line = "Queued: quit";
                stop_requested = true;
                continue;
            }

            const unsigned char uch = static_cast<unsigned char>(ch);
            if (std::isprint(uch)) {
                current_input.push_back(static_cast<char>(uch));
            }
        }

    });


    using clock = std::chrono::steady_clock;
    auto last = clock::now();
    constexpr auto frame_time = std::chrono::milliseconds(33);

    while (buddy.running()) {
        const auto frame_start = clock::now();
        std::chrono::duration<double> dt = frame_start - last;
        last = frame_start;

        {
            std::lock_guard<std::mutex> lock(io_mutex);
            while (!command_queue.empty()) {
                buddy.apply_command(command_queue.front());
                command_queue.pop();
            }
        }

        buddy.update(dt.count());
        
        std::string input_snapshot;
        {
            std::lock_guard<std::mutex> lock(io_mutex);
            input_snapshot = current_input;
        }

        const BuddyRenderState render_state = buddy.render_state();
        render::draw(render_state, input_snapshot);
        std::this_thread::sleep_until(frame_start + frame_time);
    }

    stop_requested = true;
    if (input_thread.joinable()) {
        input_thread.join();
    }

    render::show_cursor();
    std::cout << "\nGoodbye.\n";
    return 0;
}

