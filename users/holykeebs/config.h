#pragma once

/* Reset. */
#define RP2040_BOOTLOADER_DOUBLE_TAP_RESET
#define RP2040_BOOTLOADER_DOUBLE_TAP_RESET_TIMEOUT 1000U

#define MOUSE_EXTENDED_REPORT
#define WHEEL_EXTENDED_REPORT

#ifdef HK_HIRES_SCROLL
#    define POINTING_DEVICE_HIRES_SCROLL_ENABLE
#endif

// #define POINTING_DEVICE_AUTO_MOUSE_ENABLE
// #define AUTO_MOUSE_DEFAULT_LAYER 4

#undef PRINTF_SUPPORT_DECIMAL_SPECIFIERS
#define PRINTF_SUPPORT_DECIMAL_SPECIFIERS 1

#define EECONFIG_USER_DATA_SIZE 64

// Schema version for the settings block (hk_eeprom_config_t in
// eeprom_config.h): bump it whenever that layout changes, so old saves are
// discarded instead of being deserialized as garbage.
//
// QMK stores this alongside the block but outside it, and
// eeconfig_is_user_datablock_valid() compares it against what's in flash, so a
// layout change can't move the version that's supposed to describe the layout.
// It lives here rather than next to the struct because core reads it at config
// time; without it the stored value defaults to EECONFIG_USER_DATA_SIZE, which
// never changes and so never invalidates anything.
#define EECONFIG_USER_DATA_VERSION 104

#undef MASTER_LEFT
#undef EE_HANDS
#undef MASTER_RIGHT

#ifdef SPLIT_KEYBOARD
    // Handedness + master selection.
    //
    // Boards without their own hand detection infer the hand from a forced
    // master side: the master (whichever half has USB) is pinned to one hand, so
    // USB must always go to that half. Most holykeebs boards work this way.
    //
    // Boards that bring their own hand detection (e.g. keyball61plus's
    // HK_HAND_FROM_POINTER sensor probe, or a SPLIT_HAND_MATRIX_GRID /
    // SPLIT_HAND_PIN circuit) resolve the hand at runtime and the master from
    // USB/VBUS independently, so no master side is pinned and USB can go to
    // either half. Don't force one here: it would be ignored anyway (the
    // board's detection wins in is_keyboard_left_impl) but is misleading.
    #if !defined(SPLIT_HAND_MATRIX_GRID) && !defined(SPLIT_HAND_PIN) && !defined(HK_HAND_FROM_POINTER)
        #if defined(HK_MASTER_LEFT)
            #define MASTER_LEFT
        #elif defined(HK_MASTER_RIGHT)
            #define MASTER_RIGHT
        #else
            #error "HK_MASTER_LEFT or HK_MASTER_RIGHT not defined in rules.mk"
        #endif
    #endif

    #define SERIAL_USART_TX_PIN GP1

    // This helps in setups where the keyboard isn't recognized on boot if it's already plugged in.
    #define SPLIT_WATCHDOG_ENABLE
    #define SPLIT_WATCHDOG_TIMEOUT 3000

    #define USB_VBUS_PIN GP19
#endif

#ifdef HK_POINTING_DEVICE_RIGHT_PIMORONI
    #ifdef POINTING_DEVICE_COMBINED
        #define POINTING_DEVICE_ROTATION_90_RIGHT
    #else
        #define POINTING_DEVICE_ROTATION_90
    #endif

    // Not needed due to the block below, but it's untested for Cirque since they've been retired.
    // #if defined(HK_POINTING_DEVICE_LEFT_CIRQUE40) || defined(HK_POINTING_DEVICE_LEFT_CIRQUE35)
    //     // The pimoroni needs frequest updates, set a value so the Cirque configs don't set it to 10ms.
    //     #define POINTING_DEVICE_TASK_THROTTLE_MS 1
    // #endif
#endif
#ifdef HK_POINTING_DEVICE_LEFT_PIMORONI
    #define POINTING_DEVICE_ROTATION_270

    // Not needed due to the block below, but it's untested for Cirque since they've been retired.
    // #if defined(HK_POINTING_DEVICE_RIGHT_CIRQUE40) || defined(HK_POINTING_DEVICE_RIGHT_CIRQUE35)
    //     // The pimoroni needs frequest updates, set a value so the Cirque configs don't set it to 10ms.
    //     #define POINTING_DEVICE_TASK_THROTTLE_MS 1
    // #endif
#endif

#if defined(HK_POINTING_DEVICE_RIGHT_PIMORONI) || defined(HK_POINTING_DEVICE_LEFT_PIMORONI)
    // Check if we also have an Azoteq or Cirque, since those throttle.
    //
    // Checking these defines as opposed to the HK_ ones since there are on only when the side with the pointing device is
    // flashed. We don't want to set the define below for the pimoroni/trackpoint side.
    #if defined(POINTING_DEVICE_DRIVER_azoteq_iqs5xx) || defined(POINTING_DEVICE_DRIVER_cirque_pinnacle_i2c)
        // This tells the pointing device implementation to throttle the call to get_report, instead of throttling the task.
        //
        // This is needed because some pointing devices don't like being polled too often (Cirque/TPS) and throttling
        // the task causes the peripheral pointing device to also be throttled, which is detrimental to some pointing
        // devices like the pimoroni and trackpoint.
        #define POINTING_DEVICE_TASK_THROTTLE_MS 1
    #endif
#endif

#ifdef HK_POINTING_DEVICE_LEFT_CIRQUE40
    #define POINTING_DEVICE_ROTATION_180
#endif

// PMW3360 optical trackball (keyball61plus). The sensor is mounted rotated, so
// fix up its axes here. Built dual + combined, so the left half is the
// non-suffixed (main-path) device and the right half the _RIGHT device. SPI
// wiring stays in the board config.h (holykeebs devices are otherwise I2C/PS2).
#ifdef HK_POINTING_DEVICE_LEFT_PMW3360
    #define POINTING_DEVICE_ROTATION_90
    #define POINTING_DEVICE_INVERT_Y
#endif
#ifdef HK_POINTING_DEVICE_RIGHT_PMW3360
    #define POINTING_DEVICE_ROTATION_270_RIGHT
    #define POINTING_DEVICE_INVERT_Y_RIGHT
#endif

// Generic Cirque configuration.
#ifdef POINTING_DEVICE_DRIVER_cirque_pinnacle_i2c
    #if defined(HK_POINTING_DEVICE_LEFT_CIRQUE35) || defined(HK_POINTING_DEVICE_RIGHT_CIRQUE35)
        #define CIRQUE_PINNACLE_DIAMETER_MM 35
    #elif defined(HK_POINTING_DEVICE_LEFT_CIRQUE40) || defined(HK_POINTING_DEVICE_RIGHT_CIRQUE40)
        #define CIRQUE_PINNACLE_DIAMETER_MM 40
    #else
        #error "Unknown Cirque configuration."
    #endif

    // Tap for left click.
    #define CIRQUE_PINNACLE_TAP_ENABLE
    // Corner tap for right click.
    // #define CIRQUE_PINNACLE_SECONDARY_TAP_ENABLE // Unfortunatly this doesn't seem to work.
    // Enable circular scroll.
    #define POINTING_DEVICE_GESTURES_SCROLL_ENABLE
#endif

#ifdef POINTING_DEVICE_DRIVER_azoteq_iqs5xx
    #if defined(HK_POINTING_DEVICE_RIGHT_TPS43) || defined(HK_POINTING_DEVICE_LEFT_TPS43)
        #define AZOTEQ_IQS5XX_TPS43
    #endif

    #if defined(HK_POINTING_DEVICE_MIDDLE_TPS65)
        #define AZOTEQ_IQS5XX_TPS65
        #define AZOTEQ_IQS5XX_ROTATION_270
    #endif

    #if defined(HK_POINTING_DEVICE_RIGHT_TPS43)
        // We define the rotation in rules.mk in case we have a TPS43 on both sides. This is because we need to know
        // which side we're flashing for, and that information is lost outside of rules.mk.
        #if !defined(HK_POINTING_DEVICE_LEFT_TPS43)
            #define AZOTEQ_IQS5XX_ROTATION_180
        #endif
    #endif

    // These are enabled by default.
    // #define AZOTEQ_IQS5XX_TAP_ENABLE true
    // #define AZOTEQ_IQS5XX_TWO_FINGER_TAP_ENABLE true
    // #define AZOTEQ_IQS5XX_SCROLL_ENABLE true
    #define AZOTEQ_IQS5XX_PRESS_AND_HOLD_ENABLE true
#endif

#ifdef POINTING_DEVICE_DRIVER_ps2
    // The default streaming mode is flaky and doesn't always work. This setting doesn't seem to affect performance.
    #define PS2_MOUSE_USE_REMOTE_MODE

    // Serial uses PIO0, change PS2 to PIO1.
    #define PS2_PIO_USE_PIO1

    #define PS2_MOUSE_INVERT_X
    #define PS2_MOUSE_INVERT_Y

    #define PS2_DATA_PIN    GP2
    #define PS2_CLOCK_PIN   GP3

    // Start faster when keyboard resets.
    #define PS2_MOUSE_INIT_DELAY 500
#endif

#if defined(OLED_ENABLE) && defined(SPLIT_KEYBOARD)
    #define HK_SPLIT_SYNC_STATE
#endif

// keyball61plus is built dual-PMW3360 combined, but may be physically populated
// with a ball on one side, the other, or both. The PMW3360 self-reports init
// success, so the installed sides are detected at runtime (one image covers
// every population) rather than chosen at build time. The detected combination
// is handed to the keymap via hk_pointing_devices_detected_keymap (e.g. to drive
// the VIA "Ball availability" layout option).
#if defined(HK_POINTING_DEVICE_LEFT_PMW3360) && defined(HK_POINTING_DEVICE_RIGHT_PMW3360)
    #define HK_SPLIT_DETECT_POINTING
#endif

#ifdef HK_OLED_ENABLE
#    define OLED_FONT_H "users/holykeebs/logofont/logofont.c"
#    define OLED_FONT_START 32
#    define OLED_FONT_END 195
#endif

#ifdef HK_SPLIT_SYNC_STATE
    #define SPLIT_LAYER_STATE_ENABLE
    #define RPC_M2S_BUFFER_SIZE 64
#endif

// Assemble the user split transaction id list from whichever split features are
// enabled. HK_SYNC_STATE (master->peripheral state sync, OLED) and
// HK_GET_POINTING_INFO (peripheral->master pointing presence, ball detection)
// are independent, so any combination is valid.
#if defined(HK_SPLIT_SYNC_STATE) && defined(HK_SPLIT_DETECT_POINTING)
    #define SPLIT_TRANSACTION_IDS_USER HK_SYNC_STATE, HK_GET_POINTING_INFO
#elif defined(HK_SPLIT_SYNC_STATE)
    #define SPLIT_TRANSACTION_IDS_USER HK_SYNC_STATE
#elif defined(HK_SPLIT_DETECT_POINTING)
    #define SPLIT_TRANSACTION_IDS_USER HK_GET_POINTING_INFO
#endif

// Keep the key/matrix tester always available, accepting the keylogger exposure
// that implies. Each fork honors only the token for the protocol it compiles:
// VIA_INSECURE restores the matrix readout disabled by qmk/qmk_firmware#25414;
// VIAL_INSECURE skips Vial's unlock combo. The other is an inert, unused define.
// TODO: consider the safer unlock-based approach instead (SECURE_ENABLE on VIA,
// the VIAL_UNLOCK_COMBO on Vial).
#define VIA_INSECURE
#define VIAL_INSECURE

#define SPLIT_MODS_ENABLE       // show held modifiers on the half without USB
#define OLED_TIMEOUT 60000      // screens go dark after 60 s without keys, wheels or ball
#define SPLIT_ACTIVITY_ENABLE   // the half without USB follows activity on both halves