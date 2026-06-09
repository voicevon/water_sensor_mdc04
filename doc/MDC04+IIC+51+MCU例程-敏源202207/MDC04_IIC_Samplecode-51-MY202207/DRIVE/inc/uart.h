
#define UART1_TX_BUF_SIZE		20
#define UART1_RX_BUF_SIZE		100

#define UART2_TX_BUF_SIZE		100
#define UART2_RX_BUF_SIZE		100

typedef struct
{
	unsigned char head;
	unsigned char tail;
}T_Buf_Info;

#ifdef _UART_C_
#ifdef UART1_EN
	T_Buf_Info xdata	uart1_send;
	T_Buf_Info xdata	uart1_rev;
	unsigned char xdata uart1_tx_buf[UART1_TX_BUF_SIZE];
	unsigned char xdata uart1_rx_buf[UART1_RX_BUF_SIZE];
	bit uart1_tx_flag;
#endif
#ifdef UART2_EN
	T_Buf_Info xdata	uart2_send;
	T_Buf_Info xdata	uart2_rev;
	unsigned char xdata uart2_tx_buf[UART2_TX_BUF_SIZE];
	unsigned char xdata uart2_rx_buf[UART2_RX_BUF_SIZE];
	bit uart2_tx_flag;
#endif
#else
#ifdef UART1_EN
	extern T_Buf_Info xdata	uart1_send;
	extern T_Buf_Info xdata	uart1_rev;
	extern unsigned char xdata uart1_tx_buf[UART1_TX_BUF_SIZE];
	extern unsigned char xdata uart1_rx_buf[UART1_RX_BUF_SIZE];
	extern bit uart1_tx_flag;	
#endif
#ifdef UART2_EN
	extern T_Buf_Info xdata	uart2_send;
	extern T_Buf_Info xdata	uart2_rev;
	extern unsigned char xdata uart2_tx_buf[UART2_TX_BUF_SIZE];
	extern unsigned char xdata uart2_rx_buf[UART2_RX_BUF_SIZE];
	extern bit f_uart2_tx;	
#endif
#endif
#ifndef UART1_EN
	  #define Uart1_PutChar(n)
#endif
#ifndef UART2_EN
	  #define Uart2_PutChar(n)
#endif

#ifdef UART1_EN
void Uart1_PutChar(unsigned char bdat);
void Uart1_Initial(unsigned long int baudrate);
#endif

#ifdef UART2_EN
void Uart2_PutChar(unsigned char bdat);
void Uart2_Initial(unsigned long int baudrate);
#endif


#ifdef PRINT_EN
void uart_printf(char *fmt,...);
#endif