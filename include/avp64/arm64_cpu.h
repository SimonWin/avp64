/******************************************************************************
 *                                                                            *
 * Copyright 2020 Lukas Jünger                                                *
 *                                                                            *
 * This software is licensed under the MIT license found in the               *
 * LICENSE file at the root directory of this source tree.                    *
 *                                                                            *
 ******************************************************************************/

#ifndef AVP_ARM64_CPU_H
#define AVP_ARM64_CPU_H

#include "vcml.h"
#include "ocx/ocx.h"
#include <signal.h>

namespace avp64 {

    class arm64_cpu_env;

    class memory_protector {
    private:
        memory_protector();
        std::map<vcml::u64, std::pair<arm64_cpu_env*, vcml::u64>> m_protected_pages;
    public:
        static memory_protector& get_instance() {
            static memory_protector inst;
            return inst;
        }

        static void segfault_handler(int sig, siginfo_t *si, void*);
        void register_page(arm64_cpu_env *cpu, vcml::u64 page_addr, void *host_addr);
        void notify_page(void* access_addr);
        memory_protector(const memory_protector&) = delete;
        void operator=(memory_protector const&) = delete;
    };

    enum arm_generic_timer_type {
        ARM_TIMER_PHYS = 0,
        ARM_TIMER_VIRT = 1,
        ARM_TIMER_HYP  = 2,
        ARM_TIMER_SEC  = 3,
    };

    typedef ocx::core* (*create_instance_t)(ocx::u64, ocx::env&, const char*);

    class arm64_cpu;

    class arm64_cpu_env : public ocx::env {
    private:
        static void segfault_handler(int sig, siginfo_t *si, void *unused);

        arm64_cpu *m_cpu;
        std::vector<std::shared_ptr<arm64_cpu>> m_syscall_subscriber;
    public:
        virtual ocx::u8* get_page_ptr_r(ocx::u64 page_paddr) override;
        virtual ocx::u8* get_page_ptr_w(ocx::u64 page_paddr) override;

        virtual void protect_page(ocx::u8* page_ptr, ocx::u64 page_addr) override;
        void memory_protector_update(vcml::u64 page_addr);

        virtual ocx::response transport(const ocx::transaction& tx) override;
        virtual void signal(ocx::u64 sigid, bool set) override;

        virtual void broadcast_syscall(int callno, std::shared_ptr<void> arg, bool async) override;

        virtual ocx::u64 get_time_ps() override;
        virtual const char* get_param(const char* name) override;

        virtual void notify(ocx::u64 eventid, ocx::u64 time_ps) override;
        virtual void cancel(ocx::u64 eventid) override;

        virtual void hint(ocx::hint_kind kind) override;

        virtual void handle_begin_basic_block(ocx::u64 vaddr) override;
        virtual bool handle_breakpoint(ocx::u64 vaddr) override;
        virtual bool handle_watchpoint(ocx::u64 vaddr, ocx::u64 size, ocx::u64 data, bool iswr) override;

        void inject_cpu(arm64_cpu *cpu);
        void add_syscall_subscriber(std::shared_ptr<arm64_cpu> cpu);

        arm64_cpu_env();
        arm64_cpu_env(const arm64_cpu_env&) = delete;
        virtual ~arm64_cpu_env();
    };

    class arm64_cpu : public vcml::processor {
        friend arm64_cpu_env;
    private:
        ocx::core *m_core;
        vcml::u64 m_core_id;
        arm64_cpu_env m_env;
        vcml::u64 m_run_cycles;
        vcml::u64 m_sleep_cycles;
        vcml::u64 m_total_cycles;
        void *m_ocx_handle;
        create_instance_t m_create_instance_func;

        void timer_irq_trigger(int timer_id);
    protected:
        virtual void interrupt(unsigned int irq, bool set) override;
        virtual void simulate(unsigned int cycles) override;
        virtual void end_of_elaboration() override;

        virtual vcml::u64 gdb_num_registers() override;
        virtual vcml::u64 gdb_register_width(vcml::u64 idx) override;
        virtual bool gdb_read_reg(vcml::u64 idx, void* p, vcml::u64 size) override;
        virtual bool gdb_write_reg(vcml::u64 idx, const void* p, vcml::u64 sz) override;
        virtual bool gdb_page_size(vcml::u64& size) override;
        virtual bool gdb_virt_to_phys(vcml::u64 vaddr, vcml::u64& paddr) override;
        virtual bool gdb_insert_breakpoint(vcml::u64 addr) override;
        virtual bool gdb_remove_breakpoint(vcml::u64 addr) override;
        virtual bool gdb_insert_watchpoint(const vcml::range& mem, vcml::vcml_access acs) override;
        virtual bool gdb_remove_watchpoint(const vcml::range& mem, vcml::vcml_access acs) override;
    public:
        vcml::out_port_list<bool> TIMER_IRQ_OUT;
        std::vector<std::shared_ptr<sc_core::sc_event>> timer_events;

        virtual vcml::u64 cycle_count() const override;
        virtual void update_local_time(sc_core::sc_time& local_time) override;
        virtual std::string disassemble(vcml::u64&, unsigned char*) override;
        virtual vcml::u64 get_program_counter() override;
        virtual vcml::u64 get_stack_pointer() override;
        virtual vcml::u64 get_core_id() override;
        virtual void gdb_notify(int signal) override;

        void handle_syscall(int callno, std::shared_ptr<void> arg);
        void add_syscall_subscriber(std::shared_ptr<arm64_cpu> cpu);
        vcml::u64 get_page_size();

        arm64_cpu() = delete;
        arm64_cpu(const arm64_cpu &) = delete;
        explicit arm64_cpu(const sc_core::sc_module_name &name, vcml::u64 procid, vcml::u64 coreid);
        virtual ~arm64_cpu();
    };
    
} // namespace avp64
#endif
