/*
 * C code om MAX30102 uit te lezen
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <asm/ioctl.h>
#include "MAX30102.h"

#define ALPHA 0.95

#define I2C_SLAVE 0x0703
#define I2C_SMBUS 0x0720
#define I2C_READ  1
#define I2C_WRITE 0
#define I2C_BYTE_DATA  2
#define I2C_BLOCK_DATA 8
#define MAX30102_I2C_ADDRESS 0x57

#define MAX30102_FIFO_WPTR 0x04
#define MAX30102_FIFO_OVFL 0x05
#define MAX30102_FIFO_RPTR 0x06
#define MAX30102_FIFO_DATA 0x07
#define MAX30102_FIFO_CONF 0x08
#define MAX30102_MODE_CONF 0x09
#define MAX30102_SPO2_CONF 0x0A
#define MAX30102_LED_PULSEAMPLITUDE 0x0C
#define MAX30102_LED_SLOTS 0x11
#define MAX30102_TEMP_INT  0x1F
#define MAX30102_TEMP_FRAC 0x20
#define MAX30102_TEMP_CONF 0x21

static int i2c_open(int dev_id)
{
    int fd = open("/dev/i2c-1", O_RDWR);
    if (fd < 0) return fd;
    int res = ioctl(fd, I2C_SLAVE, dev_id);
    if (res < 0) return res;
    return fd;
}

union i2c_data {
    uint8_t byte;
    uint16_t word;
    uint8_t block[34];
};

struct i2c_ioctl_args {
    char rw;
    uint8_t reg;
    int size;
    union i2c_data *data;
};

static inline int i2c_write_byte(int i2c, int reg, uint8_t byte)
{
    union i2c_data data;
    struct i2c_ioctl_args args = { .rw = I2C_WRITE, .reg = reg, .size = I2C_BYTE_DATA, .data = &data };
    data.byte = byte;
    return ioctl(i2c, I2C_SMBUS, &args);
}

static inline int i2c_read_byte(int i2c, int reg)
{
    union i2c_data data;
    struct i2c_ioctl_args args = { .rw = I2C_READ, .reg = reg, .size = I2C_BYTE_DATA, .data = &data };
    if (ioctl(i2c, I2C_SMBUS, &args) < 0) return -1;
    return data.byte;
}

static inline int i2c_read_block(int i2c, int reg, int size, uint8_t *bytes)
{
    union i2c_data data;
    struct i2c_ioctl_args args = { .rw = I2C_READ, .reg = reg, .size = I2C_BLOCK_DATA, .data = &data };
    data.block[0] = (uint8_t)size;
    if (ioctl(i2c, I2C_SMBUS, &args) < 0) return -1;
    if (data.block[0] > size) return -1;
    memcpy(bytes, data.block+1, data.block[0]);
    return data.block[0];
}

/*
static inline int i2c_read_block(int i2c, int reg, int size, uint8_t *bytes)
{
    for (int i = 0; i < size; i++) {
        int bt = i2c_read_byte(i2c, reg);
        bytes[i] = bt;
    }
    return 0;
}
*/

static int set_mode(max30102_t *mx, uint8_t mode)
{
    return i2c_write_byte(mx->i2c, MAX30102_MODE_CONF, mode);
}

static int set_spo2_conf(max30102_t *mx, uint8_t spo2)
{
    return i2c_write_byte(mx->i2c, MAX30102_SPO2_CONF, spo2);
}

static int set_temp_mode(max30102_t *mx, uint8_t temp_en)
{
    return i2c_write_byte(mx->i2c, MAX30102_TEMP_CONF, temp_en);
}

static int set_currents(max30102_t *mx)
{
    if (i2c_write_byte(mx->i2c, MAX30102_LED_PULSEAMPLITUDE, mx->redcurrent) < 0) return -1;
    if (i2c_write_byte(mx->i2c, MAX30102_LED_PULSEAMPLITUDE+1, mx->ircurrent) < 0) return -1;
    return 0;
}

static int max_read(max30102_t *mx)
{
    unsigned char buf[6];
    if (i2c_read_block(mx->i2c, MAX30102_FIFO_DATA, 6, buf) < 0) {
        fprintf(stderr, "Error reading FIFO data: %s\n", strerror(errno));
        return -1;
    }
    mx->rawir = ((buf[0]<<16) + (buf[1]<<8) + buf[2]);
    mx->rawred = ((buf[3]<<16) + (buf[4]<<8) + buf[5]);

    double irw = (double)mx->rawir + ALPHA * mx->irw;
    mx->ir = irw - mx->irw;
    mx->irw = irw;

    double redw = (double)mx->rawred + ALPHA * mx->redw;
    mx->red = redw - mx->redw;
    mx->redw = redw;
    return 0;
}

int max_update(max30102_t *mx)
{
    int wrptr = i2c_read_byte(mx->i2c, MAX30102_FIFO_WPTR);
    int rdptr = i2c_read_byte(mx->i2c, MAX30102_FIFO_RPTR);
    int numsmpl = wrptr - rdptr;
    if (numsmpl < 0) numsmpl += 0x20;
    // fprintf(stderr, "WPTR = 0x%02x, RPTR = 0x%02x, Reading %d samples\n", wrptr, rdptr, numsmpl);
    for (int i = 0; i < numsmpl; i++) {
        max_read(mx);
    }
    return 0;
}

max30102_t *max_init(void)
{
    max30102_t *mx = malloc(sizeof(max30102_t));
    mx->i2c = i2c_open(MAX30102_I2C_ADDRESS);
    if (mx->i2c < 0) {
        free(mx);
        fprintf(stderr, "Unable to start I2C MAX30102: %s\n", strerror(errno));
        return NULL;
    }
#ifdef DEBUG
    fprintf(stderr, "Reading old register settings\n");
    for (int i = 0; i < 0x22; i++) {
        int byte = i2c_read_byte(mx->i2c, i);
        if (byte < 0) {
            fprintf(stderr, "Read reg 0x%02x failed: %s\n", i, strerror(errno));
        } else {
            fprintf(stderr, "Read reg 0x%02x = 0x%02x\n", i, byte);
        }
    }
#endif
    mx->pdstate = 0;
    mx->redcurrent = 0x0f;
    mx->ircurrent = 0x07;
    mx->irw = 0;
    mx->ir = 0;
    mx->redw = 0;
    mx->red = 0;
    int res;
    if (set_mode(mx, 0x43) < 0) {
        fprintf(stderr, "set_mode failed: %s\n", strerror(errno));
    }
    usleep(10000);
    if (set_mode(mx, 0x03) < 0) {
        fprintf(stderr, "set_mode failed: %s\n", strerror(errno));
    }
    if (set_spo2_conf(mx, (0x00 << 5) | (0x01 << 2) | 0x03) < 0) { /* ADC range | samplerate | pulsewidth */
        fprintf(stderr, "set_spo2_conf failed: %s\n", strerror(errno));
    }
    if (set_currents(mx) < 0) {
        fprintf(stderr, "set_currents failed: %s\n", strerror(errno));
    }
    if (set_temp_mode(mx, 0x01) < 0) {
        fprintf(stderr, "set_currents failed: %s\n", strerror(errno));
    }
#ifdef DEBUG
    fprintf(stderr, "Reading new register settings\n");
    for (int i = 0; i < 0x22; i++) {
        int byte = i2c_read_byte(mx->i2c, i);
        if (byte < 0) {
            fprintf(stderr, "Read reg 0x%02x failed: %s\n", i, strerror(errno));
        } else {
            fprintf(stderr, "Read reg 0x%02x = 0x%02x\n", i, byte);
        }
    }
#endif
    return mx;
}

int max_fini(max30102_t *mx)
{
    if (set_mode(mx, 0x80) < 0) {
        fprintf(stderr, "set_mode failed: %s\n", strerror(errno));
    }
    return 0;
}

/* vim: ai:si:expandtab:ts=4:sw=4
 */
