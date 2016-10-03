#ifndef _ASM_LKL_CPU_H
#define _ASM_LKL_CPU_H

int lkl_cpu_get(void);
void lkl_cpu_put(void);
int lkl_cpu_try_get_start(void);
void lkl_cpu_try_get_stop(void);
int lkl_cpu_init(void);
void lkl_cpu_shutdown(void);
void lkl_cpu_wait_shutdown(void);
void lkl_cpu_wakeup(void);
void lkl_cpu_set_irqs_pending(void);

#endif /* _ASM_LKL_CPU_H */
