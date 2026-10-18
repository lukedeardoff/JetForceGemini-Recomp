#include "recomp.h"

#ifdef __cplusplus
extern "C" {
#endif
int32_t jfg_mmio_mask_read32(uint8_t*, void*, uint64_t);
void jfg_mmio_mask_write32(uint8_t*, void*, uint64_t, uint32_t);
#ifdef __cplusplus
}
#endif


#if defined(JFG_NEGATIVE_EXTRA_REFERENCE)
void jfg_negative_unexpected_reference(void);
#endif

void synthetic_core(uint8_t* rdram, recomp_context* ctx);
void synthetic_core_recomp(uint8_t* rdram, recomp_context* ctx);
void synthetic_replaceable(uint8_t* rdram, recomp_context* ctx);
void synthetic_replaceable_recomp(uint8_t* rdram, recomp_context* ctx);
recomp_func_t* jfg_generated_lookup_function(int32_t vram);
extern int jfg_synthetic_body_data;
extern int jfg_synthetic_patch_data;
extern int jfg_synthetic_support_data;
extern int jfg_synthetic_wrapper_data;

static volatile int exercise_fail_closed_bridges = 0;

static int checked_dispatch(
    void* opaque,
    int32_t vram,
    uint8_t* rdram,
    recomp_context* context
) {
    (void)opaque;
    recomp_func_t* function = jfg_generated_lookup_function(vram);
    if (function == 0 || rdram == 0 || context == 0) {
        return 0;
    }
    function(rdram, context);
    return 1;
}

static int call_and_expect(recomp_func_t* function, uint64_t expected) {
    uint8_t memory[16] = {0};
    recomp_context context = {0};
    function(memory, &context);
    return context.r2 == expected;
}

static void retain_fail_closed_bridge_references(
    uint8_t* memory,
    recomp_context* context
) {
    if (exercise_fail_closed_bridges) {
#if defined(JFG_NEGATIVE_EXTRA_REFERENCE)
        jfg_negative_unexpected_reference();
#endif
        cop0_write(context, 0, 0);
        (void)cop0_read(context, 0);
        cop0_eret(memory, context);
        cache_op(memory, context, 0, 0);
        cop0_tlb_op(context, 0);
        reserved_instruction(memory, context, 0, 0);
        switch_error("synthetic", 0, 0);
        do_break(0);
        recomp_syscall_handler(memory, context, 0);
        pause_self(memory);
    }
}

int jfg_generated_link_smoke(void) {
    uint8_t memory[16] = {0};
    recomp_context context = {0};
    if (jfg_synthetic_body_data != 1 || jfg_synthetic_wrapper_data != 2 ||
        jfg_synthetic_patch_data != 3 || jfg_synthetic_support_data != 4) {
        return 8;
    }
    const size_t capacity = jfg_minimal_runtime_section_capacity();
    if (capacity < jfg_generated_section_count() ||
        jfg_generated_initialize_sections(section_addresses, capacity) == 0 ||
        jfg_minimal_runtime_initialize() == 0) {
        return 1;
    }
    jfg_mmio_mask_write32(memory, &context, UINT64_C(0xffffffff80000000), 0x12345678U);
    if (jfg_mmio_mask_read32(memory, &context, UINT64_C(0xffffffff80000000)) != 0x12345678) return 10;
    cop0_status_write(&context, UINT64_C(5));
    if (cop0_status_read(&context) != UINT64_C(5)) {
        return 2;
    }
    if (!call_and_expect(synthetic_core, UINT64_C(7))) {
        return 3;
    }
    if (!call_and_expect(synthetic_replaceable, UINT64_C(29))) {
        return 4;
    }
    if (!call_and_expect(synthetic_core_recomp, UINT64_C(7))) {
        return 5;
    }
    if (!call_and_expect(synthetic_replaceable_recomp, UINT64_C(11))) {
        return 6;
    }
    if (jfg_generated_lookup_function(1) != synthetic_core ||
        jfg_minimal_runtime_bind_dispatch(checked_dispatch, 0) == 0 ||
        !call_and_expect(get_function(1), UINT64_C(7)) ||
        jfg_minimal_runtime_unbind_dispatch(checked_dispatch, 0) == 0) {
        return 7;
    }
    retain_fail_closed_bridge_references(memory, &context);
    if (jfg_minimal_runtime_bind_cpu(0, 0) != 0 ||
        jfg_minimal_runtime_unbind_cpu(0, 0) != 0) return 9;
    return 0;
}
