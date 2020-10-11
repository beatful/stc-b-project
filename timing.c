// 沉浸式工作/学习 倒计时器
// encoding = GBK2312

#include <STC15F2K60S2.h>
#include <intrins.h>
#include <math.h>
#define uint unsigned int
#define uchar unsigned char
#define ADC_CHS1_7 0X07
#define ADC_POWER 0X80
#define ADC_FLAG 0X10  /*当A/D转换完成后，ADC_FLAG要软件清零*/
#define ADC_START 0X08
#define ADC_SPEED_90 0X60
#define	P1_7_ADC 0x80

// 结构体 time 定义
typedef struct {
    uint hour; //小时
    uint min_g; // 分钟高位
    uint min_d; // 分钟低位
    uint sec_g; // 秒高位
    uint sec_d; // 秒低位
} Time;



//  几个位的选择定义
// uchar HOUR_SEL = 0;         // 时
// uchar MIN_G_SEL = 1;        // 分钟_高位
// uchar MIN_D_SEL = 2;        // 分钟_低位
// uchar SEC_G_SEL = 3;        //秒_高位
// uchar SEC_D_SEL = 4;        //秒_低位

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
}; //显示0-9以及横线
uchar weixuan[]={0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07};			 //数码管0-7

uchar xiaoshudian[] = {  //控制是否显示小数点
    0x80,       //秒-低位 默认显示
    0x00,       //秒-高位
    0x00,       //分钟-低位
    0x00,       //分钟-高位
    0x00       //小时
};


uchar sel_xiaoshudian = 1;  // 表征当前控制的位 从 1开始
uchar t = 0;//辅助 sel_xiaoshudian
uchar temp;//辅助 sel_xiaoshudian

uint num = 0;  // 计时器数秒计数


// 当前状态 0：初始状态，可设置时间，默认1小时； 1：倒计时状态  2：完成状态
uint state = 0;

// 蜂鸣器引脚
sbit beep_io = P3^4;
//蜂鸣器方波频率辅助
uchar pp = 0;
uint aa = 0;
bit close;

// 结构体变量 time， 默认1小时
Time time = {
    1,  // hour   小时       范围：0~3 
    0,  // min_g  分钟高位    范围 0~5
    0,  // min_d  分钟低位    范围 0~9
    0,  // sec_g  秒高位      范围 0~5
    0,  // sec_d; 秒低位      范围 0~9
};

// 用于恢复时间
//Time save_time = time;

/****** 函数声明 *******/
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


// 函数名：delay
// 参数：
// 功能：基础延时 !用于数码管!
void delay(int n)				//延时函数
{
	while(n--);
}

/*******************************
 * 函数名：Delayms
 * 描述  ：毫秒延时程序
 * 输入  ：延时i毫秒
 * 输出  ：无
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

// 函数名：init
// 参数：
// 功能：设置推挽输出 | 开启外部中断 | 设置电路状态为 0
void init()
{
    state = 0; // 设置状态为初始化
    P2M0=0xff;					//设置数码管/二极管推挽输出
   	P2M1=0x00;
    P0M0=0xff;
    P0M1=0x00;
    
    EA = 1;     //打开外部中断
    EX0 = 1;
    EX1 = 1;
    sel_xiaoshudian = 1; // 设置好选择位
    xiaoshudian[0] = 0x80; // 小数点显示第一个
    xiaoshudian[4] =xiaoshudian[3] =xiaoshudian[2] =xiaoshudian[1] = 0x00; // 小数点显示第一个
    
    P3M1 = 0x00;
    P3M0 = 0x10;                  //设置P3^4为推挽模式
    beep_io = 0;                  // 保护蜂鸣器
    //P0 = 0x00;                    //关闭P0端口
    resetTime();
    closeTimer0();  // 关 定时器0 初始状态不需要定时器
    
     ET1=0;   //关定时器中断
    TR1=0;   //关定时器1
}

// 以下几个是有关导航按键的辅助函数
//==================================================== 开始 ==========================================================================
/**********************************
 * 函数名：Init_ADC
 * 描述  ：初始化P1.7口为ADC
 * 输入  ：无
 * 输出  ：无
 **********************************/
void init_ADC()
{
	P1ASF=P1_7_ADC;//将P1ASF寄存器对应位置1
	ADC_RES = 0;//结果寄存器清零
//	ADC_RESL=0;
	ADC_CONTR = ADC_POWER | ADC_FLAG | ADC_START | ADC_SPEED_90 | ADC_CHS1_7;		//对应位赋值
	Delayms(2);
}

/**************************************
 * 函数名：GetADC
 * 描述  ：获得AD转换的值,没有设置A/D转换中断（具体看IE、IP）
 * 输入  ：无
 * 输出  ：AD转换的结果
 **************************************/
unsigned char GetADC()
{
	unsigned char result;
	ADC_CONTR = ADC_POWER | ADC_START | ADC_SPEED_90 | ADC_CHS1_7;//没有将ADC_FLAG置1，用于判断A/D是否结束
	_nop_();
	_nop_();
	_nop_();
	_nop_();
	while(!(ADC_CONTR&ADC_FLAG));//等待直到A/D转换结束
	ADC_CONTR &= ~ADC_FLAG; //ADC_FLAGE软件清0
	result = ADC_RES; //获取AD的值
    //Delayms(500);
	return result;	  //返回ADC值
}

/********************************
 * 函数名：Fun_Keycheck()	   
 * 描述  ：检测功能键的上5、下2、左4、右1、确认键3、开关按键3（0），没按下返回0x07，返回相应的值  (包含消抖过程)
 * 输入  ：无
 * 输出  ：键对应的值
********************************/
unsigned char keyCheck()
{
	unsigned char key;
	key=GetADC();		  //获得ADC值赋值给key
	if(key!=255)
	{
		Delayms(10);
		key=GetADC();
		if(key!=255)	  //按键消抖
		{
	     	key=key&0xE0;//获取高3位，其他位清零
        	key=_cror_(key,5);//循环右移5位
			return key;
		}
	}
	return 0x07;
}


/**********************
函数名称：Fun_Key_task_HMS
功能描述：监听功能键，完成时分秒相关值的设置， 
入口参数：无 
出口参数：无
***********************/
uchar prev_key_val = 0x07;
uchar key_val = 0x07;
void Fun_Key_task_HMS()
{
    prev_key_val = key_val;
    //  上 0x5 下 0x2 左4 右 1
    key_val = keyCheck();
    if(key_val == 0x07)
    switch(prev_key_val)
    {
        // 上 当前对应位的值+1
        case 0x5:   // 分钟或秒的低位
            if(sel_xiaoshudian == 1 ) time.sec_d = (time.sec_d + 1) % 10;
            if(sel_xiaoshudian == 3 ) time.min_d = (time.min_d + 1) % 10;
        // 分钟或者秒的高位
            if(sel_xiaoshudian == 2) time.sec_g = (time.sec_g + 1) % 6;
            if(sel_xiaoshudian == 4) time.min_g = (time.min_g +1) % 6;
            // 小时
            if(sel_xiaoshudian == 5)
            time.hour = (time.hour+1) % 4;  
            break;
            
         case 0x2:   // 下，对应的值减一
             //秒低位
            if(sel_xiaoshudian == 1 )
            {
                if(time.sec_d == 0) time.sec_d = 9;
                else time.sec_d--;
            }
            //秒高位
            if(sel_xiaoshudian == 2 )
            {
                if(time.sec_g == 0) time.sec_g = 5;
                else time.sec_g--;
            }
            //分钟低位
            if(sel_xiaoshudian == 3 )
            {
                if(time.min_d == 0) time.min_d = 9;
                else time.min_d--;
            }
            //分钟高位
            if(sel_xiaoshudian == 4)
            {
                if(time.min_g == 0) time.min_g = 5;
                else time.min_g--;
            }
            //小时
            if(sel_xiaoshudian == 5 )
            {
                if(time.hour == 0) time.hour = 3;
                else time.hour--;
            }
            break;
//          case 0x4:   // 左，小数点循环左移
//              xiaoshudian[sel_xiaoshudian - 1] = 0x00; //取消之前位小数点的显示
//              sel_xiaoshudian++;
//              if(sel_xiaoshudian == 6) sel_xiaoshudian = 1;
//              xiaoshudian[sel_xiaoshudian - 1] = 0x80; // 开启当前位小数点的显示
         case 0x1:  // 右，小数点循环右移
             xiaoshudian[sel_xiaoshudian - 1] = 0x00; //取消之前位小数点的显示
             sel_xiaoshudian--;
             if(sel_xiaoshudian == 0) sel_xiaoshudian = 5;
              xiaoshudian[sel_xiaoshudian - 1] = 0x80; // 开启当前位小数点的显示
    }
    prev_key_val = 0x07;
 }


//==================================================== 结束 ==========================================================================



// 函数名：init_action()
// 参数：
// 功能：指定电路状态为初始化 0 时电路的行为: 显示 01-00-00，时间默认为一小时可调节
void init_action()
{   
    light_led(0x80); // 指示灯
    Fun_Key_task_HMS(); //检测导航按键 
    assignTime();
}

// 函数名：timing_action
// 参数：
// 功能：指定电路状态为倒计时状态 1 时电路的行为：从指定的时间开始倒计时
void timing_action()
{
    xiaoshudian[sel_xiaoshudian - 1] = 0x00;
    assignTime(); // 设置时间显示
    light_led(0x40);
}

//函数名：beep_action
//参数：
//功能：指定电路状态为发声（计时结束）2
void beep_action()
{
    num++;
    assignFinish(); // 设置时间显示
    light_led(0x20);
    if(pp == 100)
    {
        pp = 0;
        if(!close) beep_io = ~beep_io;   //频率为 1KHz 的音频信号
        
        aa++;
    }
    if(aa == 200)
    {
        beep_io = 1;
        close = 1;   // 方波信号持续时间 200*10*10us = 0.2s
    }
    if(aa == 300) //滴滴声周期为 300 * 10 * 10us = 0.3s
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


// 函数名 light_led
// 参数：n，n = 1,2,3, led 位数
// 功能：控制第 n 位 led 亮起，表征当前状态
void light_led(uint n)
{
    P23 = 1;    // 关闭 数码管，开启 led
    P0 = n;
    delay(200);
}

// 函数名：change0
// 参数：
// 功能：执行函数 init
void change0() interrupt 0
{
    init(); // 执行初始化
}

// 函数名：change1
// 参数：
// 功能：处理外部中断1 (KEY2)，开启计时
void change1() interrupt 2
{
    state = 1;
    TMOD=0x02;    //设置定时器0为工作方式2
    TH0=6;   //装入初值
    TL0=6;
    openTimer0(); // 开启定时器0中断
}

// 函数名：tim
// 参数：
// 功能：定时器中断函数，计算 1 s 时间，并进行操作


void tim() interrupt 1 using 1
{
       num++;
    if(state == 1) //  计时状态
    {
        if(num == 3800)  // 1秒，计算得 num==4000 根据实际情况有调整
       {
           num = 0;
           refreshTime(); // 每一秒更新一次时间
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

// 函数名：assignTime
// 参数：
// 功能：设置最高位为0 | 根据 time 为数码管其他位赋值 | 判断计时是否结束
// 注意：assignTime 并没有修改 time 的内容
void assignTime()
{
    if(isEnd() && state == 1)// 时间显示为 0 且当前是计时状态
    {
        state = 2;// 计时完成状态
        //closeTimer0();
        sel_xiaoshudian = 1;
        
         num = 0;//重置计数值 准备 beep_action
        TMOD = 0x01;
        TR0 = 1;
        TH0 = 0xff;
        TL0 = 0x9c;
        ET0 = 1;
        EA = 1;
//    
    }
    P0 = duanxuan[0];
    P2 = weixuan[0];    // 最高位显示 0 不变
    delay(600);
    for(i = 1; i < 8; i++)
    {
        P0=0;
        P2=weixuan[i];		//选择数码管的位数
        if(i==2 || i == 5)
            P0=duanxuan[10];	//数码管第2位和第5位，显示横线
        //其他位，根据 time 的值进行赋值
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

// 函数名：assignFinish
// 参数：
// 功能： 显示 END
static void assignFinish()
{
    //0~7显示
    for(i = 0; i < 3; i++)
    {
        P0=0;
        P2=weixuan[i];		//选择数码管的位数
        
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
// 函数名 resetTime
// 参数
// 功能 重置时间
static void resetTime()
{
    time.hour = 1;
    time.min_g = 0;
    time.min_d = 0;
    time.sec_g = 0;
    time.sec_d = 0;
}

// 函数名：refreshTime
// 参数：
// 功能：更新当前显示的时间
// 注意：不用考虑计时结束时的边界情况(全为0)，在 assignTime 函数中有处理
void refreshTime()
{
    // 此时为整点，之后小时数减一（跨小时）
    if(time.sec_d + time.sec_g + time.min_g + time.min_d == 0)
    {
        time.hour--;
        time.min_g = 5;
        time.min_d = 9;
        time.sec_g = 5;
        time.sec_d = 9;
        return ;
    }
    // 非整点
    //跨分钟
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

// 函数名：isEnd
// 参数：
// 功能：判断计时是否结束
int isEnd()
{
    return !(time.hour + time.min_g + time.min_d + time.sec_g + time.sec_d);
}

// 函数名：openTimer
// 参数：
// 功能：开定时器中断
static void openTimer0()
{
    ET0=1;   //开定时器中断
    TR0=1;   //启动定时器0 
}

//函数名：closeTimer
//参数：
//功能：关定时器中断
static void closeTimer0()
{
    ET0=0;   //关定时器中断
    TR0=0;   //关定时器0 
}


//主函数
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
