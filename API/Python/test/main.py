import sys
import time
import queue
import threading
from clientUI import Ui_testStupidP2PPythonAPI
from PyQt5.QtWidgets import QApplication, QWidget, QMessageBox
from PyQt5 import QtCore
sys.path.append("..") 
from stupidP2P import stupidP2PClient

class clientRecvThread(QtCore.QThread):

    printStrTrigger = QtCore.pyqtSignal(str)

    def __init__(self, clientRecvDataFunc):
        super(clientRecvThread, self).__init__()
        self._stop_event = threading.Event()
        self.clientRecvDataFunc = clientRecvDataFunc

    def stop(self):
        self._stop_event.set()

    def stopped(self):
        return self._stop_event.is_set()

    def run(self):
        while not self.stopped():
            time.sleep(0.1)
            dataList = self.clientRecvDataFunc()
            if len(dataList) > 0:
                for data in dataList:
                    self.printStrTrigger.emit("接收：" + str(bytes(data), encoding="UTF-8"))



class client(QWidget, Ui_testStupidP2PPythonAPI):
    def __init__(self, parent=None):
        super(client, self).__init__()
        self.setupUi(self)
        self.buttonAction()

        self.startFlag = 0

    def buttonAction(self):
        self.pushButton_connection.clicked.connect(self.buttonConnectionClicked)
        self.pushButton_register.clicked.connect(self.buttonRegisterClicked)
        self.pushButton_unregister.clicked.connect(self.buttonUnregisterCliecked)
        self.pushButton_check_alive.clicked.connect(self.buttoCheckAliveCliecked)
        self.pushButton_subscribe.clicked.connect(self.buttonSubscribeCliecked)
        self.pushButton_unsubscribe.clicked.connect(self.buttonUnsubscribeCliecked)
        self.pushButton_send_data.clicked.connect(self.buttonSendDataCliecked)
        

    def buttonConnectionClicked(self):
        if self.startFlag == 0:
            try:
                self.serverIp = self.lineEdit_server_ip.text()
                self.serverPort = int(self.lineEdit_server_port.text())
            except:
                self.printTips(self.__ERROR_TIPS, "服务器ip或port输入无效")
                return

            self.client = stupidP2PClient(self.serverIp, self.serverPort)
            ret = self.client.initClient()
            if ret == True:
                self.printTips(self.__SUCCESS_TIPS, "连接成功")
                self.startFlag = 1
            else:
                self.printTips(self.__ERROR_TIPS, "连接失败")
                return

            self.recvThread = clientRecvThread(self.client.recvData)
            self.recvThread.start()
            self.recvThread.printStrTrigger.connect(self.printTextBrowser)

    def buttonRegisterClicked(self):
        if self.startFlag == 0:
            self.printTips(self.__WARN_TIPS, "请先连接Server")
            return

        self.deviceId = self.lineEdit_my_id.text()
        if self.deviceId == "":
            self.printTips(self.__ERROR_TIPS, "输入本机Id无效")
            return
        
        ret = self.client.registerDevice(self.deviceId)
        self.__checkCommandReturn(ret, "注册")

    def buttonUnregisterCliecked(self):
        if self.startFlag == 0:
            self.printTips(self.__WARN_TIPS, "请先连接Server")
            return

        if self.deviceId == "":
            self.printTips(self.__WARN_TIPS, "还未注册本机Id")

        ret = self.client.unregisterDevice()
        self.__checkCommandReturn(ret, "取消注册")

    def buttoCheckAliveCliecked(self):
        if self.startFlag == 0:
            self.printTips(self.__WARN_TIPS, "请先连接Server")
            return

        subscribeId = self.lineEdit_subscribe_id.text()
        if subscribeId == "":
            self.printTips(self.__ERROR_TIPS, "输入订阅Id无效")
            return

        ret = self.client.checkDeviceAlive(subscribeId)
        self.__checkCommandReturn(ret, "id是否在线")

        
    def buttonSubscribeCliecked(self):
        if self.startFlag == 0:
            self.printTips(self.__WARN_TIPS, "请先连接Server")
            return

        self.subscribeId = self.lineEdit_subscribe_id.text()
        if self.subscribeId == "":
            self.printTips(self.__ERROR_TIPS, "输入订阅Id无效")
            return

        ret = self.client.subscribeDevice(self.subscribeId)
        self.__checkCommandReturn(ret, "订阅")

    def buttonUnsubscribeCliecked(self):
        if self.startFlag == 0:
            self.printTips(self.__WARN_TIPS, "请先连接Server")
            return

        if self.subscribeId == "":
            self.printTips(self.__WARN_TIPS, "还未订阅Id")

        ret = self.client.unscribeDevice(self.subscribeId)
        self.__checkCommandReturn(ret, "取消订阅")

    def buttonSendDataCliecked(self):
        if self.startFlag == 0:
            self.printTips(self.__WARN_TIPS, "请先连接Server")
            return

        sendStr = self.textEdit_input.toPlainText()
        if sendStr == "":
            return

        ret = self.client.sendData(sendStr)
        self.__checkCommandReturn(ret, "发送")

        self.printTextBrowser("发送： " + sendStr)

    def __checkCommandReturn(self, ret, headerStr):
        if ret == 0xff:
            self.printTips(self.__WARN_TIPS, "未收到响应")
        elif ret == 0x0:
            self.printTips(self.__ERROR_TIPS, "失败: " + headerStr)
        elif ret == 0x1:
            self.printTips(self.__SUCCESS_TIPS, "成功:" + headerStr)


    # print to tips
    __ERROR_TIPS = 0
    __SUCCESS_TIPS = 1
    __WARN_TIPS = 2
    def printTips(self, level, printStr):
        if level == self.__SUCCESS_TIPS:
            self.label_tips.setStyleSheet("color:green")
        elif level == self.__ERROR_TIPS:
            self.label_tips.setStyleSheet("color:red")
        elif level == self.__WARN_TIPS:
            self.label_tips.setStyleSheet("color:#deb887")
        self.label_tips.setText("·" + printStr)

    def printTextBrowser(self, printStr):
        self.textBrowser_show_receive.append(printStr)

    # close app exec
    def closeEvent(self, event):
        reply = QMessageBox.question(self, '提示',"系统将退出，是否确认?", QMessageBox.Yes |QMessageBox.No, QMessageBox.No)
        if reply == QMessageBox.Yes:
            if self.startFlag == 1:
                self.client.deinitClient()
                self.recvThread.stop()
                while not self.recvThread.stopped():
                    time.sleep(0.1)

                self.startFlag = 0
            
            event.accept()
        else:
            event.ignore()
                



if __name__ == '__main__':
    app = QApplication(sys.argv)
    window = client()
    window.show()
    sys.exit(app.exec())
