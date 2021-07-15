import time
import queue
import socket
import threading
import collections

class clientRecvThread(threading.Thread):

    __commandHeaderLength               = 0x4

    __registerCommandResponse           = 0x81
    __subscribeCommandResponse          = 0x82
    __sendDataCommandResponse           = 0x83
    __unregisterCommandResponse         = 0x84
    __unsubscribeCommandResponse        = 0x85
    __checkCommandResponse              = 0x86
    __heartBeatCommandResponse          = 0x87
    __sendDataCommand                   = 0x03

    __failResponse                      = 0x0
    __successResponse                   = 0x1
    __notReceiveResponse                = 0xff


    def __init__(self, sock):
        super(clientRecvThread, self).__init__()
        self._stop_event = threading.Event()
        self.sock = sock
        self._recvDeque = collections.deque()

        self.registerBack = self.__notReceiveResponse
        self.subscribeBack = self.__notReceiveResponse
        self.sendDataBack = self.__notReceiveResponse
        self.unregisterBack = self.__notReceiveResponse
        self.unsubscribeBack = self.__notReceiveResponse
        self.checkBack = self.__notReceiveResponse
        self.heartBeatBack = self.__notReceiveResponse

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
                print(str(e))
                time.sleep(1)
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
                        self.registerBack
                        self.registerBack = self._recvDeque.pop()
                        popLength += 1
                    
                    # subscribe device command response
                    elif command == self.__subscribeCommandResponse:
                        popLength, self.subscribeBack = self.__getCommandResponse(popLength)
                    
                    # send data command respon
                    elif command == self.__sendDataCommandResponse:
                        popLength, self.sendDataBack = self.__getCommandResponse(popLength)

                    # unregister command respon
                    elif command == self.__unregisterCommandResponse:
                        popLength, self.unregisterBack = self.__getCommandResponse(popLength)

                    # unsubscribe command respon
                    elif command == self.__unsubscribeCommandResponse:
                        popLength, self.unsubscribeBack = self.__getCommandResponse(popLength)

                    # check device alive command respon
                    elif command == self.__checkCommandResponse:
                        popLength, self.checkBack = self.__getCommandResponse(popLength)

                    # heart beat command respon
                    elif command == self.__heartBeatCommandResponse:
                        popLength, self.heartBeatBack = self.__getCommandResponse(popLength)

                    # receive send data command
                    elif command == self.__sendDataCommand:
                        data = []
                        for j in (headerLength - popLength):
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


            try:
                self.sock.send("hello".encode("utf-8"))
            except socket.error as e:
                print("send error" + str(e))

    def stop(self):
        self._stop_event.set()

    def stopped(self):
        return self._stop_event.is_set()

class stupidP2PClient():
    def __init__(self, serverIp, serverPort, deviceId):
        self.serverIp = serverIp
        self.serverPort = serverPort
        self.deviceId = deviceId

    def initClient(self):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.settimeout(3)
        try:
            self.sock.connect((self.serverIp, self.serverPort))
        except:
            return False

        self.recvThread = clientRecvThread(self.sock)
        self.recvThread.start()
        self.runningFlag = 1

        return True

    def deinitClient(self):
        if self.runningFlag:
            self.recvThread.stop()
            while not self.recvThread.stopped():
                time.sleep(0.1)
        
        self.sock.close()
        self.runningFlag = 0

if __name__ == '__main__':
    client = stupidP2PClient("127.0.0.1", 4040, "client1")
    print(client.initClient())
    time.sleep(100)
    client.deinitClient()
