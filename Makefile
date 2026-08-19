### variables ###

# gcc wiki-trace.c view.c fetcher.c tracer.c cJSON.c stmr.c hash_table.c utils.c -lcurl -lncurses -lpthread

# vars for easy change
CC = gcc
CFLAGS = -Wall -Wextra -Iinclude -g -lcurl -lncurses -lpthread
TARGET = wiki-trace

# important directories
SRC_DIR = src
BUILD_DIR = build

# find all .c files
SRC_FILES = $(shell find $(SRC_DIR) -name "*.c")

# pattern match and subsitute (patsubst) all .c file paths to .o file paths
OBJ_FILES = $(patsubst $(SRC_DIR)%.c, $(BUILD_DIR)/%.o, $(SRC_FILES))


### building ###

# defualt target, runs with just 'make' command
all: $(TARGET)

# linking, combines object files into exe
$(TARGET): $(OBJ_FILES)
	$(CC) $(CFLAGS) $(OBJ_FILES) -o $(TARGET)

# how to make the object files
# compiles any matching *.c file into its respective BUILD_DIR/*.o file
$(BUILD_DIR)/%.o: $(SRC_DIR)%.c
	# make respective directories in BUILD_DIR if they dont exist
	@mkdir -p $(dir $@)
	# $@ = target of this command, $< dependency of this command
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

.PHONY: all clean

