#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
enum JfgCpuOperation {
    JFG_CPU_READ_REGISTER = 0,
    JFG_CPU_WRITE_REGISTER = 1,
    JFG_CPU_TLB_OPERATION = 2,
    JFG_CPU_CACHE_OPERATION = 3,
    JFG_CPU_EXCEPTION_RETURN = 4,
    JFG_CPU_READ_MI_MASK = 5,
    JFG_CPU_WRITE_MI_MASK = 6
};
// Return one only for a fully handled operation. Unknown operations retain
// the generated runtime's fatal trap. The optional MI-mask operations instead
// retain the original memory access when declined. Context belongs to the guest.
typedef int (*JfgCpuOperationCallback)(void* owner, void* context,
    uint32_t operation, uint32_t selector, uint64_t* value);
int jfg_minimal_runtime_bind_cpu(JfgCpuOperationCallback callback, void* owner);
int jfg_minimal_runtime_unbind_cpu(JfgCpuOperationCallback callback, void* owner);
// Only a full-width canonical MI_INTR_MASK address can enter the optional
// device bridge. All other addresses retain their original mapped access.
int32_t jfg_mmio_mask_read32(uint8_t* rdram, void* context, uint64_t address);
void jfg_mmio_mask_write32(uint8_t* rdram, void* context, uint64_t address, uint32_t value);
#ifdef __cplusplus
}
#endif
