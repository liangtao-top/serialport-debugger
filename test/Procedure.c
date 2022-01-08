
#include"p33FJ256MC710.h"
#include"User.h"
#include"Spi.h"
#include"Init.h"
#include"Procedure.h"
#include"TouchHandle.h"
#include"LcdScreen.h"
#include"math.h"
#include"BarCode.h"
#include"String.h"
#include "SettingPic.h"
#include"Lis.h"
#include"HistoryResult.h"

//#include <time.h>
extern unsigned long int g_uliPulseValue,g_uliADValue;

extern void UpdateProcessBar(unsigned char ucFlag);
extern struct TestPictureAddr g_tTestPictureAddr;
struct ReagentTrayMotor   g_tReagentTrayMotor;
struct TipHorizontalMotor g_tTipHorizontalMotor;
struct TipVerticalMotor   g_tTipVerticalMotor;
struct PistonMotor        g_tPistonMotor;
unsigned int g_uiPmtVerticalTestPosition=0;

struct RunProcessFile   g_tRunProcessFile[12],g_tProcessFile;
struct ProcessControl   g_tProcessControl;
struct PmtFitValue      g_tPmtFitValue;

unsigned char g_ucUartLcdRxData[20],g_ucUartLcdTxData[20],g_ucUartLcdRecvFlag=0;

unsigned char g_ucStartButtonLock = 0;
unsigned char g_ucNextPositionButtonLock=0;
unsigned char g_ucNextPositionFlag=0;

unsigned int g_uliBottomNoiseH,g_uliBottomNoiseL,g_uliBottomNoiseCMP;
unsigned  int g_tProcessTimeFlag = 0,g_tProcessTimeFlag1 = 0;

/*******************************************************************************
 * 函数名称：WriteWasteTipsNum
 * 功能描述：记录废料桶中Tip的数量
 * 输入参数：无
 * 输出参数：无
 * 返回值  ：误
 * 修改日期            版本号            修改人            修改内容
 * -----------------------------------------------------------------------------
 * 2017/09/19          V1.0             周志强               创建
 *******************************************************************************/
void WriteWasteTipsNum(void)
{
    //unsigned long int uliAddr = (0xB5 - 0x80) * 4;
    unsigned long int uliAddr = 112001024;
    FlashWrite4BytesValue(uliAddr,(unsigned long int)g_uiTipCounterNum);
}

/*******************************************************************************
 * 函数名称：IsExpireDate
 * 功能描述：判断date的日期是否超过了当前日期
 * 输入参数：date 输入的日期
 * 输出参数：无
 * 返回值  ：1 未过期  0 已过期
 * 修改日期            版本号            修改人            修改内容
 * -----------------------------------------------------------------------------
 * 2017/09/20          V1.0             周志强               创建
 *******************************************************************************/
static unsigned char IsExpireDate(unsigned char *date)
{
    unsigned char ucYear1=0,ucMonth1=0,ucDay1=0;
    unsigned char ucYear2=0,ucMonth2=0,ucDay2=0;
    unsigned char ucTemp[6];
    GetSystemTime(ucTemp);
    ucYear1  = date[0];
    ucMonth1 = date[1];
    ucDay1   = date[2];
    ucYear2  = (ucTemp[0]/16)*10 + ucTemp[0]%16;
    ucMonth2 = (ucTemp[1]/16)*10 + ucTemp[1]%16;
    ucDay2   = (ucTemp[2]/16)*10 + ucTemp[2]%16;
    if((ucYear1<ucYear2) || ((ucYear1==ucYear2)&&(ucMonth1<ucMonth2)) || ((ucYear1==ucYear2)&&(ucMonth1==ucMonth2)&&(ucDay1<=ucDay2)))
    {
        return 0;
    }
    else
    {
        return 1;
    }
}
/*******************************************************************************
 * 函数名称：IsPlaceKit
 * 功能描述：检测是否放置了试剂盒
 * 输入参数：num:检测的通道号
 * 输出参数：无
 * 返回值  ：ucPlaceKitTipFlag: 后三位(2、1、0)分别表示TIP2 TIP1 试剂盒 0表示未放置 1表示已放置  
 * 修改日期            版本号            修改人            修改内容
 * -----------------------------------------------------------------------------
 * 2017/09/05          V1.0             周志强               创建
 *******************************************************************************/
//static unsigned char IsPlaceKit(unsigned char num)
unsigned char IsPlaceKit(unsigned char num)
{
    #define DELAY_TIME  200
    g_tTestInfo[num].ucPlaceKitTipFlag = 0;
    //检查是否放置了Tip1
    SPIMotor(4,MT_MOVE_TO,g_tReagentTrayMotor.uiaIsPlaceTip1SensorPosition[num]);
    Delay1ms(DELAY_TIME);
    if(KIT_SENSOR == 0)
    {
        g_tTestInfo[num].ucPlaceKitTipFlag |= 0x02; 
    }
    else
    {
        g_tTestInfo[num].ucPlaceKitTipFlag &= 0xFD;
    }
    //检查是否放置了Tip2
    SPIMotor(4,MT_MOVE_TO,g_tReagentTrayMotor.uiaIsPlaceTip2SensorPosition[num]);
    Delay1ms(DELAY_TIME);
    if(KIT_SENSOR == 0)
    {
        g_tTestInfo[num].ucPlaceKitTipFlag |= 0x04; 
    }
    else
    {
        g_tTestInfo[num].ucPlaceKitTipFlag &= 0xFB;
    }
    //检查是否放置了试剂盒
    SPIMotor(4,MT_MOVE_TO,g_tReagentTrayMotor.uiIsPlaceKitSensorPosition[num]);
    Delay1ms(DELAY_TIME);
    if(KIT_SENSOR == 0) //放置了试剂盒
    {
        g_tTestInfo[num].ucPlaceKitTipFlag |= 0x01;
    }
    else
    {
        g_tTestInfo[num].ucPlaceKitTipFlag &= 0xFE;
    }
    return g_tTestInfo[num].ucPlaceKitTipFlag;
}

/*******************************************************************************
 * 函数名称：IsChannelsSetOk
 * 功能描述：检查每个通道是否设置完成
 * 输入参数：无
 * 输出参数：无
 * 返回值  ：1:至少设置了一个通道  2:返回继续设置
 * 修改日期            版本号            修改人            修改内容
 * -----------------------------------------------------------------------------
 * 2017/03/28          V1.0             周志强               创建
 *******************************************************************************/
static unsigned char IsChannelsSetOk(void)
{
    unsigned char i=0,num=0;
    unsigned char ucNum=0;
    unsigned char ucTemp[30];
    unsigned char ucString1[] = "以下通道未找到标准曲线信息：";
    unsigned char ucString2[] = "以下通道未放置试剂条：";
    unsigned char ucString3[] = "以下通道未放置第一Tip：";
    unsigned char ucString4[] = "以下通道未放置第二Tip：";
    unsigned char ucString5[] = "以下通道试剂条已经过期：";
    unsigned char ucAddrNum=0;
    unsigned int uiAddr = 0x04A0;
    unsigned char IsPlaceKitFlag=0;
    if(g_tProcessControl.ucStopFlag == 1)   //用于运行正常流程的停止测试操作
        return 1;
    for(i=0;i<12;i++)
    {
        g_tTestInfo[i].ucSetOkFlag = 0;
        if(g_tTestInfo[i].ucName[0] != '\0')
        {
            if(0 == FindReadReagentDetailInfo(i))   //获取成功
            {
                num++;  
            }
            else
            {
                ucNum++;
            }
        }
    }
    for(i=0;i<12;i++)
    {
        if(g_tTestInfo[i].ucName[0] != '\0')
        {
            if (num >= 1)
            {
                //if(7 == IsPlaceKit(i) && 1 == IsExpireDate(g_tTestInfo[i].ucExpiryDate))
                IsPlaceKitFlag = IsPlaceKit(i);
                if(((((IsPlaceKitFlag&0x06) == 6)&&(g_tSystemSetting.ucInputSampleIDMethod == 1))||(((IsPlaceKitFlag&0x07) == 7)&&(g_tSystemSetting.ucInputSampleIDMethod == 0))) && (1 == IsExpireDate(g_tTestInfo[i].ucExpiryDate)))
                {
                    g_tTestInfo[i].ucSetOkFlag = 1;
                }
                else
                {
                    ucNum++;
                }
                IsPlaceKitFlag = 0;
            }
        }
    }
    if(g_tProcessControl.ucStopFlag == 1)   //用于运行正常流程的停止测试操作
        return 1;
    if(ucNum > 0)
    {
        InitTimer5(0);  //关闭流程计时的定时器
        ucNum = 0;
        memset(ucTemp,'\0',sizeof(ucTemp));
        DisplayPicture(34);
        Delay1ms(200);//延迟500ms     
        for(i=0;i<10;i++)
        {
            DisplayText(uiAddr + 0x0010 * i,'\0');
        }
        //是否读取了标准曲线信息
        for(i=0;i<12;i++)
        {
            if((g_tTestInfo[i].ucName[0] != '\0') && (g_tTestInfo[i].ucReagentBarFlag == 0))
            {
                if((i+1)<10)
                {
                    ucTemp[ucNum++] = (i+1) + 0x30;
                }
                else
                {
                    ucTemp[ucNum++] = (i+1)/10 + 0x30;
                    ucTemp[ucNum++] = (i+1)%10 + 0x30;
                }
                ucTemp[ucNum++] = ',';
            }
        }
        if(ucNum >= 1)
        {
            ucTemp[ucNum-1] = '\0';
            DisplayText(uiAddr + 0x0010 * ucAddrNum,ucString1);
            ucAddrNum++;
            DisplayText(uiAddr + 0x0010 * ucAddrNum,ucTemp);
            ucAddrNum++;
        }
        if(num >= 1)
        {
            //是否放置了试剂盒
            ucNum = 0;
            memset(ucTemp,'\0',sizeof(ucTemp));
            for(i=0;i<12;i++)
            {
                if((g_tTestInfo[i].ucName[0] != '\0') && ((g_tTestInfo[i].ucPlaceKitTipFlag&0x01) == 0) && (g_tSystemSetting.ucInputSampleIDMethod == 0))//手动扫描检测试剂条
                {
                    if((i+1)<10)
                    {
                        ucTemp[ucNum++] = (i+1) + 0x30;
                    }
                    else
                    {
                        ucTemp[ucNum++] = (i+1)/10 + 0x30;
                        ucTemp[ucNum++] = (i+1)%10 + 0x30;
                    }
                    ucTemp[ucNum++] = ',';
                }
            }
            if(ucNum >= 1)
            {
                ucTemp[ucNum-1] = '\0';
                DisplayText(uiAddr + 0x0010 * ucAddrNum,ucString2);
                ucAddrNum++;
                DisplayText(uiAddr + 0x0010 * ucAddrNum,ucTemp);
                ucAddrNum++;
            }

            //是否放置了Tip1
            ucNum = 0;
            memset(ucTemp,'\0',sizeof(ucTemp));
            for(i=0;i<12;i++)
            {
                if((g_tTestInfo[i].ucName[0] != '\0') && ((g_tTestInfo[i].ucPlaceKitTipFlag&0x02) == 0))
                {
                    if((i+1)<10)
                    {
                        ucTemp[ucNum++] = (i+1) + 0x30;
                    }
                    else
                    {
                        ucTemp[ucNum++] = (i+1)/10 + 0x30;
                        ucTemp[ucNum++] = (i+1)%10 + 0x30;
                    }
                    ucTemp[ucNum++] = ',';
                }
            }
            if(ucNum >= 1)
            {
                ucTemp[ucNum-1] = '\0';
                DisplayText(uiAddr + 0x0010 * ucAddrNum,ucString3);
                ucAddrNum++;
                DisplayText(uiAddr + 0x0010 * ucAddrNum,ucTemp);
                ucAddrNum++;
            }

            //是否放置了Tip2
            ucNum = 0;
            memset(ucTemp,'\0',sizeof(ucTemp));
            for(i=0;i<12;i++)
            {
                if((g_tTestInfo[i].ucName[0] != '\0') && ((g_tTestInfo[i].ucPlaceKitTipFlag&0x04) == 0))
                {
                    if((i+1)<10)
                    {
                        ucTemp[ucNum++] = (i+1) + 0x30;
                    }
                    else
                    {
                        ucTemp[ucNum++] = (i+1)/10 + 0x30;
                        ucTemp[ucNum++] = (i+1)%10 + 0x30;
                    }
                    ucTemp[ucNum++] = ',';
                }
            }
            if(ucNum >= 1)
            {
                ucTemp[ucNum-1] = '\0';
                DisplayText(uiAddr + 0x0010 * ucAddrNum,ucString4);
                ucAddrNum++;
                DisplayText(uiAddr + 0x0010 * ucAddrNum,ucTemp);
                ucAddrNum++;
            }

            //试剂信息是否过期
            ucNum = 0;
            memset(ucTemp,'\0',sizeof(ucTemp));
            for(i=0;i<12;i++)
            {
                if(g_tTestInfo[i].ucName[0] != '\0')
                {
                    if(0 == IsExpireDate(g_tTestInfo[i].ucExpiryDate))  //表示已经过期
                    {
                        if((i+1)<10)
                        {
                            ucTemp[ucNum++] = (i+1) + 0x30;
                        }
                        else
                        {
                            ucTemp[ucNum++] = (i+1)/10 + 0x30;
                            ucTemp[ucNum++] = (i+1)%10 + 0x30;
                        }
                        ucTemp[ucNum++] = ',';
                    }
                }
            }
            if(ucNum >= 1)
            {
                ucTemp[ucNum-1] = '\0';
                DisplayText(uiAddr + 0x0010 * ucAddrNum,ucString5);
                ucAddrNum++;
                DisplayText(uiAddr + 0x0010 * ucAddrNum,ucTemp);
                ucAddrNum++;
            }
        }
        while(g_ucUartLcdRecvFlag == 0);
        g_ucUartLcdRecvFlag = 0;
        AnalyticInstruction();
        if((g_tLcdAnalyticIns.uiLcdParamAddr == 0x0006) && (g_tLcdAnalyticIns.uiButtonValue == 1))  //返回
        {
            //DisplayPicture(g_ucPrevousPlusPic);//
            g_tLcdAnalyticIns.uiPageNum = 1;
            DisplayPicture(TEST_PICTURE1);
            SPIMotor(4,MT_HOME,0);      //试剂盘电机复位
            return 2;   //返回继续设置
        }
    }                    
    return 1;   //至少设置了一个通道
}

void OutDAC7512(unsigned int PMTCtrl)
{
	unsigned char i;
	DACSYNC=1;
	DACCLK=1;
    asm("nop");//nop();
    asm("nop");//nop();
    asm("nop");//nop();
    asm("nop");//nop();
	DACSYNC=0;
    asm("nop");//nop();
    asm("nop");//nop();
	for(i=0;i<16;i++)
    {
		DACDI=(PMTCtrl & 0x8000) ? 1 : 0;
		PMTCtrl <<=1;

		asm("nop");//nop();
		asm("nop");//nop();
		asm("nop");//nop();
		DACCLK=1;
		asm("nop");//nop();
		asm("nop");//nop();
		asm("nop");//nop();
		asm("nop");//nop();
		asm("nop");//nop();
		asm("nop");//nop();
		DACCLK=0;
		asm("nop");//nop();
		asm("nop");//nop();
		asm("nop");//nop();
		asm("nop");//nop();
		asm("nop");//nop();
		asm("nop");//nop();
	}
	asm("nop");//nop();
    asm("nop");//nop();
    asm("nop");//nop();
	DACSYNC=1;
}

/*******************************************************************************
 * 函数名称：TakeTipProcess
 * 功能描述：取Tip头流程
 * 输入参数：无
 * 输出参数：无
 * 返回值  ：无
 * 修改日期            版本号            修改人            修改内容
 * -----------------------------------------------------------------------------
 * 2017/03/29          V1.0             周志强               创建
 *******************************************************************************/
unsigned int GetAD(void)  //采集1次10us
{
	AD1CHS0bits.CH0SA = 11;
	AD1CON1bits.ADON = 1;				//打开AD转换
	AD1CON1bits.SAMP = 1;				//启动A/D采样
	while (!AD1CON1bits.DONE);			//等待AD转换结束
	return (ADC1BUF0);
}

void GetPmtData(void)
{
    unsigned int m=0,k=0,i=0;
    g_uliPulseValue = 0;
    g_uliADValue    = 0;
    InitTimer1(1);
    for(k=0;k<500;k++)  //采集500ms
    {
        for(m=0;m<16;m++)
        {
            g_uliADValue += GetAD();
        }
        for(i=0;i<7358;i++);
    }
    g_uliPulseValue = g_uliPulseValue * 65536 + TMR1;
    InitTimer1(0);
    
    if((g_uliPulseValue > g_uliBottomNoiseCMP))
    {
        g_uliPulseValue = g_uliPulseValue - g_uliBottomNoiseH;
    }
    else if((g_uliPulseValue > g_uliBottomNoiseL) && (g_uliPulseValue <= g_uliBottomNoiseCMP))
    {
        g_uliPulseValue = g_uliPulseValue - g_uliBottomNoiseL;
    }
    
}

/*******************************************************************************
 * 函数名称：LogisticOperation
 * 功能描述：根据四参数公式计算Value浓度对应的信号值
 * 输入参数：Param 四参数的四个系数  Value 浓度值
 * 输出参数：无
 * 返回值  ：信号值
 * 修改日期            版本号            修改人            修改内容
 * -----------------------------------------------------------------------------
 * 2017/12/07          V1.0             周志强               创建
 *******************************************************************************/
static unsigned long int LogisticOperation(float *Param,float Value)
{
    float fA=0,fB=0,fC=0,fD=0;
    float fTemp = 0;
    unsigned long int uliValue = 0;
    fA = Param[0];
    fB = Param[1];
    fC = Param[2];
    fD = Param[3];
    //四参数公式  y=(a-d)/(1+(x/c)^b) + d;
    fTemp = Value/fC;
    fTemp = pow(fTemp,fB) + 1.0;
    fTemp = (fA - fD)/fTemp + fD;
    
    uliValue = (unsigned long int)(fTemp + 0.5);
    return uliValue;
}
/*******************************************************************************
 * 函数名称：LogisticOperation1
 * 功能描述：根据Logistic计算Value浓度对应的信号值
 * 输入参数：Param 四参数的四个系数  Value 浓度值
 * 输出参数：无
 * 返回值  ：信号值
 * 修改日期            版本号            修改人            修改内容
 * -----------------------------------------------------------------------------
 * 2017/12/07          V1.0             周志强               创建
 *******************************************************************************/
static unsigned long int LogisticOperation1(float *Param,float Value)
{
    float fA=0,fB=0,fC=0,fD=0;
    float fTemp = 0;
    unsigned long int uliValue = 0;
    fA = Param[0];
    fB = Param[1];
    fC = Param[2];
    fD = Param[3];
    //Logistic  y=a+(b-a)/[1+e^(-c*x+d)]]
    fTemp = fD-fC*Value;
    fTemp = exp(fTemp) + 1.0;
    fTemp = (fB - fA)/fTemp + fA;
    
    uliValue = (unsigned long int)(fTemp + 0.5);
    return uliValue;
}
/*******************************************************************************
 * 函数名称：LogisticInverseOperation
 * 功能描述：根据信号值利用四参数计算浓度值
 * 输入参数：Param 四参数的四个系数  Value 信号值
 * 输出参数：无
 * 返回值  ：计算得到的浓度值
 * 修改日期            版本号            修改人            修改内容
 * -----------------------------------------------------------------------------
 * 2017/09/10          V1.0             周志强               创建
 *******************************************************************************/
//四参数的逆运算，即由信号值计算浓度
//Param为四参数的系数，Value为信号值
//四参数公式  y=(a-d)/(1+(x/c)^b) + d;
//所以: x=c*(((a-d)/y-d -1)^(1/b));
//当Value<fD或者Value>fA时，返回-1
//返回值：计算的浓度值
static float LogisticInverseOperation(float *Param,unsigned long int Value)
{
    float fA=0,fB=0,fC=0,fD=0;
    float fValue=0;
    
    fA = Param[0];
    fB = Param[1];
    fC = Param[2];
    fD = Param[3];
    
    if(Value <= fD || (Value >= fA))
    {
        return -1;
    }
    else
    {
        fValue = (fA - fD)/(Value - fD)-1;
        fValue = fC * pow(fValue,1/fB);
    }
    return fValue;
}
/*******************************************************************************
 * 函数名称：LogisticInverseOperation1
 * 功能描述：根据信号值利用四参数计算浓度值
 * 输入参数：Param 四参数的四个系数  Value 信号值
 * 输出参数：无
 * 返回值  ：计算得到的浓度值
 * 修改日期            版本号            修改人            修改内容
 * -----------------------------------------------------------------------------
 * 2017/09/10          V1.0             周志强               创建
 *******************************************************************************/
//四参数的逆运算，即由信号值计算浓度
//Param为四参数的系数，Value为信号值
//四参数公式  y=(a-d)/(1+(x/c)^b) + d;
//所以: x=c*(((a-d)/y-d -1)^(1/b));
//当Value<fD或者Value>fA时，返回-1
//返回值：计算的浓度值
static float LogisticInverseOperation1(float *Param,unsigned long int Value)
{
    float fA=0,fB=0,fC=0,fD=0;
    float fValue=0;
    
    fA = Param[0];
    fB = Param[1];
    fC = Param[2];
    fD = Param[3];
    
    //if(Value <= fD || (Value >= fA))
    //{
    //    return -1;
    //}
    //else
    //{
        fValue = (fB - fA)/(Value - fA)-1;
        fValue = fD-log(fValue);
        fValue = fValue/fC;
    //}
    return fValue;
}
/*******************************************************************************
 * 函数名称：LineOperation
 * 功能描述：根据直线的k,b值计算浓度
 * 输入参数：Param 四参数的四个系数 Param[2]为k值 Param[3]为b值  Value 浓度值
 * 输出参数：无
 * 返回值  ：计算得到的信号值
 * 修改日期            版本号            修改人            修改内容
 * -----------------------------------------------------------------------------
 * 2017/12/07          V1.0             周志强               创建
 *******************************************************************************/
static unsigned long int LineOperation(float *Param,float Value)
{
    unsigned long int uliValue = 0;
    uliValue = Param[2] * Value + Param[3];
    return uliValue;
}

/*******************************************************************************
 * 函数名称：LineInverseOperation
 * 功能描述：根据直线的k,b值计算浓度
 * 输入参数：Param 四参数的四个系数  Value 信号值
 * 输出参数：无
 * 返回值  ：计算得到的浓度值
 * 修改日期            版本号            修改人            修改内容
 * -----------------------------------------------------------------------------
 * 2017/09/10          V1.0             周志强               创建
 *******************************************************************************/
static float LineInverseOperation(float *Param,unsigned long int Value)
{
    float fValue=0;
    fValue = ((float)Value -Param[3])/Param[2];
    return fValue;
}

/*******************************************************************************
 * 函数名称：CalculateSignalValue
 * 功能描述：根据传入的系数Param来计算浓度对应的信号值，如果Param[0]不等于0，则代表四参数，
 *           否则为直线，其中Param[2]代表k,Param[3]代表b  
 * 输入参数：Param 四参数的四个系数  fValue 浓度值
 * 输出参数：无
 * 返回值  ：计算得到的信号值
 * 修改日期            版本号            修改人            修改内容
 * -----------------------------------------------------------------------------
 * 2017/12/07          V1.0             周志强               创建
 *******************************************************************************/
static unsigned long int CalculateSignalValue(float *Param,float Value)
{
    unsigned long int uliValue = 0;
    if(Param[0] == 0)
    {
        uliValue = LineOperation(Param,Value);
    }
    else if(Param[0] > 0)
    {
        uliValue = LogisticOperation(Param,Value);
    }
    else 
    {
        uliValue = LogisticOperation1(Param,Value);
    }
    return uliValue;
}

/*******************************************************************************
 * 函数名称：CalculateConcentration
 * 功能描述：根据传入的系数Param来计算浓度，如果Param[0]不等于0，则代表四参数，
 *           否则为直线，其中Param[2]代表k,Param[3]代表b  
 * 输入参数：Param 四参数的四个系数  Value 信号值
 * 输出参数：无
 * 返回值  ：计算得到的浓度值
 * 修改日期            版本号            修改人            修改内容
 * -----------------------------------------------------------------------------
 * 2017/09/10          V1.0             周志强               创建
 *******************************************************************************/
static float CalculateConcentration(float *Param,unsigned long int Value)
{
    float fValue = 0;
    if(Param[0] == 0)
    {
        fValue = LineInverseOperation(Param,Value);
        return fValue;
    }
    else if (Param[0] > 0)
    {
        fValue = LogisticInverseOperation(Param,Value);
        return fValue;
    }
    else 
    {
        fValue = LogisticInverseOperation1(Param,Value);
        return fValue;
    }
}
/*******************************************************************************
 * 函数名称：LcdCalculateConcentration
 * 功能描述：计算浓度
 * 输入参数：num 通道号
 * 输出参数：无
 * 返回值  ：无
 * 修改日期            版本号            修改人            修改内容
 * -----------------------------------------------------------------------------
 * 2017/09/10          V1.0             周志强               创建
 *******************************************************************************/
/* uliTransitionPeriodL1为较大值 uliTransitionPeriodH1为0表示有一条标准曲线
 * uliTransitionPeriodL1不为0，uliTransitionPeriodH1不为0，uliTransitionPeriodL2为较大值，uliTransitionPeriodH2为0 表示有两条标准曲线
 * uliTransitionPeriodL1、uliTransitionPeriodH1、uliTransitionPeriodL2、uliTransitionPeriodH2均不为0表示有三条标准曲线
 * 四个参数Param[0]不等于0，则代表四参数，否则为直线，其中Param[2]代表k,Param[3]代表b
 */
static void LcdCalculateConcentration(unsigned char num)
{
    unsigned char i=0;
    unsigned long int uliTransitionPeriodL1=0,uliTransitionPeriodL2=0,uliTransitionPeriodH1=0,uliTransitionPeriodH2=0;
    unsigned long int uliLowValue = 0,uliHighValue = 0;  //最低和最高检测浓度对应的信号值
    float fParam1[4],fParam2[4],fParam3[4];
    float fValue1=0,fValue2=0,fRatio=0;
    unsigned char SystemErrorAdjustmentFlag=0;
    //1、根据样本类型分配不同的标准曲线，一共有三段标准曲线
    if((g_tTestInfo[num].ucSampleType == 1) || (g_tTestInfo[num].ucSampleType == 2) || (g_tTestInfo[num].ucSampleType == 4) || (g_tTestInfo[num].ucSampleType == 5))    //血清、者血浆或质控品
    {
        for(i=0;i<4;i++)
        {
            fParam1[i] = g_tStandCurve[num].fCurveParam1[i];
        }
        uliTransitionPeriodL1 = (unsigned long int)(g_tStandCurve[num].fCurveParam1[4] + 0.5);
        uliTransitionPeriodH1 = (unsigned long int)(g_tStandCurve[num].fCurveParam1[5] + 0.5);
        for(i=0;i<4;i++)
        {
            fParam2[i] = g_tStandCurve[num].fCurveParam1[6+i];
        }
        uliTransitionPeriodL2 = (unsigned long int)(g_tStandCurve[num].fCurveParam1[10] + 0.5);
        uliTransitionPeriodH2 = (unsigned long int)(g_tStandCurve[num].fCurveParam1[11] + 0.5);
        for(i=0;i<4;i++)
        {
            fParam3[i] = g_tStandCurve[num].fCurveParam1[12+i];
        }
    }
    else if(g_tTestInfo[num].ucSampleType == 3) //原血
    {
        for(i=0;i<4;i++)
        {
            fParam1[i] = g_tStandCurve[num].fCurveParam2[i];
        }
        uliTransitionPeriodL1 = (unsigned long int)(g_tStandCurve[num].fCurveParam2[4] + 0.5);
        uliTransitionPeriodH1 = (unsigned long int)(g_tStandCurve[num].fCurveParam2[5] + 0.5);
        for(i=0;i<4;i++)
        {
            fParam2[i] = g_tStandCurve[num].fCurveParam2[6+i];
        }
        uliTransitionPeriodL2 = (unsigned long int)(g_tStandCurve[num].fCurveParam2[10] + 0.5);
        uliTransitionPeriodH2 = (unsigned long int)(g_tStandCurve[num].fCurveParam2[11] + 0.5);
        for(i=0;i<4;i++)
        {
            fParam3[i] = g_tStandCurve[num].fCurveParam2[12+i];
        }
    }
    
    //2、根据标准曲线计算最低和最高检测浓度对应的信号值
    uliLowValue = CalculateSignalValue(fParam1,g_tStandCurve[num].fLowLimit);       //计算最低检测浓度对应的信号值
    if(uliTransitionPeriodH1 == 0)  //表示只有一条标准曲线
        uliHighValue = CalculateSignalValue(fParam1,g_tStandCurve[num].fHighLimit); //计算最高检测浓度对应的信号值
    else if(uliTransitionPeriodH2 == 0)  //表示只有两条标准曲线
        uliHighValue = CalculateSignalValue(fParam2,g_tStandCurve[num].fHighLimit); //计算最高检测浓度对应的信号值
    else    //表示有三条标准曲线
        uliHighValue = CalculateSignalValue(fParam3,g_tStandCurve[num].fHighLimit); //计算最高检测浓度对应的信号值
    
    //读取系统误差调整系数并处理参数
    memcpy(g_tSystemErrorAdjustment.ucName,g_tTestInfo[num].ucName,10);
    g_tSystemErrorAdjustment.ucName[10] = '\0';
    ReadSystemErrorAdjustment();
    if(0 == g_tSystemErrorAdjustment.ucKValueSymbol)
    {
        g_tSystemErrorAdjustment.ucKValue = 0 - g_tSystemErrorAdjustment.ucKValue;
    }
    if(0 == g_tSystemErrorAdjustment.ucBValueSymbol)
    {
        g_tSystemErrorAdjustment.ucBValue = 0 - g_tSystemErrorAdjustment.ucBValue;
    }
    //3、所采集的信号值和uliLowValue、uliHighValue进行比较，判断是否超出了检测的范围
    if(g_tTestInfo[num].uliFitValue < uliLowValue)
    {
        g_tTestInfo[num].uliFitValue = (unsigned long int)((float)g_tTestInfo[num].uliFitValue*g_tSystemErrorAdjustment.ucKValue + g_tSystemErrorAdjustment.ucBValue);
        SystemErrorAdjustmentFlag=1;
        if(g_tTestInfo[num].uliFitValue < uliLowValue)
        {
            g_tTestInfo[num].ucResultFlag = 2;  //小于
            g_tTestInfo[num].fResult = g_tStandCurve[num].fLowLimit;
            return;
        }
    }
    else if(g_tTestInfo[num].uliFitValue > uliHighValue)
    {
        g_tTestInfo[num].uliFitValue = (unsigned long int)((float)g_tTestInfo[num].uliFitValue*g_tSystemErrorAdjustment.ucKValue + g_tSystemErrorAdjustment.ucBValue);
        SystemErrorAdjustmentFlag=1;
        if(g_tTestInfo[num].uliFitValue > uliHighValue)
        {
            g_tTestInfo[num].ucResultFlag = 3;  //大于
            g_tTestInfo[num].fResult = g_tStandCurve[num].fHighLimit;
            return;
        }
    }

    //3、在检测浓度范围内，计算浓度
    if(g_tTestInfo[num].uliFitValue <= uliTransitionPeriodL1)
    {
        fValue1 = CalculateConcentration(fParam1,g_tTestInfo[num].uliFitValue);
    }
    else if((g_tTestInfo[num].uliFitValue > uliTransitionPeriodL1) && (g_tTestInfo[num].uliFitValue < uliTransitionPeriodH1))
    {
        fValue1 = CalculateConcentration(fParam1,g_tTestInfo[num].uliFitValue);
        fValue2 = CalculateConcentration(fParam2,g_tTestInfo[num].uliFitValue);
        fRatio = ((float)(g_tTestInfo[num].uliFitValue - uliTransitionPeriodL1))/(uliTransitionPeriodH1 - uliTransitionPeriodL1);
        fValue1 = (1-fRatio) * fValue1 + fRatio * fValue2;
    }
    else if((g_tTestInfo[num].uliFitValue >= uliTransitionPeriodH1) && (g_tTestInfo[num].uliFitValue <= uliTransitionPeriodL2))
    {
        fValue1 = CalculateConcentration(fParam2,g_tTestInfo[num].uliFitValue);
    }
    else if((g_tTestInfo[num].uliFitValue > uliTransitionPeriodL2) && (g_tTestInfo[num].uliFitValue < uliTransitionPeriodH2))
    {
        fValue1 = CalculateConcentration(fParam2,g_tTestInfo[num].uliFitValue);
        fValue2 = CalculateConcentration(fParam3,g_tTestInfo[num].uliFitValue);
        fRatio = ((float)(g_tTestInfo[num].uliFitValue - uliTransitionPeriodL2))/(uliTransitionPeriodH2 - uliTransitionPeriodL2);
        fValue1 = (1-fRatio) * fValue1 + fRatio * fValue2;
    }
    else
    {
        fValue1 = CalculateConcentration(fParam3,g_tTestInfo[num].uliFitValue);
    }
    g_tTestInfo[num].ucResultFlag = 1;  //等于
    g_tTestInfo[num].fResult = fValue1;

    //srand((unsigned)time(NULL));

    //系统误差调整
    if(SystemErrorAdjustmentFlag==0)
        g_tTestInfo[num].fResult = g_tTestInfo[num].fResult*g_tSystemErrorAdjustment.ucKValue + g_tSystemErrorAdjustment.ucBValue;

    if(g_tTestInfo[num].fResult < g_tStandCurve[num].fLowLimit)
    {
        g_tTestInfo[num].ucResultFlag = 2;  //小于
        g_tTestInfo[num].fResult = g_tStandCurve[num].fLowLimit;
        return;
    }
    else if(g_tTestInfo[num].fResult > g_tStandCurve[num].fHighLimit)
    {
        g_tTestInfo[num].ucResultFlag = 3;  //大于
        g_tTestInfo[num].fResult = g_tStandCurve[num].fHighLimit;
        return;
    }
}

/*******************************************************************************
 * 函数名称：PmtFittingValue
 * 功能描述：将pmt的脉冲值和电压值拟合为信号值
 * 输入参数：Pulse 脉冲值  ADValue 电压值
 * 输出参数：无
 * 返回值  ：拟合后的值
 * 修改日期            版本号            修改人            修改内容
 * -----------------------------------------------------------------------------
 * 2017/09/12          V1.0             周志强               创建
 *******************************************************************************/
unsigned long int PmtFittingValue(unsigned long int Pulse,unsigned long int ADValue)
{
    unsigned long int uliValue = 0,uliValue1 = 0,uliValue2 = 0;
    unsigned long int uliFitValue = 0;
    float fValue=0,fRatio = 0;
    
    //将PMT信号的脉冲和AD拟合为一起，
    uliValue = (unsigned long int)(g_tPmtFitValue.fPmtPulseADFitK * ADValue + g_tPmtFitValue.fPmtPulseADFitB + 0.5);
    if(uliValue < g_tPmtFitValue.uliPmtTransitionLowValue)
    {
        uliFitValue = Pulse;
    }
    else if(uliValue > g_tPmtFitValue.uliPmtTransitionHighValue)
    {
        uliFitValue = uliValue;

    }
    else
    {
        fRatio = ((float)(uliValue-g_tPmtFitValue.uliPmtTransitionLowValue))/(g_tPmtFitValue.uliPmtTransitionHighValue-g_tPmtFitValue.uliPmtTransitionLowValue);
        fValue = (1-fRatio) * Pulse + fRatio * uliValue;
        uliFitValue = (unsigned long int)(fValue + 0.5);
    }

    //进行台间拟合
    if(uliFitValue <= g_tPmtFitValue.uliPmtsFitLowValue[0])
    {

    }
    else if(uliFitValue <= g_tPmtFitValue.uliPmtsFitHighValue[0])   //区间1进行拟合
    {
        if((uliFitValue >= g_tPmtFitValue.uliPmtsFitLowValue[1]) && (g_tPmtFitValue.uliPmtsFitLowValue[1] != 0))  //区间有交叉
        {
            uliValue1   = (unsigned long int)(g_tPmtFitValue.fPmtsFitK[0] * ((float)uliFitValue) + g_tPmtFitValue.fPmtsFitB[0]);
            uliValue2   = (unsigned long int)(g_tPmtFitValue.fPmtsFitK[1] * ((float)uliFitValue) + g_tPmtFitValue.fPmtsFitB[1]);
            fRatio      = ((float)(uliFitValue - g_tPmtFitValue.uliPmtsFitLowValue[1]))/((float)(g_tPmtFitValue.uliPmtsFitHighValue[0] - g_tPmtFitValue.uliPmtsFitLowValue[1]));
            uliFitValue = (unsigned long int)((1 - fRatio) * uliValue1 + fRatio * uliValue2 + 0.5);
        }
        else    //区间没有交叉
        {
            uliFitValue = (unsigned long int)(g_tPmtFitValue.fPmtsFitK[0] * ((float)uliFitValue) + g_tPmtFitValue.fPmtsFitB[0]);
        }
    }
    else if(uliFitValue <= g_tPmtFitValue.uliPmtsFitHighValue[1])   //区间2进行拟合
    {
        if((uliFitValue >= g_tPmtFitValue.uliPmtsFitLowValue[2]) && (g_tPmtFitValue.uliPmtsFitLowValue[2] != 0))  //区间有交叉
        {
            uliValue1   = (unsigned long int)(g_tPmtFitValue.fPmtsFitK[1] * ((float)uliFitValue) + g_tPmtFitValue.fPmtsFitB[1]);
            uliValue2   = (unsigned long int)(g_tPmtFitValue.fPmtsFitK[2] * ((float)uliFitValue) + g_tPmtFitValue.fPmtsFitB[2]);
            fRatio      = ((float)(uliFitValue - g_tPmtFitValue.uliPmtsFitLowValue[2]))/((float)(g_tPmtFitValue.uliPmtsFitHighValue[1] - g_tPmtFitValue.uliPmtsFitLowValue[2]));
            uliFitValue = (unsigned long int)((1 - fRatio) * uliValue1 + fRatio * uliValue2 + 0.5);
        }
        else    //区间没有交叉
        {
            uliFitValue = (unsigned long int)(g_tPmtFitValue.fPmtsFitK[1] * ((float)uliFitValue) + g_tPmtFitValue.fPmtsFitB[1]);
        }
    }
    else if(uliFitValue <= g_tPmtFitValue.uliPmtsFitHighValue[2])   //区间3进行拟合
    {
        if((uliFitValue >= g_tPmtFitValue.uliPmtsFitLowValue[3]) && (g_tPmtFitValue.uliPmtsFitLowValue[3] != 0))  //区间有交叉
        {
            uliValue1   = (unsigned long int)(g_tPmtFitValue.fPmtsFitK[2] * ((float)uliFitValue) + g_tPmtFitValue.fPmtsFitB[2]);
            uliValue2   = (unsigned long int)(g_tPmtFitValue.fPmtsFitK[3] * ((float)uliFitValue) + g_tPmtFitValue.fPmtsFitB[3]);
            fRatio      = ((float)(uliFitValue - g_tPmtFitValue.uliPmtsFitLowValue[3]))/((float)(g_tPmtFitValue.uliPmtsFitHighValue[2] - g_tPmtFitValue.uliPmtsFitLowValue[3]));
            uliFitValue = (unsigned long int)((1 - fRatio) * uliValue1 + fRatio * uliValue2 + 0.5);
        }
        else    //区间没有交叉
        {
            uliFitValue = (unsigned long int)(g_tPmtFitValue.fPmtsFitK[2] * ((float)uliFitValue) + g_tPmtFitValue.fPmtsFitB[2]);
        }
    }
    else if(uliFitValue <= g_tPmtFitValue.uliPmtsFitHighValue[3])   //区间4进行拟合
    {
        if((uliFitValue >= g_tPmtFitValue.uliPmtsFitLowValue[4]) && (g_tPmtFitValue.uliPmtsFitLowValue[4] != 0))  //区间有交叉
        {
            uliValue1   = (unsigned long int)(g_tPmtFitValue.fPmtsFitK[3] * ((float)uliFitValue) + g_tPmtFitValue.fPmtsFitB[3]);
            uliValue2   = (unsigned long int)(g_tPmtFitValue.fPmtsFitK[4] * ((float)uliFitValue) + g_tPmtFitValue.fPmtsFitB[4]);
            fRatio      = ((float)(uliFitValue - g_tPmtFitValue.uliPmtsFitLowValue[4]))/((float)(g_tPmtFitValue.uliPmtsFitHighValue[3] - g_tPmtFitValue.uliPmtsFitLowValue[4]));
            uliFitValue = (unsigned long int)((1 - fRatio) * uliValue1 + fRatio * uliValue2 + 0.5);
        }
        else    //区间没有交叉
        {
            uliFitValue = (unsigned long int)(g_tPmtFitValue.fPmtsFitK[3] * ((float)uliFitValue) + g_tPmtFitValue.fPmtsFitB[3]);
        }
    }
    else if(uliFitValue <= g_tPmtFitValue.uliPmtsFitHighValue[4])   //区间5进行拟合
    {
        if((uliFitValue >= g_tPmtFitValue.uliPmtsFitLowValue[5]) && (g_tPmtFitValue.uliPmtsFitLowValue[5] != 0))  //区间有交叉
        {
            uliValue1   = (unsigned long int)(g_tPmtFitValue.fPmtsFitK[4] * ((float)uliFitValue) + g_tPmtFitValue.fPmtsFitB[4]);
            uliValue2   = (unsigned long int)(g_tPmtFitValue.fPmtsFitK[5] * ((float)uliFitValue) + g_tPmtFitValue.fPmtsFitB[5]);
            fRatio      = ((float)(uliFitValue - g_tPmtFitValue.uliPmtsFitLowValue[5]))/((float)(g_tPmtFitValue.uliPmtsFitHighValue[4] - g_tPmtFitValue.uliPmtsFitLowValue[5]));
            uliFitValue = (unsigned long int)((1 - fRatio) * uliValue1 + fRatio * uliValue2 + 0.5);
        }
        else    //区间没有交叉
        {
            uliFitValue = (unsigned long int)(g_tPmtFitValue.fPmtsFitK[4] * ((float)uliFitValue) + g_tPmtFitValue.fPmtsFitB[4]);
        }
    }
    else if(uliFitValue <= g_tPmtFitValue.uliPmtsFitHighValue[5])   //区间6进行拟合
    {
        if((uliFitValue >= g_tPmtFitValue.uliPmtsFitLowValue[6]) && (g_tPmtFitValue.uliPmtsFitLowValue[6] != 0))  //区间有交叉
        {
            uliValue1   = (unsigned long int)(g_tPmtFitValue.fPmtsFitK[5] * ((float)uliFitValue) + g_tPmtFitValue.fPmtsFitB[5]);
            uliValue2   = (unsigned long int)(g_tPmtFitValue.fPmtsFitK[6] * ((float)uliFitValue) + g_tPmtFitValue.fPmtsFitB[6]);
            fRatio      = ((float)(uliFitValue - g_tPmtFitValue.uliPmtsFitLowValue[6]))/((float)(g_tPmtFitValue.uliPmtsFitHighValue[5] - g_tPmtFitValue.uliPmtsFitLowValue[6]));
            uliFitValue = (unsigned long int)((1 - fRatio) * uliValue1 + fRatio * uliValue2 + 0.5);
        }
        else    //区间没有交叉
        {
            uliFitValue = (unsigned long int)(g_tPmtFitValue.fPmtsFitK[5] * ((float)uliFitValue) + g_tPmtFitValue.fPmtsFitB[5]);
        }
    }
    else if(uliFitValue <= g_tPmtFitValue.uliPmtsFitHighValue[6])   //区间7进行拟合
    {
        if((uliFitValue >= g_tPmtFitValue.uliPmtsFitLowValue[7]) && (g_tPmtFitValue.uliPmtsFitLowValue[7] != 0))  //区间有交叉
        {
            uliValue1   = (unsigned long int)(g_tPmtFitValue.fPmtsFitK[6] * ((float)uliFitValue) + g_tPmtFitValue.fPmtsFitB[6]);
            uliValue2   = (unsigned long int)(g_tPmtFitValue.fPmtsFitK[7] * ((float)uliFitValue) + g_tPmtFitValue.fPmtsFitB[7]);
            fRatio      = ((float)(uliFitValue - g_tPmtFitValue.uliPmtsFitLowValue[7]))/((float)(g_tPmtFitValue.uliPmtsFitHighValue[6] - g_tPmtFitValue.uliPmtsFitLowValue[7]));
            uliFitValue = (unsigned long int)((1 - fRatio) * uliValue1 + fRatio * uliValue2 + 0.5);
        }
        else    //区间没有交叉
        {
            uliFitValue = (unsigned long int)(g_tPmtFitValue.fPmtsFitK[6] * ((float)uliFitValue) + g_tPmtFitValue.fPmtsFitB[6]);
        }
    }
    else if(g_tPmtFitValue.uliPmtsFitHighValue[7] != 0)    //区间8进行拟合
    {
            uliFitValue = (unsigned long int)(g_tPmtFitValue.fPmtsFitK[7] * ((float)uliFitValue) + g_tPmtFitValue.fPmtsFitB[7]);
    }
        
    return uliFitValue;
}

/*******************************************************************************
 * 函数名称：PmtProcess
 * 功能描述：触摸屏pmt采集流程
 * 输入参数：channel 通道号
 * 输出参数：无
 * 返回值  ：无
 * 修改日期            版本号            修改人            修改内容
 * -----------------------------------------------------------------------------
 * 2017/03/29          V1.0             周志强               创建
 *******************************************************************************/
void PmtProcess(unsigned char channel)
{
    Delay1ms(1500);
    SPISTARTHEAT(0);//关闭控温
    LASER = 1;
    Delay1ms(500);
    LASER = 0;
    Delay1ms(20);   //延迟一段时间，此时间和串口通信的时间相当
    PMTCTRL = 1;
    Delay1ms(20);//PMT打开后延迟20ms，待信号平稳后采样
    GetPmtData();
    PMTCTRL = 0;
    SPISTARTHEAT(1);//打开控温
    g_tTestInfo[channel].uliPulseValue = g_uliPulseValue;
    g_tTestInfo[channel].uliADValue    = g_uliADValue;
    g_tTestInfo[channel].uliFitValue   = PmtFittingValue(g_uliPulseValue,g_uliADValue);
}

/*******************************************************************************
 * 函数名称：PrePmtProcess
 * 功能描述：触摸屏预采集流程，由于每轮第一次数据采集数据不稳定，故预采集一次
 * 输入参数：channel 通道号
 * 输出参数：无
 * 返回值  ：无
 * 修改日期            版本号            修改人            修改内容
 * -----------------------------------------------------------------------------
 * 2017/03/29          V1.0             周志强               创建
 *******************************************************************************/
void PrePmtProcess(unsigned char channel)
{
    Delay1ms(20);
    PMTCTRL = 1;
    GetPmtData();
    PMTCTRL = 0;
    g_tTestInfo[channel].uliPulseValue = g_uliPulseValue;
    g_tTestInfo[channel].uliADValue    = g_uliADValue;
    g_tTestInfo[channel].uliFitValue   = PmtFittingValue(g_uliPulseValue,g_uliADValue);
}
void PrePmtProcess1(void)
{
    Delay1ms(20);
    PMTCTRL = 1;
    Delay1ms(5000);
    PMTCTRL = 0;
}
/*******************************************************************************
 * 函数名称：TakeTipProcess
 * 功能描述：取Tip头流程
 * 输入参数：无
 * 输出参数：无
 * 返回值  ：无
 * 修改日期            版本号            修改人            修改内容
 * -----------------------------------------------------------------------------
 * 2017/03/29          V1.0             周志强               创建
 *******************************************************************************/
void LcdTakeTipProcess(void)
{
    SPIMotor(2,MT_MOVE_TO,g_tTipVerticalMotor.uiTakeTipPosition1);           //Tip头垂直电机运动到取Tip头位置1
    if(g_tProcessControl.ucStopFlag==0)
        Delay1ms(100);                                                           //延迟100ms
    SPIMotor(2,SET_SPEED,g_tTipVerticalMotor.ucTakeTipSlowSpeed);            //设置Tip头垂直电机运动速度为慢速
    SPIMotor(2,MT_MOVE_TO,g_tTipVerticalMotor.uiTakeTipPosition2);           //Tip头垂直电机运动到取Tip头位置2
    SPIMotor(2,SET_SPEED,240);                                            //恢复Tip头垂直电机的速度
    if(g_tProcessControl.ucStopFlag==0)
        Delay1ms(300);                                                           //延迟300ms
    SPIMotor(2,MT_HOME,0);                                                   //Tip头垂直电机复位
}

/*******************************************************************************
 * 函数名称：LcdOffTipProcess
 * 功能描述：退Tip头流程
 * 输入参数：无
 * 输出参数：无
 * 返回值  ：无
 * 修改日期            版本号            修改人            修改内容
 * -----------------------------------------------------------------------------
 * 2017/03/29          V1.0             周志强               创建
 *******************************************************************************/
void LcdOffTipProcess(void)
{
    SPIMotor(1,MT_MOVE_TO,g_tTipHorizontalMotor.uiOffTipPosition1);          //Tip头水平电机运动到退Tip头位置1
    SPIMotor(2,MT_MOVE_TO,g_tTipVerticalMotor.uiOffTip);                     //Tip头垂直电机运动到退Tip头位置
    SPIMotor(1,MT_MOVE_TO,g_tTipHorizontalMotor.uiOffTipPosition2);          //Tip头水平电机运动到退Tip头位置2
    SPIMotor(2,MT_HOME,0);                                                   //Tip头垂直电机复位
}

/*******************************************************************************
 * 函数名称：LcdSuctionLiquid
 * 功能描述：取样本或试剂流程
 * 输入参数：uiTipHorizontal：Tip水平电机运动到的位置；uiSuctionLiquid：吸液体量
 * 输出参数：无
 * 返回值  ：无
 * 修改日期            版本号            修改人            修改内容
 * -----------------------------------------------------------------------------
 * 2017/03/29          V1.0             周志强               创建
 *******************************************************************************/
void LcdSuctionLiquid(unsigned int uiTipHorizontal,unsigned int uiSuctionLiquid)
{
    if(uiSuctionLiquid > 0)
    {
        SPIMotor(1,MT_MOVE_TO,uiTipHorizontal);                                 //Tip头水平电机运动到的位置
        SPIMotor(2,MT_MOVE_TO,g_tTipVerticalMotor.uiSuctionLiquidPosition);     //Tip头垂直电机运动到取液位置
        if(g_tProcessControl.ucStopFlag==0)
            Delay1ms(50);                                                           //延迟50ms
        SPIMotor(3,MT_MOVE_TO,g_uiPistonPosition+uiSuctionLiquid);              //活塞电机吸第一试剂
        if(g_tProcessControl.ucStopFlag==0)
            Delay1ms(200);                                                          //延迟200ms
        SPIMotor(2,MT_HOME,0);                                                  //Tip头垂直电机复位
    }
}

/*******************************************************************************
 * 函数名称：LcdMixLiquid
 * 功能描述：混匀受氧供氧流程
 * 输入参数：uiTipHorizontal：Tip水平电机运动到的位置；uiSuctionLiquid：吸液体量
 * 输出参数：无
 * 返回值  ：无
 * 修改日期            版本号            修改人            修改内容
 * -----------------------------------------------------------------------------
 * 2018/04/17          V1.0             许勇               创建
 *******************************************************************************/
static void LcdMixLiquid(unsigned int uiTipHorizontal,unsigned int uiSuctionLiquid)
{
    unsigned char k;
    if(uiSuctionLiquid > 0)
    {
        SPIMotor(1,MT_MOVE_TO,uiTipHorizontal);                                 //Tip头水平电机运动到的位置
        SPIMotor(2,MT_MOVE_TO,g_tTipVerticalMotor.uiSuctionLiquidPosition);     //Tip头垂直电机运动到取液位置
        SPIMotor(3,SET_SPEED,g_tPistonMotor.ucMixSpeed);                        //活塞电机的混匀速度
        if(g_tProcessControl.ucStopFlag==0)
            Delay1ms(50);                                                       //延迟50ms
        for(k=0;k<2;k++)
        {
            SPIMotor(3,MT_MOVE_TO,g_uiPistonPosition + uiSuctionLiquid);        //活塞电机混匀
            SPIMotor(3,MT_MOVE_TO,g_uiPistonPosition - (uiSuctionLiquid-20));   //活塞电机混匀
        }
        SPIMotor(3,MT_MOVE_TO,g_uiPistonPosition + uiSuctionLiquid);               //活塞电机吸液
        SPIMotor(2,MT_MOVE_TO,g_tTipVerticalMotor.uiOffLiquid);                 //Tip头垂直电机运动到排液位置
        SPIMotor(3,SET_SPEED,g_tPistonMotor.ucSuctionSpeed);                    //恢复活塞电机的速度
        //SPIMotor(3,MT_HOME,0);   //活塞电机排液
        SPIMotor(3,MT_MOVE_TO,g_uiPistonPosition - (uiSuctionLiquid + (k+1)*40));//活塞电机排液
        if(g_tProcessControl.ucStopFlag==0)
            Delay1ms(200);                                                          //延迟200ms
        SPIMotor(2,MT_HOME,0);                                                  //Tip头垂直电机复位
    }
}

/*******************************************************************************
 * 函数名称：InitStartProcess
 * 功能描述：初始化仪器运行前的变量和操作
 * 输入参数：无
 * 输出参数：无
 * 返回值  ：0:成功  1:失败
 * 修改日期            版本号            修改人            修改内容
 * -----------------------------------------------------------------------------
 * 2017/03/21          V1.0             周志强               创建
 *******************************************************************************/
static unsigned char InitStartProcess(void)
{
    unsigned char i = 0,k = 0;
    unsigned char ucTestChannelNo = 0;
    unsigned char ucFlag = 0;   //是否找到详细信息标志位，用于获取温育时间

    g_tProcessControl.uiRunTime     = 0;
    g_tProcessControl.ucStopFlag    = 0;
    
    if(g_tSystemSetting.ucInputSampleIDMethod == 1)//自动扫描
    {
        AutoGetReagentBriefBarCode();
        if(AutoGetReagentBriefBarCodeFlag == 1)
        {
            return 1;
        }      
    }
    g_tProcessControl.ucRunningFlag = 1;
    for(i=0;i<12;i++)           //计时标志量清零
    {
        g_tTestInfo[i].ucStartTimingFlag = 0;
        g_tTestInfo[i].uiTimer           = 0;
    }
    
    for(i=0;i<12;i++)
    {
        if(g_tTestInfo[i].ucName[0] != '\0')
        //if(g_tTestInfo[i].ucSampleID[0] != '\0')
        {
            ucTestChannelNo++;
            if(0 == FindReadReagentDetailInfo(i))   //获取成功
            {
                k = i;
                ucFlag = 1;
            }
        }
    }
    
    if(ucTestChannelNo == 0)
    {
        SendDataToLcdRegister(0x4F,0x22);  //未设置通道信息，请进行设置！ 对话框
        while(g_ucUartLcdRecvFlag == 0); //等待选择
        g_ucUartLcdRecvFlag = 0;
        AnalyticInstruction();
        if((g_tLcdAnalyticIns.uiLcdParamAddr == g_tTestPictureAddr.uiStartDialog) && (g_tLcdAnalyticIns.uiButtonValue == 1))
        {
            return 1;
        }
    }
    else
    {
        SPIMotor(3,MT_HOME,0);      //活塞电机复位
        Delay1ms(1000);             //延迟1s
        SPIMotor(3,MT_HOME,0);      //活塞电机复位
        SPIMotor(2,MT_HOME,0);      //Tip头垂直电机复位
        SPIMotor(5,MT_HOME,0);      //PMT垂直电机复位
        SPIMotor(4,MT_HOME,0);      //试剂盘电机复位
        SPIMotor(1,MT_HOME,0);      //Tip头水平电机复位//
    }
    
    DisplayPicture(TEST_RUN_PICTURE1);
    CutPicture(g_tTestPictureAddr.uiProcessBar,TEST_RUN_PICTURE1,0,52,599,120,0,52);
    if(ucFlag == 1) //找到
    {
        g_tProcessControl.uiTotalTime   = (g_tRunProcessFile[k].uiProcess1Timer/10 + g_tRunProcessFile[k].uiProcess2Timer/10);
        g_tProcessControl.uiTotalTime   += (unsigned int)(1.0 * (1030 - g_tProcessControl.uiTotalTime) / 12 * ucTestChannelNo);
        //g_tProcessControl.uiTotalTime   += 29 * (ucTestChannelNo-1) + 1;
    }
    else            //未找到，写一个默认值
    {
        
        
        g_tProcessControl.uiTotalTime   = 450 + 300;
        g_tProcessControl.uiTotalTime   += (unsigned int)(1.0 * (1030 - g_tProcessControl.uiTotalTime) / 12 * ucTestChannelNo);
        //g_tProcessControl.uiTotalTime   += 29 * (ucTestChannelNo-1) + 1;
    }
    UpdateProcessBar(0);
    InitTimer5(1);
    return 0;
}

/*******************************************************************************
 * 函数名称：PushStartButton
 * 功能描述：按下“开始”按键的处理函数
 * 输入参数：无
 * 输出参数：
 * 返回值  ：无
 * 修改日期            版本号            修改人            修改内容
 * -----------------------------------------------------------------------------
 * 2017/02/21          V1.0             周志强               创建
 *******************************************************************************/
void PushStartButton(void)
{
    unsigned char ucValue=0;
    if(START_BUTTON == 0)
    {
        Delay1ms(10);
        if(START_BUTTON == 0)
        {
            AutoGetReagentBriefBarCodeFlag = 0;
            g_ucStartButtonLock = 1;
            SendDataToLcdRegister(0x4F,IS_START_PROCESS);  //弹出对话框是否开始运行对话框
            while(g_ucUartLcdRecvFlag == 0); //等待选择
            g_ucUartLcdRecvFlag = 0;
            AnalyticInstruction();
            if((g_tLcdAnalyticIns.uiLcdParamAddr == DIALOG_VALUE_ADDR) && (g_tLcdAnalyticIns.uiButtonValue == 1))   //是
            {
                if(g_uiTipCounterNum > g_uiTipLimitNum)
                {
                    SendDataToLcdRegister(0x4F,WASTE_TIP_DIALOG);  //弹出清理垃圾桶对话框
                    while(1)
                    {
                        if(DUSTBIN_SENSOR1 == 1 || g_ucUartLcdRecvFlag == 1)
                        {
                            SendDataToLcdRegister(0x4F,0x01);  //清除弹出清理垃圾桶对话框
                            g_ucUartLcdRecvFlag = 0;
                            break;
                        }   
                    }
                    g_ucStartButtonLock = 0;
                    return;
                }
                if(DUSTBIN_SENSOR1 == 1) //未放置垃圾桶
                {
                    Delay1ms(50);
                    if(DUSTBIN_SENSOR1 == 1) //未放置垃圾桶
                    {
                        SendDataToLcdRegister(0x4F,DASTBIN_DIALOG);  //弹出未放置垃圾桶对话框
                        while(1)
                        {
                            if(DUSTBIN_SENSOR1 == 0 || g_ucUartLcdRecvFlag == 1)
                            {
                                SendDataToLcdRegister(0x4F,0x02);  //清除弹出清理垃圾桶对话框
                                g_ucUartLcdRecvFlag = 0;
                                break;
                            }   
                        }
                        g_ucStartButtonLock = 0;
                        return;
                    }
                }
                
                if(1 == InitStartProcess())
                {
                    g_tProcessControl.ucRunningFlag = 0;
                    g_ucStartButtonLock = 0;
                    return;
                }           
                Delay1ms(50);   
                
                ucValue = IsChannelsSetOk();
                if(1 == ucValue)            //至少设置了一个通道
                {
                    LcdRunProcess();
                }
                else if(2 == ucValue)       //返回继续设置
                {
                    InitTimer5(0);
                    g_tProcessControl.ucRunningFlag = 0;
                    g_tProcessControl.ucStopFlag = 0;
                    g_ucStartButtonLock = 0;
                } 
            }
            else        //否
            {

            }
            g_ucStartButtonLock = 0;
        }
    }
}

/*******************************************************************************
 * 函数名称：PushNextButton
 * 功能描述：按下“NEXT”按键的处理函数
 * 输入参数：无
 * 输出参数：
 * 返回值  ：无
 * 修改日期            版本号            修改人            修改内容
 * -----------------------------------------------------------------------------
 * 2017/02/21          V1.0             周志强               创建
 *******************************************************************************/
void PushNextButton(void)
{
    unsigned long int uliPulse = 0;
    if(NEXT_BUTTON == 0)
    {
        Delay1ms(10);
        if(NEXT_BUTTON == 0)
        {
            g_ucNextPositionButtonLock = 1;
            g_ucNextPositionFlag++;
            if(g_ucNextPositionFlag>=6)
            {
                g_ucNextPositionFlag = 0;
            }
            uliPulse = (unsigned long int)(g_tReagentTrayMotor.fReagentKitGap * 2 * g_ucNextPositionFlag + 0.5);
            if(g_ucNextPositionFlag == 0)
            {
                //SPIMotor(4,MT_HOME,0);                    //放置通道1和2
                SPIMotor(4,MT_MOVE_TO_FW,0);                //放置通道1和2
            }
            else if(g_ucNextPositionFlag == 1)
            {
                SPIMotor(4,MT_MOVE_TO,uliPulse);           //放置通道3和4
            }
            else if(g_ucNextPositionFlag == 2)
            {
                SPIMotor(4,MT_MOVE_TO,uliPulse);           //放置通道5和6
            }
            else if(g_ucNextPositionFlag == 3)
            {
                SPIMotor(4,MT_MOVE_TO,uliPulse);           //放置通道7和8
            }
            else if(g_ucNextPositionFlag == 4)
            {
                SPIMotor(4,MT_MOVE_TO,uliPulse);           //放置通道9和10
            }
            else if(g_ucNextPositionFlag == 5)
            {
                SPIMotor(4,MT_MOVE_TO,uliPulse);           //放置通道11和12
            }
            g_ucNextPositionButtonLock =0;
        }
    }
}

/*******************************************************************************
 * 函数名称：LcdDilutionProcess
 * 功能描述：稀释流程函数
 * 输入参数：num 通道号
 * 输出参数：无
 * 返回值  ：无
 * 修改日期            版本号            修改人            修改内容
 * -----------------------------------------------------------------------------
 * 2017/09/10          V1.0             周志强               创建
 *******************************************************************************/
static void LcdDilutionProcess(unsigned char num)
{
    unsigned char j=0;
    unsigned int uiMixPulse = 0;
    if((g_tTestInfo[num].ucDilute > 1) && ((g_tTestInfo[num].ucSampleType == 1) || (g_tTestInfo[num].ucSampleType == 2) || (g_tTestInfo[num].ucSampleType == 4) || (g_tTestInfo[num].ucSampleType == 5)))   //进行稀释
    {
        SPIMotor(1,MT_MOVE_TO,g_tTipHorizontalMotor.uiDilution);     //Tip头水平电机运动到稀释位置
        uiMixPulse =  PulseForVolume(g_tRunProcessFile[num].uiDilutionVol + g_tRunProcessFile[num].uiSampleVol) - g_tPistonMotor.uiRemainPulse-200;
        if(uiMixPulse > 24500)
            SPIMotor(2,MT_MOVE_TO,g_tTipVerticalMotor.uiDilutionMixPosition-200);     //Tip头垂直电机运动到排液混匀位置
        else
            SPIMotor(2,MT_MOVE_TO,g_tTipVerticalMotor.uiDilutionMixPosition);     //Tip头垂直电机运动到排液混匀位置
        SPIMotor(3,SET_SPEED,g_tPistonMotor.ucMixSpeed);                      //活塞电机的混匀速度
        SPIMotor(3,MT_MOVE_TO,g_uiPistonPosition-(g_tRunProcessFile[num].uiSample-200));//活塞电机排液
        if(uiMixPulse > 12500)
            uiMixPulse = 12500;
        for(j=1;j<g_tRunProcessFile[num].ucMixTimes;j++)
        {
            SPIMotor(3,MT_MOVE_TO,g_uiPistonPosition + uiMixPulse);           //活塞电机混匀
            SPIMotor(3,MT_MOVE_TO,g_uiPistonPosition - (uiMixPulse-100));     //活塞电机混匀
        }
        SPIMotor(3,MT_MOVE_TO,g_uiPistonPosition + uiMixPulse);               //活塞电机吸液
        SPIMotor(2,MT_MOVE_TO,g_tTipVerticalMotor.uiOffLiquid);               //Tip头垂直电机运动到排液位置
        SPIMotor(3,SET_SPEED,g_tPistonMotor.ucSuctionSpeed);                  //恢复活塞电机的速度
        //SPIMotor(3,MT_MOVE_TO,g_uiPistonPosition-(uiMixPulse + 50*(g_tRunProcessFile[num].ucMixTimes+1)+ g_tPistonMotor.uiOverPulse));//活塞电机排液  
        SPIMotor(3,MT_HOME,0);      //活塞电机复位
        if(g_tProcessControl.ucStopFlag==0)
            Delay1ms(100);
        SPIMotor(2,MT_HOME,0); 
        SPIMotor(3,MT_MOVE_TO,g_uiPistonPosition+g_tPistonMotor.uiPreSuction);   //活塞电机预吸液
        //以下为吸液流程
        SPIMotor(2,MT_MOVE_TO,g_tTipVerticalMotor.uiDilutionMixPosition-100);                  //Tip头垂直电机运动到混匀位置       取液位置uiSuctionLiquidPosition
        if(g_tProcessControl.ucStopFlag==0)
            Delay1ms(50);                                                                        //
        
        SPIMotor(3,MT_MOVE_TO,g_uiPistonPosition + g_tRunProcessFile[num].uiDilutionSample); //活塞电机吸液
        g_tRunProcessFile[num].uiSample = g_tRunProcessFile[num].uiDilutionSample;
        if(g_tProcessControl.ucStopFlag==0)
            Delay1ms(200);                                                                       //延迟200ms
        SPIMotor(2,MT_HOME,0);                                                               //Tip头垂直电机复位
    }
    if((g_tTestInfo[num].ucDilute_Wholeblood > 1) && (g_tTestInfo[num].ucSampleType == 3))   //进行全血稀释
    {
        SPIMotor(1,MT_MOVE_TO,g_tTipHorizontalMotor.uiDilution);     //Tip头水平电机运动到稀释位置
        uiMixPulse =  PulseForVolume(g_tRunProcessFile[num].uiDilutionVol_Wholeblood + g_tRunProcessFile[num].uiSampleVol_Wholeblood) - g_tPistonMotor.uiRemainPulse-200;
        if(uiMixPulse > 24500)
            SPIMotor(2,MT_MOVE_TO,g_tTipVerticalMotor.uiDilutionMixPosition-200);     //Tip头垂直电机运动到排液混匀位置
        else 
            SPIMotor(2,MT_MOVE_TO,g_tTipVerticalMotor.uiDilutionMixPosition);     //Tip头垂直电机运动到排液混匀位置
        SPIMotor(3,SET_SPEED,g_tPistonMotor.ucMixSpeed);                      //活塞电机的混匀速度
        SPIMotor(3,MT_MOVE_TO,g_uiPistonPosition-(g_tRunProcessFile[num].uiSample_Wholeblood-200));//活塞电机排液
        if(uiMixPulse > 12500)
            uiMixPulse = 12500;
        for(j=1;j<g_tRunProcessFile[num].ucMixTimes;j++)
        {
            SPIMotor(3,MT_MOVE_TO,g_uiPistonPosition + uiMixPulse);           //活塞电机混匀
            SPIMotor(3,MT_MOVE_TO,g_uiPistonPosition - (uiMixPulse-100));     //活塞电机混匀
        }
        SPIMotor(3,MT_MOVE_TO,g_uiPistonPosition + uiMixPulse);               //活塞电机吸液
        SPIMotor(2,MT_MOVE_TO,g_tTipVerticalMotor.uiOffLiquid);               //Tip头垂直电机运动到排液位置
        SPIMotor(3,SET_SPEED,g_tPistonMotor.ucSuctionSpeed);                  //恢复活塞电机的速度
        //SPIMotor(3,MT_MOVE_TO,g_uiPistonPosition-(uiMixPulse + 50*(g_tRunProcessFile[num].ucMixTimes+1) + g_tPistonMotor.uiOverPulse));//活塞电机排液
        SPIMotor(3,MT_HOME,0);      //活塞电机复位
        if(g_tProcessControl.ucStopFlag==0)
            Delay1ms(100);
        SPIMotor(2,MT_HOME,0); 
        SPIMotor(3,MT_MOVE_TO,g_uiPistonPosition+g_tPistonMotor.uiPreSuction);   //活塞电机预吸液
        //以下为吸液流程
        SPIMotor(2,MT_MOVE_TO,g_tTipVerticalMotor.uiDilutionMixPosition-100);                  //Tip头垂直电机运动到取液位置uiSuctionLiquidPosition
        if(g_tProcessControl.ucStopFlag==0)
            Delay1ms(50);                                                                        //延迟50ms
        SPIMotor(3,MT_MOVE_TO,g_uiPistonPosition + g_tRunProcessFile[num].uiDilutionSample_Wholeblood); //活塞电机吸液
        g_tRunProcessFile[num].uiSample_Wholeblood = g_tRunProcessFile[num].uiDilutionSample_Wholeblood;
        if(g_tProcessControl.ucStopFlag==0)
            Delay1ms(200);                                                                       //延迟200ms
        SPIMotor(2,MT_HOME,0);                                                               //Tip头垂直电机复位
    }
}


/*******************************************************************************
 * 函数名称：LcdRunProcess
 * 功能描述：运行流程函数
 * 输入参数：无
 * 输出参数：无
 * 返回值  ：无
 * 修改日期            版本号            修改人            修改内容
 * -----------------------------------------------------------------------------
 * 2017/03/21          V1.0             周志强               创建
 *******************************************************************************/
void LcdRunProcess(void)
{
    unsigned char i=0,j=0,k=0;
    unsigned char ucRunProcess1Num=0,ucRunProcess2Num=0,ucRunProcess3Num=0;
    unsigned int  uiMixPulse=0;
    unsigned char ucFistTestFlag = 0;   //PMT预采集变量
    unsigned int uiDivisionLine1[7] ={204,323,442,561,680,799,918};

    for(i=0;i<12;i++)
    {
        g_tTestInfo[i].ucTestedFlag = 0;
        g_tTestInfo[i].ucStepEndFlag = 0;
        DisplayTestRestult(i,0);
        if(g_tTestInfo[i].ucPrintFlag == 1)
            SelectPrintChannel(i);
    }
    AutoGetReagentBriefBarCodeFlag = 0;
    SPIMotor(4,MT_HOME,0);      //试剂盘电机复位
    SPIMotor(3,SET_SPEED, g_tPistonMotor.ucSuctionSpeed);                           //设置活塞电机的速度
    for(i=0;i<12;i++)   //流程1：吸样本、试剂1和试剂2
    {
        if(g_tTestInfo[i].ucSetOkFlag == 1)
        {
            g_tProcessTimeFlag = 1;
            SPIMotor(4,MT_MOVE_TO,g_tReagentTrayMotor.uiaTip1Position[i]);           //试剂盘电机运动到第一Tip位置
            SPIMotor(1,MT_MOVE_TO,g_tTipHorizontalMotor.uiTakeTip);                  //Tip头水平电机运动到取Tip头位置
            LcdTakeTipProcess();                                                     //取Tip流程
            SPIMotor(4,MT_MOVE_TO,g_tReagentTrayMotor.uiaReagentKitPosition[i]);     //试剂盘电机运动到试剂盒位置
            
            SPIMotor(1,MT_MOVE_TO,g_tTipHorizontalMotor.uiSecondReagent);                                 //Tip头水平电机运动到R2位置                                                         //延迟50ms
            SPIMotor(2,MT_MOVE_TO,g_tTipVerticalMotor.uiPrepuncturePosition+350);     //Tip头垂直电机运动到取液位置g_tTipVerticalMotor.uiOffLiquid
            Delay1ms(50);     
            SPIMotor(2,MT_HOME,0);
            SPIMotor(1,MT_MOVE_TO,g_tTipHorizontalMotor.uiFirstReagent);                                 //Tip头水平电机运动到R1位置                                                         //延迟50ms
            SPIMotor(2,MT_MOVE_TO,g_tTipVerticalMotor.uiPrepuncturePosition+350);     //Tip头垂直电机运动到取液位置g_tTipVerticalMotor.uiOffLiquid
            Delay1ms(50);     
            SPIMotor(2,MT_HOME,0);
            if((g_tTestInfo[i].ucDilute > 1)) 
            {
                SPIMotor(1,MT_MOVE_TO,g_tTipHorizontalMotor.uiDilution);                                 //Tip头水平电机运动到稀释位置
                SPIMotor(2,MT_MOVE_TO,g_tTipVerticalMotor.uiPrepuncturePosition);     //Tip头垂直电机运动到取液位置g_tTipVerticalMotor.uiOffLiquid
                Delay1ms(50);     
                SPIMotor(2,MT_HOME,0); 
            }
             
            
            SPIMotor(3,MT_MOVE_TO,g_uiPistonPosition+g_tPistonMotor.uiPreSuction);   //活塞电机预吸液
            //LcdMixLiquid(g_tTipHorizontalMotor.uiSecondReagent,g_tRunProcessFile[i].uiSecondReagent);//混匀第二试剂
            
            //SPIMotor(3,MT_MOVE_TO,g_uiPistonPosition+g_tPistonMotor.uiPreSuction);   //活塞电机预吸液
            if((g_tTestInfo[i].ucSampleType == 1) || (g_tTestInfo[i].ucSampleType == 2) || (g_tTestInfo[i].ucSampleType == 4) || (g_tTestInfo[i].ucSampleType == 5))//血清、血浆、QC1、QC2流程
            {
                LcdSuctionLiquid(g_tTipHorizontalMotor.uiSample,g_tRunProcessFile[i].uiSample);//取样本
                LcdDilutionProcess(i);  //稀释流程
                LcdSuctionLiquid(g_tTipHorizontalMotor.uiFirstReagent,g_tRunProcessFile[i].uiFistReagent);  //取第一试剂
                LcdSuctionLiquid(g_tTipHorizontalMotor.uiSecondReagent,g_tRunProcessFile[i].uiSecondReagent);//取第二试剂
            }
            if(g_tTestInfo[i].ucSampleType == 3)   //全血流程
            {
                LcdSuctionLiquid(g_tTipHorizontalMotor.uiSample,g_tRunProcessFile[i].uiSample_Wholeblood);//取样本
                LcdDilutionProcess(i);  //稀释流程
                LcdSuctionLiquid(g_tTipHorizontalMotor.uiFirstReagent,g_tRunProcessFile[i].uiFistReagent_Wholeblood);  //取第一试剂
                LcdSuctionLiquid(g_tTipHorizontalMotor.uiSecondReagent,g_tRunProcessFile[i].uiSecondReagent_Wholeblood);//取第二试剂
            }
            
            SPIMotor(1,MT_MOVE_TO,g_tTipHorizontalMotor.uiTestPositon);              //Tip头水平电机运动到检测位
            SPIMotor(2,MT_MOVE_TO,g_tTipVerticalMotor.uiOffLiquid);                  //Tip头垂直电机运动到排液高度
            if(g_tProcessControl.ucStopFlag==0)
                Delay1ms(50);                                                            //延迟50ms
            if((g_tTestInfo[i].ucSampleType == 1) || (g_tTestInfo[i].ucSampleType == 2) || (g_tTestInfo[i].ucSampleType == 4) || (g_tTestInfo[i].ucSampleType == 5))//血清、血浆、QC1、QC2流程
            {
                SPIMotor(3,MT_MOVE_TO,g_uiPistonPosition-(g_tRunProcessFile[i].uiSample+g_tRunProcessFile[i].uiFistReagent+g_tRunProcessFile[i].uiSecondReagent+g_tPistonMotor.uiOverPulse));//活塞电机排液
            }
            if(g_tTestInfo[i].ucSampleType == 3)   //全血流程
            {
                SPIMotor(3,MT_MOVE_TO,g_uiPistonPosition-(g_tRunProcessFile[i].uiSample_Wholeblood+g_tRunProcessFile[i].uiFistReagent_Wholeblood+g_tRunProcessFile[i].uiSecondReagent_Wholeblood+g_tPistonMotor.uiOverPulse));//活塞电机排液
            }
            if(g_tProcessControl.ucStopFlag==0)
                //Delay1ms(500);//修改
                Delay1ms(200);//延迟500ms            
            g_tTestInfo[i].ucStartTimingFlag = 1;                                    //该通道开始计时
            g_tTestInfo[i].ucStepEndFlag = 1; 
            SPIMotor(2,MT_HOME,0);                                                   //Tip头垂直电机复位
            LcdOffTipProcess();
            SPIMotor(3,MT_HOME,0);      //活塞电机复位
            ucRunProcess1Num++;                                                      //记录运行第一步流程的通道数
            
            while(g_tProcessTimeFlag1 < 270)
            {
                RunLcdConmunication();
                if(g_tProcessControl.ucStopFlag==1)
                {
                    break;
                }
            }
            g_tProcessTimeFlag = 0;
            g_tProcessTimeFlag1 = 0;
            
            if(g_tProcessControl.ucStopFlag==1)
            {
                break;
            }
        }
    }
    SPIMotor(4,MT_HOME,0);      //试剂盘电机复位
    SPIMotor(1,MT_HOME,0);      //水平电机复位
    while(1)    //流程2：吸试剂3并混匀
    {
        for(i=0;i<12;i++)
        {
            RunLcdConmunication();
            //if((g_tTestInfo[i].ucSetOkFlag == 1)&&(g_tTestInfo[i].uiTimer>=g_tRunProcessFile[i].uiProcess1Timer)&&(g_tTestInfo[i].uiTimer < (1.1*g_tRunProcessFile[i].uiProcess1Timer)))
            if((g_tTestInfo[i].ucSetOkFlag == 1)&&(g_tTestInfo[i].uiTimer>=g_tRunProcessFile[i].uiProcess1Timer)&&(g_tTestInfo[i].ucStepEndFlag == 1)&&(g_tProcessControl.ucStopFlag==0))
            {
                g_tTestInfo[i].ucStartTimingFlag = 0;                                 //该通道关闭计时
                g_tTestInfo[i].uiTimer           = 0;                                 //计时变量清零
                g_tTestInfo[i].ucStepEndFlag = 0;
                SPIMotor(1,MT_MOVE_TO,g_tTipHorizontalMotor.uiTakeTip);               //Tip头水平电机运动到取Tip头位置
                SPIMotor(4,MT_MOVE_TO,g_tReagentTrayMotor.uiaTip2Position[i]);        //试剂盘电机运动到取第二Tip位置
                LcdTakeTipProcess();                                                  //取Tip流程
                SPIMotor(4,MT_MOVE_TO,g_tReagentTrayMotor.uiaReagentKitPosition[i]);  //试剂盘电机运动到试剂盒位置
                
                SPIMotor(1,MT_MOVE_TO,g_tTipHorizontalMotor.uiThirdReagent);                                 //Tip头水平电机运动到R3位置                                                         //延迟50ms
                SPIMotor(2,MT_MOVE_TO,g_tTipVerticalMotor.uiPrepuncturePosition+350);     //Tip头垂直电机运动到取液位置g_tTipVerticalMotor.uiOffLiquid
                Delay1ms(50);  
                SPIMotor(2,MT_HOME,0);
                Delay1ms(3000); 
                
                SPIMotor(3,MT_MOVE_TO,g_uiPistonPosition+g_tPistonMotor.uiPreSuction);   //活塞电机预吸液
                //LcdMixLiquid(g_tTipHorizontalMotor.uiThirdReagent,g_tRunProcessFile[i].uiThirdReagent);//混匀第三试剂
                
                //SPIMotor(3,MT_MOVE_TO,g_uiPistonPosition+g_tPistonMotor.uiPreSuction);//活塞电机预吸液
                if((g_tTestInfo[i].ucSampleType == 1) || (g_tTestInfo[i].ucSampleType == 2) || (g_tTestInfo[i].ucSampleType == 4) || (g_tTestInfo[i].ucSampleType == 5))//血清、血浆、QC1、QC2流程
                {
                    LcdSuctionLiquid(g_tTipHorizontalMotor.uiThirdReagent,g_tRunProcessFile[i].uiThirdReagent);  //取第三试剂
                }
                if(g_tTestInfo[i].ucSampleType == 3)   //全血流程
                {
                    LcdSuctionLiquid(g_tTipHorizontalMotor.uiThirdReagent,g_tRunProcessFile[i].uiThirdReagent_Wholeblood);  //取第三试剂
                }
                
                SPIMotor(1,MT_MOVE_TO,g_tTipHorizontalMotor.uiTestPositon);           //Tip头水平电机运动到检测位置
                SPIMotor(2,MT_MOVE_TO,g_tTipVerticalMotor.uiTestMixPosition);         //Tip头垂直电机运动到排液混匀位置
                if(g_tProcessControl.ucStopFlag==0)
                    Delay1ms(50);                                                     //延迟50ms
                SPIMotor(2,MT_MOVE_TO,g_tTipVerticalMotor.uiTestMixPosition);         //Tip头垂直电机运动到排液混匀位置
                SPIMotor(3,SET_SPEED,g_tPistonMotor.ucMixSpeed);                      //活塞电机的混匀速度
                if((g_tTestInfo[i].ucSampleType == 1) || (g_tTestInfo[i].ucSampleType == 2) || (g_tTestInfo[i].ucSampleType == 4) || (g_tTestInfo[i].ucSampleType == 5))//血清、血浆、QC1、QC2流程
                {
                    SPIMotor(3,MT_MOVE_TO,g_uiPistonPosition-(g_tRunProcessFile[i].uiThirdReagent-200));//活塞电机排液
                    uiMixPulse = g_tRunProcessFile[i].uiSample+g_tRunProcessFile[i].uiFistReagent+g_tRunProcessFile[i].uiSecondReagent+g_tRunProcessFile[i].uiThirdReagent-g_tPistonMotor.uiRemainPulse-200;
                }
                if(g_tTestInfo[i].ucSampleType == 3)   //全血流程
                {
                    SPIMotor(3,MT_MOVE_TO,g_uiPistonPosition-(g_tRunProcessFile[i].uiThirdReagent_Wholeblood-200));//活塞电机排液
                    uiMixPulse = g_tRunProcessFile[i].uiSample_Wholeblood+g_tRunProcessFile[i].uiFistReagent_Wholeblood+g_tRunProcessFile[i].uiSecondReagent_Wholeblood+g_tRunProcessFile[i].uiThirdReagent_Wholeblood-g_tPistonMotor.uiRemainPulse-200;
                }
                if(uiMixPulse > 12500)
                    uiMixPulse = 12500;
                for(j=0;j<g_tRunProcessFile[i].ucMixTimes;j++)
                {
                    SPIMotor(3,MT_MOVE_TO,g_uiPistonPosition+uiMixPulse);           //活塞电机混匀
                    SPIMotor(3,MT_MOVE_TO,g_uiPistonPosition-(uiMixPulse-200));           //活塞电机混匀
                }
                SPIMotor(3,MT_MOVE_TO,g_uiPistonPosition+uiMixPulse);               //活塞电机吸液
                SPIMotor(2,MT_MOVE_TO,g_tTipVerticalMotor.uiOffLiquid);             //Tip头垂直电机运动到排液位置
                SPIMotor(3,SET_SPEED,g_tPistonMotor.ucSuctionSpeed);               //恢复活塞电机的速度
                SPIMotor(3,MT_MOVE_TO,g_uiPistonPosition-(uiMixPulse + 200*(g_tRunProcessFile[i].ucMixTimes+1) + g_tPistonMotor.uiOverPulse));//活塞电机排液
                if(g_tProcessControl.ucStopFlag==0)
                    Delay1ms(100);                                                      //延迟100ms
                SPIMotor(2,MT_HOME,0);                                              //Tip头垂直电机复位
                LcdOffTipProcess();
                SPIMotor(3,MT_HOME,0);                                              //活塞电机复位
                g_tTestInfo[i].ucStartTimingFlag = 1;                               //该通道开始计时
                
                ucRunProcess2Num++;                                                 //记录运行第二步流程的通道数
            }
            if(g_tProcessControl.ucStopFlag==1)
            {
                break;
            }
        }
        if((ucRunProcess2Num == ucRunProcess1Num) || (g_tProcessControl.ucStopFlag==1))
        {
            break;
        }
    }
    SPIMotor(4,MT_HOME,0);      //试剂盘电机复位
    
    while(1)    //流程3：进行检测
    {
        for(i=0;i<12;i++)
        {
            RunLcdConmunication();
            //if((g_tTestInfo[i].ucSetOkFlag == 1)&&(g_tTestInfo[i].uiTimer>=g_tRunProcessFile[i].uiProcess2Timer)&&(g_tTestInfo[i].uiTimer < (1.1*g_tRunProcessFile[i].uiProcess2Timer)))
            if((g_tTestInfo[i].ucSetOkFlag == 1)&&(g_tTestInfo[i].uiTimer>=g_tRunProcessFile[i].uiProcess2Timer)&&(g_tProcessControl.ucStopFlag==0))
            {
                g_tTestInfo[i].ucStartTimingFlag = 0;                                 //该通道关闭计时
                g_tTestInfo[i].uiTimer           = 0;                                 //计时变量清零
                g_tTestInfo[i].ucTestedFlag      = 1;
                SPIMotor(4,MT_MOVE_TO,g_tReagentTrayMotor.uiTestPosition[i]);         //试剂盘电机运动到检测位置
                SPIMotor(5,MT_MOVE_TO,g_uiPmtVerticalTestPosition);                   //PMT垂直电机运动到密光检测位置
                PrePmtProcess1();//PMT开5S
                if(ucFistTestFlag == 0)                                               //PMT预采集
                {
                    ucFistTestFlag = 1;
                    PrePmtProcess(i);
                }
                PrePmtProcess(i);
                PmtProcess(i);
                LcdCalculateConcentration(i);
                SPIMotor(5,MT_HOME,0);                                                //PMT垂直电机复位
                DisplayTestRestult(i,0);                                              //显示检测结果
                if(i < 6)//测试完成后反选
                {
                    CutPicture(0x0390+0x0010*i,TEST_PICTURE1_BUTTON,10,uiDivisionLine1[i],588,uiDivisionLine1[i+1],10,uiDivisionLine1[i]);
                }
                else if(i < 12)
                {
                    CutPicture(0x0390+0x0010*i,TEST_PICTURE2_BUTTON,10,uiDivisionLine1[i-6],588,uiDivisionLine1[i-6+1],10,uiDivisionLine1[i-6]);
                }
                GetSystemTime(g_tTestInfo[i].ucTestTime);
                SaveTestResult(i);
                if(g_ucLisInterface > 0)
                {
                    LisTxResultRecord(i);    //LIS系统发送结果
                }
                ucRunProcess3Num++;
                SaveHistoryStatistics(i);   //统计项目测试个数
                g_tTestInfo[i].ucSetOkFlag = 0;
            }
            if(g_tProcessControl.ucStopFlag==1)
            {
                break;
            }
        }
        if((ucRunProcess3Num == ucRunProcess1Num) || (g_tProcessControl.ucStopFlag==1))
        {
            break;
        }
    }
    
    if(g_tProcessControl.ucStopFlag == 1)
    {
        g_tProcessControl.ucStopFlag = 0;
        SPIMotor(2,MT_HOME,0);                             //Tip头垂直电机复位
        LcdOffTipProcess();
    }
    
    InitTimer5(0);
    UpdateProcessBar(1);        //显示满进度条
    SPIMotor(3,MT_HOME,0);      //活塞电机复位
    SPIMotor(3,MT_MOVE_TO,6000); //活塞电机
    SPIMotor(2,MT_HOME,0);      //Tip头垂直电机复位
    SPIMotor(5,MT_HOME,0);      //PMT垂直电机复位
    //SPIMotor(4,MT_HOME,0);      //试剂盘电机复位
    SPIMotor(4,MT_MOVE_TO_FW,0);           //放置通道1和2
    SPIMotor(1,MT_HOME,0);      //Tip头水平电机复位
    g_tProcessControl.ucRunningFlag = 0;
    g_tProcessControl.ucStopFlag    = 0;
    g_tLcdAnalyticIns.uiPageNum = 1;
    DisplayPicture(TEST_PICTURE1);
    
    Delay1ms(50); 
    ucRunProcess1Num = ucRunProcess1Num*2;
    g_uiTipCounterNum = g_uiTipCounterNum + ucRunProcess1Num;
    WriteWasteTipsNum();
    
    if(g_tSystemSetting.ucAutoPrint == 1)
    {
        g_ucPrintLock = 1;
        AutoPrint_Flag = 1;
        LcdPrintTestResult();
        AutoPrint_Flag = 0;
        g_ucPrintLock = 0;
    }
    for(i=0;i<12;i++)
    {
        if(i < 6)
        {
            CutPicture(0x0390+0x0010*i,TEST_PICTURE1,10,uiDivisionLine1[i],588,uiDivisionLine1[i+1],10,uiDivisionLine1[i]);
        }
        else if(i < 12)
        {
            CutPicture(0x0390+0x0010*i,TEST_PICTURE2,10,uiDivisionLine1[i-6],588,uiDivisionLine1[i-6+1],10,uiDivisionLine1[i-6]);
        }    
    }
    InitDisplay();//删除测试通道信息
    
}
