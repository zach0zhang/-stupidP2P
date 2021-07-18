import time
import queue
import socket
import threading
import collections

class clientRecvThread(threading.Thread):

    __commandHeaderLength               = 0x4

    __registerCommand           = 0x01
    __subscribeCommand          = 0x02
    __sendDataCommand           = 0x03
    __unregisterCommand         = 0x04
    __unsubscribeCommand        = 0x05
    __checkCommand              = 0x06
    __heartBeatCommand          = 0x07

    __registerCommandResponse           = 0x81
    __subscribeCommandResponse          = 0x82
    __sendDataCommandResponse           = 0x83
    __unregisterCommandResponse         = 0x84
    __unsubscribeCommandResponse        = 0x85
    __checkCommandResponse              = 0x86
    __heartBeatCommandResponse          = 0x87

    __failResponse                      = 0x0
    __successResponse                   = 0x1
    __notReceiveResponse                = 0xff


    __backIndexMax              = __heartBeatCommand + 1

    def __init__(self, sock):
        super(clientRecvThread, self).__init__()
        self._stop_event = threading.Event()
        self.sock = sock
        self._recvDeque = collections.deque()

        self.commandResponse = []
        for i in range(self.__backIndexMax):
            self.commandResponse.append(self.__notReceiveResponse)

        self.recvDataQueue = queue.Queue(maxsize = 4096)

    def __getCommandResponse(self, nowPopLength):
        return nowPopLength+1, self._recvDeque.pop()
        
        
    def run(self):
        while not self.stopped():
            try:
                recvData = self.sock.recv(1024)
            except socket.timeout:
                continue
            except socket.error as e:
                #print(str(e))
                time.sleep(1)
                continue
            else:
                if len(recvData):
                    self._recvDeque.extendleft(list(recvData))
                    #print(self._recvDeque)

            while len(self._recvDeque) > self.__commandHeaderLength:
                headerBytes = bytes()
                for i in range(self.__commandHeaderLength):
                    headerBytes += self._recvDeque.pop().to_bytes(1, "big")
                headerLength = int.from_bytes(headerBytes, "big")
                if len(self._recvDeque) >= headerLength:
                    command = self._recvDeque.pop()
                    popLength = 1
                    
                    # register device command response
                    if command == self.__registerCommandResponse:
                        popLength, self.commandResponse[self.__registerCommand] = self.__getCommandResponse(popLength)
                    
                    # subscribe device command response
                    elif command == self.__subscribeCommandResponse:
                        popLength, self.commandResponse[self.__subscribeCommand] = self.__getCommandResponse(popLength)
                    
                    # send data command respon
                    elif command == self.__sendDataCommandResponse:
                        popLength, self.commandResponse[self.__sendDataCommand] = self.__getCommandResponse(popLength)

                    # unregister command respon
                    elif command == self.__unregisterCommandResponse:
                        popLength, self.commandResponse[self.__unregisterCommand] = self.__getCommandResponse(popLength)

                    # unsubscribe command respon
                    elif command == self.__unsubscribeCommandResponse:
                        popLength, self.commandResponse[self.__unsubscribeCommand] = self.__getCommandResponse(popLength)

                    # check device alive command respon
                    elif command == self.__checkCommandResponse:
                        popLength, self.commandResponse[self.__checkCommand] = self.__getCommandResponse(popLength)

                    # heart beat command respon
                    elif command == self.__heartBeatCommandResponse:
                        popLength, self.commandResponse[self.__heartBeatCommand] = self.__getCommandResponse(popLength)

                    # receive send data command
                    elif command == self.__sendDataCommand:
                        data = []
                        for j in range(headerLength - popLength):
                            data.append(self._recvDeque.pop())

                        popLength = headerLength

                        if not self.recvDataQueue.full():
                            self.recvDataQueue.put(data)
                        else:
                            # TODO: recv data queue is full
                            pass

                    else:
                        # TODO: unknown command
                        pass
                    
                    for j in range(headerLength - popLength):
                        self._recvDeque.pop()
                else:
                    self._recvDeque.extend(list(headerLength.to_bytes(4, 'little')))
                    break

    def stop(self):
        self._stop_event.set()

    def stopped(self):
        return self._stop_event.is_set()

class clientCheckNetThread(threading.Thread):
    def __init__(self, heartBeatFunc):
        super(clientCheckNetThread, self).__init__()
        self._stop_event = threading.Event()
        self.heartBeatFunc = heartBeatFunc

    def run(self):
        while not self.stopped():
            time.sleep(3)
            self.heartBeatFunc()

    def stop(self):
        self._stop_event.set()

    def stopped(self):
        return self._stop_event.is_set()

class stupidP2PClient():

    __registerCommand           = 0x01
    __subscribeCommand          = 0x02
    __sendDataCommand           = 0x03
    __unregisterCommand         = 0x04
    __unsubscribeCommand        = 0x05
    __checkCommand              = 0x06
    __heartBeatCommand          = 0x07

    __failResponse                      = 0x0
    __successResponse                   = 0x1
    __notReceiveResponse                = 0xff


    def __init__(self, serverIp, serverPort):
        self.serverIp = serverIp
        self.serverPort = serverPort

        self.runningFlag = 0
        self.needToRestartFlag = 0

        self.notReceiveNum = 0

    def initClient(self):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.settimeout(3)
        try:
            self.sock.connect((self.serverIp, self.serverPort))
        except:
            return False

        self.recvThread = clientRecvThread(self.sock)
        self.recvThread.start()

        self.checkNetThread = clientCheckNetThread(self.__heartBeat)
        self.checkNetThread.start()
        
        self.runningFlag = 1

        return True

    def deinitClient(self):
        if self.runningFlag:
            self.runningFlag = 0

            self.checkNetThread.stop()
            while not self.checkNetThread.stopped():
                time.sleep(0.01)

            self.recvThread.stop()
            while not self.recvThread.stopped():
                time.sleep(0.01)
        
            self.sock.close()

    def restartClient(self):
        print("net lost now restart")
        ret = False
        tryNum = 0
        while ret == False:
            self.deinitClient()
            ret = self.initClient()
            if ret == True:
                print("restart success")
                self.needToRestartFlag = 0
            else:
                tryNum += 1
                print("restart false try again {0}".format(tryNum))


    def __send(self, command):
        commandLength = len(command)
        commandSendBytes = commandLength.to_bytes(4, 'big') + command
        if self.runningFlag and self.needToRestartFlag == 0:
            try:
                self.sock.send(commandSendBytes)
            except socket.error as e:
                #print(str(e))
                self.needToRestartFlag  = 1

    def __sendCommand(self, command, data):
        if self.runningFlag == 0 or self.needToRestartFlag == 1:
            return self.__failResponse

        sendCommand = bytes(0)
        sendCommand += command.to_bytes(1, 'big')
        sendCommand += data

        loop = 0

        self.recvThread.commandResponse[command] = self.__notReceiveResponse
        self.__send(sendCommand)

        while self.recvThread.commandResponse[command] == self.__notReceiveResponse:
            time.sleep(0.01)
            if loop > 300:
                break
            else:
                loop += 1

        return self.recvThread.commandResponse[command]
        

    def registerDevice(self, deviceId):
        self.deviceId = deviceId
        return self.__sendCommand(self.__registerCommand, bytes(self.deviceId, encoding="UTF-8"))

    def subscribeDevice(self, subscribeDeviceId):
        return self.__sendCommand(self.__subscribeCommand, bytes(subscribeDeviceId, encoding="UTF-8"))

    def sendData(self, data):
        if len(data) == 0:
            return self.__failResponse
        return self.__sendCommand(self.__sendDataCommand, bytes(data, encoding="UTF-8"))

    def unregisterDevice(self):
        return self.__sendCommand(self.__unregisterCommand, bytes(0))

    def unscribeDevice(self, unscriveDeviceId):
        return self.__sendCommand(self.__unsubscribeCommand, bytes(unscriveDeviceId, encoding="UTF-8"))

    def checkDeviceAlive(self, checkDeviceId):
        return self.__sendCommand(self.__checkCommand, bytes(checkDeviceId, encoding="UTF-8"))

    def recvData(self):
        dataList = []
        if self.needToRestartFlag == 0 or self.runningFlag == 1:
            while not self.recvThread.recvDataQueue.empty():
                dataList.append(self.recvThread.recvDataQueue.get())

        return dataList


    def __heartBeat(self):
        ret = self.__sendCommand(self.__heartBeatCommand, bytes(0))
        if ret == self.__notReceiveResponse:
            self.notReceiveNum += 1

        if self.notReceiveNum > 2 or self.needToRestartFlag:
            self.notReceiveNum = 0
            self.restartClient()
        return ret


if __name__ == '__main__':
    client = stupidP2PClient("192.168.11.63", 4040)
    ret = client.initClient()
    print(ret)
    if ret:
        client.registerDevice("client1")
        client.checkDeviceAlive("client2")
        client.subscribeDevice("clinet2")
        client.sendData("hello")
        print(client.recvData())
        client.unscribeDevice("client2")
        client.unregisterDevice()
        client.deinitClient()
