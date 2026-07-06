/*
 * main.c - 应用入口
 *
 * 具体模块初始化和运行线程统一放在 app_thread.c 中管理。
 */
#include "../threads/app_thread.h"

int main(void)
{
    return app_thread_init();
}
