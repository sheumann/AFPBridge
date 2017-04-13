	case	on

* This is the actual entry point for the CDEV. 
* We continue to the C code in most cases, but
* handle EventsCDEV messages here for performance.
cdeventry start
	lda	12,s		; get message
	cmp	#6		; is it EventsCDEV?
	bne	continue
	
doEvent	anop			; handle an EventsCDEV message
	pla			; move return address
	sta	9,s
	pla
	sta	9,s
	
	tsc
	phd
	tcd
	ldy	#14		; modifiers field in event structure
	lda	[4],y		; get data1->modifiers & save it away
	sta	>modifiers
	stz	10		; result = 0 (necessary?)
	stz	12
	pld
	
	tsc
	clc
	adc	#6
	tcs
	rtl
	end

FreeAllCDevMem start
	pea	0
	jsl	~DAID
	rtl
	end

continue	private
	end
