/*
 * TinyFL - Simple AVR (Attiny13) PWM LED driver
 * MCU: Attiny13(a)
 * Last version: 20.10.2020 (0.5)
 * (c) madcatdev https://github.com/madcatdev
 *---------
 * ������� ���������� ���� CREE �� Attiny13a, ��� 4.7 KHz, N-ch/P-ch ����������, ������ ��� ��������
*/


#ifndef MAIN_H_
#define MAIN_H_

// Frequency definition for gcc. Do not forget to set proper fuses.
// ����������� ������� ��� �����������. �� ������ ��������� ������� �������. 
#define F_CPU 1200000UL  // Attiny13 1.2MHz / PWM 4.6 KHz / CKDIV8 = 0
//#define F_CPU 9600000UL  // Attiny13 9.6MHz / PWM 36.8 KHz / CKDIV8 = 1

#include <util/delay.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <stdbool.h> // bool in C99

#define nop __asm__ __volatile__ ("nop");
typedef uint8_t u8;
typedef uint16_t u16;

//-------------------------------------

// Driver settings 
// ��������� ��������

// MAX brightness (max = 255) 
// ����. �������
#define RATE_MAX 250 
// MIN brightness (min = 0)	
// ���. �������
#define RATE_MIN 0 
// Restore brightness level when wakin up 
// ���������� ������� ������� �� ����� ���
#define RATE_REMEMBER 
// Non-linear brightness mode (gamma-correction). RATE_STEP_DEF will be ignores and rate_step will be calculated on the go
// ���������� ����� ��������� ������� (�����-���������). RATE_STEP_DEF ����� �������������� � rate_step �������� �����������
#define RATE_NONLINEAR  
// Brightness adjustment step
// ��� ��������� �������
#define RATE_STEP_DEF 10 
// Length of brightness adjustment step (ms)
// ������������ ������ ���� ��������� ������� (��)
#define RATE_STEP_LEN 30 
// Default brightness (when battery connected, or when each wakeup, if RATE_REMEMBER is not defined)
// ������� �� ��������� (��� ������ ���������, ���� �� �������� RATE_REMEMBER - ��� ������ ���������)
#define RATE_DEFAULT RATE_MIN 
// Battery voltage check
// �������� ������ �������
#define BAT_CHECK 
// Battery voltage check interval (ms). Once per minute is optimal for me. Maximum is 65s (65535)
// �������� �������� ������ ������� (���������), ��. ��� � ������ ����������. 65� ����������� (65535)
//#define BAT_PERIOD 60000 
#define BAT_CHECK_PERIOD 1000 // For easy calibration | ��� ����������
// Battery warning voltage, 135 is about 3.0V
// ���������� �������������� � �������, 135 ~ 3.0 V
#define BAT_WARNING 195 
// Shutdown voltage, 115 is about 2.7V
// ���������� �������� � ������ �����, 115 ~ 2.7 V
#define BAT_SHUTDOWN 180 
// ADC raw units per one warning blink (when battery is low)
// ���-�� ������ ��� �� ���� ������� ��� ��������� ������
#define BAT_INFO_STEP 3 
// Button reading on PB1
// ������ ��������� ������ �� PB1
#define BTN_READ (!(PINB & _BV(1))) 
// Double click turns on LED_BACK 
// ������� ����, ��������� ���.����������
#define BTN_DBCLICK 
// Double click delay, ms
// �������� �����, ��
#define BTN_DBCLICK_DELAY 100 
// Double click length, ms
// ������������ �����, ��
#define BTN_DBCLICK_LEN 100 
// Time between button pressed and sleep mode, ms.
// ����� ������� ������ �� ����� � ����� ���������� ��������, ��. uint16
#define BTN_ONOFF_DELAY 250 
// Additional modes, beacon and strobe
// ��� ������, ���� � �����
#define AUXMODES 
// Delay beetween entering these modes, x*0.125s
// �������� �� ����� � ���.������, x*0.125c
#define AUXMODES_DELAY 10 
// Sleep after battery connected for the first time
// �������� ����� ������ ������� �� �������
#define STARTSLEEP 
// Indication when battery connected for the first time (wery useful sometimes)
// ��������� ������ ������� ��� ������ LED_BACK (������ ��� ��������)
#define STARTBLINKS 
// Turn-on delay, ms. For EMP immunity value between 5-10 is recomended 
// �������� ��� ���������, ��. ��� ������������ ������������� 5-10
#define STARTDELAY 5 

// Peripheral setup
// ��������� �����, ���� ����������� � ��������
//#define NCH // N-channel FET | N-��������� �������
#define PCH // P-channel FET | P-��������� �������)
#ifdef NCH
	#define LED_MAIN_on PORTB |= 1; // PB0
	#define LED_MAIN_off PORTB &= ~1;
	// 10 - ��������� 0 �� ������ OC0A ��� ���������� � A, ��������� 1 �� ������ OC0A ��� ��������� �������� (����������� �����)
	#define PWM_on TCCR0A |= _BV(COM0A1);
	#define PWM_off	TCCR0A &= ~_BV(COM0A1); // ��������� ��� �� PB0
#endif
#ifdef PCH
	#define LED_MAIN_on PORTB &= ~1;
	#define LED_MAIN_off PORTB |= 1;
	// 11 - ��������� 1 �� ������ OC0A ��� ���������� � A, ��������� 0 �� ������ OC0A ��� ��������� �������� (��������� �����)
	#define PWM_on TCCR0A |= _BV(COM0A1) | _BV(COM0A0);
	#define PWM_off	TCCR0A &= ~(_BV(COM0A1) | _BV(COM0A0));
#endif
#define LED_BACK_on PORTB |= _BV(2); // ��� LED2
#define LED_BACK_off  PORTB &= ~_BV(2); // ���� LED2
#define LED_BACK_inv PORTB ^= _BV(2); // ������������� LED2


int main(void);
static void setup(void);
static void wakeup(void);
void sleep(void);
static void beacon(void);
static void strobe(void);
void longpress(void);
void shortpress(void);
static u8 bat_getvoltage(void);
static void bat_check(void);

#endif /* MAIN_H_ */