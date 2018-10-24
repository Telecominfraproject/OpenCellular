from nute.service import serverinterface

server = serverinterface.LocalLaunchedServerInterface(port=5005)


com_client = server.get_service_client('COMServiceClient', 'com')
com_client.create_service('COM10', baudrate=115200)

com_client.com_target()
