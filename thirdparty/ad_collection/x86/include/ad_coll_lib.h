#ifndef AD_COLL_LIB_H
#define AD_COLL_LIB_H
/**
 * @file AD_COLL_LIB.h
 * @brief 数字量采集SDK库（GPIO操作工具）
 * @details 该库提供GPIO引脚的导出、配置（输入/输出）、读写操作功能，
 *          用于通过sysfs接口控制Linux系统下的GPIO数字量采集与输出。
 * @version 1.0.0
 * @author lvhao
 * @date 2025-07-08
 * @copyright Copyright (c) 2025 顶点科技
 * 
 * @par 版本更新记录:
 * - v1.0.0 (2025-07-08): 初始版本，实现GPIO初始化、读写、释放资源功能。
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>
#include <errno.h>
#include <stdarg.h> 

/*X1-X8 为输入引脚  输入只能读取
Y1-Y8 为输出引脚    输出可以读写*/
#define PRINTF_OUT 1
/**
 * @brief GPIO引脚方向模式枚举
 */
typedef enum {
    GPIO_INPUT = 1,     // 输入模式
    GPIO_OUTPUT = 2    // 输出模式
} GpioDirectionMode;

/**
 * @brief GPIO读写模式枚举
 */
typedef enum {
    GPIO_READ = 1,      // 只读模式
    GPIO_WRITE = 2      // 读写模式
} GpioRwMode;

// 定义输入输出点的枚举类型
typedef enum {
    // 输入点 X1-X8
    X1 = 1, 
    X2, 
    X3, 
    X4, 
    X5, 
    X6, 
    X7, 
    X8,
    
    // 输出点 Y1-Y8
    Y1 = 20, 
    Y2, 
    Y3, 
    Y4, 
    Y5, 
    Y6, 
    Y7, 
    Y8
} IoPoint;

/**
 * @brief 设置并写入IoPoint对应的GPIO引脚值
 * 
 * @param point 要操作的IoPoint枚举值
 * @param value 要写入的GPIO值（非零为高电平，零为低电平）
 * @return int 操作成功返回0，失败返回1
 */
int sdk_write_gpio(IoPoint point, int value);
/**
 * @brief 读取IoPoint对应的GPIO引脚值
 * 
 * @param point 要操作的IoPoint枚举值
 * @param value 用于存储读取值的指针（返回1为高电平，0为低电平）
 * @return int 操作成功返回0，失败返回1
 */
int sdk_read_gpio(IoPoint point, int *value);
/**
 * @brief 初始化所有GPIO引脚（导出并设置方向）
 * @return 成功返回0，失败返回-1
 */
int gpio_init();
/**
 * @brief 清理所有GPIO引脚（取消导出）
 * @return 成功返回0，失败返回-1
 */
int gpio_cleanup();

#ifdef __cplusplus
}
#endif

#endif