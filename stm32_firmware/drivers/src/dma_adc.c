/*
 * dma_adc.c
 *
 * Ultra-Low-Power Dual-MCU IoT Telemetry Node
 *
 * Register-level ADC1 + DMA1 driver for STM32L476: ADC1 free-runs a 3
 * channel scan sequence (battery, temperature, generic), and DMA1 Channel1
 * copies each conversion straight into a circular SRAM buffer so the CPU
 * never has to wake up just to service the ADC.
 *
 * Hardware mapping: see board_config.h for the exact pins/channels.
 */

#include "dma_adc.h"
#include "board_config.h"
#include "stm32l4xx.h"

static uint16_t adc_buffer[ADC_CHANNELS * ADC_BUFFER_DEPTH];
static volatile uint8_t data_ready_flag = 0;

void dma_adc_init(void) {
    /* 1. Clocks: GPIOA for the analog input pins (already switched to
     *    Analog mode by pm_configure_unused_gpios_analog()), ADC for the
     *    converter, DMA1 for the channel that drains it. */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_ADCEN;
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;

    /* 2. ADC kernel clock: STM32L4's ADC needs an explicit clock source
     *    selection independent of AHB/APB (RCC_CCIPR.ADCSEL). Selecting
     *    SYSCLK (0b11) keeps this driver correct even if a future change
     *    reprograms the AHB prescaler, and CKMODE=00 tells the ADC that
     *    clock is asynchronous (not derived from the bus clock). */
    RCC->CCIPR |= (RCC_CCIPR_ADCSEL_0 | RCC_CCIPR_ADCSEL_1);
    ADC123_COMMON->CCR &= ~(ADC_CCR_CKMODE | ADC_CCR_PRESC);

    /* 3. Exit deep-power-down and enable the ADC's internal voltage
     *    regulator. The regulator needs ~20us to stabilize before
     *    calibration or conversion may start (RM0351 §16.4.6). At the
     *    4MHz default MSI clock this busy-loop is comfortably longer than
     *    that; it only needs to be "long enough", not precise. */
    ADC1->CR &= ~ADC_CR_DEEPPWD;
    ADC1->CR |= ADC_CR_ADVREGEN;
    for (volatile uint32_t settle = 0; settle < 2000; settle++) {
        /* wait for the ADC voltage regulator to stabilize */
    }

    /* 4. Single-ended calibration (this design has no differential
     *    inputs), then wait for ADCAL to self-clear when calibration
     *    finishes. */
    ADC1->CR &= ~ADC_CR_ADCALDIF;
    ADC1->CR |= ADC_CR_ADCAL;
    while ((ADC1->CR & ADC_CR_ADCAL) != 0U) {
        /* wait for calibration to complete */
    }

    /* 5. DMA1 Channel1 <-> ADC1 is a fixed hardware pairing on this family
     *    (DMA1_CSELR.C1S = 0 selects ADC1, which is also the reset value).
     *    Configure the channel: peripheral = ADC1 data register, memory =
     *    adc_buffer, circular so it wraps forever without CPU
     *    intervention, memory-increment so each sample lands in the next
     *    slot, half-word (16-bit) transfers matching the ADC's right-
     *    aligned 12-bit result stored in a uint16_t, and a
     *    transfer-complete interrupt so the CPU finds out when a full
     *    batch (all channels x ADC_BUFFER_DEPTH) is ready. */
    DMA1_Channel1->CCR &= ~DMA_CCR_EN;
    DMA1_Channel1->CPAR  = (uint32_t)&ADC1->DR;
    DMA1_Channel1->CMAR  = (uint32_t)adc_buffer;
    DMA1_Channel1->CNDTR = ADC_CHANNELS * ADC_BUFFER_DEPTH;
    DMA1_Channel1->CCR = DMA_CCR_MSIZE_0   /* memory size = 16 bits */
                        | DMA_CCR_PSIZE_0  /* peripheral size = 16 bits */
                        | DMA_CCR_MINC     /* memory address increments */
                        | DMA_CCR_CIRC     /* circular buffer, auto-rewind */
                        | DMA_CCR_TCIE;    /* IRQ on transfer complete */
    DMA1_CSELR->CSELR &= ~DMA_CSELR_C1S;

    NVIC_EnableIRQ(DMA1_Channel1_IRQn);

    /* 6. ADC regular sequence: 3 conversions per trigger (L = 3-1), in the
     *    fixed order battery -> temp -> generic, matching ADC_CHANNELS
     *    indices 0/1/2 used by dma_adc_get_averaged_channel(). */
    ADC1->SQR1 = ((ADC_CHANNELS - 1U) << ADC_SQR1_L_Pos)
               | (ADC_CH_BATTERY_NUM << ADC_SQR1_SQ1_Pos)
               | (ADC_CH_TEMP_NUM    << ADC_SQR1_SQ2_Pos)
               | (ADC_CH_GENERIC_NUM << ADC_SQR1_SQ3_Pos);

    /* Sample time 111 = 640.5 ADC cycles per channel: slower than the
     * minimum, but this design sacrifices sample rate for accuracy/power
     * (fewer, better-settled samples) since it only needs a handful of
     * averaged readings per wake cycle, not high-speed acquisition.
     * Channel 6/9 sample-time fields live in SMPR1, channel 10 in SMPR2. */
    ADC1->SMPR1 = (0x7UL << ADC_SMPR1_SMP6_Pos)
                | (0x7UL << ADC_SMPR1_SMP9_Pos);
    ADC1->SMPR2 = (0x7UL << ADC_SMPR2_SMP10_Pos);

    /* DMAEN: route conversions to DMA. DMACFG=1 ("circular" DMA request
     * mode): the ADC keeps re-arming DMA requests indefinitely instead of
     * stopping after one full DMA transfer, which pairs with the DMA
     * channel's own CIRC bit above so both sides free-run together. CONT:
     * once the sequence finishes, restart it immediately (continuous
     * conversion) instead of waiting for a new external trigger. */
    ADC1->CFGR |= ADC_CFGR_DMAEN | ADC_CFGR_DMACFG | ADC_CFGR_CONT;

    /* 7. Enable the ADC and wait for it to report ready before anyone
     *    calls dma_adc_start(). */
    ADC1->ISR |= ADC_ISR_ADRDY; /* ADRDY is cleared by writing 1 */
    ADC1->CR |= ADC_CR_ADEN;
    while ((ADC1->ISR & ADC_ISR_ADRDY) == 0U) {
        /* wait for "ADC ready" */
    }
}

void dma_adc_start(void) {
    /* Order matters: the DMA channel must be armed before the ADC starts
     * converting, otherwise the first conversion(s) have nowhere to go. */
    DMA1_Channel1->CCR |= DMA_CCR_EN;
    ADC1->CR |= ADC_CR_ADSTART;
}

void dma_adc_stop(void) {
    /* ADSTP requests a clean stop of the regular sequence; ADC hardware
     * clears ADSTART itself once the current conversion finishes. */
    ADC1->CR |= ADC_CR_ADSTP;
    while ((ADC1->CR & ADC_CR_ADSTART) != 0U) {
        /* wait for the in-flight conversion to stop */
    }
    DMA1_Channel1->CCR &= ~DMA_CCR_EN;
}

uint8_t dma_adc_is_data_ready(void) {
    return data_ready_flag;
}

void dma_adc_clear_data_ready(void) {
    data_ready_flag = 0;
}

uint16_t dma_adc_get_averaged_channel(uint8_t channel_index) {
    if (channel_index >= ADC_CHANNELS) {
        return 0;
    }

    /* adc_buffer is interleaved [sample0_ch0, sample0_ch1, sample0_ch2,
     * sample1_ch0, ...] because the ADC writes one full 3-channel sequence
     * per DMA burst; stride by ADC_CHANNELS to collect one channel's
     * samples across the whole circular buffer depth. */
    uint32_t sum = 0;
    for (uint16_t i = 0; i < ADC_BUFFER_DEPTH; i++) {
        sum += adc_buffer[(i * ADC_CHANNELS) + channel_index];
    }
    return (uint16_t)(sum / ADC_BUFFER_DEPTH);
}

/*
 * DMA1 Channel1 transfer-complete interrupt handler. This name is not
 * arbitrary: it overrides the weak default defined in
 * CMSIS/Device/.../startup_stm32l476xx.s, which is how the linker wires
 * this function into the vector table instead of the default handler.
 */
void DMA1_Channel1_IRQHandler(void) {
    if ((DMA1->ISR & DMA_ISR_TCIF1) != 0U) {
        /* Acknowledge the transfer-complete flag (write 1 to clear). */
        DMA1->IFCR = DMA_IFCR_CTCIF1;
        data_ready_flag = 1;
    }
    if ((DMA1->ISR & DMA_ISR_TEIF1) != 0U) {
        /* Transfer error: clear it too so the interrupt doesn't re-fire
         * forever. This design has no dedicated error-recovery path since
         * a mis-wired DMA/ADC configuration is a build-time bug, not a
         * runtime condition to handle gracefully. */
        DMA1->IFCR = DMA_IFCR_CTEIF1;
    }
}
