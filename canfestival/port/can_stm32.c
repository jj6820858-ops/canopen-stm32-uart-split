/* Prevent GCC14 newlib signal.h conflict with RT-Thread libc_signal.h */
#define _SIGNAL_H_

#include <rtthread.h>
#include <string.h>
#include "can_stm32.h"

/* STM32F103RC device select (required by HAL) */
#if !defined(STM32F103xE)
#define STM32F103xE
#endif

#include "stm32f1xx_hal.h"

#define CAN_RX_BUF_SIZE    32

static CAN_HandleTypeDef  hcan1;
static Message            rx_buf[CAN_RX_BUF_SIZE];
static volatile uint8_t   rx_head = 0;
static volatile uint8_t   rx_tail = 0;
static volatile uint8_t   rx_count = 0;

/* Convert CANfestival baud string to prescaler.
 * APB1 clock = 36 MHz, 250 Kbps target:
 *   TQ = Prescaler / 36MHz = 9 / 36MHz = 0.25 µs
 *   BitTime = (1 + 11 + 4) × TQ = 16 × 0.25 µs = 4 µs
 *   BaudRate = 1 / 4µs = 250 Kbps
 */
static void can_set_baudrate(char *baud)
{
    CAN_InitTypeDef *init = &hcan1.Init;

    /* Common settings for all baud rates */
    init->Mode                = CAN_MODE_NORMAL;
    init->SyncJumpWidth       = CAN_SJW_2TQ;
    init->TimeTriggeredMode   = DISABLE;
    init->AutoBusOff          = DISABLE;
    init->AutoWakeUp          = DISABLE;
    init->AutoRetransmission  = DISABLE;
    init->ReceiveFifoLocked   = DISABLE;
    init->TransmitFifoPriority = DISABLE;

    if (strstr(baud, "1M")) {
        init->Prescaler = 2;
        init->TimeSeg1 = CAN_BS1_5TQ;
        init->TimeSeg2 = CAN_BS2_3TQ;
    } else if (strstr(baud, "500K")) {
        init->Prescaler = 4;
        init->TimeSeg1 = CAN_BS1_7TQ;
        init->TimeSeg2 = CAN_BS2_2TQ;
    } else if (strstr(baud, "250K")) {
        /* 250 Kbps: Prescaler=9, BS1=11TQ, BS2=4TQ, SJW=2TQ */
        init->Prescaler = 9;
        init->TimeSeg1 = CAN_BS1_11TQ;
        init->TimeSeg2 = CAN_BS2_4TQ;
    } else {
        /* 50K default (PCLK1=36MHz) */
        init->Prescaler = 60;
        init->TimeSeg1 = CAN_BS1_9TQ;
        init->TimeSeg2 = CAN_BS2_2TQ;
    }
}

static void can_test_auto_start(rt_uint32_t interval_ms);

void can_hardware_init(void)
{
    __HAL_RCC_CAN1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_AFIO_CLK_ENABLE();

    /* PA15 = CAN transceiver RS pin, pull low for normal mode */
    __HAL_AFIO_REMAP_SWJ_NOJTAG();  /* release PA15 from JTAG */
    GPIO_InitTypeDef gpio_rs = {0};
    gpio_rs.Pin = GPIO_PIN_15;
    gpio_rs.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_rs.Pull = GPIO_NOPULL;
    gpio_rs.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &gpio_rs);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET);

    /* PA12=CAN_TX (output), PA11=CAN_RX (input) */
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_12;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);

    gpio.Pin = GPIO_PIN_11;
    gpio.Mode = GPIO_MODE_INPUT;       /* CAN_RX is an input, not output */
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio);

    hcan1.Instance = CAN1;
    can_set_baudrate("250K");
    if (HAL_CAN_Init(&hcan1) != HAL_OK) {
        rt_kprintf("[CAN] Init FAILED! ErrorCode=0x%08lx\n", (unsigned long)hcan1.ErrorCode);
        return;
    }

    /* Configure filter 1: ID mask mode, 32-bit, accept all (CANopen master).
     * FilterBank 1 (STM32F103 has filters 0-13 for CAN1).
     * SlaveStartFilterBank=14 is reserved for CAN2 (not used on F103). */
    CAN_FilterTypeDef filter = {0};
    filter.FilterBank           = 1;
    filter.FilterMode           = CAN_FILTERMODE_IDMASK;
    filter.FilterScale           = CAN_FILTERSCALE_32BIT;
    filter.FilterIdHigh         = 0x0000;
    filter.FilterIdLow          = 0x0000;
    filter.FilterMaskIdHigh     = 0x0000;
    filter.FilterMaskIdLow      = 0x0000;
    filter.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    filter.FilterActivation     = ENABLE;
    filter.SlaveStartFilterBank = 14;
    HAL_CAN_ConfigFilter(&hcan1, &filter);

    if (HAL_CAN_Start(&hcan1) != HAL_OK) {
        rt_kprintf("[CAN] Start FAILED! ErrorCode=0x%08lx\n", (unsigned long)hcan1.ErrorCode);
        return;
    }
    HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING);

    /* Enable CAN RX interrupt in NVIC */
    HAL_NVIC_SetPriority(USB_LP_CAN1_RX0_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(USB_LP_CAN1_RX0_IRQn);

    /* CAN test thread disabled - CANopen stack handles all CAN communication */
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CAN_RxHeaderTypeDef rx_header;
    Message msg;

    HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rx_header, msg.data);

    msg.cob_id = rx_header.StdId;
    msg.len = rx_header.DLC;
    msg.rtr = (rx_header.RTR == CAN_RTR_REMOTE) ? 1 : 0;

    if (rx_count < CAN_RX_BUF_SIZE) {
        rx_buf[rx_head] = msg;
        rx_head = (rx_head + 1) % CAN_RX_BUF_SIZE;
        rx_count++;
    }
}

void USB_LP_CAN1_RX0_IRQHandler(void)
{
    HAL_CAN_IRQHandler(&hcan1);
}

UNS8 canReceive(CAN_HANDLE fd, Message *m)
{
    (void)fd;
    if (rx_count == 0) return 0;

    rt_enter_critical();
    *m = rx_buf[rx_tail];
    rx_tail = (rx_tail + 1) % CAN_RX_BUF_SIZE;
    rx_count--;
    rt_exit_critical();

    return 1;
}

UNS8 canSend(CAN_HANDLE fd, Message const *m)
{
    (void)fd;
    CAN_TxHeaderTypeDef tx_header = {0};
    uint32_t tx_mailbox;

    tx_header.StdId = m->cob_id;
    tx_header.ExtId = 0;
    tx_header.IDE = CAN_ID_STD;
    tx_header.RTR = m->rtr ? CAN_RTR_REMOTE : CAN_RTR_DATA;
    tx_header.DLC = m->len;
    tx_header.TransmitGlobalTime = DISABLE;

    if (HAL_CAN_AddTxMessage(&hcan1, &tx_header, (uint8_t *)m->data,
                             &tx_mailbox) != HAL_OK) {
        /* Attempt Bus-Off recovery */
        if (__HAL_CAN_GET_FLAG(&hcan1, CAN_FLAG_BOF)) {
            HAL_CAN_Stop(&hcan1);
            CAN_FilterTypeDef filter = {0};
            filter.FilterBank = 1;
            filter.FilterMode = CAN_FILTERMODE_IDMASK;
            filter.FilterScale = CAN_FILTERSCALE_32BIT;
            filter.FilterIdHigh = 0x0000;
            filter.FilterIdLow = 0x0000;
            filter.FilterMaskIdHigh = 0x0000;
            filter.FilterMaskIdLow = 0x0000;
            filter.FilterFIFOAssignment = CAN_FILTER_FIFO0;
            filter.FilterActivation = ENABLE;
            HAL_CAN_ConfigFilter(&hcan1, &filter);
            HAL_CAN_Start(&hcan1);
            HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING);
            /* Retry once after recovery */
            if (HAL_CAN_AddTxMessage(&hcan1, &tx_header, (uint8_t *)m->data,
                                     &tx_mailbox) == HAL_OK) {
                return 1;
            }
        }
        return 0;
    }
    return 1;
}

CAN_HANDLE canOpen(s_BOARD *board)
{
    (void)board;
    return (CAN_HANDLE)1;
}

int canClose(CAN_HANDLE fd)
{
    (void)fd;
    HAL_CAN_Stop(&hcan1);
    return 0;
}

UNS8 canChangeBaudRate(CAN_HANDLE fd, char *baud)
{
    (void)fd;
    HAL_CAN_Stop(&hcan1);
    can_set_baudrate(baud);
    HAL_CAN_Init(&hcan1);
    HAL_CAN_Start(&hcan1);
    return 0;
}

/* ==================================================================
 * CAN Test: continuously send a message for USB-CAN analyzer testing
 * ================================================================== */
#include <finsh.h>

static rt_thread_t g_test_thread = RT_NULL;
static volatile int  g_test_running = 0;

static void can_test_entry(void *param)
{
    rt_uint32_t interval_ms = (rt_uint32_t)(rt_ubase_t)param;
    rt_uint32_t count = 0;

    rt_kprintf("[CAN_TEST] Started, interval=%dms, ID=0x123\n", interval_ms);
    rt_kprintf("[CAN_TEST] Use 'can_test_stop' to stop, 'can_test_start 200' to resume\n");

    while (g_test_running)
    {
        CAN_TxHeaderTypeDef tx_header = {0};
        uint32_t tx_mailbox;
        uint8_t data[8];

        for (int i = 0; i < 8; i++)
            data[i] = count + i;

        tx_header.StdId               = 0x123;
        tx_header.ExtId               = 0;
        tx_header.IDE                 = CAN_ID_STD;
        tx_header.RTR                 = CAN_RTR_DATA;
        tx_header.DLC                 = 8;
        tx_header.TransmitGlobalTime  = DISABLE;

        HAL_StatusTypeDef ret = HAL_CAN_AddTxMessage(&hcan1, &tx_header,
                                                      data, &tx_mailbox);
        if (ret == HAL_OK) {
            if ((count % 100) == 0) {
                rt_kprintf("[CAN_TEST] Sent %d frames OK (count=%d)\n",
                           (count + 1), count);
            }
        } else {
            uint32_t esr = hcan1.Instance->ESR;
            rt_kprintf("[CAN_TEST] FAIL count=%d, ret=%d "
                       "state=%lu err=0x%08lx "
                       "ESR=0x%08lx (TEC=%lu REC=%lu LEC=%lu)\n",
                       count, ret,
                       (unsigned long)hcan1.State,
                       (unsigned long)hcan1.ErrorCode,
                       (unsigned long)esr,
                       (unsigned long)((esr & CAN_ESR_TEC) >> 16),
                       (unsigned long)((esr & CAN_ESR_REC) >> 8),
                       (unsigned long)(esr & CAN_ESR_LEC));

            /* Bus-Off recovery: stop and restart the CAN controller */
            if (__HAL_CAN_GET_FLAG(&hcan1, CAN_FLAG_BOF)) {
                rt_kprintf("[CAN_TEST] Bus-Off detected, recovering...\n");
                HAL_CAN_Stop(&hcan1);
                rt_thread_mdelay(20);

                /* Re-init filter (HAL_CAN_Start doesn't restore filters) */
                CAN_FilterTypeDef filter = {0};
                filter.FilterBank = 0;
                filter.FilterMode = CAN_FILTERMODE_IDMASK;
                filter.FilterScale = CAN_FILTERSCALE_32BIT;
                filter.FilterIdHigh = 0x0000;
                filter.FilterIdLow = 0x0000;
                filter.FilterMaskIdHigh = 0x0000;
                filter.FilterMaskIdLow = 0x0000;
                filter.FilterFIFOAssignment = CAN_FILTER_FIFO0;
                filter.FilterActivation = ENABLE;
                HAL_CAN_ConfigFilter(&hcan1, &filter);

                HAL_CAN_Start(&hcan1);
                HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING);
                rt_kprintf("[CAN_TEST] Bus-Off recovery complete\n");
            }
        }

        count++;
        rt_thread_mdelay(interval_ms);
    }
    rt_kprintf("[CAN_TEST] Stopped after %d frames\n", count);
}

static int can_test_start(int argc, char **argv)
{
    rt_uint32_t interval = 200; /* default 200ms */

    if (g_test_running) {
        rt_kprintf("[CAN_TEST] Already running!\n");
        return 0;
    }

    if (argc >= 2) {
        interval = atoi(argv[1]);
        if (interval < 5) interval = 5;
        if (interval > 10000) interval = 10000;
    }

    g_test_running = 1;
    g_test_thread = rt_thread_create("cantest", can_test_entry,
                                      (void *)(rt_ubase_t)interval,
                                      1024, 12, 10);
    if (g_test_thread) {
        rt_thread_startup(g_test_thread);
    }
    return 0;
}
MSH_CMD_EXPORT(can_test_start, Start CAN test: can_test_start [interval_ms]);

static int can_test_stop(int argc, char **argv)
{
    (void)argc; (void)argv;
    if (!g_test_running) {
        rt_kprintf("[CAN_TEST] Not running\n");
        return 0;
    }
    g_test_running = 0;
    if (g_test_thread) {
        rt_thread_delete(g_test_thread);
        g_test_thread = RT_NULL;
    }
    return 0;
}
MSH_CMD_EXPORT(can_test_stop, Stop CAN test);

static void can_test_auto_start(rt_uint32_t interval_ms)
{
    if (g_test_running) return;
    g_test_running = 1;
    g_test_thread = rt_thread_create("cantest", can_test_entry,
                                      (void *)(rt_ubase_t)interval_ms,
                                      1024, 12, 10);
    if (g_test_thread) {
        rt_thread_startup(g_test_thread);
    }
}
