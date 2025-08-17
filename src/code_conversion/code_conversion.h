#ifndef CODE_CONVERSION_H
#define CODE_CONVERSION_H

/*   מבנה נתונים לייצוג של שורה בבינארי */
typedef struct {
    unsigned short value;
    unsigned char are;
    unsigned char translated;
    int source_line_num; 
    char ext_name[32]; /* שם סמל חיצוני אם יש */
} code_conv_t;

#define MAX_OPCODE_NAME 10

/* מבנה נתונים לייצוג פקודה */
typedef struct {
    char name[MAX_OPCODE_NAME];
    int code;
    int operand_count;
    int valid_src_addr[4];
    int valid_dst_addr[4];
} OpCodeData;

#include "../memory_map/memory_map.h" /* Use a single authoritative MEMORY_SIZE definition */

extern OpCodeData opcode_table[];
extern int num_opcodes;

const OpCodeData* findOpcodeByName(const char *name);
void valToBinStr(unsigned short value, char *out, int bits);
char getAREchar(int are);

#endif