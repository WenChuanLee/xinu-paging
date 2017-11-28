/* pagedault.S - page fault handler(x86) */

/*------------------------------------------------------------------------
 * pagefault  -  Interrupt dispatcher for page faults (x86 version)
 *------------------------------------------------------------------------
 */
		.text
		.globl	pagefault		
pagefault:
		
		pushl	%eax
		pushl	%ecx
		pushl	%edx
		pushl	%ebx
		sub		$4,	%esp
		pushl	%ebp
		pushl	%esi
		pushl	%edi
		
		cli			# Disable further interrupts

		pushl	32(%esp)
		call	pagefaultHandler	# Call high level handler
		add		$4, %esp

		sti			# Restore interrupt status
		
		popl	%edi
		popl	%esi
		popl	%ebp
		add		$4, %esp
		popl	%ebx
		popl	%edx
		popl	%ecx
		popl	%eax
		
		add		$4, %esp			# remove error code
		iret		# Return from interrupt
