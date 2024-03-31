#ifndef __SYSCALLS_H__
#define __SYSCALLS_H__

#include "types.h"
#include "interrupts.h"
#include "multitasking.h"

class SyscallHandler : public InterruptHandler
{
public:
    SyscallHandler(InterruptManager * interruptManager);
    ~SyscallHandler();

    uint32_t HandleInterrupt(uint32_t esp);

};

#endif