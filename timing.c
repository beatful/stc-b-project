// ����ʽ����/ѧϰ ����ʱ��
// 201808010805 �ƿ�1808 ���ư�
// encoding = GBK2312

#include <STC15F2K60S2.h>
#include <intrins.h>
#include <math.h>
#define uint unsigned int
#define uchar unsigned char
#define ADC_CHS1_7 0X07
#define ADC_POWER 0X80
#define ADC_FLAG 0X10  /*��A/Dת����ɺ�ADC_FLAGҪ�������*/
#define ADC_START 0X08
#define ADC_SPEED_90 0X60
#define	P1_7_ADC 0x80

// �ṹ�� time ����
typedef struct {
    uint hour; //Сʱ
    uint min_g; // ���Ӹ�λ
    uint min_d; // ���ӵ�λ
    uint sec_g; // ���λ
    uint sec_d; // ���λ
} Time;



//  ����λ��ѡ����
// uchar HOUR_SEL = 0;         // ʱ
// uchar MIN_G_SEL = 1;        // ����_��λ
// uchar MIN_D_SEL = 2;        // ����_��λ
// uchar SEC_G_SEL = 3;        //��_��λ
// uchar SEC_D_SEL = 4;        //��_��λ

uint i=0;
uchar duanxuan[]={
    0x3f,   //0
    0x06,   //1
    0x5b,   //2
    0x4f,   //3
    0x66,   //4
    0x6d,   //5
    0x7d,   //6
    0x07,   //7
    0x7f,   //8
    0x6f,   //9
    0x40    //-    
}; //��ʾ0-9�Լ�����
uchar weixuan[]={0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07};			 //�����0-7

uchar xiaoshudian[] = {  //�����Ƿ���ʾС����
    0x80,       //��-��λ Ĭ����ʾ
    0x00,       //��-��λ
    0x00,       //����-��λ
    0x00,       //����-��λ
    0x00       //Сʱ
};


uchar sel_xiaoshudian = 1;  // ������ǰ���Ƶ�λ �� 1��ʼ
uchar t = 0;//���� sel_xiaoshudian
uchar temp;//���� sel_xiaoshudian

uint num = 0;  // ��ʱ���������


// ��ǰ״̬ 0����ʼ״̬��������ʱ�䣬Ĭ��1Сʱ�� 1������ʱ״̬  2�����״̬
uint state = 0;

// ����������
sbit beep_io = P3^4;
//����������Ƶ�ʸ���
uchar pp = 0;
uint aa = 0;
bit close;

// �ṹ����� time�� Ĭ��1Сʱ
Time time = {
    1,  // hour   Сʱ       ��Χ��0~3 
    0,  // min_g  ���Ӹ�λ    ��Χ 0~5
    0,  // min_d  ���ӵ�λ    ��Χ 0~9
    0,  // sec_g  ���λ      ��Χ 0~5
    0,  // sec_d; ���λ      ��Χ 0~9
};

// ���ڻָ�ʱ��
//Time save_time = time;

/****** �������� *******/
void delay();
void Delayms();
void init();
void init_action();
void timing_action();
void beep_action();
void light_led(uint n);
void change0();
void change1();
void assignTime();
void assignFinish();
void refreshTime();
void resetTime();
int isEnd();
void openTimer0();
void closeTimer0();


// ��������delay
// ������
// ���ܣ�������ʱ !���������!
void delay(int n)				//��ʱ����
{
	while(n--);
}

/*******************************
 * ��������Delayms
 * ����  ��������ʱ����
 * ����  ����ʱi����
 * ���  ����
 *******************************/
void Delayms(char i) 			
{
	while(i--)
	{	
		int n=500;
		while (n--)
	    {
	        _nop_();
	        _nop_();
	        _nop_();
	        _nop_();
	        _nop_();
			_nop_();
	    }
	}
}

// ��������init
// ������
// ���ܣ������������ | �����ⲿ�ж� | ���õ�·״̬Ϊ 0
void init()
{
    state = 0; // ����״̬Ϊ��ʼ��
    P2M0=0xff;					//���������/�������������
   	P2M1=0x00;
    P0M0=0xff;
    P0M1=0x00;
    
    EA = 1;     //���ⲿ�ж�
    EX0 = 1;
    EX1 = 1;
    sel_xiaoshudian = 1; // ���ú�ѡ��λ
    xiaoshudian[0] = 0x80; // С������ʾ��һ��
    xiaoshudian[4] =xiaoshudian[3] =xiaoshudian[2] =xiaoshudian[1] = 0x00; // С������ʾ��һ��
    
    P3M1 = 0x00;
    P3M0 = 0x10;                  //����P3^4Ϊ����ģʽ
    beep_io = 0;                  // ����������
    //P0 = 0x00;                    //�ر�P0�˿�
    resetTime();
    closeTimer0();  // �� ��ʱ��0 ��ʼ״̬����Ҫ��ʱ��
    
     ET1=0;   //�ض�ʱ���ж�
    TR1=0;   //�ض�ʱ��1
}

// ���¼������йص��������ĸ�������
//==================================================== ��ʼ ==========================================================================
/**********************************
 * ��������Init_ADC
 * ����  ����ʼ��P1.7��ΪADC
 * ����  ����
 * ���  ����
 **********************************/
void init_ADC()
{
	P1ASF=P1_7_ADC;//��P1ASF�Ĵ�����Ӧλ��1
	ADC_RES = 0;//����Ĵ�������
//	ADC_RESL=0;
	ADC_CONTR = ADC_POWER | ADC_FLAG | ADC_START | ADC_SPEED_90 | ADC_CHS1_7;		//��Ӧλ��ֵ
	Delayms(2);
}

/**************************************
 * ��������GetADC
 * ����  �����ADת����ֵ,û������A/Dת���жϣ����忴IE��IP��
 * ����  ����
 * ���  ��ADת���Ľ��
 **************************************/
unsigned char GetADC()
{
	unsigned char result;
	ADC_CONTR = ADC_POWER | ADC_START | ADC_SPEED_90 | ADC_CHS1_7;//û�н�ADC_FLAG��1�������ж�A/D�Ƿ����
	_nop_();
	_nop_();
	_nop_();
	_nop_();
	while(!(ADC_CONTR&ADC_FLAG));//�ȴ�ֱ��A/Dת������
	ADC_CONTR &= ~ADC_FLAG; //ADC_FLAGE�����0
	result = ADC_RES; //��ȡAD��ֵ
    //Delayms(500);
	return result;	  //����ADCֵ
}

/********************************
 * ��������Fun_Keycheck()	   
 * ����  ����⹦�ܼ�����5����2����4����1��ȷ�ϼ�3�����ذ���3��0����û���·���0x07��������Ӧ��ֵ  (������������)
 * ����  ����
 * ���  ������Ӧ��ֵ
********************************/
unsigned char keyCheck()
{
	unsigned char key;
	key=GetADC();		  //���ADCֵ��ֵ��key
	if(key!=255)
	{
		Delayms(10);
		key=GetADC();
		if(key!=255)	  //��������
		{
	     	key=key&0xE0;//��ȡ��3λ������λ����
        	key=_cror_(key,5);//ѭ������5λ
			return key;
		}
	}
	return 0x07;
}


/**********************
�������ƣ�Fun_Key_task_HMS
�����������������ܼ������ʱ�������ֵ�����ã� 
��ڲ������� 
���ڲ�������
***********************/
uchar prev_key_val = 0x07;
uchar key_val = 0x07;
void Fun_Key_task_HMS()
{
    prev_key_val = key_val;
    //  �� 0x5 �� 0x2 ��4 �� 1
    key_val = keyCheck();
    if(key_val == 0x07)
    switch(prev_key_val)
    {
        // �� ��ǰ��Ӧλ��ֵ+1
        case 0x5:   // ���ӻ���ĵ�λ
            if(sel_xiaoshudian == 1 ) time.sec_d = (time.sec_d + 1) % 10;
            if(sel_xiaoshudian == 3 ) time.min_d = (time.min_d + 1) % 10;
        // ���ӻ�����ĸ�λ
            if(sel_xiaoshudian == 2) time.sec_g = (time.sec_g + 1) % 6;
            if(sel_xiaoshudian == 4) time.min_g = (time.min_g +1) % 6;
            // Сʱ
            if(sel_xiaoshudian == 5)
            time.hour = (time.hour+1) % 4;  
            break;
            
         case 0x2:   // �£���Ӧ��ֵ��һ
             //���λ
            if(sel_xiaoshudian == 1 )
            {
                if(time.sec_d == 0) time.sec_d = 9;
                else time.sec_d--;
            }
            //���λ
            if(sel_xiaoshudian == 2 )
            {
                if(time.sec_g == 0) time.sec_g = 5;
                else time.sec_g--;
            }
            //���ӵ�λ
            if(sel_xiaoshudian == 3 )
            {
                if(time.min_d == 0) time.min_d = 9;
                else time.min_d--;
            }
            //���Ӹ�λ
            if(sel_xiaoshudian == 4)
            {
                if(time.min_g == 0) time.min_g = 5;
                else time.min_g--;
            }
            //Сʱ
            if(sel_xiaoshudian == 5 )
            {
                if(time.hour == 0) time.hour = 3;
                else time.hour--;
            }
            break;
//          case 0x4:   // ��С����ѭ������
//              xiaoshudian[sel_xiaoshudian - 1] = 0x00; //ȡ��֮ǰλС�������ʾ
//              sel_xiaoshudian++;
//              if(sel_xiaoshudian == 6) sel_xiaoshudian = 1;
//              xiaoshudian[sel_xiaoshudian - 1] = 0x80; // ������ǰλС�������ʾ
         case 0x1:  // �ң�С����ѭ������
             xiaoshudian[sel_xiaoshudian - 1] = 0x00; //ȡ��֮ǰλС�������ʾ
             sel_xiaoshudian--;
             if(sel_xiaoshudian == 0) sel_xiaoshudian = 5;
              xiaoshudian[sel_xiaoshudian - 1] = 0x80; // ������ǰλС�������ʾ
    }
    prev_key_val = 0x07;
 }


//==================================================== ���� ==========================================================================



// ��������init_action()
// ������
// ���ܣ�ָ����·״̬Ϊ��ʼ�� 0 ʱ��·����Ϊ: ��ʾ 01-00-00��ʱ��Ĭ��ΪһСʱ�ɵ���
void init_action()
{   
    light_led(0x80); // ָʾ��
    Fun_Key_task_HMS(); //��⵼������ 
    assignTime();
}

// ��������timing_action
// ������
// ���ܣ�ָ����·״̬Ϊ����ʱ״̬ 1 ʱ��·����Ϊ����ָ����ʱ�俪ʼ����ʱ
void timing_action()
{
    xiaoshudian[sel_xiaoshudian - 1] = 0x00;
    assignTime(); // ����ʱ����ʾ
    light_led(0x40);
}

//��������beep_action
//������
//���ܣ�ָ����·״̬Ϊ��������ʱ������2
void beep_action()
{
    num++;
    assignFinish(); // ����ʱ����ʾ
    light_led(0x20);
    if(pp == 100)
    {
        pp = 0;
        if(!close) beep_io = ~beep_io;   //Ƶ��Ϊ 1KHz ����Ƶ�ź�
        
        aa++;
    }
    if(aa == 200)
    {
        beep_io = 1;
        close = 1;   // �����źų���ʱ�� 200*10*10us = 0.2s
    }
    if(aa == 300) //�ε�������Ϊ 300 * 10 * 10us = 0.3s
    {
        close = 0;
        aa = 0;
    }
    //beep_io = ~beep_io;
//     if(num == 10000)
//     {beep_io = ~beep_io;
//     delay(10);
//     num = 0;}
        
}


// ������ light_led
// ������n��n = 1,2,3, led λ��
// ���ܣ����Ƶ� n λ led ���𣬱�����ǰ״̬
void light_led(uint n)
{
    P23 = 1;    // �ر� ����ܣ����� led
    P0 = n;
    delay(200);
}

// ��������change0
// ������
// ���ܣ�ִ�к��� init
void change0() interrupt 0
{
    init(); // ִ�г�ʼ��
}

// ��������change1
// ������
// ���ܣ������ⲿ�ж�1 (KEY2)��������ʱ
void change1() interrupt 2
{
    state = 1;
    TMOD=0x02;    //���ö�ʱ��0Ϊ������ʽ2
    TH0=6;   //װ���ֵ
    TL0=6;
    openTimer0(); // ������ʱ��0�ж�
}

// ��������tim
// ������
// ���ܣ���ʱ���жϺ��������� 1 s ʱ�䣬�����в���


void tim() interrupt 1 using 1
{
       num++;
    if(state == 1) //  ��ʱ״̬
    {
        if(num == 3800)  // 1�룬����� num==4000 ����ʵ������е���
       {
           num = 0;
           refreshTime(); // ÿһ�����һ��ʱ��
       }
    }
       
       
       else if(state == 2)
       {
           num = 0;
            TH0 = 0xff;
           TL0 = 0x9c;
           pp++;
        }
        
        else return;
}

// ��������assignTime
// ������
// ���ܣ��������λΪ0 | ���� time Ϊ���������λ��ֵ | �жϼ�ʱ�Ƿ����
// ע�⣺assignTime ��û���޸� time ������
void assignTime()
{
    if(isEnd() && state == 1)// ʱ����ʾΪ 0 �ҵ�ǰ�Ǽ�ʱ״̬
    {
        state = 2;// ��ʱ���״̬
        //closeTimer0();
        sel_xiaoshudian = 1;
        
         num = 0;//���ü���ֵ ׼�� beep_action
        TMOD = 0x01;
        TR0 = 1;
        TH0 = 0xff;
        TL0 = 0x9c;
        ET0 = 1;
        EA = 1;
//    
    }
    P0 = duanxuan[0];
    P2 = weixuan[0];    // ���λ��ʾ 0 ����
    delay(600);
    for(i = 1; i < 8; i++)
    {
        P0=0;
        P2=weixuan[i];		//ѡ������ܵ�λ��
        if(i==2 || i == 5)
            P0=duanxuan[10];	//����ܵ�2λ�͵�5λ����ʾ����
        //����λ������ time ��ֵ���и�ֵ
        else 
        {
            switch(i) {
                case 1: P0 = duanxuan[time.hour] + xiaoshudian[4];break;
                case 3: P0 = duanxuan[time.min_g] + xiaoshudian[3];break;
                case 4: P0 = duanxuan[time.min_d] + xiaoshudian[2];break;
                case 6: P0 = duanxuan[time.sec_g] + xiaoshudian[1];break;
                case 7: P0 = duanxuan[time.sec_d] + xiaoshudian[0];break;
                default: break;
            }
        }
        delay(600);
    }
}

// ��������assignFinish
// ������
// ���ܣ� ��ʾ END
static void assignFinish()
{
    //0~7��ʾ
    for(i = 0; i < 3; i++)
    {
        P0=0;
        P2=weixuan[i];		//ѡ������ܵ�λ��
        
            switch(i) {
                case 0: P0 = 0x79;break; // E
                case 1: P0 = 0x37;break; // N
                case 2: P0 = 0x3f;break; // D
//                 case 3: P0 = 0x06;break; // I
//                 
//                 case 4: P0 = 0x6d;break; // S
//                 
//                 case 5: P0 = 0x76;break;//  H
//                 
//                 case 6: P0 = 0x79;break;//  E
//                 case 7: P0 = 0x3f;break; // D
                default: break;
            }
        delay(600);
    }
}
// ������ resetTime
// ����
// ���� ����ʱ��
static void resetTime()
{
    time.hour = 1;
    time.min_g = 0;
    time.min_d = 0;
    time.sec_g = 0;
    time.sec_d = 0;
}

// ��������refreshTime
// ������
// ���ܣ����µ�ǰ��ʾ��ʱ��
// ע�⣺���ÿ��Ǽ�ʱ����ʱ�ı߽����(ȫΪ0)���� assignTime �������д���
void refreshTime()
{
    // ��ʱΪ���㣬֮��Сʱ����һ����Сʱ��
    if(time.sec_d + time.sec_g + time.min_g + time.min_d == 0)
    {
        time.hour--;
        time.min_g = 5;
        time.min_d = 9;
        time.sec_g = 5;
        time.sec_d = 9;
        return ;
    }
    // ������
    //�����
    if(time.sec_d + time.sec_g == 0)
    {
        if(time.min_d == 0) time.min_g--, time.min_d = 9;
        else time.min_d--;
        time.sec_g = 5;
        time.sec_d = 9;
        return;
    }
     /*sec_d*/
     if(time.sec_d == 0) time.sec_d = 9, time.sec_g--;
     else time.sec_d--;   
}

// ��������isEnd
// ������
// ���ܣ��жϼ�ʱ�Ƿ����
int isEnd()
{
    return !(time.hour + time.min_g + time.min_d + time.sec_g + time.sec_d);
}

// ��������openTimer
// ������
// ���ܣ�����ʱ���ж�
static void openTimer0()
{
    ET0=1;   //����ʱ���ж�
    TR0=1;   //������ʱ��0 
}

//��������closeTimer
//������
//���ܣ��ض�ʱ���ж�
static void closeTimer0()
{
    ET0=0;   //�ض�ʱ���ж�
    TR0=0;   //�ض�ʱ��0 
}


//������
void main()
{
	init();
    init_ADC();
	while(1)
    {
        switch(state)
        {
            case 0: init_action();break;
            case 1: timing_action();break;
            case 2:beep_action();break;
            default:break;
        }
    }
}