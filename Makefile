PROJECT := $(dir $(lastword $(MAKEFILE_LIST)))
BUILDDIR := $(PROJECT)/build
SCRIPTS := $(PROJECT)/scripts
KERNEL_IMG := $(BUILDDIR)/kernel.img
QEMU_OPTS := -machine raspi3b -nographic -serial mon:stdio -m size=1G -kernel $(KERNEL_IMG)

all: build

build:
	@echo "Building kernel image..."
	@mkdir -p $(BUILDDIR)
	@cd $(BUILDDIR) && \
		cmake .. && \
		make -j4

qemu: build
	qemu-system-aarch64 $(QEMU_OPTS)

qemu-gdb: build
	qemu-system-aarch64 -S -gdb tcp::1234 $(QEMU_OPTS)

gdb:
	gdb-multiarch --nx -x $(SCRIPTS)/gdb/gdbinit

.PHONY: all build qemu qemu-gdb gdb