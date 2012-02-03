
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

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <Private/KVMAnnotationManager.h>

#ifdef USE_ANNOTATION_BUFFERS
static db_buffer_desc_t buff_desc[ANNOTATION_BUFFER_COUNT] = {{0, 0, 0, ANNOTATION_BUFFER_SIZE, {{0, NULL}}},
                                                              {1, 0, 0, ANNOTATION_BUFFER_SIZE, {{0, NULL}}},
                                                              {2, 0, 0, ANNOTATION_BUFFER_SIZE, {{0, NULL}}}};  //Empty Descriptors;

static volatile uint32_t current_buffer = 0;
extern volatile uint32_t current_thread_context;

int  buffer_add_db(db_buffer_desc_t *pbuff_desc, annotation_db_t *pdb)
{
    if(((pbuff_desc->EndIndex + 1) % pbuff_desc->Capacity) == pbuff_desc->StartIndex)
    {
        // Annotation Buffer Full; Activate Switching Logic
        return (1);
    }

    pbuff_desc->Buffer[pbuff_desc->EndIndex].thread_context = current_thread_context;
    pbuff_desc->Buffer[pbuff_desc->EndIndex].pdb = pdb;
    pbuff_desc->EndIndex = (pbuff_desc->EndIndex + 1) % pbuff_desc->Capacity;
    return 0;
}

/*
 * This functions has been explicity placed in this C file;
 * So llvm fails to annotate all functions of this file.
 */

/*
 * IMPORTANT NOTE: We should _ONLY_ call NON ANNOTATED functions from Here.
 * Otherwise We will find ourselves in a Vicious Loop ... :(
 */
void mbb_annotation(annotation_db_t *pdb)
{
    // TO DECIDE: Do we need to Take a Semaphore Here ???
    // TODO: Add dynamisim to buffers here; Allocate and Init at Runtime.
    int is_full_buffer = 0;
    // Get the Return Address of Current Function;
    // Its the address of next instruction that will be executed after mbb_annotation call.
    // TODO: Store this Return Address inside LLVM; When the Annotation Call is Added.
    if(!pdb->FuncAddr){
        pdb->FuncAddr = (uint32_t) __builtin_return_address (0);
    }

    is_full_buffer = buffer_add_db(&buff_desc[current_buffer], pdb);
    if(is_full_buffer)
    {
        // If we are here it means that the current buffer is full;
        // So we send the annotation data to H/W and Switch the Buffer.
        __asm__ volatile(
            "   mov   $0x4000,%%dx\n\t"
            "   mov   %0,%%eax\n\t"
            "   out   %%eax,(%%dx)\n\t"
            ::"r" ((db_buffer_desc_t *) (&buff_desc[current_buffer])):"%dx"
        );
    
        // Now Switch to the Next Buffer
        current_buffer = (current_buffer + 1) % ANNOTATION_BUFFER_COUNT;

        // Now Try Again into the Next Buffer
        is_full_buffer = buffer_add_db(&buff_desc[current_buffer], pdb);

        // Here we send an NULL Pointer to the SystemC; Indicating that
        // an Overflow condition has occured.
        if(is_full_buffer)
        {
            __asm__ volatile(
                "   mov   $0x4000,%%dx\n\t"
                "   mov   %0,%%eax\n\t"
                "   out   %%eax,(%%dx)\n\t"
                ::"r" ((db_buffer_desc_t *) (NULL)):"%dx"
            );
        }
    }
}

#else

void mbb_annotation(annotation_db_t *pdb)
{
    // Get the Return Address of Current Function;
    // Its the address of next instruction that will be executed after mbb_annotation call.
    if(!pdb->FuncAddr){
        pdb->FuncAddr = (uint32_t) __builtin_return_address (0);
    }

    // We send the annotation data to H/W.
    __asm__ volatile(
        "   mov   $0x4000,%%dx\n\t"
        "   mov   %0,%%eax\n\t"
        "   out   %%eax,(%%dx)\n\t"
        ::"r" ((annotation_db_t *) pdb):"%dx"
    );
}

#endif /* USE_ANNOTATION_BUFFERS */

