
import asyncio
import sys
from bleak import BleakClient, BleakScanner
from datetime import datetime
from itertools import count, takewhile

from kivy.app import App
from kivy.uix.boxlayout import BoxLayout
from kivy.uix.textinput import TextInput
from kivy.uix.label import Label
from kivy.uix.scrollview import ScrollView
from kivy.properties import StringProperty
from kivy.lang import Builder
from kivy.clock import Clock

UART_SERVICE_UUID = '49535343-fe7d-4ae5-8fa9-9fafd205e455'
UART_RX_CHAR_UUID = '49535343-8841-43f4-a8d4-ecbe34729bb3'
UART_TX_CHAR_UUID = '49535343-1e4d-4bd9-ba61-23c647249616'

MSG_COLOR = 'f1fa8c'
TX_COLOR = 'ff79c6'
RX_COLOR = '8be9fd'
ERR_COLOR = 'ff5555'

Builder.load_string('''
<ScrollableLabel>
    do_scroll_x: False
    on_text: self.scroll_to(scroll_to_point)

    BoxLayout:
        id: layout
        orientation: 'vertical'
        size_hint_y: None
        height: text_display.texture_size[1] + 15

        Label:
            id: text_display
            size_hint_y: None
            height: self.texture_size[1]
            text_size: 0.98 * self.width, None
            text: root.text
            markup: True

        Label:
            id: scroll_to_point
            text: ''

<RootWidget>
    orientation: 'vertical'
    devices: uart_devices_spinner
    input: text_input
    display: display_label
    connect_button: connect_button

    BoxLayout:
        orientation: 'horizontal'
        size_hint_y: 0.1

        Spinner:
            id: uart_devices_spinner
            font_size: '18sp'
            text: 'No devices found'
            on_text: root.uart_devices_spinner_callback()

    BoxLayout:
        orientation: 'horizontal'
        size_hint_y: 0.1

        Button:
            id: connect_button
            size_hint_x: 0.5
            text: 'Connect'
            font_size: '18sp'
            on_press: root.connect_handler()

        Button:
            size_hint_x: 0.5
            text: 'Send'
            font_size: '18sp'
            on_press: root.send_handler()

    TextInput:
        id: text_input
        size_hint_y: 0.1
        font_size: '18sp'
        multiline: True
        text: ''

    ScrollableLabel:
        id: display_label
        size_hint_y: 0.6
        font_size: '18sp'
''')

class ScrollableLabel(ScrollView):

    text = StringProperty('')

class RootWidget(BoxLayout):

    def uart_devices_spinner_callback(self):
        pass

    def connect_handler(self):
        if self.connect_button.text == 'Connect':
            app.connection_event.set()
        else:
            app.connected = False
            app.send_event.set()

    def send_handler(self):
        if app.connected:
            text = self.input.text + '\n'
            app.display(text, color = TX_COLOR)
            app.tx_buffer = text.replace('\n', '\r').encode()
            app.send_event.set()
        self.input.text = ''

class MainApp(App):

    def __init__(self):
        super().__init__()
        self.ble_devices_seen = {}
        self.ble_devices_uart_service  = []
        self.connection_event = asyncio.Event()
        self.connected = False
        self.send_event = asyncio.Event()
        self.tx_buffer = b''
        self.clear_display = False

    def build(self):
        return RootWidget()

    def on_stop(self):
        for task in asyncio.all_tasks():
            task.cancel()

    def display(self, text, **kwargs):
        color = kwargs.get('color', '')
        if color != '':
            text = f'[color={color}]{text}[/color]'

        if self.clear_display:
            self.root.display.text = ''
            self.clear_display = False

        self.root.display.text += text

    async def scan(self):
        new_device = None

        def found_device_callback(device, adv_data):
            nonlocal self
            nonlocal new_device

            if device.address in self.ble_devices_seen:
                self.ble_devices_seen[device.address] = (device, adv_data, datetime.now())
            else:
                self.ble_devices_seen[device.address] = (device, adv_data, datetime.now())
                new_device = device
                self.connection_event.set()

        scanner = BleakScanner(found_device_callback)

        while True:
            await scanner.start()
            await self.connection_event.wait()
            await scanner.stop()

            if new_device is None:
                self.connection_event.clear()
                if len(self.ble_devices_uart_service) != 0:
                    for device in self.ble_devices_uart_service:
                        if device.name == self.root.devices.text:
                            return device
                else:
                    continue

            try:
                async with BleakClient(new_device, None, None, timeout = 10.) as client:
                    for service in client.services:
                        if service.uuid == UART_SERVICE_UUID:
                            self.ble_devices_uart_service.append(new_device)
                            if len(self.root.devices.values) == 0:
                                self.root.devices.text = new_device.name
                            self.root.devices.values = [device.name for device in self.ble_devices_uart_service]
                            self.display(f'Found BLE device {new_device.name} supporting Microchip UART service.\n', color = MSG_COLOR)
                            break

            except asyncio.TimeoutError:
                pass

            new_device = None
            self.connection_event.clear()

    async def connect(self, uart_device):
        rx_buffer = ''

        def disconnect_callback(client):
            nonlocal self
            self.connected = False

        def rx_callback(ch, data):
            nonlocal rx_buffer

            rx_buffer += data.decode()
            if '\r\n' in rx_buffer:
                text, rx_buffer = rx_buffer.split('\r\n')
                self.display(text + '\n', color = RX_COLOR)
            elif '\r' in rx_buffer:
                text, rx_buffer = rx_buffer.split('\r')
                self.display(text + '\n', color = RX_COLOR)

        def packetize(data, n):
            return takewhile(len, (data[i : i + n] for i in count(0, n)))

        self.display(f'Connecting to {uart_device.name}...\n', color = MSG_COLOR)
        try:
            async with BleakClient(uart_device, disconnected_callback = disconnect_callback) as client:
                await client.start_notify(UART_TX_CHAR_UUID, rx_callback)
                uart_service = client.services.get_service(UART_SERVICE_UUID)
                rx_char = uart_service.get_characteristic(UART_RX_CHAR_UUID)

                self.connected = True
                while self.connected:
                    await self.send_event.wait()
                    if len(self.tx_buffer) == 0:
                        self.send_event.clear()
                        break
                    for packet in packetize(self.tx_buffer, rx_char.max_write_without_response_size):
                        await client.write_gatt_char(rx_char, packet)
                    self.tx_buffer = b''
                    self.send_event.clear()

            self.display(f'Disconnected from {uart_device.name}.\n', color = MSG_COLOR)

        except asyncio.TimeoutError:
            self.display(f'Connection to {uart_device.name} timed out.\n', color = ERR_COLOR)

    async def do_ble_stuff(self):
        self.display('Starting scan...\n', color = MSG_COLOR)
        while True:
            uart_device = await self.scan()
            self.root.connect_button.text = 'Disconnect'
            await self.connect(uart_device)
            self.root.connect_button.text = 'Connect'
            self.display('Resuming scan...\n', color = MSG_COLOR)

async def main(app):
    await asyncio.gather(app.async_run('asyncio'), app.do_ble_stuff())

if __name__ == '__main__':
    app = MainApp()
    try:
        asyncio.get_event_loop().run_until_complete(main(app))
    except asyncio.exceptions.CancelledError:
        pass
