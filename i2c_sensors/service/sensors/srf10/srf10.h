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


#ifndef __SRF10_H__
#define __SRF10_H__


#include <i2c/i2c.h>
#include <math/vec.h>

typedef struct
{
   /* i2c device: */
   i2c_dev_t i2c_dev0;
   i2c_dev_t i2c_dev1;
   i2c_dev_t i2c_dev2;
   i2c_dev_t i2c_dev3;
}
srf10_t;


int srf10_init(srf10_t *srf10, i2c_bus_t *bus);

int srf10_read(srf10_t *srf10, vec_t *distance);


#endif /* __SRF10_H__ */

