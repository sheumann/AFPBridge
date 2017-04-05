	case	on

dummy	private
	jml	InitStart
	end

unloadFlagPtr data
	ds	4
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
