/************************************************************************
 * extfifo_driver_driver.c : DNA rdma_driver driver                                    *
 * Copyright (C) 2008 TIMA Laboratory                                    *
 * Author: Alexandre CHAGOYA-GARZON
 *                                                                       *
 * This program is free software: you can redistribute it and/or modify  *
 * it under the terms of the GNU General Public License as published by  *
 * the Free Software Foundation, either version 3 of the License, or     *
 * (at your option) any later version.                                   *
 *                                                                       *
 * This program is distributed in the hope that it will be useful,       *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 * GNU General Public License for more details.                          *
 *                                                                       *
 * You should have received a copy of the GNU General Public License     *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *************************************************************************/

#include <Private/Driver.h>
#include <MemoryManager/MemoryManager.h>
#include <DnaTools/DnaTools.h>
#include <string.h>

status_t extfifo_open (char * name, int32_t mode, void ** data)
{
	int channel_id;
	uint32_t pos = strlen(name) - 1;

  //watch (status_t)
  //{
	while ((name[pos] != '/') && (pos > 0)) pos --;
	//ensure (name[pos] != 0, DNA_ERROR);

	/*
	 * What is being done below here is really dangerous.
	 * The storage type of void * is only 16-bit long, while
	 * the storage type of value is 32-bit long.
	 */

	channel_id = dna_atoi ((char *) & name[pos + 1]);

	*data = &EXTFIFO_CHANNELS[channel_id];

	return DNA_OK;
  //}

  //rescue (bad_file)
  //{
  //  leave;
  //}
}



