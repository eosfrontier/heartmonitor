
#ifndef USI_I2C_MASTER_H
#define USI_I2C_MASTER_H

#define PIN_SDA  0
#define PIN_SCL  2
#define I2C_PORT PORTB
#define I2C_DDR  DDRB
#define I2C_PIN  PINB

unsigned char I2C_Master_Write(unsigned char addr, unsigned char reg, unsigned char *msg, unsigned char msg_size);
unsigned char I2C_Master_Read(unsigned char addr, unsigned char reg, unsigned char *msg, unsigned char msg_size);

#endif
/* vim: ai:si:expandtab:ts=4:sw=4
 */
