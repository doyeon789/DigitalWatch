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

// fnd 동작을 위한 변수
unsigned char fnd_sel[] = {0x01, 0x02, 0x04, 0x08};
unsigned char fnd_data[] = {0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f};

// 동작 모드 설정 (기본 모드  : 0)
int mode = 0;

// 자리수를 나타내는 변수 (1: 초의 1자리, 2: 초의 10자리, 3: 분의 1자리, 4: 분의 10자리)
int place = 1;
int MnSnP[3] = {0,0,0}; //Minute and Second and Place

// mode가 0일때 시간, 분의 정보가 담겨 있는 변수
// 통신을 통해 받아올 예정
int hour_0 = 5;
int minute_0 = 20;
int system_second;

// mode가 1일떄 시간, 분의 정보가 담겨 있는 변수
int timer_second  = 0;
int isInDe = 1;

// 포트 초기화 함수
void port_init() {
    DDRA = 0xff;
    DDRC = 0xff;
    DDRG = 0xff;
    DDRE = 0x00;
}

// FND 제어 함수
void fnd_control(int second) {

	int hour = second/100;
	int minute = second%100;
	
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
		if(isInDe == 1) isInDe=0;
		else if(isInDe == 0) isInDe=1;
	}
}


void interrupt_init(){
	EICRB = 0xaa;
	EIMSK = 0xf0;
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

// adc값 초기화 함수
void adc_init(){
	DDRF &= ~(1<<0);
	ADMUX = 0;
	ADCSRA = (1<<ADEN) | (7<<ADPS0);
}
unsigned short read_adc() {
	ADCSRA |= (1<<ADSC);
	while((ADCSRA & (1<<ADIF)) != (1<<ADIF));
	unsigned char adc_low = ADCL;
	unsigned char adc_high = ADCH;
	
	if(ADMUX == 0) {
		ADMUX = 1;
	}
	else if(ADMUX == 1){
		ADMUX = 0;
	}
	
	return((unsigned short)adc_high << 8) | (unsigned short)adc_low;;
}

//mode1,2번에 있을 타임머 셋
void Time_set(int X, int Y,int mode){
	
	int first_time = 0;

	//여기 문제 있음 : 한번만 작동되도록 하기
	if(X <= 100  && isInDe == 1){
		PORTA = 0xff;
		place++;
		first_time = 0;
		if(place >= 4){
			place = 4;
		}
	}
	if(X >= 1022 && isInDe == 1){
		place--;
		first_time = 0;
		if(place <= 4){
			place = 1;
		}
	}
	

	switch(place) {
			case 1:
				timer_second += (Y >= 922 ? 1 : 0) - (Y <= 100 ? 1 : 0);
				break;
			case 2:
				timer_second += (Y >= 922 ? 10 : 0) - (Y <= 100 ? 10 : 0);
				break;
			case 3:
				timer_second += (Y >= 922 ? 60 : 0) - (Y <= 100 ? 60 : 0);
				break;
			case 4:
				timer_second += (Y >= 922 ? 600 : 0) - (Y <= 100 ? 600 : 0);
				break;
			default:
				// 아무런 액션 없음
				break;
		}
	
	

	if(timer_second <= 0) {
		timer_second = 0;
	}

	switch(mode){
		case 1:
			timer_second = (timer_second >= 9999) ? 9999 : timer_second;
			break;
		case 2:
			timer_second = (timer_second >= 2400) ? 2400 : timer_second;
			break;
		
		default:
			// 아무런 액션 없음
			break;
	}
}

// 디지털 시계 알고리즘 함수 (모드 : 0)
void Digital_Watch() {
	if (minute_0 >= 60) {
		minute_0 = 0;
		hour_0++;
		if (hour_0 >= 24) {
			hour_0 = 0;
		}
	}
}

// 디지털 타이머 알고리즘 함수 (모드 : 1)
void Digital_Timer(int X, int Y) {
	Time_set(X,Y,mode);
	// 타이머 감소 밑 깜박이기
}

// 메인 함수
int main(void) {
	
	int joystick_x = 0;
	int joystick_y = 0;
	
	adc_init(); // 아날로그 초기화
    port_init(); // 포트 초기화
    timer1_Nomalmode_init(); // 타이머1 초기화
	timer2_Nomalmode_init(); // 터이머2 초기화
	interrupt_init(); // 인터럽트 초기화
    sei(); // 전역 인터럽트 허용

    while (1) {
		switch (mode) { // 모드에 따른 동작 분기
            case 0:
				system_second = hour_0*100 + minute_0;
				PORTA = 0x01;
                Digital_Watch(); // 시계 알고리즘 호출
				fnd_control(system_second); // FND 표시
                break;
			case 1:
				PORTA = 0x02;
				
				joystick_x = read_adc();
				joystick_y = read_adc();
				
				Digital_Timer(joystick_x,joystick_y);
				fnd_control(timer_second); // FND 표시
				break;
            case 2:
				PORTA = 0x04;
				
				joystick_x = read_adc();
				joystick_y = read_adc();

				break;
        }
    }
}


//((seconds % 3600) / 60)/10
//((seconds % 3600) / 60)%10
//(seconds % 60)/10
//(seconds % 60 )%10