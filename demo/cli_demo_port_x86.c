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
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
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

/* Windows 扩展键模拟缓冲区 */
#if defined(_WIN32) || defined(_WIN64)
#define ESC_BUFFER_SIZE 8
static char s_esc_buffer[ESC_BUFFER_SIZE];
static int s_esc_buffer_rd = 0;
static int s_esc_buffer_wr = 0;

static void esc_buffer_put(char c)
{
    int next = (s_esc_buffer_wr + 1) % ESC_BUFFER_SIZE;
    if (next != s_esc_buffer_rd)
    {
        s_esc_buffer[s_esc_buffer_wr] = c;
        s_esc_buffer_wr = next;
    }
}

static int esc_buffer_get(void)
{
    if (s_esc_buffer_rd == s_esc_buffer_wr)
        return -1;
    char c = s_esc_buffer[s_esc_buffer_rd];
    s_esc_buffer_rd = (s_esc_buffer_rd + 1) % ESC_BUFFER_SIZE;
    return (int)(unsigned char)c;
}
#endif

void platform_init(void)
{
#if defined(_WIN32) || defined(_WIN64)
    SetConsoleOutputCP(CP_UTF8);
    /* 启用虚拟终端处理（支持ANSI转义序列） */
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    if (GetConsoleMode(hOut, &dwMode))
    {
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, dwMode);
    }
    s_esc_buffer_rd = s_esc_buffer_wr = 0;
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
    int c = esc_buffer_get();
    if (c != -1)
        return c;

    if (_kbhit())
    {
        int ch = _getch();
        if (ch == 0xE0 || ch == 0x00)
        {
            int ext = _getch();
            switch (ext)
            {
                case 0x48: /* 上箭头 */
                    esc_buffer_put(0x1B);
                    esc_buffer_put('[');
                    esc_buffer_put('A');
                    break;
                case 0x50: /* 下箭头 */
                    esc_buffer_put(0x1B);
                    esc_buffer_put('[');
                    esc_buffer_put('B');
                    break;
                case 0x4D: /* 右箭头 */
                    esc_buffer_put(0x1B);
                    esc_buffer_put('[');
                    esc_buffer_put('C');
                    break;
                case 0x4B: /* 左箭头 */
                    esc_buffer_put(0x1B);
                    esc_buffer_put('[');
                    esc_buffer_put('D');
                    break;
                default:
                    break;
            }
            return esc_buffer_get();
        }
        else
        {
            return ch;  /* 普通字符，包括 TAB (9) 和回车 (13) */
        }
    }
    return -1;
#elif defined(__linux__) || defined(__unix__)
    fd_set readfds;
    struct timeval tv;
    char c;
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    if (select(STDIN_FILENO + 1, &readfds, NULL, NULL, &tv) > 0)
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
