
/*
 * Copyright (C) 2011 TIMA Laboratory
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
 * You should have received new_alarm copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ANNOTATION_MANAGER_PRIVATE_H
#define ANNOTATION_MANAGER_PRIVATE_H

#include <Public/KVMAnnotationManager.h>

#ifdef USE_ANNOTATION_BUFFERS

#define ANNOTATION_BUFFER_COUNT 3
#define ANNOTATION_BUFFER_SIZE  1024+1

typedef struct {
    uint32_t            BufferID;
    uint32_t            StartIndex;       // First Valid db entry
    uint32_t            EndIndex;         // Last Valid db entry
    uint32_t            Capacity;         // The Size of DB Buffer to For H/W Knowledge
    annotation_db_ctx_t Buffer[ANNOTATION_BUFFER_SIZE];
} db_buffer_desc_t;

int  buffer_add_db(db_buffer_desc_t *pbuff_desc, annotation_db_t *pdb);
#endif /* USE_ANNOTATION_BUFFERS */
#endif
