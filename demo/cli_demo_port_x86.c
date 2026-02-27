/*
 * @file cli_port_x86.c
 * @brief 平台抽象层实现 (适用于 Windows/Linux PC 环境)
 */

#include "cli_port.h"
#include <stdio.h>
#include <stdlib.h>

#if defined(_WIN32) || defined(_WIN64)
#include <conio.h>
#include <windows.h>
#elif defined(__linux__) || defined(__unix__)
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/time.h>
#include <signal.h>
#endif

/* 静态变量，用于Linux恢复终端 */
#if defined(__linux__) || defined(__unix__)
static struct termios s_orig_termios;
static int s_termios_modified = 0;
#endif

void platform_init(void)
{
#if defined(_WIN32) || defined(_WIN64)
    SetConsoleOutputCP(CP_UTF8);
#elif defined(__linux__) || defined(__unix__)
    struct termios new_termios;
    if (tcgetattr(STDIN_FILENO, &s_orig_termios) == 0)
    {
        new_termios = s_orig_termios;
        new_termios.c_lflag &= ~(ICANON | ECHO);
        new_termios.c_cc[VMIN] = 0;
        new_termios.c_cc[VTIME] = 0;
        if (tcsetattr(STDIN_FILENO, TCSANOW, &new_termios) == 0)
        {
            s_termios_modified = 1;
        }
    }
#endif
}

void platform_cleanup(void)
{
#if defined(__linux__) || defined(__unix__)
    if (s_termios_modified)
    {
        tcsetattr(STDIN_FILENO, TCSANOW, &s_orig_termios);
        s_termios_modified = 0;
    }
#endif
}

int platform_getchar(void)
{
#if defined(_WIN32) || defined(_WIN64)
    if (_kbhit())
    {
        return _getch();
    }
    return -1;
#elif defined(__linux__) || defined(__unix__)
    fd_set readfds;
    struct timeval tv;
    int ret;
    char c;
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    ret = select(STDIN_FILENO + 1, &readfds, NULL, NULL, &tv);
    if (ret > 0 && FD_ISSET(STDIN_FILENO, &readfds))
    {
        if (read(STDIN_FILENO, &c, 1) == 1)
        {
            return (int)(unsigned char)c;
        }
    }
    return -1;
#else
    return -1;
#endif
}

void platform_putchar(char c)
{
#if defined(_WIN32) || defined(_WIN64)
    putchar(c);
    fflush(stdout);
#elif defined(__linux__) || defined(__unix__)
    write(STDOUT_FILENO, &c, 1);
    fsync(STDOUT_FILENO);
#endif
}

void platform_puts(const char *s)
{
    while (*s)
    {
        platform_putchar(*s++);
    }
}
