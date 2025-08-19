/* C-Course Final Project - Assembler (authored by Yehuda) */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "utils.h"
#include "code_conversion.h"
#include "label_table.h"
#include "emit/modular_helpers.h"
#include "../../error-handler/error-handler.h"

#ifndef IC_INIT_VALUE
#define IC_INIT_VALUE 100
#endif

/* בודק את מצב addressing של האופרנד */
/* כלומר האם זה מספר או רגיסטר לדוגמה*/
static int addressingMode(const char *operand) {
    if (!operand || !*operand) return -1;
    if (operand[0] == '#') return 0;
    if (operand[0] == 'r' && operand[1] >= '0' && operand[1] <= '7' && operand[2] == '\0') return 3;
    if (strchr(operand, '[')) return 2;
    return 1;
}

/* חותך רווחים מההתחלה ומהסוף  */
static void trimString(char *s) {
    if (!s) return;
    {
        char *p = s;
        while (isspace((unsigned char)*p)) p++;
        if (p != s) memmove(s, p, strlen(p) + 1);
    }
    {
        size_t n = strlen(s);
        while (n && isspace((unsigned char)s[n - 1])) s[--n] = '\0';
    }
}

/* עוזר: מעתיק מחרוזת לתוך באפר בגודל נתון ומסיר רווחים בקצוות */
static void copyAndTrim(char *dst, size_t dstSize, const char *src) {
    if (!dst || dstSize == 0) return;
    if (!src) {
        dst[0] = '\0';
        return;
    }
    strncpy(dst, src, dstSize - 1);
    dst[dstSize - 1] = '\0';
    trimString(dst);
}

/* מפריד בין האופרנדים; מחזיר כמה אופרנדים נמצאו (0/1/2) */
static int splitOperands(const char *operands, char *source, size_t sourceSize, char *destination, size_t destinationSize) {
    const char *comma;
    size_t len_before;
    int count = 0;

    if (!operands || !*operands) {
        if (source && sourceSize) source[0] = '\0';
        if (destination && destinationSize) destination[0] = '\0';
        return 0;
    }

    /* חפש את הפסיק המפריד בין מקור ליעד (אם קיים) */
    comma = strchr(operands, ',');
    if (!comma) {
        /* רק אופרנד אחד */
        copyAndTrim(source, sourceSize, operands);
        if (destination && destinationSize) destination[0] = '\0';
        return (source && *source) ? 1 : 0;
    }

    /* העתק את החלק שלפני הפסיק כמקור */
    len_before = (size_t)(comma - operands);
    if (source && sourceSize) {
        size_t n = len_before;
        if (n >= sourceSize) n = sourceSize - 1;
        memcpy(source, operands, n);
        source[n] = '\0';
        trimString(source);
    }

    /* העתק את החלק שאחרי הפסיק כיעד */
    copyAndTrim(destination, destinationSize, comma + 1);

    if (source && *source) count++;
    if (destination && *destination) count++;
    return count;
}

/*
 * ערכים: opcode בארבעה ביטים, source והdestination בשני ביטים כל אחד.
 * מיקומים: opcode*64, source*16, destination*4. שני הביטים התחתונים (ARE) נשארים 00.
 */
static unsigned short packFirstWord(int opcode, int sourceMode, int destinationMode) {
    int op = opcode % 16;  /* טווח 15 - 0 */
    int src = sourceMode % 4;   /* טווח 3 - 0 */
    int dst = destinationMode % 4; /* טווח 3 - 0 */
    if (op < 0) op += 16;  /* הגנה במקרה של ערך שלילי */
    if (src < 0) src += 4;
    if (dst < 0) dst += 4;
    return (unsigned short)(op * 64 + src * 16 + dst * 4);
}

/* מחזיר את מספר הרגיסטר מתוך מחרוזת רגיסטר */
int regnum(const char *reg) {
    if (reg && reg[0] == 'r' && reg[1] >= '0' && reg[1] <= '7' && reg[2] == '\0') return reg[1] - '0';
    return 0;
}

/* מחזיר את מספר הרגיסטר מתוך מטריצה */
int mat_reg(const char *mat, int which) {
    const char *p = mat;
    int found = 0;
    while (*p) {
        if (*p == '[') {
            if (which == found) {
                p++;
                if (*p == 'r' && p[1] >= '0' && p[1] <= '7') return p[1] - '0';
            }
            found++;
        }
        p++;
    }
    return 0;
}

void mat_label(const char *mat, char *out) {
    const char *p = strchr(mat, '[');
    if (!p) {
        strcpy(out, mat);
        return;
    }
    {
        size_t len = (size_t)(p - mat);
        strncpy(out, mat, len);
        out[len] = '\0';
    }
}

/*
 * אריזת זוג רגיסטרים למילת-מידע (10 סיביות) ללא הזזות ביטים.
 * src נכנס לסיביות 9..6, dst לסיביות 5..2; שני הסיביות התחתונות שמורות ל-ARE ונשארות 00.
 * נוסחה: src*64 + dst*4, תחום התוצאה ל-10 סיביות באמצעות mod 1024.
 */
static unsigned short packRegisterPairWord(int sourceRegister, int destinationRegister) {
    int src = sourceRegister % 16;  /* רגיסטר מקור בטווח 0..15 (משתמשים רק ב-0..7 בפועל) */
    int dst = destinationRegister % 16; /* רגיסטר יעד בטווח 0..15 (משתמשים רק ב-0..7 בפועל) */
    if (src < 0) src += 16;
    if (dst < 0) dst += 16;
    return (unsigned short)(((src * 64) + (dst * 4)) % 1024);
}

/* כתיבת מילה המייצגת תווית בקוד */
static void writeLabelWord(code_conv_t *code, int index, const char *symbol) {
    code[index].value = 0;
    code[index].are = 1;
    /* העתקת שם התווית */
    if (symbol && *symbol) {
        strncpy(code[index].ext_name, symbol, sizeof(code[index].ext_name) - 1);
        code[index].ext_name[sizeof(code[index].ext_name) - 1] = '\0';
    } else {
        code[index].ext_name[0] = '\0';
    }
}

/* כתיבת מילה המייצגת ערך מיידי בקוד */
static void writeImmediateWord(code_conv_t *code, int index, int number) {
    int imm = number % 256;      /* 8 ביט */
    if (imm < 0) imm += 256;     /* תיקון שלילי */
    code[index].value = (unsigned short)(imm * 4); /* ARE=00 => כפול 4 */
    code[index].are = 0;         /* ARE=00 */
    code[index].ext_name[0] = '\0';
}

/* כתיבת מילה המייצגת זוג רגיסטרים בקוד */
static int writeMatrixWords(code_conv_t *code, int index, const char *operand) {
    char label[64];
    int rowRegister;
    int colRegister;
    mat_label(operand, label);
    writeLabelWord(code, index, label);
    rowRegister = mat_reg(operand, 0);
    colRegister = mat_reg(operand, 1);
    /* אריזת זוג הרגיסטרים */
    code[index + 1].value = packRegisterPairWord(rowRegister, colRegister);
    code[index + 1].are = 0; /* ARE=00 */
    code[index + 1].ext_name[0] = '\0';
    return 2; /* זוג מילים */
}

/* כתיבת מילה המייצגת יעד בקוד */
static int writeDestinationWords(code_conv_t *code, int index, const char *destination, int destinationMode) {
    int written = 0;
    /* כתיבת מילה המייצגת יעד בקוד */
    /* אם מצב הכתיבה הוא מיידי */
    if (destinationMode == 0) {
        int number = atoi(destination + 1);
        writeImmediateWord(code, index + written, number);
        written++;
    } else if (destinationMode == 1) { /* אם מצב הכתיבה הוא תווית */
        writeLabelWord(code, index + written, destination);
        written++;
    } else if (destinationMode == 2) { /* אם מצב הכתיבה הוא מטריצה */
        written += writeMatrixWords(code, index + written, destination);
    } else if (destinationMode == 3) { /* אם מצב הכתיבה הוא רגיסטר */
        int destinationRegister = regnum(destination);
        code[index + written].value = packRegisterPairWord(0, destinationRegister);
        code[index + written].are = 0;
        code[index + written].ext_name[0] = '\0';
        written++;
    }
    return written;
}

/* כתיבת מילה המייצגת מקור בקוד */
static int writeSourceWords(code_conv_t *code, int index, const char *source, int sourceMode) {
    int written = 0;
    /* כתיבת מילה המייצגת מקור בקוד */
    /* אם מצב הכתיבה הוא מיידי */
    if (sourceMode == 0) {
        int number = atoi(source + 1);
        writeImmediateWord(code, index + written, number);
        written++;
    } else if (sourceMode == 1) { /* אם מצב הכתיבה הוא תווית */
        writeLabelWord(code, index + written, source);
        written++;
    } else if (sourceMode == 2) { /* אם מצב הכתיבה הוא מטריצה */
        written += writeMatrixWords(code, index + written, source);
    } else if (sourceMode == 3) { /* אם מצב הכתיבה הוא רגיסטר */
        int sourceRegister = regnum(source); /* קבלת מספר הרגיסטר */
        code[index + written].value = packRegisterPairWord(sourceRegister, 0);
        code[index + written].are = 0;
        code[index + written].ext_name[0] = '\0'; 
        
        written++;
    }
    return written;
}

/* כתיבת מילה המייצגת שני אופרנדים בקוד */
static int writeTwoOperandsWords(code_conv_t *code, int index, const char *source, int sourceMode, const char *destination, int destinationMode) {
    if (sourceMode == 3 && destinationMode == 3) { /* אם מצב הכתיבה הוא רגיסטרים */
        int sourceRegister = regnum(source); /* קבלת מספר הרגיסטר */
        int destinationRegister = regnum(destination); /* קבלת מספר הרגיסטר */
        code[index].value = packRegisterPairWord(sourceRegister, destinationRegister);
        code[index].are = 0;
        code[index].ext_name[0] = '\0';
        return 1;
    }
    {
        int used = 0;
        used += writeSourceWords(code, index + used, source, sourceMode);
        used += writeDestinationWords(code, index + used, destination, destinationMode);
        return used;
    }
}

/* עימוד (כןכן) של האופרנדים לבפרים */
static int parseOperandsToBuffers(const char *operands, char *source, size_t sourceSize, char *destination, size_t destinationSize) {
    return splitOperands(operands, source, sourceSize, destination, destinationSize);
}

/* קבלת מצבי הכתיבה של שני האופרנדים */
static void getTwoOperandModes(const char *source, const char *destination, int *sourceMode, int *destinationMode) {
    *sourceMode = addressingMode(source);
    *destinationMode = addressingMode(destination);
}

/* קבלת מצבי הכתיבה של אופרנדה אחת */
static void getOneOperandMode(const char *source, int *destinationMode, char *destination, size_t destinationSize) {
    *destinationMode = addressingMode(source);
    strncpy(destination, source, destinationSize - 1);
    destination[destinationSize - 1] = '\0';
}

/* קבלת מצבי הכתיבה של שני האופרנדים */
static int areOperandModesValid(const OpCodeData *op, int sourceMode, int destinationMode) {
    /* בדיקת תקינות מצבי הכתיבה */
    if (op->operand_count == 2) {
        return (sourceMode >= 0 && destinationMode >= 0 && op->valid_src_addr[sourceMode] && op->valid_dst_addr[destinationMode]);
    }
    if (op->operand_count == 1) {
        return (destinationMode >= 0 && op->valid_dst_addr[destinationMode]);
    }
    /* no operands */
    return 1;
}

/* הכנה של האופרנדים */
static int prepareOperands(const OpCodeData *op, const char *operands, char *source, size_t sourceSize, char *destination, size_t destinationSize, int *sourceMode, int *destinationMode, const char *fileName, int lineNumber, int *errorFound) {
    int found;
    char buf[128];
    int err;

    found = parseOperandsToBuffers(operands, source, sourceSize, destination, destinationSize);
    /* בדיקת מספר האופרנדים */
    if (found != op->operand_count) {
        sprintf(buf, "expected %d got %d", op->operand_count, found); /* chatgpt עזר לי למצו את הפונקציה הזאת*/
        error_report_ex(ERR_SEV_ERROR, ERR_OPERAND_COUNT_MISMATCH, fileName, lineNumber, buf);
        *errorFound = 1;
        return 0;
    }
    /* קבלת מצבי הכתיבה של שני האופרנדים */
    if (op->operand_count == 2) {
        getTwoOperandModes(source, destination, sourceMode, destinationMode);
    } else if (op->operand_count == 1) { /* קבלת מצבי הכתיבה של אופרנדה אחת */
        getOneOperandMode(source, destinationMode, destination, destinationSize);
        *sourceMode = 0;
        source[0] = '\0';
    } else { /*  - אין מקור ואין יעד קבלת מצבי הכתיבה של אפס האופרנדים */
        *sourceMode = 0;
        *destinationMode = 0;
    }
    /* בדיקת תקינות מצבי הכתיבה */
    if (!areOperandModesValid(op, *sourceMode, *destinationMode)) {
        *errorFound = 1;
        /* בדיקת תקינות מצבי הכתיבה */
        err = (op->operand_count == 2 && (!op->valid_src_addr[*sourceMode] || *sourceMode < 0)) ? ERR_ADDR_MODE_INVALID_SRC : ERR_ADDR_MODE_INVALID_DST;
        error_report_ex(ERR_SEV_ERROR, err, fileName, lineNumber, NULL);
        return 0;
    }
    return 1;
}

/* טיפול בהגדרת תווית */
static int handleLabelDefinition(InstParts *inst, int codeCount, LabelNode **labels, int *errorFound) {
    if (inst->label && *inst->label) {
        int address = IC_INIT_VALUE + codeCount;
        /* הוספת התווית לטבלת התוויות */
        if (!insert_label(labels, inst->label, address, 1, 0, 0)) { *errorFound = 1; return 0; }
    }
    return 1;
}

/* כתיבת המילה הראשונה בקוד */
static void writeFirstWord(code_conv_t *code, int index, const OpCodeData *op, int sourceMode, int destinationMode, int lineNumber) {
    code[index].value = packFirstWord(op->code, sourceMode, destinationMode);
    code[index].are = 0;
    code[index].source_line_num = lineNumber;
    code[index].ext_name[0] = '\0';
}

/* הרכבת ההוראה */
int assembleInstruction(InstParts *inst, code_conv_t *code, int codeCount, int lineNumber, LabelNode **labelTableHead, int *errorFound, const char *fileName) {
    int written = 0;
    char source[128];
    char destination[128];
    int sourceMode = 0, destinationMode = 0;
    const OpCodeData *op;
    char buf[128];
    (void)fileName;
    /* בדיקת תקינות ההוראה */
    if (!inst || !inst->opcode) return 0;
    if (!handleLabelDefinition(inst, codeCount, labelTableHead, errorFound)) return 0;

    op = findOpcodeByName(inst->opcode);
    /* בדיקת תקינות האופקוד */
    if (!op) {
        *errorFound = 1;
        sprintf(buf, "opcode '%s'", inst->opcode);
        error_report_ex(ERR_SEV_ERROR, ERR_OPCODE_UNKNOWN, fileName, lineNumber, buf);
        return 0;
    }
    /* קבלת מצבי הכתיבה של האופרנדים */
    source[0] = '\0';
    destination[0] = '\0';
    
    if (!prepareOperands(
        op, inst->operands, source,
         sizeof(source), destination,
          sizeof(destination), &sourceMode, 
          &destinationMode, fileName, lineNumber,
           errorFound)) {
       return 0;
    }

    writeFirstWord(code, codeCount + written, op, sourceMode, destinationMode, lineNumber);

    written++;

    /* כתיבת המילים הנוספות בקוד */
    if (op->operand_count == 2) { /* כתיבת שתי מילים */
        written += writeTwoOperandsWords(code, codeCount + written, source, sourceMode, destination, destinationMode);
    } else if (op->operand_count == 1) { /* כתיבת מילה אחת */
        written += writeDestinationWords(code, codeCount + written, destination, destinationMode);
    }
    return written;
}

/* פלט ההוראה */
int emit_instruction(InstParts *inst, code_conv_t *code, int code_count, int line_num, LabelNode **label_table_head, int *error_found, const char *file_name) {
    return assembleInstruction(inst, code, code_count, line_num, label_table_head, error_found, file_name);
}
