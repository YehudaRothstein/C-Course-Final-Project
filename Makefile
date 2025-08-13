CC = gcc
CFLAGS = -Wall -ansi -pedantic -Isrc -Isrc/utils -Isrc/structures -Isrc/first-run -Isrc/error-handler -Isrc/code_conversion -Isrc/memory_map -Isrc/pre-assembler
OBJDIR = build
BINDIR = bin

SRCS = \
 src/main.c \
 src/pre-assembler/pre-assembler.c \
 src/pre-assembler/spread-macros.c \
 src/utils/utils.c \
 src/utils/base4.c \
 src/error-handler/error-handler.c \
 src/code_conversion/code_conversion.c \
 src/first-run/first-run.c \
 src/first-run/data_conv.c \
 src/first-run/second-run.c \
 src/first-run/modular_helpers.c \
 src/first-run/memory_map_print.c \
 src/first-run/output_writer.c \
 src/structures/label_table.c \
 src/structures/opcode.c \
 src/structures/other_table.c \
 src/memory_map/memory_map_data.c \
 src/memory_map/memory_map.c

OBJS = $(patsubst src/%.c,$(OBJDIR)/src/%.o,$(SRCS))

TARGET = $(BINDIR)/assembler

TOOLS = $(BINDIR)/decode_ob
TOOL_SRCS = src/tools/decode_ob.c
TOOL_OBJS = $(patsubst src/%.c,$(OBJDIR)/src/%.o,$(TOOL_SRCS))

all: create_dirs $(TARGET) $(TOOLS)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@

$(BINDIR)/decode_ob: $(TOOL_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

# Generic rule to compile any C file under src/ into a mirrored path under build/
$(OBJDIR)/src/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

create_dirs:
	@mkdir -p $(BINDIR) $(OBJDIR)

clean:
	rm -rf $(OBJDIR) $(BINDIR)

.PHONY: all clean create_dirs
