#pragma once

#include <tuple>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <conio.h>

inline std::tuple<int,int> platform_init_console() {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (h == INVALID_HANDLE_VALUE) {
        return {80, 25};
    }
    DWORD mode = 0;
    if (GetConsoleMode(h, &mode)) {
        SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(h, &csbi)) {
        int rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
        int cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        return std::make_tuple(cols, rows);
    }
    return std::make_tuple(80, 25);
}

#else // POSIX

#include <chrono>
#include <thread>
#include <iostream>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/time.h>
#include <fcntl.h>
#include <stdio.h>

inline struct termios platform_orig_termios;
inline bool platform_raw_enabled = false;

static void platform_disable_raw_mode();

static void platform_enable_raw_mode() {
    if (platform_raw_enabled) return;
    if (tcgetattr(STDIN_FILENO, &platform_orig_termios) == -1) return;
    struct termios raw = platform_orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON | ISIG);
    raw.c_iflag &= ~(IXON | ICRNL);
    raw.c_oflag &= ~(OPOST);
    if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) == -1) return;
    platform_raw_enabled = true;
    atexit(platform_disable_raw_mode);
}

static void platform_disable_raw_mode() {
    if (!platform_raw_enabled) return;
    tcsetattr(STDIN_FILENO, TCSANOW, &platform_orig_termios);
    platform_raw_enabled = false;
}

inline std::tuple<int,int> platform_init_console() {
    platform_enable_raw_mode();
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
        int cols = (ws.ws_col > 0) ? ws.ws_col : 80;
        int rows = (ws.ws_row > 0) ? ws.ws_row : 24;
        return std::make_tuple(cols, rows);
    }
    return std::make_tuple(80, 24);
}

inline int _kbhit() {
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);
    struct timeval tv = {0, 0};
    int ret = select(STDIN_FILENO + 1, &readfds, NULL, NULL, &tv);
    return (ret > 0) && FD_ISSET(STDIN_FILENO, &readfds);
}

inline int _getch() {
    unsigned char c;
    if (read(STDIN_FILENO, &c, 1) <= 0) return -1;
    return c;
}

inline void Sleep(unsigned int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

inline void Beep(int /*freq*/, int ms) {
    std::cout << '\a' << std::flush;
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

#endif
