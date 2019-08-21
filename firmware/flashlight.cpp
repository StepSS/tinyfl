/*
 * flashlight.cpp
 *
 * Created: 12.09.2018 14:53:32
*/

#include "flashlight.h"

volatile u8 rate = RATE_MIN; // ������� ��������, 0 - ��������, 255 - ������������ (���)
volatile u8 rate_dir = 0; // ����������� ��������� ������� ��������, 1 - ��������� ���, 0 - ������� ���
u8 rate_step_array[12] = {10,25,50,60,75,80,90,120,130,140,150,RATE_MAX}; // ������ ����� ������ �����-���������, � ����� ����������� maxrate
bool led_state = false; // ��������� ����������

int main(void)	{
	setup();	
	wakeup();
	
	u16 bat_time = 0; // ������� ������� ��� �������� ������, ��� >=bat_time ��������� ��������
	
	while(1)    {
		
		if (BTN_READ){ // ����������� ������� ������ POWER/ADJ (PB1)
			u16 x = 0;
			while ( (BTN_READ) && (x++ < BTN_ONOFF_DELAY) )  // ���������, ������� ������� ������ ������. 
				_delay_ms(1);
			
			if  (x < BTN_ONOFF_DELAY)  // �������� ������� - ������������ ������� ���� �(���) ��������� ��
				shortpress();
			
			if (x >= BTN_ONOFF_DELAY)  // ������� ������� - ���������� �������� � ���.������
				longpress();
			
			while (BTN_READ); // ����������� (����� ����� ���������������� ����������)
		}

		#ifdef BAT_CHECK
		if (bat_time >= BAT_PERIOD)  { // �������� ������ ������� ����� ������������ ��������
			bat_time = 0; // ��������
			bat_check();
		}
		bat_time++;
		#endif
		
		_delay_ms(1);
	}
}

ISR(INT0_vect) {}	// ���������� ������ �� ������, ����� ����������� ��
 
void wakeup(void) {
	// ������������� �� ����� ��� ��� ��� ������
	rate_dir = 0;
	if (led_state) LED_ON;
	LOAD_CONNECT;
	PWM_ON;
	#ifndef RATE_REMEMBER // ���� ��� ������ �������
	rate = RATE_MIN; // ���������� ����� �� ���. �������
	#endif
	
	ADCSRA |= _BV(ADPS1) | _BV(ADPS0) | _BV(ADEN); // ������������ �������, ��������� ���
	bat_getvoltage(); // ������� ������ ��� (������ ���� �����, ���-�� � �������� ��� ��� ����)
	
	while (BTN_READ); // �����������, ����� ��� ��������� ����� ����� �����������, ��������� � ���������� ��������
	_delay_ms(100);
}

void setup(void) {
	// ��������� �� ����� ������ ������� �� �������

	// PB0 - Led CREE (OC0A ���-�����, ����������), ���������� ����
	//LOAD_CONNECT; 
	//LOAD_OFF; 
	
	// PB1 - ������ POWER/ADJ, ��� �� INT0, Flip-Flop. �����.�������� � �������
	PORTB |= _BV(1);
	
	// PB2 - LED2,  ������ ��������� (�����������)
	LED_SETPIN;  
	LED_OFF;
	
	//PB3 - �� ���. �����.�������� � �������
	PORTB |= _BV(3);
	
	//PB4 // ADC2 (�������� ������ �������)
	// Hi-z

	// PB5 - �� ���, RESET
	
	// ��������� ���. Attiny13 datasheet, page 92
	ADMUX |= _BV(MUX1); // [1:0] = 1:0 (ADC2)
	ADMUX |= _BV(ADLAR); // �������� 2 ������� ����, �������� 8-������ ����������
	ADMUX |= _BV(REFS0); // ���������� ��� 1.1V

	ACSR |= _BV(ACD); // ��������� ���������� (�� ��������� �������)
	
	pwm_setup(); // ��������� �������
	PWM_OFF;
	
	#ifdef STARTBLINKS
	LED_ON;
	_delay_ms(50);
	LED_OFF;
	#endif
	
	#ifdef STARTSLEEP
	sleep(); 
	#endif	
	
}

void sleep(void){
	// ��� � ����������� ���������
	PWM_OFF;
	LOAD_OFF;
	LOAD_DISCONNECT; // Hi-z
	LED_OFF;
	
	ADCSRA &= ~_BV(ADEN); // ��������� ���
	
	// ��������� ���������� ���
	GIMSK |= _BV(INT0); // ��� ���������� INT0
	MCUCR &= ~(_BV(ISC01) | _BV(ISC00)); // INT0 �� ������� ������
	set_sleep_mode(SLEEP_MODE_PWR_DOWN); // ����� power-down

	// �������� ���� ���, ������������������ �� ������!
	sleep_enable();
	cli();
	sei(); // �������� ����������, ����� �� ���������
	sleep_cpu(); // ���������� ���
	cli(); // ����������, �����������, ��������� ����������
	GIMSK = 0x00; // ��������� INT0
	sleep_disable();
	sei();                         
	// ������������

	wakeup(); // �������� �� � ������� ���������
}

void beacon(void) {
	// ���. ����� - ������ 1 ��. � ���� ������ ��� �������� ������ �������.
	PWM_ON; // Minrate ��� ����������
	while (!BTN_READ) {	// ���� �� ������ ������, �������� � ������ �����	
		LOAD_ON; // � �������� 1�� ���������� �� 1/20c (50��)
		_delay_ms(50);
		LOAD_OFF;
		_delay_ms(950);		
	}			
}

void strobe(void) {
	// ���. ����� - ���������� 8 ��. � ���� ������ ��� �������� ������ �������.		
	while (!BTN_READ) { // ���� �� ������ ������, �������� � ������ �����������
		LOAD_ON;
		_delay_ms(25);
		LOAD_OFF;
		_delay_ms(100);
	}		
}

void pwm_setup(void) {
	// ��������� ������� � ������ % ��� �������� rate
	// http://osboy.ru/blog/microcontrollers/attiny13-pwm.html ������ � attiny13 ����� ���� - ��� T0, �� ������������
	// ��� ���� �� OC0B ����� INT0, ������� ������������ ������ ����� (�������� ��� AUX_LED) �� ���������.
	
	TCCR0B |= _BV(CS00 ); // 001 - �������� ��������� CLK/1 (�������� ��� = CLK/256 = 4687.5 ��)
	TCCR0A |= _BV(WGM01) | _BV(WGM00); // 11 - ����� Fast PWM
	//TIMSK0 |= _BV(TOIE0); // ���������� �� ������������
	
	#ifdef NCH 
	TCCR0A |= _BV(COM0A1); // 10 - ��������� 0 �� ������ OC0A ��� ���������� � A, ��������� 1 �� ������ OC0A ��� ��������� �������� (����������� �����)
	#endif
	
	#ifdef PCH 
	TCCR0A |= _BV(COM0A1) | _BV(COM0A0); // 11 - ��������� 1 �� ������ OC0A ��� ���������� � A, ��������� 0 �� ������ OC0A ��� ��������� �������� (��������� �����)
	#endif
}

void longpress(void) {
	// ������� ������� ������ - ���������� ����������� ���
	
	u8 x = 0; // ������� ������ ��� �������� � ���.�����
	u8 rate_step = RATE_STEP_DEF;
	rate_dir = !rate_dir; // ����������� ����������� (������/������ % ���)

	while (BTN_READ) { // ���� ������ ������, �������� �������� rate (~40 ��� � �������)
		_delay_ms(RATE_STEP_LEN);

		// ��������� rate_step � ����������� �� �������� rate
		// ��� ������� ��� ����������� ���������� ����������� ������� �� �� �������� ��� (�����-���������)
		// ��������� ��� ��� ����� ������� ��� - youtube.com/watch?v=9zrobCFeeGw (� tinyfl ���������� �����)
		#ifdef RATE_NONLINEAR 
		// ��������� ������ �������� ������ step_array, ���� �� ����� ���������� � rate
		// ������ ����������� ������ ������������� �� maxrate, ����� ����� ������
		u8 y = 0;
		while (rate > rate_step_array[y])  
			y++;
		rate_step = y + 1; // ��������� rate_step ��� ����� ���������� �������� ������� ������ + 1
		#endif
	
		if (rate_dir) {	// ������� rate � ������� �������
			if (rate < RATE_MAX) {
				if (255 - rate > rate_step)
					rate += rate_step;
				else
					rate = RATE_MAX;
			}
		}
		else {	// ��� � ������� �������
			if (rate > RATE_MIN) {
				if (rate > rate_step)
					rate -= rate_step;
				else
					rate = RATE_MIN;
			}
		}
	
		// ������ ��-CREE, ����� ���������� ������� ��������, ����������� ������� � ���.�����
		if ( (rate <= RATE_MIN) || (rate >= RATE_MAX) ) {
			PWM_OFF;
			_delay_ms(125);
			PWM_ON;
			_delay_ms(125);
			x++;
		}	
	
		// ������ � ����������� ���.������, ���� ������ ��������� (125+125)*10 = 2500�� ���������
		if (x > AUXMODES_DELAY) {
			#ifdef AUXMODES		
			while (BTN_READ); // ����, ���� �������� ������	
			_delay_ms(250); 
			PWM_OFF;
			LED_OFF;
				
			if (rate <= RATE_MIN)
				beacon();
			if (rate >= RATE_MAX)
				strobe();
				
			if (led_state) LED_ON;
			PWM_ON;
			break;	
			#endif			
		}
		OCR0A = rate; // � ����� �������� ��� � ����� ��������� rate	
	} // END while (PB1_Read == BTN) 
	
}

void shortpress(void) {
	// �������� ������� ������
	#ifdef BTN_DBCLICK
	PWM_OFF;
	LOAD_OFF;
	LOAD_DISCONNECT;
	LED_OFF;
	u8 y = 0;
	_delay_ms(BTN_DBCLICK_DELAY);
	
	while ((y++ < BTN_DBCLICK_LEN) && (!BTN_READ))  // ���� ���� � ������� ������������� ���������
		_delay_ms(1);
			
	if 	(BTN_READ) { // ���� �� ���, �������� ���.�� (��� ���������)
		led_state = !led_state;
		if (led_state) LED_ON;
		LOAD_CONNECT;
		PWM_ON;
		_delay_ms(100);
	} else sleep();
	#else
	sleep(); // ����� ������ ��������
	#endif

}

u8 bat_getvoltage(void) {
	/*
	��������� ���������� ������� � �������� ���
	���������� ��� 1.1V (1.0-1.2V, page 121). ������� �������� Vref ��� ������� �������� ����� ������� �� 1.0�
	����. ����������, ������� ����� � ����� - 4.35�, ������� Kdiv ������ ���� � ������ 4.4-5.0
	��� ����� �������� ���� 750K:220K, Kdel ���������� 4.41, Vmax = 4.41 �, Vraw = 17.22 mv/adc
	
	������� ������� ���������� �� ���: ADC = (Vin/Kdiv) * 256
	���������� Vraw mv/raw = (Vion/256)*Kdiv
	������ Kmv �� Vraw � Vin = Vin/Vraw = 26.6 mV
	
	��� �������� /4.41: 174.15 � 162.54 ��������������
	*/
	PWM_OFF;
	LED_OFF;
	_delay_ms(1); // ������������� ���������� ����� ���������� ������ ��������, �������� 12 ����
	ADCSRA |= _BV(ADSC); // �������� ��������������
	while (ADCSRA & _BV(ADSC)); // ���� ��������� ��������������
	PWM_ON;
	if (led_state) LED_ON;
	return ADCH;
}

void bat_check(void) {
	// �������� ������ �������
	u8 adc_raw = bat_getvoltage();

	if (adc_raw < BAT_WARNING) { // �������������� (������ ������ �������). 
		u8 x = adc_raw; // ����� �������� ��� ��������� �������
		while ( (x < BAT_WARNING) ) { // ������ ������� �������� 5 ������� ��� (1 ������� - ����� ���������, 1+n - �������)
			PWM_OFF;
			_delay_ms(75);
			PWM_ON;
			_delay_ms(75);
			x = x + BAT_INFO_STEP;
		}
	}
	
	if (adc_raw < BAT_SHUTDOWN) // ����������
		sleep();	
}