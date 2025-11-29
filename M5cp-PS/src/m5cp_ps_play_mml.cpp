//
//	M5StickC Plus postscript library
//		play MML function module
//		(c) 2019,2023	1YEN Toru
//
// ================================
//	2021/12/11	ver.1.08
//		generate 440Hz using F_CPU
//
//	2020/10/24	ver.1.06
//		update mml syntax
//		add: "tmpo,<1/n_note>,<counts_per_minute>" command
//		code brushed up
//		bug fixed
//
//	2020/06/06	ver.1.04
//		corresponding to TC2 (#define USE_TC2)
//			otherwise use TC1
//		TC2 frequency range:
//			2<=uint8 ((F_CPU/(256*2*2))/frq + (0.5 - 1.))<=255
//			F_CPU/(256*2*2)/256<=frq<=F_CPU/(256*2*2)/3
//			F_CPU=16MHz: 62Hz<=frq<=5.2kHz (B1~E8)
//		add: "bgm" command
//		play_mml(): ply_ptr=NULL, if (*ply_ptr)=='\0'
//
//	2019/07/20	ver.1.02
//		corresponding to case insensitive mml
//		fix: frequency -> period calculation
//		corresponding to non-blocking play
//
//	2019/05/18	ver.1.00
//


#include	<M5StickCPlus.h>
#include	<m5cp_ps.h>


static	const	char	*ply_ptr;		// ply: mml pointer
static	TaskHandle_t	bgm_htsk;		// bgm: task handle
static	int		(*ply_callbk) (void);	// ply: pointer to call back function


volatile	const	char	*bgm_mml;
volatile	bool	bgm_rdy;
volatile	int		bgm_err;
int		ply_key_ofst;
uint16	ply_msec_zen;


int		play_mml (
const	char	*mbuf,
void	*htsk)
{
	// play mml mbuf, blocking edition
	// return: M5psErrNo / M5psErrPlyNoMml / M5psErrPlyBadMml / M5psErrBgmTskAlv
	const	char	*str_c;
	int		rtncod;
	int		dd;							// scale number
	int		hlf;						// # / b
	int		oct;						// octave
	uint32	msec_start;					// note's start time [ms]
	uint32	msec_len;					// (note's length [ms])*2 + connect
	float	frq;						// frequency
	const	char	*scl_c="RA BC D EF G";

	// error check
	if (mbuf==NULL || *mbuf=='\0')
		return (M5psErrPlyNoMml);
	if (htsk!=bgm_htsk)
		return (M5psErrBgmTskAlv);

	// start play
	msec_start=millis ();
	rtncod=M5psErrNo;
	for (ply_ptr=mbuf; ply_ptr!=NULL && (*ply_ptr)!='\0'; ply_ptr++)
	{
		// call back
		if (ply_callbk!=NULL)
			if ((*ply_callbk) ())
				break;
		// parse
		if ((*ply_ptr)==' ' || (*ply_ptr)=='\t' || (*ply_ptr)==',')
			continue;
		if ((*ply_ptr)=='#' || (*ply_ptr)=='/' || (*ply_ptr)==';')
		{
			// comment, ignore
			ply_ptr=NULL;
			break;
		}
		if (toupper (*ply_ptr)=='T')
		{
			// "T<whole_note_ms>" command
			ply_msec_zen=atoi (ply_ptr + 1);
			ply_msec_zen=(ply_msec_zen<128)? 128:
				(ply_msec_zen>10000)? 10000: ply_msec_zen;
			// find next, continue
			ply_ptr=strchr (ply_ptr,',');
			if (ply_ptr==NULL)
				break;
			continue;
		}
		else if (toupper (*ply_ptr)=='K')
		{
			// "K<offset_for_key>" command
			ply_key_ofst=atoi (ply_ptr + 1);
			ply_key_ofst=
				(ply_key_ofst<-4*12)? -4*12:
				(ply_key_ofst>4*12)? 4*12: ply_key_ofst;
			// find next, continue
			ply_ptr=strchr (ply_ptr,',');
			if (ply_ptr==NULL)
				break;
			continue;
		}

		// analyze mml
		// scale
		str_c=strchr (scl_c,toupper (*ply_ptr));
		if (str_c==NULL)
		{
			// ERROR:
			rtncod=M5psErrPlyBadMml;
			ply_ptr=NULL;
			break;
		}
		dd=str_c - scl_c + 1;
		if (dd==1)
			dd=-10000;					// rest
		else
			dd=dd - 2;
		// scale (# / b detection)
		hlf=0;
		ply_ptr++;
		if ((*ply_ptr)=='#')
		{
			// #
			hlf=1;
			ply_ptr++;
		}
		else if ((*ply_ptr)=='b')
		{
			// b
			hlf=-1;
			ply_ptr++;
		}
		// octave
		if ('0'<=(*ply_ptr) && (*ply_ptr)<='9')
		{
			oct=(*ply_ptr) - '0';
		}
		else
		{
			// ERROR:
			rtncod=M5psErrPlyBadMml;
			ply_ptr=NULL;
			break;
		}
		if (dd>2)
			dd=dd - 12;
		dd=dd + (oct - 4)*12;
		// scale (# / b)
		dd=dd + hlf + ply_key_ofst;
		// length
		ply_ptr++;
		msec_len=atoi (ply_ptr);
		if (msec_len>0)
			msec_len=ply_msec_zen/msec_len;
		// length (dotted / connect to the next)
		//	msec_len=<length>*2 + <connect_to_the_next>
		str_c=strchr (ply_ptr,',');
		if (str_c==NULL)
		{
			str_c=&ply_ptr[strlen (ply_ptr) - 1];
			ply_ptr=str_c;
		}
		else
		{
			ply_ptr=str_c;
			str_c--;
		}
		if ((*str_c)=='.')
			msec_len=((msec_len*3/2)<<1);
		else if ((*str_c)=='_' && msec_len>0)
			msec_len=(msec_len<<1) + 1;
		else
			msec_len <<= 1;
		// frequency
		if (dd<-1000)
			frq=0.;						// rest
		else
			frq=440.*pow (2., dd/12.);
		// counter reset
		if (frq==0. && msec_len==0)
		{
//// not implemented (not need for single channel play_mml())
//			play_mml_reset ();
			continue;
		}
		// output
		msec_len=(msec_len==0)? 1: msec_len;
		if (frq<20. || 20e3<frq)
		{
			// rest
			M5.Beep.mute ();
			while (millis () - msec_start<=msec_len/2)
				vTaskDelay (1/portTICK_PERIOD_MS);
		}
		else
		{
			// note
			M5.Beep.tone (frq);
			while (millis () - msec_start<=msec_len/2 - msec_len/16)
				vTaskDelay (1/portTICK_PERIOD_MS);
			if (!(msec_len&0x01))
				M5.Beep.mute ();
			while (millis () - msec_start<=msec_len/2)
				vTaskDelay (1/portTICK_PERIOD_MS);
			M5.Beep.mute ();
		}
		// next note
		msec_start += msec_len/2;
	}

	return (M5psErrNo);
}

int		play_mml_init (void)
{
	// initialize play_mml() function
	// return: M5psErrNo

	ply_msec_zen=2000;					// 1/4 note * 120 in 1 minute
	ply_key_ofst=0;
	ply_ptr=NULL;
	ply_callbk=NULL;

	return (M5psErrNo);
}


int		bgm_callbk (void)
{
	// call back function for play_mml()
	// return: false (continue playing) / true (break playing)

	if (bgm_mml==NULL)
		return (true);

	return (false);
}

void	bgm_play_mml (
void	*argv)
{
	// BGM(Back Ground Music) play MML subthread function
	//	argv: dummy, not used
	//	bgm_rdy: indicate play_bgm() is ready(true) / busy(false)
	//	bgm_mml: pointer to play MML string
	const	char	*bgm_cur;

	// initialize
	ply_callbk=bgm_callbk;
	bgm_rdy=true;

	// forever
	for (;;)
	{
		// check bgm_mml
		bgm_cur=(const char *)bgm_mml;
		if (bgm_cur!=NULL)
		{

			// play MML
			bgm_rdy=false;
			bgm_err=play_mml (bgm_cur, bgm_htsk);

			// finished
			bgm_mml=NULL;
			bgm_rdy=true;
		}
		else
		{
			// no MML
			bgm_rdy=true;
		}

		// wait
		vTaskDelay (1/portTICK_PERIOD_MS);
	}
}

int		play_bgm (
const	char	*mbuf,
int		opt)
{
	// play_bgm ()
	//	opt:
	//		bgm_OPT_DEFAULT: non-blocking play MML
	//		bgm_OPT_BLOCK: blocking play MML
	//		bgm_OPT_WAIT: wait for the end of playing
	// return: M5psErrNo / M5psErrBgmNoTask / bgm_err=play_mml()
	int		rtncod;

	// check thread
	if (bgm_htsk==NULL)
		return (M5psErrBgmNoTask);

	// playing MML?
	if (bgm_rdy && bgm_mml!=NULL)
	{
		// PLAY_REQ: polling until bgm_rdy==false
		while (bgm_rdy)
			vTaskDelay (1/portTICK_PERIOD_MS);
		// PLAY_REQ -> PLAY
	}
	// IDLE or PLAY or STOP_REQ:
	if (!(opt&bgm_OPT_WAIT))
	{
		// cancel playing MML
		bgm_mml=NULL;
		// PLAY -> STOP_REQ
	}
	// IDLE or STOP_REQ: polling until bgm_rdy==true
	while (!bgm_rdy)
		vTaskDelay (1/portTICK_PERIOD_MS);
	// STOP_REQ -> IDLE

	// play MML
	bgm_mml=mbuf;
	// IDLE -> PLAY_REQ

	// PLAY_REQ: polling during PLAY_REQ
	while (bgm_rdy && bgm_mml!=NULL)
		vTaskDelay (1/portTICK_PERIOD_MS);
	// PLAY_REQ -> PLAY (or STOP_REQ or IDLE)

	// option
	rtncod=M5psErrNo;
	if (opt&bgm_OPT_BLOCK)
	{
		// poling until bgm_rdy==true
		while (!bgm_rdy)
			vTaskDelay (1/portTICK_PERIOD_MS);
		// PLAY -> IDLE

		rtncod=bgm_err;
	}

	return (rtncod);
}

int		play_bgm_init (void)
{
	// initialize play_bgm() function
	// return: M5psErrNo / M5psErrBgmTskAlv

	// error check
	if (bgm_htsk!=NULL)
	{
		// ERROR: already initialized
		return (M5psErrBgmTskAlv);
	}

	// initialize
	play_mml_init ();
	bgm_rdy=false;
	bgm_mml=NULL;
	bgm_err=M5psErrNo;

	// subthread
	xTaskCreate (
		bgm_play_mml,					// thread function
		"bgm_play_mml",					// thread name
		2*1024,							// stack size
		NULL,							// parameter
		uxTaskPriorityGet (NULL),		// priority
		&bgm_htsk);						// task handle
	if (bgm_htsk==NULL)
	{
		// ERROR:
		Serial.println ("ERR: could not create task");
		tft_locate (8,8);
		tft_kprint ("ERR: could not create task");
		M5.Beep.beep ();
		delay (500);
		M5.Beep.mute ();
		for (;;)
			;
	}

	// polling until bgm_rdy==true
	while (!bgm_rdy)
		vTaskDelay (1/portTICK_PERIOD_MS);

	return (M5psErrNo);
}

