# Makefile for DraStic RetroArch Core (aarch64)

#CC = aarch64-linux-gnu-g++
CC = g++
CXX = g++
CFLAGS = -O2 -g -fPIC -std=c++11 -fpermissive -w -DDRASTIC_LIBRETRO -Isrc -fvisibility=hidden
CXXFLAGS = -O2 -g -fPIC -fpermissive -w -DDRASTIC_LIBRETRO -Isrc -fvisibility=hidden
# 使用 -Bsymbolic 确保符号优先绑定到我们的库，而不是 RetroArch 加载的 SDL2
# 使用 --export-dynamic 导出我们的 SDL 存根函数，确保它们被优先使用
# 使用版本脚本确保我们的 SDL 函数被正确导出
LDFLAGS = -shared -Wl,-z,stack-size=33554432 -Wl,--allow-multiple-definition -Wl,-Bsymbolic -Wl,--export-dynamic -Wl,--version-script=sdl_symbols.lds
LIBS = -lz -lm

TARGET = drastic_libretro.so
OBJS = src/libretro_cpp.o src/sdl_wrapper.o src/drastic.o

# Default target
all: $(TARGET)

# Build the core library
$(TARGET): $(OBJS)
	$(CC) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

# Explicit compilation rules to ensure flags are used
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<


# Clean build files
clean:
	rm -f $(OBJS) $(TARGET)

# Install to RetroArch cores directory (adjust path as needed)
install: $(TARGET)
	@echo "Installing to RetroArch cores directory..."
	@echo "Please copy $(TARGET) to your RetroArch cores directory"
	@echo "Example: cp $(TARGET) ~/.config/retroarch/cores/"

.PHONY: all clean install

