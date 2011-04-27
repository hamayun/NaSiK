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

device_cmd_t extfifo_commands =
{
  extfifo_open,
  extfifo_close,
  extfifo_free,
  extfifo_read,
  extfifo_write,
  extfifo_control
};

driver_t extfifo_module =
{
  "extfifo",
  extfifo_init_hardware,
  extfifo_init_driver,
  extfifo_uninit_driver,
  extfifo_publish_devices,
  extfifo_find_device
};

char ** extfifo_devices;

