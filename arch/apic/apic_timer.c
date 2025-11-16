#include<apic.h>
#include<stdint.h>
#include<rtc.h>
#include<process.h>

static uint32_t lapic_timer_divisor_encode(uint32_t divisor) {
    switch (divisor) {
    case 1:   return 0b1011;
    case 2:   return 0b0000;
    case 4:   return 0b0001;
    case 8:   return 0b0010;
    case 16:  return 0b0011;
    case 32:  return 0b1000;
    case 64:  return 0b1001;
    case 128: return 0b1010;
    default:  return 0b0011; /* fall back to divide by 16 */
    }
}

void lapic_timer_init_periodic(uint8_t vector, uint32_t initial_count, uint32_t divisor) {
    const uint32_t LVT_TIMER_PERIODIC = 1u << 17;
    lapic_write(LAPIC_TIMER_DIV_CFG, lapic_timer_divisor_encode(divisor));
    lapic_write(LAPIC_LVT_TIMER, (vector & 0xFFu) | LVT_TIMER_PERIODIC);
    lapic_write(LAPIC_TIMER_INIT_CNT, initial_count);
}

void lapic_timer_stop(void) {
    const uint32_t LVT_MASKED = 1u << 16;
    uint32_t lvt = lapic_read(LAPIC_LVT_TIMER);
    lapic_write(LAPIC_LVT_TIMER, lvt | LVT_MASKED);
    lapic_write(LAPIC_TIMER_INIT_CNT, 0);
}

uint32_t lapic_timer_current_count(void) {
    return lapic_read(LAPIC_TIMER_CUR_CNT);
}

uint32_t lapic_timer_calibrate_frequency(uint32_t divisor) {
    const uint32_t LVT_TIMER_ONESHOT = 0; /* One-shot mode (bit 17 = 0) */
    const uint32_t LVT_MASKED = 1u << 16;
    const uint32_t INITIAL_COUNT = 0xFFFFFFFFu; /* Maximum count value */
    
    /* Stop timer if running */
    uint32_t lvt = lapic_read(LAPIC_LVT_TIMER);
    lapic_write(LAPIC_LVT_TIMER, lvt | LVT_MASKED);
    
    /* Configure timer divisor */
    lapic_write(LAPIC_TIMER_DIV_CFG, lapic_timer_divisor_encode(divisor));
    
    /* Configure timer in one-shot mode (masked) */
    lapic_write(LAPIC_LVT_TIMER, LVT_TIMER_ONESHOT | LVT_MASKED);
    
    /* Wait for RTC update to complete and get stable start time */
    uint32_t start_time = rtc_read_time_seconds();
    uint32_t next_time = rtc_read_time_seconds();
    
    /* Wait until RTC second changes to ensure we start at a clean second boundary */
    while (next_time == start_time) {
        start_time = next_time;
        next_time = rtc_read_time_seconds();
    }
    start_time = next_time;
    
    /* Start timer with maximum count */
    lapic_write(LAPIC_TIMER_INIT_CNT, INITIAL_COUNT);
    
    /* Unmask timer to start counting */
    lvt = lapic_read(LAPIC_LVT_TIMER);
    lapic_write(LAPIC_LVT_TIMER, lvt & ~LVT_MASKED);
    
    /* Wait for 1 second using RTC */
    uint32_t end_time = start_time;
    while (end_time == start_time) {
        end_time = rtc_read_time_seconds();
    }
    
    /* Stop timer */
    lvt = lapic_read(LAPIC_LVT_TIMER);
    lapic_write(LAPIC_LVT_TIMER, lvt | LVT_MASKED);
    
    /* Read final count */
    uint32_t final_count = lapic_read(LAPIC_TIMER_CUR_CNT);
    
    /* Calculate frequency: ticks per second */
    /* We waited exactly 1 second, so frequency = (INITIAL_COUNT - final_count) */
    uint32_t ticks_elapsed = INITIAL_COUNT - final_count;
    
    /* Clear timer */
    lapic_write(LAPIC_TIMER_INIT_CNT, 0);
    
    return ticks_elapsed;
}


/* 定时器中断处理函数 */
uint32_t timer_interrupt_handler(void)
{
    lapic_eoi();
    if ( current->time_slice > 0) {
        current->time_slice--;
        return 1;
    }

    if(list_empty(&ready_task_head))
    {
        current->time_slice=PROCESS_TIME_SLICE;
        return 1;
    }
    return 0;
}