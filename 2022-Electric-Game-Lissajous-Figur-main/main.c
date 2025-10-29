/****************************************************/
// MSP432P401R
// ����Keil�����չ��� (�ѽ�ti�̼�����������)
// Keil���������ÿ���FPU
// Bilibili��m-RNA
// E-mail:m-RNA@qq.com
// ��������:2021/9/28
/****************************************************/

#include "sysinit.h"
#include "usart.h"
#include "delay.h"
#include "led.h"
#include "tim32.h"
#include "adc.h"
#include "key4x4.h"
#include "oled.h"

#include "LCD_eUSCI_B0_SPI.h" // LCD �ײ�SPI��������
#include "LCD_Display_API.h"  // LCD ��ʾ�ӿں���
#include "LCD_Font.h"		  // ASCII ���ֿ����, �Լ��Զ����ַ������

uint16_t tft_clean_cnt = 0;
uint8_t key_value = 0;
uint8_t waveform = 0;  // ����:0���Ҳ�,1���ǲ�
uint8_t frequency = 1; // Ƶ�ʱ���:1~5
uint8_t iic_sent = 50; 
uint8_t iic_cnt = 0;
char iic_num[5];

int main(void)
{
	SysInit();		   // ��3�� ʱ������
	uart_init(115200); // ��7�� ��������
	delay_init();	   // ��4�� �δ���ʱ
	KEY4x4_Init();
	// 4052: B->P2.3, A->P2.4
	// 4053: C->P2.5, B->P2.6, A->P2.7
	P2DIR |= BIT3 | BIT4 | BIT5 | BIT6 | BIT7; // ģ�⿪��ѡ��

	// ������ N_or_1->P3.0, D2->P3.5, D1->P3.6, D0->P3.7
	P3DIR |= BIT0 | BIT5 | BIT6 | BIT7; // ��������λ

	uint16_t ChNum = 0;	 // ��ʱ����
	uint16_t yStart = 0; // ��ʱ����

	// ------------------------------------------------
	// WDT��ʱ���ڸ�λʱĬ�Ͽ��������������΢��������λ
	WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD; // ֹͣ WDT ��ʱ������

	// ------------------------------------------------
	// ��˸ LED ��ָʾ�����Ƿ�������
	P1->DIR |= BIT0;			  // P1.0 ����Ϊ���
	P1->OUT |= BIT0;			  // P1.0 LED ��
	MAP_Interrupt_enableMaster(); // �������ж�
	// ------------------------------------------------
	LCD_SPI_Init(); // LCD ���ż� SPI ��ʼ��
	LCD_TFT_Init(); // ��ʼ�� 320x240 TFT LCD, ��λLCD,��ȫ������ɫ

	// ====================================================

	// ��λ�
	yStart = 0;
	for (ChNum = 0; ChNum <= 2; ChNum++)
	{
		LCD_Show_ChFont1616(145, yStart, ChNum, Red, White);
		yStart += 16;
	}
	LCD_TFT_ShowString(yStart, 145, ":", FONT1608, Red, White);

	// ���Σ�
	yStart = 0;
	for (ChNum = 3; ChNum <= 4; ChNum++)
	{
		LCD_Show_ChFont1616(167, yStart, ChNum, Red, White);
		yStart += 16;
	}
	LCD_TFT_ShowString(yStart, 167, ":", FONT1608, Red, White);

	yStart = 48;
	for (ChNum = 14; ChNum <= 16; ChNum++)
	{
		LCD_Show_ChFont1616(167, yStart, ChNum, Red, White);
		yStart += 16;
	}

	// Ƶ�ʱ�����
	yStart = 0;
	for (ChNum = 5; ChNum <= 8; ChNum++)
	{
		LCD_Show_ChFont1616(189, yStart, ChNum, Red, White);
		yStart += 16;
	}
	LCD_TFT_ShowString(yStart, 189, ":", FONT1608, Red, White);

	LCD_TFT_ShowString(80, 189, "1", FONT1608, Red, White);
	yStart = 96;
	for (ChNum = 20; ChNum <= 21; ChNum++)
	{
		LCD_Show_ChFont1616(189, yStart, ChNum, Red, White);
		yStart += 16;
	}

	// ��ѹ���ֵ��
	yStart = 0;
	for (ChNum = 9; ChNum <= 13; ChNum++)
	{
		LCD_Show_ChFont1616(211, yStart, ChNum, Red, White);
		yStart += 16;
	}
	LCD_TFT_ShowString(yStart, 211, ":", FONT1608, Red, White);

	LCD_TFT_ShowString(96, 211, "50", FONT1608, Red, White);
//	LCD_TFT_ShowString(112, 211, "V", FONT1608, Red, White);

	// ��ʼ״̬
	P2OUT |= BIT5 | BIT6 | BIT7;
	P2OUT &= ~(BIT3 | BIT4);

	P3OUT &= ~(BIT5 | BIT6 | BIT7 | BIT0);

	ADC_Config();
	while (1) // ��ѭ��
	{
		key_value = KEY4x4_Scan(0);
		switch (key_value)
		{
		case 0:
			break;
		case 1:
			// ����
			LCD_TFT_fillrect(167, 183, 48, 96, White);
			yStart = 48;
			for (ChNum = 14; ChNum <= 16; ChNum++)
			{
				LCD_Show_ChFont1616(167, yStart, ChNum, Red, White);
				yStart += 16;
			}
			// ����
			waveform = 0;
			break;
		case 2:
			// ����
			LCD_TFT_fillrect(167, 183, 48, 96, White);
			yStart = 48;
			for (ChNum = 17; ChNum <= 19; ChNum++)
			{
				LCD_Show_ChFont1616(167, yStart, ChNum, Red, White);
				yStart += 16;
			}
			// ����
			waveform = 1;
			break;
		case 3:
			// ����
			LCD_TFT_fillrect(80, 189, 96, 197, White);
			LCD_TFT_ShowString(80, 189, "1", FONT1608, Red, White);
			// ����
			frequency = 1;
			break;
		case 5:
			// ����
			LCD_TFT_fillrect(80, 189, 96, 197, White);
			LCD_TFT_ShowString(80, 189, "2", FONT1608, Red, White);
			// ����
			frequency = 2;
			break;
		case 6:
			// ����
			LCD_TFT_fillrect(80, 189, 96, 197, White);
			LCD_TFT_ShowString(80, 189, "3", FONT1608, Red, White);
			// ����
			frequency = 3;
			break;
		case 7:
			// ����
			LCD_TFT_fillrect(80, 189, 96, 197, White);
			LCD_TFT_ShowString(80, 189, "4", FONT1608, Red, White);
			// ����
			frequency = 4;
			break;
		case 9:
			// ����
			LCD_TFT_fillrect(80, 189, 96, 197, White);
			LCD_TFT_ShowString(80, 189, "5", FONT1608, Red, White);
			// ����
			frequency = 5;
			break;
		case 10:
			// ����
//			LCD_TFT_fillrect(96, 211, 112, 227, White);
//			LCD_TFT_ShowString(96, 211, "1", FONT1608, Red, White);
			// ����
			iic_sent--;
			OLED_WR_Byte(iic_sent,0);
			LCD_TFT_fillrect(96, 211, 112+32, 227, White);
			iic_cnt = IntToStr(iic_sent , iic_num);
			LCD_TFT_ShowString(96, 211, iic_num, FONT1608, Red, White);
			break;
		case 11:
			// ����
//			LCD_TFT_fillrect(96, 211, 112, 227, White);
//			LCD_TFT_ShowString(96, 211, "2", FONT1608, Red, White);
			// ����
			iic_sent++;
			OLED_WR_Byte(iic_sent,0);
			LCD_TFT_fillrect(96, 211, 112+32, 227, White);
			iic_cnt = IntToStr(iic_sent , iic_num);
			LCD_TFT_ShowString(96, 211, iic_num, FONT1608, Red, White);
			break;
		case 13:
			// ����
//			LCD_TFT_fillrect(96, 211, 112, 227, White);
//			LCD_TFT_ShowString(96, 211, "3", FONT1608, Red, White);
			// ����

			break;
		default:
			break;
		}
		if (waveform == 0) // ���Ҳ�
			switch (frequency)
			{
			case 1:
				P2OUT |= BIT5 | BIT6 | BIT7;
				P2OUT &= ~(BIT3 | BIT4);

				P3OUT &= ~(BIT5 | BIT6 | BIT7 | BIT0);
				break;
			case 2:
				P2OUT |= BIT4 | BIT5 | BIT6 | BIT7;
				P2OUT &= ~(BIT3);

				P3OUT |= BIT5 | BIT6 | BIT0;
				P3OUT &= ~(BIT7);
				break;
			case 3:
				P2OUT |= BIT3 | BIT5 | BIT6 | BIT7;
				P2OUT &= ~(BIT4);

				P3OUT |= BIT5 | BIT7 | BIT0;
				P3OUT &= ~(BIT6);
				break;
			case 4:
				P2OUT |= BIT3 | BIT4 | BIT5 | BIT6 | BIT7;
				// P2OUT &= ~BIT6;

				P3OUT |= BIT5 | BIT0;
				P3OUT &= ~(BIT6 | BIT7);
				break;
			case 5:
				P2OUT |= BIT5;
				P2OUT &= ~(BIT3 | BIT4 | BIT6 | BIT7);

				P3OUT |= BIT6 | BIT7 | BIT0;
				P3OUT &= ~(BIT5);
				break;
			default:
				break;
			}
		else
			switch (frequency)
			{
			case 1:
				P2OUT |= BIT7;
				P2OUT &= ~(BIT3 | BIT4 | BIT5 | BIT6);

				P3OUT &= ~(BIT5 | BIT6 | BIT7 | BIT0);
				break;
			case 2:
				P2OUT |= BIT4 | BIT7;
				P2OUT &= ~(BIT3 | BIT5 | BIT6);

				P3OUT |= BIT5 | BIT6 | BIT0;
				P3OUT &= ~(BIT7);
				break;
			case 3:
				P2OUT |= BIT3 | BIT7;
				P2OUT &= ~(BIT4 | BIT5 | BIT6);

				P3OUT |= BIT5 | BIT7 | BIT0;
				P3OUT &= ~(BIT6);
				break;
			case 4:
				P2OUT |= BIT3 | BIT4 | BIT7;
				P2OUT &= ~(BIT5 | BIT6);

				P3OUT |= BIT5 | BIT0;
				P3OUT &= ~(BIT6 | BIT7);
				break;
			case 5:
				P2OUT |= BIT7;
				P2OUT &= ~(BIT3 | BIT4 | BIT5 | BIT6);

				P3OUT |= BIT6 | BIT7 | BIT0;
				P3OUT &= ~(BIT5);
				break;
			default:
				break;
			}

		//printf("%d\r\t", key_value);
		if (ADC_Flag == 0)
		{
			tft_clean_cnt++;
			if (tft_clean_cnt > 10)
			{
				LCD_TFT_fillrect(10, 10, 10 + 128, 10 + 128, White);
				tft_clean_cnt = 0;
			}

			for (int j = 0; j < 1024; j++){
				LCD_TFT_pixel(10 + X[j] / 128, 10 + 128 - Y[j] / 128, Blue);
				//printf("X=%d,Y=%d\r\t", X[j]/128,Y[j]/128);
			}
			//printf("======================================\r\t");
			ADC_Flag = 1;
			ii = 0;
		}
		//				LCD_TFT_pixel(10 + resultsBuffer[0]/128 ,10 + 128 - resultsBuffer[1]/128, Blue);
		P1->OUT ^= BIT0; // ����ָʾ��(��˸)
						 //        DelayMs(500);
	}
}
