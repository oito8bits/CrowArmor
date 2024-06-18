#include "hook.h"
#include "control_registers/cr0.h"
#include "crowarmor/datacrow.h"
#include "kpobres/kallsyms_lookup.h"
#include "syscall.h"
#include <linux/kprobes.h>
#include <linux/module.h>
#include <linux/string.h>

static unsigned long **syscall_table;
static unsigned long **old_syscall_table;
static struct crow **armor;

static struct hook_syscall syscalls[] = {
    /**
     * hooked system calls
     */
    {.new_syscall = syscall_memfd_create,
     .old_syscall = NULL,
     .idx = __NR_memfd_create},

    /**
     * null byte array
     */
    {.new_syscall = NULL, .old_syscall = NULL, .idx = -1}

};

static void set_new_syscall(struct hook_syscall *syscall) {
  disable_register_cr0_wp();
  syscall->old_syscall = syscall_table[syscall->idx];
  syscall_table[syscall->idx] = syscall->new_syscall;
  enable_register_cr0_wp();
}

static void set_old_syscall(struct hook_syscall *syscall) {
  disable_register_cr0_wp();
  syscall_table[syscall->idx] = syscall->old_syscall;
  enable_register_cr0_wp();
}

void hook_remove_unknown_syscall(struct hook_syscall *syscall) {
  disable_register_cr0_wp();
  syscall_table[syscall->idx] = syscall->old_syscall;
  enable_register_cr0_wp();
}

void *hook_get_old_syscall(int idx) {
  unsigned int i;

  for (i = 0; !IS_NULL_PTR(syscalls[i].new_syscall); i++) {
    if (syscalls[i].idx == idx)
      return syscalls[i].old_syscall;
  }

  return NULL;
}

void hook_check_hooked_syscall(struct hook_syscall *syscall, int idx) {
  syscall->unknown_hook = false;

  if (syscall_table[idx] != old_syscall_table[idx]) {
    syscall->new_syscall = syscall_table[idx];
    syscall->old_syscall = old_syscall_table[idx];
    syscall->idx = idx;

    syscall->unknown_hook = true;
  }
}

// Values ​​passed as parameters into the function using registers esi, rdi
static void hook_crow_x64_sys_call(void)
{
  
}

static ERR hook_edit_x64_sys_call(void)
{
    void *x64_sys_call = (void*)kallsyms_lookup_name("x64_sys_call");

    if (!x64_sys_call) {
        pr_info("crowarmor: symbol 'x64_sys_call' not found\n");
        return ERR_FAILURE;
    }

    disable_register_cr0_wp();
    // Write the modified bytes into x64_sys_call memory
    strncpy((char *)x64_sys_call+7, "\xCC\x00\x00", 3); 
    enable_register_cr0_wp();

    return ERR_SUCCESS;
}

ERR hook_init(struct crow **crow) {
  unsigned int i;

  syscall_table = (unsigned long **)kallsyms_lookup_name("sys_call_table");

  if (syscall_table == 0)
    return ERR_FAILURE;

  old_syscall_table = kmalloc(sizeof(void *) * __NR_syscalls, __GFP_HIGH);

  if (syscall_table == NULL)
    return ERR_FAILURE;

  for (i = 0; !IS_NULL_PTR(syscalls[i].new_syscall); i++)
    set_new_syscall(&syscalls[i]);

  memcpy(old_syscall_table, syscall_table, sizeof(void *) * __NR_syscalls);

  hook_edit_x64_sys_call();

  (*crow)->hook_is_actived = true;
  armor = crow;

  return ERR_SUCCESS;
}

void hook_end(void) {
  pr_warn("crowamor: Hook syscalls shutdown ...");

  unsigned int i;

  for (i = 0; !IS_NULL_PTR(syscalls[i].new_syscall); i++)
    set_old_syscall(&syscalls[i]);
<<<<<<< Updated upstream
=======
  
  kfree(old_syscall_table);
  kfree(crowarmor_syscall_table);
      hook_crow_x64_sys_call();

>>>>>>> Stashed changes

  (*armor)->hook_is_actived = false;
}
