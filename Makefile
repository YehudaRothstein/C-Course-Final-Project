CC = gcc
CFLAGS = -Wall -Iutils -Istructures -Ifirst-run -Ierror-handler -Icode_conversion -Imemory_map
OBJDIR = build
BINDIR = bin

SRCS = assembler.c \
memory_map/memory_map_data.c \
memory_map/memory_map.c \
pre-assembler/pre-assembler.c \
pre-assembler/spread-macros.c \
utils/utils.c \
utils/base4.c \
error-handler/error-handler.c \
code_conversion/code_conversion.c \
first-run/first-run.c \
first-run/data_conv.c \
first-run/second-run.c \
structures/label_table.c \
structures/opcode.c \
structures/other_table.c \
first-run/modular_helpers.c \
first-run/memory_map_print.c \
first-run/output_writer.c

# Header-only file, no .c for data_directive.h

OBJS = $(patsubst %.c,$(OBJDIR)/%.o,$(SRCS))

TARGET = $(BINDIR)/assembler

TOOLS = tools/decode_ob

all: create_dirs $(TARGET) $(TOOLS)

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

$(OBJDIR)/first-run/modular_helpers.o: first-run/modular_helpers.c
	@mkdir -p $(OBJDIR)/first-run
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/first-run/memory_map_print.o: first-run/memory_map_print.c
	@mkdir -p $(OBJDIR)/first-run
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/first-run/output_writer.o: first-run/output_writer.c
	@mkdir -p $(OBJDIR)/first-run
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/structures/%.o: structures/%.c
	@mkdir -p $(OBJDIR)/structures
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/code_conversion/%.o: code_conversion/%.c
	@mkdir -p $(OBJDIR)/code_conversion
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/tools/%.o: tools/%.c
	@mkdir -p $(OBJDIR)/tools
	$(CC) $(CFLAGS) -c $< -o $@

$(BINDIR)/decode_ob: $(OBJDIR)/tools/decode_ob.o
	$(CC) $(CFLAGS) $^ -o $@

create_dirs:
	@mkdir -p $(OBJDIR) $(OBJDIR)/pre-assembler $(OBJDIR)/utils $(OBJDIR)/memory_map $(OBJDIR)/error-handler $(OBJDIR)/first-run $(OBJDIR)/structures $(BINDIR)

clean:
	rm -rf $(OBJDIR) $(BINDIR)

.PHONY: all clean create_dirs
