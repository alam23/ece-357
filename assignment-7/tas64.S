	.text
.globl tas
	.type	tas,@function
tas:
	movq	$1, %rax
#APP
	lock;xchgb	%al,(%rdi)
#NO_APP
	movsbq	%al,%rax
	ret
.Lfe1:
	.size	tas,.Lfe1-tas
