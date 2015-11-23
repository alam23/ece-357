#include "jmpbuf-offsets.h"

	.text
	.global	savectx
	.global restorectx
/* This looks like a function to C, but we skip the usual stack */
/* frame stuff since there are no local variables */
savectx:
	movl	4(%esp),%eax	 	/* Move jump buffer addr into eax */
	movl	%ebx, (JB_BX*4)(%eax)
	movl	%esi, (JB_SI*4)(%eax)
	movl	%edi, (JB_DI*4)(%eax)
	movl	%ebp, (JB_BP*4)(%eax)
	leal	4(%esp), %ecx			/*SP when we return */
	movl	%ecx,(JB_SP*4)(%eax)
	movl	0(%esp), %ecx			/*return address */
	movl	%ecx, (JB_PC*4)(%eax)		/*return address */
	xorl	%eax,%eax			/*return val will be 0 */
	ret

/* Call restorectx(void *jmpbuf, int arg)		*/
restorectx:
	movl	4(%esp),%ecx		/*Move jump buffer addr to ecx */
	movl	8(%esp),%eax		/*Longjmp return value */
	movl	(JB_PC*4)(%ecx),%edx	/*Save return addr in edx */
	/* Restore all registers */
	movl	(JB_BX*4)(%ecx),%ebx
	movl	(JB_SI*4)(%ecx),%esi
	movl	(JB_DI*4)(%ecx),%edi
	movl	(JB_BP*4)(%ecx),%ebp
	movl	(JB_SP*4)(%ecx),%esp
	/* Jump to setjmp point */
	jmp	*%edx