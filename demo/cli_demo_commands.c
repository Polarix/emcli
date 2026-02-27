/*
 * @file cli_commands.c
 * @brief 示例命令实现
 */

#include <cli.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

/* 静态函数声明 */
static int cmd_help(int argc, char **argv);
static int cmd_echo(int argc, char **argv);
static int cmd_clear(int argc, char **argv);
static int cmd_version(int argc, char **argv);
static int cmd_led(int argc, char **argv);

/* 定义命令结构体（静态常量，生命周期持续整个程序） */
const cli_command_t cmd_help_struct = {
    .name = "help",
    .short_name = "h",               /* 短名 h */
    .help = "Show this help message",
    .handler = cmd_help
};

const cli_command_t cmd_echo_struct = {
    .name = "echo",
    .short_name = "e",               /* 短名 e */
    .help = "Echo the arguments",
    .handler = cmd_echo
};

const cli_command_t cmd_clear_struct = {
    .name = "clear",
    .short_name = "c",               /* 短名 c */
    .help = "Clear the screen",
    .handler = cmd_clear
};

const cli_command_t cmd_version_struct = {
    .name = "version",
    .short_name = "v",               /* 短名 v */
    .help = "Show version information",
    .handler = cmd_version
};

const cli_command_t cmd_led_struct = {
    .name = "led",
    .short_name = "l",               /* 短名 l */
    .help = "Control and change the state of an LED light",
    .handler = cmd_led
};

/* 帮助命令 - 动态获取所有已注册命令 */
static int cmd_help(int argc, char **argv)
{
    int count;
    int i;
    const cli_command_t *cmd;

    (void)argc;
    (void)argv;

    count = cli_get_command_count();
    cli_puts("\r\nAvailable commands:\r\n");
    for (i = 0; i < count; i++)
    {
        cmd = cli_get_command_dsc(i);
        if (cmd != NULL)
        {
            cli_puts("  ");
            cli_puts(cmd->name);
            if (cmd->short_name != NULL)
            {
                cli_puts(" (");
                cli_puts(cmd->short_name);
                cli_puts(")");
            }
            cli_puts(" - ");
            if (cmd->help != NULL)
            {
                cli_puts(cmd->help);
            }
            cli_puts("\r\n");
        }
    }
    return 0;
}

/* 回显命令 */
static int cmd_echo(int argc, char **argv)
{
    int i;
    for (i = 1; i < argc; i++)
    {
        if (i > 1)
        {
            cli_putchar(' ');
        }
        cli_puts(argv[i]);
    }
    cli_puts("\r\n");
    return 0;
}

/* 清屏命令 */
static int cmd_clear(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    /* 发送清屏转义序列 */
    cli_puts("\033[2J\033[H");
    return 0;
}

/* 版本命令 */
static int cmd_version(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    cli_puts("CLI Framework version 1.0\r\n");
    return 0;
}

/* LED 控制命令示例 */
static int cmd_led(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    if(argc > 2)
    {
        cli_puts("LED ");
        cli_puts(argv[1]);
        cli_puts(" ");
        cli_puts(argv[2]);
        cli_puts("\n");
    }
    else
    {
        cli_puts("Incomplete parameter.\r\n");
    }
    return 0;
}
