CC = gcc
CFLAGS = -Wall -g $(CURL_CFLAGS) $(CJSON_CFLAGS)
LDFLAGS = $(CURL_LDFLAGS) $(CJSON_LDFLAGS) -lws2_32

TARGET = api_client
BUILD_DIR = build

SRCS = main.c \
       cli.c \
       platform.c \
       config.c \
       context.c \
       api_client.c \
       api_types.c \
       context_manager.c \
       spawnlore.c \
       ai_settings.c \
       utils.c \
       response_parser.c \
       book_storage.c \
       game_settings.c \
       game_menu.c \
       model_list.c \
       game_state.c \
       game_ai.c \
       $(CJSON_SRC)

OBJS = $(SRCS:.c=.o)

TARGET_EXE = $(BUILD_DIR)/$(TARGET).exe

.PHONY: all clean check-env

check-env:
	@echo Checking environment variables...
	@if not defined CURL_PATH (echo ERROR: CURL_PATH is not set! && exit /b 1)
	@if not defined CJSON_PATH (echo ERROR: CJSON_PATH is not set! && exit /b 1)
	@echo CURL_PATH: $(CURL_PATH)
	@echo CJSON_PATH: $(CJSON_PATH)

all: check-env $(BUILD_DIR) $(TARGET_EXE)

$(BUILD_DIR):
	-if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)

$(TARGET_EXE): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	-del /f /q *.o 2>nul
	-if exist $(BUILD_DIR) rmdir /s /q $(BUILD_DIR)
