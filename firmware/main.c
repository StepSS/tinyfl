/*
 * TinyFL - Simple AVR (Attiny13) PWM LED driver
 * (c) madcatdev https://github.com/madcatdev
*/

#include "main.h" // < All settings are here

// ������� ��������, 0 - ��������, 255 - ������������ (���)
u8 rate; 
// ����������� ��������� ������� ��������, 1 - ��������� ���, 0 - ������� ���
u8 rate_dir = 0; 
// ������ ����� ������ �����-���������, � ����� ����������� maxrate
const u8 rate_step_array[12] = {10,25,50,60,75,80,90,120,130,140,150,RATE_MAX}; 
// ��������� ������� ���������� LED2
bool led_state = false;  

// TODO - ����������� bool �������� �� u8


int main(void)	{
	setup();
	
	// Main LOOP
	u16 bat_time = 0; 
	u16 x;
	
	while(1)    {
		
		if (BTN_READ){ 
			x = 0;
			while ( (BTN_READ) && (x++ < BTN_ONOFF_DELAY) )  // ���������, ������� ������� ������ ������. 
				_delay_ms(1);
			
			if  (x < BTN_ONOFF_DELAY)  // �������� ������� - ������������ ������� ���� �(���) ��������� ��
				shortpress();
			
			if (x == BTN_ONOFF_DELAY)  // ������� ������� - ���������� �������� � ���.������
				longpress();
			
			while (BTN_READ); // ����������� (����� ����� ���������������� ����������)
		}

		#ifdef BAT_CHECK
		if (bat_time == BAT_CHECK_PERIOD)  { // �������� ������ ������� ����� ������������ ��������
			bat_time = 0; 
			bat_check();
		}
		bat_time++;
		#endif
		
		_delay_ms(1);
	}
}

ISR(INT0_vect) {}	// ���������� ������ �� ������, ����� ����������� ��
 
void setup(void){
	// ��������� �� ����� ������ �������

	// ��������� ���. Attiny13 datasheet, page 92
	ADMUX = _BV(MUX1) | // [1:0] = 1:0 (ADC2)
	_BV(ADLAR) | // �������� 2 ������� ����, �������� 8-������ ����������
	_BV(REFS0); // ���������� ��� 1.1V
	ACSR |= _BV(ACD); // ��������� ���������� (�� ��������� �������)

	// ��������� �������. ����� ��� ������ �� ��������� ���� LED_MAIN �� ����� (����� ������ �� �������� ���������� C1 ����� ������ �������)
	// �� OC0B ����� INT0, ������� ������������ ������ ����� (�������� ��� AUX_LED) �� ���������.
	TCCR0B = _BV(CS00 ); // 001 - �������� ��������� CLK/1 (�������� ��� = CLK/256 = 4687.5 ��)
	TCCR0A = _BV(WGM01) | _BV(WGM00); // 11 - ����� Fast PWM
	
	//TIMSK0 |= _BV(TOIE0); // ���������� �� ������������
	
	// PB0 - LED_MAIN (OC0A ���-�����, ����������), ���������� ����
	// PB1 - ������ POWER/ADJ, ��� �� INT0, Flip-Flop. �����.�������� � �������
	// PB2 - LED_BACK,  ������ ��������� (�����������)
	// PB3 - �� ���. �����.�������� � ������� (����� �� ��������� � ������� � �� ������ ���)
	// PB4 - ADC2 (�������� ������ �������), Hi-z
	// PB5 - RESET
	PORTB = _BV(1) | _BV(3);
	DDRB = _BV (0) | _BV(2);
	
	#ifdef STARTBLINKS
	LED_BACK_on;
	_delay_ms(50);
	LED_BACK_off;
	#endif
	
	#ifdef STARTSLEEP
	sleep();
	#else
	wakeup();
	#endif
}
 
void wakeup(void) {
	// ������������� �� ����� ��� ��� ��� ������
	//rate_dir = 0;
	if (led_state) 
		LED_BACK_on;
	
	#ifndef RATE_REMEMBER // ���� ��� ������ �������
		rate = RATE_DEFAULT; // ���������� ����� �� ���. �������
	#endif
	OCR0A = rate;
	
	ADCSRA |= _BV(ADPS1) | _BV(ADPS0) | _BV(ADEN) ; // ������������ �������, ��������� ���
	ADCSRA |= _BV(ADSC); // ������� ��������� (���������� �������� ������ ��� ����� ��������� ��� ������������ ���)
	
	while (BTN_READ); // �����������, ����� ��� ��������� ����� ����� �����������, ��������� � ���������� ��������
	PWM_on;
	_delay_ms(100);
}

void sleep(void){
	// ��� � ����������� ���������
	PWM_off;
	LED_MAIN_off;
	LED_BACK_off;
	ADCSRA &= ~_BV(ADEN); // ��������� ���
	
	do {
		// ��������� ���������� ���
		GIMSK = _BV(INT0); // ��� ���������� INT0
		MCUCR = _BV(SE) | _BV(SM1); // SE - SLeep_enable = 1, SM - Power-down = 10, ISC = 00 (low level of INT0 generates interrupt)
		// ����� ������ ����� (�� ����� �� 10 ���� ������) - set_sleep_mode(SLEEP_MODE_PWR_DOWN); sleep_enable();
		
		sei(); // �������� ����������, ����� ������ ��������
		asm ("sleep");  // ���������� ��� - sleep_cpu();
		cli(); // ����������, �����������, ��������� ����������
		GIMSK = 0x00; // ��������� INT0
		MCUCR = 0x00; // ������� ��� SE, � ������ ��� ���������. ����� ��� �� � sleep_disable();, �� ����� �� 4 ����� ������
		sei();
		
		// �������� �� ������������ �������� ��������� (������ �� ������������� �����)
		_delay_ms(STARTDELAY);
		
	} while (!BTN_READ);

	wakeup(); // �������� �� � ������� ���������
}

void beacon(void) {
	// ���. ����� - ������ 1 ��. � ���� ������ ��� �������� ������ �������.
	OCR0A = RATE_MAX; 
	while (!BTN_READ) {	// ���� �� ������ ������, �������� � ������ �����	
		PWM_on;
		_delay_ms(50);
		PWM_off;
		_delay_ms(950);		
	}			
}

void strobe(void) {
	// ���. ����� - ���������� 8 ��. � ���� ������ ��� �������� ������ �������.		
	OCR0A = RATE_MAX; 
	while (!BTN_READ) { // ���� �� ������ ������, �������� � ������ �����������
		PWM_on;
		_delay_ms(25);
		PWM_off;
		_delay_ms(100);
	}		
}

void longpress(void) {
	// ������� ������� ������ - ���������� ����������� ���
	
	u8 rate_step = RATE_STEP_DEF;
	u8 x = 0; // ������� ������ ��� �������� � ���.�����
	rate_dir =~rate_dir; // ����������� ����������� (������/������ % ���)

	while (BTN_READ) { // ���� ������ ������, �������� �������� rate 
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
			PWM_off;
			_delay_ms(125);
			PWM_on;
			_delay_ms(125);
			x++;
		}	
	
		// ������ � ����������� ���.������, ���� ������ ��������� (125+125)*10 = 2500�� ���������
		if (x > AUXMODES_DELAY) {
			#ifdef AUXMODES		
			while (BTN_READ); 
			_delay_ms(250); 
			PWM_off;
			LED_BACK_off;
				
			if (rate <= RATE_MIN)
				beacon();
			if (rate >= RATE_MAX)
				strobe();
				
			if (led_state) LED_BACK_on;
			PWM_on;
			x = 0;
			//break;	
			#endif			
		}
		OCR0A = rate; // � ����� �������� ��� � ����� ��������� rate	
	} // END OF while (BTN_READ)
	
}

void shortpress(void) {
	// �������� ������� ������
	#ifdef BTN_DBCLICK
	PWM_off;
	LED_BACK_off;
	_delay_ms(BTN_DBCLICK_DELAY);
	
	u8 y = 0;
	while ((y++ < BTN_DBCLICK_LEN) && (!BTN_READ))  // ���� ���� � ������� ������������� ���������
		_delay_ms(1);
			
	if 	(BTN_READ) { // ���� �� ���, �������� ���.�� (��� ���������)
		led_state = !led_state;
		if (led_state) 
			LED_BACK_on;
		PWM_on;
		_delay_ms(100);
	} else 
		sleep();
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
	
	���������� ������ ���� ��������� ����� ��������� ��� "��������" ���
	*/
	//PWM_off;
	//LED_BACK_off;
	_delay_us(500); // ������������� ���������� ����� ���������� ������ ��������, �������� 12 ����
	ADCSRA |= _BV(ADSC); // �������� ��������������
	while (ADCSRA & _BV(ADSC)); // ���� ��������� ��������������
	//PWM_on;
	//if (led_state) 
	//	LED_BACK_on;
	return ADCH;
}

void bat_check(void) {
	// �������� ������ �������
	PWM_off;
	u8 adc_raw = bat_getvoltage();
	PWM_on;

	// �������������� 
	if (adc_raw < BAT_WARNING) { 
		u8 x = adc_raw; // ����� �������� ��� ��������� �������
		while (x < BAT_WARNING) { // ������ ������� �������� 5 ������� ��� (1 ������� - ����� ���������, 1+n - �������)
			PWM_off;
			_delay_ms(50);
			PWM_on;
			_delay_ms(50);
			x += BAT_INFO_STEP;
		}
	}
	
	// ����������
	if (adc_raw < BAT_SHUTDOWN) 
		sleep();	
	
}
