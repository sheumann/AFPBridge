	case	on

* Bogus segment to go into the .root file and force generation of .a/.o file
bogus	private
	nop
	end

RamDispatch gequ $E11014

_CALLAT	start
	lda	4,s
	tax
	lda	6,s
	tay
	phb
	pla
	sta	3,s
	pla
	sta	3,s
	plb
	jml	RamDispatch
	end
