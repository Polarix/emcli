/*
 * @file cli_port.h
 * @brief 平台抽象层接口声明
 */

#ifndef CLI_PORT_H
#define CLI_PORT_H

#ifdef __cplusplus
extern "C" {
#endif

/* 平台初始化 */
void platform_init(void);

/* 平台清理（如恢复终端设置） */
void platform_cleanup(void);

/* 非阻塞获取字符，返回-1表示无字符 */
int platform_getchar(void);

/* 输出字符 */
void platform_putchar(char c);

/* 输出字符串 */
void platform_puts(const char *s);

#ifdef __cplusplus
}
#endif

#endif /* CLI_PORT_H */
