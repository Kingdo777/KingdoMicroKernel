#ifndef UAPI_SYSCALL_NUM_H
#define UAPI_SYSCALL_NUM_H

#define NR_SYSCALL 64

/* Character IO */
#define KMK_SYS_putstr 0
#define KMK_SYS_getc   1

/* PMO */
#define KMK_SYS_create_pmo        2
#define KMK_SYS_map_pmo           4
#define KMK_SYS_unmap_pmo         5
#define KMK_SYS_write_pmo         6
#define KMK_SYS_read_pmo          7
#define KMK_SYS_get_phys_addr     8

/* Capability */
#define KMK_SYS_revoke_cap        9
#define KMK_SYS_transfer_caps    10

/* Multitask */
/* - create & exit */
#define KMK_SYS_create_cap_group 11
#define KMK_SYS_exit_group       12
#define KMK_SYS_kill_group       60
#define KMK_SYS_create_thread    13
#define KMK_SYS_thread_exit      14
/* - recycle */
#define KMK_SYS_register_recycle     15
#define KMK_SYS_cap_group_recycle    16
#define KMK_SYS_ipc_close_connection 17
/* - schedule */
#define KMK_SYS_yield        18
#define KMK_SYS_set_affinity 19
#define KMK_SYS_get_affinity 20
#define KMK_SYS_set_prio     21
#define KMK_SYS_get_prio     22
#define KMK_SYS_suspend      23
#define KMK_SYS_resume       24
/* - ptrace */
#define KMK_SYS_ptrace       25

/* IPC */
#define KMK_SYS_register_server         26
#define KMK_SYS_register_client         27
#define KMK_SYS_ipc_register_cb_return  28
#define KMK_SYS_ipc_call                29
#define KMK_SYS_ipc_return              30
#define KMK_SYS_ipc_exit_routine_return 31
#define KMK_SYS_ipc_get_cap             32
#define KMK_SYS_ipc_set_cap             33
/* - notification */
#define KMK_SYS_create_notifc           34
#define KMK_SYS_wait                    35
#define KMK_SYS_notify                  36

/* Exception */
/* - irq */
#define KMK_SYS_irq_register            37
#define KMK_SYS_irq_wait                38
#define KMK_SYS_irq_ack                 39
#define KMK_SYS_configure_irq           40
/* - page fault */
#define KMK_SYS_user_fault_register     41
#define KMK_SYS_user_fault_map          42

/* POSIX */
/* - time */
#define KMK_SYS_clock_gettime           43
#define KMK_SYS_clock_nanosleep         44
/* - memory */
#define KMK_SYS_handle_brk              45
#define KMK_SYS_handle_mprotect         46

/* Hardware Access */
/* - cache */
#define KMK_SYS_cache_flush             47
#define KMK_SYS_cache_config            48
/* - timer */
#define KMK_SYS_get_current_tick        49
/* Get PCI devcie information. */
#define KMK_SYS_get_pci_device          50
/* poweroff */
#define KMK_SYS_poweroff                51

/* Utils */
#define KMK_SYS_empty_syscall           52
#define KMK_SYS_top                     53
#define KMK_SYS_get_free_mem_size       54
#define KMK_SYS_get_mem_usage_msg       55
#define KMK_SYS_get_system_info         56

/* - futex */
#define KMK_SYS_futex      57
#define KMK_SYS_set_tid_address  58

#endif /* UAPI_SYSCALL_NUM_H */
