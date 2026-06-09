//#include <assert.h>  
#include "bsp_usart1.h"


uint8_t *pDataByte;

 /**
  * @brief  USART1 GPIO configure,work mode configure¡£115200 8-N-1
  * @param  N/A
  * @retval N/A
  */
void USART1_Config(void)
{
		GPIO_InitTypeDef GPIO_InitStructure;
		USART_InitTypeDef USART_InitStructure;
		
		/* config USART1 clock */
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	  //GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 ;	
    //GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;       
    //GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    //GPIO_Init(GPIOA, &GPIO_InitStructure);  //initial PC port
    //GPIO_SetBits(GPIOA, GPIO_Pin_8 );	 // 485 set to TX mode
		
		/* USART1 GPIO config */
		/* Configure USART1 Tx (PA.09) as alternate function push-pull */
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_Init(GPIOA, &GPIO_InitStructure);
	
		/* Configure USART1 Rx (PA.10) as input floating */
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
		GPIO_Init(GPIOA, &GPIO_InitStructure);
			
		/* USART1 mode config */
		//USART_InitStructure.USART_BaudRate = 9600;
		USART_InitStructure.USART_BaudRate = 115200;
		USART_InitStructure.USART_WordLength = USART_WordLength_8b;
		USART_InitStructure.USART_StopBits = USART_StopBits_1;
		USART_InitStructure.USART_Parity = USART_Parity_No ;
		USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
		USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
		USART_Init(USART1, &USART_InitStructure); 
		
		USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
		
		USART_Cmd(USART1, ENABLE);
	
}


///redirect printf to USART1
int fputc(int ch, FILE *f)
{
		while(USART_GetFlagStatus(USART1, USART_FLAG_TC)==RESET);
	  /* send one byte to USART1 */
		USART_SendData(USART1, (uint8_t) ch);
		
		/* wait for send finish */
		while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);		
	
		return (ch);
}

///redirect scanf to USART1
int fgetc(FILE *f)
{
		/* wait USART1 input data */
		while (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == RESET);

		return (int)USART_ReceiveData(USART1);
}






/*********************************************END OF FILE**********************/
