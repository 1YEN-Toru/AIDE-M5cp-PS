//
//	M5StickC Plus postscript library
//		definition module
//		(c) 2022	1YEN Toru
//
//
//	2024/03/30	ver.1.10
//		update: MML: toupper() for "T" and "K" command
//
//	2023/05/13	ver.1.08
//		add: play_mml(); play MML function (blocking)
//		add: play_bgm(); play MML function (non-blocking)
//
//	2023/03/04	ver.1.06
//		add: i2c1_write(); byte write function
//		add: strnum3c()
//		changed battery ICON during battery charging
//
//	2023/01/14	ver.1.04
//		add: i2c1_read(), i2c1_write()
//		add: draw_title()
//		corresponding to AXP192(PMIC)
//		corresponding to BM8563(RTC)
//
//	2022/11/12	ver.1.02
//		corresponding to unicode -> S-JIS conversion
//
//	2022/11/05	ver.1.00
//


#ifndef		M5CP_PS
#define		M5CP_PS		"1.08"


#include	<Arduino.h>


#ifndef		XINTX
#define		XINTX
typedef		signed	char	int8;
typedef		short	int16;
typedef		long	int32;
typedef		unsigned	char	uint8;
typedef		unsigned	short	uint16;
typedef		unsigned	long	uint32;
#endif	//	XINTX


#ifndef		LED_BUILTIN
#define		LED_BUILTIN		10
#endif	//	LED_BUILTIN
#ifndef		LED_BUILTIN_ON
#define		LED_BUILTIN_ON		LOW
#define		LED_BUILTIN_OFF		HIGH
#endif	//	LED_BUILTIN_ON


const	int		M5psErrNo=0;			// err: No error
const	int		M5psErrBadParam=-101;	// err: bad parameter
const	int		M5psErrI2cRecv=-102;	// err: i2c: receive
const	int		M5psErrI2cSend=-103;	// err: i2c: send
const	int		M5psErrNotRdy=-104;		// err: not ready
const	int		M5psErrPlyBadMml=-105;	// err: ply: bad mml
const	int		M5psErrPlyNoMml=-106;	// err: ply: no mml to play
const	int		M5psErrBgmNoTask=-107;	// err: bgm: no subthread
const	int		M5psErrBgmTskAlv=-108;	// err: bgm: subthread alive
const	int		i2c_AXP192=0x34;		// i2c: AXP192(PMIC) slave address
const	int		i2c_BM8563=0x51;		// i2c: BM8563(RTC) slave address
const	int		i2c_MPU6886=0x68;		// i2c: MPU6886(IMU) slave address
const	int		SJIS_0CTRL=0;			// sjs: control code
const	int		SJIS_1BYTE=1;			// sjs: 1 byte code
const	int		SJIS_2BYTE=2;			// sjs: 2 byte code
const	int		UTF8_0CTRL=0;			// utf: control code
const	int		UTF8_1BYTE=1;			// utf: 1 byte code
const	int		UTF8_2BYTE=2;			// utf: 2 byte code
const	int		UTF8_3BYTE=3;			// utf: 3 byte code
const	int		UTF8_4BYTE=4;			// utf: 4 byte code
const	int		axp_PWR_LONG=0x01;		// axp: power switch long press
const	int		axp_PWR_SHORT=0x02;		// axp: power switch short press
const	int		axp_REG_PWR_STAT=0x00;	// axp: reg: power supply status
const	int		kpr_OPT_SJIS=0;			// kpr: S-JIS code
const	int		kpr_OPT_UTF8=1;			// kpr: UTF-8 code
const	int		tft_FNT_SIZ=14;			// tft: font size [dot]
const	int		tft_FNT_MGN=1;			// tft: font margin [dot]
const	int		tft_CHR_SIZ=tft_FNT_SIZ + tft_FNT_MGN;	// tft: char. size [dot]
const	int		tft_MAX_BTMP=sq (tft_CHR_SIZ);	// tft: bitmap size [dot**2]
const	int		tft_MAX_KU_TEN=94;		// tft: maximum ku & ten code
const	int		tft_MAX_KUTEN=sq (tft_MAX_KU_TEN);	// tft: maximum kuten code
const	int		rtc_REG_CNT=0x02;		// rtc: reg: counter register top
const	int		rtc_SIZ_CNT=7;			// rtc: reg: counter register size
const	int		rtc_VL_SEC=0;			// rtc: rtc_reg: VL, second (BCD)
const	int		rtc_MINU=1;				// rtc: rtc_reg: minute (BCD)
const	int		rtc_HOUR=2;				// rtc: rtc_reg: hour (BCD)
const	int		rtc_DAY=3;				// rtc: rtc_reg: day (BCD)
const	int		rtc_WEEK=4;				// rtc: rtc_reg: day of week (0~6)
const	int		rtc_C_MON=5;			// rtc: rtc_reg: C, month (BCD)
const	int		rtc_YEAR=6;				// rtc: rtc_reg: year (BCD)
const	int		rtc_MSK_VL=0x80;		// rtc: rtc_reg: VL mask
const	int		rtc_MSK_SEC=0x7f;		// rtc: rtc_reg: second mask
const	int		rtc_MSK_MINU=0x7f;		// rtc: rtc_reg: minute mask
const	int		rtc_MSK_HOUR=0x3f;		// rtc: rtc_reg: hour mask
const	int		rtc_MSK_DAY=0x3f;		// rtc: rtc_reg: day mask
const	int		rtc_MSK_WEEK=0x07;		// rtc: rtc_reg: week mask
const	int		rtc_MSK_C=0x80;			// rtc: rtc_reg: C mask
const	int		rtc_MSK_MON=0x1f;		// rtc: rtc_reg: month mask
const	int		rtc_MSK_YEAR=0xff;		// rtc: rtc_reg: year mask
const	int		bgm_OPT_DEFAULT=0x00;	// bgm: opt: default option
const	int		bgm_OPT_BLOCK=0x01;		// bgm: opt: blocking play MML
const	int		bgm_OPT_WAIT=0x02;		// bgm: opt: wait for the end of playing
const	float	bat_LOW0=3.3;			// axp: 0% voltage [V]
const	float	bat_HI100=4.1;			// axp: 100% voltage [V]

extern	const	char	*rtc_week[];	// rtc: day of week string
extern	const	uint8	icon_bat[];		// bat: battery icon
extern	const	uint8	icon_acp[];		// bat: power plug icon
extern	const	uint8	unsj_lkup_tab[];	// u2s: look up table
extern	const	uint16	unsj_hash_tab[];	// u2s: hash table
extern	const	uint16	tft_prn_font_kz[];	// tft: kanji font 0~(84*94 - 1)


extern	volatile	const	char	*bgm_mml;	// bgm: pointer to MML
extern	volatile	bool	bgm_rdy;	// bgm: ready / not
extern	volatile	int		bgm_err;	// bgm: error code
extern	int		tft_fwid;				// tft: screen size
extern	int		tft_fhei;
extern	int		tft_prn_lx;				// tft: cursor position
extern	int		tft_prn_ly;
extern	int		tft_prn_sx;				// tft: character size
extern	int		tft_prn_sy;
extern	int		ply_key_ofst;			// ply: offset for key
extern	uint16	tft_prn_fg;				// tft: foreground color (pixel)
extern	uint16	tft_prn_bg;				// tft: background color (pixel)
extern	uint16	ply_msec_zen;			// ply: length of a whole note [ms]
extern	uint8	rtc_reg[rtc_SIZ_CNT];	// rtc: counter register buffer
extern	uint16	tft_bitmap[tft_MAX_BTMP];	// tft: bit map buffer


// m5cp_ps.cpp
char	*strnum3c (
	int		num,
	char	*str=NULL);

int		i2c1_read (
	int		slv_adr,
	int		cmnd,
	int		rcnt=1,
	uint8	*rbuf=NULL);

int		i2c1_write (
	int		slv_adr,
	int		cmnd,
	int		scnt,
	uint8	*sbuf);

int		i2c1_write (
	int		slv_adr,
	int		cmnd,
	uint8	sdat);

bool	axp_is_power_avail (void);

int		axp_get_bat_level (
	int		full_scale=100);

void	rtc_get_date_time (
	uint8	*rbuf=rtc_reg);

void	rtc_set_date_time (
	uint8	*wbuf=rtc_reg);

int		rtc_day_of_week (
	int		yr,
	int		mo=1,
	int		dy=1);

int		bcd2dec (
	int		bcd);

int		dec2bcd (
	int		dec);

void	tft_draw_icon (
	const	uint8	*icn,
	int		pix_tp=-1);

void	tft_draw_bat (void);

void	tft_draw_title (
	const	char	*ttl,
	int		opt=0);


// m5cp_jpn.cpp
bool	is_sjis1 (
	int		chr);

bool	is_sjis2 (
	int		chr);

int		is_sjis (
	const	char	*str);

int		sjis2kuten (
	int		cods);

int		is_utf8 (
	int		chr);

int		is_utf8 (
	const	char	*str);

int		utf82unic (
	const	char	*str);

int		unic2sjis (
	int		codu,
	int		onerr=0x8140);				// 0x8140=zenkaku space

char	*utf82sjis (
	char	*str,
	int		onerr=0x8140);				// 0x8140=zenkaku space

uint16	tft_rgb2pix (
	int		ir,
	int		ig,
	int		ib);

void	tft_locate (
	int		lx,
	int		ly);

void	tft_font_size (
	int		sx,
	int		sy=0);

void	tft_color (
	int		fg,
	int		bg=-1);

void	tft_clear (void);

void	tft_kprint (
	const	char	*str,
	int		opt=kpr_OPT_SJIS);


// m5cp_play_mml.cpp
int		play_mml (
	const	char	*mbuf,
	void	*htsk=NULL);

int		play_mml_init (void);

int		play_bgm (
	const	char	*mbuf,
	int		opt=bgm_OPT_DEFAULT);

int		play_bgm_init (void);


#endif	//	M5CP_PS
