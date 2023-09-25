/*
 *
 */

#include <em_device.h>
#include <em_chip.h>
#include <em_emu.h>
#include <em_cmu.h>
#include <em_usart.h>
#include <em_timer.h>

#include "usart.h"
#include "target.h"

static usart_hw_t usart_hw[2] = {
	/* USART0 */
	{
		.regs = USART0,
		.clock = cmuClock_USART0,
		.rx = {
			.irq = USART0_RX_IRQn,
			.route = USART_ROUTELOC0_RXLOC_LOC8,
			.half_route = USART_ROUTELOC0_RXLOC_LOC9,
			.port = USART0_PORT_RX,
			.pin = USART0_PIN_RX,
		},
		.tx = {
			.irq = USART0_TX_IRQn,
			.route = USART_ROUTELOC0_TXLOC_LOC10,
			.half_route = USART_ROUTELOC0_TXLOC_LOC9,
			.port = USART0_PORT_TX,
			.pin = USART0_PIN_TX,
		},
	},
	/* USART1 */
	{
		.regs = USART1,
		.clock = cmuClock_USART1,
		.rx = {
			.irq = USART1_RX_IRQn,
			.route = USART_ROUTELOC0_RXLOC_LOC14,
			.half_route = USART_ROUTELOC0_RXLOC_LOC15,
			.port = USART1_PORT_RX,
			.pin = USART1_PIN_RX,
			.inverted = 1,
		},
		.tx = {
			.irq = USART1_TX_IRQn,
			.route = USART_ROUTELOC0_TXLOC_LOC16,
			.half_route = USART_ROUTELOC0_TXLOC_LOC15,
			.port = USART1_PORT_TX,
			.pin = USART1_PIN_TX,
			.inverted = 1,
		},
	},
};

static inline void USART_RX_IRQHandler(USART_TypeDef *usart, int num)
{
	uint32_t flags = USART_IntGet(usart);

	flags &= USART_IEN_RXOF | USART_IEN_RXDATAV;

	if (flags & USART_IEN_RXDATAV)
		usart_rx_irq(num);
	//USART_IntClear(usart, flags);
}

static inline void USART_TX_IRQHandler(USART_TypeDef *usart, int num)
{
	uint32_t flags = USART_IntGet(usart);

	flags &= USART_IEN_TXBL | USART_IEN_TXC;;

	if (flags & USART_IEN_TXBL)
		usart_tx_irq(num);
	if (flags & USART_IEN_TXC)
		usart_tx_complete_irq(num);

	//USART_IntClear(usart, flags);
}

/*
 * The USART0 receive interrupt
 */
void USART0_RX_IRQHandler(void)
{
	USART_RX_IRQHandler(USART0, 0);
}

/*
 * The USART1 receive interrupt
 */
void USART1_RX_IRQHandler(void)
{
	USART_RX_IRQHandler(USART1, 1);
}

/*
 *
 * The USART0 transmit interrupt
 */
void USART0_TX_IRQHandler(void)
{
	USART_TX_IRQHandler(USART0, 0);
}

/*
 *
 * The USART1 transmit interrupt
 */
void USART1_TX_IRQHandler(void)
{
	USART_TX_IRQHandler(USART1, 1);
}

static void usart_configure_pins(usart_hw_t *hw)
{
	CMU_ClockEnable(cmuClock_GPIO, true);

	/* Configure VCOM transmit pin to board controller as an output */
	GPIO_PinModeSet(hw->tx.port, hw->tx.pin, gpioModePushPull, 1);

	/* Configure VCOM reeive pin from board controller as an input */
	GPIO_PinModeSet(hw->rx.port, hw->rx.pin, gpioModeInput, 0);

	// Enable VCOM connection to board controller
	//GPIO_PinModeSet(BSP_BCC_ENABLE_PORT, BSP_BCC_ENABLE_PIN, gpioModePushPull, 1);
}

void usart_hw_half_duplex_tx(usart_hw_t *hw)
{
	GPIO_PinModeSet(hw->rx.port, hw->rx.pin, gpioModeWiredAndPullUp, 0);
	GPIO_PinModeSet(hw->tx.port, hw->tx.pin, gpioModeInput, 0);

	hw->regs->ROUTEPEN &= ~(USART_ROUTEPEN_RXPEN | USART_ROUTEPEN_TXPEN);
	hw->regs->ROUTELOC0 = hw->rx.half_route | hw->tx.half_route;
	hw->regs->ROUTEPEN |= USART_ROUTEPEN_RXPEN | USART_ROUTEPEN_TXPEN;
}

void usart_hw_half_duplex_rx(usart_hw_t *hw)
{
	GPIO_PinModeSet(hw->tx.port, hw->tx.pin, gpioModeWiredAndPullUp, 0);
	GPIO_PinModeSet(hw->rx.port, hw->rx.pin, gpioModeInput, 0);

	hw->regs->ROUTEPEN &= ~(USART_ROUTEPEN_RXPEN | USART_ROUTEPEN_TXPEN);
	hw->regs->ROUTELOC0 = hw->rx.route | hw->tx.route;
	hw->regs->ROUTEPEN |= USART_ROUTEPEN_RXPEN | USART_ROUTEPEN_TXPEN;
}

usart_hw_t *usart_hw_init(int num)
{
	usart_hw_t *hw = &usart_hw[num];
	/* Default asynchronous initializer (115.2 Kbps, 8N1, no flow control) */
	USART_InitAsync_TypeDef init = USART_INITASYNC_DEFAULT;

	usart_configure_pins(hw);

	/* Configure and enable USART */
	CMU_ClockEnable(hw->clock, true);
	USART_InitAsync(hw->regs, &init);

	USART_IntClear(hw->regs, USART_IEN_RXDATAV | USART_IEN_RXOF |
			USART_IEN_TXBL | USART_IEN_TXC);

	/* Enable NVIC USART sources */
	NVIC_ClearPendingIRQ(hw->rx.irq);
	NVIC_EnableIRQ(hw->rx.irq);
	NVIC_ClearPendingIRQ(hw->tx.irq);
	NVIC_EnableIRQ(hw->tx.irq);

	/* inverted */

	if (hw->rx.inverted)
		hw->regs->CTRL |= USART_CTRL_RXINV;
	if (hw->tx.inverted)
		hw->regs->CTRL |= USART_CTRL_TXINV;

	/* Enable RX and TX for USART VCOM connection */
	hw->regs->ROUTELOC0 = hw->rx.route | hw->tx.route;
	hw->regs->ROUTEPEN |= USART_ROUTEPEN_RXPEN | USART_ROUTEPEN_TXPEN;

	return hw;
}

static void clocks_init(void)
{
	/* Source clock for CORE and high frequency peripheral:
	 * high frequency crystal oscillator  */
	CMU_ClockSelectSet(cmuClock_HF, cmuSelect_HFXO);

	CMU_ClockEnable(cmuClock_HFLE, true);
	CMU_ClockSelectSet(cmuClock_LFA, cmuSelect_LFRCO);
	CMU_ClockSelectSet(cmuClock_LFB, cmuSelect_LFRCO);
#if defined(_CMU_LFCCLKSEL_MASK)
	CMU_ClockSelectSet(cmuClock_LFC, cmuSelect_LFRCO);
#endif
#if defined(_CMU_LFECLKSEL_MASK)
	CMU_ClockSelectSet(cmuClock_LFE, cmuSelect_LFRCO);
#endif
}

#define TIMER0_PRESCALE			timerPrescale1
/* uS ticks */
static volatile uint32_t timer_tick = 0;
/*
 * Input frequency 38.4 MHz
 * Timer tick = 1 ms
 * Timer counter top = 38400
 */
static void timer_init(void)
{
	TIMER_Init_TypeDef timerInit = TIMER_INIT_DEFAULT;
	TIMER_InitCC_TypeDef timerCCInit = TIMER_INITCC_DEFAULT;
	uint32_t top = CMU_ClockFreqGet(cmuClock_HFPER) /
			(TIMER_TICK_HZ * (1 << TIMER0_PRESCALE)) - 1;

	/* Enable clock for TIMER0 module */
	CMU_ClockEnable(cmuClock_TIMER0, true);

	/* Configure TIMER0 Compare/Capture for output compare */
	timerCCInit.mode = timerCCModeCompare;
	timerCCInit.cmoa = timerOutputActionNone;
	TIMER_InitCC(TIMER0, 0, &timerCCInit);

	TIMER_TopSet(TIMER0, top);

	/* Initialize and start timer with defined prescale */
	timerInit.prescale = TIMER0_PRESCALE;
	TIMER_Init(TIMER0, &timerInit);

	TIMER_IntClear(TIMER0, TIMER_IF_OF);
	TIMER_IntEnable(TIMER0, TIMER_IF_OF);
	NVIC_EnableIRQ(TIMER0_IRQn);
}

uint32_t timer_get_us(void)
{
	uint32_t freq = CMU_ClockFreqGet(cmuClock_HFPER) >> TIMER0_PRESCALE;
	uint32_t val = (TIMER_CounterGet(TIMER0) / (freq / 100000)) * 10  + timer_tick;

	return val;
}

uint32_t timer_get_ms(void)
{
	return timer_tick / 1000;
}

void timer_sleep_us(uint32_t us)
{
	uint32_t start = timer_get_us();

	while ((timer_get_us() - start) < us);
}

void timer_sleep_ms(uint32_t ms)
{
	uint32_t start = timer_get_ms();

	while ((timer_get_ms() - start) < ms);
}

void TIMER0_IRQHandler(void)
{
	uint16_t flags = TIMER_IntGet(TIMER0);

	if (flags & TIMER_IF_OF)
		timer_tick += TIMER_TICK_HZ;

	TIMER_IntClear(TIMER0, flags);
}

static void gpio_init(void)
{
	CMU_ClockEnable(cmuClock_GPIO, true);

	/* led red and green */
	GPIO_PinModeSet(LED_GREEN_PORT, LED_GREEN_PIN, gpioModePushPull, 0);
	GPIO_PinModeSet(LED_RED_PORT, LED_RED_PIN, gpioModePushPull, 0);

	GPIO_PinModeSet(JP1_PORT, JP1_PIN, gpioModeInputPull, 1);
	GPIO_PinModeSet(JP2_PORT, JP2_PIN, gpioModeInputPull, 1);

	/* enable LNA pin */
	GPIO_PinModeSet(RF_RX_EN_PORT, RF_RX_EN_PIN, gpioModePushPull, 1);

	/* enable PA pin */
	GPIO_PinModeSet(RF_TX_EN_PORT, RF_TX_EN_PIN, gpioModePushPull, 0);

	/* modem RTS pin */
	GPIO_PinModeSet(RF_RTS_PORT, RF_RTS_PIN, gpioModePushPull, 0);


	if (CORE_NvicIRQDisabled(GPIO_ODD_IRQn)) {
		NVIC_ClearPendingIRQ(GPIO_ODD_IRQn);
		NVIC_EnableIRQ(GPIO_ODD_IRQn);
	}
	if (CORE_NvicIRQDisabled(GPIO_EVEN_IRQn)) {
		NVIC_ClearPendingIRQ(GPIO_EVEN_IRQn);
		NVIC_EnableIRQ(GPIO_EVEN_IRQn);
	}
}

void target_init(void)
{
	clocks_init();
	gpio_init();
	timer_init();
}

