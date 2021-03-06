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
  
 Kalman Filter based Position/Speed Estimate

 System Model:

 | 1 dt | * | p | + | 0.5 * dt ^ 2 | * | a | = | p |
 | 0  1 | * | v |   |     dt       |   | v |

 Copyright (C) 2014 Tobias Simon, Integrated Communication Systems Group, TU Ilmenau
 Copyright (C) 2013 Jan Roemisch, Integrated Communication Systems Group, TU Ilmenau

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details. */


#include <stdbool.h>
#include <math.h>
#include <util.h>
#include <simple_thread.h>
#include <opcd_interface.h>
#include <threadsafe_types.h>

#include "pos.h"
#include <logger.h>
#include <math/mat.h>


/* configuration parameters: */
static tsfloat_t process_noise;
static tsfloat_t ultra_noise;
static tsfloat_t baro_noise;
static tsfloat_t gps_noise;
static tsint_t use_gps_speed;

VEC_DECL(1);

MAT_DECL(2, 2);
MAT_DECL(2, 1);


typedef struct kalman
{
   /* configuration and constant matrices: */
   mat2x2_t Q; /* process noise */
   mat2x2_t R; /* measurement noise */
   mat2x2_t I; /* identity matrix */

   /* state and transition vectors/matrices: */
   vec2_t x; /* state (location and velocity) */
   vec2_t z; /* measurement (location) */
   vec1_t u; /* control (acceleration) */
   mat2x2_t P; /* error covariance */
   mat2x2_t A; /* system matrix */
   mat2x1_t B; /* control matrix */
   mat2x2_t H; /* observer matrix */
   mat2x2_t K; /* kalman gain */

   /*  vectors and matrices for calculations: */
   vec2_t t0;
   vec2_t t1;
   mat2x2_t T0;
   mat2x2_t T1;

   /* online adaptable parameters: */
   tsfloat_t *q;
   tsfloat_t *r;

   bool use_speed;
}
kalman_t;


/* static function prototypes: */
static void kalman_init(kalman_t *kf, tsfloat_t *q, tsfloat_t *r,
                        float pos, float speed, bool use_speed);

static void kalman_run(kalman_t *kf, float *est_pos, float *est_speed,
                       float pos, float speed, float acc, float dt);


/* kalman filters: */
static kalman_t n_kalman;
static kalman_t e_kalman;
static kalman_t baro_u_kalman;
static kalman_t ultra_u_kalman;
static float ultra_prev = 0.0f;
static float baro_prev = 0.0f;


void pos_init(void)
{
   ASSERT_ONCE();

   /* read configuration and initialize scl gates: */
   opcd_param_t params[] =
   {
      {"process_noise", &process_noise},
      {"ultra_noise", &ultra_noise},
      {"baro_noise", &baro_noise},
      {"gps_noise", &gps_noise},
      {"use_gps_speed", &use_gps_speed},
      OPCD_PARAMS_END
   };
   opcd_params_apply(".", params);
   LOG(LL_DEBUG, "process noise: %f, ultra noise: %f, baro noise: %f, gps noise: %f",
       tsfloat_get(&process_noise),
       tsfloat_get(&ultra_noise),
       tsfloat_get(&baro_noise),
       tsfloat_get(&gps_noise));

   /* set-up kalman filters: */
   kalman_init(&e_kalman, &process_noise, &gps_noise, 0, 0, tsint_get(&use_gps_speed));
   kalman_init(&n_kalman, &process_noise, &gps_noise, 0, 0, tsint_get(&use_gps_speed));
   kalman_init(&baro_u_kalman, &process_noise, &baro_noise, 0, 0, false);
   kalman_init(&ultra_u_kalman, &process_noise, &ultra_noise, 0, 0, false);
}


void pos_update(pos_t *out, pos_in_t *in)
{
   ASSERT_NOT_NULL(out);
   ASSERT_NOT_NULL(in);

   /* run kalman filters: */
   kalman_run(&n_kalman,       &out->ne_pos.n,    &out->ne_speed.n,    in->pos_n,   in->speed_n, in->acc.n, in->dt);
   kalman_run(&e_kalman,       &out->ne_pos.e,    &out->ne_speed.e,    in->pos_e,   in->speed_e, in->acc.e, in->dt);
   kalman_run(&baro_u_kalman,  &out->baro_u.pos,  &out->baro_u.speed,  in->baro_u,  0.0f, in->acc.u, in->dt);
   //if (fabs(in->ultra_u - ultra_prev) > 10.0 * fabs(in->baro_u - baro_prev))
   //   kalman_run(&ultra_u_kalman, &out->ultra_u.pos, &out->ultra_u.speed, ultra_prev + in->baro_u - baro_prev, 0.0f, in->acc.u, in->dt);
   //else
      kalman_run(&ultra_u_kalman, &out->ultra_u.pos, &out->ultra_u.speed, in->ultra_u, 0.0f, in->acc.u, in->dt);
   baro_prev = out->baro_u.pos;
   ultra_prev = out->ultra_u.pos;
}


void kalman_init(kalman_t *kf, tsfloat_t *q, tsfloat_t *r, float pos, float speed, bool use_speed)
{
   kf->use_speed = use_speed;
   kf->q = q;
   kf->r = r;

   /* set up temporary vectors and matrices: */
   vec2_init(&kf->t0);
   vec2_init(&kf->t1);
   mat2x2_init(&kf->T0);
   mat2x2_init(&kf->T1);
   
   mat2x2_init(&kf->I);
   mat_ident(&kf->I);

   /* set initial state: */
   vec2_init(&kf->x);
   kf->x.ve[0] = pos;
   kf->x.ve[1] = speed;

   /* no measurement or control yet: */
   vec2_init(&kf->z);
   vec1_init(&kf->u);

   mat2x2_init(&kf->P);
   mat_ident(&kf->P);
   
   /* set up noise: */
   mat2x2_init(&kf->Q);
   mat2x2_init(&kf->R);
   
   /* initialize kalman gain: */
   mat2x2_init(&kf->K);

   /* H = | 1.0   0.0       |
          | 0.0   use_speed | */
   mat2x2_init(&kf->H);
   kf->H.me[0][0] = 1.0f;
   kf->H.me[1][1] = 1.0f * use_speed;

   /* A = | 1.0   dt  |
          | 0.0   1.0 |
      note: dt value is set in kalman_run */
   mat2x2_init(&kf->A);
   kf->A.me[0][0] = 1.0f;
   kf->A.me[1][1] = 1.0f;

   /* B = | 0.5 * dt ^ 2 |
          |     dt       |
      values are set in in kalman_run */
   mat2x1_init(&kf->B);
}


static void kalman_predict(kalman_t *kf, float a)
{
   /* x = A * x + B * u */
   kf->u.ve[0] = a;
   mat_vec_mul(&kf->t0, &kf->A, &kf->x); /* t0 = A * x */
   mat_vec_mul(&kf->t1, &kf->B, &kf->u); /* t1 = B * u */
   vec_add(&kf->x, &kf->t0, &kf->t1);    /* x = t0 + t1 */

   /* P = A * P * AT + Q */
   mat_mul(&kf->T0, &kf->A, &kf->P);   /* T0 = A * P */
   mmtr_mul(&kf->T1, &kf->T0, &kf->A); /* T1 = T0 * AT */
   mat_add(&kf->P, &kf->T1, &kf->Q);   /* P = T1 * Q */
}



static void kalman_correct(kalman_t *kf, float pos, float speed)
{
   /* update H matrix: */
   kf->H.me[1][1] = 1.0f * (kf->use_speed && speed != 0.0f);
   kf->z.ve[0] = pos;
   kf->z.ve[1] = speed;

   /* K = P * HT * inv(H * P * HT + R) */
   mat_mul(&kf->T0, &kf->H, &kf->P);   // T0 = H * P
   mmtr_mul(&kf->T1, &kf->T0, &kf->H); // T1 = T0 * HT
   mat_add(&kf->T0, &kf->T1, &kf->R);  // T0 = T1 + R
   mat_inv(&kf->T1, &kf->T0);          // T1 = inv(T0)
   mmtr_mul(&kf->T0, &kf->P, &kf->H);  // T0 = P * HT
   mat_mul(&kf->K, &kf->T0, &kf->T1);  // K = T0 * T1

   /* x = x + K * (z - H * x) */
   mat_vec_mul(&kf->t0, &kf->H, &kf->x);  // t0 = H * x
   vec_sub(&kf->t1, &kf->z, &kf->t0);     // t1 = z - t0
   mat_vec_mul(&kf->t0, &kf->K, &kf->t1); // t0 = K * t1
   vec_add(&kf->x, &kf->x, &kf->t0);      // x = x + t0

   /* P = (I - K * H) * P */
   mat_mul(&kf->T0, &kf->K, &kf->H);  // T0 = K * H
   mat_sub(&kf->T1, &kf->I, &kf->T0); // T1 = I - T0
   mat_mul(&kf->T0, &kf->T1, &kf->P); // T0 = T1 * P
   mat_copy(&kf->P, &kf->T0);         // P = T0
}


/*
 * executes kalman predict and correct step
 */
static void kalman_run(kalman_t *kf, float *est_pos, float *est_speed, float pos, float speed, float acc, float dt)
{
   /* A = | init   dt  |
          | init  init | */
   kf->A.me[0][1] = dt;

   /* B = | 0.5 * dt ^ 2 |
          |     dt       | */
   kf->B.me[0][0] = 0.5f * dt * dt;
   kf->B.me[1][0] = dt;
   
   /* Q, R: */
   mat_scalar_mul(&kf->Q, &kf->I, tsfloat_get(kf->q));
   mat_scalar_mul(&kf->R, &kf->I, tsfloat_get(kf->r));
 
   kalman_predict(kf, acc);
   kalman_correct(kf, pos, speed);
   *est_pos = kf->x.ve[0];
   *est_speed = kf->x.ve[1];
}

