//
//	M5StickC Plus postscript library
//		Japanese font function module
//		(c) 2022	1YEN Toru
//


#include	<M5StickCPlus.h>
#include	<m5cp_ps.h>


#ifdef		ARDUINO_M5Stick_C_PLUS
#else	// ERROR:
#error	"This architecture is not supported."
#endif


int		tft_fwid;						// tft: screen size
int		tft_fhei;
int		tft_prn_lx;						// tft: cursor position
int		tft_prn_ly;
int		tft_prn_sx=tft_CHR_SIZ;			// tft: character size
int		tft_prn_sy=tft_CHR_SIZ;
uint16	tft_prn_fg=WHITE;				// tft: foreground color (pixel)
uint16	tft_prn_bg=BLACK;				// tft: background color (pixel)
uint16	tft_bitmap[tft_MAX_BTMP];		// tft: bit map buffer


bool	is_sjis1 (
int		chr)
{
	// is character chr S-JIS 1st byte code

	chr &= 0xff;

	return ((0x81<=chr && chr<=0x9f) || (0xe0<=chr && chr<=0xfc));
}

bool	is_sjis2 (
int		chr)
{
	// is character chr S-JIS 2nd byte code

	chr &= 0xff;

	return (0x40<=chr && chr<=0xfc && chr!=0x7f);
}

int		is_sjis (
const	char	*str)
{
	// test 1st character of string str
	// return: 0=(control code) / 1=(ascii code) / 2=(S-JIS 2 bytes code)
	int		chr;

	chr=(*str)&0xff;
	if (is_sjis1 (chr))
	{
		// possibility
		chr=(*(str + 1))&0xff;
		if (is_sjis2 (chr))
		{
			// yes S-JIS
			return (SJIS_2BYTE);
		}
		else
		{
			// not S-JIS
			return ((0xa1<=chr && chr<=0xdf)? SJIS_1BYTE: SJIS_0CTRL);
		}
	}
	else
	{
		// not S-JIS
		return ((0x20<=chr && chr<=0x7e)? SJIS_1BYTE: SJIS_0CTRL);
	}
}

int		sjis2kuten (
int		cods)
{
	// convert S-JIS code cods to KU-TEN code
	// cods: must be a S-JIS code, but it is not checked
	// return: KU-TEN code (0~sq (94) - 1) / on error (0)
	int		codk;						// KU code (1~94)
	int		codt;						// TEN code (1~94)
	int		codkt;						// KU-TEN code (0~sq (94) - 1)

	// conversion
	if (cods>=0xe000)
	{
		// 63~94 KU
		codt=cods - 0xe000 + 0x100*32;
	}
	else
	{
		// 1~62 KU
		codt=cods - 0x8100 + 0x100;
	}
	if ((codt&0xff)>=0x7f)
	{
		// rewind 1 TEN, because 0x7f is not a S-JIS code
		codt=codt - 1;
	}
	codk=(codt>>8)*2;
	codt=((codt&0xff) - 0x40 + 1);
	if (codt>tft_MAX_KU_TEN)
	{
		codt=codt - tft_MAX_KU_TEN;
	}
	else
	{
		codk=codk - 1;
	}
	codkt=(codk - 1)*tft_MAX_KU_TEN + codt - 1;

	// rescue: non 1st / 2nd level kanji code
	if (codkt>=tft_MAX_KU_TEN*114 - 1)
		codkt=codkt - tft_MAX_KU_TEN*26 - 29 + 1;

	if (codkt>=tft_MAX_KUTEN)
	{
		// ERROR: ignore
		codkt=0;
	}

	return (codkt);
}

int		is_utf8 (
int		chr)
{
	// test 1st character of string str
	// return: 0=(control code) / 1=(ascii code) / 2,3=(2,3 bytes code)

	// 1st byte of 4 byte code
	chr &= 0xf8;
	if (chr==0xf0)
		return (UTF8_4BYTE);
	// 1st byte of 3 byte code
	chr &= 0xf0;
	if (chr==0xe0)
		return (UTF8_3BYTE);
	// 1st byte of 2 byte code
	chr &= 0xe0;
	if (chr==0xc0)
		return (UTF8_2BYTE);
	// 2nd byte code
	chr &= 0xc0;
	if (chr==0x80)
		return (UTF8_0CTRL);

	return (UTF8_1BYTE);
}

int		is_utf8 (
const	char	*str)
{
	// test 1st character of string str
	// return: 0=(control code) / 1~3=(UTF-8 1~3 bytes code)

	// 4 bytes code
	if (is_utf8 (*str)==UTF8_4BYTE &&
		is_utf8 (*(str + 1))==UTF8_0CTRL &&
		is_utf8 (*(str + 2))==UTF8_0CTRL &&
		is_utf8 (*(str + 3))==UTF8_0CTRL)
		return (UTF8_4BYTE);
	// 3 bytes code
	if (is_utf8 (*str)==UTF8_3BYTE &&
		is_utf8 (*(str + 1))==UTF8_0CTRL &&
		is_utf8 (*(str + 2))==UTF8_0CTRL)
		return (UTF8_3BYTE);
	// 2 bytes code
	if (is_utf8 (*str)==UTF8_2BYTE &&
		is_utf8 (*(str + 1))==UTF8_0CTRL)
		return (UTF8_2BYTE);
	// 1 bytes code
	if (is_utf8 (*str)==UTF8_1BYTE && 0x20<=(*str) && (*str)<=0x7e)
		return (UTF8_1BYTE);

	return (UTF8_0CTRL);
}

int		utf82unic (
const	char	*str)
{
	// convert UTF-8 multi byte code to unicode
	int		cod;
	int		cnt;

	cnt=is_utf8 (str);
	if (cnt==UTF8_4BYTE)
	{
		// 4 bytes code
		cod=(*str)&0x07;
		str++;
		cod=(cod<<6) + ((*str)&0x3f);
		str++;
		cod=(cod<<6) + ((*str)&0x3f);
		str++;
		cod=(cod<<6) + ((*str)&0x3f);
	}
	else if (cnt==UTF8_3BYTE)
	{
		// 3 bytes code
		cod=(*str)&0x0f;
		str++;
		cod=(cod<<6) + ((*str)&0x3f);
		str++;
		cod=(cod<<6) + ((*str)&0x3f);
	}
	else if (cnt==UTF8_2BYTE)
	{
		// 2 bytes code
		cod=(*str)&0x1f;
		str++;
		cod=(cod<<6) + ((*str)&0x3f);
	}
	else
	{
		// 1 byte code, including control code
		cod=(*str)&0x7f;
	}

	return (cod);
}

int		unic2sjis (
int		codu,
int		onerr)
{
	// convert unicode to S-JIS
	int		cods;
	int		codh;
	int		ll;
	int		rr;
	int		mm;

	// not support
	if (codu>0xffff)
		return (onerr);

	// hash table
	codh=codu&0xff;
	codu=(codu>>8)&0xff;
	ll=unsj_hash_tab[codh];
	rr=unsj_hash_tab[codh + 1] - 1;

	// binary search
	while (rr>=ll)
	{
		mm=(rr + ll)/2;
		if (unsj_lkup_tab[mm*3]==codu)
			break;
		else if (unsj_lkup_tab[mm*3]>codu)
			rr=mm - 1;
		else
			ll=mm + 1;
	}
	if (rr>=ll)
		cods=(unsj_lkup_tab[mm*3 + 1]<<8) + unsj_lkup_tab[mm*3 + 2];
	else
		cods=onerr;						// not found

	return (cods);
}

char	*utf82sjis (
char	*str,
int		onerr)
{
	// convert and replace string str UTF-8 to S-JIS
	const	char	*str_c;
	char	*str_p;
	int		cnt;
	int		cod;

	str_p=str;
	for (str_c=str; (*str_c)!='\0'; str_c++)
	{
		cnt=is_utf8 (str_c);
		if (cnt==4)
		{
			// 4 bytes code
			cod=utf82unic (str_c);
			str_c += 3;
			cod=unic2sjis (cod, onerr);
			(*str_p)=(cod>>8)&0xff;
			str_p++;
			(*str_p)=cod&0xff;
			str_p++;
		}
		else if (cnt==3)
		{
			// 3 bytes code
			cod=utf82unic (str_c);
			str_c += 2;
			cod=unic2sjis (cod, onerr);
			if (cod>=0xff)
			{
				// except S-JIS KATAKANA code
				(*str_p)=(cod>>8)&0xff;
				str_p++;
			}
			(*str_p)=cod&0xff;
			str_p++;
		}
		else if (cnt==2)
		{
			// 2 bytes code
			cod=utf82unic (str_c);
			str_c++;
			cod=unic2sjis (cod, onerr);
			(*str_p)=(cod>>8)&0xff;
			str_p++;
			(*str_p)=cod&0xff;
			str_p++;
		}
		else
		{
			// 1 byte code, including control code
			(*str_p)=(*str_c);
			str_p++;
		}
	}
	(*str_p)='\0';

	return (str);
}


uint16	tft_rgb2pix (
int		ir,
int		ig,
int		ib)
{
	// 6 bits rgb to 16 bits pixel {R5,G6,B5} conversion
	// ir, ig, ib: 0~63
	// return: 16 bits pixel
	uint16	pix;

	ir=(ir<0)? 0: (ir>63)? 63: ir;
	ig=(ig<0)? 0: (ig>63)? 63: ig;
	ib=(ib<0)? 0: (ib>63)? 63: ib;
	pix=((ir&0x3e)<<10) | ((ig&0x3f)<<5) | ((ib&0x3e)>>1);

	return (pix);
}

void	tft_locate (
int		lx,
int		ly)
{
	// set print location (lx, ly) [dot]

	tft_fwid=M5.Lcd.width ();
	tft_fhei=M5.Lcd.height ();
	lx=(lx<0)? 0: (lx>tft_fwid - 1)? tft_fwid - 1: lx;
	ly=(ly<0)? 0: (ly>tft_fhei - 1)? tft_fhei - 1: ly;
	tft_prn_lx=lx;
	tft_prn_ly=ly;
}

void	tft_font_size (
int		sx,
int		sy)
{
	// set font size sx*sy [dot]

	sx=(sx<tft_CHR_SIZ)? tft_CHR_SIZ: (sx>255)? 255: sx;
	sy=(sy<tft_CHR_SIZ)? sx: (sy>255)? 255: sy;
	tft_prn_sx=sx;
	tft_prn_sy=sy;
}

void	tft_color (
int		fg,
int		bg)
{
	// set print color fg, bg ; 16 bits pixel {R5,G6,B5}

	tft_prn_fg=fg&0xffff;
	if (bg>=0)
		tft_prn_bg=bg&0xffff;
}

void	tft_clear (void)
{
	// clear tft screen

	M5.Lcd.fillScreen (BLACK);
	tft_locate (0,0);
}

void	tft_putchr_kz (
int		codkt)
{
	// put character codkt to cursor position
	// codkt: kuten code (0~84*94-1), zenkaku kanji font
	//	0x0300<=codkt<=0x03ff: hankaku ascii font
	int		idx;
	int		ix;
	int		iy;
	int		siz;
	uint16	dat;
	const	uint16	*fnt;

	if (tft_prn_font_kz==NULL)
	{
		// ERROR: no font data, ignore
		return;
	}

	// parameter
	if (codkt<0 || 84*tft_MAX_KU_TEN<=codkt)
	{
		// ERROR: ignore
		codkt=0;
	}
	siz=tft_CHR_SIZ;
	if (0x0300<=codkt && codkt<=0x03ff)
		siz=tft_FNT_SIZ/2 + tft_FNT_MGN;

	// draw character
	fnt=&tft_prn_font_kz[codkt*tft_FNT_SIZ];
	idx=0;
	for (iy=0; iy<tft_CHR_SIZ; iy++)
	{
		if (iy>=tft_FNT_SIZ)
			dat=0;
		else
		{
			dat=(*fnt);
			fnt++;
		}
		for (ix=0; ix<siz; ix++)
		{
			if (dat&0x8000U)
				tft_bitmap[idx]=tft_prn_fg;
			else
				tft_bitmap[idx]=
					(tft_prn_fg==tft_prn_bg)? ~tft_prn_fg: tft_prn_bg;
			dat <<= 1;
			idx++;
		}
	}

	// draw
	if (tft_prn_fg==tft_prn_bg)
		M5.Lcd.drawBitmap (tft_prn_lx,tft_prn_ly,siz,tft_CHR_SIZ,
			&tft_bitmap[0],~tft_prn_fg);
	else
		M5.Lcd.drawBitmap (tft_prn_lx,tft_prn_ly,siz,tft_CHR_SIZ,
			&tft_bitmap[0]);
}

int		tft_ksymbol_chr (
const	char	*str)
{
	// draw kanji symbol *str
	// return: 1=drawn 1 byte code / 2=drawn 2 bytes code
	int		siz;
	int		codkt;
	int		ix;
	int		iy;
	int		x1;
	int		y1;
	int		x2;
	int		y2;
	uint16	dat;
	const	uint16	*fnt;

	// parameter
	siz=(is_sjis (str)==SJIS_2BYTE)? 2: 1;
	if (siz==1)
	{
		// ascii, 1 byte code
		codkt=(*str)&0xff;
		if (codkt<0x20 || 0xff<codkt)
		{
			// ERROR: ignore
			codkt=0x20;
		}
		codkt += 0x0300;
	}
	else
	{
		// sjis, 2 bytes code
		codkt=(((*str)&0xff)<<8) | ((*(str + 1))&0xff);
		codkt=sjis2kuten (codkt);
	}
	codkt=codkt*tft_FNT_SIZ;

	// draw character
	fnt=&tft_prn_font_kz[codkt];
	for (iy=0; iy<tft_FNT_SIZ + tft_FNT_MGN; iy++)
	{
		y1=tft_prn_ly + iy*tft_prn_sy/(tft_FNT_SIZ + tft_FNT_MGN);
		y2=tft_prn_ly + (iy + 1)*tft_prn_sy/(tft_FNT_SIZ + tft_FNT_MGN) - 1;
		if (y1>tft_fhei - 1)
			break;
		else if (y2>tft_fhei - 1)
			y2=tft_fhei - 1;
		if (iy>=tft_FNT_SIZ)
			dat=0;
		else
		{
			dat=(*fnt);
			fnt++;
		}
		for (ix=0; ix<tft_FNT_SIZ*siz/2 + tft_FNT_MGN; ix++)
		{
			x1=tft_prn_lx +
				ix*(tft_prn_sx*siz/2 + 1)/
				(tft_FNT_SIZ*siz/2 + tft_FNT_MGN);
			x2=tft_prn_lx +
				(ix + 1)*(tft_prn_sx*siz/2 + 1)/
				(tft_FNT_SIZ*siz/2 + tft_FNT_MGN) - 1;
			if (x1>tft_fwid - 1)
				break;
			else if (x2>tft_fwid - 1)
				x2=tft_fwid - 1;
			if (dat&0x8000U)
			{
				// foreground color
				M5.Lcd.fillRect (x1,y1,x2 - x1 + 1,y2 - y1 + 1, tft_prn_fg);
			}
			else if (tft_prn_fg!=tft_prn_bg)
			{
				// background color
				M5.Lcd.fillRect (x1,y1,x2 - x1 + 1,y2 - y1 + 1, tft_prn_bg);
			}
			dat <<= 1;
		}
	}

	return (siz);
}

void	tft_kprint_upcs (void)
{
	// kanji print: update cursor position

	if (tft_prn_lx>tft_fwid - 1)
	{
		tft_prn_lx=0;
		tft_prn_ly += tft_prn_sy;
	}
	else if (tft_prn_lx<0)
	{
		tft_prn_lx=tft_fwid - tft_prn_sx;
		tft_prn_ly -= tft_prn_sy;
	}
	if (tft_prn_ly>tft_fhei - tft_prn_sy)
	{
		tft_prn_ly=0;
	}
	else if (tft_prn_ly<0)
	{
		tft_prn_ly=tft_fhei - tft_prn_sy;
	}
}

void	tft_kprint (
const	char	*str,
int		opt)
{
	// kanji print: string str
	//	opt:kpr_OPT_UTF8: str encoded in S-JIS code
	//		kpr_OPT_UTF8: str encoded in UTF-8 code

	// parameter
	if (str==NULL || (*str)=='\0')
	{
		// ERROR: ignore
		return;
	}
	tft_fwid=M5.Lcd.width ();
	tft_fhei=M5.Lcd.height ();

	// option
	if (opt==kpr_OPT_UTF8)
	{
		char	*str_sjis;
		int		len;

		len=strlen (str);

		str_sjis=new char [len + 1];
		if (str_sjis==NULL)
		{
			// ERROR: ignore
			return;
		}

		strcpy (str_sjis,str);
		utf82sjis (str_sjis, 0x8240);	// 0x8240=undefined
		tft_kprint (str_sjis);

		delete [] str_sjis;

		return;
	}

	// print
	for (; (*str)!='\0'; str++)
	{
		if ((*str)=='\r')
		{
			// CR
			tft_prn_lx=0;
		}
		else if ((*str)=='\n')
		{
			// LF
			tft_prn_ly += tft_prn_sy;
			tft_kprint_upcs ();
		}
		else if ((*str)=='\v')
		{
			// cursor up
			tft_prn_ly -= tft_prn_sy;
			tft_kprint_upcs ();
		}
		else if ((*str)=='\b')
		{
			// cursor left
			tft_prn_lx -= tft_prn_sx;
			tft_kprint_upcs ();
		}
		else if ((*str)=='\t')
		{
			// cursor right
			tft_prn_lx += tft_prn_sx;
			tft_kprint_upcs ();
		}
		else if ((*str)=='\f')
		{
			// clear screen
			M5.Lcd.fillScreen (BLACK);
			tft_prn_lx=0;
			tft_prn_ly=0;
		}
		else if ((*str)=='\x10' || (*str)=='\x11')
		{
			// set cursor x / y
			uint8	chr;
			int		cs;

			chr=(*str);
			str++;
			cs=(*str)&0xff;
			if (chr=='\x10')
			{
				cs=(cs>tft_fwid - 1)? tft_fwid - 1: cs;
				tft_prn_lx=cs;
			}
			else
			{
				cs=(cs>tft_fhei - 1)? tft_fhei - 1: cs;
				tft_prn_ly=cs;
			}
		}
		else if ((*str)=='\x12' || (*str)=='\x13')
		{
			// set foreground / background
			uint8	chr;
			uint16	pix;

			chr=(*str);
			str++;
			pix=((*str)&0xff)<<8;
			str++;
			pix |= (*str)&0xff;
			if (chr=='\x12')
				tft_prn_fg=pix;
			else
				tft_prn_bg=pix;
		}
		else if ((*str)=='\x14')
		{
			// set font size
			str++;
			tft_prn_sx=(*str)&0xff;
			str++;
			tft_prn_sy=(*str)&0xff;
			tft_prn_sx=(tft_prn_sx<tft_CHR_SIZ)? tft_CHR_SIZ: tft_prn_sx;
			tft_prn_sy=(tft_prn_sy<tft_CHR_SIZ)? tft_CHR_SIZ: tft_prn_sy;
		}
		else
		{
			// kprint
			int		codkt;
			int		siz;

			if (is_sjis (str)==SJIS_2BYTE)
			{
				// S-JIS
				codkt=(((*str)&0xff)<<8) | ((*(str + 1))&0xff);
				codkt=sjis2kuten (codkt);
				siz=tft_prn_sx;
			}
			else
			{
				// ASCII / KATAKANA
				codkt=((*str)&0xff) + 0x0300;
				siz=(tft_prn_sx + 1)/2;
			}
			if (tft_prn_lx>tft_fwid - siz)
			{
				// new line
				tft_prn_lx=0;
				tft_prn_ly += tft_prn_sy;
				tft_kprint_upcs ();
			}
			if (tft_prn_sx==tft_CHR_SIZ && tft_prn_sy==tft_CHR_SIZ)
			{
				// kprint
				tft_putchr_kz (codkt);
			}
			else
			{
				// ksymbol
				tft_ksymbol_chr (str);
			}
			tft_prn_lx += siz;
			tft_kprint_upcs ();
			if (is_sjis (str)==SJIS_2BYTE)
				str++;
		}
	}
}
