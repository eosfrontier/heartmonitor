
#ifndef USI_I2C_MASTER_H
#define USI_I2C_MASTER_H

unsigned char I2C_Master_Write(unsigned char addr, unsigned char reg, unsigned char *msg, unsigned char msg_size);
unsigned char I2C_Master_Read(unsigned char addr, unsigned char reg, unsigned char *msg, unsigned char msg_size);

#endif
/* vim: ai:si:expandtab:ts=4:sw=4
 */
