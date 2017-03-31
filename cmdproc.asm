	case	on

* Location of command rec ptr on entry to an AppleTalk command procedure
* (in the system zero page, which is the current direct page)
cmdRecPtr gequ	$80

* Bogus segment to go into the .root file and force generation of .a/.o file
bogus	private
	nop
	end

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
