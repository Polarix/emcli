/*
 * @file cli_demo.c
 * @brief 演示程序主文件
 */

#include <cli.h>

/* 注册命令数据 */
extern const cli_command_t cmd_help_struct;
extern const cli_command_t cmd_echo_struct;
extern const cli_command_t cmd_clear_struct;
extern const cli_command_t cmd_version_struct;
extern const cli_command_t cmd_led_struct;

/* 声明平台函数（在 cli_port_x86.c 中实现） */
void platform_init(void);
void platform_cleanup(void);
int  platform_getchar(void);
void platform_putchar(char c);
void platform_puts(const char *s);

extern const cli_command_t g_cli_commands[];

/* Linux 信号处理 */
#if defined(__linux__) || defined(__unix__)
#include <signal.h>
#include <stdlib.h>
static void signal_handler(int sig)
{
    (void)sig;
    platform_cleanup();
    exit(0);
}
#endif

int main(void)
{
    /* 初始化平台 */
    platform_init();

#if defined(__linux__) || defined(__unix__)
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
#endif

    /* 定义IO接口 */
    cli_io_t io = {
        .getchar = platform_getchar,
        .putchar = platform_putchar,
        .puts    = platform_puts
    };

    /* 初始化CLI */
    cli_init(&io);

    /* 注册内置命令（传入命令结构体指针） */
    cli_command_register(&cmd_help_struct);
    cli_command_register(&cmd_echo_struct);
    cli_command_register(&cmd_clear_struct);
    cli_command_register(&cmd_version_struct);
    cli_command_register(&cmd_led_struct);

    /* 主循环 */
    while (1)
    {
        /* 周期性的调用处理函数，处理端口输入内容 */
        cli_ticks_handler();
        /* 可添加其他后台任务 */
    }

    /* 不会到达这里，但保留 */
    platform_cleanup();
    return 0;
}
