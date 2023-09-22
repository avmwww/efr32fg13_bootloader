/*
 *
 */

#ifndef _TARGET_H_
#define _TARGET_H_

#include <em_chip.h>
#include <em_cmu.h>
#include <em_gpio.h>
#include <em_usart.h>

#define RF_RX_EN_PORT			gpioPortD
#define RF_RX_EN_PIN			15
#define RF_TX_EN_PORT			gpioPortD
#define RF_TX_EN_PIN			14
#define RF_RTS_PORT			gpioPortB
#define RF_RTS_PIN			11
#define LED_GREEN_PORT			gpioPortA
#define LED_GREEN_PIN			0
#define LED_RED_PORT			gpioPortA
#define LED_RED_PIN			1
#define JP1_PORT			gpioPortF
#define JP1_PIN				3
#define JP2_PORT			gpioPortF
#define JP2_PIN				2

#define led_red_on()			GPIO_PinOutSet(LED_RED_PORT, LED_RED_PIN)
#define led_red_off()			GPIO_PinOutClear(LED_RED_PORT, LED_RED_PIN)
#define led_red_toggle()		GPIO_PinOutToggle(LED_RED_PORT, LED_RED_PIN)
#define led_green_on()			GPIO_PinOutSet(LED_GREEN_PORT, LED_GREEN_PIN)
#define led_green_off()			GPIO_PinOutClear(LED_GREEN_PORT, LED_GREEN_PIN)
#define led_green_toggle()		GPIO_PinOutToggle(LED_GREEN_PORT, LED_GREEN_PIN)

#define modem_power_rx_on()		GPIO_PinOutSet(RF_RX_EN_PORT, RF_RX_EN_PIN)
#define modem_power_rx_off()		GPIO_PinOutClear(RF_RX_EN_PORT, RF_RX_EN_PIN)
#define modem_power_tx_on()		GPIO_PinOutSet(RF_TX_EN_PORT, RF_TX_EN_PIN)
#define modem_power_tx_off()		GPIO_PinOutClear(RF_TX_EN_PORT, RF_TX_EN_PIN)
#define modem_rts_set()			GPIO_PinOutClear(RF_RTS_PORT, RF_RTS_PIN)
#define modem_rts_clear()		GPIO_PinOutSet(RF_RTS_PORT, RF_RTS_PIN)

#define modem_led_tx_on()		led_green_on()
#define modem_led_tx_off()		led_green_off()
#define modem_led_rx_on()		led_red_on()
#define modem_led_rx_off()		led_red_off()

#define USART0_PORT_TX			gpioPortB
#define USART0_PIN_TX			15
#define USART0_PORT_RX			gpioPortB
#define USART0_PIN_RX			14

#define USART1_PORT_TX			gpioPortC
#define USART1_PIN_TX			11
#define USART1_PORT_RX			gpioPortC
#define USART1_PIN_RX			10

#define USART0_BUF_LEN			512
#define USART1_BUF_LEN			512

typedef struct usart_hw_s {
	USART_TypeDef *regs;
	CMU_Clock_TypeDef clock;
	struct {
		IRQn_Type irq;
		uint32_t route;
		uint32_t half_route;
		GPIO_Port_TypeDef port;
		uint32_t pin;
		int inverted;
	} tx, rx;
} usart_hw_t;

static inline void usart_hw_tx_irq_disable(usart_hw_t *hw)
{
	USART_IntDisable(hw->regs, USART_IEN_TXBL);
}

static inline void usart_hw_tx_complete_irq_disable(usart_hw_t *hw)
{
	USART_IntDisable(hw->regs, USART_IEN_TXC);
}

static inline void usart_hw_tx_irq_enable(usart_hw_t *hw)
{
	USART_IntEnable(hw->regs, USART_IEN_TXBL);
}

static inline void usart_hw_tx_complete_irq_enable(usart_hw_t *hw)
{
	USART_IntEnable(hw->regs, USART_IEN_TXC);
}

static inline void usart_hw_rx_irq_disable(usart_hw_t *hw)
{
	USART_IntDisable(hw->regs, USART_IEN_RXDATAV | USART_IEN_RXOF);
}

static inline void usart_hw_rx_irq_enable(usart_hw_t *hw)
{
	USART_IntEnable(hw->regs, USART_IEN_RXDATAV | USART_IEN_RXOF);
}

static inline void usart_hw_tx(usart_hw_t *hw, uint8_t data)
{
	hw->regs->TXDATA = data;
}

static inline int usart_hw_rx_ready(usart_hw_t *hw)
{
	return USART_IntGet(hw->regs) & USART_IEN_RXDATAV;
}

static inline uint8_t usart_hw_rx(usart_hw_t *hw)
{
	return hw->regs->RXDATA;;
}

static inline uint32_t usart_hw_get_baudrate(usart_hw_t *hw)
{
	return USART_BaudrateGet(hw->regs);
}

static inline void usart_hw_set_baudrate(usart_hw_t *hw, uint32_t baud)
{
	USART_BaudrateAsyncSet(hw->regs, 0, baud, usartOVS16);
}

static inline void usart_hw_set_msb(usart_hw_t *hw, int msb)
{
	if (msb)
		hw->regs->CTRL |= USART_CTRL_MSBF;
	else
		hw->regs->CTRL &= ~USART_CTRL_MSBF;
}

static inline void usart_hw_set_inv(usart_hw_t *hw, int rx, int tx)
{
	if (rx)
		hw->regs->CTRL |= USART_CTRL_RXINV;
	else
		hw->regs->CTRL &= ~USART_CTRL_RXINV;

	if (tx)
		hw->regs->CTRL |= USART_CTRL_TXINV;
	else
		hw->regs->CTRL &= ~USART_CTRL_TXINV;

}

static inline void usart_hw_enable(usart_hw_t *hw, int ena_tx, int ena_rx)
{
	uint32_t cmd = 0;

	if (ena_tx)
		cmd |= USART_CMD_TXEN;
	if (ena_rx)
		cmd |= USART_CMD_RXEN;

	hw->regs->CMD = cmd;
}

//GPIO_PinOutSet

void usart_hw_half_duplex_tx(usart_hw_t *hw);
void usart_hw_half_duplex_rx(usart_hw_t *hw);
usart_hw_t *usart_hw_init(int num);

void target_init(void);

#define TIMER_TICK_HZ			1000
uint32_t timer_get_us(void);
uint32_t timer_get_ms(void);
void timer_sleep_ms(uint32_t ms);
void timer_sleep_us(uint32_t us);

#define XSTR(s) STR(s)
#define STR(s) #s

#define BTL_VERSION_MAJOR	1
#define BTL_VERSION_MINOR	0

#define BTL_VERSION_STR		"BTL V" XSTR(BTL_VERSION_MAJOR) "." XSTR(BTL_VERSION_MINOR)




void dump_buf(const char *prefix, const void *buf, unsigned int len, int as);
#include <stdio.h>
#define dbg			printf

#endif
