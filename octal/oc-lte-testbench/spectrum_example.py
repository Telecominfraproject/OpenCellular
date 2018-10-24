

from opentest.server import serverinterface
if __name__ == '__main__':
    server = serverinterface.LocalLaunchedServerInterface(port=5050)

    server.spectrum = server.get_service_client_from_name('KeysightEXASpectrumClient', url='spectrum')
    server.spectrum.create_service("TCPIP0::K-N9030B-41982.local::inst0::INSTR")

    server.spectrum.instrument_reset()
    server.spectrum.setup_lte_dl_mode(lte_bw=5, mode='ACP', cont='OFF')
    server.spectrum.read_evm()
    print(server.spectrum.fetch_evm())
