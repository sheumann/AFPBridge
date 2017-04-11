	case	on

dummy	private
	end

FreeAllCDevMem start
	pea	0
	jsl	~DAID
	rtl
	end
