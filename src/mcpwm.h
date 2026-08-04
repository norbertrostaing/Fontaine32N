#pragma once

#include <Arduino.h>
#include "driver/mcpwm.h"

// API interne utilisée uniquement pour régler la période en ticks
#include "hal/mcpwm_ll.h"

class StrobePWM
{
public:
    void begin(
        int pin1,
        int pin2,
        float frequencyHz = 60.0f)
    {
        ESP_ERROR_CHECK(
            mcpwm_gpio_init(
                MCPWM_UNIT_0,
                MCPWM0A,
                pin1
            )
        );

        ESP_ERROR_CHECK(
            mcpwm_gpio_init(
                MCPWM_UNIT_0,
                MCPWM0B,
                pin2
            )
        );

        mcpwm_config_t config = {};

        // Valeur entière uniquement pour démarrer le périphérique
        config.frequency = 60;

        config.cmpr_a = 0.0f;
        config.cmpr_b = 0.0f;

        config.counter_mode = MCPWM_UP_COUNTER;
        config.duty_mode = MCPWM_DUTY_MODE_0;

        ESP_ERROR_CHECK(
            mcpwm_init(
                MCPWM_UNIT_0,
                MCPWM_TIMER_0,
                &config
            )
        );

        /*
         * Période et comparateurs appliqués ensemble
         * au début du cycle PWM suivant.
         */
        portENTER_CRITICAL(&mux_);

        mcpwm_ll_timer_enable_update_period_on_tez(
            hardware_,
            0,
            true
        );

        mcpwm_ll_operator_enable_update_compare_on_tez(
            hardware_,
            0,
            0,
            true
        );

        mcpwm_ll_operator_enable_update_compare_on_tez(
            hardware_,
            0,
            1,
            true
        );

        portEXIT_CRITICAL(&mux_);

        setFrequency(frequencyHz);
    }

    void setFrequency(float frequencyHz)
    {
        frequencyHz_ = frequencyHz;
        apply();
    }

    void setDuty1(int value)
    {
        duty1_ = constrain(value, 0, 1023);
        apply();
    }

    void setDuty2(int value)
    {
        duty2_ = constrain(value, 0, 1023);
        apply();
    }

    void set(
        float frequencyHz,
        int duty1,
        int duty2)
    {
        duty1_ = constrain(duty1, 0, 1023);
        duty2_ = constrain(duty2, 0, 1023);

        apply();
    }

    float getActualFrequency() const
    {
        return actualFrequencyHz_;
    }

private:
    static constexpr uint32_t TIMER_RESOLUTION_HZ = 1000000UL;

    mcpwm_dev_t* hardware_ = MCPWM_LL_GET_HW(0);

    portMUX_TYPE mux_ = portMUX_INITIALIZER_UNLOCKED;

    float frequencyHz_ = 60.0f;
    float actualFrequencyHz_ = 60.0f;

    int duty1_ = 0;
    int duty2_ = 0;

    void apply()
    {
        const uint32_t periodTicks = lroundf(
            static_cast<float>(TIMER_RESOLUTION_HZ)
            / frequencyHz_
        );

        const uint32_t compare1 =
            static_cast<uint64_t>(periodTicks) * duty1_
            / 1023UL;

        const uint32_t compare2 =
            static_cast<uint64_t>(periodTicks) * duty2_
            / 1023UL;

        portENTER_CRITICAL(&mux_);

        /*
         * Ces valeurs sont écrites dans les registres d’ombre.
         * Elles deviennent actives ensemble au prochain TEZ,
         * c’est-à-dire lorsque le compteur revient à zéro.
         */
        mcpwm_ll_operator_set_compare_value(
            hardware_,
            0,
            0,
            compare1
        );

        mcpwm_ll_operator_set_compare_value(
            hardware_,
            0,
            1,
            compare2
        );

        mcpwm_ll_timer_set_peak(
            hardware_,
            0,
            periodTicks,
            false
        );

        portEXIT_CRITICAL(&mux_);

        actualFrequencyHz_ =
            static_cast<float>(TIMER_RESOLUTION_HZ)
            / periodTicks;
    }
};