/ chmod - change mode

chmode:
	mov	sp,r5
	mov	(r5),r4
	cmp	r4,$3
	blt	chmerr
	add	$4,r5
	mov	(r5)+,r1
	clr	0f
1:
	movb	(r1)+,r0
	beq	1f
	asl	0f
	asl	0f
	asl	0f
	bic	$!7,r0
	bis	r0,0f
	br	1b
1:
	mov	(r5)+,0f-2
	sys	chmod; ..; 0:..
	bes	chmerr
	dec	r4
	cmp	r4,$3
	bge	1b
	sys	exit

chmerr:
	mov	$1,r0
	sys	write; 1f; 2
	sys	exit

1:	<?\n>
