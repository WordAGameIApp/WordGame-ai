#include "cli.h"
#include "platform.h"
#include "config.h"
#include "context.h"
#include "api_client.h"
#include "api_types.h"
#include "context_manager.h"
#include "spawnlore.h"
#include "ai_settings.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_usage(const char *program_name) {
    printf("用法:\n");
    printf("  %s --config <settings.json> <prompt>       使用配置文件发送消息\n", program_name);
    printf("  %s --spawnworld [context]                  使用spawnworld提示词发送消息\n", program_name);
    printf("  %s --context-create <name> [filename]      创建新上下文\n", program_name);
    printf("  %s --context-list                          列出所有上下文\n", program_name);
    printf("  %s --context-switch <name>                 切换到指定上下文\n", program_name);
    printf("  %s --context-delete <name>                 删除上下文\n", program_name);
    printf("  %s --context-compress <name> [summary]     压缩上下文\n", program_name);
    printf("  %s --context-stats <name>                  显示上下文统计\n", program_name);
    printf("  %s --context-rename <old> <new>            重命名上下文\n", program_name);
    printf("  %s --version                               显示版本号\n", program_name);
    printf("\n示例:\n");
    printf("  %s --config settings.json \"你好\"\n", program_name);
    printf("  %s --spawnworld \"创建一个魔法世界\"\n", program_name);
    printf("  %s --context-create work work_context.json\n", program_name);
    printf("  %s --context-list\n", program_name);
    printf("  %s --context-switch work\n", program_name);
}

static int cmd_spawnworld(int argc, char *argv[]) {
    if (argc < 3) {
        printf("错误: 请提供上下文描述\n");
        printf("用法: %s --spawnworld <context>\n", argv[0]);
        return 1;
    }

    const char *context = argv[2];
    const char *prompt_file = "prompt/spawnworld.prompt";
    const char *config_file = "settings.json";

    char *prompt_content = utils_read_prompt_file(prompt_file);
    if (!prompt_content) {
        fprintf(stderr, "无法读取提示词文件: %s\n", prompt_file);
        return 1;
    }

    AiSettings aiSettings;
    aiSettingsInit(&aiSettings);

    if (!aiSettingsLoad(&aiSettings, config_file)) {
        fprintf(stderr, "无法加载 AI 设置\n");
        free(prompt_content);
        aiSettingsFree(&aiSettings);
        return 1;
    }

    SpawnLoreResult *result = spawnLore(prompt_content, context, &aiSettings);

    int exit_code = 1;
    if (result) {
        if (result->success) {
            printf("\n=== AI 回复 ===\n");
            if (result->content) {
                printf("内容:\n%s\n", result->content);
            }
            if (result->reasoningContent) {
                printf("\n思考过程:\n%s\n", result->reasoningContent);
            }
            exit_code = 0;
        } else {
            fprintf(stderr, "请求失败\n");
            if (result->content) {
                fprintf(stderr, "错误: %s\n", result->content);
            }
        }
        spawnLoreResultFree(result);
    }

    free(prompt_content);
    aiSettingsFree(&aiSettings);
    return exit_code;
}

static int cmd_context_create(int argc, char *argv[]) {
    if (argc < 3) {
        printf("错误: 请指定上下文名称\n");
        printf("用法: %s --context-create <name> [filename]\n", argv[0]);
        return 1;
    }

    const char *name = argv[2];
    const char *filename = argc > 3 ? argv[3] : NULL;

    ContextManager mgr;
    context_manager_init(&mgr, 10);
    context_manager_load_state(&mgr, "context_state.json");

    if (context_manager_exists(&mgr, name)) {
        fprintf(stderr, "上下文 '%s' 已存在\n", name);
        context_manager_free(&mgr);
        return 1;
    }

    if (!context_manager_create(&mgr, name, filename, 100, 1, 50)) {
        fprintf(stderr, "创建上下文 '%s' 失败\n", name);
        context_manager_free(&mgr);
        return 1;
    }
    printf("上下文 '%s' 创建成功\n", name);
    if (filename) {
        printf("存储文件: %s\n", filename);
    }

    context_manager_save_state(&mgr, "context_state.json");
    context_manager_save_all(&mgr);
    context_manager_free(&mgr);
    return 0;
}

static int cmd_context_list(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    ContextManager mgr;
    context_manager_init(&mgr, 10);

    context_manager_load_state(&mgr, "context_state.json");
    context_manager_load_all(&mgr);

    context_manager_list(&mgr);
    context_manager_free(&mgr);
    return 0;
}

static int cmd_context_switch(int argc, char *argv[]) {
    if (argc < 3) {
        printf("错误: 请指定上下文名称\n");
        return 1;
    }

    ContextManager mgr;
    context_manager_init(&mgr, 10);
    context_manager_load_state(&mgr, "context_state.json");

    if (!context_manager_switch(&mgr, argv[2])) {
        fprintf(stderr, "切换失败: 上下文 '%s' 不存在\n", argv[2]);
        context_manager_free(&mgr);
        return 1;
    }

    context_manager_save_state(&mgr, "context_state.json");
    printf("已切换到上下文: %s\n", argv[2]);
    context_manager_free(&mgr);
    return 0;
}

static int cmd_context_delete(int argc, char *argv[]) {
    if (argc < 3) {
        printf("错误: 请指定上下文名称\n");
        return 1;
    }

    ContextManager mgr;
    context_manager_init(&mgr, 10);
    context_manager_load_state(&mgr, "context_state.json");

    if (!context_manager_remove(&mgr, argv[2])) {
        fprintf(stderr, "删除失败: 上下文 '%s' 不存在\n", argv[2]);
        context_manager_free(&mgr);
        return 1;
    }

    context_manager_save_state(&mgr, "context_state.json");
    printf("已删除上下文: %s\n", argv[2]);
    context_manager_free(&mgr);
    return 0;
}

static int cmd_context_compress(int argc, char *argv[]) {
    if (argc < 3) {
        printf("错误: 请指定上下文名称\n");
        return 1;
    }

    const char *name = argv[2];
    const char *summary = argc > 3 ? argv[3] : NULL;

    ContextManager mgr;
    context_manager_init(&mgr, 10);
    context_manager_load_state(&mgr, "context_state.json");
    context_manager_load_all(&mgr);

    NamedContext *nctx = context_manager_get(&mgr, name);
    if (!nctx) {
        fprintf(stderr, "错误: 上下文 '%s' 不存在\n", name);
        context_manager_free(&mgr);
        return 1;
    }

    if (summary) {
        context_compress_manual(nctx, summary);
    } else {
        context_compress_manual(nctx, "用户请求压缩上下文");
    }

    context_manager_save_all(&mgr);
    context_manager_list(&mgr);
    context_manager_free(&mgr);
    return 0;
}

static int cmd_context_stats(int argc, char *argv[]) {
    if (argc < 3) {
        printf("错误: 请指定上下文名称\n");
        return 1;
    }

    ContextManager mgr;
    context_manager_init(&mgr, 10);
    context_manager_load_state(&mgr, "context_state.json");
    context_manager_load_all(&mgr);

    NamedContext *nctx = context_manager_get(&mgr, argv[2]);
    if (nctx) {
        int msgs, chars;
        context_get_stats(nctx, &msgs, &chars);
        printf("上下文 '%s' 统计:\n", argv[2]);
        printf("  消息数: %d\n", msgs);
        printf("  总字符: %d\n", chars);
        printf("  平均每消息: %.1f 字符\n", msgs > 0 ? (float)chars / msgs : 0.0f);
    } else {
        fprintf(stderr, "错误: 上下文 '%s' 不存在\n", argv[2]);
        context_manager_free(&mgr);
        return 1;
    }

    context_manager_free(&mgr);
    return 0;
}

static int cmd_context_rename(int argc, char *argv[]) {
    if (argc < 4) {
        printf("错误: 请指定旧名称和新名称\n");
        return 1;
    }

    ContextManager mgr;
    context_manager_init(&mgr, 10);
    context_manager_load_state(&mgr, "context_state.json");

    if (!context_manager_rename(&mgr, argv[2], argv[3])) {
        fprintf(stderr, "重命名失败: 上下文 '%s' 不存在或 '%s' 已被占用\n", argv[2], argv[3]);
        context_manager_free(&mgr);
        return 1;
    }

    context_manager_save_state(&mgr, "context_state.json");
    printf("已重命名: '%s' -> '%s'\n", argv[2], argv[3]);
    context_manager_free(&mgr);
    return 0;
}

static int cmd_config(int argc, char *argv[]) {
    if (argc < 4) {
        printf("错误: 请提供配置文件和提示词\n");
        printf("用法: %s --config <settings.json> <prompt>\n", argv[0]);
        return 1;
    }

    const char *config_file = argv[2];
    const char *prompt = argv[3];

    ApiConfig config;
    memset(&config, 0, sizeof(ApiConfig));
    ContextManager mgr;

    config_init_default_params(&config.params);
    context_manager_init(&mgr, 10);

    if (!config_load(config_file, &config)) {
        fprintf(stderr, "无法加载配置文件: %s\n", config_file);
        context_manager_free(&mgr);
        return 1;
    }

    if (!config_validate(&config)) {
        fprintf(stderr, "配置文件验证失败: %s\n", config_file);
        config_free(&config);
        context_manager_free(&mgr);
        return 1;
    }

    const char *ctx_name = "default";
    const char *ctx_file = config.context_file ? config.context_file : "context.json";

    NamedContext *nctx = context_manager_get(&mgr, ctx_name);
    if (!nctx) {
        context_manager_create(&mgr, ctx_name, ctx_file, 100,
                               config.params.use_default ? 0 : 1, 50);
        nctx = context_manager_get(&mgr, ctx_name);
    }

    if (!nctx) {
        fprintf(stderr, "无法创建上下文\n");
        config_free(&config);
        context_manager_free(&mgr);
        return 1;
    }

    if (nctx->auto_compress && nctx->ctx.count >= nctx->compress_threshold) {
        context_auto_compress_if_needed(nctx);
    }

    context_add_message(&nctx->ctx, "user", prompt, NULL);

    ApiProvider provider = api_provider_from_string(config.provider);

    printf("\n=== 请求信息 ===\n");
    printf("提供商: %s\n", config.provider);
    printf("模型: %s\n", config.model);
    printf("上下文: %s (%d 条消息)\n", nctx->name, nctx->ctx.count);
    if (!config.params.use_default) {
        printf("Temperature: %.2f\n", config.params.temperature);
        printf("Top_p: %.2f\n", config.params.top_p);
        printf("Max tokens: %d\n", config.params.max_tokens);
    }
    printf("================\n\n");

    int result = send_api_request(provider, config.url, config.api_key, config.model,
                                   &nctx->ctx, config.tools, config.tool_choice, &config.params);

    if (result == 0 && nctx->filename) {
        if (context_save(&nctx->ctx, nctx->filename)) {
            printf("\n上下文已保存到: %s\n", nctx->filename);
        }
    }

    config_free(&config);
    context_manager_free(&mgr);
    return result;
}

static int cmd_raw(int argc, char *argv[]) {
    if (argc < 6) return 1;

    ApiConfig config;
    memset(&config, 0, sizeof(ApiConfig));

    config.provider = strdup(argv[1]);
    config.url = strdup(argv[2]);
    config.api_key = strdup(argv[3]);
    config.model = strdup(argv[4]);

    if (!config.provider || !config.url || !config.api_key || !config.model) {
        config_free(&config);
        fprintf(stderr, "内存分配失败\n");
        return 1;
    }

    const char *prompt = argv[5];

    config_init_default_params(&config.params);
    Context ctx;
    context_init(&ctx, 10);
    context_add_message(&ctx, "user", prompt, NULL);

    ApiProvider provider = api_provider_from_string(config.provider);

    int result = send_api_request(provider, config.url, config.api_key, config.model,
                                   &ctx, NULL, NULL, &config.params);

    context_free(&ctx);
    config_free(&config);
    return result;
}

int cli_run(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    const char *command = argv[1];

    if (strcmp(command, "--spawnworld") == 0) return cmd_spawnworld(argc, argv);
    if (strcmp(command, "--context-create") == 0) return cmd_context_create(argc, argv);
    if (strcmp(command, "--context-list") == 0) return cmd_context_list(argc, argv);
    if (strcmp(command, "--context-switch") == 0) return cmd_context_switch(argc, argv);
    if (strcmp(command, "--context-delete") == 0) return cmd_context_delete(argc, argv);
    if (strcmp(command, "--context-compress") == 0) return cmd_context_compress(argc, argv);
    if (strcmp(command, "--context-stats") == 0) return cmd_context_stats(argc, argv);
    if (strcmp(command, "--context-rename") == 0) return cmd_context_rename(argc, argv);
    if (strcmp(command, "--version") == 0) {
        printf("WordGame-AI 版本: %s\n", platform_get_version());
        return 0;
    }
    if (strcmp(command, "--config") == 0) return cmd_config(argc, argv);

    if (argc == 6) return cmd_raw(argc, argv);

    print_usage(argv[0]);
    return 1;
}
