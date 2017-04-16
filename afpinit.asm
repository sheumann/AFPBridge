	case	on

dummy	private
	bra	InitStart
	end

unloadFlagPtr data
	ds	4
	end

* This gets called when control-reset is pressed in P8 mode.
* It's intended to basically reinitialize the whole AppleTalk stack.
resetRoutine start
	jsl	oldSoftReset
	jml	ResetAllSessions
	end

oldSoftReset start
	ds	4		; to be modified
	end

InitStart private
	tay
	tsc
	clc
	adc	#4
	sta	>unloadFlagPtr
	lda	#0
	sta	>unloadFlagPtr+2
	tya
	end
