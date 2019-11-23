ifeq ($(OS),Windows_NT)
	SHELL := cmd.exe
	MAKE_DIR := $(shell cd)
	CC = gcc
else
	MAKE_DIR := $(shell pwd)
	CC = clang
endif

TARGET_COMPILER_FLAGS := -w -std=c99
EXEC_NAME := astera

ifeq ($(OS),Windows_NT)
	RM_CMD := -del
	TARGET_LINKER_FLAGS := -I$(MAKE_DIR)\dep\ -I$(MAKE_DIR)\dep\glfw\include -I$(MAKE_DIR)\dep\openal-soft\include\ -L$(MAKE_DIR)\dep\glfw\src -L$(MAKE_DIR)\dep\openal-soft\ -lopengl32 -lglfw3 -lgdi32 -lm -lopenal32
	TARGET_EXEC_NAME := $(EXEC_NAME)
	TARGET_COMPILER_FLAGS += -D WIN32
else
	UNAME_S := $(shell uname -s)
	RM_CMD := -rm
	TARGET_EXEC_NAME := $(EXEC_NAME)
	TARGET_LINKER_FLAGS := -I$(MAKE_DIR)/dep/ -lglfw -lGL -lGLU -lX11 -lXi -lXrandr -lXxf86vm -lXinerama -lpthread -ldl -lXcursor -lm -lopenal -lzip
	
	ifeq ($(UNAME_S),Linux)
		TARGET_COMPILER_FLAGS += -D LINUX
	else ifeq ($(UNAME_S),Darwin)
		TARGET_LINKER_FLAGS := -I$(MAKE_DIR)/dep/ -I/usr/local/include/ -L/usr/local/lib -lglfw -framework Cocoa -framework OpenGL -framework IOKit -framework CoreFoundation -framework CoreVideo -lopenal -lm -lzip -stdlib=libc++
		TARGET_COMPILER_FLAGS += -D OSX
	else ifeq ($(UNAME_S),FreeBSD)
		TARGET_COMPILER_FLAGS += -D FreeBSD
	else ifeq ($(UNAME_S),NetBSD)
		TARGET_COMPILER_FLAGS += NetBSD
	else ifeq ($(UNAME_S),OpenBSD)
		TARGET_COMPILER_FLAGS += OpenBSD
	endif
endif

all : $(OBJS)
	$(CC) $(wildcard src/*.c) $(TARGET_COMPILER_FLAGS) $(TARGET_LINKER_FLAGS) -o $(TARGET_EXEC_NAME)	

PHONY: clean
clean :
	$(RM_CMD) $(TARGET_EXEC_NAME)
