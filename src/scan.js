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
 * | Date: 2022/1/7 16:44
 * +----------------------------------------------------------------------
 */

const SerialPort = require("serialport");
const LcdCalculateConcentration = require("./procedure");
const Readline = SerialPort.parsers.Readline;

const REAGENT_BAR_CODE_BYTE_NUM = 176;
const REAGENT_BAR_INFO_HEADER = "!"
const REAGENT_94BAR_CODE_NUM = 220;

// 混淆秘钥
const Str94Table = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz`~!@#$%^&*()-_=+[{]};:'\"\\|,<.>/?";

// 混淆解码
function convert94ToHex(buf) {
  const p = Array.from(buf.toString('ascii'))
  const k = Array.from(Str94Table)
  for ( let i = 0; i < p.length; i ++ ) {
    for ( let j = 0; j < 94; j ++ ) {
      if (p[ i ] === k[ j ]) {
        p[ i ] = j;
        break;
      }
    }
  }
  return p;
}

/*******************************************************************************
 * 函数名称：LcdGetReagentDetailInfo
 * 功能描述：获取试剂盒的详细信息
 * 输入参数：无
 * 输出参数：无
 * 返回值  ：0 获取成功  1 获取失败
 * 修改日期            版本号            修改人            修改内容
 * -----------------------------------------------------------------------------
 * 2017/08/29          V1.0              周志强               创建
 *******************************************************************************/
function lcdGetReagentDetailInfo(g_uc94BarCodeBuffer) {
  const g_ucBarCodeByteBuffer = [];
  let uliValue, uiValue
  for ( let i = 0; ; i ++ ) {
    let ucNum = REAGENT_94BAR_CODE_NUM - i * 5 - 5;
    uliValue = g_uc94BarCodeBuffer[ ucNum ];
    uliValue = uliValue * 94 + g_uc94BarCodeBuffer[ ucNum + 1 ];
    uliValue = uliValue * 94 + g_uc94BarCodeBuffer[ ucNum + 2 ];
    uliValue = uliValue * 94 + g_uc94BarCodeBuffer[ ucNum + 3 ];
    uliValue = uliValue * 94 + g_uc94BarCodeBuffer[ ucNum + 4 ];
    g_ucBarCodeByteBuffer[ REAGENT_BAR_CODE_BYTE_NUM - 4 * i - 4 ] = ( uliValue >> 24 ) & 0xFF;
    g_ucBarCodeByteBuffer[ REAGENT_BAR_CODE_BYTE_NUM - 4 * i - 3 ] = ( uliValue >> 16 ) & 0xFF;
    g_ucBarCodeByteBuffer[ REAGENT_BAR_CODE_BYTE_NUM - 4 * i - 2 ] = ( uliValue >> 8 ) & 0xFF;
    g_ucBarCodeByteBuffer[ REAGENT_BAR_CODE_BYTE_NUM - 4 * i - 1 ] = ( uliValue ) & 0xFF;
    if (ucNum <= 5) {
      break;
    }
  }
  uliValue = g_uc94BarCodeBuffer[ 1 ];
  uliValue = uliValue * 94 + g_uc94BarCodeBuffer[ 2 ];
  uliValue = uliValue * 94 + g_uc94BarCodeBuffer[ 3 ];
  uliValue = uliValue * 94 + g_uc94BarCodeBuffer[ 4 ];
  g_ucBarCodeByteBuffer[ 0 ] = REAGENT_BAR_INFO_HEADER; //标志位
  g_ucBarCodeByteBuffer[ 1 ] = ( uliValue >> 16 ) & 0xFF;
  g_ucBarCodeByteBuffer[ 2 ] = ( uliValue >> 8 ) & 0xFF;
  g_ucBarCodeByteBuffer[ 3 ] = ( uliValue & 0xFF ) & 0xFF;
  //显示项目名称
  const ucTemp = [], res = {};
  g_ucBarCodeByteBuffer.forEach((value, key) => {
    if (value !== undefined && value !== null && key > 0 && key < 11) {
      ucTemp.push(value)
    }
  })
  ucTemp[ 10 ] = '\0'.charCodeAt(0);
  res[ 'name' ] = Buffer.from(ucTemp).toString('ascii')

  const char0 = '0'.charCodeAt(0)
  //显示批号
  ucTemp[ 0 ] = 2 + char0;
  ucTemp[ 1 ] = char0;
  uiValue = ( g_ucBarCodeByteBuffer[ 20 ] & 0xFE ) >> 1;//年
  ucTemp[ 2 ] = uiValue / 10 + char0;
  ucTemp[ 3 ] = uiValue % 10 + char0;
  uiValue = ( ( g_ucBarCodeByteBuffer[ 20 ] & 0x01 ) << 3 ) | ( ( g_ucBarCodeByteBuffer[ 21 ] & 0xE0 ) >> 5 );//月
  ucTemp[ 4 ] = uiValue / 10 + char0;
  ucTemp[ 5 ] = uiValue % 10 + char0;
  uiValue = ( ( g_ucBarCodeByteBuffer[ 21 ] & 0x1F ) << 5 ) | ( ( g_ucBarCodeByteBuffer[ 22 ] & 0xF8 ) >> 3 );//序列号
  ucTemp[ 6 ] = uiValue / 100 + char0;
  ucTemp[ 7 ] = ( uiValue % 100 ) / 10 + char0;
  ucTemp[ 8 ] = uiValue % 10 + char0;
  ucTemp[ 9 ] = '\0'.charCodeAt(0);
  res[ 'batch' ] = Buffer.from(ucTemp).toString('ascii')
  //显示有效期
  ucTemp[ 0 ] = 2 + char0;
  ucTemp[ 1 ] = char0;
  uiValue = ( g_ucBarCodeByteBuffer[ 18 ] & 0xFE ) >> 1;//年
  ucTemp[ 2 ] = uiValue / 10 + char0;
  ucTemp[ 3 ] = uiValue % 10 + char0;
  ucTemp[ 4 ] = '/'.charCodeAt(0);
  uiValue = ( ( g_ucBarCodeByteBuffer[ 18 ] & 0x01 ) << 3 ) | ( ( g_ucBarCodeByteBuffer[ 19 ] & 0xE0 ) >> 5 );//月
  ucTemp[ 5 ] = uiValue / 10 + char0;
  ucTemp[ 6 ] = uiValue % 10 + char0;
  ucTemp[ 7 ] = '/'.charCodeAt(0);
  uiValue = g_ucBarCodeByteBuffer[ 19 ] & 0x1F;//日
  ucTemp[ 8 ] = uiValue / 10 + char0;
  ucTemp[ 9 ] = uiValue % 10 + char0;
  ucTemp[ 10 ] = '\0'.charCodeAt(0);
  res[ 'expiration' ] = Buffer.from(ucTemp).toString('ascii')
  res.detailInfo = analysisReagentDetailInfo(g_ucBarCodeByteBuffer);
  return res;
}

/*******************************************************************************
 * 函数名称：AnalysisReagentDetailInfo
 * 功能描述：解析试剂条的详细信息
 * 输入参数：p 需要解析的字符串
 * 输出参数：无
 * 返回值  ：无
 * 修改日期            版本号            修改人            修改内容
 * -----------------------------------------------------------------------------
 * 2017/08/30          V1.0              周志强               创建
 *******************************************************************************/
function analysisReagentDetailInfo(p) {
  let i = 0, j = 0, k = 11, m = 168;
  const ucTemp = [];

  let ucValue1 = 0, ucValue2 = 0;

  const g_tTestInfo = {}, g_tRunProcessFile = {}, g_tStandCurve = {};
  //结果单位
  g_tTestInfo.ucUnit = ( ( p[ 22 ] & 0x07 ) << 1 ) | ( ( p[ 23 ] & 0x80 ) >> 7 );
  //稀释倍数
  g_tTestInfo.ucDilute = p[ 23 ] & 0x7F;

  //以下为流程参数
  g_tRunProcessFile.uiProcess1Timer = p[ k ++ ] * 10 * 10;  //ms
  //g_tRunProcessFile.uiProcess1Timer  = g_tRunProcessFile.uiProcess1Timer - 300;  //ms
  g_tRunProcessFile.uiProcess2Timer = p[ k ++ ] * 10 * 10;  //ms
  //g_tRunProcessFile.uiProcess2Timer  = g_tRunProcessFile.uiProcess2Timer + 300;  //ms
  if (g_tTestInfo.ucDilute > 1)
    g_tRunProcessFile.uiDilutionVol = p[ k ] * ( g_tTestInfo.ucDilute - 1 );   //稀释液的体积
  g_tRunProcessFile.uiSampleVol = p[ k ];
  g_tRunProcessFile.uiSample = p[ k ++ ];
  g_tRunProcessFile.uiDilutionSample = p[ k ++ ];
  g_tRunProcessFile.uiFistReagent = p[ k ++ ];
  g_tRunProcessFile.uiSecondReagent = p[ k ++ ];
  g_tRunProcessFile.uiThirdReagent = p[ k ++ ];
  g_tRunProcessFile.ucMixTimes = 3;
  k = k + 4;
  ucValue1 = p[ k ++ ];
  ucValue2 = p[ k ];
  g_tTestInfo.ucUnit = ( ( ucValue1 & 0x07 ) << 1 ) | ( ( ucValue2 & 0x80 ) >> 7 );
  g_tTestInfo.ucDilute = p[ k ++ ] & 0x7F;

  for ( i = 0; i < 16; i ++ ) {
    for ( j = 0; j < 4; j ++ ) {
      ucTemp[ j ] = p[ k ++ ];
    }
    if (!g_tStandCurve.hasOwnProperty("fCurveParam1")) {
      g_tStandCurve.fCurveParam1 = [];
    }
    g_tStandCurve.fCurveParam1[ i ] = HexToSingle(Buffer.from(ucTemp).toString('hex'))
  }
  for ( i = 0; i < 16; i ++ ) {
    for ( j = 0; j < 4; j ++ ) {
      ucTemp[ j ] = p[ k ++ ];
    }
    if (!g_tStandCurve.hasOwnProperty("fCurveParam2")) {
      g_tStandCurve.fCurveParam2 = [];
    }
    g_tStandCurve.fCurveParam2[ i ] = HexToSingle(Buffer.from(ucTemp).toString('hex'))
  }
  for ( j = 0; j < 4; j ++ ) {
    ucTemp[ j ] = p[ k ++ ];
  }
  g_tStandCurve.fLowLimit = HexToSingle(Buffer.from(ucTemp).toString('hex'))
  for ( j = 0; j < 4; j ++ ) {
    ucTemp[ j ] = p[ k ++ ];
  }
  g_tStandCurve.fHighLimit = HexToSingle(Buffer.from(ucTemp).toString('hex'))
  for ( j = 0; j < 4; j ++ ) {
    ucTemp[ j ] = p[ k ++ ];
  }
  g_tStandCurve.fLowReferencevalue = HexToSingle(Buffer.from(ucTemp).toString('hex'))
  for ( j = 0; j < 4; j ++ ) {
    ucTemp[ j ] = p[ k ++ ];
  }
  g_tStandCurve.fHighReferencevalue = HexToSingle(Buffer.from(ucTemp).toString('hex'))
//以下为全血流程参数
  g_tTestInfo.ucDilute_Wholeblood = p[ 173 ];
  if (g_tTestInfo.ucDilute_Wholeblood > 1)
    g_tRunProcessFile.uiDilutionVol_Wholeblood = p[ m ] * ( g_tTestInfo.ucDilute_Wholeblood - 1 );   //稀释液的体积
  g_tRunProcessFile.uiSampleVol_Wholeblood = p[ m ];
  g_tRunProcessFile.uiSample_Wholeblood = p[ m ++ ];
  g_tRunProcessFile.uiDilutionSample_Wholeblood = p[ m ++ ];
  g_tRunProcessFile.uiFistReagent_Wholeblood = p[ m ++ ];
  g_tRunProcessFile.uiSecondReagent_Wholeblood = p[ m ++ ];
  g_tRunProcessFile.uiThirdReagent_Wholeblood = p[ m ++ ];

  g_tTestInfo.ucResultPoint = p[ 174 ];
  if (g_tTestInfo.ucResultPoint > 154) {
    g_tTestInfo.ucResultPoint = g_tTestInfo.ucResultPoint - 155;
  } else {
    g_tTestInfo.ucResultPoint = 3;
  }
  return {testInfo: g_tTestInfo, runProcessFile: g_tRunProcessFile, standCurve: g_tStandCurve}
}

// 16进制转浮点数需要用到的函数
function FillString(t, c, n, b) {
  if (( t == "" ) || ( c.length != 1 ) || ( n <= t.length )) {
    return t;
  }
  var l = t.length;
  for ( var i = 0; i < n - l; i ++ ) {
    if (b == true) {
      t = c + t;
    } else {
      t += c;
    }
  }
  return t;
}

// 16进制转浮点数
function HexToSingle(t) {
  t = t.replace(/\s+/g, "");
  if (t == "") {
    return "";
  }
  if (t == "00000000") {
    return "0";
  }
  if (( t.length > 8 ) || ( isNaN(parseInt(t, 16)) )) {
    return "Error";
  }
  if (t.length < 8) {
    t = FillString(t, "0", 8, true);
  }
  t = parseInt(t, 16).toString(2);
  t = FillString(t, "0", 32, true);
  var s = t.substring(0, 1);
  var e = t.substring(1, 9);
  var m = t.substring(9);
  e = parseInt(e, 2) - 127;
  m = "1" + m;
  if (e >= 0) {
    m = m.substr(0, e + 1) + "." + m.substring(e + 1)
  } else {
    m = "0." + FillString(m, "0", m.length - e - 1, true)
  }
  if (m.indexOf(".") == - 1) {
    m = m + ".0";
  }
  var a = m.split(".");
  var mi = parseInt(a[ 0 ], 2);
  var mf = 0;
  for ( var i = 0; i < a[ 1 ].length; i ++ ) {
    mf += parseFloat(a[ 1 ].charAt(i)) * Math.pow(2, - ( i + 1 ));
  }
  m = parseInt(mi) + parseFloat(mf);
  if (s == 1) {
    m = 0 - m;
  }
  return m;
}

// 监听扫码
function scan(event, arg) {
  const port = new SerialPort(arg.path, {autoOpen: false, baudRate: Number(arg.baudRate), rtscts: false});

  // The open event is always emitted
  port.on('open', function () {
    console.log("open success")
    event.sender.send('async-io-reply', {code: 1, io: 1});
  })

  // The close event is always emitted
  port.on('close', function () {
    console.log("close success")
    event.sender.send('async-io-reply', {code: 1, io: 0});
  })

  // Switches the port into "flowing mode"
  port.on('data', function (data) {
    console.log('Data:', data)
    console.log(data.toString('ascii'))
    const info = lcdGetReagentDetailInfo(convert94ToHex(data))
    console.log(info)
    event.sender.send('async-reply', JSON.stringify(info));
    const g_tTestInfo = info.detailInfo.testInfo
    g_tTestInfo.ucSampleType = 1
    g_tTestInfo.uliFitValue = 70000
    const g_tStandCurve = info.detailInfo.standCurve
    const res = LcdCalculateConcentration(g_tTestInfo, g_tStandCurve)
    console.log(res)
    event.sender.send('async-reply', JSON.stringify(res));
  })

  // Read data that is available but keep the stream in "paused mode"
  // port.on('readable', function () {
  //   console.log('Data:', port.read())
  // })

  // Open errors will be emitted as an error event
  port.on('error', function (err) {
    console.log('Error: ', err.message)
  })

  // Pipe the data into another stream (like a parser or standard out)
  port.pipe(new Readline())

  // The open
  port.open(function (err) {
    if (err) {
      console.log('Error opening port: ', err.message)
      event.sender.send('async-io-reply', {code: 0, io: 1, err: err});
    }
  });
}

module.exports = scan
