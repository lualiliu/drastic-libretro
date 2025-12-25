# Makefile for DraStic RetroArch Core (aarch64)

#CC = aarch64-linux-gnu-g++
CC = g++
CFLAGS = -O2 -g -fPIC -std=c++11 -fpermissive -w
LDFLAGS = -shared -Wl,-version-script=link.T
LIBS = -lz -lm

TARGET = drastic_libretro.so
OBJS = src/libretro.o src/drastic_impl.o src/interpreter.o src/spu.o

# Default target
all: $(TARGET)

# Build the core library
$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

# Compile libretro.cpp (as C++)
src/libretro.o: src/libretro.c src/libretro.h src/drastic.h
	$(CC) $(CFLAGS) -c $< -o $@

# Compile drastic_impl.c (独立实现，不依赖 drastic.cpp)
src/drastic_impl.o: src/drastic_impl.c src/drastic.h src/interpreter.h
	$(CC) $(CFLAGS) -x c -c $< -o $@

# Compile interpreter.c (CPU 解释器实现)
src/interpreter.o: src/interpreter.c src/interpreter.h src/drastic.h
	$(CC) $(CFLAGS) -x c -c $< -o $@

# Compile spu.c (SPU 音频处理实现)
src/spu.o: src/spu.c src/spu.h
	$(CC) $(CFLAGS) -x c -c $< -o $@

# 如果 drastic.cpp 可以编译，使用这个规则
# src/drastic.o: src/drastic.cpp
#	$(CC) $(CFLAGS) -c $< -o $@

# Clean build files
clean:
	rm -f $(OBJS) $(TARGET)

# Install to RetroArch cores directory (adjust path as needed)
install: $(TARGET)
	@echo "Installing to RetroArch cores directory..."
	@echo "Please copy $(TARGET) to your RetroArch cores directory"
	@echo "Example: cp $(TARGET) ~/.config/retroarch/cores/"

.PHONY: all clean install

