/* C-Course Final Project - Assembler (authored by Yehuda) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "first-run.h"
#include "../utils/utils.h"
#include "../error-handler/error-handler.h"
#include "../structures/label_table.h"
#include "../structures/other_table.h"
#include "../code_conversion/code_conversion.h"
#include "../first-pass/emit/modular_helpers.h"
#include "../first-pass/data/process_data_directives.h"
#include "../first-pass/output/output_writer.h"
#include "../first-pass/print/memory_map_print.h"
#include "../second-run/second-run.h"
#include "../memory_map/memory_map.h"

static int is_data_directive_opcode(const char *opcode) {
    return (strcmp(opcode, ".data") == 0 || strcmp(opcode, ".string") == 0 || strcmp(opcode, ".mat") == 0);
}

static int ensure_mat_adjacency(const char *originalLine,
                                const char *fileName,
                                int lineNumber) {
    if (strstr(originalLine, ".mat[") == NULL) {
        error_report_ex(ERR_SEV_ERROR, ERR_MAT_SIZE_INVALID, fileName, lineNumber,
                        "'.mat' must be followed immediately by '['");
        return 0;
    }
    return 1;
}

static void copy_inst_to_data_directive(DataDirective *dest,
                                        const InstParts *instParts,
                                        int lineNumber) {
    memset(dest, 0, sizeof(*dest));

    strncpy(dest->type, instParts->opcode, sizeof(dest->type) - 1);

    if (instParts->label) {
        strncpy(dest->label, instParts->label, sizeof(dest->label) - 1);
    }

    if (instParts->operands) {
        strncpy(dest->operands, instParts->operands, sizeof(dest->operands) - 1);
    }

    dest->src_line = lineNumber;
}

static int grow_directives_if_needed(DataDirective ***directives,
                                     int *directivesCapacity,
                                     int *directivesCount) {
    DataDirective *tmp;
    int newCap;

    if (*directivesCount < *directivesCapacity) {
        return 1;
    }

    newCap = *directivesCapacity ? (*directivesCapacity * 2) : 16;

    tmp = (DataDirective *)realloc(**directives, sizeof(DataDirective) * newCap);
    if (!tmp) {
        return 0;
    }

    **directives = tmp;
    *directivesCapacity = newCap;

    return 1;
}

static int ensure_code_capacity(code_conv_t ***codeBuffer,
                                int *codeCapacity,
                                int *errorFound) {
    code_conv_t *tmp;
    int newCap;

    if ((**codeBuffer) && *codeCapacity > 0) {
        return 1;
    }

    newCap = *codeCapacity ? (*codeCapacity * 2) : 64;

    tmp = (code_conv_t *)realloc(**codeBuffer, sizeof(code_conv_t) * newCap);
    if (!tmp) {
        *errorFound = 1;
        return 0;
    }

    **codeBuffer = tmp;
    *codeCapacity = newCap;

    return 1;
}

static void count_words_for_directive(const DataDirective *directive,
                                      int *requiredWords) {
    if (strcmp(directive->type, ".data") == 0) {
        const char *p = directive->operands;
        int count = 0;
        while (*p) {
            char *end;
            /* skip spaces and commas */
            while (*p && isspace((unsigned char)*p)) p++;
            if (*p == ',') { p++; continue; }
            while (*p && isspace((unsigned char)*p)) p++;
            if (!*p) break;
            (void)strtol(p, &end, 10);
            if (end != p) {
                count++;
                p = end;
            } else {
                break;
            }
            while (*p && isspace((unsigned char)*p)) p++;
            if (*p == ',') p++;
        }
        if (count > 0) {
            *requiredWords += count;
        }
    } else if (strcmp(directive->type, ".string") == 0) {
        const char *p = directive->operands;
        int len = 0;
        while (*p && *p != '"') p++;
        if (*p == '"') {
            p++;
            while (*p && *p != '"') { len++; p++; }
            /* include null terminator if closing quote exists */
            if (*p == '"') len++;
        }
        if (len > 0) {
            *requiredWords += len;
        }
    } else if (strcmp(directive->type, ".mat") == 0) {
        const char *p = directive->operands;
        int rows = 0;
        int cols = 0;

        while (*p && *p != '[') {
            p++;
        }

        if (*p == '[') {
            p++;
            rows = (int)strtol(p, (char **)&p, 10);

            while (*p && *p != '[') {
                p++;
            }

            if (*p == '[') {
                p++;
                cols = (int)strtol(p, (char **)&p, 10);
            }
        }

        if (rows > 0 && cols > 0) {
            *requiredWords += rows * cols;
        }
    }
}

static int allocate_data_image_if_needed(int requiredWords,
                                         const char *fileName,
                                         data_word **dataImage,
                                         int *errorFound) {
    data_word *img;
    int j;

    if (requiredWords <= 0) {
        return 1;
    }

    img = (data_word *)malloc(sizeof(data_word) * requiredWords);
    if (!img) {
        error_report_ex(ERR_SEV_ERROR, ERR_OUT_OF_MEMORY, fileName, 0, "data image");
        *errorFound = 1;
        return 0;
    }

    for (j = 0; j < requiredWords; j++) {
        img[j].value = 0;
        img[j].src_line = 0;
    }

    *dataImage = img;

    return 1;
}

static void dispatch_instruction_or_directive(const char *originalLine,
                                              InstParts *inst,
                                              DataDirective **directives, int *directivesCount, int *directivesCapacity,
                                              LabelNode **labelTable,
                                              other_table **entryTable, int *entryCount,
                                              other_table **externalTable, int *externalCount,
                                              code_conv_t **codeBuffer, int *codeCount, int *codeCapacity,
                                              const char *fileName, int lineNumber,
                                              int *errorFound) {
    if (isInstr(inst->opcode)) {
        /* Directive path */
        if (strcmp(inst->opcode, ".extern") == 0) {
            add_to_other_table(externalTable, externalCount, inst->operands);

            if (inst->operands && *inst->operands) {
                int lerr = 0;
                LabelNode *existing;

                if (!legal_label_decl(inst->operands, &lerr)) {
                    print_external_error(lerr, fileName, lineNumber);
                    *errorFound = 1;
                } else {
                    existing = find_label(*labelTable, inst->operands);

                    if (existing) {
                        if (!existing->is_extern) {
                            error_report_ex(ERR_SEV_ERROR, ERR_EXTERN_LOCAL_CONFLICT, fileName, lineNumber, inst->operands);
                            *errorFound = 1;
                        } else {
                            error_report_ex(ERR_SEV_WARNING, ERR_EXTERN_DUPLICATE, fileName, lineNumber, inst->operands);
                        }
                    } else {
                        insert_label(labelTable, inst->operands, 0, 0, 0, 1);
                    }
                }
            }

            return;
        }

        if (strcmp(inst->opcode, ".entry") == 0) {
            add_to_other_table(entryTable, entryCount, inst->operands);
            return;
        }

        if (is_data_directive_opcode(inst->opcode)) {
            if (strcmp(inst->opcode, ".mat") == 0) {
                if (!ensure_mat_adjacency(originalLine, fileName, lineNumber)) {
                    return; /* error already reported */
                }
            }

            if (!grow_directives_if_needed(&directives, directivesCapacity, directivesCount)) {
                return; /* OOM, will be reported later when trying to use */
            }

            copy_inst_to_data_directive(&(*directives)[(*directivesCount)++], inst, lineNumber);
            return;
        }

        /* Unknown directive, nothing to do here */
        return;
    } else {
        /* Instruction path */
        {
            int wordsEmitted;

            if (*codeCount >= *codeCapacity) {
                int newCap = *codeCapacity ? *codeCapacity * 2 : 64;
                code_conv_t *tmp = (code_conv_t *)realloc(*codeBuffer, sizeof(code_conv_t) * newCap);

                if (!tmp) {
                    *errorFound = 1;
                    return;
                }

                *codeBuffer = tmp;
                *codeCapacity = newCap;
            }

            wordsEmitted = emit_instruction(inst, *codeBuffer, *codeCount, lineNumber, labelTable, errorFound, fileName);

            if (wordsEmitted < 0) {
                *errorFound = 1;
                return;
            }

            /* Cap absolute code addresses at 255: IC starts at 100 */
            if ((100 + *codeCount + wordsEmitted) > MEMORY_SIZE) {
                error_report_ex(ERR_SEV_ERROR, ERR_MEMORY_OVERFLOW, fileName, lineNumber, "code exceeds memory size");
                *errorFound = 1;
                return; /* do not increase codeCount */
            }

            *codeCount += wordsEmitted;
        }
    }
}

/* --- Original helpers kept, reformatted and simplified where helpful --- */
static int handle_directive(const char *originalLine,
                            InstParts *instParts,
                            DataDirective **directives, int *directivesCount, int *directivesCapacity,
                            LabelNode **labelTable,
                            other_table **entryTable, int *entryCount,
                            other_table **externalTable, int *externalCount,
                            const char *fileName, int lineNumber, int *errorFound) {
    /* Redirect to the unified dispatcher for directives */
    dispatch_instruction_or_directive(originalLine,
                                      instParts,
                                      directives, directivesCount, directivesCapacity,
                                      labelTable,
                                      entryTable, entryCount,
                                      externalTable, externalCount,
                                      NULL, NULL, NULL,
                                      fileName, lineNumber,
                                      errorFound);
    return 1;
}

static void handle_instruction_line(const InstParts *instParts,
                                    code_conv_t **codeBuffer, int *codeCount, int *codeCapacity,
                                    LabelNode **labelTable,
                                    int *errorFound,
                                    const char *fileName, int lineNumber) {
    /* Use the same dispatcher (it branches on opcode type) */
    {
        InstParts temp = *instParts;
        dispatch_instruction_or_directive("",
                                          &temp,
                                          NULL, NULL, NULL,
                                          labelTable,
                                          NULL, NULL,
                                          NULL, NULL,
                                          codeBuffer, codeCount, codeCapacity,
                                          fileName, lineNumber,
                                          errorFound);
    }
}

static int compute_required_and_allocate(DataDirective *directives, int directivesCount,
                                         int dataBaseAddress,
                                         const char *fileName,
                                         data_word **dataImage, int *errorFound) {
    int requiredWords = 0;
    int i;

    for (i = 0; i < directivesCount; i++) {
        count_words_for_directive(&directives[i], &requiredWords);
    }

    /* Absolute address cap (MEMORY_SIZE - 1) */
    if (dataBaseAddress >= MEMORY_SIZE || (dataBaseAddress + requiredWords) > MEMORY_SIZE) {
        error_report_ex(ERR_SEV_ERROR, ERR_MEMORY_OVERFLOW, fileName, 0, "program exceeds memory size");
        *errorFound = 1;
        return 0;
    }

    if (!allocate_data_image_if_needed(requiredWords, fileName, dataImage, errorFound)) {
        return 0;
    }

    return requiredWords;
}

static void finalize_outputs(const char *fileName,
                             code_conv_t *codeBuffer, int codeCount,
                             data_word *dataImage, int dataCount,
                             LabelNode *labelTable,
                             other_table *entryTable, int entryCount,
                             other_table *externalTable, int externalCount) {
    char baseName[128];

    get_basefile(fileName, baseName, sizeof(baseName));

    exe_second_pass(codeBuffer, codeCount, labelTable, entryTable, entryCount,
                    externalTable, externalCount, baseName);

    write_code_file(baseName, codeBuffer, codeCount, dataImage, dataCount);

    print_memory_map(codeCount, codeBuffer, dataCount, dataImage, labelTable);

    print_symbol_table(labelTable);
}

static void process_single_line(const char *file_name,
                                char *lineBuffer,
                                int lineNumber,
                                DataDirective **directives, int *directivesCount, int *directivesCapacity,
                                LabelNode **labelTable,
                                other_table **entryTable, int *entryCount,
                                other_table **externalTable, int *externalCount,
                                code_conv_t **codeBuffer, int *codeCount, int *codeCapacity,
                                int *errorFound) {
    InstParts inst;

    inst = parseInstLine(lineBuffer);

    if (!inst.opcode) {
        freeInstParts(&inst);
        return;
    }

    if (inst.label && *inst.label) {
        int error_code = 0;

        if (!legal_label_decl(inst.label, &error_code)) {
            char details[100];

            sprintf(details, "Illegal label '%s'", inst.label);

            error_report_ex(ERR_SEV_ERROR, ERR_LABEL_REDEFINED, file_name, lineNumber, details);
            print_external_error(error_code, file_name, lineNumber);

            freeInstParts(&inst);
            return;
        }
    }

    dispatch_instruction_or_directive(lineBuffer,
                                      &inst,
                                      directives, directivesCount, directivesCapacity,
                                      labelTable,
                                      entryTable, entryCount,
                                      externalTable, externalCount,
                                      codeBuffer, codeCount, codeCapacity,
                                      file_name, lineNumber,
                                      errorFound);

    freeInstParts(&inst);
}

static void finalize_data_phase(const char *file_name,
                                int codeCount,
                                DataDirective *directives, int directivesCount,
                                int *dataBaseAddress,
                                data_word **dataImage, int *dataCount, int *dataCounter,
                                LabelNode **labelTable,
                                int *errorFound) {
    if (directivesCount > 0) {
        *dataBaseAddress = 100 + codeCount;

        (void)compute_required_and_allocate(directives,
                                            directivesCount,
                                            *dataBaseAddress,
                                            file_name,
                                            dataImage,
                                            errorFound);

        if (!*errorFound) {
            process_data_directives(directives,
                                    directivesCount,
                                    *dataBaseAddress,
                                    dataImage,
                                    dataCount,
                                    dataCounter,
                                    labelTable);
        }
    }
}

int exe_first_pass(char *file_name) {
    FILE *inputFile;
    char lineBuffer[256];

    int lineNumber = 0;
    int errorFound = 0;

    int dataCounter = 0;

    code_conv_t *codeBuffer = NULL;
    int codeCount = 0;
    int codeCapacity = 0;

    data_word *dataImage = NULL;
    int dataCount = 0;

    LabelNode *labelTable = NULL;

    int entryCount = 0;
    int externalCount = 0;

    other_table *entryTable = NULL;
    other_table *externalTable = NULL;

    MemoryMap dataMemoryMap;

    int dataBaseAddress = 100;

    DataDirective *directives = NULL;
    int directivesCount = 0;
    int directivesCapacity = 0;

    memory_map_init(&dataMemoryMap);

    inputFile = fopen(file_name, "r");
    if (!inputFile) {
        error_report(ERR_IO_INPUT_OPEN_FAIL, file_name, 0, file_name);
        return 1;
    }

    while (fgets(lineBuffer, sizeof(lineBuffer), inputFile)) {
        int line_len;

        lineNumber++;

        line_len = (int)strlen(lineBuffer);

        if (line_len >= 1 && (lineBuffer[line_len - 1] == '\n' || lineBuffer[line_len - 1] == '\r')) {
            lineBuffer[line_len - 1] = '\0';
        }

        removeExtraSpacesStr(lineBuffer);
        removeSpacesNextToComma(lineBuffer);

        if ((int)strlen(lineBuffer) > 80) {
            error_report_ex(ERR_SEV_ERROR, ERR_LINE_TOO_LONG, file_name, lineNumber, NULL);
            continue;
        }

        if (lineBuffer[0] == '\0' || lineBuffer[0] == ';') {
            continue;
        }

        process_single_line(file_name,
                            lineBuffer,
                            lineNumber,
                            &directives, &directivesCount, &directivesCapacity,
                            &labelTable,
                            &entryTable, &entryCount,
                            &externalTable, &externalCount,
                            &codeBuffer, &codeCount, &codeCapacity,
                            &errorFound);
    }

    fclose(inputFile);

    finalize_data_phase(file_name,
                        codeCount,
                        directives, directivesCount,
                        &dataBaseAddress,
                        &dataImage, &dataCount, &dataCounter,
                        &labelTable,
                        &errorFound);

    if (!check_duplicate_labels(labelTable)) {
        errorFound = 1;
    }

    /*אם נמצאה שגיאה או שיש שגיאות נוספות*/
    if (errorFound || error_get_error_count() > 0) {
        error_report_ex(ERR_SEV_ERROR, ERR_PASS1_FAILED, file_name, 0, NULL);

        free_label_list(labelTable);
        free(codeBuffer);
        free(dataImage);
        free(entryTable);
        free(externalTable);
        free(directives);

        return 1;
    }

    finalize_outputs(file_name, codeBuffer, codeCount, dataImage, dataCount,
                     labelTable, entryTable, entryCount, externalTable, externalCount);

    free_label_list(labelTable);
    free(codeBuffer);
    free(dataImage);
    free(entryTable);
    free(externalTable);
    free(directives);

    return 0;
}

