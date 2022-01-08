const {ipcRenderer} = require('electron');

window.addEventListener('DOMContentLoaded', () => {
  ipcRenderer.send('async-port-message', 'ping') //给主进程发送消息“ping”
  const port_number = document.getElementById("port_number");
  const baud_rate = document.getElementById("baud_rate");
  ipcRenderer.on('async-port-reply', function (event, arg) { // 接收到Main进程返回的消息
    console.log(arg);
    let html = '';
    arg.forEach(port => {
      const selected = port.path === "COM2" ? "selected" : "";
      html += `<option value="${port.path}" ${selected}>${port.path} ${port.pnpId}</option>`;
    });
    port_number.innerHTML = html;
  });

  // 发送指令
  const send_btn = document.getElementById("send_btn")
  send_btn.onclick = function () {
    const data = document.getElementById("cmd").value;
    ipcRenderer.send('async-message', data)
  }
  ipcRenderer.on('async-message-reply', function (event, arg) { // 接收到Main进程返回的消息
    console.log(arg)
    if (!arg.code) {
      alert(arg.err.message)
    }
  });

  // 打开/关闭串口
  const io_btn = document.getElementById("io")
  io_btn.onclick = function () {
    const args = {io: 0, path: port_number.value, baudRate: baud_rate.value};
    args.io = io_btn.innerText === "打开串口";
    console.log(args)
    ipcRenderer.send('async-io-message', args);
  }
  const io_state = document.getElementById("io_state")
  ipcRenderer.on('async-io-reply', function (event, arg) { // 接收到Main进程返回的消息
    if (arg.code) {
      if (arg.io) {
        io_state.classList.remove('close')
        io_state.classList.add('open');
        return io_btn.innerText = "关闭串口";
      } else {
        io_state.classList.remove('open')
        io_state.classList.add('close');
        return io_btn.innerText = "打开串口";
      }
    }
    alert(arg.err.message)
  });

  // 监听回复
  const pre = document.getElementById("pre")
  ipcRenderer.on('async-reply', function (event, arg) { // 接收到Main进程返回的消息
    const div = document.createElement("p");   //创建div节点
    div.innerText = arg;
    pre.appendChild(div);
    pre.scrollTo(0, pre.scrollHeight);
  });


  // 一键升级
  const upgrade_stm32_btn = document.getElementById("upgrade_stm32")
  upgrade_stm32_btn.onclick = function () {
    const args = {path: port_number.value, baudRate: baud_rate.value};
    ipcRenderer.send('async-upgrade-message', args)
  }

  // 监听扫描
  const scan = document.getElementById("scan")
  scan.onclick = function () {
    const args = {path: port_number.value, baudRate: baud_rate.value};
    ipcRenderer.send('async-scan-message', args)
  }

  // 打印
  const print = document.getElementById("print")
  print.onclick = function () {
    const data = document.getElementById("cmd").value;
    ipcRenderer.send('async-print-message', data)
  }

})

