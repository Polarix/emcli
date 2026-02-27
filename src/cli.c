/*
 * @file cli.c
 * @brief 命令行接口核心实现（支持Tab键命令补全和格式化输出）
 */

#include <cli.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>

/* 配置宏 */
#ifndef CLI_MAX_LINE_LENGTH
#define CLI_MAX_LINE_LENGTH     128   /* 命令行最大长度 */
#endif

#ifndef CLI_MAX_ARGS
#define CLI_MAX_ARGS            16    /* 最大参数个数 */
#endif

#ifndef CLI_HISTORY_SIZE
#define CLI_HISTORY_SIZE        0     /* 历史命令条数，0表示不启用 */
#endif

/* 命令行状态 */
typedef enum
{
    CLI_STATE_NORMAL,          /* 正常接收状态 */
    CLI_STATE_ESC,             /* 收到ESC，等待后续 */
    CLI_STATE_CSI              /* 收到CSI，等待后续 */
} cli_state_t;

/* 全局CLI数据 */
static struct
{
    char line[CLI_MAX_LINE_LENGTH];   /* 当前行缓冲区 */
    size_t pos;                        /* 当前光标位置 */
    size_t len;                        /* 当前行长度 */
    cli_state_t state;                  /* 转义状态 */
#if CLI_HISTORY_SIZE > 0
    /* 历史记录暂不实现 */
#endif
} s_cli;

/* 保存注册的IO接口 */
static const cli_io_t *s_io = NULL;

/* 命令表结构体，封装命令数组和计数 */
typedef struct
{
    cli_command_t commands[CLI_MAX_COMMANDS];
    int count;
} cli_command_table_t;

/* 静态命令表实例 */
static cli_command_table_t s_cmd_table = { .count = 0 };

/* 静态函数声明 */
static void cli_newline(void);
static void cli_backspace(void);
static void cli_redraw_line(void);
static void cli_execute(void);
static int  cli_parse_line(char *line, char **argv, int max_args);
static void cli_handle_tab(void);
static int  cli_find_command_matches(const char *prefix, char *matched_name, size_t matched_name_size);
static void cli_vprintf(const char *format, va_list args);
static const char* cli_get_prompt(void);

/* 初始化 */
void cli_init(const cli_io_t *io)
{
    s_io = io;
    memset(&s_cli, 0, sizeof(s_cli));
    s_cli.state = CLI_STATE_NORMAL;
    cli_puts(cli_get_prompt());
}

/* 注册命令 */
cli_error_t cli_command_register(const cli_command_t *cmd)
{
    cli_error_t result;

    if (cmd == NULL || cmd->name == NULL || cmd->handler == NULL)
    {
        result = CLI_ERR_INVALID_PARAM;
    }
    else
    {
        if (s_cmd_table.count >= CLI_MAX_COMMANDS)
        {
            result = CLI_ERR_TABLE_FULL;
        }
        else
        {
            result = CLI_SUCCESS;
            int cmd_idx = 0;
            /* 检查名称是否重复 */
            while (cmd_idx < s_cmd_table.count)
            {
                if (strcmp(s_cmd_table.commands[cmd_idx].name, cmd->name) == 0)
                {
                    result = CLI_ERR_DUPLICATE;
                    break;
                }
                cmd_idx++;
            }

            if(CLI_SUCCESS == result)
            {
                /* 添加命令到表 */
                s_cmd_table.commands[s_cmd_table.count].name = cmd->name;
                s_cmd_table.commands[s_cmd_table.count].short_name = cmd->short_name;
                s_cmd_table.commands[s_cmd_table.count].help = cmd->help;
                s_cmd_table.commands[s_cmd_table.count].handler = cmd->handler;
                s_cmd_table.count++;
            }
            else
            {
                /* 已存在相同命令，返回错误 */
                /* result = CLI_ERR_DUPLICATE; */
            }
        }
    }
    return result;
}

/* 获取已注册命令的数量 */
int cli_get_command_count(void)
{
    return s_cmd_table.count;
}

/* 根据索引获取命令结构体指针 */
const cli_command_t* cli_get_command_dsc(int index)
{
    if (index < 0 || index >= s_cmd_table.count)
    {
        return NULL;
    }
    return &s_cmd_table.commands[index];
}

/* 定时处理函数 */
void cli_ticks_handler(void)
{
    if (s_io == NULL) return;
    int c = s_io->getchar();
    if (c != -1)
    {
        cli_process_char((char)c);
    }
}

/* 输出字符（供外部使用） */
void cli_putchar(char c)
{
    if (s_io && s_io->putchar)
    {
        s_io->putchar(c);
    }
}

/* 输出字符串（供外部使用） */
void cli_puts(const char *s)
{
    if (s_io && s_io->puts)
    {
        s_io->puts(s);
    }
}

/* 获取提示符（静态私有） */
static const char* cli_get_prompt(void)
{
    return "CLI> ";
}

/* 处理单个字符 */
void cli_process_char(char c)
{
    /* 先处理转义序列 */
    if (s_cli.state != CLI_STATE_NORMAL)
    {
        /* 简化的转义处理，只处理方向键等，可以扩展 */
        if (s_cli.state == CLI_STATE_ESC)
        {
            if (c == '[')
            {
                s_cli.state = CLI_STATE_CSI;
            }
            else
            {
                /* 未知序列，复位 */
                s_cli.state = CLI_STATE_NORMAL;
            }
        }
        else if (s_cli.state == CLI_STATE_CSI)
        {
            /* 处理CSI命令，例如方向键 */
            switch (c)
            {
                case 'A': /* 上箭头 */
                    /* 历史命令，暂不实现 */
                    break;
                case 'B': /* 下箭头 */
                    break;
                case 'C': /* 右箭头 */
                    if (s_cli.pos < s_cli.len)
                    {
                        s_cli.pos++;
                        cli_putchar(c); /* 移动光标 */
                    }
                    break;
                case 'D': /* 左箭头 */
                    if (s_cli.pos > 0)
                    {
                        s_cli.pos--;
                        cli_putchar(c); /* 移动光标 */
                    }
                    break;
                default:
                    break;
            }
            s_cli.state = CLI_STATE_NORMAL;
        }
        return;
    }

    /* 处理普通字符 */
    if (c == '\r' || c == '\n')
    {
        /* 回车 */
        cli_newline();
        cli_execute();
        /* 重新显示提示符 */
        cli_puts(cli_get_prompt());
    }
    else if (c == '\b' || c == 0x7F) /* 退格 */
    {
        cli_backspace();
    }
    else if (c == '\t') /* Tab键：命令补全 */
    {
        cli_handle_tab();
    }
    else if (c == 0x1B) /* ESC */
    {
        s_cli.state = CLI_STATE_ESC;
    }
    else if (c >= 0x20 && c <= 0x7E) /* 可打印字符 */
    {
        if (s_cli.len < CLI_MAX_LINE_LENGTH - 1)
        {
            /* 如果有字符在光标后，需要插入 */
            if (s_cli.pos < s_cli.len)
            {
                /* 移动后续字符 */
                memmove(&s_cli.line[s_cli.pos + 1],
                        &s_cli.line[s_cli.pos],
                        s_cli.len - s_cli.pos);
            }
            s_cli.line[s_cli.pos] = c;
            s_cli.pos++;
            s_cli.len++;

            /* 回显并重绘后续字符 */
            cli_putchar(c);
            if (s_cli.pos < s_cli.len)
            {
                size_t i;
                for (i = s_cli.pos; i < s_cli.len; i++)
                {
                    cli_putchar(s_cli.line[i]);
                }
                for (i = s_cli.pos; i < s_cli.len; i++)
                {
                    cli_putchar('\b');
                }
            }
        }
    }
}

/* 换行 */
static void cli_newline(void)
{
    cli_puts("\r\n");
}

/* 重绘当前行（在列出候选命令后恢复输入行） */
static void cli_redraw_line(void)
{
    size_t i;
    cli_puts(cli_get_prompt());
    cli_puts(s_cli.line);
    /* 将光标移回原位置（从行尾左移 len - pos 个字符） */
    for (i = s_cli.len; i > s_cli.pos; i--)
    {
        cli_putchar('\b');
    }
}

/* 退格处理 */
static void cli_backspace(void)
{
    if (s_cli.pos > 0)
    {
        /* 删除光标前一个字符 */
        memmove(&s_cli.line[s_cli.pos - 1],
                &s_cli.line[s_cli.pos],
                s_cli.len - s_cli.pos);
        s_cli.pos--;
        s_cli.len--;

        /* 退格显示：光标左移，输出空格，再左移 */
        cli_putchar('\b');
        cli_putchar(' ');
        cli_putchar('\b');

        /* 如果删除后还有后续字符，需要重新显示并移回光标 */
        if (s_cli.pos < s_cli.len)
        {
            size_t i;
            for (i = s_cli.pos; i < s_cli.len; i++)
            {
                cli_putchar(s_cli.line[i]);
            }
            for (i = s_cli.pos; i < s_cli.len; i++)
            {
                cli_putchar('\b');
            }
        }
    }
}

/* 查找匹配的命令前缀
 * 返回值：匹配的命令个数
 * 如果 count == 1，matched_name 填充实际匹配的名字（长名或短名）
 */
static int cli_find_command_matches(const char *prefix, char *matched_name, size_t matched_name_size)
{
    int count = 0;
    size_t prefix_len = strlen(prefix);
    const char *first_match = NULL;

    for (int i = 0; i < s_cmd_table.count; i++)
    {
        const cli_command_t *cmd = &s_cmd_table.commands[i];
        /* 检查长名是否匹配 */
        if (strncmp(prefix, cmd->name, prefix_len) == 0)
        {
            count++;
            if (count == 1)
            {
                first_match = cmd->name;
            }
        }
        /* 检查短名是否匹配（如果存在） */
        else if (cmd->short_name != NULL && strncmp(prefix, cmd->short_name, prefix_len) == 0)
        {
            count++;
            if (count == 1)
            {
                first_match = cmd->short_name;
            }
        }
    }

    if (count == 1 && matched_name != NULL && first_match != NULL)
    {
        strncpy(matched_name, first_match, matched_name_size - 1);
        matched_name[matched_name_size - 1] = '\0';
    }

    return count;
}

/* 处理Tab键命令补全 */
static void cli_handle_tab(void)
{
    char *word_start;
    size_t word_len;
    char matched_name[CLI_MAX_LINE_LENGTH];
    int match_count;

    /* 检查行中是否有空格，若有则忽略Tab */
    for (size_t i = 0; i < s_cli.len; i++)
    {
        if (s_cli.line[i] == ' ' || s_cli.line[i] == '\t')
        {
            cli_putchar('\a');
            return;
        }
    }

    /* 定位当前第一个单词 */
    word_start = s_cli.line;
    word_len = s_cli.len;

    /* 提取前缀 */
    char prefix[CLI_MAX_LINE_LENGTH];
    strncpy(prefix, word_start, word_len);
    prefix[word_len] = '\0';

    /* 查找匹配的命令 */
    match_count = cli_find_command_matches(prefix, matched_name, sizeof(matched_name));

    if (match_count == 0)
    {
        cli_putchar('\a');
    }
    else if (match_count == 1)
    {
        size_t full_len = strlen(matched_name);
        size_t prefix_len = word_len;

        if (full_len > prefix_len)
        {
            size_t new_len = s_cli.len + (full_len - prefix_len);
            if (new_len >= CLI_MAX_LINE_LENGTH - 1)
            {
                cli_putchar('\a');
                return;
            }

            if (s_cli.len > word_len)
            {
                memmove(word_start + full_len,
                        word_start + prefix_len,
                        s_cli.len - prefix_len);
            }

            strncpy(word_start, matched_name, full_len);
            s_cli.len = new_len;
            s_cli.pos = word_start - s_cli.line + full_len;

            cli_puts("\r");
            cli_redraw_line();
        }
    }
    else
    {
        /* 列出所有匹配命令（显示长名，括号内短名） */
        cli_newline();
        for (int i = 0; i < s_cmd_table.count; i++)
        {
            const cli_command_t *cmd = &s_cmd_table.commands[i];
            int match = 0;
            if (strncmp(prefix, cmd->name, word_len) == 0)
                match = 1;
            else if (cmd->short_name != NULL && strncmp(prefix, cmd->short_name, word_len) == 0)
                match = 1;
            if (match)
            {
                cli_puts("  ");
                cli_puts(cmd->name);
                if (cmd->short_name != NULL)
                {
                    cli_puts(" (");
                    cli_puts(cmd->short_name);
                    cli_puts(")");
                }
                cli_newline();
            }
        }
        cli_redraw_line();
    }
}

/* 执行命令行 */
static void cli_execute(void)
{
    char *argv[CLI_MAX_ARGS];
    int argc;
    int found = 0;

    argc = cli_parse_line(s_cli.line, argv, CLI_MAX_ARGS);

    if (argc > 0)
    {
        for (int i = 0; i < s_cmd_table.count; i++)
        {
            const cli_command_t *cmd = &s_cmd_table.commands[i];
            /* 同时匹配长名和短名 */
            if (strcmp(argv[0], cmd->name) == 0 ||
                (cmd->short_name != NULL && strcmp(argv[0], cmd->short_name) == 0))
            {
                found = 1;
                int ret = cmd->handler(argc, argv);
                if (ret != 0)
                {
                    cli_puts("Command returned error\r\n");
                }
                break;
            }
        }

        if (!found)
        {
            cli_puts("Unknown command: ");
            cli_puts(argv[0]);
            cli_newline();
        }
    }

    /* 清空命令行缓冲区 */
    s_cli.len = 0;
    s_cli.pos = 0;
    memset(s_cli.line, 0, sizeof(s_cli.line));
}

/* 解析命令行参数 */
static int cli_parse_line(char *line, char **argv, int max_args)
{
    int argc = 0;
    char *p = line;
    char *start;
    int in_quote = 0;

    while (*p != '\0' && argc < max_args)
    {
        while (*p == ' ' || *p == '\t')
        {
            p++;
        }
        if (*p == '\0')
        {
            break;
        }

        start = p;

        if (*p == '"')
        {
            in_quote = 1;
            p++;
            start = p;
            while (*p != '\0' && (*p != '"' || in_quote))
            {
                if (*p == '\\' && *(p+1) == '"')
                {
                    memmove(p, p+1, strlen(p));
                }
                p++;
            }
            if (*p == '"')
            {
                *p = '\0';
                p++;
            }
            in_quote = 0;
        }
        else
        {
            while (*p != '\0' && *p != ' ' && *p != '\t')
            {
                p++;
            }
        }

        if (*p == ' ' || *p == '\t')
        {
            *p = '\0';
            p++;
        }

        argv[argc++] = start;
    }

    argv[argc] = NULL;
    return argc;
}

/* 内部格式化输出实现 */
static void cli_vprintf(const char *format, va_list args)
{
    for (const char *p = format; *p != '\0'; p++)
    {
        if (*p != '%')
        {
            cli_putchar(*p);
            continue;
        }

        p++;
        switch (*p)
        {
            case 'd':
            {
                int val = va_arg(args, int);
                if (val < 0)
                {
                    cli_putchar('-');
                    val = -val;
                }
                char buf[12];
                int i = 0;
                do {
                    buf[i++] = '0' + (val % 10);
                    val /= 10;
                } while (val > 0);
                while (i > 0)
                {
                    cli_putchar(buf[--i]);
                }
                break;
            }

            case 'u':
            {
                unsigned int val = va_arg(args, unsigned int);
                char buf[12];
                int i = 0;
                do {
                    buf[i++] = '0' + (val % 10);
                    val /= 10;
                } while (val > 0);
                while (i > 0)
                {
                    cli_putchar(buf[--i]);
                }
                break;
            }

            case 'x':
            {
                unsigned int val = va_arg(args, unsigned int);
                char buf[12];
                int i = 0;
                do {
                    int digit = val % 16;
                    buf[i++] = (digit < 10) ? ('0' + digit) : ('a' + digit - 10);
                    val /= 16;
                } while (val > 0);
                while (i > 0)
                {
                    cli_putchar(buf[--i]);
                }
                break;
            }

            case 's':
            {
                const char *s = va_arg(args, const char*);
                if (s != NULL)
                {
                    while (*s)
                    {
                        cli_putchar(*s++);
                    }
                }
                else
                {
                    cli_puts("(null)");
                }
                break;
            }

            case 'c':
            {
                char c = (char)va_arg(args, int);
                cli_putchar(c);
                break;
            }

            case '%':
                cli_putchar('%');
                break;

            default:
                cli_putchar('%');
                cli_putchar(*p);
                break;
        }
    }
}

/* 公开的格式化输出函数 */
void cli_printf(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    cli_vprintf(format, args);
    va_end(args);
}
