	case	on
	mcopy	asmglue.macros

ROMIN	gequ	$C081
LCBANK2	gequ	$C083
STATEREG	gequ	$C068

ForceLCBank2 start
	short	i,m
	lda	>STATEREG	;get original state reg.
	tax
	lda	>LCBANK2		;force LC bank 2
	lda	>LCBANK2
	long	i,m
	txa
	rtl
	end

ForceRomIn start
	short	i,m
	lda	>STATEREG	;get original state reg.
	tax
	lda	>ROMIN		;force ROM in to Language Card space
	lda	>ROMIN
	long	i,m
	txa
	rtl
	end

RestoreStateReg start
	short	m
	plx
	pla
	ply
	pha
	phx
	tya
	sta	>STATEREG
	long	m
	rtl
	end
