
/* Linker script by Lou Sortman */

MEMORY 
{
	flash	: ORIGIN = 0x00100000, LENGTH = 256K
	ram	: ORIGIN = 0x00200000, LENGTH = 64K
}


SECTIONS 
{
	__HEAPSIZE__ = 0;
	__STACKSIZE__ = 1500;
	__STACKSIZE_IRQ__ = 512;
	__STACKSIZE_FIQ__ = 512;
	__STACKSIZE_SVC__ = 256;
	__STACKSIZE_ABT__ = 0;
	__STACKSIZE_UND__ = 0;


	.vectors : {
		__vectors_ram_start__ = .;
		__vectors_load_start__ = . + (LOADADDR(.vectors) - ADDR(.vectors));
		*(.vectors .vectors.*)
		. = ALIGN(4);
		__vectors_ram_end__ = .;
	} >ram AT>flash


	.init : {
		*(.init .init.*)
		. = ALIGN(4);
	} >flash


	.text : {
		__text_start__ = .;
		__text_load_start__ = .;
		*(.text .text.* .glue_7t .glue_7 .gnu.linkonce.t.*)
		. = ALIGN(4);
		__text_end__ = .;
	} >flash


	.dtors : {
		__dtors_start__ = .;
		KEEP (*(SORT(.dtors.*))) KEEP (*(.dtors))
		. = ALIGN(4);
		__dtors_end__ = .;
	} >flash


	.ctors : {
		__ctors_start__ = .;
		KEEP (*(SORT(.ctors.*))) KEEP (*(.ctors))
		. = ALIGN(4);
		__ctors_end__ = .;
	} >flash


	.rodata : {
		*(.rodata .rodata.* .gnu.linkonce.r.*)
		. = ALIGN(4);
	} >flash


	.fast : {
		__fast_start__ = .;
		__fast_load_start__ = . + (LOADADDR(.fast) - ADDR(.fast));
		*(.fast .fast.*)
		. = ALIGN(4);
		__fast_end__ = .;
	} >ram AT>flash


	.stack : {
		__stack_start__ = .;
		*(.stack)
		. = MAX(__stack_start__ + __STACKSIZE__ , .);
		. = ALIGN(4);
		__stack_end__ = .;
	} >ram


	.stack_irq : {
		__stack_irq_start__ = .;
		*(.stack_irq)
		. = MAX(__stack_irq_start__ + __STACKSIZE_IRQ__ , .);
		. = ALIGN(4);
		__stack_irq_end__ = .;
	} >ram


	.stack_fiq : {
		__stack_fiq_start__ = .;
		*(.stack_fiq)
		. = MAX(__stack_fiq_start__ + __STACKSIZE_FIQ__ , .);
		. = ALIGN(4);
		__stack_fiq_end__ = .;
	} >ram


	.stack_svc : {
		__stack_svc_start__ = .;
		*(.stack_svc)
		. = MAX(__stack_svc_start__ + __STACKSIZE_SVC__ , .);
		. = ALIGN(4);
		__stack_svc_end__ = .;
	} >ram


	.stack_abt : {
		__stack_abt_start__ = .;
		*(.stack_abt)
		. = MAX(__stack_abt_start__ + __STACKSIZE_ABT__ , .);
		. = ALIGN(4);
		__stack_abt_end__ = .;
	} >ram


	.stack_und : {
		__stack_und_start__ = .;
		*(.stack_und)
		. = MAX(__stack_und_start__ + __STACKSIZE_UND__ , .);
		. = ALIGN(4);
		__stack_und_end__ = .;
	} >ram


	.data : {
		__data_start__ = .;
		__data_load_start__ = . + (LOADADDR(.data) - ADDR(.data));
		*(.data .data.* .gnu.linkonce.d.*)
		. = ALIGN(4);
		__data_end__ = .;
	} >ram AT>flash


	.bss : {
		__bss_start__ = .;
		*(.bss .bss.* .gnu.linkonce.b.*)
		*(COMMON)
		. = ALIGN(4);
		__bss_end__ = .;
	} >ram

	_bss_end__ = __bss_end__;
	__end__ = __bss_end__;
	_end__ = __end__;
	PROVIDE (end = __end__);


	/* this is here to keep the CrossWorks startup code happy. */
	/* This could conceivably be used by the MC firmware, but it establishes its own heap space internally */

	__heap_end__ = ALIGN(ORIGIN(ram) + LENGTH(ram) - (4 - 1), 4);

	.heap : {
		__heap_start__ = .;
		*(.heap)
		. = MAX(__heap_start__ + __HEAPSIZE__ , .);
		. = ALIGN(4);
	} > ram

	. = ASSERT(
		__heap_end__ >= (__heap_start__ + (2 * 4)),
		"error: insufficient free RAM to establish heap for startup code"
	);

}
