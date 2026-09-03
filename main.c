#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define TICKS_GREEN    5U
#define TICKS_YELLOW   2U
#define TICKS_RED      4U
#define QUEUE_BUSY     6U
#define LOG_LEN       20U

typedef enum { LIGHT_GREEN = 0, LIGHT_YELLOW, LIGHT_RED } LightState_t;

#define BIT_NIGHT      0U
#define BIT_BUSY       1U
#define BIT_BLINK_ON   2U

#define SET_BIT(reg, n)    ((reg) |=  (uint8_t)(1U << (n)))
#define CLR_BIT(reg, n)    ((reg) &= (uint8_t)~(1U << (n)))
#define TOGGLE_BIT(reg, n) ((reg) ^=  (uint8_t)(1U << (n)))
#define READ_BIT(reg, n)   ((uint8_t)(((reg) >> (n)) & 1U))

static LightState_t light = LIGHT_RED;
static uint8_t status;
static uint8_t ticksLeft;
static uint8_t carsWaiting;
static uint32_t carsPassed;
static uint32_t totalTicks;
static char logLine[LOG_LEN];

static LightState_t nextState(LightState_t state);

static void clearInput(void)
{
    int character;

    while ((character = getchar()) != '\n' && character != EOF) {
    }
}

static void resetCrossing(void)
{
    while (light != LIGHT_RED) {
        nextState(light);
    }
    status = 0U;
    ticksLeft = (uint8_t)TICKS_RED;
    carsWaiting = 0U;
    carsPassed = 0U;
    totalTicks = 0U;
    memset(logLine, 0, sizeof(logLine));
}

static uint8_t ticksFor(LightState_t state)
{
    if (state == LIGHT_GREEN) {
        return (uint8_t)(TICKS_GREEN + (READ_BIT(status, BIT_BUSY) ? 2U : 0U));
    }
    return (state == LIGHT_YELLOW) ? (uint8_t)TICKS_YELLOW : (uint8_t)TICKS_RED;
}

static LightState_t nextState(LightState_t state)
{
    light = (state == LIGHT_GREEN) ? LIGHT_YELLOW :
            (state == LIGHT_YELLOW) ? LIGHT_RED : LIGHT_GREEN;
    return light;
}

static void drawLight(void)
{
    const char *names[] = { "GREEN", "YELLOW", "RED" };
    LightState_t shown = READ_BIT(status, BIT_NIGHT) ? LIGHT_YELLOW : light;
    uint8_t yellowOn = READ_BIT(status, BIT_NIGHT) ? READ_BIT(status, BIT_BLINK_ON) : 0U;

    printf("  (%s)\n", shown == LIGHT_GREEN ? "*" : " ");
    printf("  (%s)\n", shown == LIGHT_YELLOW && (!READ_BIT(status, BIT_NIGHT) || yellowOn) ? "*" : " ");
    printf("  (%s)\n", shown == LIGHT_RED ? "*" : " ");
    printf("Colour: %s | Ticks left: %u | Cars waiting: %u\n",
           names[shown], ticksLeft, carsWaiting);
}

static void updateBusy(void)
{
    if (carsWaiting > QUEUE_BUSY) {
        SET_BIT(status, BIT_BUSY);
    } else {
        CLR_BIT(status, BIT_BUSY);
    }
}

static void pushLog(char entry)
{
    memmove(logLine, logLine + 1, LOG_LEN - 1U);
    logLine[LOG_LEN - 1U] = entry;
}

static void tick(void)
{
    totalTicks++;
    if (READ_BIT(status, BIT_NIGHT)) {
        TOGGLE_BIT(status, BIT_BLINK_ON);
        pushLog('y');
        return;
    }
    if (light == LIGHT_GREEN && carsWaiting > 0U) {
        uint8_t leaving = carsWaiting > 2U ? 2U : carsWaiting;
        carsWaiting = (uint8_t)(carsWaiting - leaving);
        carsPassed += leaving;
        updateBusy();
    }
    ticksLeft--;
    if (ticksLeft == 0U) {
        nextState(light);
        ticksLeft = ticksFor(light);
    }
    pushLog(light == LIGHT_GREEN ? 'G' : light == LIGHT_YELLOW ? 'Y' : 'R');
}

static void addCars(void)
{
    unsigned int arriving;

    printf("Cars arriving (0-255): ");
    if (scanf("%u", &arriving) != 1) {
        printf("Invalid number.\n");
        clearInput();
        return;
    }
    clearInput();
    if (arriving > (unsigned int)UINT8_MAX - carsWaiting) {
        printf("Queue limit exceeded.\n");
        return;
    }
    carsWaiting = (uint8_t)(carsWaiting + arriving);
    updateBusy();
}

static void toggleNight(void)
{
    TOGGLE_BIT(status, BIT_NIGHT);
    if (READ_BIT(status, BIT_NIGHT)) {
        SET_BIT(status, BIT_BLINK_ON);
    } else {
        while (light != LIGHT_RED) {
            nextState(light);
        }
        ticksLeft = (uint8_t)TICKS_RED;
    }
}

static void showLog(void)
{
    printf("History: %.*s\n", (int)LOG_LEN, logLine);
}

static void crossingReport(void)
{
    unsigned int bit;

    printf("Ticks: %lu | Passed: %lu | Waiting: %u\n",
           (unsigned long)totalTicks, (unsigned long)carsPassed, carsWaiting);
    printf("Night: %s | Busy: %s\n",
           READ_BIT(status, BIT_NIGHT) ? "yes" : "no",
           READ_BIT(status, BIT_BUSY) ? "yes" : "no");
    printf("Status: ");
    for (bit = 8U; bit-- > 0U;) {
        printf("%u", READ_BIT(status, bit));
    }
    printf(" (0x%02X)\n", status);
}

int main(void)
{
    unsigned int choice;

    resetCrossing();
    do {
        printf("\n1 Draw  2 Tick  3 Add cars  4 Night/day\n");
        printf("5 History  6 Report  0 Quit\nChoice: ");
        if (scanf("%u", &choice) != 1) {
            printf("Invalid choice.\n");
            clearInput();
            continue;
        }
        clearInput();
        switch (choice) {
        case 1U: drawLight(); break;
        case 2U: tick(); break;
        case 3U: addCars(); break;
        case 4U: toggleNight(); break;
        case 5U: showLog(); break;
        case 6U: crossingReport(); break;
        case 0U: break;
        default: printf("Unknown choice.\n"); break;
        }
    } while (choice != 0U);
    return 0;
}
