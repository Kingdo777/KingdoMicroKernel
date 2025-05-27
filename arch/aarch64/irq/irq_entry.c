#include <irq/irq.h>
#include <arch/machine/esr.h>
#include <common/utils.h>
#include <common/macro.h>
#include "irq_entry.h"

u8 irq_handle_type[MAX_IRQ_NUM];

void arch_interrupt_init_per_cpu(void)
{
	disable_irq();
	set_exception_vector();
	plat_interrupt_init();
}

void arch_interrupt_init(void)
{
	arch_interrupt_init_per_cpu();
	memset(irq_handle_type, HANDLE_KERNEL, MAX_IRQ_NUM);
}

u64 handle_sync(int type, u64 esr, u64 address)
{
	/* ec: exception class */
	u32 esr_ec = GET_ESR_EL1_EC(esr);

	kdebug("Exception type: %d, ESR: 0x%lx, Fault address: 0x%lx, "
	       "EC 0b%b\n",
	       type, esr, address, esr_ec);

	if (type < SYNC_EL0_64) {
		if (esr_ec != ESR_EL1_EC_DABT_CEL) {
			kinfo("%s: irq type is %d\n", __func__, type);
			BUG_ON(1);
		}
	}

	u64 fix_addr = 0;
	switch (esr_ec) {
	case ESR_EL1_EC_UNKNOWN:
		kdebug("Unknown\n");
		break;
	case ESR_EL1_EC_WFI_WFE:
		kdebug("Trapped WFI or WFE instruction execution\n");
		return address;
	case ESR_EL1_EC_ENFP:
		kdebug("Access to SVE, Advanced SIMD, or floating-point functionality\n");
		break;
	case ESR_EL1_EC_ILLEGAL_EXEC:
		kdebug("Illegal Execution state\n");
		break;
	case ESR_EL1_EC_SVC_32:
		kdebug("SVC instruction execution in AArch32 state\n");
		break;
	case ESR_EL1_EC_SVC_64:
		kdebug("SVC instruction execution in AArch64 state\n");
		break;
	case ESR_EL1_EC_MRS_MSR_64:
		kdebug("Using MSR or MRS from a lower Exception level\n");
		break;
	case ESR_EL1_EC_IABT_LEL:
		kdebug("Instruction Abort from a lower Exception level\n");
		// do_page_fault(esr, address, type, &fix_addr);
		return address;
	case ESR_EL1_EC_IABT_CEL:
		kinfo("Instruction Abort from current Exception level\n");
		break;
	case ESR_EL1_EC_PC_ALIGN:
		kdebug("PC alignment fault exception\n");
		break;
	case ESR_EL1_EC_DABT_LEL:
		kdebug("Data Abort from a lower Exception level\n");
		// do_page_fault(esr, address, type, &fix_addr);
		return address;
	case ESR_EL1_EC_DABT_CEL:
		kdebug("Data Abort from a current Exception level\n");
		// do_page_fault(esr, address, type, &fix_addr);
		if (fix_addr)
			return fix_addr;
		else
			return address;
	case ESR_EL1_EC_SP_ALIGN:
		kdebug("SP alignment fault exception\n");
		break;
	case ESR_EL1_EC_FP_32:
		kdebug("Trapped floating-point exception taken from AArch32 state\n");
		break;
	case ESR_EL1_EC_FP_64:
		kdebug("Trapped floating-point exception taken from AArch64 state\n");
		break;
	case ESR_EL1_EC_SError:
		kdebug("SERROR\n");
		break;
	default:
		kdebug("Unsupported Exception ESR %lx\n", esr);
		break;
	}

	BUG("Exception type: %d, ESR: 0x%lx, Fault address: 0x%lx, "
	    "EC 0b%b\n",
	    type, esr, address, esr_ec);
	__builtin_unreachable();
}

void handle_irq(void)
{
	plat_handle_irq();
}

void unexpected_handler(void)
{
	BUG("[fatal error] unexpected_handler\n");
	__builtin_unreachable();
}
