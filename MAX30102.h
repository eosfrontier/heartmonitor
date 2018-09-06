/* MAX30102 uitlees code
 */

#include <stdint.h>

typedef struct {
    int i2c;
    double red, ir;
    double redw, irw;
    uint32_t rawred, rawir;
    uint8_t redcurrent, ircurrent;
    char pdstate;
} max30102_t;

int max_update(max30102_t *mx);
max30102_t *max_init(void);
int max_fini(max30102_t *mx);
