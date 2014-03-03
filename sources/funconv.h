/* SECU-3  - An open source, free engine control unit
   Copyright (C) 2007 Alexey A. Shabelnikov. Ukraine, Gorlovka

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   contacts:
              http://secu-3.org
              email: shabelnikov@secu-3.org
*/

/** \file funconv.h
 * Core mathematics and regulation logic.
 * (�������� ����� ��������������� �������� � ������ �������������).
 */

#ifndef _FUNCONV_H_
#define _FUNCONV_H_

#include <stdint.h>
#include "vstimer.h"

/** f(x) liniar interpolation for function with single argument
 * \param x argument value
 * \param a1 function value at the beginning of interval
 * \param a2 function value at the end of interval
 * \param x_s argument value at the beginning of interval
 * \param x_l length of interval in x
 * \param m function multiplier
 * \return interpolated value of function * m
 */
int16_t simple_interpolation(int16_t x,int16_t a1,int16_t a2,int16_t x_s,int16_t x_l, uint8_t m);

/** f(x,y) liniar interpolation for function with two arguments
 * \param x first argument value
 * \param y second argument value
 * \param a1 function value at the beginning of interval (1 corner)
 * \param a2 function value at the beginning of interval (2 corner)
 * \param a3 function value at the beginning of interval (3 corner)
 * \param a4 function value at the beginning of interval (4 corner)
 * \param x_s first argument value at the beginning of interval
 * \param y_s second argument value at the beginning of interval
 * \param x_l length of interval in x
 * \param y_l length of interval in y
 * \return interpolated value of function * 16
 */
int16_t bilinear_interpolation(int16_t x,int16_t y,int16_t a1,int16_t a2,int16_t a3,int16_t a4,int16_t x_s,int16_t y_s,int16_t x_l,int16_t y_l);

struct ecudata_t;

/** Calculates advance angle from "start" map
 * \param d pointer to ECU data structure
 * \return value of advance angle * 32
 */
int16_t start_function(struct ecudata_t* d);

/** Calculates advance angle from "idle" map
 * \param d pointer to ECU data structure
 * \return value of advance angle * 32
 */
int16_t idling_function(struct ecudata_t* d);

/** Calculates advance angle from "work" map
 * \param d pointer to ECU data structure
 * \param i_update_airflow_only
 * \return value of advance angle * 32
 */
int16_t work_function(struct ecudata_t* d, uint8_t i_update_airflow_only);

/** Calculates advance angle correction using coolant temperature
 * \param d pointer to ECU data structure
 * \return value of advance angle * 32
 */
int16_t coolant_function(struct ecudata_t* d);

/** Knock attenuator look up table function
 * \param d pointer to ECU data structure
 * \return
 */
uint8_t knock_attenuator_function(struct ecudata_t* d);

/**Initialization of idling regulator's data structures */
void idling_regulator_init(void);

/** Idling RPM Regulator function
 * \param d pointer to ECU data structure
 * \param io_timer
 * \return value of advance angle * 32
 */
int16_t idling_pregulator(struct ecudata_t* d, volatile s_timer8_t* io_timer);

/** function for restricting of advance angle alternation speed
 * \param new_advance_angle New value of advance angle (input)
 * \param ip_prev_state state variable for storing value between calls of function
 * \param intstep_p Speed limit for increasing
 * \param intstep_m Speed limit for decreasing
 * \return value of advance angle * 32
 */
int16_t advance_angle_inhibitor(int16_t new_advance_angle, int16_t* ip_prev_state, int16_t intstep_p, int16_t intstep_m);

/** Restricts specified value to specified limits
 * \param io_value pointer to value to be restricted. This parameter will also receive result.
 * \param i_bottom_limit bottom limit
 * \param i_top_limit upper limit
 */
void restrict_value_to(int16_t *io_value, int16_t i_bottom_limit, int16_t i_top_limit);

#ifdef DWELL_CONTROL
/** Calculates current accumulation time (dwell control) using current board voltage
 * \param d pointer to ECU data structure
 * \return accumulation time in timer's ticks (1 tick = 4uS, when clock is 16mHz and 1 tick = 3.2uS, when clock is 20mHz)
 */
uint16_t accumulation_time(struct ecudata_t* d);
#endif

#ifdef THERMISTOR_CS
/**Converts ADC value into phisical magnitude - temperature (given from thermistor)
 * (��������� �������� ��� � ���������� �������� - ����������� ��� ������������ ������� (���������))
 * \param adcvalue Voltage from sensor (���������� � ������� - �������� � ��������� ���))
 * \return ���������� �������� * TEMP_PHYSICAL_MAGNITUDE_MULTIPLAYER
 */
int16_t thermistor_lookup(uint16_t adcvalue);
#endif

#ifdef SM_CONTROL
/** Obtains choke position (closing %) from coolant temperature using lookup table
 * �������� ��������� ��������� �������� (% ��������) �� ���������� ����������� ��������
 * \param d pointer to ECU data structure
 * \param p_prev_temp pointer to state variable used to store temperature value between calls of
 * this function
 * \return choke closing percentage (value * 2)
 */
uint8_t choke_closing_lookup(struct ecudata_t* d, int16_t* p_prev_temp);

/**Initialization of regulator's data structures*/
void chokerpm_regulator_init(void);

/** RPM regulator function for choke position
 * \param d pointer to ECU data structure
 * \param p_prev_corr pointer to state variable used to store calculated correction between calls of
 * this function
 * \return choke closing correction in SM steps
 */
int16_t choke_rpm_regulator(struct ecudata_t* d, int16_t* p_prev_corr);
#endif

#if defined(AIRTEMP_SENS) && defined(SECU3T)
/** Calculates advance angle correction using intake air temperature
 * \param d pointer to ECU data structure
 * \return value of advance angle * 32
 */
int16_t airtemp_function(struct ecudata_t* d);

/**Converts ADC value into phisical magnitude - temperature (given from air temperature sensor)
 * (��������� �������� ��� � ���������� �������� - ����������� �������)
 * \param adcvalue Voltage from sensor (���������� � ������� - �������� � ��������� ���))
 * \return ���������� �������� * TEMP_PHYSICAL_MAGNITUDE_MULTIPLAYER
 */
int16_t ats_lookup(uint16_t adcvalue);
#endif

#endif //_FUNCONV_H_
