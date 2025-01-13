# 可执行文件名
PROG = PSTest

# 设置编译器
CC = gcc

# 文件目录
OBJDIR = build
INCDIR = include
LIBDIR = lib
SRCDIR = src

# 源文件列表（所有.c文件）
SRCS = $(wildcard $(SRCDIR)/*.c)

# 对象文件列表（从源文件列表生成）
OBJS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRCS))

# 可执行文件路径 
TARGET = $(PROG)

# 需要链接的库
LIBS = -L$(LIBDIR) -L/usr/lib/mysql -lmysqlclient

# 头文件
INCS = -I$(INCDIR) -I/usr/include/mysql

# 编译选项
CFLAGS = -Wall -g -DLOG_USE_COLOR

# 默认目标
all: $(TARGET)

# 编译规则：从.c文件生成.o文件
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) -c $< -o $@ $(INCS) $(CFLAGS)

# 链接规则：从.o文件生成可执行文件，并链接库
$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(LIBS) -o $@

# 清理编译生成的文件
clean:
	rm -f $(OBJDIR)/*.o $(TARGET)

# 确保编译生成的可执行文件和对象文件目录存在
$(shell mkdir -p $(OBJDIR) || true)

# 依赖关系，确保在编译之前对象文件目录存在
$(OBJS): | $(OBJDIR)
