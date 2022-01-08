/**
 * +----------------------------------------------------------------------
 * | CodeEngine
 * +----------------------------------------------------------------------
 * | Copyright 艾邦
 * +----------------------------------------------------------------------
 * | Licensed ( http://www.apache.org/licenses/LICENSE-2.0 )
 * +----------------------------------------------------------------------
 * | Author: TaoGe <liangtao.gz@foxmail.com>
 * +----------------------------------------------------------------------
 * | Date: 2022/1/8 11:45
 * +----------------------------------------------------------------------
 */

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
function LogisticOperation(Param, Value) {
  let fA = 0, fB = 0, fC = 0, fD = 0;
  let fTemp = 0;
  let uliValue = 0;
  fA = Param[ 0 ];
  fB = Param[ 1 ];
  fC = Param[ 2 ];
  fD = Param[ 3 ];
  //四参数公式  y=(a-d)/(1+(x/c)^b) + d;
  fTemp = Value / fC;
  fTemp = Math.pow(fTemp, fB) + 1.0;
  fTemp = ( fA - fD ) / fTemp + fD;
  uliValue = Math.round(fTemp);
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
function LogisticOperation1(Param, Value) {
  let fA = 0, fB = 0, fC = 0, fD = 0;
  let fTemp = 0;
  let uliValue = 0;
  fA = Param[ 0 ];
  fB = Param[ 1 ];
  fC = Param[ 2 ];
  fD = Param[ 3 ];
  //Logistic  y=a+(b-a)/[1+e^(-c*x+d)]]
  fTemp = fD - fC * Value;
  fTemp = Math.exp(fTemp) + 1.0;
  fTemp = ( fB - fA ) / fTemp + fA;
  uliValue = Math.round(fTemp);
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
function LogisticInverseOperation(Param, Value) {
  let fA = 0, fB = 0, fC = 0, fD = 0;
  let fValue = 0;

  fA = Param[ 0 ];
  fB = Param[ 1 ];
  fC = Param[ 2 ];
  fD = Param[ 3 ];

  if (Value <= fD || ( Value >= fA )) {
    return - 1;
  } else {
    fValue = ( fA - fD ) / ( Value - fD ) - 1;
    fValue = fC * Math.pow(fValue, 1 / fB);
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
function LogisticInverseOperation1(Param, Value) {
  let fA = 0, fB = 0, fC = 0, fD = 0;
  let fValue = 0;

  fA = Param[ 0 ];
  fB = Param[ 1 ];
  fC = Param[ 2 ];
  fD = Param[ 3 ];

  //if(Value <= fD || (Value >= fA))
  //{
  //    return -1;
  //}
  //else
  //{
  fValue = ( fB - fA ) / ( Value - fA ) - 1;
  fValue = fD - Math.log(fValue);
  fValue = fValue / fC;
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
function LineOperation(Param, Value) {
  let uliValue = 0;
  uliValue = Param[ 2 ] * Value + Param[ 3 ];
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
function LineInverseOperation(Param, Value) {
  let fValue = 0;
  fValue = ( parseFloat(Value) - Param[ 3 ] ) / Param[ 2 ];
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
function CalculateSignalValue(Param, Value) {
  let uliValue = 0;
  if (Param[ 0 ] === 0) {
    uliValue = LineOperation(Param, Value);
  } else if (Param[ 0 ] > 0) {
    uliValue = LogisticOperation(Param, Value);
  } else {
    uliValue = LogisticOperation1(Param, Value);
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
function CalculateConcentration(Param, Value) {
  let fValue = 0;
  if (Param[ 0 ] === 0) {
    fValue = LineInverseOperation(Param, Value);
    return fValue;
  } else if (Param[ 0 ] > 0) {
    fValue = LogisticInverseOperation(Param, Value);
    return fValue;
  } else {
    fValue = LogisticInverseOperation1(Param, Value);
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
function LcdCalculateConcentration(g_tTestInfo, g_tStandCurve) {
  let i = 0;
  let uliTransitionPeriodL1 = 0, uliTransitionPeriodL2 = 0, uliTransitionPeriodH1 = 0, uliTransitionPeriodH2 = 0;
  let uliLowValue = 0, uliHighValue = 0;  //最低和最高检测浓度对应的信号值
  const fParam1 = [], fParam2 = [], fParam3 = [];
  let fValue1 = 0, fValue2 = 0, fRatio = 0;
  // let SystemErrorAdjustmentFlag = 0;
  //1、根据样本类型分配不同的标准曲线，一共有三段标准曲线
  if (g_tTestInfo.hasOwnProperty('ucSampleType') && ( ( g_tTestInfo.ucSampleType === 1 ) || ( g_tTestInfo.ucSampleType === 2 ) || ( g_tTestInfo.ucSampleType === 4 ) || ( g_tTestInfo.ucSampleType === 5 ) ))    //血清、者血浆或质控品
  {
    for ( i = 0; i < 4; i ++ ) {
      fParam1[ i ] = g_tStandCurve.fCurveParam1[ i ];
    }
    uliTransitionPeriodL1 = Math.round(g_tStandCurve.fCurveParam1[ 4 ]);
    uliTransitionPeriodH1 = Math.round(g_tStandCurve.fCurveParam1[ 5 ]);
    for ( i = 0; i < 4; i ++ ) {
      fParam2[ i ] = g_tStandCurve.fCurveParam1[ 6 + i ];
    }
    uliTransitionPeriodL2 = parseInt(g_tStandCurve.fCurveParam1[ 10 ]);
    uliTransitionPeriodH2 = parseInt(g_tStandCurve.fCurveParam1[ 11 ]);
    for ( i = 0; i < 4; i ++ ) {
      fParam3[ i ] = g_tStandCurve.fCurveParam1[ 12 + i ];
    }
  } else if (g_tTestInfo.ucSampleType === 3) //原血
  {
    for ( i = 0; i < 4; i ++ ) {
      fParam1[ i ] = g_tStandCurve.fCurveParam2[ i ];
    }
    uliTransitionPeriodL1 = parseInt(g_tStandCurve.fCurveParam2[ 4 ]);
    uliTransitionPeriodH1 = parseInt(g_tStandCurve.fCurveParam2[ 5 ]);
    for ( i = 0; i < 4; i ++ ) {
      fParam2[ i ] = g_tStandCurve.fCurveParam2[ 6 + i ];
    }
    uliTransitionPeriodL2 = parseInt(g_tStandCurve.fCurveParam2[ 10 ]);
    uliTransitionPeriodH2 = parseInt(g_tStandCurve.fCurveParam2[ 11 ]);
    for ( i = 0; i < 4; i ++ ) {
      fParam3[ i ] = g_tStandCurve.fCurveParam2[ 12 + i ];
    }
  }
  //2、根据标准曲线计算最低和最高检测浓度对应的信号值
  uliLowValue = CalculateSignalValue(fParam1, g_tStandCurve.fLowLimit);       //计算最低检测浓度对应的信号值
  if (uliTransitionPeriodH1 === 0)  //表示只有一条标准曲线
    uliHighValue = CalculateSignalValue(fParam1, g_tStandCurve.fHighLimit); //计算最高检测浓度对应的信号值
  else if (uliTransitionPeriodH2 === 0)  //表示只有两条标准曲线
    uliHighValue = CalculateSignalValue(fParam2, g_tStandCurve.fHighLimit); //计算最高检测浓度对应的信号值
  else    //表示有三条标准曲线
    uliHighValue = CalculateSignalValue(fParam3, g_tStandCurve.fHighLimit); //计算最高检测浓度对应的信号值

  /*  //读取系统误差调整系数并处理参数
    memcpy(g_tSystemErrorAdjustment.ucName,g_tTestInfo.ucName,10);
    g_tSystemErrorAdjustment.ucName[10] = '\0';
    ReadSystemErrorAdjustment();
    if(0 == g_tSystemErrorAdjustment.ucKValueSymbol)
    {
      g_tSystemErrorAdjustment.ucKValue = 0 - g_tSystemErrorAdjustment.ucKValue;
    }
    if(0 == g_tSystemErrorAdjustment.ucBValueSymbol)
    {
      g_tSystemErrorAdjustment.ucBValue = 0 - g_tSystemErrorAdjustment.ucBValue;
    }*/
  //3、所采集的信号值和uliLowValue、uliHighValue进行比较，判断是否超出了检测的范围
  if (g_tTestInfo.hasOwnProperty('uliFitValue') && g_tTestInfo.uliFitValue < uliLowValue) {
    // g_tTestInfo.uliFitValue = (unsigned long int)((float)g_tTestInfo.uliFitValue*g_tSystemErrorAdjustment.ucKValue + g_tSystemErrorAdjustment.ucBValue);
    // SystemErrorAdjustmentFlag=1;
    if (g_tTestInfo.uliFitValue < uliLowValue) {
      g_tTestInfo.ucResultFlag = 2;  //小于
      g_tTestInfo.fResult = g_tStandCurve.fLowLimit;
      return g_tTestInfo;
    }
  } else if (g_tTestInfo.hasOwnProperty('uliFitValue') && g_tTestInfo.uliFitValue > uliHighValue) {
    // g_tTestInfo.uliFitValue = (unsigned long int)((float)g_tTestInfo.uliFitValue*g_tSystemErrorAdjustment.ucKValue + g_tSystemErrorAdjustment.ucBValue);
    // SystemErrorAdjustmentFlag=1;
    if (g_tTestInfo.uliFitValue > uliHighValue) {
      g_tTestInfo.ucResultFlag = 3;  //大于
      g_tTestInfo.fResult = g_tStandCurve.fHighLimit;
      return g_tTestInfo;
    }
  }

  //3、在检测浓度范围内，计算浓度
  if (g_tTestInfo.hasOwnProperty('uliFitValue') && g_tTestInfo.uliFitValue <= uliTransitionPeriodL1) {
    fValue1 = CalculateConcentration(fParam1, g_tTestInfo.uliFitValue);
  } else if (g_tTestInfo.hasOwnProperty('uliFitValue') && ( g_tTestInfo.uliFitValue > uliTransitionPeriodL1 ) && ( g_tTestInfo.uliFitValue < uliTransitionPeriodH1 )) {
    fValue1 = CalculateConcentration(fParam1, g_tTestInfo.uliFitValue);
    fValue2 = CalculateConcentration(fParam2, g_tTestInfo.uliFitValue);
    fRatio = ( parseFloat(g_tTestInfo.uliFitValue - uliTransitionPeriodL1) ) / ( uliTransitionPeriodH1 - uliTransitionPeriodL1 );
    fValue1 = ( 1 - fRatio ) * fValue1 + fRatio * fValue2;
  } else if (g_tTestInfo.hasOwnProperty('uliFitValue') && ( g_tTestInfo.uliFitValue >= uliTransitionPeriodH1 ) && ( g_tTestInfo.uliFitValue <= uliTransitionPeriodL2 )) {
    fValue1 = CalculateConcentration(fParam2, g_tTestInfo.uliFitValue);
  } else if (g_tTestInfo.hasOwnProperty('uliFitValue') && ( g_tTestInfo.uliFitValue > uliTransitionPeriodL2 ) && ( g_tTestInfo.uliFitValue < uliTransitionPeriodH2 )) {
    fValue1 = CalculateConcentration(fParam2, g_tTestInfo.uliFitValue);
    fValue2 = CalculateConcentration(fParam3, g_tTestInfo.uliFitValue);
    fRatio = ( parseFloat(g_tTestInfo.uliFitValue - uliTransitionPeriodL2) ) / ( uliTransitionPeriodH2 - uliTransitionPeriodL2 );
    fValue1 = ( 1 - fRatio ) * fValue1 + fRatio * fValue2;
  } else if (g_tTestInfo.hasOwnProperty('uliFitValue')) {
    fValue1 = CalculateConcentration(fParam3, g_tTestInfo.uliFitValue);
  }
  g_tTestInfo.ucResultFlag = 1;  //等于
  g_tTestInfo.fResult = fValue1;

  //srand((unsigned)time(NULL));

  //系统误差调整
  // if (SystemErrorAdjustmentFlag === 0) {
  //   g_tTestInfo.fResult = g_tTestInfo.fResult * g_tSystemErrorAdjustment.ucKValue + g_tSystemErrorAdjustment.ucBValue;
  // }
  if (g_tTestInfo.fResult < g_tStandCurve.fLowLimit) {
    g_tTestInfo.ucResultFlag = 2;  //小于
    g_tTestInfo.fResult = g_tStandCurve.fLowLimit;
  } else if (g_tTestInfo.fResult > g_tStandCurve.fHighLimit) {
    g_tTestInfo.ucResultFlag = 3;  //大于
    g_tTestInfo.fResult = g_tStandCurve.fHighLimit;
  }
  return g_tTestInfo;
}

module.exports = LcdCalculateConcentration
