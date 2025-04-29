PROJECT := $(dir $(lastword $(MAKEFILE_LIST)))
BUILDDIR := $(PROJECT)/build
SCRIPTS := $(PROJECT)/scripts
TOOLS := $(PROJECT)/tools
KERNEL_IMG := $(BUILDDIR)/kernel.img
# 第一个 -serial 表示禁用PL011，第二个表示启用mini uart
# QEMU_OPTS := -machine raspi3b -nographic -serial null -serial mon:stdio -m size=1G -kernel $(KERNEL_IMG)
QEMU_OPTS := -machine raspi3b -nographic -serial mon:stdio -m size=1G -kernel $(KERNEL_IMG)

all: build

format:
	@echo "Formatting code..."
	@bash ${TOOLS}/format.sh

build: format
	@echo "Building kernel image..."
	@mkdir -p $(BUILDDIR)
	@cd $(BUILDDIR) && \
		cmake -DMKM_KERNEL_DEBUG=1 .. && \
		make -j4

qemu: build
	qemu-system-aarch64 $(QEMU_OPTS)

qemu-gdb: build
	qemu-system-aarch64 -S -gdb tcp::1234 $(QEMU_OPTS)

gdb:
	gdb-multiarch --nx -x $(SCRIPTS)/gdb/gdbinit

.PHONY: all build qemu qemu-gdb gdb format