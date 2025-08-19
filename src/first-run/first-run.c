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
#include "../first-pass/data/data_directive.h"
#include "../first-pass/data/process_data_directives.h"
#include "../first-pass/output/output_writer.h"
#include "../second-run/second-run.h"
#include "../memory_map/memory_map.h"

/* האם שורת הפקודה היא מסוג data */
static int is_data_directive_opcode(const char *opcode) {
    return (strcmp(opcode, ".data") == 0 || strcmp(opcode, ".string") == 0 || strcmp(opcode, ".mat") == 0);
}

/* מעתיק את חלקי הפקודה לתוך מבנה DataParts */
static void copy_inst_to_data_directive(DataParts *dest, const InstParts *instParts, int lineNumber) {
    /* מאפס את המבנה */
    memset(dest, 0, sizeof(*dest));

    /* מעתיק את סוג הפקודה */
    strncpy(dest->type, instParts->opcode, sizeof(dest->type) - 1);

    /* מעתיק את שם התווית */
    if (instParts->label) {
        strncpy(dest->label, instParts->label, sizeof(dest->label) - 1);
    }

    /* מעתיק את האופרנדים */
    if (instParts->operands) {
        strncpy(dest->operands, instParts->operands, sizeof(dest->operands) - 1);
    }

    /* מעתיק את מספר השורה המקורי */
    dest->src_line = lineNumber;
}

/* בודק אם יש צורך להגדיל את המערך הדינמי של ההנחיות Data */
static int grow_directives_if_needed(DataParts **directives, int *directivesCapacity, int *directivesCount) {
    /* אם יש מקום פנוי במערך הנוכחי, אין צורך להגדיל */
    if (*directivesCount < *directivesCapacity) {
        return 1;
    }

    /* הקצאה פשוטה: מגדילים את הקיבולת בדיוק באיבר אחד (לא היעילה ביותר, אבל פשוטה) */
    {
        int newCap = *directivesCount + 1; /* אם הקיבולת 0, newCap יהיה 1 */
        /* מגדיל את המערך */
        DataParts *tmp = (DataParts *)realloc(*directives, sizeof(DataParts) * newCap);
        /* בודק אם ההקצאה הצליחה */
        if (!tmp) {
            return 0;
        }
        *directives = tmp;
        *directivesCapacity = newCap;
        return 1;
    }
}

/* מונה מילים עבור הנחיית .data (פשוט יותר: ספרות עם סימן אופציונלי מופרדות בפסיקים/רווחים) */
static void count_words_for_data(const char *operands, int *requiredWords) {
    const char *p = operands;
    int count = 0;

    while (*p) {
        /* דלג על רווחים */
        while (*p && isspace((unsigned char)*p)) p++;
        /* דלג על פסיקים עודפים */
        if (*p == ',') { p++; continue; }

        /* סימן  */
        if (*p == '+' || *p == '-') p++;

        /* חייבת להיות לפחות ספרה אחת כדי להחשיב כמספר */
        if (isdigit((unsigned char)*p)) {
            while (isdigit((unsigned char)*p)) p++;
            count++;
            /* מדלגים על רווחים ופסיק בודד אחרי המספר */
            while (*p && isspace((unsigned char)*p)) p++;
            if (*p == ',') p++;
        } else {
            /* לא מספר- מדלגים עד הפסיק הבא או סוף מחרוזת כדי למנוע לולאה אין-סופית */
        while (*p && *p != ',') {
            p++;
        }
            if (*p == ',')
                p++;
        }
    }

    if (count > 0) {
        *requiredWords += count;
    }
}

/* מונה מילים עבור הנחיית .string (כולל תו סיום) */
static void count_words_for_string(const char *operands, int *requiredWords) {
    const char *p = operands;
    int len = 0;

    /* מחפש את התו הראשון של המחרוזת */
    while (*p && *p != '"') p++;
    /* אם מצאנו ציטוט פותח, סופרים עד ציטוט סוגר או סוף שורה; מוסיפים תמיד תו סיום NUL */
    if (*p == '"') {
        p++;

        while (*p && *p != '"') {
            len++;
            p++;
        }

        /* מוסיפים תו סיום תמיד (גם אם אין ציטוט סוגר) */
        *requiredWords += (len + 1);
    }
}

/* מונה מילים עבור הנחיית .mat (rows*cols) */
static void count_words_for_mat(const char *operands, int *requiredWords) {
    const char *p = operands;
    int rows = 0, cols = 0;

    /* קפוץ לסוגר המרובע הראשון */
    while (*p && *p != '[') p++;

    if (*p == '[') {
        /* קרא בפשטות עם sscanf: תבנית "[rows][cols]" עם רווחים אופציונליים */
        if (sscanf(p, " [ %d ] [ %d ]", &rows, &cols) == 2 && rows > 0 && cols > 0) {
            *requiredWords += rows * cols;
        }
    }
}

/* ספירת מילים להנחיה אחת לפי סוגה */
static void count_words_for_directive(const DataParts *dataPart, int *requiredWords) {
    if (strcmp(dataPart->type, ".data") == 0) {
        count_words_for_data(dataPart->operands, requiredWords);
    } else if (strcmp(dataPart->type, ".string") == 0) {
        count_words_for_string(dataPart->operands, requiredWords);
    } else if (strcmp(dataPart->type, ".mat") == 0) {
        count_words_for_mat(dataPart->operands, requiredWords);
    }
}

/* הקצאת תמונת נתונים אם יש צורך */
static int allocate_data_image_if_needed(int requiredWords, const char *fileName,
                                        data_word **dataImage, int *errorFound) {
    data_word *img;
    int j;

    if (requiredWords <= 0) {
        return 1;
    }

    /* הקצאת זיכרון לתמונת הנתונים */
    img = (data_word *)malloc(sizeof(data_word) * requiredWords);
    /* בודק אם ההקצאה הצליחה */
    if (!img) {
        error_report_ex(ERR_SEV_ERROR, ERR_OUT_OF_MEMORY, fileName, 0, "data image");
        *errorFound = 1;
        return 0;
    }

    /* מאתחל את ערכי ברירת המחדל של תמונת הנתונים */
    for (j = 0; j < requiredWords; j++) {
        img[j].value = 0;
        img[j].src_line = 0;
    }

    *dataImage = img;

    return 1;
}

/* פועל על שורה אחת של קוד או הנחיה */
static void dispatch_instruction_or_directive(const char *originalLine, InstParts *inst, DataParts **directives, int *directivesCount, int *directivesCapacity,
                                              LabelNode **labelTable, other_table **entryTable, int *entryCount,
                                              other_table **externalTable, int *externalCount, code_conv_t **codeBuffer, int *codeCount, int *codeCapacity,
                                              const char *fileName, int lineNumber, int *errorFound) {
    /* אם זו הנחיה */
    if (isInstr(inst->opcode)) {
        /* אם ההנחיה מסוג .extern */
        if (strcmp(inst->opcode, ".extern") == 0) {
            /* מוסיף את הסמל החיצוני לטבלה */
            add_to_other_table(externalTable, externalCount, inst->operands);

            /* אם יש תו אופרטור */
            if (inst->operands && *inst->operands) {
                int lerr = 0;
                LabelNode *existing;

                /* בודק אם התווית חוקית */
                if (!legal_label_decl(inst->operands, &lerr)) {
                    print_external_error(lerr, fileName, lineNumber);
                    *errorFound = 1;
                } else {
                    existing = find_label(*labelTable, inst->operands);
                    /* אם התווית כבר קיימת */
                    if (existing) {
                        /* אם התווית היא חיצונית */
                        if (!existing->is_extern) {
                            error_report_ex(ERR_SEV_ERROR, ERR_EXTERN_LOCAL_CONFLICT, fileName, lineNumber, inst->operands);
                            *errorFound = 1;
                        } else {
                            /* אם התווית היא חיצונית, מדווח אזהרה */
                            error_report_ex(ERR_SEV_WARNING, ERR_EXTERN_DUPLICATE, fileName, lineNumber, inst->operands);
                        }
                    } else {
                        /* אם התווית לא קיימת, מוסיף אותה לטבלה */
                        insert_label(labelTable, inst->operands, 0, 0, 0, 1);
                    }
                }
            }
            return;
        }

        /* אם זו הנחיה מסוג .entry */
        if (strcmp(inst->opcode, ".entry") == 0) {
            /* אימות שם תווית */
            if (!inst->operands || !*inst->operands) {
                error_report_ex(ERR_SEV_ERROR, ERR_LABEL_EMPTY, fileName, lineNumber, ".entry requires a label");
                *errorFound = 1;
                return;
            }
            {
                int lerr2 = 0;
                if (!legal_label_decl(inst->operands, &lerr2)) {
                    print_external_error(lerr2, fileName, lineNumber);
                    *errorFound = 1;
                    return;
                }
            }
            /* מוסיף את הסמל לרשימת entries */
            add_to_other_table(entryTable, entryCount, inst->operands);
            return;
        }

        /* אם זו הנחיה מסוג .data */
        if (is_data_directive_opcode(inst->opcode)) {
            /* אם זו הנחיה מסוג .data */
            if (!grow_directives_if_needed(directives, directivesCapacity, directivesCount)) {
                return; /* לא הצלחנו להרחיב את המערך */
            }

            copy_inst_to_data_directive(&(*directives)[(*directivesCount)++], inst, lineNumber); /* מעתיק את ההנחיה להנחיה מסוג .data */
            return;
        }

        /* אם זו הנחיה לא ידועה */
        return;
    } else {
        {
            int wordsEmitted;

            /* ודא שיש מספיק מקום ל-"הכי גרוע" (עד 5 מילים להוראה) כדי למנוע גלישה של הבאפר */
            {
                const int MAX_WORDS_PER_INSTRUCTION = 5; /* מילה ראשונה + עד 4 מילות-מידע */
                if ((*codeCount + MAX_WORDS_PER_INSTRUCTION) > *codeCapacity) {
                    int newCap = *codeCount + MAX_WORDS_PER_INSTRUCTION;
                    code_conv_t *tmp = (code_conv_t *)realloc(*codeBuffer, sizeof(code_conv_t) * newCap);
                    if (!tmp) {
                        *errorFound = 1;
                        return;
                    }
                    *codeBuffer = tmp;
                    *codeCapacity = newCap;
                }
            }

            /* מקודדת את ההוראה לבאפר ומציבה ב‑wordsEmitted את מספר המילים שפלטה ההרכבה. */
            wordsEmitted = encode_instruction(inst, *codeBuffer, *codeCount, lineNumber, labelTable, errorFound, fileName);

            /* בודקים אם ההנחיה פלטה שגיאה */
            if (wordsEmitted < 0) {
                *errorFound = 1;
                return;
            }

            /* מגביל את כתובות הקוד האבסולוטיות ל-255: IC מתחיל ב-100 */
            if ((100 + *codeCount + wordsEmitted) > MEMORY_SIZE) {
                error_report_ex(ERR_SEV_ERROR, ERR_MEMORY_OVERFLOW, fileName, lineNumber, "code exceeds memory size");
                *errorFound = 1;
                return; /* לא מצליח להוסיף עוד קוד */
            }

            /* מעדכן את מספר המילים בקוד */
            *codeCount += wordsEmitted;
        }
    }
}

/* מחשב את מספר המילים הנדרש ומבצע הקצאה אם צריך */
static int compute_required_and_allocate(DataParts *directives, int directivesCount,
                                         int dataBaseAddress, const char *fileName,
                                         data_word **dataImage, int *errorFound) {
    int requiredWords = 0;
    int i;

    /* סופר את מספר המילים הנדרש לכל הנחיה */
    for (i = 0; i < directivesCount; i++) {
        count_words_for_directive(&directives[i], &requiredWords);
    }

    /* מגביל את כתובת הבסיס האבסולוטית (MEMORY_SIZE - 1) */
    if (dataBaseAddress >= MEMORY_SIZE || (dataBaseAddress + requiredWords) > MEMORY_SIZE) {
        error_report_ex(ERR_SEV_ERROR, ERR_MEMORY_OVERFLOW, fileName, 0, "program exceeds memory size");
        *errorFound = 1;
        return 0;
    }

    /* בודק אם יש צורך בהקצאת תמונת נתונים */
    if (!allocate_data_image_if_needed(requiredWords, fileName, dataImage, errorFound)) {
        return 0;
    }

    /* מעדכן את מספר המילים הנדרש */
    return requiredWords;
}

/* מסיים את שלב הפלט */
static void finalize_outputs(const char *fileName,
                             code_conv_t *codeBuffer, int codeCount,
                             data_word *dataImage, int dataCount,
                             LabelNode *labelTable,
                             other_table *entryTable, int entryCount,
                             other_table *externalTable, int externalCount) {
    /* כותב את קובץ הפלט */
    char baseName[128];
    int errs_before;

    /* מקבל את שם הקובץ הבסיסי */
    get_basefile(fileName, baseName, sizeof(baseName));
    errs_before = error_get_error_count();
    /* מבצע את המעבר השני */
    exe_second_pass(codeBuffer, codeCount, labelTable, entryTable, entryCount,
                    externalTable, externalCount, baseName);
    /* אם נוספו שגיאות במעבר השני – אל תכתוב פלט */
    if (error_get_error_count() > errs_before) {
        /* נסה למחוק קבצי ent/ext שנוצרו חלקית */
        char tmp[300];
        get_basefile(baseName, tmp, sizeof(tmp));
        {
            char path[320];
            size_t cap = sizeof(path);
            path[0] = '\0';
            strncat(path, tmp, cap - 1);
            strncat(path, ".ent", cap - 1 - strlen(path));
            remove(path);
        }
        {
            char path[320];
            size_t cap = sizeof(path);
            path[0] = '\0';
            strncat(path, tmp, cap - 1);
            strncat(path, ".ext", cap - 1 - strlen(path));
            remove(path);
        }
        return;
    }

    /* כותב את קובץ הפלט */
    write_code_file(baseName, codeBuffer, codeCount, dataImage, dataCount);

    /* כותב את מפת הזיכרון */
}

/* מעבד שורה בודדת */
static void process_single_line(const char *file_name,char *lineBuffer, int lineNumber,
                                DataParts **directives, int *directivesCount, int *directivesCapacity,
                                LabelNode **labelTable,
                                other_table **entryTable, int *entryCount,
                                other_table **externalTable, int *externalCount,
                                code_conv_t **codeBuffer, int *codeCount, int *codeCapacity,
                                int *errorFound) {
    InstParts inst;

    /* מנתח את שורת הקוד */
    inst = parseInstLine(lineBuffer);

    /* בודק אם יש הוראת פעולה חוקית */
    if (!inst.opcode) {
        freeInstParts(&inst);
        return;
    }

    /* בודק אם יש תווית חוקית */
    if (inst.label && *inst.label) {
        int error_code = 0;

        if (!legal_label_decl(inst.label, &error_code)) {
            char details[100];
            {
                char *p = details; const char *h1 = "Illegal label '"; const char *s = inst.label ? inst.label : ""; const char *h2 = "'";
                while (*h1) { *p++ = *h1++; }
                while (*s && (p - details) < (int)sizeof(details) - 2) { *p++ = *s++; }
                while (*h2) { *p++ = *h2++; }
                *p = '\0';
            }

            error_report_ex(ERR_SEV_ERROR, ERR_LABEL_REDEFINED, file_name, lineNumber, details);
            print_external_error(error_code, file_name, lineNumber);

            freeInstParts(&inst);
            return;
        }
    }

    /* בודק אם יש הוראת פעולה חוקית */
    dispatch_instruction_or_directive(lineBuffer,
                                      &inst,
                                      directives, directivesCount, directivesCapacity,
                                      labelTable, entryTable, entryCount,
                                      externalTable, externalCount,
                                      codeBuffer, codeCount, codeCapacity,
                                      file_name, lineNumber,  errorFound );

    freeInstParts(&inst);
}

/* מסיים את שלב נתוני הקלט */
static void finalize_data_phase(const char *file_name,
                                int codeCount,DataParts *directives, int directivesCount,
                                int *dataBaseAddress,
                                data_word **dataImage, int *dataCount, int *dataCounter,
                                LabelNode **labelTable,
                                int *errorFound) {

    /* בודק אם יש הוראות נתונים */
    if (directivesCount > 0) {
        /* מחשב את כתובת הבסיס לנתונים */
        *dataBaseAddress = 100 + codeCount;

        /* אם כבר קיימות שגיאות, נריץ מעבר אימות בלבד כדי לאסוף שגיאות נוספות בהנחיות נתונים */
        if (*errorFound) {
            data_word *null_image = NULL; /* מצב אימות: אין כתיבה בפועל */
            int tmpDataCount = *dataCount;
            int tmpDC = *dataCounter;
            LabelNode *tmpLabels = *labelTable; /* לא יתווספו תוויות ב-validation_only */

            process_data_directives(directives,
                                    directivesCount,
                                    *dataBaseAddress,
                                    &null_image,
                                    &tmpDataCount,
                                    &tmpDC,
                                    &tmpLabels);
        } else {
            /* מקצה זיכרון לנתונים ומבצע כתיבה מלאה */
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
}

/* מתחיל את שלב הקלט ואת המעבר הראשון*/
int exe_first_pass(char *file_name) {
    /* פותח את קובץ הקלט */
    FILE *inputFile;
    char lineBuffer[256];

    /* משתנה לשמירת מספר השורה הנוכחית */
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

    int dataBaseAddress = 100;

    DataParts *directives = NULL;
    int directivesCount = 0;
    int directivesCapacity = 0;

    
    inputFile = fopen(file_name, "r");
    if (!inputFile) {
        error_report(ERR_IO_INPUT_OPEN_FAIL, file_name, 0, file_name);
        return 1;
    }

    /* כל עוד יש שורות לקלט */
    while (fgets(lineBuffer, sizeof(lineBuffer), inputFile)) {

        char *checkPtr;
        lineNumber++;

        /* מסיר את סיומי השורה (\r או \n) בפשטות */
        lineBuffer[strcspn(lineBuffer, "\r\n")] = '\0';

        /* מאפשר שורות מוזחות: בודק אחרי דילוג על רווחים/טאבים ורק אז מחליט לדלג */
        checkPtr = lineBuffer;
        while (*checkPtr == ' ' || *checkPtr == '\t') checkPtr++;
        if (*checkPtr == '\0' || *checkPtr == ';') {
            continue; /* שורה ריקה או הערה */
        }

        /* מעבד שורה בודדת */
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

    /* מסיים את שלב הנתונים */
    finalize_data_phase(file_name,
                        codeCount,
                        directives, directivesCount,
                        &dataBaseAddress,
                        &dataImage, &dataCount, &dataCounter,
                        &labelTable,
                        &errorFound);

    /*
     * אם התגלו שגיאות במעבר הראשון, עדיין נריץ מעבר שני לצורך דיאגנוסטיקה
     * (כדי לגלות תוויות לא מוגדרות וכד'), אך לא נייצר אף קובץ פלט.
     * אם אין שגיאות – נריץ את finalize_outputs שמבצע מעבר שני מלא וכתיבת פלטים.
     */
    if (error_get_error_count() > 0) {
        char baseNameDiag[128];
        get_basefile(file_name, baseNameDiag, sizeof(baseNameDiag));
        /* מעבר שני דיאגנוסטי – second-pass עצמו ימנע כתיבת ent/ext כאשר יש שגיאות קיימות */
        exe_second_pass(codeBuffer, codeCount, labelTable, entryTable, entryCount,
                        externalTable, externalCount, baseNameDiag);
    } else {
        /* לא נמצאו שגיאות עד כה – המשך רגיל: מעבר שני מלא וכתיבת פלטים */
        printf("First pass successful for %s\n", file_name);
        finalize_outputs(file_name,
                         codeBuffer, codeCount,
                         dataImage, dataCount,
                         labelTable,
                         entryTable, entryCount,
                         externalTable, externalCount);
    }

    /* משחרר זיכרון */
    free(directives);
    free(dataImage);
    free(codeBuffer);
    free(entryTable);
    free(externalTable);
    free_label_list(labelTable);

    /* אם נמצאה שגיאה, מחזירים 1 אחרת מחזירים 0 */
    return errorFound ? 1 : 0;
}

