# USART3 protocol, version 1

USART3 uses 921600 baud, 8 data bits, no parity, 1 stop bit (8N1), without
hardware flow control. The link is full-duplex: the PC sends control frames
to the MCU, and the MCU sends audio frames to the PC. Binary frames are sent
back-to-back with no text, delimiters, or additional padding.

## Frame layout

All multi-byte integer fields are little-endian. The fixed header is 10 bytes;
the complete frame is `10 + payload_length + 2` bytes.

| Offset | Size | Field | Description |
|---:|---:|---|---|
| 0 | 1 | Sync byte 0 | `0xA5` |
| 1 | 1 | Sync byte 1 | `0x5A` |
| 2 | 1 | Version | `0x01` |
| 3 | 1 | Type | Frame type listed below |
| 4 | 4 | Sequence | Unsigned 32-bit packet number |
| 8 | 2 | Payload length | Unsigned 16-bit byte count, excludes header and CRC |
| 10 | variable | Payload | Data for this frame type |
| 10 + length | 2 | CRC-16/CCITT-FALSE | Little-endian CRC covering version through payload |

CRC parameters: polynomial `0x1021`, initial value `0xFFFF`, no reflection,
final XOR `0x0000`. The sync bytes are not included in the CRC.

## Frame types

| Type | Direction | Payload length | Meaning |
|---:|---|---:|---|
| `0x01` | PC → MCU | 0 | Start recording |
| `0x02` | PC → MCU | 0 | Stop recording |
| `0x10` | MCU → PC | 160 | Audio block |

For start/stop commands the sequence field is currently ignored; send zero.
Commands with an unsupported version, invalid length, or incorrect CRC are
ignored. The MCU does not send acknowledgements. The PC should wait for the
first audio frame to confirm that recording has started.

An audio payload consists of 80 mono signed PCM samples, each 16-bit
little-endian, sampled at 16 kHz. The resulting audio frame is 172 bytes and
represents 5 ms of audio. The audio sequence starts at zero on each start
command and advances for each captured block offered for transmission. A
sequence gap indicates a dropped block; the sequence wraps modulo 2^32.

After a stop command, no new audio frames are scheduled. An audio frame whose
UART DMA transmission had already started may finish before the line becomes
idle. A subsequent start command begins a new recording and resets the audio
sequence to zero.

## Example command frames

The start command has a zero sequence and zero-length payload:

```text
A5 5A 01 01 00 00 00 00 00 00 8C CE
```

The stop command with the same sequence:

```text
A5 5A 01 02 00 00 00 00 00 00 0E 16
```

The last two bytes in each frame are the CRC-16/CCITT-FALSE in little-endian
order.
