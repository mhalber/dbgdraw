# Determine system
SYS_NAME=$(shell uname -s)

# Useful Directories
BIN_DIR = bin/
OUT_DIR = out/
EXT_DIR = external/
BACKEND_NAME = opengl45/

DD_PRIM_EXE = dd_frustum_culling
DD_PRIM_SRCS = debugdraw.c examples/$(BACKEND_NAME)frustum_culling.c 
DD_PRIM_OBJS = $(OUT_DIR)debugdraw.o $(OUT_DIR)frustum_culling.o $(OUT_DIR)glad.o

# Compile and link options
CC = gcc
WARNINGS = -Wall -Wextra
DBG_FLAGS = -O0 -g
REL_FLAGS = -O3 -g0

CFLAGS = $(WARNINGS) $(REL_FLAGS) -std=c99 -I. -I../ -I${EXT_DIR} -Iexamples/$(BACKEND_NAME)
LIBS = -lopengl32 -lgdi32 -lglfw3

# Make targets
all: clean dd_frustum_culling

dd_frustum_culling: $(DD_PRIM_OBJS)
		$(CC) $(CFLAGS) $(DD_PRIM_OBJS) $(LIBS) -o ${BIN_DIR}${DD_PRIM_EXE}

clean:
		rm -f $(DD_PRIM_OBJS) ${BIN_DIR}${DD_PRIM_EXE}

# Compile command
$(OUT_DIR)%.o: %.c
		$(CC) $(CFLAGS) -c $< -o $@

$(OUT_DIR)%.o: examples/$(BACKEND_NAME)%.c
		$(CC) $(CFLAGS) -c $< -o $@

$(OUT_DIR)%.o: $(EXT_DIR)%.c
		$(CC) $(CFLAGS) -c $< -o $@

# GNU Make: targets that don't build files
.PHONY: all clean
