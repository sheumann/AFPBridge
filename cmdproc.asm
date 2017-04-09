	case	on

RamGoComp gequ	$E1100C
RamForbid gequ	$E11018
RamPermit gequ	$E1101C

* Location of command rec ptr on entry to an AppleTalk command procedure
* (in the system zero page, which is the current direct page)
cmdRecPtr gequ	$80

* Location to put completion routine ptr before calling RamGoComp
compPtr	gequ	$84


* AppleTalk command procedure (which acts as a dispatcher for all commands)
cmdProc	start
	lda	3,s
	pha
	lda	3,s
	pha
	lda	cmdRecPtr
	sta	4,s
	lda	cmdRecPtr+2
	sta	6,s
	jml	DispatchASPCommand
	end

nbpCmdProc start
	lda	3,s
	pha
	lda	3,s
	pha
	lda	cmdRecPtr
	sta	4,s
	lda	cmdRecPtr+2
	sta	6,s
	jml	DoLookupName
	end

CallCompletionRoutine start
	phb
	jsl	ForceLCBank2	;use LC bank 2
	pha
	phd
	lda	#0		;set direct page = system zero page
	tcd
	
	jsl	RamForbid
	lda	9,s
	sta	compPtr
	lda	9+2,s
	sta	compPtr+2
	ora	9,s
	beq	skip		;skip call if compPtr = 0
	jsl	>RamGoComp
skip	jsl	RamPermit
	pld
	jsl	RestoreStateReg
	
	pla
	sta	3,s
	pla
	sta	3,s
	plb
	rtl
	end
