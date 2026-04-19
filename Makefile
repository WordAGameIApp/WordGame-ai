CC = gcc
CFLAGS = -Wall -g -I../cjson -I../curl-8.19.0_7-win64-mingw/include
LDFLAGS = -L../curl-8.19.0_7-win64-mingw/lib -lcurl -lws2_32

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
       ../cjson/cJSON.c

OBJS = $(SRCS:.c=.o)

TARGET_EXE = $(BUILD_DIR)/$(TARGET).exe

.PHONY: all clean

all: $(BUILD_DIR) $(TARGET_EXE)

$(BUILD_DIR):
	-if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)

$(TARGET_EXE): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	-del /f /q *.o 2>nul
	-if exist $(BUILD_DIR) rmdir /s /q $(BUILD_DIR)
