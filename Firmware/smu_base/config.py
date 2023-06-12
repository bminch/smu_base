def timercon(period):
    FCY = 16e6
    TCY = 62.5e-9

    if period > 256. * 65536. * TCY:
        period = 256. * 65536. * TCY
    if period > 64. * 65536. * TCY:
        TxCON = 0x0030
        PRx = int(period * (FCY / 256.)) - 1
    elif period > 8. * 65536. * TCY:
        TxCON = 0x0020
        PRx = int(period * (FCY / 64.)) - 1
    elif period > 65536. * TCY:
        TxCON = 0x0010
        PRx = int(period * (FCY / 8.)) - 1
    else:
        TxCON = 0x0000
        PRx = int(period * FCY) - 1

    print('TxCON = 0x{:04X}'.format(TxCON))
    print('PRx = 0x{:04X}'.format(PRx))

def spicon(freq, mode):
    modebits = [0x0100, 0x0000, 0x0140, 0x0040]
    FCY = 16e6
    TCY = 62.5e-9

    # Clip freq to be in allowable range of values
    if freq > FCY / 2.:
        freq = FCY / 2.
    if freq < FCY / (64. * 8.):
        freq = FCY / (64. * 8.)

    # Select primary prescale bits
    if freq <= FCY / (2. * 64.):
        freq *= 64.
        primary = 0     # Set primary prescale bits for 64:1
    elif freq <= FCY / (2. * 16.):
        freq *= 16.
        primary = 1     # Set primary prescale bits for 16:1
    elif freq <= FCY / (2. * 4.):
        freq *= 4.
        primary = 2     # Set primary prescale bits for 4:1
    else:
        primary = 3     # Set primary prescale bits for 1:1

    # Compute secondary prescale value to get closest SPI clock freq to that 
    # specified
    secondary = int(0.5 + FCY / freq)
    secondary = (8 - secondary) << 2    # Map secondary prescale bits for SPIxCON1
    SPIxCON1 = 0x0020 | modebits[mode & 0x03] | primary | secondary
    SPIxCON2 = 0
    SPIxSTAT = 0x8000

    print('SPIxCON1 = 0x{:04X}'.format(SPIxCON1))
    print('SPIxCON2 = 0')
    print('SPIxSTAT = 0x8000')

def uartcon(baudrate, parity, stopbits, **kwargs):
    FCY = 16e6
    TCY = 62.5e-9

    parity_dict = {'n': 0x0000, 'e': 0x0002, 'o': 0x0004}
    stopbits_dict = {1: 0x0000, 2: 0x0001}

    UxMODE = 0

    flowcontrol = kwargs.get('flowcontrol', False)
    if flowcontrol:
        UxMODE |= 0x0200

    # Clip baudrate to be in allowable range of values
    if baudrate > FCY / 4.:
        baudrate = FCY / 4.
    if baudrate < FCY / (16. * 65536.):
        baudrate = FCY / (16. * 65536.)

    # Select BRGH value and compute BRG value
    if baudrate <= FCY / (4. * 65536.):
        UxBRG = int(0.5 + (FCY / 16.) / baudrate) - 1
    else:
        UxMODE |= 0x0008
        UxBRG = int(0.5 + (FCY / 4.) / baudrate) - 1

    # Set parity as specified
    UxMODE |= parity_dict[parity.lower()]

    # Set stopbits as specified
    UxMODE |= stopbits_dict[stopbits]

    print('UxMODE = 0x{:04X}'.format(UxMODE))
    print('UxBRG = 0x{:04X}'.format(UxBRG))

