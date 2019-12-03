#ifndef __SYSCALLS_H
#define __SYSCALLS_H

#include <stdint.h>

#define SYSCall_windows_create 		0
#define SYSCall_memory_sbrk     	1
#define SYSCall_debug_puts      	2
#define SYSCall_messaging_get  		3
#define SYSCall_messaging_create 	4
#define SYSCall_windows_present  	5
#define SYSCall_process_create    6
#define SYSCall_file_open					7
#define SYSCall_file_read					8
#define SYSCall_process_from_file 9

extern uint64_t syscall(uint64_t sys_no, ...);

#endif