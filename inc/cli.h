/*
 * @file cli.h
 * @brief 命令行接口核心头文件
 */

#ifndef CLI_H
#define CLI_H

#include <stddef.h> /* for size_t */

#ifdef __cplusplus
extern "C" {
#endif

/* 最大支持命令数（可根据需求调整） */
#ifndef CLI_MAX_COMMANDS
#define CLI_MAX_COMMANDS 16
#endif

/* 错误码定义 */
typedef enum
{
    CLI_SUCCESS = 0,                /* 成功 */
    CLI_ERR_INVALID_PARAM = -1,      /* 无效参数 */
    CLI_ERR_TABLE_FULL = -2,         /* 命令表已满 */
    CLI_ERR_DUPLICATE = -3           /* 命令名重复 */
} cli_error_t;

/* IO接口结构体 */
typedef struct
{
    int  (*getchar)(void);          /* 非阻塞获取字符，返回-1表示无字符 */
    void (*putchar)(char c);         /* 输出字符 */
    void (*puts)(const char *s);     /* 输出字符串 */
} cli_io_t;

/* 命令结构体定义 */
typedef struct cli_command
{
    const char *name;               /* 命令长名称 */
    const char *short_name;          /* 命令短名称，可为NULL */
    const char *help;                /* 帮助信息 */
    int (*handler)(int argc, char **argv);  /* 命令处理函数指针 */
} cli_command_t;

/* CLI初始化，传入IO接口 */
void cli_init(const cli_io_t *io);

/* 注册命令（可多次调用，返回0成功，负值错误） */
cli_error_t cli_command_register(const cli_command_t *cmd);

/* 获取已注册命令的数量 */
int cli_get_command_count(void);

/* 根据索引获取命令结构体指针，索引范围 0 ~ cli_get_command_count()-1，返回NULL表示无效索引 */
const cli_command_t* cli_get_command_dsc(int index);

/* 定时处理函数（通常在主循环中调用），处理输入字符 */
void cli_ticks_handler(void);

/* 处理单个字符（如果外部直接提供字符，也可以调用此函数） */
void cli_process_char(char c);

/* 输出字符（供命令处理函数使用） */
void cli_putchar(char c);

/* 输出字符串（供命令处理函数使用） */
void cli_puts(const char *s);

/* 格式化输出（支持 %d, %u, %x, %s, %c, %%），通过 cli_putchar 逐字符输出 */
void cli_printf(const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif /* CLI_H */
