CC = gcc
CFLAGS = -Wall -Iutils

OBJDIR = build
BINDIR = bin

SRCS = assembler.c pre-assembler/pre-assembler.c pre-assembler/spread-macros.c utils/utils.c error-handler/error-handler.c

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

$(OBJDIR)/pre-assembler/spread-macros.o: pre-assembler/spread-macros.c
	@mkdir -p $(OBJDIR)/pre-assembler
	$(CC) $(CFLAGS) -c $< -o $@


$(OBJDIR)/error-handler/%.o: error-handler/%.c
	@mkdir -p $(OBJDIR)/error-handler
	$(CC) $(CFLAGS) -c $< -o $@

create_dirs:
	@mkdir -p $(OBJDIR) $(OBJDIR)/pre-assembler $(OBJDIR)/utils $(OBJDIR)/error-handler $(BINDIR)

clean:
	rm -rf $(OBJDIR) $(BINDIR)

.PHONY: all clean create_dirs
