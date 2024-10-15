/*
 * DigitalWatch.c
 *
 * Created: 2024-09-24 오후 1:52:08
 * Author : doyeon
 */ 
#define F_CPU 16000000
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

//* [변수 설정] *//
// fnd 동작을 위한 변수
unsigned char fnd_sel[] = {0x01, 0x02, 0x04, 0x08};
unsigned char fnd_data[] = {0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f};

// 동작 모드 설정 (기본 모드  : 0)
int mode = 0;

// {"공용 변수"}
// 자리수를 나타내는 변수 (1: 초의 1자리, 2: 초의 10자리, 3: 분의 1자리, 4: 분의 10자리)
int place = 1;
volatile int tempX = 50;
volatile int tempY = 30;
int temp_second = 0;

// {"mode : 0"}
// mode가 0일때 시간, 분의 정보가 담겨 있는 변수
int hour_0 = 16;
int minute_0 = 21;
int system_second;

// {"mode : 1"}
// mode가 1때 시간, 분의 정보가 담겨 있는 변수
int timer_second  = 0;
int isTimer_set = 0;
int isTimerBuzzer = 0;

// {"mode : 2"}
// mode가 2일때 시간, 분 정보가 담겨 있는 변수
int alarm_second = 0;
int isAlarm_set = 0;
int isAlarmBuzzer = 0;

//* [함수] *//
// 포트 초기화 함수
void port_init() {
    DDRA = 0xff;
    DDRC = 0xff;
    DDRG = 0xff;
    DDRE = 0x00;
	DDRD = 0x00;
	DDRB = 0xff;
}

// FND 제어 함수
void fnd_control(int second) {
	int hour = second/60;
	int minute = second%60;
	
    PORTG = fnd_sel[0];
    PORTC = ~fnd_data[hour / 10];
    _delay_ms(1);
    
    PORTG = fnd_sel[1];
    PORTC = ~fnd_data[hour % 10] + 128;
    _delay_ms(1);
    
    PORTG = fnd_sel[2];
    PORTC = ~fnd_data[minute / 10];
    _delay_ms(1);
    
    PORTG = fnd_sel[3];
    PORTC = ~fnd_data[minute % 10];
    _delay_ms(1);
}

// 인터럽트 초기화 함수
void interrupt_init(){
	EICRB = 0xaa;
	EICRA = 0xff;
	EIMSK |= 0xf0|(1<<INT0);
}
// 인터럽트 동작 
ISR(INT0_vect){
	// 채터링 방지
	_delay_ms(100);
	EIFR = (1 << 1);
	if ((PINE & (1 << PINE1)) == 0) {}
	
	// 모드가 1일때 누르면 하강 시작
	if(temp_second!=0 && mode == 1){
		isTimer_set = 1;
		timer_second = temp_second;
	}
	// 모드가 2일때 누르면 알람 설정
	if(temp_second!=0 && mode == 2){
		isAlarm_set = 1;
		alarm_second = temp_second;
	}	

	if(isTimerBuzzer == 1 || isAlarmBuzzer == 1){
		isTimerBuzzer = 0;
		isTimer_set = 0;
		isAlarmBuzzer = 0;
		isAlarm_set = 0;
	}

	
}
ISR(INT4_vect){
	// 채터링 방지
	_delay_ms(50);
	EIFR = (1 << 4);
	if ((PINE & (1 << PINE4)) == 0) {}

	mode = 0; // 모드 0 설정
	
	// fnd 끄기
	for (int i = 0; i < 4 ;i++) {
		PORTG = fnd_sel[i];
		PORTC = ~0;
		_delay_ms(1);
	}
}
ISR(INT5_vect){
	// 채터링 방지
	_delay_ms(100);
	EIFR = (1 << 5);
	if ((PINE & (1 << PINE5)) == 0) {}
	
	mode = 1; // 모드 1 설정
	
	// fnd 끄기
	for (int i = 0; i < 4 ;i++){
		PORTG = fnd_sel[i];
		PORTC = ~0;
		_delay_ms(1);
	}
}
ISR(INT6_vect){
	// 채터링 방지
	_delay_ms(100);
	EIFR = (1 << 6);
	if ((PINE & (1 << PINE6)) == 0) {}
	
	mode = 2; // 모드 2 설정
	
	// fnd 끄기
	for (int i = 0; i < 4 ;i++){
		PORTG = fnd_sel[i];
		PORTC = ~0;
		_delay_ms(1);
	}
}

// 타이머0 초기화 함수
void timer0_Nomalmode_init(){
	TCCR0 = (1<<1)|(1<<0);
	TIMSK &= ~(1<<TOIE0);
}
// 타이머0 인터럽트 서비스 루틴  
ISR(TIMER0_OVF_vect){
	PORTB^=1<<4;
	TCNT0=17;
}

// 타이머1 초기화 함수
void timer1_Nomalmode_init() {
    TCCR1A = 0; // 일반 모드 설정
    TCCR1B = (1 << WGM12) | (1 << CS12) | (1 << CS10); // CTC 모드, 프리스케일러 설정
	TCNT1 = 0;
    OCR1A = 15624; // 1초마다 비교
    TIMSK |= (1 << OCIE1A); // 비교 매치 인터럽트 허용
}
// 타이머1 인터럽트 서비스 루틴
ISR(TIMER1_COMPA_vect) {
	static int repeat1A = 0;
	repeat1A++;
	if(repeat1A >= 60){
		minute_0++; // 분 증가
		repeat1A = 0;
	}
	
	// 타이머 설정 완료시 하강 시작
	if(isTimer_set == 1){
		// 타이머 완료
		if(--timer_second <= 0){
			timer_second = 0;
			temp_second = 0;			
			isTimerBuzzer = 1;
		}
	}
}

// 타이머2 초기화 함수
void timer2_Nomalmode_init(){
	TCCR2 |= (1<<CS22)|(1<<CS20);
	TCNT2=4;
	TIMSK|=(1<<TOIE2);
}
// 타이머2 인터럽트 서비스 루틴
ISR(TIMER2_OVF_vect){
	static int repeat2=0;
	TCNT2=4;
	repeat2++;
	if(repeat2>=62*1){
		repeat2=0;
	}
}

// adc값 초기화 함수
void adc_init(){
	DDRF &= ~(1<<0);
	ADMUX = 0;
	ADCSRA = (1<<ADEN) | (7<<ADPS0);
}
// adc값 읽어오는 함수
unsigned short read_adc() {
	ADCSRA |= (1<<ADSC);
	while((ADCSRA & (1<<ADIF)) != (1<<ADIF));
	unsigned char adc_low = ADCL;
	unsigned char adc_high = ADCH;
	
	if(ADMUX == 0)	ADMUX = 1;
	else if(ADMUX == 1)	ADMUX = 0;
	
	return((unsigned short)adc_high << 8) | (unsigned short)adc_low;;
}

//* [모드에 따른 함수] *//
//mode1,2번에 있을 타임머 셋
void Time_set(int X, int Y,int mode){
	
	// 조이스틱의 위치에 따라 시간의 자리수 선택 변경
	if (X <= 100 && --tempX <= 0) {
		place = (place < 4) ? place + 1 : 4;
		tempX = 50;
	}
	if (X >= 1022 && --tempX <= 0) {
		place = (place > 1) ? place - 1 : 1;
		tempX = 50;
	}

	// 조이스틱의 위치에 따라 시간설정하는 시간을 바꾸기
	switch(place) {
			case 1:
				tempY--;
				if(tempY <= 0){
					temp_second += (Y >= 922 ? 1 : 0) - (Y <= 200 ? 1 : 0);
					tempY = 30;
				}
				break;
			case 2:
				tempY--;
				if(tempY <= 0){
					temp_second += (Y >= 922 ? 10 : 0) - (Y <= 100 ? 10 : 0);
					tempY = 30;
				}
				break;
			case 3:
				tempY--;
				if(tempY <= 0){
					temp_second += (Y >= 922 ? 60 : 0) - (Y <= 100 ? 60 : 0);
					tempY = 30;
				}
				break;
			case 4:
				tempY--;
				if(tempY <= 0){
					temp_second += (Y >= 922 ? 600 : 0) - (Y <= 100 ? 600 : 0);
					tempY = 30;
				}
				break;
			default:
				// 아무런 액션 없음
				break;
		}
	if(temp_second <= 0) {
		temp_second = 0;
	}

	switch(mode){
		case 1:
			// 60 * 99 + 59 = 5999
			temp_second = (temp_second >= 5999) ? 5999 : temp_second;
			break;
		case 2:
			// 60 * 24 = 1440
			temp_second = (temp_second >= 1440) ? 1440 : temp_second;
			break;
		
		default:
			// 아무런 액션 없음
			break;
	}
}

//* 디지털 시계 알고리즘 함수 (모드 : 0) *//
void Digital_Watch() {
	if (minute_0 >= 60) {
		minute_0 = 0;
		hour_0++;
		if (hour_0 >= 24) {
			hour_0 = 0;
		}
	}
}
//* 디지털 타이머 알고리즘 함수 (모드 : 1) *//
void Digital_Timer(int X, int Y) {
	Time_set(X,Y,mode);
}
//* 디지털 알람 시계 알고리즘 함수 (모드 : 2) *//
void Digital_Alarm(int X, int Y){
	Time_set(X,Y,mode);
}

//* 메인 함수 *//
int main(void) {
	
	int joystick_x = 0;
	int joystick_y = 0;
	
	adc_init();	 // 아날로그 초기화
    port_init(); // 포트 초기화
	interrupt_init(); // 인터럽트 초기화
	timer0_Nomalmode_init(); // 터이머0 초기화
    timer1_Nomalmode_init(); // 타이머1 초기화
    sei(); // 전역 인터럽트 허용

    while (1) {
		switch (mode) { // 모드에 따른 동작 분기
            case 0:
				system_second = hour_0*60 + minute_0;
				PORTA = 0x01;
                Digital_Watch(); // 시계 알고리즘 호출
				fnd_control(system_second); // FND 표시
                break;
			case 1:
				PORTA = 0x02;
				if(isTimer_set == 0){
					joystick_x = read_adc();
					joystick_y = read_adc();
				}
				Digital_Timer(joystick_x,joystick_y);
				if(isTimer_set == 0) fnd_control(temp_second);
				if(isTimer_set == 1) fnd_control(timer_second);
				break;
            case 2:
				PORTA = 0x04;
				if(isAlarm_set == 0){
					joystick_x = read_adc();
					joystick_y = read_adc();
				}
				Digital_Alarm(joystick_x,joystick_y);
				if(alarm_second == system_second){
					alarm_second = 0;
					isAlarmBuzzer = 1;
				}
				if(isAlarm_set == 0) fnd_control(temp_second);
				if(isAlarm_set == 1) mode=1;
				break;
        }

		if((isTimerBuzzer == 1) && (isTimer_set != 0)){
			TIMSK |= (1<<TOIE0);
		}
		else{
			TIMSK &= ~(1<<TOIE0);
		}
	}
}
/*
 *[Todo list]*

*/
