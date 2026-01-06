#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <stdint.h>
#include <string.h>

typedef struct {
    uint8_t  crash_flag;
    uint8_t  crash_reason;
    uint16_t stack_pointer_val;
    uint8_t  mcusr_mirror;
} CrashReport_t;

#define CRASH_FLAG_SET      0xDE
#define CRASH_FLAG_CLEAR    0x00

typedef enum {
    CRASH_REASON_NONE       = 0x00,
    CRASH_REASON_WDT        = 0x01,
    CRASH_REASON_EXPLICIT   = 0x02,
    CRASH_REASON_STACK_OVF  = 0x03,
    CRASH_REASON_UNKNOWN    = 0xFF
} CrashReason_e;

#define EEPROM_CRASH_REPORT_ADDR 0x0000

static volatile CrashReport_t g_crash_report;

#define BAUD_RATE 9600
#define UBRR_VALUE ((F_CPU / (16UL * BAUD_RATE)) - 1)

void UART_init(void) {
    UBRR0H = (uint8_t)(UBRR_VALUE >> 8);
    UBRR0L = (uint8_t)(UBRR_VALUE);
    UCSR0B = (1 << TXEN0) | (1 << RXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void UART_transmit(uint8_t data) {
    while (!(UCSR0A & (1 << UDRE0))) {
    }
    UDR0 = data;
}

uint8_t UART_receive(void) {
    while (!(UCSR0A & (1 << RXC0))) {
    }
    return UDR0;
}

uint8_t UART_available(void) {
    return (UCSR0A & (1 << RXC0)) ? 1 : 0;
}

void UART_flush(void) {
    uint8_t dummy;
    while (UCSR0A & (1 << RXC0)) {
        dummy = UDR0;
        (void)dummy;
    }
    _delay_ms(10);
}

void UART_print(const char *str) {
    while (*str) {
        UART_transmit(*str++);
    }
}

void UART_println(const char *str) {
    UART_print(str);
    UART_transmit('\r');
    UART_transmit('\n');
}

void UART_print_hex8(uint8_t val) {
    const char hex_chars[] = "0123456789ABCDEF";
    UART_print("0x");
    UART_transmit(hex_chars[(val >> 4) & 0x0F]);
    UART_transmit(hex_chars[val & 0x0F]);
}

void UART_print_hex16(uint16_t val) {
    UART_print("0x");
    const char hex_chars[] = "0123456789ABCDEF";
    UART_transmit(hex_chars[(val >> 12) & 0x0F]);
    UART_transmit(hex_chars[(val >> 8) & 0x0F]);
    UART_transmit(hex_chars[(val >> 4) & 0x0F]);
    UART_transmit(hex_chars[val & 0x0F]);
}

void EEPROM_write(uint16_t address, uint8_t data) {
    while (EECR & (1 << EEPE)) {
    }
    EEARH = (uint8_t)(address >> 8);
    EEARL = (uint8_t)(address);
    EEDR = data;
    cli();
    EECR |= (1 << EEMPE);
    EECR |= (1 << EEPE);
    sei();
}

uint8_t EEPROM_read(uint16_t address) {
    while (EECR & (1 << EEPE)) {
    }
    EEARH = (uint8_t)(address >> 8);
    EEARL = (uint8_t)(address);
    EECR |= (1 << EERE);
    return EEDR;
}

void EEPROM_write_block(uint16_t address, const uint8_t *data, uint8_t length) {
    for (uint8_t i = 0; i < length; i++) {
        EEPROM_write(address + i, data[i]);
    }
}

void EEPROM_read_block(uint16_t address, uint8_t *buffer, uint8_t length) {
    for (uint8_t i = 0; i < length; i++) {
        buffer[i] = EEPROM_read(address + i);
    }
}

#define WDT_TIMEOUT_2S  ((1 << WDP2) | (1 << WDP1) | (1 << WDP0))

void WDT_init(void) {
    MCUSR = 0;
    cli();
    WDTCSR = (1 << WDCE) | (1 << WDE);
    WDTCSR = (1 << WDE) | (1 << WDIE) | WDT_TIMEOUT_2S;
    sei();
}

void WDT_disable(void) {
    cli();
    MCUSR &= ~(1 << WDRF);
    WDTCSR = (1 << WDCE) | (1 << WDE);
    WDTCSR = 0;
    sei();
}

void WDT_reset(void) {
    wdt_reset();
}

uint16_t get_stack_pointer(void) {
    uint8_t sp_low, sp_high;
    sp_low = SPL;
    sp_high = SPH;
    return ((uint16_t)sp_high << 8) | sp_low;
}

void save_crash_dump(uint8_t reason) {
    CrashReport_t report;
    report.stack_pointer_val = get_stack_pointer();
    report.crash_reason = reason;
    report.crash_flag = CRASH_FLAG_SET;
    report.mcusr_mirror = MCUSR;
    cli();
    EEPROM_write(EEPROM_CRASH_REPORT_ADDR + 0, report.crash_flag);
    EEPROM_write(EEPROM_CRASH_REPORT_ADDR + 1, report.crash_reason);
    EEPROM_write(EEPROM_CRASH_REPORT_ADDR + 2, (uint8_t)(report.stack_pointer_val & 0xFF));
    EEPROM_write(EEPROM_CRASH_REPORT_ADDR + 3, (uint8_t)(report.stack_pointer_val >> 8));
    EEPROM_write(EEPROM_CRASH_REPORT_ADDR + 4, report.mcusr_mirror);
    while (EECR & (1 << EEPE)) {
    }
}

uint8_t load_crash_report(CrashReport_t *report) {
    report->crash_flag = EEPROM_read(EEPROM_CRASH_REPORT_ADDR + 0);
    report->crash_reason = EEPROM_read(EEPROM_CRASH_REPORT_ADDR + 1);
    uint8_t sp_low = EEPROM_read(EEPROM_CRASH_REPORT_ADDR + 2);
    uint8_t sp_high = EEPROM_read(EEPROM_CRASH_REPORT_ADDR + 3);
    report->stack_pointer_val = ((uint16_t)sp_high << 8) | sp_low;
    report->mcusr_mirror = EEPROM_read(EEPROM_CRASH_REPORT_ADDR + 4);
    return (report->crash_flag == CRASH_FLAG_SET) ? 1 : 0;
}

void clear_crash_flag(void) {
    EEPROM_write(EEPROM_CRASH_REPORT_ADDR, CRASH_FLAG_CLEAR);
}

void force_system_reset(void) {
    cli();
    WDTCSR = (1 << WDCE) | (1 << WDE);
    WDTCSR = (1 << WDE);
    while(1) {
    }
}

ISR(WDT_vect) {
    save_crash_dump(CRASH_REASON_WDT);
}

void print_crash_analysis(const CrashReport_t *report) {
    UART_println("");
    UART_println("╔══════════════════════════════════════════════════════════════╗");
    UART_println("║        ☠  CRASH DETECTED - POST-MORTEM ANALYSIS  ☠          ║");
    UART_println("╠══════════════════════════════════════════════════════════════╣");
    UART_print("║ Crash Reason: ");
    switch (report->crash_reason) {
        case CRASH_REASON_WDT:
            UART_println("WATCHDOG TIMEOUT (System Hung)                   ║");
            break;
        case CRASH_REASON_EXPLICIT:
            UART_println("EXPLICIT CRASH DUMP (User Triggered)             ║");
            break;
        case CRASH_REASON_STACK_OVF:
            UART_println("STACK OVERFLOW DETECTED                          ║");
            break;
        default:
            UART_println("UNKNOWN                                          ║");
            break;
    }
    UART_print("║ Stack Pointer at Crash: ");
    UART_print_hex16(report->stack_pointer_val);
    UART_println("                              ║");
    UART_print("║ Stack Health: ");
    if (report->stack_pointer_val > 0x0700) {
        UART_println("HEALTHY (low stack usage)                     ║");
    } else if (report->stack_pointer_val > 0x0400) {
        UART_println("MODERATE (watch stack depth)                  ║");
    } else {
        UART_println("CRITICAL (possible overflow!)                 ║");
    }
    UART_print("║ MCUSR Mirror: ");
    UART_print_hex8(report->mcusr_mirror);
    UART_print(" → ");
    if (report->mcusr_mirror & (1 << WDRF)) UART_print("[WDT] ");
    if (report->mcusr_mirror & (1 << BORF)) UART_print("[BOD] ");
    if (report->mcusr_mirror & (1 << EXTRF)) UART_print("[EXT] ");
    if (report->mcusr_mirror & (1 << PORF)) UART_print("[PWR] ");
    UART_println("                        ║");
    UART_println("╚══════════════════════════════════════════════════════════════╝");
    UART_println("");
}

void print_menu(void) {
    UART_println("");
    UART_println("┌──────────────────────────────────────────────────────────────┐");
    UART_println("│       BLACK BOX FORENSIC CRASH REPORTER - TORTURE MENU       │");
    UART_println("├──────────────────────────────────────────────────────────────┤");
    UART_println("│ [1] Trigger Watchdog Timeout (Infinite Loop - Hangs!)        │");
    UART_println("│ [2] Trigger Explicit Crash Dump (Manual Trigger)             │");
    UART_println("│ [3] Clear EEPROM Crash Data                                  │");
    UART_println("│ [4] Read Current Stack Pointer                               │");
    UART_println("│ [5] Test Deep Recursion (Stack Stress)                       │");
    UART_println("└──────────────────────────────────────────────────────────────┘");
    UART_print("Select option: ");
}

void deep_recursion_test(uint16_t depth) {
    volatile uint8_t stack_eater[32];
    stack_eater[0] = depth & 0xFF;
    if (depth > 0) {
        if (depth % 10 == 0) {
            UART_print("  Recursion depth: ");
            UART_print_hex16(depth);
            UART_print(" | SP: ");
            UART_print_hex16(get_stack_pointer());
            UART_println("");
        }
        WDT_reset();
        deep_recursion_test(depth - 1);
    }
}

void trigger_infinite_loop(void) {
    UART_println("");
    UART_println(">>> Entering infinite loop... Watchdog will trigger in ~2s <<<");
    UART_println(">>> System will capture crash dump and reset <<<");
    UART_println("");
    while(1) {
        __asm__ __volatile__ ("nop");
    }
}

void trigger_explicit_crash(void) {
    UART_println("");
    UART_println(">>> User-triggered crash dump - saving state... <<<");
    save_crash_dump(CRASH_REASON_EXPLICIT);
    UART_println(">>> Dump saved! Forcing system reset... <<<");
    force_system_reset();
}

int main(void) {
    CrashReport_t report;
    UART_flush();
    uint8_t mcusr_saved = MCUSR;
    MCUSR = 0;
    wdt_disable();
    UART_init();
    _delay_ms(100);
    UART_println("");
    UART_println("====================================================");
    UART_println("   ATmega328P Black Box Forensic Crash Reporter");
    UART_println("====================================================");
    UART_println("");
    UART_println("[BOOT] Checking EEPROM for crash report...");
    if (load_crash_report(&report)) {
        UART_println("[BOOT] *** PREVIOUS CRASH DETECTED! ***");
        report.mcusr_mirror = mcusr_saved;
        print_crash_analysis(&report);
        UART_println("[BOOT] Clearing crash flag...");
        clear_crash_flag();
        UART_println("[BOOT] Crash data cleared. Ready for new crashes!");
    } else {
        UART_println("[BOOT] No crash report found. Clean boot!");
    }
    UART_flush();
    UART_println("[BOOT] Initializing Watchdog Timer (2s timeout)...");
    WDT_init();
    UART_println("[BOOT] System ready!");
    print_menu();
    while(1) {
        WDT_reset();
        if (UART_available()) {
            char input = UART_receive();
            if (input < '1' || input > '5') {
                _delay_ms(10);
                UART_flush();
                continue;
            }
            UART_transmit(input);
            UART_println("");
            UART_flush();
            switch (input) {
                case '1':
                    trigger_infinite_loop();
                    break;
                case '2':
                    UART_flush();
                    trigger_explicit_crash();
                    break;
                case '3':
                    UART_println(">>> Clearing EEPROM crash data... <<<");
                    clear_crash_flag();
                    UART_println(">>> EEPROM cleared! <<<");
                    print_menu();
                    break;
                case '4':
                    UART_print(">>> Current Stack Pointer: ");
                    UART_print_hex16(get_stack_pointer());
                    UART_println(" <<<");
                    UART_print(">>> RAMEND: ");
                    UART_print_hex16(RAMEND);
                    UART_println(" <<<");
                    UART_flush();
                    print_menu();
                    break;
                case '5':
                    UART_println(">>> Starting deep recursion test (50 levels)... <<<");
                    UART_print(">>> Initial SP: ");
                    UART_print_hex16(get_stack_pointer());
                    UART_println(" <<<");
                    deep_recursion_test(50);
                    UART_print(">>> Final SP: ");
                    UART_print_hex16(get_stack_pointer());
                    UART_println(" <<<");
                    UART_println(">>> Recursion test complete! <<<");
                    UART_flush();
                    print_menu();
                    break;
                default:
                    UART_println(">>> Invalid option! <<<");
                    UART_flush();
                    print_menu();
                    break;
            }
        }
        _delay_ms(10);
    }
    return 0;
}
