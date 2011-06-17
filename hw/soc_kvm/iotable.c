/*
 * Kernel-based Virtual Machine test driver
 *
 * This test driver provides a simple way of testing kvm, without a full
 * device model.
 *
 * Copyright (C) 2006 Qumranet
 *
 * Authors:
 *
 *  Avi Kivity <avi@qumranet.com>
 *  Yaniv Kamay <yaniv@qumranet.com>
 *
 * This work is licensed under the GNU LGPL license, version 2.
 */

#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <stdio.h>

#include "iotable.h"

struct io_table_entry *io_table_lookup(struct io_table *io_table, uint64_t addr)
{
	int i;

	for (i = 0; i < io_table->nr_entries; i++) {
		if (io_table->entries[i].start <= addr &&
		    addr < io_table->entries[i].end)
			return &io_table->entries[i];
	}

	return NULL;
}

int io_table_register(struct io_table *io_table, uint64_t start, uint64_t size,
		      io_table_handler_t *handler, void *opaque)
{
	struct io_table_entry *entry;

	if (io_table->nr_entries == MAX_IO_TABLE)
		return -ENOSPC;

	entry = &io_table->entries[io_table->nr_entries];
	io_table->nr_entries++;

	entry->start = start;
	entry->end = start + size;
	entry->handler = handler;
	entry->opaque = opaque;

	return 0;
}

int io_table_print(struct io_table *io_table)
{
    struct io_table_entry *entry;
    int index;

    printf("The IO Table: \n");
    printf("==============================================\n");
    printf("Start       End         Handler     Opaque\n");
    printf("==============================================\n");
    for (index = 0; index < io_table->nr_entries; index++)
    {
        entry = &io_table->entries[index];
        printf("0x%08x  0x%08x  0x%08x  0x%08x\n",
                (uintptr_t)entry->start, (uintptr_t)entry->end,
                (uintptr_t)entry->handler, (uintptr_t)entry->opaque);
    }
    printf("==============================================\n");

    return 0;
}
