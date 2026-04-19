# WordGame AI

一个基于 AI 的文字冒险游戏。

## 功能特点

- 使用 LLM 生成游戏世界和剧情
- 支持多种 AI 模型（DeepSeek、OpenAI 等）
- 游戏状态自动保存
- 剧情审查和修正机制

## 依赖

本项目使用了以下第三方库：

- **libcurl** - 用于 HTTP 请求 ([MIT 许可证](https://curl.se/docs/copyright.html))
- **cJSON** - 用于 JSON 解析 ([MIT 许可证](https://github.com/DaveGamble/cJSON/blob/master/LICENSE))

## 编译

```bash
make
```

## 运行

```bash
cd build
./api_client.exe
```

## 配置

1. 复制模板文件：
```bash
cp modelList.json.template modelList.json
cp game_settings.json.template game_settings.json
```

2. 编辑 `modelList.json`，填入你的 API 密钥

3. 编辑 `game_settings.json`，配置游戏参数

## 项目结构

- `api_client.c/h` - API 客户端
- `game_menu.c/h` - 游戏菜单
- `game_settings.c/h` - 游戏设置
- `game_state.c/h` - 游戏状态管理
- `game_ai.c/h` - AI 调用封装
- `model_list.c/h` - 模型列表管理
- `prompt/` - AI 提示词文件
- `book/` - 生成的世界书（自动创建）
- `context/` - 游戏上下文存档（自动创建）

## 第三方许可证

### libcurl
Copyright (c) 1996 - 2024, Daniel Stenberg, <daniel@haxx.se>, and many contributors.
See the [curl 许可证](https://curl.se/docs/copyright.html) 获取完整信息。

### cJSON
Copyright (c) 2009-2017 Dave Gamble and cJSON contributors
Permission is hereby granted, free of charge, to any person obtaining a copy...
[完整 MIT 许可证](https://github.com/DaveGamble/cJSON/blob/master/LICENSE)

## 许可证

本项目使用 MIT 许可证。
