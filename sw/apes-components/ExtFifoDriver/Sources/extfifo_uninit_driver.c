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

void extfifo_uninit_driver (void)
{
#if 0
	/* External memory space should not be managed by kernel??? */
	for (int32_t i = 0; i < EXTFIFO_CHANNEL_NDEV; i++) kernel_free ((void *)extfifo_devices[i]);
	kernel_free (extfifo_devices);
#endif
}

