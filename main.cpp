#include <stdint.h>
#include <avr/io.h>

#ifndef F_CPU
#define F_CPU 8000000
#endif

#define MS_TO_TICKS(ms) ((uint16_t)((F_CPU / 1000.0 / 1024.0) * (ms)))    // MAX 32ms
#define US_TO_TICKS(us) ((uint16_t)((F_CPU / 1000000.0 / 1024.0) * (us))) // MAX 32640us

#define NUM_TASKS 2
#define STACK_SIZE 128
uint8_t stack1[STACK_SIZE];
uint8_t stack2[STACK_SIZE];

struct Task
{
    uint8_t *stack_pointer;
    void (*fun_ptr)(void);
    uint16_t next_run;
};

volatile struct Task tasks[] = {
    {stack1 + STACK_SIZE - 1, task1},
    {stack2 + STACK_SIZE - 1, task2}};

volatile struct Task *current_task; // Pointer to the current task
volatile uint8_t *kernel_sp;        // Pointer to the kernel stack pointer
int task_idx = 0;                   // Used to track witch task is active.

// Functions decleared in ASM_helper.s, backs up the registers and
// switches stacks between current task and kernel
extern void start_task();
extern void suspend_task();

// Our task functions
void task1();
void task2();

// Initializes each tasks stack pointer to the respective function pointer.
void initialize_tasks()
{
    for (uint8_t i = 0; i < NUM_TASKS; i++)
    {
        *(tasks[i].stack_pointer) = (uint16_t)tasks[i].fun_ptr;
        tasks[i].stack_pointer--;
        *(tasks[i].stack_pointer) = (uint16_t)tasks[i].fun_ptr;
        tasks[i].stack_pointer--;
    }
}

void initialize_timer()
{
    // TCCR0A Register setup (timer0)
    // COM0A1, COM0B1, COM0A0, COM0B0 is initialized to zero, OK
    // WGM02, WGM01, WGM00 is initialized to zero for normal mode (count from 0 to MAX eg. 255)

    // Prescaler is set to 1024.
    // 8Mhz / 1024 = 7812Hz
    // 7812Hz has a period of 128us.
    // this means that we can delay for a max of 128us * 255 = 32640us or 32.64ms
    TCCR0A = (1 << CS00) | (1 << CS02);
}

inline uint16_t get_time()
{
    return TCNT0;
}

/**
 * @param ticks max delay is 255 ticks, 1 tick is 128us
 */
void task_delay(uint16_t ticks)
{
    current_task->next_run = get_time() + ticks;
    suspend_task();
}

bool is_time_past(uint16_t target_time)
{
    // Since the timer rolls over at 255, we have to do some arithmetic to check for this.
    uint8_t current_time = get_time();
    const int16_t HALF_TIMER = 127;

    int16_t diff = ((int16_t)target_time) - ((int16_t)current_time);

    // If target_time rolled over, but current_time hasn't
    if (diff < -HALF_TIMER)
    {
        return false;
    }
    // If current_time rolled over, but target_time hasn't
    else if (diff > HALF_TIMER)
    {
        return true;
    }
    else
    {
        return diff < 0;
    }
}

void task1()
{
    PORTB |= (1 << PB0);
    task_delay(MS_TO_TICKS(20));
    PORTB &= ~(1 << PB0);
    task_delay(MS_TO_TICKS(10));
}

void task2()
{
    PORTB |= (1 << PB0);
    task_delay(MS_TO_TICKS(20));
    PORTB &= ~(1 << PB0);
    task_delay(MS_TO_TICKS(30));
}

int main(void)
{
    initialize_timer();
    initialize_tasks();

    while (1)
    {
        current_task = tasks + task_idx;
        if (is_time_past(current_task->next_run))
        {
            start_task();
        }
        task_idx = (task_idx + 1) % NUM_TASKS;
    }
}