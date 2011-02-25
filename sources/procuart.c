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

#include <inavr.h>
#include <stdint.h>
#include "ce_errors.h"
#include "ckps.h"
#include "knock.h"
#include "procuart.h"
#include "secu3.h"
#include "suspendop.h"
#include "uart.h"
#include "ufcodes.h"
#include "vstimer.h"

void process_uart_interface(struct ecudata_t* d)
{ 
 uint8_t descriptor;

 if (uart_is_packet_received())//������� ����� ����� ?
 { 
  descriptor = uart_recept_packet(d);
  switch(descriptor)
  {
   case TEMPER_PAR:            
   case CARBUR_PAR:   
   case IDLREG_PAR:   
   case ANGLES_PAR:   
   case FUNSET_PAR:   
   case STARTR_PAR:
   case ADCCOR_PAR: 
   case CKPS_PAR: 
   case KNOCK_PAR:  
   case MISCEL_PAR:     
    //���� ���� �������� ��������� �� ���������� ������� �������
    s_timer16_set(save_param_timeout_counter, SAVE_PARAM_TIMEOUT_VALUE);
    break;        
   case OP_COMP_NC: 
    if (d->op_actn_code == OPCODE_EEPROM_PARAM_SAVE) //������� ������� ���������� ����������
    {
     sop_set_operation(SOP_SAVE_PARAMETERS);     
     d->op_actn_code = 0; //���������� 
    }
    if (d->op_actn_code == OPCODE_CE_SAVE_ERRORS) //������� ������� ������ ����������� ����� ������  
    {
     sop_set_operation(SOP_READ_CE_ERRORS);     
     d->op_actn_code = 0; //���������� 
    }
    if (d->op_actn_code == OPCODE_READ_FW_SIG_INFO) //������� ������� ������ � �������� ���������� � ��������
    {
     sop_set_operation(SOP_SEND_FW_SIG_INFO);
     d->op_actn_code = 0; //����������        
    }
    break;    
      
   case CE_SAVED_ERR:
    sop_set_operation(SOP_SAVE_CE_ERRORS);
    break;       
  }
  
  //���� ���� �������� ��������� ����, �� ���������� ��������� �� �� ���������� ���������
  if (descriptor == CKPS_PAR)
  {
   ckps_set_cyl_number(d->param.ckps_engine_cyl);  //<--����������� � ������ �������! 
   ckps_set_edge_type(d->param.ckps_edge_type);
   ckps_set_cogs_btdc(d->param.ckps_cogs_btdc);
   ckps_set_ignition_cogs(d->param.ckps_ignit_cogs);
  }
  
  //���������� ��� ��������� ���������, ����������� ����� CKPS_PAR!
  if (descriptor == KNOCK_PAR)
  {
   //�������������� ��������� ��������� � ������ ���� �� �� �������������, � ������ ��������� ������� ��� ������������.
   if (!d->use_knock_channel_prev && d->param.knock_use_knock_channel)
    if (!knock_module_initialize())
    {//��� ����������� ���������� ��������� ���������� - �������� ��
     ce_set_error(ECUERROR_KSP_CHIP_FAILED);   
    }    

   knock_set_band_pass(d->param.knock_bpf_frequency);
   //gain ��������������� � ������ ������� �����
   knock_set_int_time_constant(d->param.knock_int_time_const);
   ckps_set_knock_window(d->param.knock_k_wnd_begin_angle, d->param.knock_k_wnd_end_angle);  
   ckps_use_knock_channel(d->param.knock_use_knock_channel);   
    
   //���������� ��������� ����� ��� ���� ����� ����� ����� ���� ���������� ����� ����������������
   //��������� ��������� ��� ���.   
   d->use_knock_channel_prev = d->param.knock_use_knock_channel;  
  }

  //�� ���������� �������� ������ - �������� ����� ������ �� ��������
  uart_notify_processed();         
 }

 //������������ �������� ������ � �������
 if (s_timer_is_action(send_packet_interval_counter))
 {
  if (!uart_is_sender_busy())
  {                
   uart_send_packet(d, 0);    //������ ���������� �������� ��������� ������
   s_timer_set(send_packet_interval_counter, d->param.uart_period_t_ms);
   
   //����� �������� ������� ��� ������
   d->ecuerrors_for_transfer = 0;
  }
 }
}