/* MAX30102 uitlees code
 */

typedef struct {
    int i2c;
    double red, ir;
    double redw, irw;
    uint8_t redcurrent, ircurrent;
    char pdstate;
} max30102_t;

void max_update(max30102_t *mx);
max30102_t *max_init(void);
