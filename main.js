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
 * | Date: 2022/1/4 16:11
 * +----------------------------------------------------------------------
 */
const path = require("path");
const {app, BrowserWindow, ipcMain, dialog} = require('electron');
const upgrade = require('./src/upgrade');
const scan = require('./src/scan');
const print = require('./src/print');

const SerialPort = require('serialport');
const Readline = SerialPort.parsers.Readline;
SerialPort.parsers = {
  ByteLength: require('@serialport/parser-byte-length'),
  CCTalk: require('@serialport/parser-cctalk'),
  Delimiter: require('@serialport/parser-delimiter'),
  Readline: require('@serialport/parser-readline'),
  Ready: require('@serialport/parser-ready'),
  Regex: require('@serialport/parser-regex'),
}

// SerialPort
let port;
// 保持对window对象的全局引用，如果不这么做的话，当JavaScript对象被
// 垃圾回收的时候，window对象将会自动的关闭
let win;

function createSerialPort(event, arg) {
  port = new SerialPort(arg.path, {autoOpen: false, baudRate: Number(arg.baudRate), rtscts: false});

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
    event.sender.send('async-reply', data.toString("hex"));
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
      return event.sender.send('async-io-reply', {code: 0, io: 1, err: err});
    }
  });
}

ipcMain.on('async-port-message', (event, arg) => {
  SerialPort.list().then(ports => {
    event.sender.send('async-port-reply', ports);
    ports.forEach(function (port) {
      console.log(port)
    })
  }, err => console.error(err));
});

ipcMain.on('async-io-message', (event, arg) => {
  console.log(arg)
  if (arg.io) {
    createSerialPort(event, arg)
  } else {
    port.close(function (err) {
      if (err) {
        console.log('Error close port: ', err.message)
        return event.sender.send('async-io-reply', {code: 0, io: 0, err: err});
      }
    })
  }
});

ipcMain.on('async-message', (event, arg) => { // arg为接受到的消息
                                              // Because there's no callback to write, write errors will be emitted on the port:
  console.log(arg)
  const cmd = Buffer.from(arg.replace(/\s*/g, ""), "hex");
  console.log(cmd);
  port.write(cmd, function (err) {
    if (err) {
      console.log('Error on write: ', err.message)
      return event.sender.send('async-message-reply', {code: 0, err: err});
    }
    console.log('message written')
    event.sender.send('async-message-reply', {code: 1});
  })
})

// 一键升级
ipcMain.on('async-upgrade-message', (event, arg) => { // arg为接受到的消息
  // Because there's no callback to write, write errors will be emitted on the port:
  if (port === undefined) {
    return dialog.showMessageBox(win, {
      title: "提示",
      type: 'info',
      message: '请先打开串口，再尝试一键升级。'
    });
  }
  if (Number(arg.baudRate) > 57600) {
    return dialog.showMessageBox(win, {
      title: "提示",
      type: 'info',
      message: '升级程序时串口波特率设置为57600bps以下。'
    });
  }
  const u = dialog.showOpenDialog(win, {
    title: "请选择文件",
    properties: ['openFile'],
    filters: [{name: 'HEX', extensions: ['hex']}]
  }).then(result => {
    upgrade(port, result.filePaths[ 0 ], event, arg)
  }).catch(err => {
    console.log(err)
    event.sender.send('async-upgrade-reply', {code: 0, err: err});
  });
  console.log(u);
});

// 监听扫描
ipcMain.on('async-scan-message', (event, arg) => { // arg为接受到的消息
                                                   // Because there's no callback to write, write errors will be emitted on the port:
  console.log(arg)
  if (Number(arg.baudRate) !== 9600) {
    return dialog.showMessageBox(win, {
      title: "提示",
      type: 'info',
      message: '扫描枪串口波特率必须设置为9600bps。'
    });
  }
  scan(event, arg)
});

// 监听打印
ipcMain.on('async-print-message', (event, arg) => { // arg为接受到的消息
  console.log(arg)
  if (port === undefined) {
    return dialog.showMessageBox(win, {
      title: "提示",
      type: 'info',
      message: '请先打开串口，再尝试驱动打印。'
    });
  }
  print(port, event, arg)
});

function createWindow() {
  //创建浏览器窗口
  win = new BrowserWindow({
    width: 600, height: 400,
    x: ( 1920 + ( ( 1080 - 600 ) / 2 ) ),
    y: 200,
    webPreferences: {
      nodeIntegration: true, contextIsolation: false, preload: path.join(__dirname, 'preload.js')
    }
  });
  // console.log(process)
  //加载index.html文件
  win.loadFile('main.html');
  // win.loadFile('dist/index.html');
  //打开开发者工具
  // win.webContents.openDevTools();
  // win.maximize()
  // win.show()
  //当window被关闭，这个事件被触发
  win.on('closed', () => {
    //取消引用的window对象
    win = null;
  });
}

// Electron 会在初始化后并准备
// 创建浏览器窗口时，调用这个函数。
// 部分 API 在 ready 事件触发后才能使用。
app.on('ready', createWindow);

// 当全部窗口关闭时退出。
app.on('window-all-closed', () => {
  // 在 macOS 上，除非用户用 Cmd + Q 确定地退出，
  // 否则绝大部分应用及其菜单栏会保持激活。
  if (process.platform !== 'darwin') {
    app.quit()
  }
})

app.on('activate', () => {
  // 在macOS上，当单击dock图标并且没有其他窗口打开时，
  // 通常在应用程序中重新创建一个窗口。
  if (win === null) {
    createWindow()
  }
})

// 在这个文件中，你可以续写应用剩下主进程代码。
// 也可以拆分成几个文件，然后用 require 导入。
