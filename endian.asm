	case	on

htons	start
ntohs	entry
	ply
	phb
	plx
	pla
	xba
	phx
	plb
	phy
	rtl
	end

htonl	start
ntohl	entry
	lda	4,s
	xba
	tax
	lda	6,s
	xba
	tay
	phb
	pla
	sta	3,s
	pla
	sta	3,s
	plb
	tya
	rtl
	end
