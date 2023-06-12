
import asyncio
import sys
from bleak import BleakClient, BleakScanner
from datetime import datetime
from itertools import count, takewhile

UART_SERVICE_UUID = '49535343-fe7d-4ae5-8fa9-9fafd205e455'
UART_RX_CHAR_UUID = '49535343-8841-43f4-a8d4-ecbe34729bb3'
UART_TX_CHAR_UUID = '49535343-1e4d-4bd9-ba61-23c647249616'

ble_devices_seen = {}
ble_devices_uart_service  = []

async def scanning():
    global ble_devices_seen
    global ble_devices_uart_service

    check_new_device_event = asyncio.Event()
    new_device = None
    uart_device = None

    def found_device_callback(device, adv_data):
        global ble_devices_seen
        nonlocal check_new_device_event
        nonlocal new_device

        if device.address in ble_devices_seen:
            ble_devices_seen[device.address] = (device, adv_data, datetime.now())
        else:
            ble_devices_seen[device.address] = (device, adv_data, datetime.now())
            new_device = device
            check_new_device_event.set()

    scanner = BleakScanner(found_device_callback)

    print('Starting scan...')
    while True:
        await scanner.start()
        await check_new_device_event.wait()
        await scanner.stop()
        try:
            async with BleakClient(new_device, None, None, timeout = 2.) as client:
                for service in client.services:
                    if service.uuid == UART_SERVICE_UUID:
                        ble_devices_uart_service.append(new_device)
                        print(f'Found BLE device {new_device} supporting Microchip UART service...')
                        return new_device
        except asyncio.TimeoutError:
            pass
        new_device = None
        check_new_device_event.clear()

async def connected(uart_device):
    connected = True
    rx_buffer = ''

    def disconnect_callback(client):
        nonlocal connected
        connected = False

    def rx_callback(ch, data):
        nonlocal rx_buffer

        rx_buffer += data.decode()
        if '\r\n' in rx_buffer:
            display, rx_buffer = rx_buffer.split('\r\n')
            print(display)
        elif '\r' in rx_buffer:
            display, rx_buffer = rx_buffer.split('\r')
            print(display)

    def packetize(data, n):
        return takewhile(len, (data[i : i + n] for i in count(0, n)))

    print(f'Connecting to {uart_device}...')
    try:
        async with BleakClient(uart_device, disconnected_callback = disconnect_callback) as client:
            await client.start_notify(UART_TX_CHAR_UUID, rx_callback)

            loop = asyncio.get_running_loop()
            uart_service = client.services.get_service(UART_SERVICE_UUID)
            rx_char = uart_service.get_characteristic(UART_RX_CHAR_UUID)

            while connected:
                data = await loop.run_in_executor(None, sys.stdin.buffer.readline)
                if not data:
                    break
                data = data.replace(b'\n', b'\r')

                for packet in packetize(data, rx_char.max_write_without_response_size):
                    await client.write_gatt_char(rx_char, packet)

    except asyncio.TimeoutError:
        print(f'Could not connect to {uart_device}.')

async def main():
    uart_device = await scanning()
    await connected(uart_device)

if __name__ == '__main__':
    asyncio.run(main())

