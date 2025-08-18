#include "utils.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include "../error-handler/error-handler.h"

/* מסירה רווחים מיותרים ממחרוזת */
void removeExtraSpacesStr(char *str) {
    char *src = str, *dst = str;
    int space = 0;
    if (!str) return; /* אם המחרוזת ריקה, אין מה לעשות */
    /* עובר על כל התווים במחרוזת */
    while (*src) {
        /* אם התו הוא רווח או טאב, בודקים אם זה לא רווח מיותר */
        if (*src == ' ' || *src == '\t') {
            /* אם זה רווח מיותר, לא מעתיקים אותו */
            if (!space) *dst++ = ' ';
            space = 1;
        } else {
            /* אם זה תו אחר, מעתיקים אותו */
            *dst++ = *src;
            space = 0;
        }
        /* ממשיכים לתו הבא */
        src++;
    }
    /* מסיימים את המחרוזת */
    *dst = '\0';
}

/* מסיר רווחים מיותרים ליד פסיקים */
void removeSpacesNextToComma(char *str) {
    char *src = str, *dst = str;
    /* עובר על כל התווים במחרוזת */
    while (*src) {
        /* אם התו הוא פסיק, מעתיקים אותו */
        if ((*src == ' ' || *src == '\t') && (src > str && (*(src-1) == ',' || *(src+1) == ','))) {
            src++;
            continue;
        }
        /* אם התו הוא פסיק, מעתיקים אותו */
        *dst++ = *src++;
    }
    /* מסיימים את המחרוזת */
    *dst = '\0';
}

/* עוזר לפצל מחרוזת לתתי מחרוזות */
static char *substrdup(const char *start, size_t len) {
    /* מקצה זיכרון לתוצאה החדשה */
    char *s = (char *)malloc(len + 1);
    /* אם לא הצליח להקצות זיכרון, מחזיר NULL */
    if (!s) return NULL;
    /* מעתיק את התת-מחרוזת לתוך התוצאה */
    memcpy(s, start, len);
    /* מסיים את המחרוזת */
    s[len] = '\0';
    return s;
}

/* מפצל שורה של הוראה לחלקים: תווית, אופקוד, אופרנדים */
InstParts parseInstLine(char *line) {
    /* אתחול מבנה החלקים של ההוראה */
    InstParts inst;
    char *p;
    char *colon;
    char *after_colon = NULL;
    char *opcode_start;
    char *opcode_end; 

    /* אתחול כל השדות של ההוראה */
    inst.label = NULL;
    inst.opcode = NULL;
    inst.operands = NULL;
    inst.src_operand = NULL;
    inst.dst_operand = NULL;

    p = line;
    /* מחפש את תחילת האופקוד */
    while (isspace((unsigned char)*p)) p++;
    /* אם השורה ריקה או מתחילה בסימן סיום שורה, מחזיר מבנה ריק */
    if (*p == '\0' || *p == ';') return inst;

    /* מחפש את התו ':' שמפריד בין תווית לאופקוד */
    colon = strchr(p, ':');
    /* אם יש תו ':', אז יש תווית */
    if (colon && (colon > p)) {
        
        const char *label_start = p;
        /* מחשב את האורך של התווית */
        size_t label_len = (size_t)(colon - p);
        /* מעתיק את התווית לתוך המבנה */
        inst.label = substrdup(label_start, label_len);
        /* מעדכן את המצביע אחרי התו ':' */
        after_colon = colon + 1;
        /* מסיר רווחים מיותרים אחרי התו ':' */
        p = after_colon;
        while (isspace((unsigned char)*p)) p++;
    }

    /* מחפש את תחילת האופקוד */
    opcode_start = p;
    /* מחפש את סוף האופקוד עד לתו רווח או סיום שורה */
    while (*p && !isspace((unsigned char)*p)) p++;
    opcode_end = p;

    /* מוודא שבמטריצה הסוגר המררובע מחובר לשם */
    if (opcode_end > opcode_start && (size_t)(opcode_end - opcode_start) >= 5 && strncmp(opcode_start, ".mat[", 5) == 0) {
        const char *br = opcode_start + 4; /* מצביע על '[' */
        size_t op_len;
        /* אורך האופקוד */
        inst.opcode = substrdup(".mat", 4);
        /* אורך האופרנדים */
        inst.operands = substrdup(br, strlen(br));
        /* מסיר רווחים מיותרים בסוף האופרנדים */
        if (inst.operands) {
            op_len = strlen(inst.operands);
            while (op_len > 0 && isspace((unsigned char)inst.operands[op_len - 1])) {
                inst.operands[--op_len] = '\0';
            }
        }
        return inst;
    }

    if (p > opcode_start) {
        inst.opcode = substrdup(opcode_start, (size_t)(p - opcode_start));
    } else {
        return inst;
    }
    while (isspace((unsigned char)*p)) p++;

    if (*p) {
        size_t len;
        inst.operands = substrdup(p, strlen(p));
        len = strlen(inst.operands);
        while (len > 0 && isspace((unsigned char)inst.operands[len-1])) {
            inst.operands[--len] = '\0';
        }
    }

    return inst;
}

/* משחרר את הזיכרון שהוקצה לחלקי ההוראה */
void freeInstParts(InstParts *inst) {
    if (!inst) return;
    if (inst->label) { free(inst->label); inst->label = NULL; }
    if (inst->opcode) { free(inst->opcode); inst->opcode = NULL; }
    if (inst->operands) { free(inst->operands); inst->operands = NULL; }
    if (inst->src_operand) { free(inst->src_operand); inst->src_operand = NULL; }
    if (inst->dst_operand) { free(inst->dst_operand); inst->dst_operand = NULL; }
}

/* בודק אם האופקוד הוא הוראה */
int isInstr(const char *opcode) {
    return opcode && opcode[0] == '.';
}

/* מסיר רווחים מיותרים מתחילת המחרוזת */
char* ltrim(char* str) {
    while (isspace((unsigned char)*str)) str++;
    return str;
}

/* משווה שתי מחרוזות תוך התעלמות מהאותיות הגדולות והקטנות */
int startsWithIgnoreCase(const char* str, const char* prefix) {
    while (*prefix && *str && tolower((unsigned char)*str) == tolower((unsigned char)*prefix)) {
        str++;
        prefix++;
    }
    return *prefix == '\0';
}

/* בודק אם התווית חוקית */
int legal_label_decl(const char *label, int *error_code) {
    const char *p;
    int len;
    if (error_code) *error_code = 0;
    /* אם התווית ריקה או NULL, מחזיר שגיאה */
    if (!label || !*label) {
        if (error_code) *error_code = ERR_LABEL_EMPTY;
        return 0;
    }
    /* אם התו הראשון אינו אות, מחזיר שגיאה */
    if (!isalpha((unsigned char)label[0])) {
        if (error_code) *error_code = ERR_LABEL_NOT_START_ALPHA;
        return 0;
    }
    /* אם התו הראשון הוא רווח, מחזיר שגיאה */
    len = (int)strlen(label);
    if (len > 30) {
        if (error_code) *error_code = ERR_LABEL_TOO_LONG;
        return 0;
    }
    p = label;
    /* עובר על כל התווים בתווית */
    while (*p) {
        if (!isalnum((unsigned char)*p)) {
            if (error_code) *error_code = ERR_LABEL_NON_ALNUM;
            return 0;
        }
        p++;
    }
    return 1;
}

/* מדווח על שגיאה חיצונית */
void print_external_error(int error_code, const char *file, int line) {
    /* בדוק את קוד השגיאה */
    switch (error_code) {
        case ERR_LABEL_EMPTY:
        case ERR_LABEL_NOT_START_ALPHA:
        case ERR_LABEL_TOO_LONG:
        case ERR_LABEL_NON_ALNUM:
            error_report_ex(ERR_SEV_ERROR, (ErrorCode)error_code, file, line, NULL);
            break;
        default:
            error_report_ex(ERR_SEV_ERROR, ERR_LABEL_REDEFINED, file, line, NULL);
            break;
    }
}

/* מקבל את הנתיב של הקובץ ומחזיר את שם הקובץ ללא הסיומת */
void get_basefile(const char *path, char *out_base, size_t out_size) {
    const char *slash;
    const char *base;
    const char *dot;
    size_t len;
    if (!path || !out_base || out_size == 0) return;
    slash = strrchr(path, '/');
    base = slash ? slash + 1 : path;
    dot = strrchr(base, '.');
    len = dot ? (size_t)(dot - base) : strlen(base);
    if (len >= out_size) len = out_size - 1;
    memcpy(out_base, base, len);
    out_base[len] = '\0';
}

/* מוודא שיש תיקיית פלט */
int ensure_output_dir(const char *base_name, char *out_dir, size_t out_dir_size) {
    /* אם אין תיקיית פלט ייעודית: כתוב קבצים לתוך התיקיה הנוכחית. */
    if (!out_dir || out_dir_size == 0) return 0;
    /* אם גודל המערך קטן מדי, מחזיר שגיאה */
    if (out_dir_size < 2) return 0;
    out_dir[0] = '.';
    out_dir[1] = '\0';
    return 1;
}
