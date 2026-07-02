/*
 * motor_test.c — X-axis motor debug console commands
 *
 * Usage (via finsh/msh on UART1 debug console):
 *   motor_pos  <position>   — set mX_position (OD 0x2002), triggers TPDO2
 *   motor_vel  <velocity>   — set mX_velocity (OD 0x2003), triggers TPDO2
 *   motor_mode <mode>       — set mX_modes    (OD 0x2001)
 *   motor_ctrl <ctrl_word>  — set mX_control_word (OD 0x2005)
 *   motor_stat              — read all X-axis OD variables
 */

#include <rtthread.h>
#include <stdlib.h>
#include "canopen_master.h"
#include "pdo.h"                 /* sendPDOevent() */

static int motor_pos(int argc, char **argv)
{
    if (argc < 2) {
        rt_kprintf("Usage: motor_pos <position>\n");
        rt_kprintf("  current mX_position = %ld\n", (long)mX_position);
        return 0;
    }
    INTEGER32 val = (INTEGER32)atol(argv[1]);
    UNS32 size = sizeof(INTEGER32);
    writeLocalDict(&Master_Data, 0x2002, 0x00, &val, &size, RW);
    sendPDOevent(&Master_Data);
    rt_kprintf("  mX_position (0x2002) = %ld\n", (long)val);
    return 0;
}
MSH_CMD_EXPORT(motor_pos, set X motor target position (OD 0x2002));

static int motor_vel(int argc, char **argv)
{
    if (argc < 2) {
        rt_kprintf("Usage: motor_vel <velocity>\n");
        rt_kprintf("  current mX_velocity = %ld\n", (long)mX_velocity);
        return 0;
    }
    INTEGER32 val = (INTEGER32)atol(argv[1]);
    UNS32 size = sizeof(INTEGER32);
    writeLocalDict(&Master_Data, 0x2003, 0x00, &val, &size, RW);
    sendPDOevent(&Master_Data);
    rt_kprintf("  mX_velocity (0x2003) = %ld\n", (long)val);
    return 0;
}
MSH_CMD_EXPORT(motor_vel, set X motor velocity (OD 0x2003));

static int motor_mode(int argc, char **argv)
{
    if (argc < 2) {
        rt_kprintf("Usage: motor_mode <mode>\n");
        rt_kprintf("  current mX_modes = %d\n", (int)mX_modes);
        rt_kprintf("  1=PP  3=PV  6=HM  8=CSP\n");
        return 0;
    }
    INTEGER8 val = (INTEGER8)atoi(argv[1]);
    UNS32 size = sizeof(INTEGER8);
    writeLocalDict(&Master_Data, 0x2001, 0x00, &val, &size, RW);
    sendPDOevent(&Master_Data);
    rt_kprintf("  mX_modes (0x2001) = %d\n", (int)val);
    return 0;
}
MSH_CMD_EXPORT(motor_mode, set X motor op mode (OD 0x2001));

static int motor_ctrl(int argc, char **argv)
{
    if (argc < 2) {
        rt_kprintf("Usage: motor_ctrl <control_word>\n");
        rt_kprintf("  current mX_control_word = 0x%04X\n", (unsigned)mX_control_word);
        rt_kprintf("  bits: 0=SwOn 1=EnVolt 2=QuickStop 3=EnOp 4=NewSP 5=ChgImm 6=Abs/Rel\n");
        return 0;
    }
    UNS16 val = (UNS16)strtoul(argv[1], NULL, 0);
    UNS32 size = sizeof(UNS16);
    writeLocalDict(&Master_Data, 0x2005, 0x00, &val, &size, RW);
    sendPDOevent(&Master_Data);
    rt_kprintf("  mX_control_word (0x2005) = 0x%04X\n", (unsigned)val);
    return 0;
}
MSH_CMD_EXPORT(motor_ctrl, set X motor control word (OD 0x2005));

static int motor_stat(int argc, char **argv)
{
    (void)argc; (void)argv;
    rt_kprintf("---- X-axis Motor Status ----\n");
    rt_kprintf("  mX_modes        (0x2001) = %d\n", (int)mX_modes);
    rt_kprintf("  mX_position     (0x2002) = %ld\n", (long)mX_position);
    rt_kprintf("  mX_velocity     (0x2003) = %ld\n", (long)mX_velocity);
    rt_kprintf("  mX_status_word  (0x2004) = 0x%04X\n", (unsigned)mX_status_word);
    rt_kprintf("  mX_control_word (0x2005) = 0x%04X\n", (unsigned)mX_control_word);
    rt_kprintf("  mX_Current_actual(0x2028)= %d\n", (int)mX_Current_actual);
    rt_kprintf("----------------------------\n");
    return 0;
}
MSH_CMD_EXPORT(motor_stat, read all X motor OD variables);
