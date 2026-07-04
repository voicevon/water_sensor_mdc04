#ifndef __CA51F003_H__
#define __CA51F003_H__

sfr P0              = 0x80;
sfr SP              = 0x81;
sfr DP0L            = 0x82;
sfr DP0H            = 0x83;
sfr DP1L            = 0x84;
sfr DP1H            = 0x85;
sfr PWCON           = 0x86;
sfr PCON            = 0x87;
sfr TCON            = 0x88;
sfr TMOD            = 0x89;
sfr TL0             = 0x8a;
sfr TL1             = 0x8b;
sfr TH0             = 0x8c;
sfr TH1             = 0x8d;
sfr IT1CON          = 0x8e;
sfr IT0CON          = 0x8f;

sfr P1              = 0x90;

sfr S1CON           = 0x9a;
sfr S1BUF           = 0x9b;
sfr S1RELL          = 0x9c;
sfr S1RELH          = 0x9d;


sfr P2              = 0xa0;
sfr S2CON           = 0xa1;
sfr S2BUF           = 0xa2;
sfr S2RELL          = 0xa3;
sfr S2RELH          = 0xa4;
sfr SPCON           = 0xa5;
sfr SPDAT           = 0xa6;
sfr SPSTA           = 0xa7;
sfr IE              = 0xa8;
sfr WDCON           = 0xaa;
sfr WDFLG           = 0xab;
sfr WDVTHL          = 0xac;
sfr WDVTHH          = 0xad;


sfr P3              = 0xb0;
sfr I2CCON          = 0xb1;
sfr I2CADR          = 0xb2;
sfr I2CADM          = 0xb3;
sfr I2CCCR          = 0xb4;
sfr I2CDAT          = 0xb5;
sfr I2CSTA          = 0xb6;
sfr I2CFLG          = 0xb7;
sfr IP              = 0xb8;
sfr ADCON           = 0xb9;
sfr ADCFGL          = 0xba;
sfr ADCFGH          = 0xbb;
sfr ADCDL           = 0xbc;
sfr ADCDH           = 0xbd;

sfr T2CON           = 0xc8;
sfr T2MOD           = 0xc9;
sfr T2CL            = 0xca;
sfr T2CH            = 0xcb;
sfr TL2             = 0xcc;
sfr TH2             = 0xcd;


sfr PSW             = 0xd0;
sfr PWMDIVH         = 0xd1;
sfr PWMDUTL         = 0xd2;
sfr PWMDUTH         = 0xd3;
sfr PWMAIF          = 0xd4;
sfr PWMBIF          = 0xd5;
sfr PWMCIF          = 0xd6;
sfr PWMDIF          = 0xd7;

sfr PWMEN           = 0xda;
sfr PWMUPD          = 0xdb;
sfr PWMCMAX         = 0xdc;
sfr PWMCON          = 0xdd;
sfr PWMCFG          = 0xde;
sfr PWMDIVL         = 0xdf;

sfr ACC             = 0xe0;

sfr MDUCON          = 0xe6;
sfr MDUDAT          = 0xe7;
sfr EXIE            = 0xe8;

sfr LVDCON          = 0xef;

sfr B               = 0xf0;

sfr INDEX           = 0xf7;
sfr EXIP            = 0xf8;
sfr EPIE            = 0xf9;
sfr EPIF            = 0xfa;
sfr EPCON           = 0xfb;
sfr IDLSTL          = 0xfc;
sfr IDLSTH          = 0xfd;
sfr STPSTL          = 0xfe;
sfr STPSTH          = 0xff;


/*  BIT Register  */
/*	PSW	*/
sbit CY   		= 0xD7;
sbit AC   		= 0xD6;
sbit F0   		= 0xD5;
sbit RS1  		= 0xD4;
sbit RS0  		= 0xD3;
sbit OV   		= 0xD2;
sbit DPS   		= 0xD1;
sbit P    		= 0xD0;
		  		
/*	TCON	*/
sbit TF1  		= 0x8F;
sbit TR1  		= 0x8E;
sbit TF0  		= 0x8D;
sbit TR0  		= 0x8C;
sbit IE1  		= 0x8B;
sbit IT1  		= 0x8A;
sbit IE0  		= 0x89;
sbit IT0  		= 0x88;
/*	IE	*/
sbit EA   		= 0xAF;
sbit ES1  		= 0xAE;
sbit ET2  		= 0xAD;
sbit ES0	   	= 0xAC;
sbit ET1  		= 0xAB;
sbit EX1  		= 0xAA;
sbit ET0  		= 0xA9;
sbit EX0  		= 0xA8;

/*	IP	*/ 
sbit PS1  		= 0xBE;
sbit PT2  		= 0xBD;
sbit PS0   		= 0xBC;
sbit PT1  		= 0xBB;
sbit PX1  		= 0xBA;
sbit PT0  		= 0xB9;
sbit PX0  		= 0xB8;
/*	T2CON	*/
sbit TF2   		= 0xCF;
sbit TR2  		= 0xCE;
sbit T2R1  		= 0xCD;
sbit T2R0  		= 0xCC;
sbit T2IE     = 0xCB;
sbit UCKS  		= 0xCA;
sbit T2P1  		= 0xC9;
sbit T2P0  		= 0xC8;
/*	EXIE	*/
sbit INT9EN   		= 0xEF;
sbit INT8EN  		= 0xEE;
sbit INT7EN  		= 0xED;
sbit INT6EN	   		= 0xEC;
sbit INT5EN  		= 0xEB;
sbit INT4EN  		= 0xEA;
sbit INT3EN  		= 0xE9;
sbit INT2EN  		= 0xE8;
/*	P0	*/
sbit P00		= P0^0;
sbit P01		= P0^1;
sbit P02		= P0^2;
sbit P03		= P0^3;
sbit P04		= P0^4;
sbit P05		= P0^5;
sbit P06		= P0^6;
sbit P07		= P0^7;
/*	P1	*/     	
sbit P10		= P1^0;
sbit P11		= P1^1;
sbit P12		= P1^2;
sbit P13		= P1^3;
sbit P14		= P1^4;
sbit P15		= P1^5;
sbit P16		= P1^6;
sbit P17		= P1^7;
/*	P2	*/        	
sbit P20		= P2^0;


/*	P3	*/
sbit P30		= P3^0;
	
#endif  //__CA51F003_H__
