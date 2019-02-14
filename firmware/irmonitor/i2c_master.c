#include <avr/io.h>
#include <util/delay.h>
#include <string.h>
#include "i2c_master.h"

#define cbi(p,b) asm volatile ( "cbi %[port], %[bit] \n\t" :: [port] "I" (_SFR_IO_ADDR(p)),[bit] "I" (b) )
#define sbi(p,b) asm volatile ( "sbi %[port], %[bit] \n\t" :: [port] "I" (_SFR_IO_ADDR(p)),[bit] "I" (b) )
#define SDA_HI() cbi(I2C_DDR,PIN_SDA)
#define SCL_HI() cbi(I2C_DDR,PIN_SCL)
#define SDA_LO() sbi(I2C_DDR,PIN_SDA)
#define SCL_LO() sbi(I2C_DDR,PIN_SCL)

static void i2c_start_condition()
{
    SDA_HI();
    SCL_HI();
    cbi(I2C_PORT,PIN_SCL);
    cbi(I2C_PORT,PIN_SDA);
    _delay_us(5);
    SDA_LO();
    _delay_us(5);
    SCL_LO();
}

static void i2c_stop_condition()
{
    SDA_LO();
    _delay_us(5);
    SCL_HI();
    _delay_us(5);
    SDA_HI();
    _delay_us(5);
}

static unsigned char i2c_write_byte(unsigned char b)
{
    unsigned char i;
    for (i = 0x80; i; i >>= 1) {
        if (b & i) {
            SDA_HI();
        } else {
            SDA_LO();
        }
        _delay_us(3);
        SCL_HI();
        _delay_us(3);
        while (!(I2C_PIN & (1 << PIN_SCL))) { /* Wait */ }
        SCL_LO();
        _delay_us(3);
    }
    SDA_HI();
    _delay_us(3);
    SCL_HI();
    _delay_us(3);
    while (!(I2C_PIN & (1 << PIN_SCL))) { /* Wait */ }
    _delay_us(3);
    if (I2C_PIN & (1 << PIN_SDA)) {
        return 1;
    }
    SCL_LO();
    return 0;
}

static unsigned char i2c_read_byte(unsigned char ack)
{
    unsigned char i, b;
    SDA_HI();
    _delay_us(3);
    b = 0;
    for (i = 0x80; i; i >>= 1) {
        SCL_HI();
        _delay_us(3);
        while (!(I2C_PIN & (1 << PIN_SCL))) { /* Wait */ }
        if (I2C_PIN & (1 << PIN_SDA)) {
            b |= i;
        }
        _delay_us(3);
        SCL_LO();
        _delay_us(3);
    }
    if (ack) SDA_LO();
    else SDA_HI();
    _delay_us(3);
    SCL_HI();
    _delay_us(3);
    while (!(I2C_PIN & (1 << PIN_SCL))) { /* Wait */ }
    _delay_us(3);
    SCL_LO();
    _delay_us(3);
    SDA_LO();
    return b;
}

unsigned char I2C_Master_Write(unsigned char addr, unsigned char reg, unsigned char *msg, unsigned char msg_size)
{
    i2c_start_condition();
    if (i2c_write_byte(addr << 1)) return 0x01;
    if (i2c_write_byte(reg)) return 0x02;
    while (msg_size--) {
        if (i2c_write_byte(*msg++)) return 0x04;
    }
    i2c_stop_condition();
    return 0;
}

unsigned char I2C_Master_Read(unsigned char addr, unsigned char reg, unsigned char *msg, unsigned char msg_size)
{
    i2c_start_condition();
    if (i2c_write_byte(addr << 1)) return 0x10;
    if (i2c_write_byte(reg)) return 0x20;
    i2c_start_condition();
    if (i2c_write_byte((addr << 1) | 1)) return 0x40;
    while (msg_size--) {
        *msg++ = i2c_read_byte(msg_size);
    }
    i2c_stop_condition();
    return 0;
}
/* vim: ai:si:expandtab:ts=4:sw=4
 */
