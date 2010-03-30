#ifndef CPU_TRAP_H
#define CPU_TRAP_H

#include <stdint.h>

typedef enum exception_id {
	CPU_TRAP_DATA_ABORT,
	CPU_TRAP_PREFETCH_ABORT,
	CPU_TRAP_UNDEFINED
} exception_id_t;

typedef uint32_t interrupt_id_t;
typedef uint32_t interrupt_status_t;
typedef int32_t (* exception_handler_t) (void);
typedef int32_t (* interrupt_handler_t) (int32_t itn);

#define CPU_N_IT 5

extern interrupt_handler_t handler_table[CPU_N_IT];

extern void __dnaos_hal_trap_attach_esr (exception_id_t id, exception_handler_t isr);
extern void __dnaos_hal_trap_attach_isr (interrupt_id_t id, uint32_t mode, interrupt_handler_t isr);

extern interrupt_status_t __dnaos_hal_trap_mask_and_backup (void);
extern void __dnaos_hal_trap_restore (interrupt_status_t backup);

extern void __dnaos_hal_trap_enable (interrupt_id_t id);
extern void __dnaos_hal_trap_disable (interrupt_id_t id);

#define CPU_TRAP_ATTACH_ESR(id, isr) __dnaos_hal_trap_attach_esr(id,isr)
#define CPU_TRAP_ATTACH_ISR(id, mode, isr) __dnaos_hal_trap_attach_isr(id,mode,isr)

#define CPU_TRAP_MASK_AND_BACKUP() __dnaos_hal_trap_mask_and_backup()
#define CPU_TRAP_RESTORE(backup) __dnaos_hal_trap_restore(backup)

#define CPU_TRAP_ENABLE(id) __dnaos_hal_trap_enable(id)
#define CPU_TRAP_DISABLE(id) __dnaos_hal_trap_disable(id)

#endif

