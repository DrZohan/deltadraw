#ifndef I2C_H
#define I2C_H

#include "mbed.h"
#include "common.h"

#define I2C_ERROR 0xFF

S32 i2c_read(S32 address);

#endif