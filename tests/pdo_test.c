/*
 * pdo_test.c - 全轴和外设 PDO 调试命令
 *
 * 覆盖 ObjDict.c 中配置的 TPDO/RPDO 映射。
 * 调试方式和 motor_test.c 一致: writeLocalDict 后触发 sendPDOevent。
 *
 * 用法: 在 UART1 的 finsh/msh 控制台执行:
 *   motorY_* / motorZ_* / motorE_* / motorT_*  控制各轴对象字典变量
 *   photo_ctl / photo_stat                     调试光度计 PDO
 *   temp_ctl / temp_target / temp_stat         调试温控 PDO
 *   weight_stat                                读取称重反馈
 *   pdo_stat                                   汇总显示全部 PDO 反馈变量
 */

#include <rtthread.h>
#include <stdlib.h>
#include "../applications/canopen/canopen_master.h"
#include "../canfestival/include/pdo.h"     /* 触发 PDO 发送 */

/* ═══════════════════════════════════════════════════════════════════
 *  Y 轴命令 (TPDO3: 0x1A02, TPDO4: 0x1A03, RPDO2: 0x1601)
 * ═══════════════════════════════════════════════════════════════════ */

static int motorY_mode(int argc, char **argv)
{
    if (argc < 2) {
        rt_kprintf("Usage: motorY_mode <mode>\n");
        rt_kprintf("  current mY_modes = %d\n", (int)mY_modes);
        rt_kprintf("  1=PP  3=PV  6=HM  8=CSP\n");
        return 0;
    }
    INTEGER8 val = (INTEGER8)atoi(argv[1]);
    UNS32 size = sizeof(INTEGER8);
    writeLocalDict(&Master_Data, 0x2006, 0x00, &val, &size, RW);
    sendPDOevent(&Master_Data);
    rt_kprintf("  mY_modes (0x2006) = %d  → TPDO3\n", (int)val);
    return 0;
}
MSH_CMD_EXPORT(motorY_mode, set Y motor op mode (OD 0x2006 → TPDO3));

static int motorY_ctrl(int argc, char **argv)
{
    if (argc < 2) {
        rt_kprintf("Usage: motorY_ctrl <control_word>\n");
        rt_kprintf("  current mY_control_word = 0x%04X\n", (unsigned)mY_control_word);
        rt_kprintf("  bits: 0=SwOn 1=EnVolt 2=QuickStop 3=EnOp 4=NewSP\n");
        return 0;
    }
    UNS16 val = (UNS16)strtoul(argv[1], NULL, 0);
    UNS32 size = sizeof(UNS16);
    writeLocalDict(&Master_Data, 0x200A, 0x00, &val, &size, RW);
    sendPDOevent(&Master_Data);
    rt_kprintf("  mY_control_word (0x200A) = 0x%04X  → TPDO3\n", (unsigned)val);
    return 0;
}
MSH_CMD_EXPORT(motorY_ctrl, set Y motor control word (OD 0x200A → TPDO3));

static int motorY_pos(int argc, char **argv)
{
    if (argc < 2) {
        rt_kprintf("Usage: motorY_pos <position>\n");
        rt_kprintf("  current mY_position = %ld\n", (long)mY_position);
        return 0;
    }
    INTEGER32 val = (INTEGER32)atol(argv[1]);
    UNS32 size = sizeof(INTEGER32);
    writeLocalDict(&Master_Data, 0x2007, 0x00, &val, &size, RW);
    sendPDOevent(&Master_Data);
    rt_kprintf("  mY_position (0x2007) = %ld  → TPDO4\n", (long)val);
    return 0;
}
MSH_CMD_EXPORT(motorY_pos, set Y motor target position (OD 0x2007 → TPDO4));

static int motorY_vel(int argc, char **argv)
{
    if (argc < 2) {
        rt_kprintf("Usage: motorY_vel <velocity>\n");
        rt_kprintf("  current mY_velocity = %ld\n", (long)mY_velocity);
        return 0;
    }
    INTEGER32 val = (INTEGER32)atol(argv[1]);
    UNS32 size = sizeof(INTEGER32);
    writeLocalDict(&Master_Data, 0x2008, 0x00, &val, &size, RW);
    sendPDOevent(&Master_Data);
    rt_kprintf("  mY_velocity (0x2008) = %ld  → TPDO4\n", (long)val);
    return 0;
}
MSH_CMD_EXPORT(motorY_vel, set Y motor velocity (OD 0x2008 → TPDO4));

static int motorY_stat(int argc, char **argv)
{
    (void)argc; (void)argv;
    rt_kprintf("---- Y-axis Motor Status (TPDO3/TPDO4, RPDO2) ----\n");
    rt_kprintf("  mY_modes        (0x2006) = %d\n", (int)mY_modes);
    rt_kprintf("  mY_position     (0x2007) = %ld\n", (long)mY_position);
    rt_kprintf("  mY_velocity     (0x2008) = %ld\n", (long)mY_velocity);
    rt_kprintf("  mY_status_word  (0x2009) = 0x%04X\n", (unsigned)mY_status_word);
    rt_kprintf("  mY_control_word (0x200A) = 0x%04X\n", (unsigned)mY_control_word);
    rt_kprintf("  mY_Current_actual(0x2029)= %d\n", (int)mY_Current_actual);
    rt_kprintf("----------------------------------------------------\n");
    return 0;
}
MSH_CMD_EXPORT(motorY_stat, read all Y motor OD variables);

/* ═══════════════════════════════════════════════════════════════════
 *  Z 轴命令 (TPDO5: 0x1A04, TPDO6: 0x1A05, RPDO3: 0x1602)
 * ═══════════════════════════════════════════════════════════════════ */

static int motorZ_mode(int argc, char **argv)
{
    if (argc < 2) {
        rt_kprintf("Usage: motorZ_mode <mode>\n");
        rt_kprintf("  current mZ_modes = %d\n", (int)mZ_modes);
        return 0;
    }
    INTEGER8 val = (INTEGER8)atoi(argv[1]);
    UNS32 size = sizeof(INTEGER8);
    writeLocalDict(&Master_Data, 0x200B, 0x00, &val, &size, RW);
    sendPDOevent(&Master_Data);
    rt_kprintf("  mZ_modes (0x200B) = %d  → TPDO5\n", (int)val);
    return 0;
}
MSH_CMD_EXPORT(motorZ_mode, set Z motor op mode (OD 0x200B → TPDO5));

static int motorZ_ctrl(int argc, char **argv)
{
    if (argc < 2) {
        rt_kprintf("Usage: motorZ_ctrl <control_word>\n");
        rt_kprintf("  current mZ_control_word = 0x%04X\n", (unsigned)mZ_control_word);
        return 0;
    }
    UNS16 val = (UNS16)strtoul(argv[1], NULL, 0);
    UNS32 size = sizeof(UNS16);
    writeLocalDict(&Master_Data, 0x200F, 0x00, &val, &size, RW);
    sendPDOevent(&Master_Data);
    rt_kprintf("  mZ_control_word (0x200F) = 0x%04X  → TPDO5\n", (unsigned)val);
    return 0;
}
MSH_CMD_EXPORT(motorZ_ctrl, set Z motor control word (OD 0x200F → TPDO5));

static int motorZ_pos(int argc, char **argv)
{
    if (argc < 2) {
        rt_kprintf("Usage: motorZ_pos <position>\n");
        rt_kprintf("  current mZ_position = %ld\n", (long)mZ_position);
        return 0;
    }
    INTEGER32 val = (INTEGER32)atol(argv[1]);
    UNS32 size = sizeof(INTEGER32);
    writeLocalDict(&Master_Data, 0x200C, 0x00, &val, &size, RW);
    sendPDOevent(&Master_Data);
    rt_kprintf("  mZ_position (0x200C) = %ld  → TPDO6\n", (long)val);
    return 0;
}
MSH_CMD_EXPORT(motorZ_pos, set Z motor target position (OD 0x200C → TPDO6));

static int motorZ_vel(int argc, char **argv)
{
    if (argc < 2) {
        rt_kprintf("Usage: motorZ_vel <velocity>\n");
        rt_kprintf("  current mZ_velocity = %ld\n", (long)mZ_velocity);
        return 0;
    }
    INTEGER32 val = (INTEGER32)atol(argv[1]);
    UNS32 size = sizeof(INTEGER32);
    writeLocalDict(&Master_Data, 0x200D, 0x00, &val, &size, RW);
    sendPDOevent(&Master_Data);
    rt_kprintf("  mZ_velocity (0x200D) = %ld  → TPDO6\n", (long)val);
    return 0;
}
MSH_CMD_EXPORT(motorZ_vel, set Z motor velocity (OD 0x200D → TPDO6));

static int motorZ_stat(int argc, char **argv)
{
    (void)argc; (void)argv;
    rt_kprintf("---- Z-axis Motor Status (TPDO5/TPDO6, RPDO3) ----\n");
    rt_kprintf("  mZ_modes        (0x200B) = %d\n", (int)mZ_modes);
    rt_kprintf("  mZ_position     (0x200C) = %ld\n", (long)mZ_position);
    rt_kprintf("  mZ_velocity     (0x200D) = %ld\n", (long)mZ_velocity);
    rt_kprintf("  mZ_status_word  (0x200E) = 0x%04X\n", (unsigned)mZ_status_word);
    rt_kprintf("  mZ_control_word (0x200F) = 0x%04X\n", (unsigned)mZ_control_word);
    rt_kprintf("  mZ_Current_actual(0x202A)= %d\n", (int)mZ_Current_actual);
    rt_kprintf("----------------------------------------------------\n");
    return 0;
}
MSH_CMD_EXPORT(motorZ_stat, read all Z motor OD variables);

/* ═══════════════════════════════════════════════════════════════════
 *  E 轴命令 (TPDO7: 0x1A06, TPDO8: 0x1A07, RPDO4: 0x1603)
 *  E = 升降臂
 * ═══════════════════════════════════════════════════════════════════ */

static int motorE_mode(int argc, char **argv)
{
    if (argc < 2) {
        rt_kprintf("Usage: motorE_mode <mode>\n");
        rt_kprintf("  current mE_modes = %d\n", (int)mE_modes);
        return 0;
    }
    INTEGER8 val = (INTEGER8)atoi(argv[1]);
    UNS32 size = sizeof(INTEGER8);
    writeLocalDict(&Master_Data, 0x2010, 0x00, &val, &size, RW);
    sendPDOevent(&Master_Data);
    rt_kprintf("  mE_modes (0x2010) = %d  → TPDO7\n", (int)val);
    return 0;
}
MSH_CMD_EXPORT(motorE_mode, set E motor (lift) op mode (OD 0x2010 → TPDO7));

static int motorE_ctrl(int argc, char **argv)
{
    if (argc < 2) {
        rt_kprintf("Usage: motorE_ctrl <control_word>\n");
        rt_kprintf("  current mE_control_word = 0x%04X\n", (unsigned)mE_control_word);
        return 0;
    }
    UNS16 val = (UNS16)strtoul(argv[1], NULL, 0);
    UNS32 size = sizeof(UNS16);
    writeLocalDict(&Master_Data, 0x2014, 0x00, &val, &size, RW);
    sendPDOevent(&Master_Data);
    rt_kprintf("  mE_control_word (0x2014) = 0x%04X  → TPDO7\n", (unsigned)val);
    return 0;
}
MSH_CMD_EXPORT(motorE_ctrl, set E motor (lift) control word (OD 0x2014 → TPDO7));

static int motorE_pos(int argc, char **argv)
{
    if (argc < 2) {
        rt_kprintf("Usage: motorE_pos <position>\n");
        rt_kprintf("  current mE_position = %ld\n", (long)mE_position);
        return 0;
    }
    INTEGER32 val = (INTEGER32)atol(argv[1]);
    UNS32 size = sizeof(INTEGER32);
    writeLocalDict(&Master_Data, 0x2011, 0x00, &val, &size, RW);
    sendPDOevent(&Master_Data);
    rt_kprintf("  mE_position (0x2011) = %ld  → TPDO8\n", (long)val);
    return 0;
}
MSH_CMD_EXPORT(motorE_pos, set E motor (lift) position (OD 0x2011 → TPDO8));

static int motorE_vel(int argc, char **argv)
{
    if (argc < 2) {
        rt_kprintf("Usage: motorE_vel <velocity>\n");
        rt_kprintf("  current mE_velocity = %ld\n", (long)mE_velocity);
        return 0;
    }
    INTEGER32 val = (INTEGER32)atol(argv[1]);
    UNS32 size = sizeof(INTEGER32);
    writeLocalDict(&Master_Data, 0x2012, 0x00, &val, &size, RW);
    sendPDOevent(&Master_Data);
    rt_kprintf("  mE_velocity (0x2012) = %ld  → TPDO8\n", (long)val);
    return 0;
}
MSH_CMD_EXPORT(motorE_vel, set E motor (lift) velocity (OD 0x2012 → TPDO8));

static int motorE_stat(int argc, char **argv)
{
    (void)argc; (void)argv;
    rt_kprintf("---- E-axis (Lift) Motor Status (TPDO7/TPDO8, RPDO4) ----\n");
    rt_kprintf("  mE_modes        (0x2010) = %d\n", (int)mE_modes);
    rt_kprintf("  mE_position     (0x2011) = %ld\n", (long)mE_position);
    rt_kprintf("  mE_velocity     (0x2012) = %ld\n", (long)mE_velocity);
    rt_kprintf("  mE_status_word  (0x2013) = 0x%04X\n", (unsigned)mE_status_word);
    rt_kprintf("  mE_control_word (0x2014) = 0x%04X\n", (unsigned)mE_control_word);
    rt_kprintf("  mB_Current_actual(0x202B)= %d  (shared on RPDO4)\n", (int)mB_Current_actual);
    rt_kprintf("----------------------------------------------------------\n");
    return 0;
}
MSH_CMD_EXPORT(motorE_stat, read all E motor (lift) OD variables);

/* ═══════════════════════════════════════════════════════════════════
 *  T 轴命令 (TPDO9: 0x1A08, TPDO10: 0x1A09, RPDO5: 0x1604)
 *  T = 转盘
 * ═══════════════════════════════════════════════════════════════════ */

static int motorT_mode(int argc, char **argv)
{
    if (argc < 2) {
        rt_kprintf("Usage: motorT_mode <mode>\n");
        rt_kprintf("  current mT_modes = %d\n", (int)mT_modes);
        return 0;
    }
    INTEGER8 val = (INTEGER8)atoi(argv[1]);
    UNS32 size = sizeof(INTEGER8);
    writeLocalDict(&Master_Data, 0x2015, 0x00, &val, &size, RW);
    sendPDOevent(&Master_Data);
    rt_kprintf("  mT_modes (0x2015) = %d  → TPDO9\n", (int)val);
    return 0;
}
MSH_CMD_EXPORT(motorT_mode, set T motor (turntable) op mode (OD 0x2015 → TPDO9));

static int motorT_ctrl(int argc, char **argv)
{
    if (argc < 2) {
        rt_kprintf("Usage: motorT_ctrl <control_word>\n");
        rt_kprintf("  current mT_control_word = 0x%04X\n", (unsigned)mT_control_word);
        return 0;
    }
    UNS16 val = (UNS16)strtoul(argv[1], NULL, 0);
    UNS32 size = sizeof(UNS16);
    writeLocalDict(&Master_Data, 0x2019, 0x00, &val, &size, RW);
    sendPDOevent(&Master_Data);
    rt_kprintf("  mT_control_word (0x2019) = 0x%04X  → TPDO9\n", (unsigned)val);
    return 0;
}
MSH_CMD_EXPORT(motorT_ctrl, set T motor (turntable) control word (OD 0x2019 → TPDO9));

static int motorT_pos(int argc, char **argv)
{
    if (argc < 2) {
        rt_kprintf("Usage: motorT_pos <position>\n");
        rt_kprintf("  current mT_position = %ld\n", (long)mT_position);
        return 0;
    }
    INTEGER32 val = (INTEGER32)atol(argv[1]);
    UNS32 size = sizeof(INTEGER32);
    writeLocalDict(&Master_Data, 0x2016, 0x00, &val, &size, RW);
    sendPDOevent(&Master_Data);
    rt_kprintf("  mT_position (0x2016) = %ld  → TPDO10\n", (long)val);
    return 0;
}
MSH_CMD_EXPORT(motorT_pos, set T motor (turntable) position (OD 0x2016 → TPDO10));

static int motorT_vel(int argc, char **argv)
{
    if (argc < 2) {
        rt_kprintf("Usage: motorT_vel <velocity>\n");
        rt_kprintf("  current mT_velocity = %ld\n", (long)mT_velocity);
        return 0;
    }
    INTEGER32 val = (INTEGER32)atol(argv[1]);
    UNS32 size = sizeof(INTEGER32);
    writeLocalDict(&Master_Data, 0x2017, 0x00, &val, &size, RW);
    sendPDOevent(&Master_Data);
    rt_kprintf("  mT_velocity (0x2017) = %ld  → TPDO10\n", (long)val);
    return 0;
}
MSH_CMD_EXPORT(motorT_vel, set T motor (turntable) velocity (OD 0x2017 → TPDO10));

static int motorT_stat(int argc, char **argv)
{
    (void)argc; (void)argv;
    rt_kprintf("---- T-axis (Turntable) Status (TPDO9/TPDO10, RPDO5) ----\n");
    rt_kprintf("  mT_modes        (0x2015) = %d\n", (int)mT_modes);
    rt_kprintf("  mT_position     (0x2016) = %ld\n", (long)mT_position);
    rt_kprintf("  mT_velocity     (0x2017) = %ld\n", (long)mT_velocity);
    rt_kprintf("  mT_status_word  (0x2018) = 0x%04X\n", (unsigned)mT_status_word);
    rt_kprintf("  mT_control_word (0x2019) = 0x%04X\n", (unsigned)mT_control_word);
    rt_kprintf("---------------------------------------------------------\n");
    return 0;
}
MSH_CMD_EXPORT(motorT_stat, read all T motor (turntable) OD variables);

/* ═══════════════════════════════════════════════════════════════════
 *  B 轴状态 (RPDO4 与 E 轴共用 mB_Current_actual)
 * ═══════════════════════════════════════════════════════════════════ */

static int motorB_stat(int argc, char **argv)
{
    (void)argc; (void)argv;
    rt_kprintf("---- B-axis Current (RPDO4, shared with E) ----\n");
    rt_kprintf("  mB_Current_actual(0x202B)= %d\n", (int)mB_Current_actual);
    rt_kprintf("----------------------------------------------\n");
    return 0;
}
MSH_CMD_EXPORT(motorB_stat, read B motor current (OD 0x202B, RPDO4));

/* ═══════════════════════════════════════════════════════════════════
 *  光度计命令 (TPDO11: 0x1A0A, TPDO14: 0x1A0D, RPDO6: 0x1605)
 * ═══════════════════════════════════════════════════════════════════ */

static int photo_ctl(int argc, char **argv)
{
    if (argc < 2) {
        rt_kprintf("Usage: photo_ctl <led> [rate] [gain]\n");
        rt_kprintf("  current photometer_led  (0x201C) = %lu\n", (unsigned long)photometer_led);
        rt_kprintf("  current photometer_rate (0x2022) = %u\n", (unsigned)photometer_rate);
        rt_kprintf("  current photometer_gain (0x2023) = %u\n", (unsigned)photometer_gain);
        rt_kprintf("  Sets LED (TPDO11) and optionally rate+gain (TPDO14)\n");
        return 0;
    }
    UNS32 val = (UNS32)strtoul(argv[1], NULL, 0);
    UNS32 size = sizeof(UNS32);
    writeLocalDict(&Master_Data, 0x201C, 0x00, &val, &size, RW);
    sendPDOevent(&Master_Data);
    rt_kprintf("  photometer_led (0x201C) = %lu  → TPDO11\n", (unsigned long)val);

    if (argc >= 3) {
        UNS16 rate = (UNS16)strtoul(argv[2], NULL, 0);
        UNS32 rs = sizeof(UNS16);
        writeLocalDict(&Master_Data, 0x2022, 0x00, &rate, &rs, RW);
        rt_kprintf("  photometer_rate (0x2022) = %u  → TPDO14\n", (unsigned)rate);
    }
    if (argc >= 4) {
        UNS8 gain = (UNS8)strtoul(argv[3], NULL, 0);
        UNS32 gs = sizeof(UNS8);
        writeLocalDict(&Master_Data, 0x2023, 0x00, &gain, &gs, RW);
        rt_kprintf("  photometer_gain (0x2023) = %u  → TPDO14\n", (unsigned)gain);
    }
    if (argc >= 3) sendPDOevent(&Master_Data);
    return 0;
}
MSH_CMD_EXPORT(photo_ctl, set photometer LED/rate/gain (TPDO11+TPDO14));

static int photo_stat(int argc, char **argv)
{
    (void)argc; (void)argv;
    rt_kprintf("---- Photometer Status (TPDO11/TPDO14, RPDO6) ----\n");
    rt_kprintf("  photometer_ch0  (0x201A) = %u\n", (unsigned)photometer_ch0);
    rt_kprintf("  photometer_ch1  (0x201B) = %u\n", (unsigned)photometer_ch1);
    rt_kprintf("  photometer_led  (0x201C) = %lu\n", (unsigned long)photometer_led);
    rt_kprintf("  photometer_rate (0x2022) = %u\n", (unsigned)photometer_rate);
    rt_kprintf("  photometer_gain (0x2023) = %u\n", (unsigned)photometer_gain);
    rt_kprintf("--------------------------------------------------\n");
    return 0;
}
MSH_CMD_EXPORT(photo_stat, read all photometer OD variables);

/* ═══════════════════════════════════════════════════════════════════
 *  温控命令 (TPDO12: 0x1A0B, TPDO13: 0x1A0C, RPDO7/8: 0x1606/07)
 * ═══════════════════════════════════════════════════════════════════ */

static int temp_ctl(int argc, char **argv)
{
    if (argc < 2) {
        rt_kprintf("Usage: temp_ctl <control_word>\n");
        rt_kprintf("  current TEMP_control_word (0x201D) = 0x%08lX\n", (unsigned long)TEMP_control_word);
        rt_kprintf("  Sends to TPDO13\n");
        return 0;
    }
    UNS32 val = (UNS32)strtoul(argv[1], NULL, 0);
    UNS32 size = sizeof(UNS32);
    writeLocalDict(&Master_Data, 0x201D, 0x00, &val, &size, RW);
    sendPDOevent(&Master_Data);
    rt_kprintf("  TEMP_control_word (0x201D) = 0x%08lX  → TPDO13\n", (unsigned long)val);
    return 0;
}
MSH_CMD_EXPORT(temp_ctl, set TEMP control word (OD 0x201D → TPDO13));

static int temp_target(int argc, char **argv)
{
    if (argc < 3) {
        rt_kprintf("Usage: temp_target <heat> <refrig>\n");
        rt_kprintf("  current heating_target      (0x201E) = %lu\n", (unsigned long)heating_target);
        rt_kprintf("  current refrigeration_target(0x201F) = %lu\n", (unsigned long)refrigeration_target);
        rt_kprintf("  Values: x100 (e.g. 2500 = 25.00°C). Sends to TPDO12\n");
        return 0;
    }
    UNS32 heat = (UNS32)strtoul(argv[1], NULL, 0);
    UNS32 refr = (UNS32)strtoul(argv[2], NULL, 0);
    UNS32 size = sizeof(UNS32);

    writeLocalDict(&Master_Data, 0x201E, 0x00, &heat, &size, RW);
    writeLocalDict(&Master_Data, 0x201F, 0x00, &refr, &size, RW);
    sendPDOevent(&Master_Data);
    rt_kprintf("  heating_target (0x201E) = %lu  → TPDO12\n", (unsigned long)heat);
    rt_kprintf("  refrigeration_target (0x201F) = %lu  → TPDO12\n", (unsigned long)refr);
    return 0;
}
MSH_CMD_EXPORT(temp_target, set heating + refrigeration targets (OD 0x201E/1F → TPDO12));

static int temp_stat(int argc, char **argv)
{
    (void)argc; (void)argv;
    rt_kprintf("---- Temperature Status (TPDO12/TPDO13, RPDO7/RPDO8) ----\n");
    rt_kprintf("  TEMP_control_word    (0x201D) = 0x%08lX\n", (unsigned long)TEMP_control_word);
    rt_kprintf("  heating_target       (0x201E) = %lu\n", (unsigned long)heating_target);
    rt_kprintf("  refrigeration_target (0x201F) = %lu\n", (unsigned long)refrigeration_target);
    rt_kprintf("  current_heating      (0x2020) = %lu  (RPDO7)\n", (unsigned long)current_heating);
    rt_kprintf("  current_refrigeration(0x2021) = %lu  (RPDO7)\n", (unsigned long)current_refrigeration);
    rt_kprintf("  TEMP_status_word     (0x2024) = 0x%08lX  (RPDO8)\n", (unsigned long)TEMP_status_word);
    rt_kprintf("---------------------------------------------------------\n");
    return 0;
}
MSH_CMD_EXPORT(temp_stat, read all temperature OD variables);

/* ═══════════════════════════════════════════════════════════════════
 *  称重命令 (RPDO9/RPDO10: 0x1608/09)
 *  没有 TPDO，重量数据只从从站读取
 * ═══════════════════════════════════════════════════════════════════ */

static int weight_stat(int argc, char **argv)
{
    (void)argc; (void)argv;
    rt_kprintf("---- Weight Status (RPDO9/RPDO10) ----\n");
    rt_kprintf("  weight_clean_water (0x2025) = %ld g\n", (long)weight_clean_water);
    rt_kprintf("  weight_buff_liq    (0x2026) = %ld g\n", (long)weight_buff_liq);
    rt_kprintf("  weight_waste_liq   (0x2027) = %ld g\n", (long)weight_waste_liq);
    rt_kprintf("-------------------------------------\n");
    return 0;
}
MSH_CMD_EXPORT(weight_stat, read all weight OD variables);

/* ═══════════════════════════════════════════════════════════════════
 *  主站汇总: 一次性显示全部 PDO 反馈变量
 * ═══════════════════════════════════════════════════════════════════ */

static int pdo_stat(int argc, char **argv)
{
    (void)argc; (void)argv;
    rt_kprintf("╔════════════════════════════════════════════════════════╗\n");
    rt_kprintf("║        CANopen Master — Complete PDO Status          ║\n");
    rt_kprintf("╚════════════════════════════════════════════════════════╝\n");

    /* ── Axes control & status ── */
    rt_kprintf("\n── Axes Control (TPDO) ──\n");
    rt_kprintf("%-4s %-12s %-12s %-12s %-16s\n",
               "Axis", "Modes", "Position", "Velocity", "ControlWord");
    rt_kprintf("---- ------------ ------------ ------------ ----------------\n");
    rt_kprintf("X    %-12d %-12ld %-12ld 0x%04X\n",
               (int)mX_modes, (long)mX_position, (long)mX_velocity, (unsigned)mX_control_word);
    rt_kprintf("Y    %-12d %-12ld %-12ld 0x%04X\n",
               (int)mY_modes, (long)mY_position, (long)mY_velocity, (unsigned)mY_control_word);
    rt_kprintf("Z    %-12d %-12ld %-12ld 0x%04X\n",
               (int)mZ_modes, (long)mZ_position, (long)mZ_velocity, (unsigned)mZ_control_word);
    rt_kprintf("E    %-12d %-12ld %-12ld 0x%04X\n",
               (int)mE_modes, (long)mE_position, (long)mE_velocity, (unsigned)mE_control_word);
    rt_kprintf("T    %-12d %-12ld %-12ld 0x%04X\n",
               (int)mT_modes, (long)mT_position, (long)mT_velocity, (unsigned)mT_control_word);

    /* ── Axes feedback ── */
    rt_kprintf("\n── Axes Feedback (RPDO) ──\n");
    rt_kprintf("%-4s %-18s %-16s\n", "Axis", "StatusWord", "CurrentActual");
    rt_kprintf("---- ------------------ ----------------\n");
    rt_kprintf("X    0x%04X           %-12d\n", (unsigned)mX_status_word, (int)mX_Current_actual);
    rt_kprintf("Y    0x%04X           %-12d\n", (unsigned)mY_status_word, (int)mY_Current_actual);
    rt_kprintf("Z    0x%04X           %-12d\n", (unsigned)mZ_status_word, (int)mZ_Current_actual);
    rt_kprintf("E    0x%04X           %-12d\n", (unsigned)mE_status_word, (int)mE_modes);
    rt_kprintf("T    0x%04X           %-12d\n", (unsigned)mT_status_word, (int)mT_modes);
    rt_kprintf("B    —                %-12d  (via RPDO4 with E)\n", (int)mB_Current_actual);

    /* ── Sensors ── */
    rt_kprintf("\n── Photometer (RPDO6, TPDO11/14) ──\n");
    rt_kprintf("  Ch0=%u  Ch1=%u  LED=%lu  Rate=%u  Gain=%u\n",
               (unsigned)photometer_ch0, (unsigned)photometer_ch1,
               (unsigned long)photometer_led,
               (unsigned)photometer_rate, (unsigned)photometer_gain);

    rt_kprintf("\n── Temperature (RPDO7/8, TPDO12/13) ──\n");
    rt_kprintf("  Ctrl=0x%08lX  Target H=%lu / R=%lu  Current H=%lu / R=%lu  Status=0x%08lX\n",
               (unsigned long)TEMP_control_word,
               (unsigned long)heating_target, (unsigned long)refrigeration_target,
               (unsigned long)current_heating, (unsigned long)current_refrigeration,
               (unsigned long)TEMP_status_word);

    rt_kprintf("\n── Weight (RPDO9/10, read-only) ──\n");
    rt_kprintf("  CleanWater=%ldg  BuffLiq=%ldg  WasteLiq=%ldg\n",
               (long)weight_clean_water, (long)weight_buff_liq, (long)weight_waste_liq);

    rt_kprintf("\n");
    return 0;
}
MSH_CMD_EXPORT(pdo_stat, dump all PDO status axes sensors weights);
