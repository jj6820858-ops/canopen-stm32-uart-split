/* 避免 GCC14 newlib signal.h 与 RT-Thread libc_signal.h 冲突 */
#define _SIGNAL_H_

#include <rtthread.h>
#include <string.h>
#include "can_stm32.h"

/* HAL 需要明确芯片型号宏 */
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

/* CAN 诊断计数 */
volatile uint32_t g_can_tx_ok  = 0;
volatile uint32_t g_can_tx_err = 0;
volatile uint32_t g_can_rx_cnt = 0;

/* 将 CANfestival 波特率字符串转换为 HAL 分频参数。
 * APB1 时钟 = 36 MHz，目标波特率 250 Kbps:
 *   TQ = Prescaler / 36MHz = 9 / 36MHz = 0.25us
 *   BitTime = (1 + 11 + 4) * TQ = 16 * 0.25us = 4us
 *   BaudRate = 1 / 4us = 250 Kbps
 */
static void can_set_baudrate(char *baud)
{
    CAN_InitTypeDef *init = &hcan1.Init;

    /* 各波特率共用配置 */
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
        /* 默认 50K，PCLK1=36MHz */
        init->Prescaler = 60;
        init->TimeSeg1 = CAN_BS1_9TQ;
        init->TimeSeg2 = CAN_BS2_2TQ;
    }
}

void can_hardware_init(void)
{
    __HAL_RCC_CAN1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_AFIO_CLK_ENABLE();

    /* PA15 = CAN transceiver RS pin, pull low for normal mode */
    __HAL_AFIO_REMAP_SWJ_NOJTAG();  /* 释放 JTAG 占用的 PA15 */
    GPIO_InitTypeDef gpio_rs = {0};
    gpio_rs.Pin = GPIO_PIN_15;
    gpio_rs.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_rs.Pull = GPIO_NOPULL;
    gpio_rs.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &gpio_rs);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET);

    /* PA12=CAN_TX 输出，PA11=CAN_RX 输入 */
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_12;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);

    gpio.Pin = GPIO_PIN_11;
    gpio.Mode = GPIO_MODE_INPUT;       /* CAN_RX 必须配置为输入 */
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio);

    hcan1.Instance = CAN1;
    can_set_baudrate("250K");
    if (HAL_CAN_Init(&hcan1) != HAL_OK) {
        rt_kprintf("[CAN] Init FAILED! ErrorCode=0x%08lx\n", (unsigned long)hcan1.ErrorCode);
        return;
    }

    /* 配置过滤器 1: ID 掩码模式，32 位，主站接收全部标准帧。
     * STM32F103 的 CAN1 使用过滤器 0~13。
     * SlaveStartFilterBank=14 预留给 CAN2，本项目未使用 CAN2。 */
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

    /* 打开 CAN RX 中断 */
    HAL_NVIC_SetPriority(USB_LP_CAN1_RX0_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(USB_LP_CAN1_RX0_IRQn);

    /* CAN 测试命令已移到 tests/can_test.c，线程创建由 app_thread.c 统一管理。 */
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
    g_can_rx_cnt++;
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
        g_can_tx_err++;
        /* 尝试 Bus-Off 恢复 */
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
            /* 恢复后重试一次 */
            if (HAL_CAN_AddTxMessage(&hcan1, &tx_header, (uint8_t *)m->data,
                                     &tx_mailbox) == HAL_OK) {
                g_can_tx_ok++;
                return 0;   /* 成功 */
            }
        }
        return 1;   /* 失败 */
    }
    g_can_tx_ok++;
    return 0;   /* 成功 */
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

