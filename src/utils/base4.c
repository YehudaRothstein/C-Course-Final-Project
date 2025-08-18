#include "base4.h"
#include <string.h>

/* ממירה ערך בגודל 10 סיביות למחרוזת של 5 תווים בבסיס 4*/
void to_special_base4_str(unsigned short value, char *out) {
    static const char digits[] = "abcd";
    int i;
    /* לולאה שממירה את הערך לבסיס 4 */
    for (i = 4; i >= 0; --i) {
        /* מתאימים הערך הנוכחי לספרה המתאימה בבסיס 4 */
        out[i] = digits[value % 4];
        /* מחלקים את הערך ב-4 כדי לעבור לספרה הבאה */
        value /= 4;
    }
    out[5] = '\0';
}
