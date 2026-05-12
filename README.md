# FlashLoggerV2

A portable, platform-agnostic C/C++ library for logging structured data to flash memory (NOR/NAND, internal or external).

---

## Goals

- **Zero dynamic allocation** — works on bare-metal systems with no heap
- **Configurable schema** — log record layout defined at compile time or init time
- **Wear leveling** — circular buffer with sector rotation to spread writes
- **Crash safe** — power-loss tolerant; incomplete writes are detected and discarded on recovery
- **Portable** — hardware access abstracted behind a thin HAL interface (read page, write page, erase sector)

---

## Design Overview

```
┌─────────────────────────────────────────────────┐
│                  Application                    │
└────────────────────┬────────────────────────────┘
                     │ flashlogger_write(record)
                     ▼
┌─────────────────────────────────────────────────┐
│               FlashLogger Core                  │
│  - Record framing & CRC                         │
│  - Circular sector management                   │
│  - Write pointer / read pointer tracking        │
└────────────────────┬────────────────────────────┘
                     │ hal_read / hal_write / hal_erase
                     ▼
┌─────────────────────────────────────────────────┐
│            HAL (user-provided)                  │
│  e.g. SPI flash, internal flash, EEPROM         │
└─────────────────────────────────────────────────┘
```

---

## Flash Layout

```
[ Sector 0 | Sector 1 | Sector 2 | ... | Sector N ]
     ▲
  active write sector (rotates on full)

Each sector:
  [ Sector Header (magic + seq number) | Record 0 | Record 1 | ... ]

Each record:
  [ Length (2B) | Type (1B) | Payload (variable) | CRC16 (2B) ]
```

---

## API Sketch

```c
// Initialize with flash geometry and HAL callbacks
int flashlogger_init(FlashLogger *ctx, const FlashLoggerConfig *cfg);

// Write a variable-length record
int flashlogger_write(FlashLogger *ctx, uint8_t type, const void *data, uint16_t len);

// Iterate records (oldest-first)
int flashlogger_read_next(FlashLogger *ctx, FlashLoggerRecord *out);

// Erase all log data
int flashlogger_format(FlashLogger *ctx);

// HAL interface the user must implement
typedef struct {
    int (*read) (uint32_t addr, uint8_t *buf, uint32_t len);
    int (*write)(uint32_t addr, const uint8_t *buf, uint32_t len);
    int (*erase)(uint32_t sector_addr, uint32_t sector_size);
} FlashLoggerHAL;
```

---

## Configuration

| Parameter | Description | Example |
|---|---|---|
| `sector_size` | Size of one erasable sector in bytes | `4096` |
| `sector_count` | Number of sectors reserved for the log | `16` |
| `max_record_size` | Maximum payload size per record | `256` |
| `crc_enabled` | Enable CRC16 per record | `true` |

---

## Open Questions / To Decide

- [ ] Should sector headers carry a timestamp, or leave time to the caller?
- [ ] Support for read-back streaming over UART/USB?
- [ ] Encryption or obfuscation of log data?
- [ ] C only, or C++ wrapper with RAII?
- [ ] Log levels (debug / info / warning / error)?

---

## Directory Structure (planned)

```
FlashLoggerV2/
├── include/
│   └── flashlogger.h
├── src/
│   ├── flashlogger.c
│   └── crc.c
├── hal/
│   ├── hal_stm32_spi.c      (example)
│   └── hal_esp32_spi.c      (example)
├── test/
│   └── host_sim/            (PC-side unit tests with simulated flash)
└── README.md
```

---

## License

TBD
