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
  
 I2CXL Reader Implementation

 Copyright (C) 2014 Tobias Simon, Integrated Communication Systems Group, TU Ilmenau

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details. */



#include <simple_thread.h>
#include <threadsafe_types.h>
#include <scl.h>
#include <msgpack.h>
#include <util.h>
#include <math/vec.h>
#include <pp_prio.h>

#include "../platform/platform.h"
#include "sonar_emitter.h"


static simple_thread_t thread;
static void *sonar_raw_socket;
 

SIMPLE_THREAD_BEGIN(thread_func)
{
   MSGPACK_PACKER_DECL_INFUNC();
   SIMPLE_THREAD_LOOP_BEGIN
   {
      msleep(30);
      vec_t vec;
	  vec_alloc(&vec, 4);
	  
      int ret = platform_read_sonar(&vec);
      msgpack_sbuffer_clear(msgpack_buf);
      if (status == 0)
         msgpack_pack_array(pk, 4);
         PACKFV(vec.ve, 4);
      else
         continue;

		 scl_copy_send_dynamic(sonar_raw_socket, msgpack_buf->data, msgpack_buf->size);
   }
   SIMPLE_THREAD_LOOP_END
}
SIMPLE_THREAD_END


int sonar_emitter_start(void)
{
   ASSERT_ONCE();
   THROW_BEGIN();
   sonar_raw_socket = scl_get_socket("sonar_raw", "pub");
   THROW_IF(sonar_raw_socket == NULL, -EIO);
   simple_thread_start(&thread, thread_func, "sonar_emitter", PP_PRIO_1, NULL);
   THROW_END();
}

