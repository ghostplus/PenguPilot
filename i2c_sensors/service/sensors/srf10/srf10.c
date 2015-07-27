/*___________________________________________________
 |  _____                       _____ _ _       _    |
 | |  __ \                     |  __ (_) |     | |   |
 | | |__) |__ _ __   __ _ _   _| |__) || | ___ | |_  |
 | |  ___/ _ \ '_ \ / _` | | | |  ___/ | |/ _ \| __| |
 | | |  |  __/ | | | (_| | |_| | |   | | | (_) | |_  |
 | |_|   \___|_| |_|\__, |\__,_|_|   |_|_|\___/ \__| |
 |                   __/ |                           |
 |  GNU/Linux based |___/  Multi-Rotor UAV Autopilot |
 |___________________________________________________|
  
 SRF10 Driver Implementation

 Copyright (C) 2014 Tobias Simon, Integrated Communication Systems Group, TU Ilmenau

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details. */


#include "srf10.h"

#include <util.h>

//four ultrasonic sensors SRF10
#define SRF10_ADDRESS0       0x71
#define SRF10_ADDRESS1       0x72
#define SRF10_ADDRESS2       0x73
#define SRF10_ADDRESS3       0x74

#define SRF10_CMD_REGISTER  0x00
#define SRF10_RANGE_COMMAND 0x51
#define SRF10_READ          0x02
#define SRF10_MIN_RANGE     0.2f
#define SRF10_MAX_RANGE     5.0f
#define SRF10_M_SCALE        1.0e-2f

#define SRF10_RANGE_REGISTER 0x02
#define SRF10_RANGE          0x5D
#define SRF10_GAIN_REGISTER  0x01
#define SRF10_GAIN           0x09



int srf10_init(srf10_t *srf10, i2c_bus_t *bus)
{
   /* copy values */
   i2c_dev_init(&srf10->i2c_dev0, bus, SRF10_ADDRESS1);
   i2c_dev_init(&srf10->i2c_dev1, bus, SRF10_ADDRESS2);
   i2c_dev_init(&srf10->i2c_dev2, bus, SRF10_ADDRESS3);
   i2c_dev_init(&srf10->i2c_dev3, bus, SRF10_ADDRESS4);
   
   //set range register and gain register
   THROW_BEGIN();
   
   THROW_ON_ERR(i2c_write_reg(&srf10->i2c_dev0, SRF10_RANGE_REGISTER, SRF10_RANGE));  
   THROW_ON_ERR(i2c_write_reg(&srf10->i2c_dev0, SRF10_GAIN_REGISTER, SRF10_GAIN));  
      
   THROW_ON_ERR(i2c_write_reg(&srf10->i2c_dev1, SRF10_RANGE_REGISTER, SRF10_RANGE));  
   THROW_ON_ERR(i2c_write_reg(&srf10->i2c_dev1, SRF10_GAIN_REGISTER, SRF10_GAIN));  
   
   THROW_ON_ERR(i2c_write_reg(&srf10->i2c_dev2, SRF10_RANGE_REGISTER, SRF10_RANGE));  
   THROW_ON_ERR(i2c_write_reg(&srf10->i2c_dev2, SRF10_GAIN_REGISTER, SRF10_GAIN));  
      
   THROW_ON_ERR(i2c_write_reg(&srf10->i2c_dev3, SRF10_RANGE_REGISTER, SRF10_RANGE));  
   THROW_ON_ERR(i2c_write_reg(&srf10->i2c_dev3, SRF10_GAIN_REGISTER, SRF10_GAIN));  
   
   THROW_END();
   
   return 0;
}


int srf10_read(srf10_t *srf10, vec_t *distance)
{
   THROW_BEGIN();
   /* start measurement: */
   THROW_ON_ERR(i2c_write_reg(&srf10->i2c_dev0, SRF10_CMD_REGISTER, SRF10_RANGE_COMMAND));
   THROW_ON_ERR(i2c_write_reg(&srf10->i2c_dev1, SRF10_CMD_REGISTER, SRF10_RANGE_COMMAND));
   THROW_ON_ERR(i2c_write_reg(&srf10->i2c_dev2, SRF10_CMD_REGISTER, SRF10_RANGE_COMMAND));
   THROW_ON_ERR(i2c_write_reg(&srf10->i2c_dev3, SRF10_CMD_REGISTER, SRF10_RANGE_COMMAND));

   msleep(80);  //value from datasheet is 65 ms, depends on values in the range and gain registers
   
   /* read back the result: */
   uint8_t raw[2];
   THROW_ON_ERR(i2c_read_block_reg(&srf10->i2c_dev0, SRF10_READ, raw, sizeof(raw)));
   distance->ve[0] = limit(SRF10_M_SCALE * (float)((raw[0] << 8) | raw[1]), SRF10_MIN_RANGE, SRF10_MAX_RANGE);
   
   THROW_ON_ERR(i2c_read_block_reg(&srf10->i2c_dev1, SRF10_READ, raw, sizeof(raw)));
   distance->ve[1] = limit(SRF10_M_SCALE * (float)((raw[0] << 8) | raw[1]), SRF10_MIN_RANGE, SRF10_MAX_RANGE);

   THROW_ON_ERR(i2c_read_block_reg(&srf10->i2c_dev2, SRF10_READ, raw, sizeof(raw)));
   distance->ve[2] = limit(SRF10_M_SCALE * (float)((raw[0] << 8) | raw[1]), SRF10_MIN_RANGE, SRF10_MAX_RANGE);

   THROW_ON_ERR(i2c_read_block_reg(&srf10->i2c_dev3, SRF10_READ, raw, sizeof(raw)));
   distance->ve[3] = limit(SRF10_M_SCALE * (float)((raw[0] << 8) | raw[1]), SRF10_MIN_RANGE, SRF10_MAX_RANGE);   
      
   THROW_END();
   
}

