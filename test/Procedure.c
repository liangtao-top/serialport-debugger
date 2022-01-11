
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
 * �������ƣ�WriteWasteTipsNum
 * ������������¼����Ͱ��Tip������
 * �����������
 * �����������
 * ����ֵ  ����
 * �޸�����            �汾��            �޸���            �޸�����
 * -----------------------------------------------------------------------------
 * 2017/09/19          V1.0             ��־ǿ               ����
 *******************************************************************************/
void WriteWasteTipsNum(void)
{
    //unsigned long int uliAddr = (0xB5 - 0x80) * 4;
    unsigned long int uliAddr = 112001024;
    FlashWrite4BytesValue(uliAddr,(unsigned long int)g_uiTipCounterNum);
}

/*******************************************************************************
 * �������ƣ�IsExpireDate
 * �����������ж�date�������Ƿ񳬹��˵�ǰ����
 * ���������date ���������
 * �����������
 * ����ֵ  ��1 δ����  0 �ѹ���
 * �޸�����            �汾��            �޸���            �޸�����
 * -----------------------------------------------------------------------------
 * 2017/09/20          V1.0             ��־ǿ               ����
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
 * �������ƣ�IsPlaceKit
 * ��������������Ƿ�������Լ���
 * ���������num:����ͨ����
 * �����������
 * ����ֵ  ��ucPlaceKitTipFlag: ����λ(2��1��0)�ֱ��ʾTIP2 TIP1 �Լ��� 0��ʾδ���� 1��ʾ�ѷ���  
 * �޸�����            �汾��            �޸���            �޸�����
 * -----------------------------------------------------------------------------
 * 2017/09/05          V1.0             ��־ǿ               ����
 *******************************************************************************/
//static unsigned char IsPlaceKit(unsigned char num)
unsigned char IsPlaceKit(unsigned char num)
{
    #define DELAY_TIME  200
    g_tTestInfo[num].ucPlaceKitTipFlag = 0;
    //����Ƿ������Tip1
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
    //����Ƿ������Tip2
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
    //����Ƿ�������Լ���
    SPIMotor(4,MT_MOVE_TO,g_tReagentTrayMotor.uiIsPlaceKitSensorPosition[num]);
    Delay1ms(DELAY_TIME);
    if(KIT_SENSOR == 0) //�������Լ���
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
 * �������ƣ�IsChannelsSetOk
 * �������������ÿ��ͨ���Ƿ��������
 * �����������
 * �����������
 * ����ֵ  ��1:����������һ��ͨ��  2:���ؼ�������
 * �޸�����            �汾��            �޸���            �޸�����
 * -----------------------------------------------------------------------------
 * 2017/03/28          V1.0             ��־ǿ               ����
 *******************************************************************************/
static unsigned char IsChannelsSetOk(void)
{
    unsigned char i=0,num=0;
    unsigned char ucNum=0;
    unsigned char ucTemp[30];
    unsigned char ucString1[] = "����ͨ��δ�ҵ���׼������Ϣ��";
    unsigned char ucString2[] = "����ͨ��δ�����Լ�����";
    unsigned char ucString3[] = "����ͨ��δ���õ�һTip��";
    unsigned char ucString4[] = "����ͨ��δ���õڶ�Tip��";
    unsigned char ucString5[] = "����ͨ���Լ����Ѿ����ڣ�";
    unsigned char ucAddrNum=0;
    unsigned int uiAddr = 0x04A0;
    unsigned char IsPlaceKitFlag=0;
    if(g_tProcessControl.ucStopFlag == 1)   //���������������̵�ֹͣ���Բ���
        return 1;
    for(i=0;i<12;i++)
    {
        g_tTestInfo[i].ucSetOkFlag = 0;
        if(g_tTestInfo[i].ucName[0] != '\0')
        {
            if(0 == FindReadReagentDetailInfo(i))   //��ȡ�ɹ�
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
    if(g_tProcessControl.ucStopFlag == 1)   //���������������̵�ֹͣ���Բ���
        return 1;
    if(ucNum > 0)
    {
        InitTimer5(0);  //�ر����̼�ʱ�Ķ�ʱ��
        ucNum = 0;
        memset(ucTemp,'\0',sizeof(ucTemp));
        DisplayPicture(34);
        Delay1ms(200);//�ӳ�500ms     
        for(i=0;i<10;i++)
        {
            DisplayText(uiAddr + 0x0010 * i,'\0');
        }
        //�Ƿ��ȡ�˱�׼������Ϣ
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
            //�Ƿ�������Լ���
            ucNum = 0;
            memset(ucTemp,'\0',sizeof(ucTemp));
            for(i=0;i<12;i++)
            {
                if((g_tTestInfo[i].ucName[0] != '\0') && ((g_tTestInfo[i].ucPlaceKitTipFlag&0x01) == 0) && (g_tSystemSetting.ucInputSampleIDMethod == 0))//�ֶ�ɨ�����Լ���
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

            //�Ƿ������Tip1
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

            //�Ƿ������Tip2
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

            //�Լ���Ϣ�Ƿ����
            ucNum = 0;
            memset(ucTemp,'\0',sizeof(ucTemp));
            for(i=0;i<12;i++)
            {
                if(g_tTestInfo[i].ucName[0] != '\0')
                {
                    if(0 == IsExpireDate(g_tTestInfo[i].ucExpiryDate))  //��ʾ�Ѿ�����
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
        if((g_tLcdAnalyticIns.uiLcdParamAddr == 0x0006) && (g_tLcdAnalyticIns.uiButtonValue == 1))  //����
        {
            //DisplayPicture(g_ucPrevousPlusPic);//
            g_tLcdAnalyticIns.uiPageNum = 1;
            DisplayPicture(TEST_PICTURE1);
            SPIMotor(4,MT_HOME,0);      //�Լ��̵����λ
            return 2;   //���ؼ�������
        }
    }                    
    return 1;   //����������һ��ͨ��
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
 * �������ƣ�TakeTipProcess
 * ����������ȡTipͷ����
 * �����������
 * �����������
 * ����ֵ  ����
 * �޸�����            �汾��            �޸���            �޸�����
 * -----------------------------------------------------------------------------
 * 2017/03/29          V1.0             ��־ǿ               ����
 *******************************************************************************/
unsigned int GetAD(void)  //�ɼ�1��10us
{
	AD1CHS0bits.CH0SA = 11;
	AD1CON1bits.ADON = 1;				//��ADת��
	AD1CON1bits.SAMP = 1;				//����A/D����
	while (!AD1CON1bits.DONE);			//�ȴ�ADת������
	return (ADC1BUF0);
}

void GetPmtData(void)
{
    unsigned int m=0,k=0,i=0;
    g_uliPulseValue = 0;
    g_uliADValue    = 0;
    InitTimer1(1);
    for(k=0;k<500;k++)  //�ɼ�500ms
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
 * �������ƣ�LogisticOperation
 * ���������������Ĳ�����ʽ����ValueŨ�ȶ�Ӧ���ź�ֵ
 * ���������Param �Ĳ������ĸ�ϵ��  Value Ũ��ֵ
 * �����������
 * ����ֵ  ���ź�ֵ
 * �޸�����            �汾��            �޸���            �޸�����
 * -----------------------------------------------------------------------------
 * 2017/12/07          V1.0             ��־ǿ               ����
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
    //�Ĳ�����ʽ  y=(a-d)/(1+(x/c)^b) + d;
    fTemp = Value/fC;
    fTemp = pow(fTemp,fB) + 1.0;
    fTemp = (fA - fD)/fTemp + fD;
    
    uliValue = (unsigned long int)(fTemp + 0.5);
    return uliValue;
}
/*******************************************************************************
 * �������ƣ�LogisticOperation1
 * ��������������Logistic����ValueŨ�ȶ�Ӧ���ź�ֵ
 * ���������Param �Ĳ������ĸ�ϵ��  Value Ũ��ֵ
 * �����������
 * ����ֵ  ���ź�ֵ
 * �޸�����            �汾��            �޸���            �޸�����
 * -----------------------------------------------------------------------------
 * 2017/12/07          V1.0             ��־ǿ               ����
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
 * �������ƣ�LogisticInverseOperation
 * ���������������ź�ֵ�����Ĳ�������Ũ��ֵ
 * ���������Param �Ĳ������ĸ�ϵ��  Value �ź�ֵ
 * �����������
 * ����ֵ  ������õ���Ũ��ֵ
 * �޸�����            �汾��            �޸���            �޸�����
 * -----------------------------------------------------------------------------
 * 2017/09/10          V1.0             ��־ǿ               ����
 *******************************************************************************/
//�Ĳ����������㣬�����ź�ֵ����Ũ��
//ParamΪ�Ĳ�����ϵ����ValueΪ�ź�ֵ
//�Ĳ�����ʽ  y=(a-d)/(1+(x/c)^b) + d;
//����: x=c*(((a-d)/y-d -1)^(1/b));
//��Value<fD����Value>fAʱ������-1
//����ֵ�������Ũ��ֵ
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
 * �������ƣ�LogisticInverseOperation1
 * ���������������ź�ֵ�����Ĳ�������Ũ��ֵ
 * ���������Param �Ĳ������ĸ�ϵ��  Value �ź�ֵ
 * �����������
 * ����ֵ  ������õ���Ũ��ֵ
 * �޸�����            �汾��            �޸���            �޸�����
 * -----------------------------------------------------------------------------
 * 2017/09/10          V1.0             ��־ǿ               ����
 *******************************************************************************/
//�Ĳ����������㣬�����ź�ֵ����Ũ��
//ParamΪ�Ĳ�����ϵ����ValueΪ�ź�ֵ
//�Ĳ�����ʽ  y=(a-d)/(1+(x/c)^b) + d;
//����: x=c*(((a-d)/y-d -1)^(1/b));
//��Value<fD����Value>fAʱ������-1
//����ֵ�������Ũ��ֵ
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
 * �������ƣ�LineOperation
 * ��������������ֱ�ߵ�k,bֵ����Ũ��
 * ���������Param �Ĳ������ĸ�ϵ�� Param[2]Ϊkֵ Param[3]Ϊbֵ  Value Ũ��ֵ
 * �����������
 * ����ֵ  ������õ����ź�ֵ
 * �޸�����            �汾��            �޸���            �޸�����
 * -----------------------------------------------------------------------------
 * 2017/12/07          V1.0             ��־ǿ               ����
 *******************************************************************************/
static unsigned long int LineOperation(float *Param,float Value)
{
    unsigned long int uliValue = 0;
    uliValue = Param[2] * Value + Param[3];
    return uliValue;
}

/*******************************************************************************
 * �������ƣ�LineInverseOperation
 * ��������������ֱ�ߵ�k,bֵ����Ũ��
 * ���������Param �Ĳ������ĸ�ϵ��  Value �ź�ֵ
 * �����������
 * ����ֵ  ������õ���Ũ��ֵ
 * �޸�����            �汾��            �޸���            �޸�����
 * -----------------------------------------------------------------------------
 * 2017/09/10          V1.0             ��־ǿ               ����
 *******************************************************************************/
static float LineInverseOperation(float *Param,unsigned long int Value)
{
    float fValue=0;
    fValue = ((float)Value -Param[3])/Param[2];
    return fValue;
}

/*******************************************************************************
 * �������ƣ�CalculateSignalValue
 * �������������ݴ����ϵ��Param������Ũ�ȶ�Ӧ���ź�ֵ�����Param[0]������0��������Ĳ�����
 *           ����Ϊֱ�ߣ�����Param[2]����k,Param[3]����b  
 * ���������Param �Ĳ������ĸ�ϵ��  fValue Ũ��ֵ
 * �����������
 * ����ֵ  ������õ����ź�ֵ
 * �޸�����            �汾��            �޸���            �޸�����
 * -----------------------------------------------------------------------------
 * 2017/12/07          V1.0             ��־ǿ               ����
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
 * �������ƣ�CalculateConcentration
 * �������������ݴ����ϵ��Param������Ũ�ȣ����Param[0]������0��������Ĳ�����
 *           ����Ϊֱ�ߣ�����Param[2]����k,Param[3]����b  
 * ���������Param �Ĳ������ĸ�ϵ��  Value �ź�ֵ
 * �����������
 * ����ֵ  ������õ���Ũ��ֵ
 * �޸�����            �汾��            �޸���            �޸�����
 * -----------------------------------------------------------------------------
 * 2017/09/10          V1.0             ��־ǿ               ����
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
 * �������ƣ�LcdCalculateConcentration
 * ��������������Ũ��
 * ���������num ͨ����
 * �����������
 * ����ֵ  ����
 * �޸�����            �汾��            �޸���            �޸�����
 * -----------------------------------------------------------------------------
 * 2017/09/10          V1.0             ��־ǿ               ����
 *******************************************************************************/
/* uliTransitionPeriodL1Ϊ�ϴ�ֵ uliTransitionPeriodH1Ϊ0��ʾ��һ����׼����
 * uliTransitionPeriodL1��Ϊ0��uliTransitionPeriodH1��Ϊ0��uliTransitionPeriodL2Ϊ�ϴ�ֵ��uliTransitionPeriodH2Ϊ0 ��ʾ��������׼����
 * uliTransitionPeriodL1��uliTransitionPeriodH1��uliTransitionPeriodL2��uliTransitionPeriodH2����Ϊ0��ʾ��������׼����
 * �ĸ�����Param[0]������0��������Ĳ���������Ϊֱ�ߣ�����Param[2]����k,Param[3]����b
 */
static void LcdCalculateConcentration(unsigned char num)
{
    unsigned char i=0;
    unsigned long int uliTransitionPeriodL1=0,uliTransitionPeriodL2=0,uliTransitionPeriodH1=0,uliTransitionPeriodH2=0;
    unsigned long int uliLowValue = 0,uliHighValue = 0;  //��ͺ���߼��Ũ�ȶ�Ӧ���ź�ֵ
    float fParam1[4],fParam2[4],fParam3[4];
    float fValue1=0,fValue2=0,fRatio=0;
    unsigned char SystemErrorAdjustmentFlag=0;
    //1�������������ͷ��䲻ͬ�ı�׼���ߣ�һ�������α�׼����
    if((g_tTestInfo[num].ucSampleType == 1) || (g_tTestInfo[num].ucSampleType == 2) || (g_tTestInfo[num].ucSampleType == 4) || (g_tTestInfo[num].ucSampleType == 5))    //Ѫ�塢��Ѫ�����ʿ�Ʒ
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
    else if(g_tTestInfo[num].ucSampleType == 3) //ԭѪ
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
    
    //2�����ݱ�׼���߼�����ͺ���߼��Ũ�ȶ�Ӧ���ź�ֵ
    uliLowValue = CalculateSignalValue(fParam1,g_tStandCurve[num].fLowLimit);       //������ͼ��Ũ�ȶ�Ӧ���ź�ֵ
    if(uliTransitionPeriodH1 == 0)  //��ʾֻ��һ����׼����
        uliHighValue = CalculateSignalValue(fParam1,g_tStandCurve[num].fHighLimit); //������߼��Ũ�ȶ�Ӧ���ź�ֵ
    else if(uliTransitionPeriodH2 == 0)  //��ʾֻ��������׼����
        uliHighValue = CalculateSignalValue(fParam2,g_tStandCurve[num].fHighLimit); //������߼��Ũ�ȶ�Ӧ���ź�ֵ
    else    //��ʾ��������׼����
        uliHighValue = CalculateSignalValue(fParam3,g_tStandCurve[num].fHighLimit); //������߼��Ũ�ȶ�Ӧ���ź�ֵ
    
    //��ȡϵͳ������ϵ�����������
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
    //3�����ɼ����ź�ֵ��uliLowValue��uliHighValue���бȽϣ��ж��Ƿ񳬳��˼��ķ�Χ
    if(g_tTestInfo[num].uliFitValue < uliLowValue)
    {
        g_tTestInfo[num].uliFitValue = (unsigned long int)((float)g_tTestInfo[num].uliFitValue*g_tSystemErrorAdjustment.ucKValue + g_tSystemErrorAdjustment.ucBValue);
        SystemErrorAdjustmentFlag=1;
        if(g_tTestInfo[num].uliFitValue < uliLowValue)
        {
            g_tTestInfo[num].ucResultFlag = 2;  //С��
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
            g_tTestInfo[num].ucResultFlag = 3;  //����
            g_tTestInfo[num].fResult = g_tStandCurve[num].fHighLimit;
            return;
        }
    }

    //3���ڼ��Ũ�ȷ�Χ�ڣ�����Ũ��
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
    g_tTestInfo[num].ucResultFlag = 1;  //����
    g_tTestInfo[num].fResult = fValue1;

    //srand((unsigned)time(NULL));

    //ϵͳ������
    if(SystemErrorAdjustmentFlag==0)
        g_tTestInfo[num].fResult = g_tTestInfo[num].fResult*g_tSystemErrorAdjustment.ucKValue + g_tSystemErrorAdjustment.ucBValue;

    if(g_tTestInfo[num].fResult < g_tStandCurve[num].fLowLimit)
    {
        g_tTestInfo[num].ucResultFlag = 2;  //С��
        g_tTestInfo[num].fResult = g_tStandCurve[num].fLowLimit;
        return;
    }
    else if(g_tTestInfo[num].fResult > g_tStandCurve[num].fHighLimit)
    {
        g_tTestInfo[num].ucResultFlag = 3;  //����
        g_tTestInfo[num].fResult = g_tStandCurve[num].fHighLimit;
        return;
    }
}

/*******************************************************************************
 * �������ƣ�PmtFittingValue
 * ������������pmt������ֵ�͵�ѹֵ���Ϊ�ź�ֵ
 * ���������Pulse ����ֵ  ADValue ��ѹֵ
 * �����������
 * ����ֵ  ����Ϻ��ֵ
 * �޸�����            �汾��            �޸���            �޸�����
 * -----------------------------------------------------------------------------
 * 2017/09/12          V1.0             ��־ǿ               ����
 *******************************************************************************/
unsigned long int PmtFittingValue(unsigned long int Pulse,unsigned long int ADValue)
{
    unsigned long int uliValue = 0,uliValue1 = 0,uliValue2 = 0;
    unsigned long int uliFitValue = 0;
    float fValue=0,fRatio = 0;
    
    //��PMT�źŵ������AD���Ϊһ��
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

    //����̨�����
    if(uliFitValue <= g_tPmtFitValue.uliPmtsFitLowValue[0])
    {

    }
    else if(uliFitValue <= g_tPmtFitValue.uliPmtsFitHighValue[0])   //����1�������
    {
        if((uliFitValue >= g_tPmtFitValue.uliPmtsFitLowValue[1]) && (g_tPmtFitValue.uliPmtsFitLowValue[1] != 0))  //�����н���
        {
            uliValue1   = (unsigned long int)(g_tPmtFitValue.fPmtsFitK[0] * ((float)uliFitValue) + g_tPmtFitValue.fPmtsFitB[0]);
            uliValue2   = (unsigned long int)(g_tPmtFitValue.fPmtsFitK[1] * ((float)uliFitValue) + g_tPmtFitValue.fPmtsFitB[1]);
            fRatio      = ((float)(uliFitValue - g_tPmtFitValue.uliPmtsFitLowValue[1]))/((float)(g_tPmtFitValue.uliPmtsFitHighValue[0] - g_tPmtFitValue.uliPmtsFitLowValue[1]));
            uliFitValue = (unsigned long int)((1 - fRatio) * uliValue1 + fRatio * uliValue2 + 0.5);
        }
        else    //����û�н���
        {
            uliFitValue = (unsigned long int)(g_tPmtFitValue.fPmtsFitK[0] * ((float)uliFitValue) + g_tPmtFitValue.fPmtsFitB[0]);
        }
    }
    else if(uliFitValue <= g_tPmtFitValue.uliPmtsFitHighValue[1])   //����2�������
    {
        if((uliFitValue >= g_tPmtFitValue.uliPmtsFitLowValue[2]) && (g_tPmtFitValue.uliPmtsFitLowValue[2] != 0))  //�����н���
        {
            uliValue1   = (unsigned long int)(g_tPmtFitValue.fPmtsFitK[1] * ((float)uliFitValue) + g_tPmtFitValue.fPmtsFitB[1]);
            uliValue2   = (unsigned long int)(g_tPmtFitValue.fPmtsFitK[2] * ((float)uliFitValue) + g_tPmtFitValue.fPmtsFitB[2]);
            fRatio      = ((float)(uliFitValue - g_tPmtFitValue.uliPmtsFitLowValue[2]))/((float)(g_tPmtFitValue.uliPmtsFitHighValue[1] - g_tPmtFitValue.uliPmtsFitLowValue[2]));
            uliFitValue = (unsigned long int)((1 - fRatio) * uliValue1 + fRatio * uliValue2 + 0.5);
        }
        else    //����û�н���
        {
            uliFitValue = (unsigned long int)(g_tPmtFitValue.fPmtsFitK[1] * ((float)uliFitValue) + g_tPmtFitValue.fPmtsFitB[1]);
        }
    }
    else if(uliFitValue <= g_tPmtFitValue.uliPmtsFitHighValue[2])   //����3�������
    {
        if((uliFitValue >= g_tPmtFitValue.uliPmtsFitLowValue[3]) && (g_tPmtFitValue.uliPmtsFitLowValue[3] != 0))  //�����н���
        {
            uliValue1   = (unsigned long int)(g_tPmtFitValue.fPmtsFitK[2] * ((float)uliFitValue) + g_tPmtFitValue.fPmtsFitB[2]);
            uliValue2   = (unsigned long int)(g_tPmtFitValue.fPmtsFitK[3] * ((float)uliFitValue) + g_tPmtFitValue.fPmtsFitB[3]);
            fRatio      = ((float)(uliFitValue - g_tPmtFitValue.uliPmtsFitLowValue[3]))/((float)(g_tPmtFitValue.uliPmtsFitHighValue[2] - g_tPmtFitValue.uliPmtsFitLowValue[3]));
            uliFitValue = (unsigned long int)((1 - fRatio) * uliValue1 + fRatio * uliValue2 + 0.5);
        }
        else    //����û�н���
        {
            uliFitValue = (unsigned long int)(g_tPmtFitValue.fPmtsFitK[2] * ((float)uliFitValue) + g_tPmtFitValue.fPmtsFitB[2]);
        }
    }
    else if(uliFitValue <= g_tPmtFitValue.uliPmtsFitHighValue[3])   //����4�������
    {
        if((uliFitValue >= g_tPmtFitValue.uliPmtsFitLowValue[4]) && (g_tPmtFitValue.uliPmtsFitLowValue[4] != 0))  //�����н���
        {
            uliValue1   = (unsigned long int)(g_tPmtFitValue.fPmtsFitK[3] * ((float)uliFitValue) + g_tPmtFitValue.fPmtsFitB[3]);
            uliValue2   = (unsigned long int)(g_tPmtFitValue.fPmtsFitK[4] * ((float)uliFitValue) + g_tPmtFitValue.fPmtsFitB[4]);
            fRatio      = ((float)(uliFitValue - g_tPmtFitValue.uliPmtsFitLowValue[4]))/((float)(g_tPmtFitValue.uliPmtsFitHighValue[3] - g_tPmtFitValue.uliPmtsFitLowValue[4]));
            uliFitValue = (unsigned long int)((1 - fRatio) * uliValue1 + fRatio * uliValue2 + 0.5);
        }
        else    //����û�н���
        {
            uliFitValue = (unsigned long int)(g_tPmtFitValue.fPmtsFitK[3] * ((float)uliFitValue) + g_tPmtFitValue.fPmtsFitB[3]);
        }
    }
    else if(uliFitValue <= g_tPmtFitValue.uliPmtsFitHighValue[4])   //����5�������
    {
        if((uliFitValue >= g_tPmtFitValue.uliPmtsFitLowValue[5]) && (g_tPmtFitValue.uliPmtsFitLowValue[5] != 0))  //�����н���
        {
            uliValue1   = (unsigned long int)(g_tPmtFitValue.fPmtsFitK[4] * ((float)uliFitValue) + g_tPmtFitValue.fPmtsFitB[4]);
            uliValue2   = (unsigned long int)(g_tPmtFitValue.fPmtsFitK[5] * ((float)uliFitValue) + g_tPmtFitValue.fPmtsFitB[5]);
            fRatio      = ((float)(uliFitValue - g_tPmtFitValue.uliPmtsFitLowValue[5]))/((float)(g_tPmtFitValue.uliPmtsFitHighValue[4] - g_tPmtFitValue.uliPmtsFitLowValue[5]));
            uliFitValue = (unsigned long int)((1 - fRatio) * uliValue1 + fRatio * uliValue2 + 0.5);
        }
        else    //����û�н���
        {
            uliFitValue = (unsigned long int)(g_tPmtFitValue.fPmtsFitK[4] * ((float)uliFitValue) + g_tPmtFitValue.fPmtsFitB[4]);
        }
    }
    else if(uliFitValue <= g_tPmtFitValue.uliPmtsFitHighValue[5])   //����6�������
    {
        if((uliFitValue >= g_tPmtFitValue.uliPmtsFitLowValue[6]) && (g_tPmtFitValue.uliPmtsFitLowValue[6] != 0))  //�����н���
        {
            uliValue1   = (unsigned long int)(g_tPmtFitValue.fPmtsFitK[5] * ((float)uliFitValue) + g_tPmtFitValue.fPmtsFitB[5]);
            uliValue2   = (unsigned long int)(g_tPmtFitValue.fPmtsFitK[6] * ((float)uliFitValue) + g_tPmtFitValue.fPmtsFitB[6]);
            fRatio      = ((float)(uliFitValue - g_tPmtFitValue.uliPmtsFitLowValue[6]))/((float)(g_tPmtFitValue.uliPmtsFitHighValue[5] - g_tPmtFitValue.uliPmtsFitLowValue[6]));
            uliFitValue = (unsigned long int)((1 - fRatio) * uliValue1 + fRatio * uliValue2 + 0.5);
        }
        else    //����û�н���
        {
            uliFitValue = (unsigned long int)(g_tPmtFitValue.fPmtsFitK[5] * ((float)uliFitValue) + g_tPmtFitValue.fPmtsFitB[5]);
        }
    }
    else if(uliFitValue <= g_tPmtFitValue.uliPmtsFitHighValue[6])   //����7�������
    {
        if((uliFitValue >= g_tPmtFitValue.uliPmtsFitLowValue[7]) && (g_tPmtFitValue.uliPmtsFitLowValue[7] != 0))  //�����н���
        {
            uliValue1   = (unsigned long int)(g_tPmtFitValue.fPmtsFitK[6] * ((float)uliFitValue) + g_tPmtFitValue.fPmtsFitB[6]);
            uliValue2   = (unsigned long int)(g_tPmtFitValue.fPmtsFitK[7] * ((float)uliFitValue) + g_tPmtFitValue.fPmtsFitB[7]);
            fRatio      = ((float)(uliFitValue - g_tPmtFitValue.uliPmtsFitLowValue[7]))/((float)(g_tPmtFitValue.uliPmtsFitHighValue[6] - g_tPmtFitValue.uliPmtsFitLowValue[7]));
            uliFitValue = (unsigned long int)((1 - fRatio) * uliValue1 + fRatio * uliValue2 + 0.5);
        }
        else    //����û�н���
        {
            uliFitValue = (unsigned long int)(g_tPmtFitValue.fPmtsFitK[6] * ((float)uliFitValue) + g_tPmtFitValue.fPmtsFitB[6]);
        }
    }
    else if(g_tPmtFitValue.uliPmtsFitHighValue[7] != 0)    //����8�������
    {
            uliFitValue = (unsigned long int)(g_tPmtFitValue.fPmtsFitK[7] * ((float)uliFitValue) + g_tPmtFitValue.fPmtsFitB[7]);
    }
        
    return uliFitValue;
}

/*******************************************************************************
 * �������ƣ�PmtProcess
 * ����������������pmt�ɼ�����
 * ���������channel ͨ����
 * �����������
 * ����ֵ  ����
 * �޸�����            �汾��            �޸���            �޸�����
 * -----------------------------------------------------------------------------
 * 2017/03/29          V1.0             ��־ǿ               ����
 *******************************************************************************/
void PmtProcess(unsigned char channel)
{
    Delay1ms(1500);
    SPISTARTHEAT(0);//�رտ���
    LASER = 1;
    Delay1ms(500);
    LASER = 0;
    Delay1ms(20);   //�ӳ�һ��ʱ�䣬��ʱ��ʹ���ͨ�ŵ�ʱ���൱
    PMTCTRL = 1;
    Delay1ms(20);//PMT�򿪺��ӳ�20ms�����ź�ƽ�Ⱥ����
    GetPmtData();
    PMTCTRL = 0;
    SPISTARTHEAT(1);//�򿪿���
    g_tTestInfo[channel].uliPulseValue = g_uliPulseValue;
    g_tTestInfo[channel].uliADValue    = g_uliADValue;
    g_tTestInfo[channel].uliFitValue   = PmtFittingValue(g_uliPulseValue,g_uliADValue);
}

/*******************************************************************************
 * �������ƣ�PrePmtProcess
 * ����������������Ԥ�ɼ����̣�����ÿ�ֵ�һ�����ݲɼ����ݲ��ȶ�����Ԥ�ɼ�һ��
 * ���������channel ͨ����
 * �����������
 * ����ֵ  ����
 * �޸�����            �汾��            �޸���            �޸�����
 * -----------------------------------------------------------------------------
 * 2017/03/29          V1.0             ��־ǿ               ����
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
 * �������ƣ�TakeTipProcess
 * ����������ȡTipͷ����
 * �����������
 * �����������
 * ����ֵ  ����
 * �޸�����            �汾��            �޸���            �޸�����
 * -----------------------------------------------------------------------------
 * 2017/03/29          V1.0             ��־ǿ               ����
 *******************************************************************************/
void LcdTakeTipProcess(void)
{
    SPIMotor(2,MT_MOVE_TO,g_tTipVerticalMotor.uiTakeTipPosition1);           //Tipͷ��ֱ����˶���ȡTipͷλ��1
    if(g_tProcessControl.ucStopFlag==0)
        Delay1ms(100);                                                           //�ӳ�100ms
    SPIMotor(2,SET_SPEED,g_tTipVerticalMotor.ucTakeTipSlowSpeed);            //����Tipͷ��ֱ����˶��ٶ�Ϊ����
    SPIMotor(2,MT_MOVE_TO,g_tTipVerticalMotor.uiTakeTipPosition2);           //Tipͷ��ֱ����˶���ȡTipͷλ��2
    SPIMotor(2,SET_SPEED,240);                                            //�ָ�Tipͷ��ֱ������ٶ�
    if(g_tProcessControl.ucStopFlag==0)
        Delay1ms(300);                                                           //�ӳ�300ms
    SPIMotor(2,MT_HOME,0);                                                   //Tipͷ��ֱ�����λ
}

/*******************************************************************************
 * �������ƣ�LcdOffTipProcess
 * ������������Tipͷ����
 * �����������
 * �����������
 * ����ֵ  ����
 * �޸�����            �汾��            �޸���            �޸�����
 * -----------------------------------------------------------------------------
 * 2017/03/29          V1.0             ��־ǿ               ����
 *******************************************************************************/
void LcdOffTipProcess(void)
{
    SPIMotor(1,MT_MOVE_TO,g_tTipHorizontalMotor.uiOffTipPosition1);          //Tipͷˮƽ����˶�����Tipͷλ��1
    SPIMotor(2,MT_MOVE_TO,g_tTipVerticalMotor.uiOffTip);                     //Tipͷ��ֱ����˶�����Tipͷλ��
    SPIMotor(1,MT_MOVE_TO,g_tTipHorizontalMotor.uiOffTipPosition2);          //Tipͷˮƽ����˶�����Tipͷλ��2
    SPIMotor(2,MT_HOME,0);                                                   //Tipͷ��ֱ�����λ
}

/*******************************************************************************
 * �������ƣ�LcdSuctionLiquid
 * ����������ȡ�������Լ�����
 * ���������uiTipHorizontal��Tipˮƽ����˶�����λ�ã�uiSuctionLiquid����Һ����
 * �����������
 * ����ֵ  ����
 * �޸�����            �汾��            �޸���            �޸�����
 * -----------------------------------------------------------------------------
 * 2017/03/29          V1.0             ��־ǿ               ����
 *******************************************************************************/
void LcdSuctionLiquid(unsigned int uiTipHorizontal,unsigned int uiSuctionLiquid)
{
    if(uiSuctionLiquid > 0)
    {
        SPIMotor(1,MT_MOVE_TO,uiTipHorizontal);                                 //Tipͷˮƽ����˶�����λ��
        SPIMotor(2,MT_MOVE_TO,g_tTipVerticalMotor.uiSuctionLiquidPosition);     //Tipͷ��ֱ����˶���ȡҺλ��
        if(g_tProcessControl.ucStopFlag==0)
            Delay1ms(50);                                                           //�ӳ�50ms
        SPIMotor(3,MT_MOVE_TO,g_uiPistonPosition+uiSuctionLiquid);              //�����������һ�Լ�
        if(g_tProcessControl.ucStopFlag==0)
            Delay1ms(200);                                                          //�ӳ�200ms
        SPIMotor(2,MT_HOME,0);                                                  //Tipͷ��ֱ�����λ
    }
}

/*******************************************************************************
 * �������ƣ�LcdMixLiquid
 * ��������������������������
 * ���������uiTipHorizontal��Tipˮƽ����˶�����λ�ã�uiSuctionLiquid����Һ����
 * �����������
 * ����ֵ  ����
 * �޸�����            �汾��            �޸���            �޸�����
 * -----------------------------------------------------------------------------
 * 2018/04/17          V1.0             ����               ����
 *******************************************************************************/
static void LcdMixLiquid(unsigned int uiTipHorizontal,unsigned int uiSuctionLiquid)
{
    unsigned char k;
    if(uiSuctionLiquid > 0)
    {
        SPIMotor(1,MT_MOVE_TO,uiTipHorizontal);                                 //Tipͷˮƽ����˶�����λ��
        SPIMotor(2,MT_MOVE_TO,g_tTipVerticalMotor.uiSuctionLiquidPosition);     //Tipͷ��ֱ����˶���ȡҺλ��
        SPIMotor(3,SET_SPEED,g_tPistonMotor.ucMixSpeed);                        //��������Ļ����ٶ�
        if(g_tProcessControl.ucStopFlag==0)
            Delay1ms(50);                                                       //�ӳ�50ms
        for(k=0;k<2;k++)
        {
            SPIMotor(3,MT_MOVE_TO,g_uiPistonPosition + uiSuctionLiquid);        //�����������
            SPIMotor(3,MT_MOVE_TO,g_uiPistonPosition - (uiSuctionLiquid-20));   //�����������
        }
        SPIMotor(3,MT_MOVE_TO,g_uiPistonPosition + uiSuctionLiquid);               //���������Һ
        SPIMotor(2,MT_MOVE_TO,g_tTipVerticalMotor.uiOffLiquid);                 //Tipͷ��ֱ����˶�����Һλ��
        SPIMotor(3,SET_SPEED,g_tPistonMotor.ucSuctionSpeed);                    //�ָ�����������ٶ�
        //SPIMotor(3,MT_HOME,0);   //���������Һ
        SPIMotor(3,MT_MOVE_TO,g_uiPistonPosition - (uiSuctionLiquid + (k+1)*40));//���������Һ
        if(g_tProcessControl.ucStopFlag==0)
            Delay1ms(200);                                                          //�ӳ�200ms
        SPIMotor(2,MT_HOME,0);                                                  //Tipͷ��ֱ�����λ
    }
}

/*******************************************************************************
 * �������ƣ�InitStartProcess
 * ������������ʼ����������ǰ�ı����Ͳ���
 * �����������
 * �����������
 * ����ֵ  ��0:�ɹ�  1:ʧ��
 * �޸�����            �汾��            �޸���            �޸�����
 * -----------------------------------------------------------------------------
 * 2017/03/21          V1.0             ��־ǿ               ����
 *******************************************************************************/
static unsigned char InitStartProcess(void)
{
    unsigned char i = 0,k = 0;
    unsigned char ucTestChannelNo = 0;
    unsigned char ucFlag = 0;   //�Ƿ��ҵ���ϸ��Ϣ��־λ�����ڻ�ȡ����ʱ��

    g_tProcessControl.uiRunTime     = 0;
    g_tProcessControl.ucStopFlag    = 0;
    
    if(g_tSystemSetting.ucInputSampleIDMethod == 1)//�Զ�ɨ��
    {
        AutoGetReagentBriefBarCode();
        if(AutoGetReagentBriefBarCodeFlag == 1)
        {
            return 1;
        }      
    }
    g_tProcessControl.ucRunningFlag = 1;
    for(i=0;i<12;i++)           //��ʱ��־������
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
            if(0 == FindReadReagentDetailInfo(i))   //��ȡ�ɹ�
            {
                k = i;
                ucFlag = 1;
            }
        }
    }
    
    if(ucTestChannelNo == 0)
    {
        SendDataToLcdRegister(0x4F,0x22);  //δ����ͨ����Ϣ����������ã� �Ի���
        while(g_ucUartLcdRecvFlag == 0); //�ȴ�ѡ��
        g_ucUartLcdRecvFlag = 0;
        AnalyticInstruction();
        if((g_tLcdAnalyticIns.uiLcdParamAddr == g_tTestPictureAddr.uiStartDialog) && (g_tLcdAnalyticIns.uiButtonValue == 1))
        {
            return 1;
        }
    }
    else
    {
        SPIMotor(3,MT_HOME,0);      //���������λ
        Delay1ms(1000);             //�ӳ�1s
        SPIMotor(3,MT_HOME,0);      //���������λ
        SPIMotor(2,MT_HOME,0);      //Tipͷ��ֱ�����λ
        SPIMotor(5,MT_HOME,0);      //PMT��ֱ�����λ
        SPIMotor(4,MT_HOME,0);      //�Լ��̵����λ
        SPIMotor(1,MT_HOME,0);      //Tipͷˮƽ�����λ//
    }
    
    DisplayPicture(TEST_RUN_PICTURE1);
    CutPicture(g_tTestPictureAddr.uiProcessBar,TEST_RUN_PICTURE1,0,52,599,120,0,52);
    if(ucFlag == 1) //�ҵ�
    {
        g_tProcessControl.uiTotalTime   = (g_tRunProcessFile[k].uiProcess1Timer/10 + g_tRunProcessFile[k].uiProcess2Timer/10);
        g_tProcessControl.uiTotalTime   += (unsigned int)(1.0 * (1030 - g_tProcessControl.uiTotalTime) / 12 * ucTestChannelNo);
        //g_tProcessControl.uiTotalTime   += 29 * (ucTestChannelNo-1) + 1;
    }
    else            //δ�ҵ���дһ��Ĭ��ֵ
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
 * �������ƣ�PushStartButton
 * �������������¡���ʼ�������Ĵ�����
 * �����������
 * ���������
 * ����ֵ  ����
 * �޸�����            �汾��            �޸���            �޸�����
 * -----------------------------------------------------------------------------
 * 2017/02/21          V1.0             ��־ǿ               ����
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
            SendDataToLcdRegister(0x4F,IS_START_PROCESS);  //�����Ի����Ƿ�ʼ���жԻ���
            while(g_ucUartLcdRecvFlag == 0); //�ȴ�ѡ��
            g_ucUartLcdRecvFlag = 0;
            AnalyticInstruction();
            if((g_tLcdAnalyticIns.uiLcdParamAddr == DIALOG_VALUE_ADDR) && (g_tLcdAnalyticIns.uiButtonValue == 1))   //��
            {
                if(g_uiTipCounterNum > g_uiTipLimitNum)
                {
                    SendDataToLcdRegister(0x4F,WASTE_TIP_DIALOG);  //������������Ͱ�Ի���
                    while(1)
                    {
                        if(DUSTBIN_SENSOR1 == 1 || g_ucUartLcdRecvFlag == 1)
                        {
                            SendDataToLcdRegister(0x4F,0x01);  //���������������Ͱ�Ի���
                            g_ucUartLcdRecvFlag = 0;
                            break;
                        }   
                    }
                    g_ucStartButtonLock = 0;
                    return;
                }
                if(DUSTBIN_SENSOR1 == 1) //δ��������Ͱ
                {
                    Delay1ms(50);
                    if(DUSTBIN_SENSOR1 == 1) //δ��������Ͱ
                    {
                        SendDataToLcdRegister(0x4F,DASTBIN_DIALOG);  //����δ��������Ͱ�Ի���
                        while(1)
                        {
                            if(DUSTBIN_SENSOR1 == 0 || g_ucUartLcdRecvFlag == 1)
                            {
                                SendDataToLcdRegister(0x4F,0x02);  //���������������Ͱ�Ի���
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
                if(1 == ucValue)            //����������һ��ͨ��
                {
                    LcdRunProcess();
                }
                else if(2 == ucValue)       //���ؼ�������
                {
                    InitTimer5(0);
                    g_tProcessControl.ucRunningFlag = 0;
                    g_tProcessControl.ucStopFlag = 0;
                    g_ucStartButtonLock = 0;
                } 
            }
            else        //��
            {

            }
            g_ucStartButtonLock = 0;
        }
    }
}

/*******************************************************************************
 * �������ƣ�PushNextButton
 * �������������¡�NEXT�������Ĵ�����
 * �����������
 * ���������
 * ����ֵ  ����
 * �޸�����            �汾��            �޸���            �޸�����
 * -----------------------------------------------------------------------------
 * 2017/02/21          V1.0             ��־ǿ               ����
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
                //SPIMotor(4,MT_HOME,0);                    //����ͨ��1��2
                SPIMotor(4,MT_MOVE_TO_FW,0);                //����ͨ��1��2
            }
            else if(g_ucNextPositionFlag == 1)
            {
                SPIMotor(4,MT_MOVE_TO,uliPulse);           //����ͨ��3��4
            }
            else if(g_ucNextPositionFlag == 2)
            {
                SPIMotor(4,MT_MOVE_TO,uliPulse);           //����ͨ��5��6
            }
            else if(g_ucNextPositionFlag == 3)
            {
                SPIMotor(4,MT_MOVE_TO,uliPulse);           //����ͨ��7��8
            }
            else if(g_ucNextPositionFlag == 4)
            {
                SPIMotor(4,MT_MOVE_TO,uliPulse);           //����ͨ��9��10
            }
            else if(g_ucNextPositionFlag == 5)
            {
                SPIMotor(4,MT_MOVE_TO,uliPulse);           //����ͨ��11��12
            }
            g_ucNextPositionButtonLock =0;
        }
    }
}

/*******************************************************************************
 * �������ƣ�LcdDilutionProcess
 * ����������ϡ�����̺���
 * ���������num ͨ����
 * �����������
 * ����ֵ  ����
 * �޸�����            �汾��            �޸���            �޸�����
 * -----------------------------------------------------------------------------
 * 2017/09/10          V1.0             ��־ǿ               ����
 *******************************************************************************/
static void LcdDilutionProcess(unsigned char num)
{
    unsigned char j=0;
    unsigned int uiMixPulse = 0;
    if((g_tTestInfo[num].ucDilute > 1) && ((g_tTestInfo[num].ucSampleType == 1) || (g_tTestInfo[num].ucSampleType == 2) || (g_tTestInfo[num].ucSampleType == 4) || (g_tTestInfo[num].ucSampleType == 5)))   //����ϡ��
    {
        SPIMotor(1,MT_MOVE_TO,g_tTipHorizontalMotor.uiDilution);     //Tipͷˮƽ����˶���ϡ��λ��
        uiMixPulse =  PulseForVolume(g_tRunProcessFile[num].uiDilutionVol + g_tRunProcessFile[num].uiSampleVol) - g_tPistonMotor.uiRemainPulse-200;
        if(uiMixPulse > 24500)
            SPIMotor(2,MT_MOVE_TO,g_tTipVerticalMotor.uiDilutionMixPosition-200);     //Tipͷ��ֱ����˶�����Һ����λ��
        else
            SPIMotor(2,MT_MOVE_TO,g_tTipVerticalMotor.uiDilutionMixPosition);     //Tipͷ��ֱ����˶�����Һ����λ��
        SPIMotor(3,SET_SPEED,g_tPistonMotor.ucMixSpeed);                      //��������Ļ����ٶ�
        SPIMotor(3,MT_MOVE_TO,g_uiPistonPosition-(g_tRunProcessFile[num].uiSample-200));//���������Һ
        if(uiMixPulse > 12500)
            uiMixPulse = 12500;
        for(j=1;j<g_tRunProcessFile[num].ucMixTimes;j++)
        {
            SPIMotor(3,MT_MOVE_TO,g_uiPistonPosition + uiMixPulse);           //�����������
            SPIMotor(3,MT_MOVE_TO,g_uiPistonPosition - (uiMixPulse-100));     //�����������
        }
        SPIMotor(3,MT_MOVE_TO,g_uiPistonPosition + uiMixPulse);               //���������Һ
        SPIMotor(2,MT_MOVE_TO,g_tTipVerticalMotor.uiOffLiquid);               //Tipͷ��ֱ����˶�����Һλ��
        SPIMotor(3,SET_SPEED,g_tPistonMotor.ucSuctionSpeed);                  //�ָ�����������ٶ�
        //SPIMotor(3,MT_MOVE_TO,g_uiPistonPosition-(uiMixPulse + 50*(g_tRunProcessFile[num].ucMixTimes+1)+ g_tPistonMotor.uiOverPulse));//���������Һ  
        SPIMotor(3,MT_HOME,0);      //���������λ
        if(g_tProcessControl.ucStopFlag==0)
            Delay1ms(100);
        SPIMotor(2,MT_HOME,0); 
        SPIMotor(3,MT_MOVE_TO,g_uiPistonPosition+g_tPistonMotor.uiPreSuction);   //�������Ԥ��Һ
        //����Ϊ��Һ����
        SPIMotor(2,MT_MOVE_TO,g_tTipVerticalMotor.uiDilutionMixPosition-100);                  //Tipͷ��ֱ����˶�������λ��       ȡҺλ��uiSuctionLiquidPosition
        if(g_tProcessControl.ucStopFlag==0)
            Delay1ms(50);                                                                        //
        
        SPIMotor(3,MT_MOVE_TO,g_uiPistonPosition + g_tRunProcessFile[num].uiDilutionSample); //���������Һ
        g_tRunProcessFile[num].uiSample = g_tRunProcessFile[num].uiDilutionSample;
        if(g_tProcessControl.ucStopFlag==0)
            Delay1ms(200);                                                                       //�ӳ�200ms
        SPIMotor(2,MT_HOME,0);                                                               //Tipͷ��ֱ�����λ
    }
    if((g_tTestInfo[num].ucDilute_Wholeblood > 1) && (g_tTestInfo[num].ucSampleType == 3))   //����ȫѪϡ��
    {
        SPIMotor(1,MT_MOVE_TO,g_tTipHorizontalMotor.uiDilution);     //Tipͷˮƽ����˶���ϡ��λ��
        uiMixPulse =  PulseForVolume(g_tRunProcessFile[num].uiDilutionVol_Wholeblood + g_tRunProcessFile[num].uiSampleVol_Wholeblood) - g_tPistonMotor.uiRemainPulse-200;
        if(uiMixPulse > 24500)
            SPIMotor(2,MT_MOVE_TO,g_tTipVerticalMotor.uiDilutionMixPosition-200);     //Tipͷ��ֱ����˶�����Һ����λ��
        else 
            SPIMotor(2,MT_MOVE_TO,g_tTipVerticalMotor.uiDilutionMixPosition);     //Tipͷ��ֱ����˶�����Һ����λ��
        SPIMotor(3,SET_SPEED,g_tPistonMotor.ucMixSpeed);                      //��������Ļ����ٶ�
        SPIMotor(3,MT_MOVE_TO,g_uiPistonPosition-(g_tRunProcessFile[num].uiSample_Wholeblood-200));//���������Һ
        if(uiMixPulse > 12500)
            uiMixPulse = 12500;
        for(j=1;j<g_tRunProcessFile[num].ucMixTimes;j++)
        {
            SPIMotor(3,MT_MOVE_TO,g_uiPistonPosition + uiMixPulse);           //�����������
            SPIMotor(3,MT_MOVE_TO,g_uiPistonPosition - (uiMixPulse-100));     //�����������
        }
        SPIMotor(3,MT_MOVE_TO,g_uiPistonPosition + uiMixPulse);               //���������Һ
        SPIMotor(2,MT_MOVE_TO,g_tTipVerticalMotor.uiOffLiquid);               //Tipͷ��ֱ����˶�����Һλ��
        SPIMotor(3,SET_SPEED,g_tPistonMotor.ucSuctionSpeed);                  //�ָ�����������ٶ�
        //SPIMotor(3,MT_MOVE_TO,g_uiPistonPosition-(uiMixPulse + 50*(g_tRunProcessFile[num].ucMixTimes+1) + g_tPistonMotor.uiOverPulse));//���������Һ
        SPIMotor(3,MT_HOME,0);      //���������λ
        if(g_tProcessControl.ucStopFlag==0)
            Delay1ms(100);
        SPIMotor(2,MT_HOME,0); 
        SPIMotor(3,MT_MOVE_TO,g_uiPistonPosition+g_tPistonMotor.uiPreSuction);   //�������Ԥ��Һ
        //����Ϊ��Һ����
        SPIMotor(2,MT_MOVE_TO,g_tTipVerticalMotor.uiDilutionMixPosition-100);                  //Tipͷ��ֱ����˶���ȡҺλ��uiSuctionLiquidPosition
        if(g_tProcessControl.ucStopFlag==0)
            Delay1ms(50);                                                                        //�ӳ�50ms
        SPIMotor(3,MT_MOVE_TO,g_uiPistonPosition + g_tRunProcessFile[num].uiDilutionSample_Wholeblood); //���������Һ
        g_tRunProcessFile[num].uiSample_Wholeblood = g_tRunProcessFile[num].uiDilutionSample_Wholeblood;
        if(g_tProcessControl.ucStopFlag==0)
            Delay1ms(200);                                                                       //�ӳ�200ms
        SPIMotor(2,MT_HOME,0);                                                               //Tipͷ��ֱ�����λ
    }
}


/*******************************************************************************
 * �������ƣ�LcdRunProcess
 * �����������������̺���
 * �����������
 * �����������
 * ����ֵ  ����
 * �޸�����            �汾��            �޸���            �޸�����
 * -----------------------------------------------------------------------------
 * 2017/03/21          V1.0             ��־ǿ               ����
 *******************************************************************************/
void LcdRunProcess(void)
{
    unsigned char i=0,j=0,k=0;
    unsigned char ucRunProcess1Num=0,ucRunProcess2Num=0,ucRunProcess3Num=0;
    unsigned int  uiMixPulse=0;
    unsigned char ucFistTestFlag = 0;   //PMTԤ�ɼ�����
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
    SPIMotor(4,MT_HOME,0);      //�Լ��̵����λ
    SPIMotor(3,SET_SPEED, g_tPistonMotor.ucSuctionSpeed);                           //���û���������ٶ�
    for(i=0;i<12;i++)   //����1�����������Լ�1���Լ�2
    {
        if(g_tTestInfo[i].ucSetOkFlag == 1)
        {
            g_tProcessTimeFlag = 1;
            SPIMotor(4,MT_MOVE_TO,g_tReagentTrayMotor.uiaTip1Position[i]);           //�Լ��̵���˶�����һTipλ��
            SPIMotor(1,MT_MOVE_TO,g_tTipHorizontalMotor.uiTakeTip);                  //Tipͷˮƽ����˶���ȡTipͷλ��
            LcdTakeTipProcess();                                                     //ȡTip����
            SPIMotor(4,MT_MOVE_TO,g_tReagentTrayMotor.uiaReagentKitPosition[i]);     //�Լ��̵���˶����Լ���λ��
            
            SPIMotor(1,MT_MOVE_TO,g_tTipHorizontalMotor.uiSecondReagent);                                 //Tipͷˮƽ����˶���R2λ��                                                         //�ӳ�50ms
            SPIMotor(2,MT_MOVE_TO,g_tTipVerticalMotor.uiPrepuncturePosition+350);     //Tipͷ��ֱ����˶���ȡҺλ��g_tTipVerticalMotor.uiOffLiquid
            Delay1ms(50);     
            SPIMotor(2,MT_HOME,0);
            SPIMotor(1,MT_MOVE_TO,g_tTipHorizontalMotor.uiFirstReagent);                                 //Tipͷˮƽ����˶���R1λ��                                                         //�ӳ�50ms
            SPIMotor(2,MT_MOVE_TO,g_tTipVerticalMotor.uiPrepuncturePosition+350);     //Tipͷ��ֱ����˶���ȡҺλ��g_tTipVerticalMotor.uiOffLiquid
            Delay1ms(50);     
            SPIMotor(2,MT_HOME,0);
            if((g_tTestInfo[i].ucDilute > 1)) 
            {
                SPIMotor(1,MT_MOVE_TO,g_tTipHorizontalMotor.uiDilution);                                 //Tipͷˮƽ����˶���ϡ��λ��
                SPIMotor(2,MT_MOVE_TO,g_tTipVerticalMotor.uiPrepuncturePosition);     //Tipͷ��ֱ����˶���ȡҺλ��g_tTipVerticalMotor.uiOffLiquid
                Delay1ms(50);     
                SPIMotor(2,MT_HOME,0); 
            }
             
            
            SPIMotor(3,MT_MOVE_TO,g_uiPistonPosition+g_tPistonMotor.uiPreSuction);   //�������Ԥ��Һ
            //LcdMixLiquid(g_tTipHorizontalMotor.uiSecondReagent,g_tRunProcessFile[i].uiSecondReagent);//���ȵڶ��Լ�
            
            //SPIMotor(3,MT_MOVE_TO,g_uiPistonPosition+g_tPistonMotor.uiPreSuction);   //�������Ԥ��Һ
            if((g_tTestInfo[i].ucSampleType == 1) || (g_tTestInfo[i].ucSampleType == 2) || (g_tTestInfo[i].ucSampleType == 4) || (g_tTestInfo[i].ucSampleType == 5))//Ѫ�塢Ѫ����QC1��QC2����
            {
                LcdSuctionLiquid(g_tTipHorizontalMotor.uiSample,g_tRunProcessFile[i].uiSample);//ȡ����
                LcdDilutionProcess(i);  //ϡ������
                LcdSuctionLiquid(g_tTipHorizontalMotor.uiFirstReagent,g_tRunProcessFile[i].uiFistReagent);  //ȡ��һ�Լ�
                LcdSuctionLiquid(g_tTipHorizontalMotor.uiSecondReagent,g_tRunProcessFile[i].uiSecondReagent);//ȡ�ڶ��Լ�
            }
            if(g_tTestInfo[i].ucSampleType == 3)   //ȫѪ����
            {
                LcdSuctionLiquid(g_tTipHorizontalMotor.uiSample,g_tRunProcessFile[i].uiSample_Wholeblood);//ȡ����
                LcdDilutionProcess(i);  //ϡ������
                LcdSuctionLiquid(g_tTipHorizontalMotor.uiFirstReagent,g_tRunProcessFile[i].uiFistReagent_Wholeblood);  //ȡ��һ�Լ�
                LcdSuctionLiquid(g_tTipHorizontalMotor.uiSecondReagent,g_tRunProcessFile[i].uiSecondReagent_Wholeblood);//ȡ�ڶ��Լ�
            }
            
            SPIMotor(1,MT_MOVE_TO,g_tTipHorizontalMotor.uiTestPositon);              //Tipͷˮƽ����˶������λ
            SPIMotor(2,MT_MOVE_TO,g_tTipVerticalMotor.uiOffLiquid);                  //Tipͷ��ֱ����˶�����Һ�߶�
            if(g_tProcessControl.ucStopFlag==0)
                Delay1ms(50);                                                            //�ӳ�50ms
            if((g_tTestInfo[i].ucSampleType == 1) || (g_tTestInfo[i].ucSampleType == 2) || (g_tTestInfo[i].ucSampleType == 4) || (g_tTestInfo[i].ucSampleType == 5))//Ѫ�塢Ѫ����QC1��QC2����
            {
                SPIMotor(3,MT_MOVE_TO,g_uiPistonPosition-(g_tRunProcessFile[i].uiSample+g_tRunProcessFile[i].uiFistReagent+g_tRunProcessFile[i].uiSecondReagent+g_tPistonMotor.uiOverPulse));//���������Һ
            }
            if(g_tTestInfo[i].ucSampleType == 3)   //ȫѪ����
            {
                SPIMotor(3,MT_MOVE_TO,g_uiPistonPosition-(g_tRunProcessFile[i].uiSample_Wholeblood+g_tRunProcessFile[i].uiFistReagent_Wholeblood+g_tRunProcessFile[i].uiSecondReagent_Wholeblood+g_tPistonMotor.uiOverPulse));//���������Һ
            }
            if(g_tProcessControl.ucStopFlag==0)
                //Delay1ms(500);//�޸�
                Delay1ms(200);//�ӳ�500ms            
            g_tTestInfo[i].ucStartTimingFlag = 1;                                    //��ͨ����ʼ��ʱ
            g_tTestInfo[i].ucStepEndFlag = 1; 
            SPIMotor(2,MT_HOME,0);                                                   //Tipͷ��ֱ�����λ
            LcdOffTipProcess();
            SPIMotor(3,MT_HOME,0);      //���������λ
            ucRunProcess1Num++;                                                      //��¼���е�һ�����̵�ͨ����
            
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
    SPIMotor(4,MT_HOME,0);      //�Լ��̵����λ
    SPIMotor(1,MT_HOME,0);      //ˮƽ�����λ
    while(1)    //����2�����Լ�3������
    {
        for(i=0;i<12;i++)
        {
            RunLcdConmunication();
            //if((g_tTestInfo[i].ucSetOkFlag == 1)&&(g_tTestInfo[i].uiTimer>=g_tRunProcessFile[i].uiProcess1Timer)&&(g_tTestInfo[i].uiTimer < (1.1*g_tRunProcessFile[i].uiProcess1Timer)))
            if((g_tTestInfo[i].ucSetOkFlag == 1)&&(g_tTestInfo[i].uiTimer>=g_tRunProcessFile[i].uiProcess1Timer)&&(g_tTestInfo[i].ucStepEndFlag == 1)&&(g_tProcessControl.ucStopFlag==0))
            {
                g_tTestInfo[i].ucStartTimingFlag = 0;                                 //��ͨ���رռ�ʱ
                g_tTestInfo[i].uiTimer           = 0;                                 //��ʱ��������
                g_tTestInfo[i].ucStepEndFlag = 0;
                SPIMotor(1,MT_MOVE_TO,g_tTipHorizontalMotor.uiTakeTip);               //Tipͷˮƽ����˶���ȡTipͷλ��
                SPIMotor(4,MT_MOVE_TO,g_tReagentTrayMotor.uiaTip2Position[i]);        //�Լ��̵���˶���ȡ�ڶ�Tipλ��
                LcdTakeTipProcess();                                                  //ȡTip����
                SPIMotor(4,MT_MOVE_TO,g_tReagentTrayMotor.uiaReagentKitPosition[i]);  //�Լ��̵���˶����Լ���λ��
                
                SPIMotor(1,MT_MOVE_TO,g_tTipHorizontalMotor.uiThirdReagent);                                 //Tipͷˮƽ����˶���R3λ��                                                         //�ӳ�50ms
                SPIMotor(2,MT_MOVE_TO,g_tTipVerticalMotor.uiPrepuncturePosition+350);     //Tipͷ��ֱ����˶���ȡҺλ��g_tTipVerticalMotor.uiOffLiquid
                Delay1ms(50);  
                SPIMotor(2,MT_HOME,0);
                Delay1ms(3000); 
                
                SPIMotor(3,MT_MOVE_TO,g_uiPistonPosition+g_tPistonMotor.uiPreSuction);   //�������Ԥ��Һ
                //LcdMixLiquid(g_tTipHorizontalMotor.uiThirdReagent,g_tRunProcessFile[i].uiThirdReagent);//���ȵ����Լ�
                
                //SPIMotor(3,MT_MOVE_TO,g_uiPistonPosition+g_tPistonMotor.uiPreSuction);//�������Ԥ��Һ
                if((g_tTestInfo[i].ucSampleType == 1) || (g_tTestInfo[i].ucSampleType == 2) || (g_tTestInfo[i].ucSampleType == 4) || (g_tTestInfo[i].ucSampleType == 5))//Ѫ�塢Ѫ����QC1��QC2����
                {
                    LcdSuctionLiquid(g_tTipHorizontalMotor.uiThirdReagent,g_tRunProcessFile[i].uiThirdReagent);  //ȡ�����Լ�
                }
                if(g_tTestInfo[i].ucSampleType == 3)   //ȫѪ����
                {
                    LcdSuctionLiquid(g_tTipHorizontalMotor.uiThirdReagent,g_tRunProcessFile[i].uiThirdReagent_Wholeblood);  //ȡ�����Լ�
                }
                
                SPIMotor(1,MT_MOVE_TO,g_tTipHorizontalMotor.uiTestPositon);           //Tipͷˮƽ����˶������λ��
                SPIMotor(2,MT_MOVE_TO,g_tTipVerticalMotor.uiTestMixPosition);         //Tipͷ��ֱ����˶�����Һ����λ��
                if(g_tProcessControl.ucStopFlag==0)
                    Delay1ms(50);                                                     //�ӳ�50ms
                SPIMotor(2,MT_MOVE_TO,g_tTipVerticalMotor.uiTestMixPosition);         //Tipͷ��ֱ����˶�����Һ����λ��
                SPIMotor(3,SET_SPEED,g_tPistonMotor.ucMixSpeed);                      //��������Ļ����ٶ�
                if((g_tTestInfo[i].ucSampleType == 1) || (g_tTestInfo[i].ucSampleType == 2) || (g_tTestInfo[i].ucSampleType == 4) || (g_tTestInfo[i].ucSampleType == 5))//Ѫ�塢Ѫ����QC1��QC2����
                {
                    SPIMotor(3,MT_MOVE_TO,g_uiPistonPosition-(g_tRunProcessFile[i].uiThirdReagent-200));//���������Һ
                    uiMixPulse = g_tRunProcessFile[i].uiSample+g_tRunProcessFile[i].uiFistReagent+g_tRunProcessFile[i].uiSecondReagent+g_tRunProcessFile[i].uiThirdReagent-g_tPistonMotor.uiRemainPulse-200;
                }
                if(g_tTestInfo[i].ucSampleType == 3)   //ȫѪ����
                {
                    SPIMotor(3,MT_MOVE_TO,g_uiPistonPosition-(g_tRunProcessFile[i].uiThirdReagent_Wholeblood-200));//���������Һ
                    uiMixPulse = g_tRunProcessFile[i].uiSample_Wholeblood+g_tRunProcessFile[i].uiFistReagent_Wholeblood+g_tRunProcessFile[i].uiSecondReagent_Wholeblood+g_tRunProcessFile[i].uiThirdReagent_Wholeblood-g_tPistonMotor.uiRemainPulse-200;
                }
                if(uiMixPulse > 12500)
                    uiMixPulse = 12500;
                for(j=0;j<g_tRunProcessFile[i].ucMixTimes;j++)
                {
                    SPIMotor(3,MT_MOVE_TO,g_uiPistonPosition+uiMixPulse);           //�����������
                    SPIMotor(3,MT_MOVE_TO,g_uiPistonPosition-(uiMixPulse-200));           //�����������
                }
                SPIMotor(3,MT_MOVE_TO,g_uiPistonPosition+uiMixPulse);               //���������Һ
                SPIMotor(2,MT_MOVE_TO,g_tTipVerticalMotor.uiOffLiquid);             //Tipͷ��ֱ����˶�����Һλ��
                SPIMotor(3,SET_SPEED,g_tPistonMotor.ucSuctionSpeed);               //�ָ�����������ٶ�
                SPIMotor(3,MT_MOVE_TO,g_uiPistonPosition-(uiMixPulse + 200*(g_tRunProcessFile[i].ucMixTimes+1) + g_tPistonMotor.uiOverPulse));//���������Һ
                if(g_tProcessControl.ucStopFlag==0)
                    Delay1ms(100);                                                      //�ӳ�100ms
                SPIMotor(2,MT_HOME,0);                                              //Tipͷ��ֱ�����λ
                LcdOffTipProcess();
                SPIMotor(3,MT_HOME,0);                                              //���������λ
                g_tTestInfo[i].ucStartTimingFlag = 1;                               //��ͨ����ʼ��ʱ
                
                ucRunProcess2Num++;                                                 //��¼���еڶ������̵�ͨ����
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
    SPIMotor(4,MT_HOME,0);      //�Լ��̵����λ
    
    while(1)    //����3�����м��
    {
        for(i=0;i<12;i++)
        {
            RunLcdConmunication();
            //if((g_tTestInfo[i].ucSetOkFlag == 1)&&(g_tTestInfo[i].uiTimer>=g_tRunProcessFile[i].uiProcess2Timer)&&(g_tTestInfo[i].uiTimer < (1.1*g_tRunProcessFile[i].uiProcess2Timer)))
            if((g_tTestInfo[i].ucSetOkFlag == 1)&&(g_tTestInfo[i].uiTimer>=g_tRunProcessFile[i].uiProcess2Timer)&&(g_tProcessControl.ucStopFlag==0))
            {
                g_tTestInfo[i].ucStartTimingFlag = 0;                                 //��ͨ���رռ�ʱ
                g_tTestInfo[i].uiTimer           = 0;                                 //��ʱ��������
                g_tTestInfo[i].ucTestedFlag      = 1;
                SPIMotor(4,MT_MOVE_TO,g_tReagentTrayMotor.uiTestPosition[i]);         //�Լ��̵���˶������λ��
                SPIMotor(5,MT_MOVE_TO,g_uiPmtVerticalTestPosition);                   //PMT��ֱ����˶����ܹ���λ��
                PrePmtProcess1();//PMT��5S
                if(ucFistTestFlag == 0)                                               //PMTԤ�ɼ�
                {
                    ucFistTestFlag = 1;
                    PrePmtProcess(i);
                }
                PrePmtProcess(i);
                PmtProcess(i);
                LcdCalculateConcentration(i);
                SPIMotor(5,MT_HOME,0);                                                //PMT��ֱ�����λ
                DisplayTestRestult(i,0);                                              //��ʾ�����
                if(i < 6)//������ɺ�ѡ
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
                    LisTxResultRecord(i);    //LISϵͳ���ͽ��
                }
                ucRunProcess3Num++;
                SaveHistoryStatistics(i);   //ͳ����Ŀ���Ը���
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
        SPIMotor(2,MT_HOME,0);                             //Tipͷ��ֱ�����λ
        LcdOffTipProcess();
    }
    
    InitTimer5(0);
    UpdateProcessBar(1);        //��ʾ��������
    SPIMotor(3,MT_HOME,0);      //���������λ
    SPIMotor(3,MT_MOVE_TO,6000); //�������
    SPIMotor(2,MT_HOME,0);      //Tipͷ��ֱ�����λ
    SPIMotor(5,MT_HOME,0);      //PMT��ֱ�����λ
    //SPIMotor(4,MT_HOME,0);      //�Լ��̵����λ
    SPIMotor(4,MT_MOVE_TO_FW,0);           //����ͨ��1��2
    SPIMotor(1,MT_HOME,0);      //Tipͷˮƽ�����λ
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
    InitDisplay();//ɾ������ͨ����Ϣ
    
}
