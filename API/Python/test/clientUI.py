# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file '.\testStupidP2PPythonAPI.ui'
#
# Created by: PyQt5 UI code generator 5.15.4
#
# WARNING: Any manual changes made to this file will be lost when pyuic5 is
# run again.  Do not edit this file unless you know what you are doing.


from PyQt5 import QtCore, QtGui, QtWidgets


class Ui_testStupidP2PPythonAPI(object):
    def setupUi(self, testStupidP2PPythonAPI):
        testStupidP2PPythonAPI.setObjectName("testStupidP2PPythonAPI")
        testStupidP2PPythonAPI.resize(1097, 935)
        self.verticalLayout_2 = QtWidgets.QVBoxLayout(testStupidP2PPythonAPI)
        self.verticalLayout_2.setObjectName("verticalLayout_2")
        self.horizontalLayout_5 = QtWidgets.QHBoxLayout()
        self.horizontalLayout_5.setObjectName("horizontalLayout_5")
        self.horizontalLayout = QtWidgets.QHBoxLayout()
        self.horizontalLayout.setObjectName("horizontalLayout")
        self.label_server_ip = QtWidgets.QLabel(testStupidP2PPythonAPI)
        self.label_server_ip.setStyleSheet("font: 75 14pt \"Arial\";")
        self.label_server_ip.setObjectName("label_server_ip")
        self.horizontalLayout.addWidget(self.label_server_ip)
        self.lineEdit_server_ip = QtWidgets.QLineEdit(testStupidP2PPythonAPI)
        self.lineEdit_server_ip.setStyleSheet("font: 75 14pt \"Arial\";")
        self.lineEdit_server_ip.setObjectName("lineEdit_server_ip")
        self.horizontalLayout.addWidget(self.lineEdit_server_ip)
        self.horizontalLayout_5.addLayout(self.horizontalLayout)
        self.horizontalLayout_2 = QtWidgets.QHBoxLayout()
        self.horizontalLayout_2.setObjectName("horizontalLayout_2")
        self.label_server_port = QtWidgets.QLabel(testStupidP2PPythonAPI)
        self.label_server_port.setStyleSheet("font: 75 14pt \"Arial\";")
        self.label_server_port.setObjectName("label_server_port")
        self.horizontalLayout_2.addWidget(self.label_server_port)
        self.lineEdit_server_port = QtWidgets.QLineEdit(testStupidP2PPythonAPI)
        self.lineEdit_server_port.setStyleSheet("font: 75 14pt \"Arial\";")
        self.lineEdit_server_port.setObjectName("lineEdit_server_port")
        self.horizontalLayout_2.addWidget(self.lineEdit_server_port)
        self.horizontalLayout_5.addLayout(self.horizontalLayout_2)
        self.pushButton_connection = QtWidgets.QPushButton(testStupidP2PPythonAPI)
        self.pushButton_connection.setStyleSheet("font: 75 14pt \"Arial\";")
        self.pushButton_connection.setObjectName("pushButton_connection")
        self.horizontalLayout_5.addWidget(self.pushButton_connection)
        self.verticalLayout_2.addLayout(self.horizontalLayout_5)
        self.horizontalLayout_6 = QtWidgets.QHBoxLayout()
        self.horizontalLayout_6.setObjectName("horizontalLayout_6")
        self.horizontalLayout_3 = QtWidgets.QHBoxLayout()
        self.horizontalLayout_3.setObjectName("horizontalLayout_3")
        self.label_my_id = QtWidgets.QLabel(testStupidP2PPythonAPI)
        self.label_my_id.setStyleSheet("font: 75 14pt \"Arial\";")
        self.label_my_id.setObjectName("label_my_id")
        self.horizontalLayout_3.addWidget(self.label_my_id)
        self.lineEdit_my_id = QtWidgets.QLineEdit(testStupidP2PPythonAPI)
        self.lineEdit_my_id.setStyleSheet("font: 75 14pt \"Arial\";")
        self.lineEdit_my_id.setObjectName("lineEdit_my_id")
        self.horizontalLayout_3.addWidget(self.lineEdit_my_id)
        self.horizontalLayout_6.addLayout(self.horizontalLayout_3)
        self.pushButton_register = QtWidgets.QPushButton(testStupidP2PPythonAPI)
        self.pushButton_register.setStyleSheet("font: 75 14pt \"Arial\";")
        self.pushButton_register.setObjectName("pushButton_register")
        self.horizontalLayout_6.addWidget(self.pushButton_register)
        self.pushButton_unregister = QtWidgets.QPushButton(testStupidP2PPythonAPI)
        self.pushButton_unregister.setStyleSheet("font: 75 14pt \"Arial\";")
        self.pushButton_unregister.setObjectName("pushButton_unregister")
        self.horizontalLayout_6.addWidget(self.pushButton_unregister)
        self.verticalLayout_2.addLayout(self.horizontalLayout_6)
        self.horizontalLayout_4 = QtWidgets.QHBoxLayout()
        self.horizontalLayout_4.setObjectName("horizontalLayout_4")
        self.label_subscribe_id = QtWidgets.QLabel(testStupidP2PPythonAPI)
        self.label_subscribe_id.setStyleSheet("font: 75 14pt \"Arial\";")
        self.label_subscribe_id.setObjectName("label_subscribe_id")
        self.horizontalLayout_4.addWidget(self.label_subscribe_id)
        self.lineEdit_subscribe_id = QtWidgets.QLineEdit(testStupidP2PPythonAPI)
        self.lineEdit_subscribe_id.setStyleSheet("font: 75 14pt \"Arial\";")
        self.lineEdit_subscribe_id.setObjectName("lineEdit_subscribe_id")
        self.horizontalLayout_4.addWidget(self.lineEdit_subscribe_id)
        self.pushButton_check_alive = QtWidgets.QPushButton(testStupidP2PPythonAPI)
        self.pushButton_check_alive.setStyleSheet("font: 75 14pt \"Arial\";")
        self.pushButton_check_alive.setObjectName("pushButton_check_alive")
        self.horizontalLayout_4.addWidget(self.pushButton_check_alive)
        self.pushButton_subscribe = QtWidgets.QPushButton(testStupidP2PPythonAPI)
        self.pushButton_subscribe.setStyleSheet("font: 75 14pt \"Arial\";")
        self.pushButton_subscribe.setObjectName("pushButton_subscribe")
        self.horizontalLayout_4.addWidget(self.pushButton_subscribe)
        self.pushButton_unsubscribe = QtWidgets.QPushButton(testStupidP2PPythonAPI)
        self.pushButton_unsubscribe.setStyleSheet("font: 75 14pt \"Arial\";")
        self.pushButton_unsubscribe.setObjectName("pushButton_unsubscribe")
        self.horizontalLayout_4.addWidget(self.pushButton_unsubscribe)
        self.verticalLayout_2.addLayout(self.horizontalLayout_4)
        self.verticalLayout = QtWidgets.QVBoxLayout()
        self.verticalLayout.setObjectName("verticalLayout")
        self.textBrowser_show_receive = QtWidgets.QTextBrowser(testStupidP2PPythonAPI)
        self.textBrowser_show_receive.setStyleSheet("font: 75 14pt \"Arial\";")
        self.textBrowser_show_receive.setObjectName("textBrowser_show_receive")
        self.verticalLayout.addWidget(self.textBrowser_show_receive)
        self.horizontalLayout_7 = QtWidgets.QHBoxLayout()
        self.horizontalLayout_7.setObjectName("horizontalLayout_7")
        self.textEdit_input = QtWidgets.QTextEdit(testStupidP2PPythonAPI)
        sizePolicy = QtWidgets.QSizePolicy(QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Fixed)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.textEdit_input.sizePolicy().hasHeightForWidth())
        self.textEdit_input.setSizePolicy(sizePolicy)
        self.textEdit_input.setMaximumSize(QtCore.QSize(16777215, 100))
        self.textEdit_input.setStyleSheet("font: 75 14pt \"Arial\";")
        self.textEdit_input.setObjectName("textEdit_input")
        self.horizontalLayout_7.addWidget(self.textEdit_input)
        self.pushButton_send_data = QtWidgets.QPushButton(testStupidP2PPythonAPI)
        sizePolicy = QtWidgets.QSizePolicy(QtWidgets.QSizePolicy.Fixed, QtWidgets.QSizePolicy.Fixed)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.pushButton_send_data.sizePolicy().hasHeightForWidth())
        self.pushButton_send_data.setSizePolicy(sizePolicy)
        self.pushButton_send_data.setStyleSheet("font: 75 14pt \"Arial\";")
        self.pushButton_send_data.setObjectName("pushButton_send_data")
        self.horizontalLayout_7.addWidget(self.pushButton_send_data)
        self.verticalLayout.addLayout(self.horizontalLayout_7)
        self.verticalLayout_2.addLayout(self.verticalLayout)
        self.label_tips = QtWidgets.QLabel(testStupidP2PPythonAPI)
        self.label_tips.setStyleSheet("background-color: rgb(221, 221, 221);\n"
"font: 75 18pt \"Arial\";\n"
"")
        self.label_tips.setText("")
        self.label_tips.setObjectName("label_tips")
        self.verticalLayout_2.addWidget(self.label_tips)

        self.retranslateUi(testStupidP2PPythonAPI)
        QtCore.QMetaObject.connectSlotsByName(testStupidP2PPythonAPI)

    def retranslateUi(self, testStupidP2PPythonAPI):
        _translate = QtCore.QCoreApplication.translate
        testStupidP2PPythonAPI.setWindowTitle(_translate("testStupidP2PPythonAPI", "test stupidP2P Python API"))
        self.label_server_ip.setText(_translate("testStupidP2PPythonAPI", "server ip："))
        self.lineEdit_server_ip.setText(_translate("testStupidP2PPythonAPI", "192.168.2.174"))
        self.label_server_port.setText(_translate("testStupidP2PPythonAPI", "server port："))
        self.lineEdit_server_port.setText(_translate("testStupidP2PPythonAPI", "4040"))
        self.pushButton_connection.setText(_translate("testStupidP2PPythonAPI", "连接"))
        self.label_my_id.setText(_translate("testStupidP2PPythonAPI", "本机Id："))
        self.lineEdit_my_id.setText(_translate("testStupidP2PPythonAPI", "client1"))
        self.pushButton_register.setText(_translate("testStupidP2PPythonAPI", "注册"))
        self.pushButton_unregister.setText(_translate("testStupidP2PPythonAPI", "取消注册"))
        self.label_subscribe_id.setText(_translate("testStupidP2PPythonAPI", "订阅Id："))
        self.lineEdit_subscribe_id.setText(_translate("testStupidP2PPythonAPI", "client2"))
        self.pushButton_check_alive.setText(_translate("testStupidP2PPythonAPI", "检测存在"))
        self.pushButton_subscribe.setText(_translate("testStupidP2PPythonAPI", "订阅"))
        self.pushButton_unsubscribe.setText(_translate("testStupidP2PPythonAPI", "取消订阅"))
        self.pushButton_send_data.setText(_translate("testStupidP2PPythonAPI", "发送"))
