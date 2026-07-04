//CKCON寄存器定义
#define ILCKE		(1<<7)
#define IHCKE		(1<<6)
#define XHCS		(1<<4)
#define XLCKE		(1<<3)
#define XLSTA		(1<<2)
#define XHCKE		(1<<1)
#define XHSTA		(1<<0)

//CKSEL寄存器定义
#define RTCKS(N)		(N<<7)

#define CKSEL_IRCH	0	
#define CKSEL_IRCL	1
#define CKSEL_XOSCH	2


void Sys_Clk_Set_IRCH(void);
void Sys_Clk_Set_IRCL(void);
void Sys_Clk_Set_XOSCH(void);

