#include "serial_protocol.h"
#include "reg_router.h"
#include <rtthread.h>
#include <rtdevice.h>

#define DBG_TAG "serial"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

#define FRAME_HDR1  0xAA
#define FRAME_HDR2  0x55

#define CMD_READ         0x03
#define CMD_WRITE_SINGLE 0x06
#define CMD_WRITE_MULTI  0x10

#define ERR_OK           0x00
#define ERR_CHECKSUM     0x01
#define ERR_CMD          0x02
#define ERR_ADDR         0x03
#define ERR_ACCESS       0x04

static rt_device_t g_serial = RT_NULL;

/* Frame receive state machine */
static enum { S_HDR1, S_HDR2, S_CMD, S_ADDR_L, S_ADDR_H,
              S_LEN, S_DATA, S_XOR } g_state = S_HDR1;
static uint8_t  g_rx_buf[256];
static uint8_t  g_rx_idx = 0;
static uint8_t  g_rx_data_len = 0;

/* ── Helpers ────────────────────────────────────── */

static uint8_t xor_checksum(const uint8_t *buf, uint8_t len)
{
    uint8_t x = 0;
    for (int i = 0; i < len; i++) x ^= buf[i];
    return x;
}

static void hex_dump(const char *tag, const uint8_t *buf, int len)
{
    rt_kprintf("[%s]", tag);
    for (int i = 0; i < len; i++) rt_kprintf(" %02X", buf[i]);
    rt_kprintf("\n");
}

static void send_response(uint8_t cmd, uint16_t addr,
                          uint8_t count, const uint8_t *data)
{
    uint8_t buf[256];
    uint8_t idx = 0;

    buf[idx++] = FRAME_HDR1;
    buf[idx++] = FRAME_HDR2;
    buf[idx++] = cmd;
    buf[idx++] = (uint8_t)(addr & 0xFF);
    buf[idx++] = (uint8_t)((addr >> 8) & 0xFF);
    buf[idx++] = count;

    for (int i = 0; i < count; i++) buf[idx++] = data[i];

    buf[idx] = xor_checksum(buf, idx);
    idx++;

    hex_dump("AA55 RSP", buf, idx);
    if (g_serial)
        rt_device_write(g_serial, 0, buf, idx);
}

static void send_error(uint8_t code)
{
    uint8_t buf[] = { FRAME_HDR1, FRAME_HDR2, 0xFF, code, 0x00 };
    buf[4] = xor_checksum(buf, 4);
    hex_dump("AA55 ERR", buf, 5);
    if (g_serial)
        rt_device_write(g_serial, 0, buf, 5);
}

static void send_read_error(uint8_t code, uint16_t addr)
{
    uint8_t buf[8];
    buf[0] = FRAME_HDR1;
    buf[1] = FRAME_HDR2;
    buf[2] = 0xFF;
    buf[3] = code;
    buf[4] = (uint8_t)(addr & 0xFF);
    buf[5] = (uint8_t)((addr >> 8) & 0xFF);
    buf[6] = 0x00; /* no data */
    buf[7] = xor_checksum(buf, 7);
    hex_dump("AA55 ERR", buf, 8);
    if (g_serial)
        rt_device_write(g_serial, 0, buf, 8);
}

/* ── Frame processor ────────────────────────────── */

static void process_frame(void)
{
    uint8_t cmd   = g_rx_buf[2];
    uint16_t addr = g_rx_buf[3] | ((uint16_t)g_rx_buf[4] << 8);
    uint8_t count = g_rx_buf[5];
    uint8_t xsum  = xor_checksum(g_rx_buf, g_rx_idx - 1);

    if (xsum != g_rx_buf[g_rx_idx - 1]) {
        LOG_W("Checksum mismatch: calc=0x%02X got=0x%02X",
              xsum, g_rx_buf[g_rx_idx - 1]);
        send_error(ERR_CHECKSUM);
        return;
    }

    switch (cmd) {
    case CMD_READ: {
        uint8_t resp[256];
        int n = reg_read(addr, count, resp);
        if (n < 0) {
            send_read_error(ERR_ADDR, addr);
        } else {
            send_response(CMD_READ, addr, (uint8_t)n, resp);
        }
    } break;

    case CMD_WRITE_SINGLE:
    case CMD_WRITE_MULTI: {
        if (reg_write(addr, count, g_rx_buf + 6) != 0) {
            send_error(ERR_ACCESS);
        } else {
            send_response(cmd, addr, 0, NULL);
        }
    } break;

    default: {
        LOG_W("Unknown command: 0x%02X", cmd);
        send_error(ERR_CMD);
    } break;
    }
}

/* ── RX interrupt callback ──────────────────────── */

static rt_err_t serial_rx_cb(rt_device_t dev, rt_size_t size)
{
    (void)size;
    uint8_t ch;

    while (rt_device_read(dev, 0, &ch, 1) == 1) {
        switch (g_state) {

        case S_HDR1:
            if (ch == FRAME_HDR1) {
                g_rx_buf[0] = ch;
                g_rx_idx = 1;
                g_state = S_HDR2;
            }
            break;

        case S_HDR2:
            if (ch == FRAME_HDR2) {
                g_rx_buf[1] = ch;
                g_rx_idx = 2;
                g_state = S_CMD;
            } else {
                g_state = S_HDR1;
            }
            break;

        case S_CMD:
            g_rx_buf[2] = ch;
            g_rx_idx = 3;
            g_state = S_ADDR_L;
            break;

        case S_ADDR_L:
            g_rx_buf[3] = ch;
            g_rx_idx = 4;
            g_state = S_ADDR_H;
            break;

        case S_ADDR_H:
            g_rx_buf[4] = ch;
            g_rx_idx = 5;
            g_state = S_LEN;
            break;

        case S_LEN:
            g_rx_buf[5] = ch;
            g_rx_idx = 6;
            g_rx_data_len = ch;

            /* Prevent buffer overflow: max safe data = 256 - 6 (header) - 1 (XOR) = 249 */
            if (g_rx_data_len > 249) {
                LOG_W("Frame too long: len=%d > 249, discarded", g_rx_data_len);
                g_state = S_HDR1;
                break;
            }

            /* For read (0x03): count = number of registers to read, no data follows
             * For write (0x06/0x10): count = byte count for 0x10, or 2 for 0x06
             * For unknown cmd: treat like read (no data, next byte is XOR) */
            if (g_rx_buf[2] == CMD_READ ||
                (g_rx_buf[2] != CMD_WRITE_SINGLE && g_rx_buf[2] != CMD_WRITE_MULTI)) {
                /* Read or unknown → no data bytes */
                g_state = S_XOR;
            } else {
                /* Write → data bytes follow */
                if (g_rx_data_len > 0) {
                    g_state = S_DATA;
                } else {
                    g_state = S_XOR;
                }
            }
            break;

        case S_DATA:
            g_rx_buf[g_rx_idx] = ch;
            g_rx_idx++;
            if (g_rx_idx >= (uint8_t)(6 + g_rx_data_len)) {
                g_state = S_XOR;
            }
            break;

        case S_XOR:
            g_rx_buf[g_rx_idx] = ch;
            g_rx_idx++;
            process_frame();
            g_state = S_HDR1;
            break;
        }
    }

    return RT_EOK;
}

/* ── Init ───────────────────────────────────────── */

void serial_protocol_init(void)
{
    g_serial = rt_device_find("uart2");
    if (!g_serial) {
        LOG_E("Cannot find uart2 device");
        return;
    }

    rt_device_open(g_serial,
                   RT_DEVICE_FLAG_RDWR |
                   RT_DEVICE_FLAG_INT_RX);

    rt_device_set_rx_indicate(g_serial, serial_rx_cb);

    LOG_I("Serial protocol ready (uart2)");
}

/* ── Echo test ─────────────────────────────────── */

static rt_device_t g_echo_serial = RT_NULL;
static int g_echo_bytes = 0;

static rt_err_t echo_rx_cb(rt_device_t dev, rt_size_t size)
{
    (void)size;
    uint8_t ch;

    while (rt_device_read(dev, 0, &ch, 1) == 1) {
        /* Echo this byte back with prefix */
        rt_device_write(dev, 0, &ch, 1);
        g_echo_bytes++;

        /* Print stats every 16 bytes */
        if ((g_echo_bytes & 0x0F) == 0) {
            rt_kprintf("\n[echo] %d bytes so far\n", g_echo_bytes);
        }
    }

    return RT_EOK;
}

void uart2_echo_test(void)
{
    g_echo_serial = rt_device_find("uart2");
    if (!g_echo_serial) {
        rt_kprintf("[echo] uart2 not found!\n");
        return;
    }

    rt_device_open(g_echo_serial,
                   RT_DEVICE_FLAG_RDWR |
                   RT_DEVICE_FLAG_INT_RX);

    rt_device_set_rx_indicate(g_echo_serial, echo_rx_cb);

    g_echo_bytes = 0;
    rt_kprintf("[echo] UART2 (PA2/PA3) echo test started\n");
    rt_kprintf("[echo] Send anything on UART2 TX -> will echo back\n");
}

/* ── AA55 protocol test via console ─────────────────── */
#include <stdlib.h>

static int aa55_test(int argc, char **argv)
{
    if (argc < 2) {
        rt_kprintf("Usage: aa55_test <hex bytes...>\n");
        rt_kprintf("  Last byte is XOR (use '00' to auto-calc)\n");
        rt_kprintf("Examples:\n");
        rt_kprintf("  aa55_test AA 55 03 01 00 01 56     (read 0x0001)\n");
        rt_kprintf("  aa55_test AA 55 06 01 00 02 00 00  (write 0x0001 = 0x00 0x00)\n");
        rt_kprintf("  aa55_test AA 55 03 01 00 01 00     (00 = auto XOR)\n");
        rt_kprintf("  cmd=03(read) 06(write_single) 10(write_multi)\n");
        return 0;
    }

    /* Cap input at 256 bytes */
    uint8_t buf[256];
    int auto_xor = 0;
    int len = 0;
    for (int i = 1; i < argc && len < 256; i++) {
        unsigned long v = strtoul(argv[i], NULL, 16);
        if (v == 0 && argv[i][0] == '0' && argv[i][1] == '0' && argv[i][2] == '\0') {
            /* "00" last byte = auto XOR */
            if (i == argc - 1) { auto_xor = 1; buf[len++] = 0; }
            else               { buf[len++] = 0; }
        } else {
            buf[len++] = (uint8_t)v;
        }
    }

    if (len < 6) {
        rt_kprintf("[AA55] Frame too short (need >=6 bytes)\n");
        return 0;
    }

    /* Auto-calculate XOR */
    if (auto_xor) {
        uint8_t x = 0;
        for (int i = 0; i < len - 1; i++) x ^= buf[i];
        buf[len - 1] = x;
    }

    rt_kprintf("[AA55] Inject: ");
    for (int i = 0; i < len; i++) rt_kprintf("%02X ", buf[i]);
    rt_kprintf("\n");

    rt_kprintf("[AA55] CMD=0x%02X ADDR=0x%04X LEN=%d",
               buf[2], buf[3] | ((uint16_t)buf[4] << 8), buf[5]);
    if (len > 6) {
        rt_kprintf(" DATA=");
        for (int i = 6; i < len - 1; i++) rt_kprintf("%02X ", buf[i]);
    }
    rt_kprintf(" XOR=0x%02X%s\n", buf[len - 1], auto_xor ? " (auto)" : "");

    /* Feed into state machine */
    g_state = S_HDR1;
    for (int i = 0; i < len; i++) {
        switch (g_state) {
        case S_HDR1:
            if (buf[i] == FRAME_HDR1) {
                g_rx_buf[0] = buf[i]; g_rx_idx = 1; g_state = S_HDR2;
            }
            break;
        case S_HDR2:
            if (buf[i] == FRAME_HDR2) {
                g_rx_buf[1] = buf[i]; g_rx_idx = 2; g_state = S_CMD;
            } else { g_state = S_HDR1; }
            break;
        case S_CMD:  g_rx_buf[2] = buf[i]; g_rx_idx = 3; g_state = S_ADDR_L; break;
        case S_ADDR_L: g_rx_buf[3] = buf[i]; g_rx_idx = 4; g_state = S_ADDR_H; break;
        case S_ADDR_H: g_rx_buf[4] = buf[i]; g_rx_idx = 5; g_state = S_LEN; break;
        case S_LEN:
            g_rx_buf[5] = buf[i]; g_rx_idx = 6; g_rx_data_len = buf[i];
            if (buf[2] == CMD_READ || (buf[2] != CMD_WRITE_SINGLE && buf[2] != CMD_WRITE_MULTI))
                g_state = S_XOR;
            else
                g_state = (g_rx_data_len > 0) ? S_DATA : S_XOR;
            break;
        case S_DATA:
            g_rx_buf[g_rx_idx] = buf[i]; g_rx_idx++;
            if (g_rx_idx >= (uint8_t)(6 + g_rx_data_len)) g_state = S_XOR;
            break;
        case S_XOR:
            g_rx_buf[g_rx_idx] = buf[i]; g_rx_idx++;
            process_frame();
            g_state = S_HDR1;
            break;
        }
    }
    return 0;
}
MSH_CMD_EXPORT(aa55_test, "Inject AA55 frame for protocol test");
MSH_CMD_EXPORT(uart2_echo_test, UART2 PA2/PA3 echo test);
