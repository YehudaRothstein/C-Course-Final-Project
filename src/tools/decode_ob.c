#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static unsigned short from_special_base4(const char *s) {
    unsigned short v = 0;
    const char *p;
    for (p = s; *p; ++p) {
        int d;
        switch (*p) {
            case 'a': d = 0; break;
            case 'b': d = 1; break;
            case 'c': d = 2; break;
            case 'd': d = 3; break;
            default: continue; 
        }
        v = (unsigned short)((v << 2) | (d & 0x3));
    }
    return (unsigned short)(v & 0x3FF);
}


static void to_binary10(unsigned short v, char out[11]) {
    int i;
    for (i = 9; i >= 0; --i) {
        out[9 - i] = ((v >> i) & 1) ? '1' : '0';
    }
    out[10] = '\0';
}


int main(int argc, char **argv) {
    const char *filename;
    FILE *fp;
    char cc[32], dc[32];
    char addr[32], val[32];
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file.ob>\n", argv[0]);
        return 1;
    }
    filename = argv[1];
    fp = fopen(filename, "r");
    if (!fp) {
        perror("fopen");
        return 1;
    }

    
    if (fscanf(fp, "%31s %31s", cc, dc) != 2) {
        fprintf(stderr, "Invalid .ob header\n");
        fclose(fp);
        return 1;
    }
    printf("code=%s data=%s\n", cc, dc);

    
    while (fscanf(fp, "%31s %31s", addr, val) == 2) {
        unsigned short bits = from_special_base4(val);
        {
            char bin[11];
            to_binary10(bits, bin);
            printf("%s %s %s\n", addr, val, bin);
        }
    }

    fclose(fp);
    return 0;
}
