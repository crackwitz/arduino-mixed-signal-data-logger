# Arduino Data Logger

This Arduino sketch provides a data logger for both analog and digital inputs. The data is read periodically and written to the UART.

## Features

- Output format is "CSV with comments".
- Supports both analog and digital channels.
- Allows enabling and disabling of individual channels.
- Supports changing the sampling period and frequency at 1 ms resolution.
- Supports changing the baud rate.
- Provides status updates on enabled channels and sampling frequency.

## Usage

The data logger accepts commands via the serial interface. The following commands are supported:

- `h`, `help`, `?`: Print help information.
- `s`: Print status (enabled channels, sampling period/frequency).
- `b<baud>`: Change baud rate. 1000000 and 2000000 baud are supported.
- `<ENTER>`: Toggle sampling (blank command).
- `<CTRL-C>`, `.`: Stop sampling.
- `p<period>`: Set sampling period in ms.
- `f<freq>`: Set sampling frequency in Hz, floats allowed, 1 ms resolution.
- `t`: Toggle printing of timestamps (in milliseconds).
- `aN,N...`, `a`: Disable analog channels, N = 0..7.
- `AN,N...`, `A`: Enable analog channels, N = 0..7.
- `dN,N...`, `d`: Disable digital channels, N = 0..19.
- `DN,N...`: Enable digital channels, N = 0..19.

## Setup

The sketch sets all ADC channels to input in the `setup()` function. It also starts the serial communication at a baud rate of 115200.

## Note

This sketch is designed for Arduino Nano. The analog channel values are in the range 0..1023, and digital channel values are reported as 0 or 1024.
