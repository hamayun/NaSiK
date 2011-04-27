/*
 * Copyright (C) 2007 TIMA Laboratory                                    
 *                                                                       
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, either version 3 of the License, or     
 * (at your option) any later version.                                   
 *                                                                       
 * This program is distributed in the hope that it will be useful,       
 * but WITHOUT ANY WARRANTY; without even the implied warranty of        
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         
 * GNU General Public License for more details.                          
 *                                                                       
 * You should have received a copy of the GNU General Public License     
 * along with this program.  If not, see <http://www.gnu.org/licenses/>. 
 */

#include <Private/Driver.h>
#include <DnaTools/DnaTools.h>

device_cmd_t tg_commands =
{
  tg_open,
  tg_close,
  tg_free,
  tg_read,
  tg_write,
  tg_control
};

driver_t tg_module =
{
  "tg",
  tg_init_hardware,
  tg_init_driver,
  tg_uninit_driver,
  tg_publish_devices,
  tg_find_device
};

char ** tg_devices;

