/****h* VirtualFileSystem/File
 * SUMMARY
 * VFS File header.
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

#ifndef DNA_VFS_FILE_PUBLIC_H
#define DNA_VFS_FILE_PUBLIC_H

#include <stdint.h>

/****t* File/file_type_t
 * SUMMARY
 * File type type.
 *
 * SOURCE
 */

typedef enum _file_type
{
  DNA_FILE_REGULAR,
  DNA_FILE_DIRECTORY,
  DNA_FILE_SYMBOLIC_LINK,
  DNA_FILE_SPECIAL_CHARACTER,
  DNA_FILE_SPECIAL_BLOCK
}
file_type_t;

/*
 ****/

/****t* File/file_info_t
 * SUMMARY
 * File information type.
 *
 * SOURCE
 */

typedef struct _file_info
{
  int32_t volume;
  int64_t vnode;
  file_type_t type;

  int64_t last_access;
  int64_t last_modification_time;
  int64_t last_change_time;

  int64_t byte_size;
  int32_t block_count;
  int32_t optimal_io_block_size;
}
file_info_t;

/*
 ****/

/****t* File/directory_entry_t
 * SUMMARY
 * Directory entry type.
 *
 * SOURCE
 */

typedef struct _directory_entry
{
  int64_t vnid;
  int32_t vid;
  uint16_t d_reclen;
  char d_name[1];
}
directory_entry_t;

/*
 ****/

#endif
