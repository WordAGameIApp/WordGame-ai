#include "platform.h"
#include "cli.h"
#include "book_storage.h"
#include "api_client.h"
#include "game_menu.h"
#include "game_settings.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {
    platform_setup_utf8();

#ifdef _WIN32
    char **utf8_argv = platform_get_utf8_argv(&argc);
    if (!utf8_argv) {
        fprintf(stderr, "无法获取命令行参数\n");
        return 1;
    }
    argv = utf8_argv;
#endif

    // 如果有命令行参数，使用 CLI 模式
    if (argc > 1) {
        if (!api_global_init()) {
            fprintf(stderr, "API 全局初始化失败\n");
            return 1;
        }

        bookStorageInit("book", NULL);

        int result = cli_run(argc, argv);

        bookStorageCleanup();
        api_global_cleanup();

#ifdef _WIN32
        platform_free_utf8_argv(argc, utf8_argv);
#endif
        return result;
    }

    // 无参数启动游戏菜单模式
    printf("╔══════════════════════════════════════════╗\n");
    printf("║                                          ║\n");
    printf("║     Welcome to WordGame AI v%s       ║\n", platform_get_version());
    printf("║                                          ║\n");
    printf("╚══════════════════════════════════════════╝\n");

    if (!api_global_init()) {
        fprintf(stderr, "API 全局初始化失败\n");
        return 1;
    }

    bookStorageInit("book", NULL);

    GameSettings settings;
    game_settings_init(&settings);

    int result = game_menu_run(&settings);

    game_settings_free(&settings);
    bookStorageCleanup();
    api_global_cleanup();

#ifdef _WIN32
    platform_free_utf8_argv(argc, utf8_argv);
#endif

    return result;
}
