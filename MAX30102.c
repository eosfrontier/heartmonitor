/*
 * C code om MAX30102 uit te lezen
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <asm/ioctl.h>
#include "MAX30102.h"

#define ALPHA 0.95

#define I2C_SLAVE 0x703
#define I2C_SMBUS 0x720
#define I2C_SMBUS_READ  1
#define I2C_SMBUS_WRITE 0

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

static inline int i2c_ioctl(int i2c, char rw, int reg, int size, void *data)
{
    struct {
        char rw;
        uint8_t reg;
        int size;
        void *data;
    } args;
    args.rw = rw;
    args.reg = reg;
    args.size = size;
    args.data = data;
    return ioctl(i2c, I2C_SMBUS, &data);
}

static inline int i2c_update_reg(int i2c, int reg, uint8_t mask, uint8_t val)
{
    uint8_t curval;
    int res;
    res = i2c_ioctl(i2c, I2C_SMBUS_READ, reg, 1, &curval);
    if (res < 0) { return res; }
    val = (curval & (mask ^ 0xff)) | (val & mask);
    return i2c_ioctl(i2c, I2C_SMBUS_WRITE, reg, 1, &val);
}

static int set_mode(max30102_t *mx, uint8_t mode)
{
    return i2c_ioctl(mx->i2c, I2C_SMBUS_WRITE, MAX30102_MODE_CONF, 1, &mode);
}

static int set_spo2_conf(max30102_t *mx, uint8_t spo2)
{
    return i2c_ioctl(mx->i2c, I2C_SMBUS_WRITE, MAX30102_SPO2_CONF, 1, &spo2);
}

static int set_currents(max30102_t *mx)
{
    struct { uint8_t red, ir; } amps;
    amps.red = mx->redcurrent;
    amps.ir = mx->ircurrent;
    return i2c_ioctl(mx->i2c, I2C_SMBUS_WRITE, MAX30102_LED_PULSEAMPLITUDE, 2, &amps);
}

void max_update(max30102_t *mx)
{
    unsigned char buf[4];
    i2c_ioctl(mx->i2c, I2C_SMBUS_READ, MAX30102_FIFO_DATA, 4, &buf);
    double rawir = (double)((buf[0]<<8) + buf[1]);
    double rawred = (double)((buf[2]<<8) + buf[3]);

    double irw = rawir  + ALPHA * mx->irw;
    mx->ir = irw  - mx->irw;
    mx->irw = irw;

    double redw = rawred + ALPHA * mx->redw;
    mx->red = redw - mx->redw;
    mx->redw = redw;
}

max30102_t *max_init(void)
{
    max30102_t *mx = malloc(sizeof(max30102_t));
    mx->i2c = i2c_open(0x57);
    if (mx->i2c < 0) {
        free(mx);
        fprintf(stderr, "Unable to start I2C MAX30102: %s\n", strerror(errno));
        return NULL;
    }
    mx->pdstate = 0;
    mx->redcurrent = 0x80;
    mx->ircurrent = 0x80;
    mx->irw = 0;
    mx->ir = 0;
    mx->redw = 0;
    mx->red = 0;
    set_mode(mx, 0x03);
    set_spo2_conf(mx, (0x00 << 5) | (0x01 << 2) | 0x03); /* ADC range | samplerate | pulsewidth */
    set_currents(mx);
    return mx;
}

/* vim: ai:si:expandtab:ts=4:sw=4
 */
