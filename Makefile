# Makefile for DraStic RetroArch Core (aarch64)

#CC = aarch64-linux-gnu-g++
CC = g++
CXX = g++
CFLAGS = -O2 -g -fPIC -std=c++11 -fpermissive -w -DDRASTIC_LIBRETRO -Isrc
CXXFLAGS = -O2 -g -fPIC -fpermissive -w -DDRASTIC_LIBRETRO -Isrc
LDFLAGS = -shared -Wl,-z,stack-size=33554432 -Wl,--allow-multiple-definition 
LIBS = -lz -lm

TARGET = drastic_libretro.so
OBJS = src/libretro.o src/sdl_wrapper.o src/drastic.o

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

