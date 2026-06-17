# ============================================================================
# Makefile for vgcp-server (multi-platform, FLTK static)
#
# Build targets by machine:
#   Linux machine : make linux
#   macOS machine : make macArm  |  make macIntel  |  make win
# ============================================================================

# Compilers
CXX_LINUX   := g++
CXX_WIN     := x86_64-w64-mingw32-g++
CXX_MAC_ARM := g++
CXX_MAC_INT := g++

# Compiler flags
CXXFLAGS_COMMON := -std=c++17 -Os -fno-ident -fno-asynchronous-unwind-tables
CXXFLAGS_MAC    := -std=c++17 -Os -fno-ident -fno-asynchronous-unwind-tables
CXXFLAGS_WIN    := -std=c++17 -Os -s -ffunction-sections -fdata-sections -fno-ident \
                   -static-libgcc -static-libstdc++ -Wl,--gc-sections \
                   -D_USE_MATH_DEFINES

# Directories
SRC_DIR := src
FLTK_INC := include/fltk
BIN_DIR  := bin

MAIN_SRC := $(SRC_DIR)/vgcp_server.cpp

# FLTK lib directories (static, pre-built per platform)
LIB_LINUX     := lib/linux/fltk
LIB_MAC_ARM   := lib/macArm/fltk
LIB_MAC_INTEL := lib/macIntel/fltk
LIB_WIN       := lib/win/fltk

# Binary output directories
LINUX_DIR     := $(BIN_DIR)/linux
MAC_ARM_DIR   := $(BIN_DIR)/macArm
MAC_INTEL_DIR := $(BIN_DIR)/macIntel
WIN_DIR       := $(BIN_DIR)/win

# FLTK static libs (order: images/jpeg/png before core)
FLTK_LIBS_MAC   := -lfltk_images -lfltk_jpeg -lfltk_png -lfltk
FLTK_LIBS_LINUX := -lfltk_images -lfltk_jpeg -lfltk
FLTK_LIBS_WIN   := -lfltk_images -lfltk_jpeg -lfltk_png -lfltk_z -lfltk

# System link flags
SYS_LIBS_MAC   := -framework Cocoa -framework AppKit -framework Carbon \
                  -framework ApplicationServices -lpthread -lz
SYS_LIBS_LINUX := -lX11 -lXft -lXrender -lXext -lXfixes -lXcursor -lXinerama \
                  -lfontconfig -lfreetype -lm -lpthread
SYS_LIBS_WIN   := -lgdi32 -lole32 -luuid -lcomctl32 -lws2_32 \
                  -lcomdlg32 -lwinspool -mwindows

DIRS := $(LINUX_DIR) $(MAC_ARM_DIR) $(MAC_INTEL_DIR) $(WIN_DIR)

.PHONY: all linux macArm macIntel win clean help directories

all: directories linux macArm macIntel win

directories: $(DIRS)

$(DIRS):
	mkdir -p $@

# ============================================================================
# Linux x86_64 — compilar en máquina Linux nativa
# ============================================================================
linux: $(LINUX_DIR)/vgcp-server

$(LINUX_DIR)/vgcp-server: $(MAIN_SRC) | $(LINUX_DIR)
	$(CXX_LINUX) $(CXXFLAGS_COMMON) \
	  -I$(FLTK_INC) -I$(SRC_DIR) \
	  -L$(LIB_LINUX) \
	  $< -o $@ \
	  $(FLTK_LIBS_LINUX) $(SYS_LIBS_LINUX)

# ============================================================================
# macOS ARM (Apple Silicon) — compilar en Mac
# ============================================================================
macArm: $(MAC_ARM_DIR)/vgcp-server

$(MAC_ARM_DIR)/vgcp-server: $(MAIN_SRC) | $(MAC_ARM_DIR)
	$(CXX_MAC_ARM) $(CXXFLAGS_MAC) \
	  -I$(FLTK_INC) -I$(SRC_DIR) \
	  -L$(LIB_MAC_ARM) \
	  $< -o $@ \
	  $(FLTK_LIBS_MAC) $(SYS_LIBS_MAC)

# ============================================================================
# macOS Intel (x86_64) — compilar en Mac
# ============================================================================
macIntel: $(MAC_INTEL_DIR)/vgcp-server

$(MAC_INTEL_DIR)/vgcp-server: $(MAIN_SRC) | $(MAC_INTEL_DIR)
	$(CXX_MAC_INT) --target=x86_64-apple-darwin $(CXXFLAGS_MAC) \
	  -I$(FLTK_INC) -I$(SRC_DIR) \
	  -L$(LIB_MAC_INTEL) \
	  $< -o $@ \
	  $(FLTK_LIBS_MAC) $(SYS_LIBS_MAC)

# ============================================================================
# Windows x86_64 — cross-compilar en Mac con mingw
# ============================================================================
win: $(WIN_DIR)/vgcp-server.exe

$(WIN_DIR)/vgcp-server.exe: $(MAIN_SRC) | $(WIN_DIR)
	$(CXX_WIN) $(CXXFLAGS_WIN) \
	  -I$(FLTK_INC) -I$(SRC_DIR) \
	  -L$(LIB_WIN) \
	  $< -o $@ \
	  $(FLTK_LIBS_WIN) $(SYS_LIBS_WIN)

# ============================================================================
# CLEAN / HELP
# ============================================================================
clean:
	rm -rf $(BIN_DIR)

help:
	@echo "Targets:"
	@echo "  linux     - Compilar en máquina Linux nativa (g++)"
	@echo "  macArm    - macOS Apple Silicon  (compilar en Mac)"
	@echo "  macIntel  - macOS Intel x86_64   (compilar en Mac)"
	@echo "  win       - Windows x86_64       (cross-compilar en Mac con mingw)"
	@echo "  clean     - Eliminar bin/"
	@echo ""
	@echo "  all       - Todos los targets (requiere Linux + Mac)"

.DEFAULT_GOAL := help
