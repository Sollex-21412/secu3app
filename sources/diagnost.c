/* SECU-3  - An open source, free engine control unit
   Copyright (C) 2007 Alexey A. Shabelnikov. Ukraine, Kiev

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

/** \file diagnost.c
 * \author Alexey A. Shabelnikov
 * Implementation of hardware diagnostics
 */

#ifdef DIAGNOSTICS

#include "port/avrio.h"
#include "port/intrinsic.h"
#include "bitmask.h"
#include "diagnost.h"
#include "ecudata.h"
#include "knock.h"
#include "magnitude.h"
#include "procuart.h"
#include "suspendop.h"
#include "uart.h"
#include "ufcodes.h"
#include "wdt.h"
#include "ioconfig.h"


#ifndef SECU3T //---SECU-3i---
extern uint8_t spi_PORTB;
extern uint8_t spi_PORTA;
extern uint8_t spi_IODIRA;
extern uint8_t spi_IODIRB;
extern uint8_t spi_GPPUA;
#endif

/**Helpful macro used for generation of bit mask for outputs*/
#define _OBV(b) (((uint32_t)1) << b)

/**Helpful macro used for generation of bit mask for inputs*/
#define _IBV(v,b) (((uint16_t)v) << b)

/**Helpful macro for ADC compensation, f,c - must be floationg point constants*/
#define _ADC_COMPENSATE(v, f, c) adc_compensate(v, ADC_COMP_FACTOR(f), ADC_COMP_CORR(f, c))

/**Describes state variables data*/
typedef struct
{
 uint8_t diag_started;   //!< flags indicates that diagnostic mode is started
 uint8_t fsm_state;      //!< finite state machine state
 uint8_t ksp_channel;    //!< number of current knock channel
 uint16_t knock_value[2];//!< latest measurements of two knock sensors
 uint8_t skip_loops;     //!< counter used for skipping of several measurement loop after entering diag. mode
}diag_state_t;

/**Instance of diagnostics state variables */
diag_state_t diag;

void diagnost_start(void)
{
 diag.diag_started = 1;
 diag.fsm_state = 0;
 diag.ksp_channel = KSP_CHANNEL_0;
 diag.skip_loops = 5;
 uart_set_send_mode(DIAGINP_DAT);
}

void diagnost_stop(void)
{
 if (diag.diag_started)
 {
  sop_set_operation(SOP_SEND_NC_LEAVE_DIAG);
  diag.diag_started = 0;
 }
}

/**Initialization of outputs in diagnostic mode. All outputs are OFF*/
void init_digital_outputs(void)
{
#ifdef SECU3T
 //IGN_OU1, IGN_OUT2, IGN_OUT3, IGN_OUT4
 PORTD&= ~(_BV(PD5)|_BV(PD4));
 PORTC&= ~(_BV(PC1)|_BV(PC0));
 DDRD|= (_BV(DDD5)|_BV(DDD4));
 DDRC|= (_BV(DDC1)|_BV(DDC0));
 //ADD_IO1, ADD_IO2
 PORTC&= ~(_BV(PC5));
 PORTA&= ~(_BV(PA4));
 DDRC|= (_BV(DDC5));
 DDRA|= (_BV(DDA4));
 //IE
 PORTB&= ~(_BV(PB0));
 DDRB|= _BV(DDB0);
 //FE
 PORTC&= ~_BV(PC7);
 DDRC|= _BV(DDC7);
 //ECF
#ifdef REV9_BOARD
 PORTD&= ~(_BV(PD7));
#else
 PORTD|= _BV(PD7);
#endif
 DDRD |= _BV(DDD7);
 //CE
#ifdef REV9_BOARD
 PORTB&= ~(_BV(PB2));
#else
 PORTB|= _BV(PB2);
#endif
 DDRB |= _BV(DDB2);
 //ST_BLOCK
#ifdef REV9_BOARD
 PORTB&= ~(_BV(PB1));
#else
 PORTB|= _BV(PB1);
#endif
 DDRB |= _BV(DDB1);

#else //---SECU-3i---

 //IGN_O1, IGN_O2, IGN_O3, IGN_O4, IGN_O5
 PORTD|= (_BV(PD5)|_BV(PD4));
 PORTC|= (_BV(PC1)|_BV(PC0)|_BV(PC5));
 DDRD|= (_BV(DDD5)|_BV(DDD4));
 DDRC|= (_BV(DDC1)|_BV(DDC0)|_BV(DDC5));

 //ECF
 PORTD&= ~(_BV(PD7));
 DDRD |= _BV(DDD7);

 //INJ_O1, INJ_O2, INJ_O3, INJ_O4, INJ_O5
 PORTB&= ~(_BV(PB2)|_BV(PB1)|_BV(PB0));
 DDRB|= (_BV(DDB2)|_BV(DDB1)|_BV(DDB0));
 PORTC&= ~(_BV(PC7)|_BV(PC6));
 DDRC|= (_BV(DDC7)|_BV(DDC6));

 //STBL_O, CEL_O, FPMP_O, PWRR_O, COND_O, O2SH_O, EVAP_O, ADD_O2
 spi_PORTB&= (uint8_t)(~(_BV(7)|_BV(6)|_BV(5)|_BV(4)|_BV(3)|_BV(2)|_BV(1)|_BV(0)));
 spi_IODIRB&= (uint8_t)(~(_BV(7)|_BV(6)|_BV(5)|_BV(4)|_BV(3)|_BV(2)|_BV(1)|_BV(0)));

#endif
}

/**Set states of outputs
 * \param o Bits containing output states
 */
void set_outputs(uint32_t o)
{

#ifdef SECU3T

 WRITEBIT(PORTD, PD4, (o & _OBV(0)));  //IGN_OUT1
 WRITEBIT(PORTD, PD5, (o & _OBV(1)));  //IGN_OUT2
 WRITEBIT(PORTC, PC0, (o & _OBV(2)));  //IGN_OUT3
 WRITEBIT(PORTC, PC1, (o & _OBV(3)));  //IGN_OUT4
 WRITEBIT(PORTB, PB0, (o & _OBV(4)));  //IE
 WRITEBIT(PORTC, PC7, (o & _OBV(5)));  //FE

#ifdef REV9_BOARD
 WRITEBIT(PORTD, PD7, (o & _OBV(6)));  //ECF
#else
 WRITEBIT(PORTD, PD7,!(o & _OBV(6)));
#endif

#ifdef REV9_BOARD
 WRITEBIT(PORTB, PB2, (o & _OBV(7)));  //CE
#else
 WRITEBIT(PORTB, PB2,!(o & _OBV(7)));
#endif

 //ST_BLOCK
#ifdef REV9_BOARD
 WRITEBIT(PORTB, PB1, (o & _OBV(8))); //ST_BLOCK
#else
 WRITEBIT(PORTB, PB1,!(o & _OBV(8)));
#endif

 WRITEBIT(PORTC, PC5, (o & _OBV(9)));  //ADD_IO1
 WRITEBIT(PORTA, PA4, (o & _OBV(10)));  //ADD_IO2

 //BL
 if (o & _OBV(12)) {
  WRITEBIT(PORTC, PC3, (o & _OBV(11))); }
 else {
  WRITEBIT(PORTC, PC3, 1); }             //pull up input
 WRITEBIT(DDRC, DDC3, (o & _OBV(12)));   //select mode: input/output

 //DE
 if (o & _OBV(14)) {
  WRITEBIT(PORTC, PC2, (o & _OBV(13))); }
 else {
  WRITEBIT(PORTC, PC2, 1); }             //pull up input
 WRITEBIT(DDRC, DDC2, (o & _OBV(14)));   //select mode: input/output

#else //---SECU-3i---

 WRITEBIT(PORTD, PD4, !(o & _OBV(0)));  //IGN_O1
 WRITEBIT(PORTD, PD5, !(o & _OBV(1)));  //IGN_O2
 WRITEBIT(PORTC, PC0, !(o & _OBV(2)));  //IGN_O3
 WRITEBIT(PORTC, PC1, !(o & _OBV(3)));  //IGN_O4
 WRITEBIT(PORTC, PC5, !(o & _OBV(4)));  //IGN_O5
 WRITEBIT(PORTD, PD7, (o & _OBV(5)));  //ECF
 WRITEBIT(PORTB, PB1, (o & _OBV(6)));  //INJ_O1
 WRITEBIT(PORTC, PC6, (o & _OBV(7)));  //INJ_O2
 WRITEBIT(PORTB, PB2, (o & _OBV(8)));  //INJ_O3
 WRITEBIT(PORTC, PC7, (o & _OBV(9)));  //INJ_O4
 WRITEBIT(PORTB, PB0, (o & _OBV(10))); //INJ_O5

 //BL
 if (o & _OBV(12)) {
  WRITEBIT(PORTC, PC3, (o & _OBV(11))); }
 else {
  WRITEBIT(PORTC, PC3, 1); }             //pull up input
 WRITEBIT(DDRC, DDC3, (o & _OBV(12)));   //select mode: input/output

 //DE
 if (o & _OBV(14)) {
  WRITEBIT(PORTC, PC2, (o & _OBV(13))); }
 else {
  WRITEBIT(PORTC, PC2, 1); }             //pull up input
 WRITEBIT(DDRC, DDC2, (o & _OBV(14)));   //select mode: input/output

 WRITEBIT(spi_PORTB, 1, (o & _OBV(15))); //STBL_O
 WRITEBIT(spi_PORTB, 5, (o & _OBV(16))); //CEL_O
 WRITEBIT(spi_PORTB, 3, (o & _OBV(17))); //FPMP_O
 WRITEBIT(spi_PORTB, 2, (o & _OBV(18))); //PWRR_O
 WRITEBIT(spi_PORTB, 6, (o & _OBV(19))); //EVAP_O
 WRITEBIT(spi_PORTB, 7, (o & _OBV(20))); //O2SH_O
 WRITEBIT(spi_PORTB, 4, (o & _OBV(21))); //COND_O
 WRITEBIT(spi_PORTB, 0, (o & _OBV(22))); //ADD_O2

 //TACH_O
 if (o & _OBV(24))
 {
  WRITEBIT(PORTC, PC4, (o & _OBV(23)));
 }

#endif

}

/**Initialization of digital inputs in diagnostic mode*/
void init_digital_inputs(void)
{
#ifdef SECU3T
 //GAS_V, CKPS, REF_S, PS
 DDRC&=~(_BV(DDC6));
 DDRD&=~(_BV(DDD6)|_BV(DDD2)|_BV(DDD3));
 //BL jmp, DE jmp
 DDRC &= ~(_BV(DDC3)|_BV(DDC2)); //inputs
 PORTC|= _BV(PC3)|_BV(PC2);
#else //---SECU-3i---

 //CKPS, REF_S, PS
 DDRD&=~(_BV(DDD6)|_BV(DDD2)|_BV(DDD3));

 //BL jmp, DE jmp
 DDRC &= ~(_BV(DDC3)|_BV(DDC2)); //inputs
 PORTC|= _BV(PC3)|_BV(PC2);

 //GAS_V, IGN_I, COND_I, EPAS_I without pull-up resistors
 spi_IODIRA|= (_BV(3)|_BV(2)|_BV(1)|_BV(0));
 spi_GPPUA &= ~(_BV(3)|_BV(2)|_BV(1)|_BV(0));

#endif
}

/**Get states of inputs
 * \return bits containing state of digital inputs
 */
uint16_t get_inputs(void)
{
#ifdef SECU3T
 //GAS_V, CKPS, REF_S, PS
 uint16_t i = _IBV((!!CHECKBIT(PINC, PINC6)), 0) | _IBV((!!CHECKBIT(PIND, PIND6)), 1) | _IBV((!!CHECKBIT(PIND, PIND2)), 2) |
              _IBV((!!CHECKBIT(PIND, PIND3)), 3) |
              _IBV((!!CHECKBIT(PINC, PINC3)), 4) | _IBV((!!CHECKBIT(PINC, PINC2)), 5);  //BL jmp, DE jmp

#else //---SECU-3i---

 uint16_t i = _IBV((!!CHECKBIT(spi_PORTA, 0)), 0) | _IBV((!!CHECKBIT(PIND, PIND6)), 1) | _IBV((!!CHECKBIT(PIND, PIND2)), 2) |
              _IBV((!!CHECKBIT(PIND, PIND3)), 3) | //GAS_V, CKPS, REF_S, PS
              _IBV((!!CHECKBIT(PINC, PINC3)), 4) | _IBV((!!CHECKBIT(PINC, PINC2)), 5) |  //BL jmp, DE jmp
              _IBV((!!CHECKBIT(spi_PORTA, 3)), 6) | _IBV((!!CHECKBIT(spi_PORTA, 2)), 7) | _IBV((!!CHECKBIT(spi_PORTA, 1)), 8); //IGN_I, COND_I, EPAS_I

#endif
 return i;
}

void diagnost_process(void)
{
 if (0==diag.diag_started)
  return; //normal mode

 //We are in diagnostic mode
 sop_set_operation(SOP_SEND_NC_ENTER_DIAG);

 //perform initialization of digital outputs
 init_digital_outputs();

 //perform initialization of digital inputs
 init_digital_inputs();

#ifndef SECU3T
 knock_expander_initialize();
#endif

 //Diasable unneeded interrupts
 TIMSK0&=~(_BV(OCIE0A)|_BV(OCIE0B)|_BV(TOIE0));
 TIMSK1&=~(_BV(ICIE1)|_BV(OCIE1A)|_BV(OCIE1B)|_BV(TOIE1));
 TIMSK2&=~(_BV(OCIE2A)|_BV(OCIE2B));

 //Disable external interrupts
 EIMSK&=  ~(_BV(INT0) | _BV(INT1) | _BV(INT2));

 //local loop
 while(1)
 {
  //check & execute suspended operations
  sop_execute_operations();
  //process data being received and sent via serial port
  process_uart_interface();

  switch(diag.fsm_state)
  {
   //start measurements (sensors)
   case 0:
    //select next channel
    knock_set_channel(diag.ksp_channel);
    if (++diag.ksp_channel > KSP_CHANNEL_1)
     diag.ksp_channel = KSP_CHANNEL_0;
    adc_begin_measure(0); //<--normal speed
    ++diag.fsm_state;
    break;

   //wait for completion of measurements, start integration of current knock channel's signal
   case 1:
    if (adc_is_measure_ready())
    {
     if (!(d.diag_out & _OBV(24)))
      knock_set_integration_mode(KNOCK_INTMODE_INT);
     _DELAY_US(1000);   //1ms
     ++diag.fsm_state;
    }
    break;

   //start measurements (knock signal)
   case 2:
    if (!(d.diag_out & _OBV(24)))
     knock_set_integration_mode(KNOCK_INTMODE_HOLD);

    //start the process of downloading the settings into the HIP9011 (and getting ADC result for TPIC8101)
#ifndef SECU3T
    if (IOCFG_CHECK(IOP_KSP_CS))
#endif
     knock_start_settings_latching();
#ifndef TPIC8101
    adc_begin_measure_knock(0); //HIP9011 only
#endif
    ++diag.fsm_state;
    break;

   //wait for completion of measurements, and reinitialize state machine
   //for TPIC8101 we wait for latching of settings
   //for HIP9011 we wait for ADC measurement
   case 3:
#ifdef TPIC8101
    if (knock_is_latching_idle())
    {
     diag.knock_value[diag.ksp_channel] = knock_get_adc_value(); //get ADC value read from TPIC8101
#else //HIP9011
    if (adc_is_measure_ready())
    {
     diag.knock_value[diag.ksp_channel] = adc_get_knock_value();
#endif
     _DELAY_US(100);
     diag.fsm_state = 0;
    }
    break;
  };

  if (diag.skip_loops == 0)
  {
   //analog inputs
#ifdef SECU3T
   d.diag_inp.flags = 1; //SECU-3T
#else
   d.diag_inp.flags = 0; //SECU-3i
#endif
   d.diag_inp.voltage = _ADC_COMPENSATE(adc_get_ubat_value(), ADC_VREF_FACTOR, 0.0);
   d.diag_inp.map = _ADC_COMPENSATE(adc_get_map_value(), ADC_VREF_FACTOR, 0.0);
   d.diag_inp.temp = _ADC_COMPENSATE(adc_get_temp_value(), ADC_VREF_FACTOR, 0.0);
   d.diag_inp.ks_1 = _ADC_COMPENSATE(diag.knock_value[0], ADC_VREF_FACTOR, 0.0);
   d.diag_inp.ks_2 = _ADC_COMPENSATE(diag.knock_value[1], ADC_VREF_FACTOR, 0.0);
   d.diag_inp.add_i1 = _ADC_COMPENSATE(adc_get_add_i1_value(), ADC_VREF_FACTOR, 0.0);
   d.diag_inp.add_i2 = _ADC_COMPENSATE(adc_get_add_i2_value(), ADC_VREF_FACTOR, 0.0);
#ifndef SECU3T
   d.diag_inp.add_i3 = _ADC_COMPENSATE(adc_get_add_i3_value(), ADC_VREF_FACTOR, 0.0);
#ifdef TPIC8101
   d.diag_inp.add_i4 = _ADC_COMPENSATE(adc_get_knock_value(), ADC_VREF_FACTOR, 0.0);
#endif
#endif
   d.diag_inp.carb = _ADC_COMPENSATE(adc_get_carb_value(), ADC_VREF_FACTOR, 0.0);

   //digital inputs
   d.diag_inp.bits = get_inputs();

   //outputs
   set_outputs(d.diag_out);
  }
  else
   --diag.skip_loops;

  wdt_reset_timer();
 }
}

#endif //DIAGNOSTICS
