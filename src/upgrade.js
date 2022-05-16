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
 * | Date: 2022/1/6 10:35
 * +----------------------------------------------------------------------
 */

const fs = require('fs');
const SerialPort = require("serialport");
const Readline = SerialPort.parsers.Readline;

let bufferList;

// 系统接入BootLoader(进入串口升级模式)
function bootLoader(serialport, filePath, event, args) {
  // 读取hex文件
  readBinary(filePath, (err, buf) => {
    if (err) {
      throw err;
    }
    // Buffer按255长度分割
    bufferList = bufferSlice(buf, 255);
    console.log('bufferList', bufferList.length)
    event.sender.send("async-reply", '读取：' + filePath + ' 长度：' + bufferList.length);
    // 计数器
    let index = 0;
    // 监听回复
    serialport.on('data', function (data) {
      const hex = data.toString("hex")
      console.log('Data:', hex)
      if (hex === '79') {
        switch ( index ) {
          case 0:
            event.sender.send("async-reply", 'STM32 波特率匹配成功');
            portWrite(serialport, '44bb')
            break;
          case 1:
            event.sender.send("async-reply", 'Flash 准备全部擦除');
            portWrite(serialport, 'ffff00')
            break;
          case 2:
            event.sender.send("async-reply", 'Flash 全部擦除成功');
            serialport.close(function (err) {
              if (err) {
                console.log('Error close port: ', err.message)
              }
              upgrade(event, args, filePath);
            });
            break;
        }
        index ++;
      }
    })
    serialport.set({dtr: true, rts: true}, errorCallback)
    serialport.set({dtr: false}, errorCallback)
    setTimeout(() => {
      serialport.set({dtr: true}, errorCallback)
      event.sender.send("async-reply", '进入 BootLoader 串口升级模式');
      portWrite(serialport, '7f', 100)
    }, 100);
  });
}

// STM32F4 升级
function upgrade(event, args) {

  const port = new SerialPort(args.path, {autoOpen: false, baudRate: Number(args.baudRate), rtscts: false, parity: 'even'});

  let step = 0;
  let index = 0;
  let timestamp = new Date().getTime();

  // The open event is always emitted
  port.on('open', function () {
    console.log("open success")
  })

  // The close event is always emitted
  port.on('close', function () {
    console.log("close success")
    event.sender.send('async-io-reply', {code: 1, io: 0});
  })

  // Switches the port into "flowing mode"
  port.on('data', function (data) {
    const hex = data.toString("hex")
    console.log('Data:', hex)
    event.sender.send("async-reply", hex);
    if (hex === '79') {
      switch ( step ) {
        case 0:
          const useTime = new Date().getTime() - timestamp;
          event.sender.send("async-reply", `bufferList[${index - 1}] use time ${useTime}ms`);
          event.sender.send("async-reply", `bufferList[${index}] start WM`);
          timestamp = new Date().getTime();
          startWM(port);
          step ++;
          break;
        case 1:
          event.sender.send("async-reply", `bufferList[${index}] write memory start address`);
          writeMemoryStartAddress(port, index);
          step ++;
          break;
        case 2:
          event.sender.send("async-reply", `bufferList[${index}] write memory byte`);
          writeMemory(port, index);
          step = 0;
          index ++;
          break;
      }
      if (index > bufferList.length) {
        event.sender.send("async-reply", `烧录完成，进入Flash启动(正常启动模式)`);
        bootFlash(port)
      }
    }
  })

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
    }
    event.sender.send('async-io-reply', {code: 1, io: 1});
    startWM(port);
    step ++;
  });
  return port;
}

// Write Memory 写入准备
function startWM(port) {
  portWrite(port, '31ce')
}

// Write Memory 写入内存起始地址
function writeMemoryStartAddress(port, index) {
  const address_str = dec2hex(parseInt('0x8000000') + ( parseInt('0xff') * index ), 8);
  const start_address = Buffer.from(address_str, 'hex');
  const xor = bufferXOR(start_address)
  const buf = Buffer.concat([start_address, xor])
  portWrite(port, buf);
}

// Write Memory
function writeMemory(port, index) {
  if (bufferList.hasOwnProperty(index)) {
    const buf = bufferList[ index ]
    const xorBuf = Buffer.concat([Buffer.from([buf.length - 1]), buf]);
    console.log(xorBuf.toString("hex"))
    const xor = bufferXOR(xorBuf)
    console.log(xor)
    const buffer = Buffer.concat([xorBuf, xor])
    console.log(index, buffer)
    port.write(buffer, writeCallback)
    // portWrite(port, buffer.toString('hex'), 0, 30);
  }
}

// Flash启动(正常启动模式)
function bootFlash(port) {
  port.set({dtr: false}, errorCallback)
  port.set({rts: false}, errorCallback)
  setTimeout(() => {
    port.set({dtr: true}, errorCallback)
    setTimeout(() => port.close(function (err) {
      if (err) {
        console.log('Error close port: ', err.message)
      }
    }), 100)
  }, 100)
}

// 10进制转16进制补0
function dec2hex(dec, len) {//10进制转16进制补0
  let hex = "";
  while ( dec ) {
    const last = dec & 15;
    hex = String.fromCharCode(( ( last > 9 ) ? 55 : 48 ) + last) + hex;
    dec >>= 4;
  }
  if (len) {
    while ( hex.length < len ) hex = '0' + hex;
  }
  return hex;
}

// 16进制转10进制
function hex2dec(hex) {
  let len = hex.length, a = new Array(len), code;
  for ( let i = 0; i < len; i ++ ) {
    code = hex.charCodeAt(i);
    if (48 <= code && code < 58) {
      code -= 48;
    } else {
      code = ( code & 0xdf ) - 65 + 10;
    }
    a[ i ] = code;
  }
  return a.reduce(function (acc, c) {
    acc = 16 * acc + c;
    return acc;
  }, 0);
}

// 逐个字节延迟写入
function portWrite(port, hex, int = 0, ms = 20) {
  const buffer = Buffer.from(hex, 'hex')
  for ( let i = 0; i < buffer.length; i ++ ) {
    const buf = buffer.slice(i, i + 1);
    setTimeout(() => {
    console.log(i, buf)
    port.write(buf, writeCallback)
    }, int + ( ms * i ));
  }
}

// 异或运算
function bufferXOR(buffer) {
  let xor = null;
  for ( let i = 0; i < buffer.length; i ++ ) {
    const buf = buffer.slice(i, i + 1).toString("hex");
    if (xor === null) {
      xor = hex2dec(buf)
    } else {
      xor ^= hex2dec(buf)
    }
  }
  return Buffer.from([xor]);
}

// Buffer分割
function bufferSlice(buffer, slice) {
  const chunks = [];
  for ( let i = 0; i < buffer.length; i = i + slice ) {
    const end = ( i + slice ) > buffer.length ? buffer.length : i + slice;
    const buf = buffer.slice(i, end);
    chunks.push(buf);
  }
  return chunks;
}

// 读取hex文件
function readBinary(filename, callback) {
  const rstream = fs.createReadStream(filename);
  const chunks = [];
  let size = 0;
  rstream.on('readable', function () {
    const chunk = rstream.read();
    if (chunk != null) {
      console.log("read", chunk)
      chunks.push(chunk);
      size += chunk.length;
    }
  });
  rstream.on('end', function () {
    callback(null, Buffer.concat(chunks, size));
  });
  rstream.on('error', function (err) {
    callback(err, null);
  });
}

function errorCallback(err) {
  if (err) {
    console.error(err.message)
  }
}

function writeCallback(err) {
  if (err) {
    console.log('Error on write: ', err.message)
  }
  console.log('message written')
}

module.exports = bootLoader
