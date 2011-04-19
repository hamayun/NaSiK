/****h* libos/component
 * SUMMARY
 * Component definition.
 ****
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

#ifndef DNA_INTERFACE_COMPONENT_H
#define DNA_INTERFACE_COMPONENT_H

#include <stdint.h>
#include <DnaTools/Status.h>

/****t* component/component_t
 * SUMMARY
 * Component type.
 *
 * SOURCE
 */

typedef struct _component {
  char * name;
  status_t (* create) (void);
  status_t (* destroy) (void);
  status_t (* start) (void);
  status_t (* stop) (void);
} component_t;

/*
 ****/

#endif

