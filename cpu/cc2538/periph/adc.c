/*
 * Copyright (c) 2013, ADVANSEE - http://www.advansee.com/
 * Benoît Thébaudeau <benoit.thebaudeau@advansee.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

 /**
 * @ingroup     cpu_cc2538
 * @{
 *
 * @file
 * @brief       Low-Level ADC driver interface implementation
 *
 * @author      Benoît Thébaudeau <benoit.thebaudeau@advansee.com>
 * @author      Ankith ShashiKanthReddy (anrg.usc.edu) <ashashik@usc.edu>
 * @author      Jason A. Tran (anrg.usc.edu) <jasontra@usc.edu>
 *
 * @}
  */

#include <stdio.h>
#include "board.h"
#include "cpu.h"
#include "sched.h"
#include "thread.h"
#include "periph_conf.h"
#include "cpu_conf.h"
#include "periph/adc.h"
#include "cc2538.h"
#include <debug.h>

enum
{
    EOC = 1,
    STSEL = 3,
};

int adc_init(adc_t line)
{
    /* make sure the given ADC line is valid */
    if (line >= ADC_NUMOF) {
        return -1;
    }

    cc2538_soc_adc_t *adcaccess;
    adcaccess = SOC_ADC;

    /* Setting up ADC */
    gpio_hardware_control(GPIO_PXX_TO_NUM(PORT_A, line));
    IOC_PXX_OVER[GPIO_PXX_TO_NUM(PORT_A, line)] = IOC_OVERRIDE_ANA;
    
    /* Start conversions when ADCCON1.ST = 1 */
    adcaccess->cc2538_adc_adccon1.ADCCON1 |= adcaccess->cc2538_adc_adccon1.ADCCON1bits.STSEL;

    IOC_PXX_OVER[GPIO_PXX_TO_NUM(PORT_A, line)] = IOC_OVERRIDE_ANA;

    return 0;
}

int adc_sample(adc_t line, adc_res_t res)
{
    /* make sure the given ADC line is valid */
    if (line >= ADC_NUMOF) {
        return -1;
    }
    
    cc2538_soc_adc_t *adcaccess = SOC_ADC;
    int16_t result;

    /* Note - This has been hard coded .
     *  Can choose from any of the choices below:
     *  SOC_ADC_ADCCON_REF_INT or
     *  SOC_ADC_ADCCON_REF_EXT_SINGLE or
     *  SOC_ADC_ADCCON_REF_AVDD5
     */
    uint8_t refvoltage = SOC_ADC_ADCCON_REF_AVDD5;

    /* Start a single extra conversion with the given parameters. */
    adcaccess->ADCCON3 = ((adcaccess->ADCCON3) &
        ~(SOC_ADC_ADCCON3_EREF | SOC_ADC_ADCCON3_EDIV | SOC_ADC_ADCCON3_ECH)) |
            refvoltage | res | line;

    gpio_set(GPIO_PIN(PORT_D,2));
    /* Poll until end of conversion */
    while ((adcaccess->cc2538_adc_adccon1.ADCCON1 & 0x80) == 0);
    gpio_clear(GPIO_PIN(PORT_D,2));

    /* Read conversion result, reading SOC_ADC_ADCH last to clear
        SOC_ADC_ADCCON1.EOC */
    result  = (((adcaccess->ADCL) & 0xfc));
    result |= (((adcaccess->ADCH) & 0xff) << 8);

    switch (res)
    {
        case ADC_RES_7BIT:
            result = result >> SOCADC_7_BIT_RSHIFT;
            break;
        case ADC_RES_9BIT:
            result = result >> SOCADC_9_BIT_RSHIFT;
            break;
        case ADC_RES_10BIT:
            result = result >> SOCADC_10_BIT_RSHIFT;
            break;
        case ADC_RES_12BIT:
            result = result >> SOCADC_12_BIT_RSHIFT;
            break;
        default:
            return -1;
    }

    /* Return conversion result */
    return (int)result;
}

int adc_start_sampling(adc_t line, adc_res_t res)
{
    /* make sure the given ADC line is valid */
    if (line >= ADC_NUMOF) {
        return -1;
    }
    
    cc2538_soc_adc_t *adcaccess = SOC_ADC;

    /* Note - This has been hard coded .
     *  Can choose from any of the choices below:
     *  SOC_ADC_ADCCON_REF_INT or
     *  SOC_ADC_ADCCON_REF_EXT_SINGLE or
     *  SOC_ADC_ADCCON_REF_AVDD5
     */
    uint8_t refvoltage = SOC_ADC_ADCCON_REF_AVDD5;

    /* Start a single extra conversion with the given parameters. */
    adcaccess->ADCCON3 = ((adcaccess->ADCCON3) &
        ~(SOC_ADC_ADCCON3_EREF | SOC_ADC_ADCCON3_EDIV | SOC_ADC_ADCCON3_ECH)) |
            refvoltage | res | line;

    return 1;
}

int adc_is_sample_ready(adc_t line, adc_res_t res)
{
    cc2538_soc_adc_t *adcaccess = SOC_ADC;

    if ((adcaccess->cc2538_adc_adccon1.ADCCON1 & 0x80) == 0) {
        return 0;
    }

    return 1;
}

uint16_t adc_read_sample(adc_t line, adc_res_t res)
{
    cc2538_soc_adc_t *adcaccess = SOC_ADC;
    uint16_t result;

    /* Read conversion result, reading SOC_ADC_ADCH last to clear
        SOC_ADC_ADCCON1.EOC */
    result  = (((adcaccess->ADCL) & 0xfc));
    result |= (((adcaccess->ADCH) & 0xff) << 8);

    switch (res)
    {
        case ADC_RES_7BIT:
            result = result >> SOCADC_7_BIT_RSHIFT;
            break;
        case ADC_RES_9BIT:
            result = result >> SOCADC_9_BIT_RSHIFT;
            break;
        case ADC_RES_10BIT:
            result = result >> SOCADC_10_BIT_RSHIFT;
            break;
        case ADC_RES_12BIT:
            result = result >> SOCADC_12_BIT_RSHIFT;
            break;
        default:
            return -1;
    }

    /* Return conversion result */
    return (int)result;
}