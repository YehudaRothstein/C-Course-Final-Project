FLAGS = -Wall -ansi -pedantic -Isrc -Isrc/utils -Isrc/structures -Isrc/first-run -Isrc/first-pass -Isrc/first-pass/data -Isrc/first-pass/emit -Isrc/first-pass/output -Isrc/first-pass/print -Isrc/second-run -Isrc/error-handler -Isrc/code_conversion -Isrc/memory_map -Isrc/pre-assembler

SRCS = \
 src/assembler.c \
 src/pre-assembler/pre-assembler.c \
 src/pre-assembler/spread-macros.c \
 src/utils/utils.c \
 src/utils/base4.c \
 src/error-handler/error-handler.c \
 src/code_conversion/code_conversion.c \
 src/first-run/first-run.c \
 src/first-pass/data/process_data_directives.c \
 src/second-run/second-run.c \
 src/first-pass/emit/modular_helpers.c \
 src/first-pass/output/output_writer.c \
 src/structures/label_table.c \
 src/structures/opcode.c \
 src/structures/other_table.c \
 src/memory_map/memory_map_data.c \
 src/memory_map/memory_map.c

all: assembler

assembler:
	gcc $(FLAGS) $(SRCS) -o assembler

clean:
	rm -f assembler

.PHONY: all clean
