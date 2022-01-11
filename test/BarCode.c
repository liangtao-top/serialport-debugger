#include"BarCode.h"
#include"Spi.h"
#include"User.h"
#include"TouchHandle.h"
#include"ProcessFile.h"
#include"Procedure.h"
#include"string.h"
#include"LcdScreen.h"
#include "SettingPic.h"
#include"HistoryResult.h"
/*
 *0     ϵͳ������0-4095��  
 *1-7   ͳ�ƣ�4096-32767�� 4096�洢��Ŀѭ����־   4100-31299
 * 8    ���ã�32768-36863��32768����    32772�Զ���ӡ   32776�������뷽ʽ   32780Ĭ����������   32784LIS����    32788�û�����
 * 9    ϵͳ��36864-40959��36864�洢ѭ����־   32868--
 * 10-23������40960-94207��40960�洢ѭ����־    40964-41363��ͬ���Ŵ洢��־ 41364-94163
 * 24   ��ʷ�洢��ַ��94208-98303��94208�洢ѭ����־    94212�洢����־ 100000--
 * 27344 Tipͷ�����洢��ַ��112001024��
 */
#define     REAGENT_BAR_INFO_HEADER     (unsigned char)101
#define     REAGENT_BAR_INFO_ADDR       (unsigned long int)41364//600
#define     REAGENT_BAR_INFO_SIZE       (unsigned int)176
#define     REAGENT_BAR_INFO_ADDR1       (unsigned long int)40964//54000����ѭ��     100*4byte
#define     REAGENT_BAR_INFO_ADDR2       (unsigned long int)40960//420��Ŀ����ѭ��   1*4byte

/*94�������*/
static unsigned char Str94Table[94]="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz`~!@#$%^&*()-_=+[{]};:'\"\\|,<.>/?";
unsigned char g_uc94BarCodeBuffer[REAGENT_94BAR_CODE_NUM+10],g_ucBarCodeByteBuffer[REAGENT_BAR_CODE_BYTE_NUM+10];

struct StandardCurve g_tStandCurve[12];
struct ScanReagentBarCodeAddr g_tScanReagentBarCodeAddr={0x06F0,0x0700,0x0710};
unsigned long int ReagentDetail_flag,ReagentDetail_flag1;
unsigned char AutoGetReagentBriefBarCodeFlag=0;
unsigned char StartHandDeviceScanFlag=0;
/*******************************************************************************
 * �������ƣ�Convert94ToHex
 * ������������94���Ƶ��ַ�ת��Ϊ��ֵ
 * ���������p Ҫת�����ַ�����num �ַ��ĸ���
 * ���������p ��ת�������ֵ��д��p��
 * ����ֵ  ����
 * �޸�����            �汾��            �޸���            �޸�����
 * -----------------------------------------------------------------------------
 * 2017/08/29          V1.0             ��־ǿ               ����
 *******************************************************************************/
void Convert94ToHex(unsigned char *p,unsigned char num)
{
	unsigned char i,j;
	for(i=0;i<num;i++)
	{
		for(j=0;j<94;j++)
		{
			if(p[i]==Str94Table[j])
			{
				p[i]=j;
				break;
			}
		}
	}
}

/*******************************************************************************
 * �������ƣ�SaveReagentDetailInfo
 * ���������������Լ�����ϸ��Ϣ
 * ���������p ��Ҫ�������Ϣ
 * �����������
 * ����ֵ  ��0 ����ɹ�  1 ����ʧ��
 * �޸�����            �汾��            �޸���            �޸�����
 * -----------------------------------------------------------------------------
 * 2017/08/29          V1.0             ��־ǿ               ����
 *******************************************************************************/
unsigned char SaveReagentDetailInfo(void)
{
    unsigned int i=0;
    unsigned char j=0;
    unsigned char ucTemp[15],ucName1[11],ucName2[11],ucName3[3],ucName4[3],ucName5[11],ucLotNum1[23];
    unsigned long int uliAddr = 0,uliAddr1 = 0;

    //Ѱ���Ƿ�����ͬ��Ŀ���ƺ���ͬ�Լ����ŵ�
    for(i=0;i<100;i++)
    {
        uliAddr = REAGENT_BAR_INFO_ADDR + REAGENT_BAR_INFO_SIZE * i * 3;
        W25QXX_Read(ucTemp,uliAddr,11);
        memcpy(ucName1,ucTemp+1,10);
        ucName1[10] = '\0';
        memcpy(ucName2,g_ucBarCodeByteBuffer+1,10);
        ucName2[10] = '\0';
        ucName3[0] = (g_ucBarCodeByteBuffer[20]&0xFE)>>1;//��
        ucName3[1] = ((g_ucBarCodeByteBuffer[20]&0x01)<<3) | ((g_ucBarCodeByteBuffer[21]&0xE0)>>5);//��
        ucName3[2] = ((g_ucBarCodeByteBuffer[21]&0x1F)<<5) | ((g_ucBarCodeByteBuffer[22]&0xF8)>>3);//���к�
        if(ucTemp[0] == REAGENT_BAR_INFO_HEADER)
        {
            if(0 == strcmp((const char *)ucName1,(const char *)ucName2))
            {
                for(j=0;j<3;j++)
                {
                    uliAddr = REAGENT_BAR_INFO_ADDR + REAGENT_BAR_INFO_SIZE * (i*3+j);
                    W25QXX_Read(ucLotNum1,uliAddr,23);
                    
                    memcpy(ucName5,ucLotNum1+1,10);
                    ucName5[10] = '\0';
                    
                    ucName4[0] = (ucLotNum1[20]&0xFE)>>1;//��
                    ucName4[1] = ((ucLotNum1[20]&0x01)<<3) | ((ucLotNum1[21]&0xE0)>>5);//��
                    ucName4[2] = ((ucLotNum1[21]&0x1F)<<5) | ((ucLotNum1[22]&0xF8)>>3);//���к�

                    //if(ucLotNum1[0] == REAGENT_BAR_INFO_HEADER && 0 == strncmp((const char *)ucName3,(const char *)ucName4,3) && 0 != strcmp((const char *)ucName5,(const char *)ucName1))//��ͬ��Ŀ�����Ƿ�����ͬ�Լ����Ż����λ��
                    if((ucLotNum1[0] != REAGENT_BAR_INFO_HEADER) || 0 == strncmp((const char *)ucName3,(const char *)ucName4,3) || 0 != strcmp((const char *)ucName5,(const char *)ucName1))//��ͬ��Ŀ�����Ƿ�����ͬ�Լ����Ż����λ��
                    {
                        W25QXX_Write(g_ucBarCodeByteBuffer,uliAddr,REAGENT_BAR_INFO_SIZE);
                        return 0;
                    }
                    
                }
                uliAddr1 = REAGENT_BAR_INFO_ADDR1 + i*4;
                ReagentDetail_flag1 = FlashRead1ByteValue((unsigned long int)uliAddr1/4);   //�洢ѭ����־λ
                if(ReagentDetail_flag1 > 2)
                    ReagentDetail_flag1=0;
                uliAddr = REAGENT_BAR_INFO_ADDR + REAGENT_BAR_INFO_SIZE * (i * 3+ReagentDetail_flag1);
                W25QXX_Write(g_ucBarCodeByteBuffer,uliAddr,REAGENT_BAR_INFO_SIZE);
                ReagentDetail_flag1 = ReagentDetail_flag1 + 1;
                FlashWrite4BytesValue((unsigned long int)uliAddr1,(unsigned long int)ReagentDetail_flag1);
                return 0;
            }
        }
    }
    //��ͬ��Ŀ����Ѱ�ҿ���λ��
    for(i=0;i<100;i++)
    {
        uliAddr = REAGENT_BAR_INFO_ADDR + REAGENT_BAR_INFO_SIZE * i * 3;
        W25QXX_Read(ucTemp,uliAddr,11);
        if(ucTemp[0] != REAGENT_BAR_INFO_HEADER)
        {
            W25QXX_Write(g_ucBarCodeByteBuffer,uliAddr,REAGENT_BAR_INFO_SIZE);
            return 0;
        }
    }
    ReagentDetail_flag = FlashRead1ByteValue((unsigned long int)REAGENT_BAR_INFO_ADDR2 / 4);   //�洢ѭ����־λ
    if(ReagentDetail_flag > 99)
         ReagentDetail_flag=0;
    uliAddr = REAGENT_BAR_INFO_ADDR + REAGENT_BAR_INFO_SIZE * ReagentDetail_flag*3;  
    W25QXX_Write(g_ucBarCodeByteBuffer,uliAddr,REAGENT_BAR_INFO_SIZE);
    ReagentDetail_flag = ReagentDetail_flag + 1;
    FlashWrite4BytesValue((unsigned long int)REAGENT_BAR_INFO_ADDR2,(unsigned long int)ReagentDetail_flag);
    return 0;

}

void DeleteReagentDetailInfo(void)//ɾ��������Ϣ
{
    unsigned int i=0;
    unsigned char j=0;
    unsigned char ucTemp[15],ucName1[11],ucDelByteBuffer[REAGENT_BAR_INFO_SIZE];
    unsigned long int uliAddr = 0,uliAddr1 = 0;
    unsigned long int uliReagentDetail_flag1 = 0,uliReagentDetail_flag2 = 0;

    for(i=0;i<REAGENT_BAR_INFO_SIZE;i++)
    {
        ucDelByteBuffer[i] = '\0';
    }
        
    //Ѱ���Ƿ�����ͬ��Ŀ���ƺ���ͬ�Լ����ŵ�
    for(i=0;i<100;i++)
    {
        uliAddr = REAGENT_BAR_INFO_ADDR + REAGENT_BAR_INFO_SIZE * i * 3;
        W25QXX_Read(ucTemp,uliAddr,11);
        memcpy(ucName1,ucTemp+1,10);
        ucName1[10] = '\0';
        if(ucName1[0] != '\0')
        {
            for(j=0;j<3;j++)
            {
                uliAddr = REAGENT_BAR_INFO_ADDR + REAGENT_BAR_INFO_SIZE * (i*3+j);
                W25QXX_Write(ucDelByteBuffer,uliAddr,REAGENT_BAR_INFO_SIZE);  
            }
        }
        uliAddr1 = REAGENT_BAR_INFO_ADDR1 + i*4;
        FlashWrite4BytesValue((unsigned long int)uliAddr1,(unsigned long int)uliReagentDetail_flag1);
    }
    FlashWrite4BytesValue((unsigned long int)REAGENT_BAR_INFO_ADDR2,(unsigned long int)uliReagentDetail_flag2);
}

void DisplayReagentInfoPicture(void)
{
    DisplayPicture(SCAN_REAGENT_INFO_PICTURE);
    DisplayText(g_tScanReagentBarCodeAddr.uiName,'\0');
    DisplayText(g_tScanReagentBarCodeAddr.uiLotNum,'\0');
    DisplayText(g_tScanReagentBarCodeAddr.uiExpire,'\0');
}
/*******************************************************************************
 * �������ƣ�LcdGetReagentDetailInfo
 * ������������ȡ�Լ��е���ϸ��Ϣ
 * �����������
 * �����������
 * ����ֵ  ��0 ��ȡ�ɹ�  1 ��ȡʧ��
 * �޸�����            �汾��            �޸���            �޸�����
 * -----------------------------------------------------------------------------
 * 2017/08/29          V1.0              ��־ǿ               ����
 *******************************************************************************/
unsigned char LcdGetReagentDetailInfo(void)
{
    unsigned char i=0,ucNum=0;
    unsigned char ucTemp[11];
    unsigned int uiValue=0;
    unsigned long int uliValue=0;
    unsigned char Distemp1[]= "*���ɨ�谴ť����ɨ�������Ϣ";
    unsigned char Distemp2[]= "*������水ť���������Ϣ";
    unsigned char Distemp3[]= "*������ذ�ť�����������Ϣ";
    DisplayText(0x0848,'\0');
    DisplayText(0x0858,'\0');
    DisplayText(0x0868,'\0');
    DisplayText(g_tScanReagentBarCodeAddr.uiName,'\0');
    DisplayText(g_tScanReagentBarCodeAddr.uiLotNum,'\0');
    DisplayText(g_tScanReagentBarCodeAddr.uiExpire,'\0');
    SendDataToLcdRegister(0x4F,1);  //��������ʹ��ɨ��ǹɨ���Լ���Ϣ������ʾ��
    g_uc94BarCodeBuffer[0] = '\0';
    SPIStartHandDeviceScan();
    while(1)
    {
        if(HAND_SCAN_ACK == 1)
        {
            SPIGetHandDeviceScanInfo(g_uc94BarCodeBuffer);
            SendDataToLcdRegister(0x4F,2);  //�ر���ʾ��
            while(g_ucUartLcdRecvFlag == 0);
            g_ucUartLcdRecvFlag = 0;
            break;
        }
        if(g_ucUartLcdRecvFlag == 1)
        {
            g_ucUartLcdRecvFlag = 0;
            break;
        }
    }
    if(g_uc94BarCodeBuffer[0] == '!') //�Լ�����ϸ��Ϣ
    {
        Convert94ToHex(g_uc94BarCodeBuffer,REAGENT_94BAR_CODE_NUM);
        for(i=0;;i++)
        {
            ucNum = (unsigned char)REAGENT_94BAR_CODE_NUM - i*5 - 5;
            uliValue = g_uc94BarCodeBuffer[ucNum];
            uliValue = uliValue*94 + g_uc94BarCodeBuffer[ucNum+1];
            uliValue = uliValue*94 + g_uc94BarCodeBuffer[ucNum+2];
            uliValue = uliValue*94 + g_uc94BarCodeBuffer[ucNum+3];
            uliValue = uliValue*94 + g_uc94BarCodeBuffer[ucNum+4];
            g_ucBarCodeByteBuffer[REAGENT_BAR_CODE_BYTE_NUM-4*i-4]   = (uliValue>>24)&0xFF;
            g_ucBarCodeByteBuffer[REAGENT_BAR_CODE_BYTE_NUM-4*i-3]   = (uliValue>>16)&0xFF;
            g_ucBarCodeByteBuffer[REAGENT_BAR_CODE_BYTE_NUM-4*i-2]   = (uliValue>>8)&0xFF;
            g_ucBarCodeByteBuffer[REAGENT_BAR_CODE_BYTE_NUM-4*i-1]   = (uliValue)&0xFF;
            if(ucNum<=5)
            {
                break;
            }
        }
        uliValue = g_uc94BarCodeBuffer[1];
        uliValue = uliValue*94 + g_uc94BarCodeBuffer[2];
        uliValue = uliValue*94 + g_uc94BarCodeBuffer[3];
        uliValue = uliValue*94 + g_uc94BarCodeBuffer[4];
        g_ucBarCodeByteBuffer[1] = (uliValue>>16)&0xFF;
        g_ucBarCodeByteBuffer[2] = (uliValue>>8)&0xFF;
        g_ucBarCodeByteBuffer[3] = (uliValue&0xFF)&0xFF;
        g_ucBarCodeByteBuffer[0] = REAGENT_BAR_INFO_HEADER; //��־λ
        
        //��ʾ��Ŀ����
        memcpy(ucTemp,g_ucBarCodeByteBuffer+1,10);
        ucTemp[10] = '\0';
        DisplayText(g_tScanReagentBarCodeAddr.uiName,ucTemp);
        
        //��ʾ����
        ucTemp[0] = 2 + '0';
        ucTemp[1] = 0 + '0';
        uiValue = (g_ucBarCodeByteBuffer[20]&0xFE)>>1;//��
        ucTemp[2] =  uiValue / 10+ '0';
        ucTemp[3] =  uiValue % 10+ '0';
        uiValue = ((g_ucBarCodeByteBuffer[20]&0x01)<<3) | ((g_ucBarCodeByteBuffer[21]&0xE0)>>5);//��
        ucTemp[4] =  uiValue / 10+ '0';
        ucTemp[5] =  uiValue % 10+ '0';
        uiValue = ((g_ucBarCodeByteBuffer[21]&0x1F)<<5) | ((g_ucBarCodeByteBuffer[22]&0xF8)>>3);//���к�
        ucTemp[6] =  uiValue / 100+ '0';
        ucTemp[7] = (uiValue % 100)/10 + '0';
        ucTemp[8] =  uiValue % 10 + '0';
        ucTemp[9] = '\0';
        DisplayText(g_tScanReagentBarCodeAddr.uiLotNum,ucTemp);
        //��ʾ��Ч��
        ucTemp[0] = 2 + '0';
        ucTemp[1] = 0 + '0';
        uiValue = (g_ucBarCodeByteBuffer[18]&0xFE)>>1;//��
        ucTemp[2] =  uiValue / 10+ '0';
        ucTemp[3] =  uiValue % 10+ '0';
        ucTemp[4] =  '/';
        uiValue = ((g_ucBarCodeByteBuffer[18]&0x01)<<3) | ((g_ucBarCodeByteBuffer[19]&0xE0)>>5);//��
        ucTemp[5] =  uiValue / 10+ '0';
        ucTemp[6] =  uiValue % 10+ '0';
        ucTemp[7] =  '/';
        uiValue = g_ucBarCodeByteBuffer[19]&0x1F;//��
        ucTemp[8] =  uiValue / 10+ '0';
        ucTemp[9] =  uiValue % 10+ '0';
        ucTemp[10] ='\0';
        DisplayText(g_tScanReagentBarCodeAddr.uiExpire,ucTemp);
        
        DisplayText(0x0848,Distemp1);
        DisplayText(0x0858,Distemp2);
        DisplayText(0x0868,Distemp3);
        return 0;
    }
    else
    {
        return 1; //��ȡʧ��
    }
}

/*******************************************************************************
 * �������ƣ�GetReagentBriefBarCode
 * ������������ȡ�Լ����ļ�Ҫ��Ϣ
 * �����������
 * �����������
 * ����ֵ  ��0 ��ȡ�ɹ�  1 ��ȡʧ��
 * �޸�����            �汾��            �޸���            �޸�����
 * -----------------------------------------------------------------------------
 * 2017/08/30          V1.0              ��־ǿ               ����
 *******************************************************************************/
unsigned char GetReagentBriefBarCode(void)
{
    unsigned char i=0,ucNum=0;
    unsigned int uiValue=0,uiValue2=0;
    unsigned long int uliValue=0;
    SendDataToLcdRegister(0x4F,5);  //��������ʹ��ɨ��ǹɨ���Լ���Ϣ������ʾ��
    g_uc94BarCodeBuffer[0] = '\0';
    SPIStartHandDeviceScan();
    while(1)
    {
        if(HAND_SCAN_ACK == 1)
        {
            SPIGetHandDeviceScanInfo(g_uc94BarCodeBuffer);
            SendDataToLcdRegister(0x4F,6);  //�ر���ʾ��
            while(g_ucUartLcdRecvFlag == 0);
            g_ucUartLcdRecvFlag = 0;
            break;
        }
        if(g_ucUartLcdRecvFlag == 1)
        {
            g_ucUartLcdRecvFlag = 0;
            break;
        }
    }
    if(g_uc94BarCodeBuffer[0] == '@') //�Լ�����Ҫ��Ϣ
    {
        Convert94ToHex(g_uc94BarCodeBuffer,REAGENT_BRIEF_94BAR_CODE_NUM);
        for(i=0;;i++)
        {
            ucNum = (unsigned char)REAGENT_BRIEF_94BAR_CODE_NUM - i*5 - 5;
            uliValue = g_uc94BarCodeBuffer[ucNum];
            uliValue = uliValue*94 + g_uc94BarCodeBuffer[ucNum+1];
            uliValue = uliValue*94 + g_uc94BarCodeBuffer[ucNum+2];
            uliValue = uliValue*94 + g_uc94BarCodeBuffer[ucNum+3];
            uliValue = uliValue*94 + g_uc94BarCodeBuffer[ucNum+4];
            g_ucBarCodeByteBuffer[REAGENT_BRIEF_BAR_CODE_BYTE_NUM-4*i-4]  = (uliValue>>24)&0xFF;
            g_ucBarCodeByteBuffer[REAGENT_BRIEF_BAR_CODE_BYTE_NUM-4*i-3]  = (uliValue>>16)&0xFF;
            g_ucBarCodeByteBuffer[REAGENT_BRIEF_BAR_CODE_BYTE_NUM-4*i-2]  = (uliValue>>8)&0xFF;
            g_ucBarCodeByteBuffer[REAGENT_BRIEF_BAR_CODE_BYTE_NUM-4*i-1]  = (uliValue)&0xFF;
            if(ucNum<5)
            {
                break;
            }
        }
        uliValue = g_uc94BarCodeBuffer[1];
        uliValue = uliValue*94 + g_uc94BarCodeBuffer[2];
        g_ucBarCodeByteBuffer[0] = (uliValue)&0xFF;
        
        memcpy(g_tTestInfoTemp.ucName,g_ucBarCodeByteBuffer,10);    //��Ŀ����
        g_tTestInfoTemp.ucName[10] = '\0';
        g_tTestInfoTemp.ucExpiryDate[0] = (g_ucBarCodeByteBuffer[10]&0xFE)>>1;  //��Ч�� ��
        g_tTestInfoTemp.ucExpiryDate[1] = ((g_ucBarCodeByteBuffer[10]&0x01)<<3) | ((g_ucBarCodeByteBuffer[11]&0xE0)>>5);  //��Ч�� ��
        g_tTestInfoTemp.ucExpiryDate[2] = g_ucBarCodeByteBuffer[11]&0x1F;  //��Ч�� ��
        g_tTestInfoTemp.ucLotNum[0] = (g_ucBarCodeByteBuffer[12]&0xFE)>>1; 
        uiValue= ((g_ucBarCodeByteBuffer[12]&0x01)<<3) | ((g_ucBarCodeByteBuffer[13]&0xE0)>>5);
        uiValue2 = ((g_ucBarCodeByteBuffer[13]&0x1F)<<5) | ((g_ucBarCodeByteBuffer[14]&0xF8)>>3);
        g_tTestInfoTemp.ucLotNum[1] = (uiValue<<2) | (uiValue2>>8);
        g_tTestInfoTemp.ucLotNum[2] =  uiValue2 & 0xFF;
        g_tTestInfoTemp.uliReagentKitNum = g_ucBarCodeByteBuffer[14]&0x07;
        g_tTestInfoTemp.uliReagentKitNum = g_tTestInfoTemp.uliReagentKitNum * 65536 + (unsigned long int)g_ucBarCodeByteBuffer[15] * 256 + (unsigned long int)g_ucBarCodeByteBuffer[16];
        return 0;
    }
    else
    {
        SendDataToLcdRegister(0x4F,7);  //�������Լ���Ϣɨ��ʧ�ܡ���ʾ��
        while(g_ucUartLcdRecvFlag == 0);
        g_ucUartLcdRecvFlag = 0;
        ucInputSampleID_Scan_Flag=1;
        return 1;
    }
}

/*******************************************************************************
 * �������ƣ�GetSampleIDBarCode
 * ������������ȡ��������Ϣ
 * �����������
 * �����������
 * ����ֵ  ��0 ��ȡ�ɹ�  1 ��ȡʧ��
 * �޸�����            �汾��            �޸���            �޸�����
 * -----------------------------------------------------------------------------
 * 2017/08/30          V1.0              ��־ǿ               ����
 *******************************************************************************/
unsigned char GetSampleIDBarCode(void)
{
    SendDataToLcdRegister(0x4F,1);  //��������ɨ����������Ϣ����ʾ��
    g_uc94BarCodeBuffer[0] = '\0';
    SPIStartHandDeviceScan();
    while(1)
    {
        if(HAND_SCAN_ACK == 1)
        {
            SPIGetHandDeviceScanInfo(g_uc94BarCodeBuffer);
            SendDataToLcdRegister(0x4F,2);  //�ر���ʾ��
            while(g_ucUartLcdRecvFlag == 0);
            g_ucUartLcdRecvFlag = 0;
            break;
        }
        if(g_ucUartLcdRecvFlag == 1)
        {
            g_ucUartLcdRecvFlag = 0;
            return 1;
        }
    }
    if((g_uc94BarCodeBuffer[0] != '@') && (g_uc94BarCodeBuffer[0] != '!'))
    {
        memcpy(g_tTestInfoTemp.ucSampleID,g_uc94BarCodeBuffer,20);    //������
        g_tTestInfoTemp.ucSampleID[20] = '\0';
        return 0;
    }
    else
    {
        SendDataToLcdRegister(0x4F,3);  //ɨ�������ʾ��
        while(g_ucUartLcdRecvFlag == 0);
        g_ucUartLcdRecvFlag = 0;
        ucInputSampleID_Scan_Flag=0;
        return 1;
    }
}
unsigned char GetBarCode(void)
{
    unsigned char i=0,ucNum=0;
    unsigned int uiValue=0,uiValue2=0;
    unsigned long int uliValue=0;
    g_uc94BarCodeBuffer[0] = '\0';
    if(StartHandDeviceScanFlag == 0)
    {
        SPIStartHandDeviceScan();
        StartHandDeviceScanFlag = 1;
    }
    if(HAND_SCAN_ACK == 1)
        SPIGetHandDeviceScanInfo(g_uc94BarCodeBuffer);
    else
        return;
    
    if((g_uc94BarCodeBuffer[0] == '@') && (g_tSystemSetting.ucInputSampleIDMethod == 0 || (g_tSystemSetting.ucInputSampleIDMethod == 1 && AutoGetReagentBriefBarCodeFlag == 1))) //�Լ�����Ҫ��Ϣ
    {
        Convert94ToHex(g_uc94BarCodeBuffer,REAGENT_BRIEF_94BAR_CODE_NUM);
        for(i=0;;i++)
        {
            ucNum = (unsigned char)REAGENT_BRIEF_94BAR_CODE_NUM - i*5 - 5;
            uliValue = g_uc94BarCodeBuffer[ucNum];
            uliValue = uliValue*94 + g_uc94BarCodeBuffer[ucNum+1];
            uliValue = uliValue*94 + g_uc94BarCodeBuffer[ucNum+2];
            uliValue = uliValue*94 + g_uc94BarCodeBuffer[ucNum+3];
            uliValue = uliValue*94 + g_uc94BarCodeBuffer[ucNum+4];
            g_ucBarCodeByteBuffer[REAGENT_BRIEF_BAR_CODE_BYTE_NUM-4*i-4]  = (uliValue>>24)&0xFF;
            g_ucBarCodeByteBuffer[REAGENT_BRIEF_BAR_CODE_BYTE_NUM-4*i-3]  = (uliValue>>16)&0xFF;
            g_ucBarCodeByteBuffer[REAGENT_BRIEF_BAR_CODE_BYTE_NUM-4*i-2]  = (uliValue>>8)&0xFF;
            g_ucBarCodeByteBuffer[REAGENT_BRIEF_BAR_CODE_BYTE_NUM-4*i-1]  = (uliValue)&0xFF;
            if(ucNum<5)
            {
                break;
            }
        }
        uliValue = g_uc94BarCodeBuffer[1];
        uliValue = uliValue*94 + g_uc94BarCodeBuffer[2];
        g_ucBarCodeByteBuffer[0] = (uliValue)&0xFF;
        
        memcpy(g_tTestInfoTemp.ucName,g_ucBarCodeByteBuffer,10);    //��Ŀ����
        g_tTestInfoTemp.ucName[10] = '\0';
        g_tTestInfoTemp.ucExpiryDate[0] = (g_ucBarCodeByteBuffer[10]&0xFE)>>1;  //��Ч�� ��
        g_tTestInfoTemp.ucExpiryDate[1] = ((g_ucBarCodeByteBuffer[10]&0x01)<<3) | ((g_ucBarCodeByteBuffer[11]&0xE0)>>5);  //��Ч�� ��
        g_tTestInfoTemp.ucExpiryDate[2] = g_ucBarCodeByteBuffer[11]&0x1F;  //��Ч�� ��
        g_tTestInfoTemp.ucLotNum[0] = (g_ucBarCodeByteBuffer[12]&0xFE)>>1; 
        uiValue= ((g_ucBarCodeByteBuffer[12]&0x01)<<3) | ((g_ucBarCodeByteBuffer[13]&0xE0)>>5);
        uiValue2 = ((g_ucBarCodeByteBuffer[13]&0x1F)<<5) | ((g_ucBarCodeByteBuffer[14]&0xF8)>>3);
        g_tTestInfoTemp.ucLotNum[1] = (uiValue<<2) | (uiValue2>>8);
        g_tTestInfoTemp.ucLotNum[2] =  uiValue2 & 0xFF;
        g_tTestInfoTemp.uliReagentKitNum = g_ucBarCodeByteBuffer[14]&0x07;
        g_tTestInfoTemp.uliReagentKitNum = g_tTestInfoTemp.uliReagentKitNum * 65536 + (unsigned long int)g_ucBarCodeByteBuffer[15] * 256 + (unsigned long int)g_ucBarCodeByteBuffer[16];
        DisplayReagentBriefBarInfo(&g_tTestInfoTemp);
        StartHandDeviceScanFlag = 0;
        return ;
    }
    else if((g_uc94BarCodeBuffer[0] != '@') && (g_uc94BarCodeBuffer[0] != '!') && (g_uc94BarCodeBuffer[0] != '\0'))
    {
        memcpy(g_tTestInfoTemp.ucSampleID,g_uc94BarCodeBuffer,20);    //������
        g_tTestInfoTemp.ucSampleID[20] = '\0';
        DisplayText(0x0070,g_tTestInfoTemp.ucSampleID);
        StartHandDeviceScanFlag = 0;
        //g_tTestInfo[g_ucSelectedChannelNo].ucName[0]        = '\0';
        //g_tTestInfo[g_ucSelectedChannelNo].ucLotNum[0]      = '\0';
        //g_tTestInfo[g_ucSelectedChannelNo].ucExpiryDate[0]  = '\0';
        return ;
    }
    else
    {
        StartHandDeviceScanFlag = 0;
        return ;
    }
}

unsigned char HistoryGetBarCode(void)
{
    g_uc94BarCodeBuffer[0] = '\0';
    SendDataToLcdRegister(0x4F,0x37);  //��������ʹ��ɨ��ǹɨ���Լ���Ϣ������ʾ��
    SPIStartHandDeviceScan();
    while(1)
    {
        if(HAND_SCAN_ACK == 1)
        {
            SPIGetHandDeviceScanInfo(g_uc94BarCodeBuffer);
            //SendDataToLcdRegister(0x4F,2);  //�ر���ʾ��
            while(g_ucUartLcdRecvFlag == 0);
            g_ucUartLcdRecvFlag = 0;
            AnalyticInstruction();
            if(g_tLcdAnalyticIns.uiLcdParamAddr == 0x0003 && g_tLcdAnalyticIns.uiButtonValue == 0x0007)
            {
                DisplayPicture(g_ucCurrentPic);
                return;
            }
            break;
        }
        if(g_ucUartLcdRecvFlag == 1)
        {
            g_ucUartLcdRecvFlag = 0;
            AnalyticInstruction();
            if(g_tLcdAnalyticIns.uiLcdParamAddr == 0x0003 && g_tLcdAnalyticIns.uiButtonValue == 0x0007)
            {
                DisplayPicture(g_ucCurrentPic);
                return;
            }
            memcpy(g_tSelectResultInfo.ucSampleID,g_tLcdAnalyticIns.ucLcdInputValue,g_tLcdAnalyticIns.ucLcdRecvBytesNum+1);
            g_tSelectResultInfo.ucSampleID[20] = '\0';
            DisplayText(0x0698,g_tSelectResultInfo.ucSampleID);
            return;
        }
    }
    if((g_uc94BarCodeBuffer[0] != '@') && (g_uc94BarCodeBuffer[0] != '!'))
    {
        StartHandDeviceScanFlag = 0;
        memcpy(g_tSelectResultInfo.ucSampleID,g_uc94BarCodeBuffer,20);    //������
        g_tSelectResultInfo.ucSampleID[20] = '\0';
        DisplayText(0x0698,g_tSelectResultInfo.ucSampleID);
        return ;
    }      
}
unsigned char SystemErrorAdjustmentGetBarCode(unsigned char st)
{
    unsigned char i=0,ucNum=0;
    unsigned long int uliValue=0;
    g_uc94BarCodeBuffer[0] = '\0';
    SendDataToLcdRegister(0x4F,0x30);  //��������ʹ��ɨ��ǹɨ���Լ���Ϣ������ʾ��
    SPIStartHandDeviceScan();
    while(1)
    {
        if(HAND_SCAN_ACK == 1)
        {
            SPIGetHandDeviceScanInfo(g_uc94BarCodeBuffer);
            SendDataToLcdRegister(0x4F,2);  //�ر���ʾ��
            while(g_ucUartLcdRecvFlag == 0);
            g_ucUartLcdRecvFlag = 0;
            break;
        }
        if(g_ucUartLcdRecvFlag == 1)
        {
            g_ucUartLcdRecvFlag = 0;
            if(st == 0)
            {
                g_tSelectResultInfo.ucName[0] = '\0';
                DisplayText(0x06C8,g_tSelectResultInfo.ucName);
            }
            else 
            {
                g_tSystemErrorAdjustment.ucName[0] = '\0';
                DisplayText(0x0718,g_tSystemErrorAdjustment.ucName);
            }
            break;
        }
    }
    if(g_uc94BarCodeBuffer[0] == '!') //�Լ��м�Ҫ��Ϣ
    {
        StartHandDeviceScanFlag = 0;
        Convert94ToHex(g_uc94BarCodeBuffer,REAGENT_94BAR_CODE_NUM);
        for(i=0;;i++)
        {
            ucNum = (unsigned char)REAGENT_94BAR_CODE_NUM - i*5 - 5;
            uliValue = g_uc94BarCodeBuffer[ucNum];
            uliValue = uliValue*94 + g_uc94BarCodeBuffer[ucNum+1];
            uliValue = uliValue*94 + g_uc94BarCodeBuffer[ucNum+2];
            uliValue = uliValue*94 + g_uc94BarCodeBuffer[ucNum+3];
            uliValue = uliValue*94 + g_uc94BarCodeBuffer[ucNum+4];
            g_ucBarCodeByteBuffer[REAGENT_BAR_CODE_BYTE_NUM-4*i-4]   = (uliValue>>24)&0xFF;
            g_ucBarCodeByteBuffer[REAGENT_BAR_CODE_BYTE_NUM-4*i-3]   = (uliValue>>16)&0xFF;
            g_ucBarCodeByteBuffer[REAGENT_BAR_CODE_BYTE_NUM-4*i-2]   = (uliValue>>8)&0xFF;
            g_ucBarCodeByteBuffer[REAGENT_BAR_CODE_BYTE_NUM-4*i-1]   = (uliValue)&0xFF;
            if(ucNum<=5)
            {
                break;
            }
        }
        uliValue = g_uc94BarCodeBuffer[1];
        uliValue = uliValue*94 + g_uc94BarCodeBuffer[2];
        uliValue = uliValue*94 + g_uc94BarCodeBuffer[3];
        uliValue = uliValue*94 + g_uc94BarCodeBuffer[4];
        g_ucBarCodeByteBuffer[1] = (uliValue>>16)&0xFF;
        g_ucBarCodeByteBuffer[2] = (uliValue>>8)&0xFF;
        g_ucBarCodeByteBuffer[3] = (uliValue&0xFF)&0xFF;
        g_ucBarCodeByteBuffer[0] = REAGENT_BAR_INFO_HEADER; //��־λ
        
        //��ʾ��Ŀ����
        if(st==0)
        {
            memcpy(g_tSelectResultInfo.ucName,g_ucBarCodeByteBuffer+1,10);
            g_tSelectResultInfo.ucName[10] = '\0';
            DisplayText(0x06C8,g_tSelectResultInfo.ucName);
        }
        else 
        {
            memcpy(g_tSystemErrorAdjustment.ucName,g_ucBarCodeByteBuffer+1,10);
            g_tSystemErrorAdjustment.ucName[10] = '\0';
            DisplayText(0x0718,g_tSystemErrorAdjustment.ucName);
        }
        return ;
    }
    else if(g_uc94BarCodeBuffer[0] == '@') //�Լ�����Ҫ��Ϣ
    {
        StartHandDeviceScanFlag = 0;
        Convert94ToHex(g_uc94BarCodeBuffer,REAGENT_BRIEF_94BAR_CODE_NUM);
        for(i=0;;i++)
        {
            ucNum = (unsigned char)REAGENT_BRIEF_94BAR_CODE_NUM - i*5 - 5;
            uliValue = g_uc94BarCodeBuffer[ucNum];
            uliValue = uliValue*94 + g_uc94BarCodeBuffer[ucNum+1];
            uliValue = uliValue*94 + g_uc94BarCodeBuffer[ucNum+2];
            uliValue = uliValue*94 + g_uc94BarCodeBuffer[ucNum+3];
            uliValue = uliValue*94 + g_uc94BarCodeBuffer[ucNum+4];
            g_ucBarCodeByteBuffer[REAGENT_BRIEF_BAR_CODE_BYTE_NUM-4*i-4]  = (uliValue>>24)&0xFF;
            g_ucBarCodeByteBuffer[REAGENT_BRIEF_BAR_CODE_BYTE_NUM-4*i-3]  = (uliValue>>16)&0xFF;
            g_ucBarCodeByteBuffer[REAGENT_BRIEF_BAR_CODE_BYTE_NUM-4*i-2]  = (uliValue>>8)&0xFF;
            g_ucBarCodeByteBuffer[REAGENT_BRIEF_BAR_CODE_BYTE_NUM-4*i-1]  = (uliValue)&0xFF;
            if(ucNum<5)
            {
                break;
            }
        }
        uliValue = g_uc94BarCodeBuffer[1];
        uliValue = uliValue*94 + g_uc94BarCodeBuffer[2];
        g_ucBarCodeByteBuffer[0] = (uliValue)&0xFF;
        if(st == 0)
        {
            memcpy(g_tSelectResultInfo.ucName,g_ucBarCodeByteBuffer,10);    //��Ŀ����
            g_tSelectResultInfo.ucName[10] = '\0';
            DisplayText(0x06C8,g_tSelectResultInfo.ucName);
        }
        else 
        {
            memcpy(g_tSystemErrorAdjustment.ucName,g_ucBarCodeByteBuffer,10);    //��Ŀ����
            g_tSystemErrorAdjustment.ucName[10] = '\0';
            DisplayText(0x0718,g_tSystemErrorAdjustment.ucName);
        }
        return ;
    }
}
/*******************************************************************************
 * �������ƣ�AnalysisReagentDetailInfo
 * ���������������Լ�������ϸ��Ϣ
 * ���������p ��Ҫ�������ַ�����num ͨ����
 * �����������
 * ����ֵ  ����
 * �޸�����            �汾��            �޸���            �޸�����
 * -----------------------------------------------------------------------------
 * 2017/08/30          V1.0              ��־ǿ               ����
 *******************************************************************************/
void AnalysisReagentDetailInfo(unsigned char *p,unsigned char num)
{
    unsigned char i=0,j=0,k=11,m=168;
    unsigned char ucTemp[4];
    
    unsigned char ucValue1=0,ucValue2=0;
    
    //�����λ
    g_tTestInfo[num].ucUnit                 = ((p[22]&0x07)<<1) | ((p[23]&0x80)>>7);
    //ϡ�ͱ���
    g_tTestInfo[num].ucDilute               = p[23]&0x7F;
    
    //����Ϊ���̲���
    g_tRunProcessFile[num].uiProcess1Timer  = p[k++] * 10 * 10;  //ms
    //g_tRunProcessFile[num].uiProcess1Timer  = g_tRunProcessFile[num].uiProcess1Timer - 300;  //ms
    g_tRunProcessFile[num].uiProcess2Timer  = p[k++] * 10 * 10;  //ms
    //g_tRunProcessFile[num].uiProcess2Timer  = g_tRunProcessFile[num].uiProcess2Timer + 300;  //ms
    if(g_tTestInfo[num].ucDilute > 1)
        g_tRunProcessFile[num].uiDilutionVol    = p[k] * (g_tTestInfo[num].ucDilute - 1);   //ϡ��Һ�����
    g_tRunProcessFile[num].uiSampleVol      = p[k];
    g_tRunProcessFile[num].uiSample         = PulseForVolume(p[k++]);
    g_tRunProcessFile[num].uiDilutionSample = PulseForVolume(p[k++]);
    g_tRunProcessFile[num].uiFistReagent    = PulseForVolume(p[k++]);
    g_tRunProcessFile[num].uiSecondReagent  = PulseForVolume(p[k++]);
    g_tRunProcessFile[num].uiThirdReagent   = PulseForVolume(p[k++]);
    g_tRunProcessFile[num].ucMixTimes       = 3;
    k = k + 4;
    ucValue1 = p[k++];
    ucValue2 = p[k];
    g_tTestInfo[num].ucUnit  = ((ucValue1&0x07)<<1) | ((ucValue2&0x80)>>7);
    g_tTestInfo[num].ucDilute = p[k++]&0x7F;
    
    for(i=0;i<16;i++)
    {
        for(j=0;j<4;j++)
        {
            ucTemp[3-j] = p[k++];
        }
        memcpy(&g_tStandCurve[num].fCurveParam1[i],&ucTemp,4);
    }
    for(i=0;i<16;i++)
    {
        for(j=0;j<4;j++)
        {
            ucTemp[3-j] = p[k++];
        }
        memcpy(&g_tStandCurve[num].fCurveParam2[i],&ucTemp,4);
    }
    for(j=0;j<4;j++)
    {
        ucTemp[3-j] = p[k++];
    }
    memcpy(&g_tStandCurve[num].fLowLimit,&ucTemp,4);
    for(j=0;j<4;j++)
    {
        ucTemp[3-j] = p[k++];
    }
    memcpy(&g_tStandCurve[num].fHighLimit,&ucTemp,4);
    for(j=0;j<4;j++)
    {
        ucTemp[3-j] = p[k++];
    }
    memcpy(&g_tStandCurve[num].fLowReferencevalue,&ucTemp,4);
    for(j=0;j<4;j++)
    {
        ucTemp[3-j] = p[k++];
    }
    memcpy(&g_tStandCurve[num].fHighReferencevalue,&ucTemp,4);
//����ΪȫѪ���̲���
    g_tTestInfo[num].ucDilute_Wholeblood = p[173];
    if(g_tTestInfo[num].ucDilute_Wholeblood > 1)
        g_tRunProcessFile[num].uiDilutionVol_Wholeblood    = p[m] * (g_tTestInfo[num].ucDilute_Wholeblood - 1);   //ϡ��Һ�����
    g_tRunProcessFile[num].uiSampleVol_Wholeblood      = p[m];
    g_tRunProcessFile[num].uiSample_Wholeblood         = PulseForVolume(p[m++]);
    g_tRunProcessFile[num].uiDilutionSample_Wholeblood = PulseForVolume(p[m++]);
    g_tRunProcessFile[num].uiFistReagent_Wholeblood    = PulseForVolume(p[m++]);
    g_tRunProcessFile[num].uiSecondReagent_Wholeblood  = PulseForVolume(p[m++]);
    g_tRunProcessFile[num].uiThirdReagent_Wholeblood   = PulseForVolume(p[m++]);
    
    g_tTestInfo[num].ucResultPoint = p[174];
    if(g_tTestInfo[num].ucResultPoint > 154)
        g_tTestInfo[num].ucResultPoint = g_tTestInfo[num].ucResultPoint - 155;
    else 
        g_tTestInfo[num].ucResultPoint = 3;
}
/*******************************************************************************
 * �������ƣ�FindReadReagentDetailInfo
 * ����������Ѱ�ҺͶ�ȡ�Լ�����ϸ��Ϣ
 * ���������num ͨ����
 * �����������
 * ����ֵ  ��0 ��ȡ�ɹ�  1 ��ȡʧ��
 * �޸�����            �汾��            �޸���            �޸�����
 * -----------------------------------------------------------------------------
 * 2017/08/30          V1.0              ��־ǿ               ����
 *******************************************************************************/
unsigned char FindReadReagentDetailInfo(unsigned char num)
{
    unsigned int i=0;
    unsigned char j=0;
    unsigned char ucTemp[REAGENT_BAR_INFO_SIZE],ucTemp1[15];
    unsigned char ucName[11],ucName1[11],ucName2[3],ucLotNum[3];
    unsigned long int uliAddr = 0;
    
    g_tTestInfo[num].ucReagentBarFlag = 0;
    //Ѱ���Ƿ�����ͬ����Ŀ���ƺ�����
    for(i=0;i<num;i++)
    {
        if((0 == strcmp((const char *)g_tTestInfo[i].ucName,(const char *)g_tTestInfo[num].ucName)) && 
           (0 == strncmp((const char *)g_tTestInfo[i].ucLotNum,(const char *)g_tTestInfo[num].ucLotNum,3)) && (g_tTestInfo[i].ucReagentBarFlag == 1))
        {
            g_tRunProcessFile[num].uiProcess1Timer = g_tRunProcessFile[i].uiProcess1Timer;
            g_tRunProcessFile[num].uiProcess2Timer = g_tRunProcessFile[i].uiProcess2Timer;
            g_tTestInfo[num].ucDilute              = g_tTestInfo[i].ucDilute;
            g_tRunProcessFile[num].uiDilutionVol   = g_tRunProcessFile[i].uiDilutionVol;
            g_tRunProcessFile[num].uiDilutionSample= g_tRunProcessFile[i].uiDilutionSample;
            g_tRunProcessFile[num].uiSample        = g_tRunProcessFile[i].uiSample;
            g_tRunProcessFile[num].uiSampleVol     = g_tRunProcessFile[i].uiSampleVol;
            g_tRunProcessFile[num].uiFistReagent   = g_tRunProcessFile[i].uiFistReagent;
            g_tRunProcessFile[num].uiSecondReagent = g_tRunProcessFile[i].uiSecondReagent;
            g_tRunProcessFile[num].uiThirdReagent  = g_tRunProcessFile[i].uiThirdReagent;
            g_tRunProcessFile[num].ucMixTimes      = g_tRunProcessFile[i].ucMixTimes;
            memcpy(&g_tStandCurve[num],&g_tStandCurve[i],sizeof(g_tStandCurve[0]));
            g_tTestInfo[num].ucUnit = g_tTestInfo[i].ucUnit;
            g_tTestInfo[num].ucReagentBarFlag = 1;
            
            g_tTestInfo[num].ucDilute_Wholeblood              = g_tTestInfo[i].ucDilute_Wholeblood;
            g_tRunProcessFile[num].uiDilutionVol_Wholeblood   = g_tRunProcessFile[i].uiDilutionVol_Wholeblood;
            g_tRunProcessFile[num].uiDilutionSample_Wholeblood= g_tRunProcessFile[i].uiDilutionSample_Wholeblood;
            g_tRunProcessFile[num].uiSample_Wholeblood        = g_tRunProcessFile[i].uiSample_Wholeblood;
            g_tRunProcessFile[num].uiSampleVol_Wholeblood     = g_tRunProcessFile[i].uiSampleVol_Wholeblood;
            g_tRunProcessFile[num].uiFistReagent_Wholeblood   = g_tRunProcessFile[i].uiFistReagent_Wholeblood;
            g_tRunProcessFile[num].uiSecondReagent_Wholeblood = g_tRunProcessFile[i].uiSecondReagent_Wholeblood;
            g_tRunProcessFile[num].uiThirdReagent_Wholeblood  = g_tRunProcessFile[i].uiThirdReagent_Wholeblood;
            
            g_tTestInfo[num].ucResultPoint  = g_tTestInfo[i].ucResultPoint;
            return 0;
        }
    }
    //Ѱ���ѱ�����Լ���Ϣ�Ƿ�����ͬ����Ŀ���ƺ�����
    for(i=0;i<100;i++)
    {
        uliAddr = REAGENT_BAR_INFO_ADDR + REAGENT_BAR_INFO_SIZE * i * 3;
        W25QXX_Read(ucTemp1,uliAddr,11);
        if(ucTemp1[0] == REAGENT_BAR_INFO_HEADER)
        {
            
            memcpy(ucName,ucTemp1+1,10);
            ucName[10] = '\0';
            //�Լ����Ÿ�ʽ ��һ���ֽڵĵ�7λΪ�꣬�ڶ����ֽڵ�2~5��λΪ�£��ڶ����ֽڵĵ���λ�͵������ֽ�Ϊ���к�
            if((0 == strcmp((const char *)g_tTestInfo[num].ucName,(const char *)ucName)))
            {
                for(j=0;j<3;j++)
                {
                    uliAddr = REAGENT_BAR_INFO_ADDR + REAGENT_BAR_INFO_SIZE * (i*3+j);
                    W25QXX_Read(ucTemp1,uliAddr,11);
                    memcpy(ucName1,ucTemp1+1,10);
                    ucName1[10] = '\0';
                    W25QXX_Read(ucLotNum,uliAddr+20,3);
                    
                    ucName2[0] = (ucLotNum[0]&0xFE)>>1;//��
                    ucName2[1] = ((ucLotNum[0]&0x01)<<5) | ((ucLotNum[1]&0xF8)>>3);
                    ucName2[2] = ((ucLotNum[1]&0x07)<<5) | ((ucLotNum[2]&0xF8)>>3);
                    if((0 == strcmp((const char *)g_tTestInfo[num].ucName,(const char *)ucName1)) && (0 == strncmp((const char *)g_tTestInfo[num].ucLotNum,(const char *)ucName2,3)))
                    {
                        W25QXX_Read(ucTemp,uliAddr,REAGENT_BAR_INFO_SIZE);
                        AnalysisReagentDetailInfo(ucTemp,num);
                        g_tTestInfo[num].ucReagentBarFlag = 1;
                        return 0;
                    }
                }  
            }
        }
    }
    //δ�ҵ���ͬ����Ŀ���ƺ��Լ�����
    return 1;
}
/*******************************************************************************
 * �������ƣ�AutoGetReagentBriefBarCode
 * ������������ȡ�Լ����ļ�Ҫ��Ϣ
 * �����������
 * �����������
 * ����ֵ  ��0 ��ȡ�ɹ�  1 ��ȡʧ��
 * �޸�����            �汾��            �޸���            �޸�����
 * -----------------------------------------------------------------------------
 * 2017/08/30          V1.0              ��־ǿ               ����
 *******************************************************************************/
unsigned char AutoGetReagentBriefBarCode(void)
{
    unsigned char i=0,j=0,k=0,m=0;
    unsigned char ucNum=0;
    unsigned int uiValue=0,uiValue2=0;
    unsigned long int uliValue=0;
    unsigned char ucTemp[30];
    unsigned char num=0;
    unsigned char ucString[] = "�Լ�����Ϣɨ��ʧ��ͨ����";
    SPIMotor(4,MT_HOME,0);      //�Լ��̵����λ
    for(i=0;i<12;i++)
    {
        g_tTestInfo[i].ucAutoReagentBarFlag = 0;
        if(g_tTestInfo[i].ucSampleID[0] != '\0' && g_tTestInfo[i].ucName[0] == '\0')   
        {
            //����Ƿ�������Լ���
            SPIMotor(4,MT_MOVE_TO,g_tReagentTrayMotor.uiaAutoScanPosition[i]);
            //SPIMotor(4,MT_MOVE_TO,3250);
            for(k=0;k<1;k++)
            {
                Delay1ms(100);
                SPIStartBuiltInScan();
                SPIGetBuiltInScanInfo(g_uc94BarCodeBuffer);
                if(g_uc94BarCodeBuffer[0] == '@')
                {
                    g_tTestInfo[i].ucAutoReagentBarFlag = 1;
                    break;
                }
                    
                SPIMotor(4,MT_MOVE_TO,g_tReagentTrayMotor.uiaAutoScanPosition[i]-20);
                Delay1ms(100);
                SPIStartBuiltInScan();               
                SPIGetBuiltInScanInfo(g_uc94BarCodeBuffer);
                if(g_uc94BarCodeBuffer[0] == '@')
                {
                    g_tTestInfo[i].ucAutoReagentBarFlag = 1;
                    break;
                }
                SPIMotor(4,MT_MOVE_TO,g_tReagentTrayMotor.uiaAutoScanPosition[i]+40);
                Delay1ms(100);
                SPIStartBuiltInScan();     
                SPIGetBuiltInScanInfo(g_uc94BarCodeBuffer);
                if(g_uc94BarCodeBuffer[0] == '@')
                {
                    g_tTestInfo[i].ucAutoReagentBarFlag = 1;
                    break;
                }
                SPIMotor(4,MT_MOVE_TO,g_tReagentTrayMotor.uiaAutoScanPosition[i]-50);
                Delay1ms(100);
                SPIStartBuiltInScan();     
                SPIGetBuiltInScanInfo(g_uc94BarCodeBuffer);
                if(g_uc94BarCodeBuffer[0] == '@')
                {
                    g_tTestInfo[i].ucAutoReagentBarFlag = 1;
                    break;
                }
                SPIMotor(4,MT_MOVE_TO,g_tReagentTrayMotor.uiaAutoScanPosition[i]+60);
                Delay1ms(100);
                SPIStartBuiltInScan();     
                SPIGetBuiltInScanInfo(g_uc94BarCodeBuffer);
                if(g_uc94BarCodeBuffer[0] == '@')
                {
                    g_tTestInfo[i].ucAutoReagentBarFlag = 1;
                    break;
                }
                if((i+1)<10)
                {
                    ucTemp[m++] = (i+1) + 0x30;
                }
                else
                {
                    ucTemp[m++] = (i+1)/10 + 0x30;
                    ucTemp[m++] = (i+1)%10 + 0x30;
                }
                ucTemp[m++] = ',';
                num++;
            }
            //SPIGetBuiltInScanInfo(g_uc94BarCodeBuffer);
            if(g_uc94BarCodeBuffer[0] == '@') //�Լ�����Ҫ��Ϣ
            {
                Convert94ToHex(g_uc94BarCodeBuffer,REAGENT_BRIEF_94BAR_CODE_NUM);
                for(j=0;;j++)
                {
                    ucNum = (unsigned char)REAGENT_BRIEF_94BAR_CODE_NUM - j*5 - 5;
                    uliValue = g_uc94BarCodeBuffer[ucNum];
                    uliValue = uliValue*94 + g_uc94BarCodeBuffer[ucNum+1];
                    uliValue = uliValue*94 + g_uc94BarCodeBuffer[ucNum+2];
                    uliValue = uliValue*94 + g_uc94BarCodeBuffer[ucNum+3];
                    uliValue = uliValue*94 + g_uc94BarCodeBuffer[ucNum+4];
                    g_ucBarCodeByteBuffer[REAGENT_BRIEF_BAR_CODE_BYTE_NUM-4*j-4]  = (uliValue>>24)&0xFF;
                    g_ucBarCodeByteBuffer[REAGENT_BRIEF_BAR_CODE_BYTE_NUM-4*j-3]  = (uliValue>>16)&0xFF;
                    g_ucBarCodeByteBuffer[REAGENT_BRIEF_BAR_CODE_BYTE_NUM-4*j-2]  = (uliValue>>8)&0xFF;
                    g_ucBarCodeByteBuffer[REAGENT_BRIEF_BAR_CODE_BYTE_NUM-4*j-1]  = (uliValue)&0xFF;
                    if(ucNum<5)
                    {
                        break;
                    }
                }
                uliValue = g_uc94BarCodeBuffer[1];
                uliValue = uliValue*94 + g_uc94BarCodeBuffer[2];
                g_ucBarCodeByteBuffer[0] = (uliValue)&0xFF;

                memcpy(g_tTestInfo[i].ucName,g_ucBarCodeByteBuffer,10);    //��Ŀ����
                g_tTestInfo[i].ucName[10] = '\0';
                g_tTestInfo[i].ucExpiryDate[0] = (g_ucBarCodeByteBuffer[10]&0xFE)>>1;  //��Ч�� ��
                g_tTestInfo[i].ucExpiryDate[1] = ((g_ucBarCodeByteBuffer[10]&0x01)<<3) | ((g_ucBarCodeByteBuffer[11]&0xE0)>>5);  //��Ч�� ��
                g_tTestInfo[i].ucExpiryDate[2] = g_ucBarCodeByteBuffer[11]&0x1F;  //��Ч�� ��
                g_tTestInfo[i].ucLotNum[0] = (g_ucBarCodeByteBuffer[12]&0xFE)>>1; 
                uiValue= ((g_ucBarCodeByteBuffer[12]&0x01)<<3) | ((g_ucBarCodeByteBuffer[13]&0xE0)>>5);
                uiValue2 = ((g_ucBarCodeByteBuffer[13]&0x1F)<<5) | ((g_ucBarCodeByteBuffer[14]&0xF8)>>3);
                g_tTestInfo[i].ucLotNum[1] = (uiValue<<2) | (uiValue2>>8);
                g_tTestInfo[i].ucLotNum[2] =  uiValue2 & 0xFF;
                g_tTestInfo[i].uliReagentKitNum = g_ucBarCodeByteBuffer[14]&0x07;
                g_tTestInfo[i].uliReagentKitNum = g_tTestInfo[i].uliReagentKitNum * 65536 + (unsigned long int)g_ucBarCodeByteBuffer[15] * 256 + (unsigned long int)g_ucBarCodeByteBuffer[16];
                //CopyTestSetInfo(&g_tTestInfo[i],&g_tTestInfoTemp);
                SendTestInfoToAddr(&g_tTestInfo[i],i);
            }    
       }
    }
    if(num > 0)
    {
        DisplayPicture(37);
        DisplayText(0x0540,ucString);
        ucTemp[m-1] = '\0';
        DisplayText(0x0550,ucTemp);
        while(g_ucUartLcdRecvFlag == 0); //�ȴ�ѡ��
        g_ucUartLcdRecvFlag = 0;
        AnalyticInstruction();
        if(g_tLcdAnalyticIns.uiLcdParamAddr == 0x0D )
        {
            if(g_tLcdAnalyticIns.uiButtonValue == 1)//����
            {
                AutoGetReagentBriefBarCodeFlag = 0;
                DisplayPicture(TEST_PICTURE1);
                g_tLcdAnalyticIns.uiPageNum = 1;
            }  
            else if(g_tLcdAnalyticIns.uiButtonValue == 2)//�ֶ�ɨ���Լ�����Ϣ
            {
                AutoGetReagentBriefBarCodeFlag = 1;
                DisplayPicture(TEST_PICTURE1);
                g_tLcdAnalyticIns.uiPageNum = 1;
                SPIMotor(4,MT_HOME,0);      //�Լ��̵����λ
            }     
        }
    }
    else
    {
        AutoGetReagentBriefBarCodeFlag = 0;
    }
}
