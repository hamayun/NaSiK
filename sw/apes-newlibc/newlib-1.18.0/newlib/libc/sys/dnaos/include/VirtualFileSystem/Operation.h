/****h* VirtualFileSystem/Operations
 * SUMMARY
 * VFS operations.
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

#ifndef DNA_VFS_OPERATION_PUBLIC_H
#define DNA_VFS_OPERATION_PUBLIC_H

#include <stdarg.h>
#include <stdint.h>

#include <VirtualFileSystem/File.h>
#include <DnaTools/DnaTools.h>

#define DNA_SEEK_SET                    0x00000000
#define DNA_SEEK_FROM_CURRENT           0x00000001
#define DNA_SEEK_FROM_END               0x00000002

#define DNA_READ_ONLY                   0x00000000
#define DNA_WRITE_ONLY                  0x00000001
#define DNA_READ_WRITE                  0x00000002
#define DNA_APPEND                      0x00000008
#define DNA_CREATE                      0x00000200
#define DNA_TRUNCATE                    0x00000400
#define DNA_FAIL_IF_EXIST               0x00000800
#define DNA_NON_BLOCKING                0x00004000

extern status_t vfs_mount (char * source,
    char * target, char * fs_type,
    uint32_t flags, void * data);

extern status_t vfs_umount (char * target);

extern status_t vfs_open (char * path, int32_t mode,
    int32_t perms, int16_t * p_fd);

extern status_t vfs_close (int16_t fd);

extern status_t vfs_mkdir (char * path, int32_t mode);

extern status_t vfs_readdir (int16_t fd, directory_entry_t * entry_array,
    uint32_t count, int32_t * p_res);

extern status_t vfs_read (int16_t fd, void * data,
    int32_t count, int32_t * p_ret);
extern status_t vfs_write (int16_t fd, void * data,
    int32_t count, int32_t * p_ret);

extern status_t vfs_get_info (int16_t fd, file_info_t * p_info);
extern status_t vfs_set_info (int16_t fd, file_info_t info, int32_t mask);

extern status_t vfs_lseek (int16_t fd, int64_t offset,
    int32_t whence, int64_t * p_ret);

extern status_t vfs_ioctl (int16_t fd, int32_t function,
    va_list arguments, int32_t * p_ret);

#endif
