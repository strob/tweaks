#include <mach/mach_init.h>
#include <mach/thread_act.h>
#include <mach/task.h>
#include <mach/port.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <mach/mach_vm.h>

void increaseIfNotPointer(__uint64_t *reg, mach_port_name_t task) {
    __uint64_t reg_value = *reg;
    if (reg_value > 0x100000000) {
        // if (pointer)
                
        vm_offset_t receiver = 0;
        mach_msg_type_number_t bytes_read = 0;
        mach_vm_size_t memory_read_size = 1;
        
        printf("About to mach_vm_read reg_value %llx\n", reg_value);
        
        mach_vm_read(task, reg_value, memory_read_size, &receiver, &bytes_read);
        ((char *)receiver)[0] += 1; // Increment the first byte of the memory by one
        mach_vm_write(task, reg_value, receiver, bytes_read);
        
//        printf("Size is %d. First eight bytes: %llx\n", bytes_read, *(unsigned long long*)receiver);
    }
}

void printRegisterState(x86_thread_state64_t *thread_state) {
    
}

void modify_thread_state(x86_thread_state64_t *thread_state, mach_port_name_t task) {
    increaseIfNotPointer(&(thread_state -> __rax), task);
    increaseIfNotPointer(&(thread_state -> __rbx), task);
    increaseIfNotPointer(&(thread_state -> __rcx), task);
    increaseIfNotPointer(&(thread_state -> __rdx), task);
    increaseIfNotPointer(&(thread_state -> __rdi), task);
    increaseIfNotPointer(&(thread_state -> __rsi), task);
    increaseIfNotPointer(&(thread_state -> __rbp), task);
    increaseIfNotPointer(&(thread_state -> __rsp), task);
    increaseIfNotPointer(&(thread_state -> __r8), task);
    increaseIfNotPointer(&(thread_state -> __r9), task);
    increaseIfNotPointer(&(thread_state -> __r10), task);
    increaseIfNotPointer(&(thread_state -> __r11), task);
    increaseIfNotPointer(&(thread_state -> __r12), task);
    increaseIfNotPointer(&(thread_state -> __r13), task);
    increaseIfNotPointer(&(thread_state -> __r14), task);
    increaseIfNotPointer(&(thread_state -> __r15), task);
}

int main(int argc, char **argv) {
    int kret = 0;
    mach_port_name_t task = MACH_PORT_NULL;
    thread_act_port_array_t threadList;
    mach_msg_type_number_t threadCount;
    
    int pid = 0;
    if (argc > 1) {
        pid = atoi(argv[1]);
    }
    if (pid == 0) {
        printf("Input PID to pause: ");
        scanf("%d", &pid);
    }
    
    printf("PID is: %d\n", pid);

    kret = task_for_pid(mach_task_self(), pid, &task);
    if (kret != KERN_SUCCESS) {
        printf("task_for_pid() failed with error %d!\n",kret);
        exit(0);
    }
    
    kret = task_threads(task, &threadList, &threadCount);
    if (kret!=KERN_SUCCESS) {
        printf("task_threads() failed with error %d!\n", kret);
        exit(0);
    }
    
    printf("Modifying registers\n");
    
    for (int i = 0; i < threadCount; i++) {
        kret = thread_suspend((thread_t) threadList[i]);
        if (kret!=KERN_SUCCESS) {
            printf("thread_suspend(%d) failed with error %d!\n", i, kret);
            exit(0);
        }
        
        // Flavors are defined here: https://purplefish.apple.com/index.php?action=search_cached&path=osfmk%2Fmach%2Fi386%2Fthread_status.h&version=xnu-6153.2.3&project=xnu&q=&language=all&index=Yukon line 104.
        // We use x86_THREAD_STATE64
        
        // count: x86_THREAD_STATE64_COUNT
        
        /**
         _STRUCT_X86_THREAD_STATE64
         {
             __uint64_t    __rax;
             __uint64_t    __rbx;
             __uint64_t    __rcx;
             __uint64_t    __rdx;
             __uint64_t    __rdi;
             __uint64_t    __rsi;
             __uint64_t    __rbp;
             __uint64_t    __rsp;
             __uint64_t    __r8;
             __uint64_t    __r9;
             __uint64_t    __r10;
             __uint64_t    __r11;
             __uint64_t    __r12;
             __uint64_t    __r13;
             __uint64_t    __r14;
             __uint64_t    __r15;
             __uint64_t    __rip;
             __uint64_t    __rflags;
             __uint64_t    __cs;
             __uint64_t    __fs;
             __uint64_t    __gs;
         };
         */
        x86_thread_state64_t thread_state;
        mach_msg_type_number_t stateCount = x86_THREAD_STATE64_COUNT;
        thread_get_state(threadList[i], x86_THREAD_STATE64, (thread_state_t) &thread_state, &stateCount);
        if (kret!=KERN_SUCCESS) {
            printf("thread_get_state(%d) failed with error %d!\n", i, kret);
            exit(0);
        }
        // printf("old __rip for %d is %llx\n", i, thread_state.__rip);
        printf("Registers for %d:\n", i);
        printf("rax: %llx\nrbx: %llx\nrcx:%llx\nrdx:%llx\n", thread_state.__rax, thread_state.__rbx, thread_state.__rcx, thread_state.__rdx);
        modify_thread_state(&thread_state, task);
        // thread_state.__rip += 1; // Probably just cause a misaligned instruction instantaneously
        // What if you just set the overflow bit randomly?
        printf("rax: %llx\nrbx: %llx\nrcx:%llx\nrdx:%llx\n", thread_state.__rax, thread_state.__rbx, thread_state.__rcx, thread_state.__rdx);
//        printf("new __rip for %d is %llx\n", i, thread_state.__rip);

        thread_set_state(threadList[i], x86_THREAD_STATE64, (thread_state_t) &thread_state, stateCount);
        if (kret!=KERN_SUCCESS) {
            printf("thread_set_state(%d) failed with error %d!\n", i, kret);
            exit(0);
        }
         
        kret = thread_resume((thread_t) threadList[i]);
        if (kret!=KERN_SUCCESS) {
            printf("thread_resume(%d) failed with error %d!\n", i, kret);
            exit(0);
        }
    }
    
    printf("Registers modified\n");
}