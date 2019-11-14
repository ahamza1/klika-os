#include <process.h>
#include <kernel.h>
#include <string.h>
#include <isr.h>
#include <x86.h>

#define __UNUSED__ __attribute__((unused))

long syscall_debug_puts(isr_ctx_t *regs) {
	DEBUG("APP[%i]: %s\n\r", task_list_current->id, regs->rdi);
	return 1;
}

