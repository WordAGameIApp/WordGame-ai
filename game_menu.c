#include "game_menu.h"
#include "game_ai.h"
#include "game_state.h"
#include "api_client.h"
#include "book_storage.h"
#include "model_list.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MODEL_LIST_FILE "modelList.json"

void game_menu_show(void) {
    printf("\n");
    printf("╔════════════════════════════════╗\n");
    printf("║        WordGame AI 菜单        ║\n");
    printf("╠════════════════════════════════╣\n");
    printf("║  1. New Game  (新游戏)         ║\n");
    printf("║  2. Load Game (加载游戏)       ║\n");
    printf("║  3. Settings  (设置)           ║\n");
    printf("║  4. Exit      (退出)           ║\n");
    printf("╚════════════════════════════════╝\n");
    printf("\n请选择 (1-4): ");
}

MenuChoice game_menu_get_choice(void) {
    int choice;
    if (scanf("%d", &choice) != 1) {
        while (getchar() != '\n');
        return MENU_INVALID;
    }
    while (getchar() != '\n');

    switch (choice) {
        case 1: return MENU_NEW_GAME;
        case 2: return MENU_LOAD_GAME;
        case 3: return MENU_SETTINGS;
        case 4: return MENU_EXIT;
        default: return MENU_INVALID;
    }
}

void game_new_game(const GameSettings *settings) {
    printf("\n========== 新游戏 ==========\n");
    
    // 1. 输入世界名字
    char world_name[256];
    printf("请输入世界名称: ");
    if (!fgets(world_name, sizeof(world_name), stdin)) {
        printf("输入错误\n");
        return;
    }
    world_name[strcspn(world_name, "\n")] = 0;
    
    if (strlen(world_name) == 0) {
        strcpy(world_name, "未命名世界");
    }
    
    // 2. 输入世界基本设定
    printf("\n请输入世界基本设定 (例如: 奇幻魔法世界、科幻未来世界等):\n");
    char description[1024];
    if (!fgets(description, sizeof(description), stdin)) {
        printf("输入错误\n");
        return;
    }
    description[strcspn(description, "\n")] = 0;
    
    if (strlen(description) == 0) {
        strcpy(description, "一个充满魔法与冒险的奇幻世界");
    }
    
    // 初始化游戏状态
    GameState state;
    game_state_init(&state, world_name);
    
    printf("\n正在使用模型 '%s' 生成世界 '%s'...\n", settings->world_gen_model, world_name);
    
    // 3. 调用世界生成AI
    char *world_content = NULL;
    if (!ai_generate_world(settings, world_name, description, &world_content)) {
        printf("错误: 世界生成失败\n");
        game_state_free(&state);
        return;
    }
    
    printf("\n========== 世界生成完成 ==========\n");
    printf("%s\n", world_content);
    
    // 4. 保存到 book/worldname.book
    game_state_add_to_book(&state, "世界设定", world_content);
    printf("\n世界设定已保存到: %s\n", state.book_file);
    
    // 将世界设定添加到上下文
    context_add_message(&state.context, "system", 
                        "你是这个世界的剧情引导者。以下是世界的背景设定:", NULL);
    context_add_message(&state.context, "system", world_content, NULL);
    
    free(world_content);
    
    // 5. 调用主剧情生成AI生成第一段话
    printf("\n正在使用模型 '%s' 生成开场剧情...\n", settings->main_plot_model);
    
    char *opening_plot = NULL;
    const char *opening_prompt = "请为这个世界写一个引人入胜的开场场景。描述玩家角色醒来或开始冒险时的情景，让玩家感受到世界的氛围。";
    
    if (!ai_generate_plot(settings, &state, opening_prompt, &opening_plot)) {
        printf("错误: 开场剧情生成失败\n");
        game_state_free(&state);
        return;
    }
    
    printf("\n========== 故事开始 ==========\n");
    printf("%s\n", opening_plot);
    
    // 保存到 book
    game_state_add_to_book(&state, "开场剧情", opening_plot);
    free(opening_plot);
    
    // 6. 保存上下文
    game_state_save(&state);
    printf("\n游戏状态已保存到: %s\n", state.context_file);
    
    // 7. 进入游戏循环
    printf("\n========== 游戏开始 ==========\n");
    printf("提示: 输入你的行动，或输入 'save' 保存，'exit' 退出\n\n");
    
    char user_input[1024];
    while (1) {
        printf("\n你想做什么？\n> ");
        
        if (!fgets(user_input, sizeof(user_input), stdin)) {
            break;
        }
        user_input[strcspn(user_input, "\n")] = 0;
        
        if (strcmp(user_input, "exit") == 0 || strcmp(user_input, "退出") == 0) {
            printf("\n保存并退出游戏...\n");
            game_state_save(&state);
            break;
        }
        
        if (strcmp(user_input, "save") == 0 || strcmp(user_input, "保存") == 0) {
            game_state_save(&state);
            printf("游戏已保存!\n");
            continue;
        }
        
        if (strlen(user_input) == 0) {
            continue;
        }
        
        // 增加回合数
        int turn = game_state_increment_turn(&state);
        printf("\n[第 %d 回合]\n", turn);
        
        // 生成剧情
        printf("正在生成剧情...\n");
        char *plot_content = NULL;
        
        if (!ai_generate_plot(settings, &state, user_input, &plot_content)) {
            printf("错误: 剧情生成失败\n");
            continue;
        }
        
        printf("\n%s\n", plot_content);
        
        // 保存到 book
        char plot_title[256];
        snprintf(plot_title, sizeof(plot_title), "第%d回合", turn);
        game_state_add_to_book(&state, plot_title, plot_content);
        
        // 保存上下文
        game_state_save(&state);
        
        free(plot_content);
        
        // 8. 每5回合调用审查AI
        if (game_state_should_review(&state)) {
            printf("\n[系统] 正在进行剧情审查...\n");
            
            char *review_feedback = NULL;
            if (ai_review_plot(settings, &state, &review_feedback)) {
                printf("\n========== 审查反馈 ==========\n");
                printf("%s\n", review_feedback);
                
                // 检查是否需要修改
                if (strstr(review_feedback, "无需修改") == NULL && 
                    strstr(review_feedback, "剧情良好") == NULL) {
                    printf("\n[系统] 根据审查意见调整剧情...\n");
                    
                    // 将审查建议添加到上下文
                    char review_prompt[4096];
                    snprintf(review_prompt, sizeof(review_prompt),
                             "根据以下审查建议调整后续剧情:\n%s", review_feedback);
                    context_add_message(&state.context, "system", review_prompt, NULL);
                    
                    // 生成修正后的剧情
                    char *corrected_plot = NULL;
                    if (ai_generate_plot(settings, &state, 
                                         "请根据审查建议，重新描述当前场景，修正之前的问题。", 
                                         &corrected_plot)) {
                        printf("\n========== 修正后的剧情 ==========\n");
                        printf("%s\n", corrected_plot);
                        game_state_add_to_book(&state, "审查修正", corrected_plot);
                        free(corrected_plot);
                    }
                }
                
                free(review_feedback);
            }
        }
    }
    
    game_state_free(&state);
    printf("\n游戏已保存，感谢游玩!\n");
}

void game_load_game(const GameSettings *settings) {
    printf("\n========== 加载游戏 ==========\n");
    
    // 列出可用的世界
    printf("可用的存档:\n");
    // TODO: 列出 book 目录下的所有 .book 文件
    
    char world_name[256];
    printf("\n请输入要加载的世界名称: ");
    if (!fgets(world_name, sizeof(world_name), stdin)) {
        return;
    }
    world_name[strcspn(world_name, "\n")] = 0;
    
    if (strlen(world_name) == 0) {
        printf("取消加载\n");
        return;
    }
    
    // 加载游戏状态
    GameState state;
    game_state_init(&state, world_name);
    
    if (!context_load(&state.context, state.context_file)) {
        printf("错误: 找不到存档 '%s'\n", world_name);
        game_state_free(&state);
        return;
    }
    
    printf("\n成功加载世界 '%s'\n", world_name);
    printf("当前回合: %d\n", state.turn_count);
    
    // 显示最后一条剧情
    if (state.context.count > 0) {
        const Message *last_msg = &state.context.messages[state.context.count - 1];
        if (strcmp(last_msg->role, "assistant") == 0) {
            printf("\n上次剧情:\n%s\n", last_msg->content);
        }
    }
    
    // 继续游戏循环
    printf("\n========== 继续游戏 ==========\n");
    printf("提示: 输入你的行动，或输入 'save' 保存，'exit' 退出\n\n");
    
    char user_input[1024];
    while (1) {
        printf("\n你想做什么？\n> ");
        
        if (!fgets(user_input, sizeof(user_input), stdin)) {
            break;
        }
        user_input[strcspn(user_input, "\n")] = 0;
        
        if (strcmp(user_input, "exit") == 0 || strcmp(user_input, "退出") == 0) {
            printf("\n保存并退出游戏...\n");
            game_state_save(&state);
            break;
        }
        
        if (strcmp(user_input, "save") == 0 || strcmp(user_input, "保存") == 0) {
            game_state_save(&state);
            printf("游戏已保存!\n");
            continue;
        }
        
        if (strlen(user_input) == 0) {
            continue;
        }
        
        // 增加回合数
        int turn = game_state_increment_turn(&state);
        printf("\n[第 %d 回合]\n", turn);
        
        // 生成剧情
        printf("正在生成剧情...\n");
        char *plot_content = NULL;
        
        if (!ai_generate_plot(settings, &state, user_input, &plot_content)) {
            printf("错误: 剧情生成失败\n");
            continue;
        }
        
        printf("\n%s\n", plot_content);
        
        // 保存到 book
        char plot_title[256];
        snprintf(plot_title, sizeof(plot_title), "第%d回合", turn);
        game_state_add_to_book(&state, plot_title, plot_content);
        
        // 保存上下文
        game_state_save(&state);
        
        free(plot_content);
        
        // 每5回合审查
        if (game_state_should_review(&state)) {
            printf("\n[系统] 正在进行剧情审查...\n");
            
            char *review_feedback = NULL;
            if (ai_review_plot(settings, &state, &review_feedback)) {
                printf("\n========== 审查反馈 ==========\n");
                printf("%s\n", review_feedback);
                
                if (strstr(review_feedback, "无需修改") == NULL && 
                    strstr(review_feedback, "剧情良好") == NULL) {
                    printf("\n[系统] 根据审查意见调整剧情...\n");
                    
                    char review_prompt[4096];
                    snprintf(review_prompt, sizeof(review_prompt),
                             "根据以下审查建议调整后续剧情:\n%s", review_feedback);
                    context_add_message(&state.context, "system", review_prompt, NULL);
                    
                    char *corrected_plot = NULL;
                    if (ai_generate_plot(settings, &state, 
                                         "请根据审查建议，重新描述当前场景，修正之前的问题。", 
                                         &corrected_plot)) {
                        printf("\n========== 修正后的剧情 ==========\n");
                        printf("%s\n", corrected_plot);
                        game_state_add_to_book(&state, "审查修正", corrected_plot);
                        free(corrected_plot);
                    }
                }
                
                free(review_feedback);
            }
        }
    }
    
    game_state_free(&state);
    printf("\n游戏已保存，感谢游玩!\n");
}

int game_menu_run(GameSettings *settings) {
    if (!settings) return 1;

    game_settings_load(settings, "game_settings.json");

    while (1) {
        game_menu_show();
        MenuChoice choice = game_menu_get_choice();

        switch (choice) {
            case MENU_NEW_GAME:
                game_new_game(settings);
                break;

            case MENU_LOAD_GAME:
                game_load_game(settings);
                break;

            case MENU_SETTINGS:
                printf("\n========== 设置 ==========\n");
                game_settings_edit(settings);
                game_settings_save(settings, "game_settings.json");
                printf("设置已保存\n");
                break;

            case MENU_EXIT:
                printf("\n感谢游玩，再见！\n");
                return 0;

            case MENU_INVALID:
            default:
                printf("无效选择，请重新输入\n");
                break;
        }
    }
}
