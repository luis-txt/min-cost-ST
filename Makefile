CFLAGS=-std=c99 -g -Wall -Wpedantic -O2 -D_POSIX_C_SOURCE=200809L -fopenmp
LDFLAGS=-lglpk -fopenmp # Including OpenMP and GLPK

SRC_DIR = src
OBJ_DIR = obj

# Create obj-file-structure if not exists.
$(shell [ ! $(OBJ_DIR) ] && mkdir $(OBJ_DIR)) # Create obj folder if it does not exist.
# Create subfolders in obj.
$(shell find $(SRC_DIR) -mindepth 1 -type d | sed 's/^src\///g' | xargs -I{} mkdir -p $(OBJ_DIR)/{})

SRCS = $(shell find $(SRC_DIR) -name '*.c')
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))
EXEC = min-cost-ST

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(OBJ_DIR)/*.o $(EXEC)

.PHONY: clean
