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
 * | Date: 2022/1/8 13:40
 * +----------------------------------------------------------------------
 */

const iconv = require('iconv-lite');

function trim(str) {
  return str.replace(/\s*/g, '')
}

function buffer(hex) {
  return Buffer.from(trim(hex), 'hex')
}

function serialportPrint(port, event, arg) {
  const list = []
  list.push(buffer('1b 40 00 0d 0a 1b 38 00 1B 63 01 1c 26'))
  if (arg !== null && arg !== undefined && arg !== '') {
    const content = iconv.encode(arg, 'GBK')
    console.log(content.toString('hex'), content)
    list.push(content)
  }
  list.push(buffer('1c 2e 0d 0a 0d 0a 0d 0a 0d 0a 0d 0a 0d 0a 0d 0a 0d 0a'))
  port.write(Buffer.concat(list))
}

function templatePrint(port, event, args) {
  const list = []
  list.push(buffer('1b 40 00 0d 0a 1b 38 00 1B 63 00 1c 26'))
  template(args).forEach(v => list.push(v))
  list.push(buffer('1c 2e 0d 0a 0d 0a 0d 0a 0d 0a 0d 0a 0d 0a 0d 0a 0d 0a'))
  port.write(Buffer.concat(list))
}

function template(args) {
  const defaultSeting = {
    hospitalName: "XX",
    name: "张三",
    sex: '男',
    // 样本号
    sampleNumber: 'xxxxxxxxxxxxxxxx',
    // 年龄
    age: 18,
    // 科室
    department: '神经内科',
    // 病历号
    medicalRecordNumber: 'xxxxxxxxxxxxxxxx',
    // 床号
    bedNumber: 'xxxxxxxxxxxxxxxx',
    // 样本类型
    sampleType: '血清',
    // 检测方法
    testMethod: '化学发光',
    // 检测列表
    items: [{
      // 项目
      project: "CK-MB",
      // 结果
      result: "40.000",
      // 参考范围
      referenceRange: "2.000~10.000",
      // 单位
      company: 'ng/mL',
      // 标志
      sign: '↑',
    }],
    // 送检时间
    inspectionTime: '2020/12/14 18:00',
    // 检测时间
    detectionTime: '2020/12/14 18:00',
    // 报告时间
    reportTime: '2020/12/14 18:00',
    // 打印时间
    printTime: '2020/12/14 18:00',
    // 送检医生
    sendingDoctor: '黄鹤楼',
    // 操作人
    operator: '王小翠',
    // 仪器编号
    instrumentNo: 'xxxxxxxxxxxxxxxx',
    // 备注
    remarks: '本报告仅对该送检样本负责！'
  }
  args = {...defaultSeting, ...args}
  let row = [];
  // 行居中对齐
  const center = buffer('1B 61 01')
  // 行左对齐
  const left = buffer('1B 61 48')
  // 行右对齐
  const right = buffer('1B 61 50')
  // 换行
  const br = buffer('0d 0a')
  row.push(Buffer.concat([
    left,
    iconv.encode(`${args.hospitalName}医院检验报告`, 'GBK'),
    br, br
  ]));
  row.push(Buffer.concat([
    left,
    iconv.encode(`姓名 ${args.name} 性别 ${args.sex} 样本号 ${args.sampleNumber}`, 'GBK'),
    br
  ]));
  row.push(Buffer.concat([
    left,
    iconv.encode(`年龄 ${args.age} 科室 ${args.department} 病历号 ${args.medicalRecordNumber}`, 'GBK'),
    br
  ]));
  row.push(Buffer.concat([
    left,
    iconv.encode(`床号 ${args.bedNumber} 样本类型 ${args.sampleType} 检测方法 ${args.testMethod}`, 'GBK'),
    br
  ]));
  row.push(Buffer.concat([
    left,
    iconv.encode(`项目     结果     参考范围       单位     标志`, 'GBK'),
    br
  ]));
  args.items.forEach(v => {
    row.push(Buffer.concat([
      left,
      iconv.encode(`${v.project}   ${v.result}   ${v.referenceRange}    ${v.company}     ${v.sign}`, 'GBK'),
      br
    ]));
  })
  row.push(Buffer.concat([
    left,
    iconv.encode(`送检时间   ${args.inspectionTime}`, 'GBK'),
    br
  ]));
  row.push(Buffer.concat([
    left,
    iconv.encode(`检测时间   ${args.detectionTime}`, 'GBK'),
    br
  ]));
  row.push(Buffer.concat([
    left,
    iconv.encode(`报告时间   ${args.reportTime}`, 'GBK'),
    br
  ]));
  row.push(Buffer.concat([
    left,
    iconv.encode(`打印时间   ${args.printTime}`, 'GBK'),
    br
  ]));
  row.push(Buffer.concat([
    left,
    iconv.encode(`送检医生   ${args.sendingDoctor}`, 'GBK'),
    br
  ]));
  row.push(Buffer.concat([
    left,
    iconv.encode(`操作人     ${args.operator}`, 'GBK'),
    br
  ]));
  row.push(Buffer.concat([
    left,
    iconv.encode(`仪器编号   ${args.instrumentNo}`, 'GBK'),
    br
  ]));
  row.push(Buffer.concat([
    left,
    iconv.encode(`备注       ${args.remarks}`, 'GBK'),
    br
  ]));
  return row;
}

exports.serialportPrint = serialportPrint
exports.templatePrint = templatePrint
