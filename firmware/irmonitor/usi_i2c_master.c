/*-----------------------------------------------------*\
|  USI I2C Slave Master                                 |
|                                                       |
| This library provides a robust I2C master protocol    |
| implementation on top of Atmel's Universal Serial     |
| Interface (USI) found in many ATTiny microcontrollers.|
|                                                       |
| Adam Honse (GitHub: CalcProgrammer1) - 7/29/2012      |
|            -calcprogrammer1@gmail.com                 |
\*-----------------------------------------------------*/

#include "usi_i2c_master.h"
#include <avr/interrupt.h>
///////////////////////////////////////////////////////////////////////////////
////USI Master Macros//////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#define USISR_TRANSFER_8_BIT         0b11110000 | (0x00<<USICNT0)
#define USISR_TRANSFER_1_BIT         0b11110000 | (0x0E<<USICNT0)

#define USICR_CLOCK_STROBE_MASK        0b00101011

#define USI_CLOCK_STROBE()            { USICR = USICR_CLOCK_STROBE_MASK; }

#define USI_SET_SDA_OUTPUT()        { DDR_USI |=  (1 << PORT_USI_SDA); }
#define USI_SET_SDA_INPUT()         { DDR_USI &= ~(1 << PORT_USI_SDA); }

#define USI_SET_SDA_HIGH()            { PORT_USI |=  (1 << PORT_USI_SDA); }
#define USI_SET_SDA_LOW()            { PORT_USI &= ~(1 << PORT_USI_SDA); }

#define USI_SET_SCL_OUTPUT()        { DDR_USI |=  (1 << PORT_USI_SCL); }
#define USI_SET_SCL_INPUT()         { DDR_USI &= ~(1 << PORT_USI_SCL); }

#define USI_SET_SCL_HIGH()            { PORT_USI |=  (1 << PORT_USI_SCL); }
#define USI_SET_SCL_LOW()            { PORT_USI &= ~(1 << PORT_USI_SCL); }

#define USI_I2C_WAIT_HIGH()            { _delay_us(I2C_THIGH); }
#define USI_I2C_WAIT_LOW()            { _delay_us(I2C_TLOW);  }

/////////////////////////////////////////////////////////////////////
// USI_I2C_Master_Transfer                                         //
//  Transfers either 8 bits (data) or 1 bit (ACK/NACK) on the bus. //
/////////////////////////////////////////////////////////////////////

static char USI_I2C_Master_Transfer(char USISR_temp)
{
    USISR = USISR_temp;                                //Set USISR as requested by calling function

    // Shift Data
    do
    {
        USI_I2C_WAIT_LOW();
        USI_CLOCK_STROBE();                                //SCL Positive Edge
        while (!(PIN_USI&(1<<PIN_USI_SCL)));        //Wait for SCL to go high
        USI_I2C_WAIT_HIGH();
        USI_CLOCK_STROBE();                                //SCL Negative Edge
    } while (!(USISR&(1<<USIOIF)));                    //Do until transfer is complete
    
    USI_I2C_WAIT_LOW();

    return USIDR;
}

static void USI_I2C_Send_Stop(void)
{
    USI_SET_SDA_LOW();                           // Pull SDA low.
    USI_I2C_WAIT_LOW();
    USI_SET_SCL_INPUT();                         // Release SCL.
    while( !(PIN_USI & (1<<PIN_USI_SCL)) );      // Wait for SCL to go high.
    USI_I2C_WAIT_HIGH();
    USI_SET_SDA_INPUT();                         // Release SDA.
    while( !(PIN_USI & (1<<PIN_USI_SDA)) );      // Wait for SDA to go high. 
}

char USI_I2C_Master_Write(unsigned char *msg, char msg_size)
{
    char dostop = 1;
    if (msg_size <= 2) { dostop = 0; }
    /////////////////////////////////////////////////////////////////
    //  Generate Start Condition                                   //
    /////////////////////////////////////////////////////////////////

    USI_SET_SCL_HIGH();                         //Setting input makes line pull high

    while (!(PIN_USI & (1<<PIN_USI_SCL)));        //Wait for SCL to go high

    #ifdef I2C_FAST_MODE
        USI_I2C_WAIT_HIGH();
    #else
        USI_I2C_WAIT_LOW();
    #endif
    USI_SET_SDA_OUTPUT();
    USI_SET_SCL_OUTPUT();
    USI_SET_SDA_LOW();
    USI_I2C_WAIT_HIGH();
    USI_SET_SCL_LOW();
    USI_I2C_WAIT_LOW();
    USI_SET_SDA_HIGH();
    
    /////////////////////////////////////////////////////////////////

    /* Write loop */
    while (msg_size--) {
        ///////////////////////////////////////////////////////////////////
        // Write Operation                                               //
        //  Writes a byte to the slave and checks for ACK                //
        //  If no ACK, then reset and exit                               //
        ///////////////////////////////////////////////////////////////////

        USI_SET_SCL_LOW();

        USIDR = *msg;                //Load data
    
        USI_I2C_Master_Transfer(USISR_TRANSFER_8_BIT);

        USI_SET_SDA_INPUT();

        if(USI_I2C_Master_Transfer(USISR_TRANSFER_1_BIT) & 0x01)
        {
            USI_I2C_Send_Stop();
            return 0x01;
        }

        USI_SET_SDA_OUTPUT();
        msg++;
    }

    /////////////////////////////////////////////////////////////////
    // Send Stop Condition                                         //
    /////////////////////////////////////////////////////////////////

    if (dostop) USI_I2C_Send_Stop();
    return 0;
}

char USI_I2C_Master_Read(unsigned char *msg, char msg_size)
{
    // Wrte address
    if (USI_I2C_Master_Write(msg, 2)) {
        return 0x1;
    }
    
    USI_SET_SDA_HIGH();
    while (!(PIN_USI & (1<<PIN_USI_SDA)));

    USI_SET_SCL_HIGH();
    while (!(PIN_USI & (1<<PIN_USI_SCL)));

    #ifdef I2C_FAST_MODE
        USI_I2C_WAIT_HIGH();
    #else
        USI_I2C_WAIT_LOW();
    #endif
    USI_SET_SDA_LOW();
    USI_I2C_WAIT_HIGH();
    USI_SET_SCL_LOW();
    USI_I2C_WAIT_LOW();
    USI_SET_SDA_HIGH();

    USIDR = *msg|1;              //Load data

    USI_I2C_Master_Transfer(USISR_TRANSFER_8_BIT);

    USI_SET_SDA_INPUT();

    if(USI_I2C_Master_Transfer(USISR_TRANSFER_1_BIT) & 0x01)
    {
        USI_I2C_Send_Stop();
        return 0x02;
    }

    msg += 2;
    msg_size -= 2;

    /* Read loop.  Only entered if the above loop is broken out of */
    while (msg_size--) {
        ///////////////////////////////////////////////////////////////////
        // Read Operation                                                //
        //  Reads a byte from the slave and sends ACK or NACK            //
        ///////////////////////////////////////////////////////////////////

        USI_SET_SDA_INPUT();

        *msg = USI_I2C_Master_Transfer(USISR_TRANSFER_8_BIT);

        USI_SET_SDA_OUTPUT();
        
        if(msg_size) {
            USIDR = 0x00;            //Load ACK
        } else {
            USIDR = 0xFF;            //Load NACK to end transmission
        }

        USI_I2C_Master_Transfer(USISR_TRANSFER_1_BIT);
        msg++;
    }
    
    /////////////////////////////////////////////////////////////////
    // Send Stop Condition                                         //
    /////////////////////////////////////////////////////////////////

    USI_I2C_Send_Stop();

    return 0;
}

/* vim: ai:si:expandtab:ts=4:sw=4
 */
