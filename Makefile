CC = gcc
CFLAGS = -Wall -Iutils -Istructures -Ifirst-run -Ierror-handler -Icode_conversion
OBJDIR = build
BINDIR = bin

SRCS = assembler.c \
pre-assembler/pre-assembler.c \
pre-assembler/spread-macros.c \
utils/utils.c \
error-handler/error-handler.c \
code_conversion/code_conversion.c \
first-run/first-run.c \
first-run/data_conv.c \
structures/label_table.c \
structures/opcode.c \
structures/other_table.c

OBJS = $(patsubst %.c,$(OBJDIR)/%.o,$(SRCS))

TARGET = $(BINDIR)/assembler

all: create_dirs $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@

$(OBJDIR)/%.o: %.c
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/pre-assembler/%.o: pre-assembler/%.c
	@mkdir -p $(OBJDIR)/pre-assembler
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/utils/%.o: utils/%.c
	@mkdir -p $(OBJDIR)/utils
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/error-handler/%.o: error-handler/%.c
	@mkdir -p $(OBJDIR)/error-handler
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/first-run/%.o: first-run/%.c
	@mkdir -p $(OBJDIR)/first-run
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/structures/%.o: structures/%.c
	@mkdir -p $(OBJDIR)/structures
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/code_conversion/%.o: code_conversion/%.c
	@mkdir -p $(OBJDIR)/code_conversion
	$(CC) $(CFLAGS) -c $< -o $@



create_dirs:
	@mkdir -p $(OBJDIR) $(OBJDIR)/pre-assembler $(OBJDIR)/utils $(OBJDIR)/error-handler $(OBJDIR)/first-run $(OBJDIR)/structures $(BINDIR)

clean:
	rm -rf $(OBJDIR) $(BINDIR)

.PHONY: all clean create_dirs
