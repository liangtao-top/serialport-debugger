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
  list.push(buffer('1b 40 00 0d 0a 1b 38 00 1B 63 00 1c 26'))
  if (arg !== null && arg !== undefined && arg !== '') {
    const content = iconv.encode(arg, 'GBK')
    console.log(content.toString('hex'), content)
    list.push(content)
  }
  list.push(buffer('1c 2e 0d 0a 0d 0a 0d 0a 0d 0a 0d 0a 0d 0a 0d 0a 0d 0a'))
  port.write(Buffer.concat(list))
}

module.exports = serialportPrint
