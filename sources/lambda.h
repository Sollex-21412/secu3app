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

/** \file lambda.h
 * Correction algorithms using an exhaust gas oxygen sensor
 * (������������� ������� ����� ��������� ������ ���������).
 */

#ifndef _LAMBDA_H_
#define _LAMBDA_H_

#ifdef FUEL_INJECT

struct ecudata_t;

/** Initialization of state variables
 */
void lambda_init_state(void);

/**Control of lambda correction
 * \param d pointer to ECU data structure
 */
void lambda_control(struct ecudata_t* d);

/** Must be called from the main loop to notify about stroke events
 * \param d pointer to ECU data structure
 */
void lambda_stroke_event_notification(struct ecudata_t* d);

#endif

#endif //_LAMBDA_H_
