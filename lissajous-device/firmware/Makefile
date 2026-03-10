#******************************************************************************
# Makefile for MSP430G2553 Lissajous Frequency Multiplier
# 2026电子信息杯 - 李萨如图形发生器倍频模块
#
# 编译环境: Texas Instruments MSP430 GCC
# 目标芯片: MSP430G2553 
# 
# 使用方法:
#   make            - 编译项目
#   make clean      - 清理编译文件  
#   make flash      - 编译并下载到MCU
#   make size       - 显示代码大小统计
#******************************************************************************

# 项目配置
PROJECT_NAME = lissajous_frequency_multiplier
TARGET_MCU = msp430g2553

# 工具链配置 (TI MSP430 GCC)
CC = msp430-elf-gcc
LD = msp430-elf-gcc
OBJCOPY = msp430-elf-objcopy
OBJDUMP = msp430-elf-objdump
SIZE = msp430-elf-size

# 编译器标志
CFLAGS = -mmcu=$(TARGET_MCU) \
         -Wall \
         -Wextra \
         -Werror \
         -std=c99 \
         -O2 \
         -g \
         -ffunction-sections \
         -fdata-sections

# 链接器标志  
LDFLAGS = -mmcu=$(TARGET_MCU) \
          -Wl,--gc-sections \
          -Wl,--print-memory-usage

# 头文件路径
INCLUDE_DIRS = -I. \
               -I$(TI_MSP430_INCLUDES)

# 源文件
SOURCES = main.c \
          frequency_multiplier.c

# 生成的目标文件
OBJECTS = $(SOURCES:.c=.o)

# 输出文件
ELF_FILE = $(PROJECT_NAME).elf
HEX_FILE = $(PROJECT_NAME).hex
MAP_FILE = $(PROJECT_NAME).map

# 默认目标
all: $(ELF_FILE) $(HEX_FILE) size

# 编译规则
%.o: %.c
	@echo "编译: $<"
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -c $< -o $@

# 链接规则
$(ELF_FILE): $(OBJECTS)
	@echo "链接: $@"
	$(LD) $(LDFLAGS) -Wl,-Map=$(MAP_FILE) $(OBJECTS) -o $@

# 生成HEX文件
$(HEX_FILE): $(ELF_FILE)
	@echo "生成HEX: $@"
	$(OBJCOPY) -O ihex $< $@

# 显示代码大小
size: $(ELF_FILE)
	@echo "代码大小统计:"
	$(SIZE) -B $<
	@echo ""
	@echo "内存使用情况:"
	$(SIZE) -A $<

# 反汇编 (调试用)
disasm: $(ELF_FILE)
	$(OBJDUMP) -D $< > $(PROJECT_NAME).lst

# 清理编译文件
clean:
	@echo "清理编译文件..."
	rm -f $(OBJECTS) $(ELF_FILE) $(HEX_FILE) $(MAP_FILE) *.lst

# 下载程序 (需要mspdebug)
flash: $(HEX_FILE)
	@echo "下载程序到MSP430G2553..."
	mspdebug rf2500 "prog $(HEX_FILE)"

# CCS项目导入 (可选)
ccs-import:
	@echo "导入到Code Composer Studio..."
	@echo "请在CCS中选择: File -> Import -> Existing CCS Eclipse Projects"
	@echo "然后选择当前目录: $(shell pwd)"

# 依赖关系检查
dep-check:
	@echo "检查依赖工具..."
	@which $(CC) > /dev/null || (echo "错误: 未找到 $(CC)" && false)
	@which mspdebug > /dev/null || (echo "警告: 未找到 mspdebug (下载工具)")
	@echo "依赖检查完成"

# 项目信息
info:
	@echo "项目信息:"
	@echo "  项目名称: $(PROJECT_NAME)"
	@echo "  目标芯片: $(TARGET_MCU)"
	@echo "  编译器:   $(CC)"
	@echo "  源文件:   $(SOURCES)"
	@echo "  编译标志: $(CFLAGS)"

# 帮助信息
help:
	@echo "李萨如图形发生器倍频模块 - Makefile"
	@echo ""
	@echo "可用目标:"
	@echo "  all       - 编译项目 (默认)"
	@echo "  clean     - 清理编译文件"
	@echo "  flash     - 编译并下载到MCU"
	@echo "  size      - 显示代码大小统计"
	@echo "  disasm    - 生成反汇编文件"
	@echo "  dep-check - 检查依赖工具"
	@echo "  info      - 显示项目信息"
	@echo "  help      - 显示此帮助信息"
	@echo ""
	@echo "编译环境要求:"
	@echo "  - TI MSP430 GCC工具链"
	@echo "  - mspdebug (用于程序下载)"
	@echo ""
	@echo "使用示例:"
	@echo "  make              # 编译项目"
	@echo "  make clean all    # 清理后重新编译"
	@echo "  make flash        # 编译并下载"

# 伪目标声明
.PHONY: all clean flash size disasm dep-check info help ccs-import

#******************************************************************************
# 项目特定配置
#******************************************************************************

# 调试配置
DEBUG ?= 1
ifeq ($(DEBUG), 1)
    CFLAGS += -DDEBUG_MODE=1 -g3
    LDFLAGS += -g
else
    CFLAGS += -DNDEBUG -Os
    LDFLAGS += -s
endif

# 优化等级
OPTIMIZATION ?= 2
CFLAGS += -O$(OPTIMIZATION)

# 芯片特定配置
ifeq ($(TARGET_MCU), msp430g2553)
    CFLAGS += -DMCU_MSP430G2553=1
    # Flash: 16KB, RAM: 512B, 20-pin PDIP
    MEMORY_CHECK = true
endif

# 内存检查
ifeq ($(MEMORY_CHECK), true)
memory-check: $(ELF_FILE)
	@echo "检查内存使用情况..."
	@$(SIZE) -B $< | awk 'NR==2 {if($$1>16384) print "警告: Flash使用超过16KB限制"; if($$2>512) print "警告: RAM使用超过512B限制"}'
endif

#******************************************************************************
# 版本控制和发布
#******************************************************************************

VERSION = 2.3
BUILD_DATE = $(shell date +"%Y%m%d")
GIT_HASH = $(shell git rev-parse --short HEAD 2>/dev/null || echo "unknown")

version-info:
	@echo "版本信息:"
	@echo "  版本号:    $(VERSION)" 
	@echo "  构建日期:  $(BUILD_DATE)"
	@echo "  Git哈希:   $(GIT_HASH)"

# 在编译时注入版本信息
CFLAGS += -DVERSION_MAJOR=2 -DVERSION_MINOR=3 -DBUILD_DATE=\"$(BUILD_DATE)\" -DGIT_HASH=\"$(GIT_HASH)\"