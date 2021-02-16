/*
* @brief Contains board specific variables and initialization functions
*/

#include <board.h>

#include <odrive_main.h>

#include <Drivers/STM32/stm32_adc.hpp>
#include <Drivers/STM32/stm32_basic_pwm_output.hpp>
#include <Drivers/STM32/stm32_can.hpp>
#include <Drivers/STM32/stm32_nvm_file.hpp>
#include <Drivers/STM32/stm32_pwm_input.hpp>
#include <Drivers/STM32/stm32_spi.hpp>
#include <Drivers/STM32/stm32_timer.hpp>
#include <Drivers/STM32/stm32_usart.hpp>

#include <adc.h>
#include <tim.h>
#include <freertos_vars.h>

// this should technically be in task_timer.cpp but let's not make a one-line file
bool TaskTimer::enabled = false;

extern "C" void SystemClock_Config(void); // defined in main.c generated by CubeMX

#define ControlLoop_IRQHandler OTG_HS_IRQHandler
#define ControlLoop_IRQn OTG_HS_IRQn

/* System Settings -----------------------------------------------------------*/

// This array is placed at the very start of the ram (0x20000000) and will be
// used during manufacturing to test the struct that will go to the OTP before
// _actually_ putting anything into OTP. This avoids bulk-destroying STM32's if
// we introduce unintended breakage in our manufacturing scripts.
uint8_t __attribute__((section(".testdata"))) fake_otp[FLASH_OTP_END + 1 - FLASH_OTP_BASE];

// refer to page 75 of reference manual:
// http://www.st.com/content/ccc/resource/technical/document/reference_manual/3d/6d/5a/66/b4/99/40/d4/DM00031020.pdf/files/DM00031020.pdf/jcr:content/translations/en.DM00031020.pdf
std::array<uint32_t, 2> nvm_sectors = {FLASH_SECTOR_10, FLASH_SECTOR_11};
Stm32NvmFile nvm_impl = {
    0, nvm_sectors.data(), nvm_sectors.size(), 0x080C0000UL, 0x40000UL
};
template<> File& BoardSupportPackage::nvm = nvm_impl;


/* Internal GPIOs ------------------------------------------------------------*/

Stm32Gpio drv0_ncs_gpio = {GPIOC, GPIO_PIN_13};
Stm32Gpio drv1_ncs_gpio = {GPIOC, GPIO_PIN_14};
Stm32Gpio drv_nfault_gpio = {GPIOD, GPIO_PIN_2};
Stm32Gpio drv_en_gate_gpio = {GPIOB, GPIO_PIN_12};

Stm32Gpio spi_miso_gpio = {GPIOC, GPIO_PIN_11};
Stm32Gpio spi_mosi_gpio = {GPIOC, GPIO_PIN_12};
Stm32Gpio spi_clk_gpio = {GPIOC, GPIO_PIN_10};

#if (HW_VERSION_MINOR == 1) || (HW_VERSION_MINOR == 2)
Stm32Gpio vbus_s_gpio = {GPIOA, GPIO_PIN_0};
Stm32Gpio aux_fet_temp_gpio = {GPIOC, GPIO_PIN_4};
const std::array<Stm32Gpio, 2> fet_thermistor_gpios = {{{GPIOC, GPIO_PIN_5},
                                                        {GPIOA, GPIO_PIN_1}}};
#elif (HW_VERSION_MINOR == 3) || (HW_VERSION_MINOR == 4)
Stm32Gpio vbus_s_gpio = {GPIOA, GPIO_PIN_6};
Stm32Gpio aux_fet_temp_gpio = {GPIOC, GPIO_PIN_4};
const std::array<Stm32Gpio, 2> fet_thermistor_gpios = {{{GPIOC, GPIO_PIN_5},
                                                        {GPIOA, GPIO_PIN_4}}};
#elif (HW_VERSION_MINOR == 5) || (HW_VERSION_MINOR == 6)
Stm32Gpio vbus_s_gpio = {GPIOA, GPIO_PIN_6};
Stm32Gpio aux_fet_temp_gpio = {GPIOA, GPIO_PIN_5};
const std::array<Stm32Gpio, 2> fet_thermistor_gpios = {{{GPIOC, GPIO_PIN_5},
                                                        {GPIOA, GPIO_PIN_4}}};
#endif

Stm32Gpio m0_sob_gpio = {GPIOC, GPIO_PIN_0};
Stm32Gpio m0_soc_gpio = {GPIOC, GPIO_PIN_1};
Stm32Gpio m1_sob_gpio = {GPIOC, GPIO_PIN_3};
Stm32Gpio m1_soc_gpio = {GPIOC, GPIO_PIN_2};


/* External GPIOs ------------------------------------------------------------*/

#if (HW_VERSION_MINOR == 1) || (HW_VERSION_MINOR == 2)
template<> const std::array<Stm32Gpio, GPIO_COUNT> BoardSupportPackage::gpios = {{
    {nullptr, 0}, // dummy GPIO0 so that PCB labels and software numbers match

    {GPIOB, GPIO_PIN_2}, // GPIO1
    {GPIOA, GPIO_PIN_5}, // GPIO2
    {GPIOA, GPIO_PIN_4}, // GPIO3
    {GPIOA, GPIO_PIN_3}, // GPIO4
    {nullptr, 0}, // GPIO5 (doesn't exist on this board)
    {nullptr, 0}, // GPIO6 (doesn't exist on this board)
    {nullptr, 0}, // GPIO7 (doesn't exist on this board)
    {nullptr, 0}, // GPIO8 (doesn't exist on this board)

    {GPIOB, GPIO_PIN_4}, // ENC0_A
    {GPIOB, GPIO_PIN_5}, // ENC0_B
    {GPIOA, GPIO_PIN_15}, // ENC0_Z
    {GPIOB, GPIO_PIN_6}, // ENC1_A
    {GPIOB, GPIO_PIN_7}, // ENC1_B
    {GPIOB, GPIO_PIN_3}, // ENC1_Z
    {GPIOB, GPIO_PIN_8}, // CAN_R
    {GPIOB, GPIO_PIN_9}, // CAN_D
}};
#elif (HW_VERSION_MINOR == 3) || (HW_VERSION_MINOR == 4)
template<> const std::array<Stm32Gpio, GPIO_COUNT> BoardSupportPackage::gpios = {{
    {nullptr, 0}, // dummy GPIO0 so that PCB labels and software numbers match

    {GPIOA, GPIO_PIN_0}, // GPIO1
    {GPIOA, GPIO_PIN_1}, // GPIO2
    {GPIOA, GPIO_PIN_2}, // GPIO3
    {GPIOA, GPIO_PIN_3}, // GPIO4
    {GPIOB, GPIO_PIN_2}, // GPIO5
    {nullptr, 0}, // GPIO6 (doesn't exist on this board)
    {nullptr, 0}, // GPIO7 (doesn't exist on this board)
    {nullptr, 0}, // GPIO8 (doesn't exist on this board)

    {GPIOB, GPIO_PIN_4}, // ENC0_A
    {GPIOB, GPIO_PIN_5}, // ENC0_B
    {GPIOA, GPIO_PIN_15}, // ENC0_Z
    {GPIOB, GPIO_PIN_6}, // ENC1_A
    {GPIOB, GPIO_PIN_7}, // ENC1_B
    {GPIOB, GPIO_PIN_3}, // ENC1_Z
    {GPIOB, GPIO_PIN_8}, // CAN_R
    {GPIOB, GPIO_PIN_9}, // CAN_D
}};
#elif (HW_VERSION_MINOR == 5) || (HW_VERSION_MINOR == 6)
template<> const std::array<Stm32Gpio, GPIO_COUNT> BoardSupportPackage::gpios = {{
    {nullptr, 0}, // dummy GPIO0 so that PCB labels and software numbers match

    {GPIOA, GPIO_PIN_0}, // GPIO1
    {GPIOA, GPIO_PIN_1}, // GPIO2
    {GPIOA, GPIO_PIN_2}, // GPIO3
    {GPIOA, GPIO_PIN_3}, // GPIO4
    {GPIOC, GPIO_PIN_4}, // GPIO5
    {GPIOB, GPIO_PIN_2}, // GPIO6
    {GPIOA, GPIO_PIN_15}, // GPIO7
    {GPIOB, GPIO_PIN_3}, // GPIO8
    
    {GPIOB, GPIO_PIN_4}, // ENC0_A
    {GPIOB, GPIO_PIN_5}, // ENC0_B
    {GPIOC, GPIO_PIN_9}, // ENC0_Z
    {GPIOB, GPIO_PIN_6}, // ENC1_A
    {GPIOB, GPIO_PIN_7}, // ENC1_B
    {GPIOC, GPIO_PIN_15}, // ENC1_Z
    {GPIOB, GPIO_PIN_8}, // CAN_R
    {GPIOB, GPIO_PIN_9}, // CAN_D
}};
#else
#error "unknown GPIOs"
#endif

std::array<GpioFunction, 3> alternate_functions[GPIO_COUNT] = {
    /* GPIO0 (inexistent): */ {{}},

#if HW_VERSION_MINOR >= 3
    /* GPIO1: */ {{{ODrive::GPIO_MODE_UART_A, GPIO_AF8_UART4}, {ODrive::GPIO_MODE_PWM, GPIO_AF2_TIM5}}},
    /* GPIO2: */ {{{ODrive::GPIO_MODE_UART_A, GPIO_AF8_UART4}, {ODrive::GPIO_MODE_PWM, GPIO_AF2_TIM5}}},
    /* GPIO3: */ {{{ODrive::GPIO_MODE_UART_B, GPIO_AF7_USART2}, {ODrive::GPIO_MODE_PWM, GPIO_AF2_TIM5}}},
#else
    /* GPIO1: */ {{}},
    /* GPIO2: */ {{}},
    /* GPIO3: */ {{}},
#endif

    /* GPIO4: */ {{{ODrive::GPIO_MODE_UART_B, GPIO_AF7_USART2}, {ODrive::GPIO_MODE_PWM, GPIO_AF2_TIM5}}},
    /* GPIO5: */ {{}},
    /* GPIO6: */ {{}},
    /* GPIO7: */ {{}},
    /* GPIO8: */ {{}},
    /* ENC0_A: */ {{{ODrive::GPIO_MODE_ENC0, GPIO_AF2_TIM3}}},
    /* ENC0_B: */ {{{ODrive::GPIO_MODE_ENC0, GPIO_AF2_TIM3}}},
    /* ENC0_Z: */ {{}},
    /* ENC1_A: */ {{{ODrive::GPIO_MODE_I2C_A, GPIO_AF4_I2C1}, {ODrive::GPIO_MODE_ENC1, GPIO_AF2_TIM4}}},
    /* ENC1_B: */ {{{ODrive::GPIO_MODE_I2C_A, GPIO_AF4_I2C1}, {ODrive::GPIO_MODE_ENC1, GPIO_AF2_TIM4}}},
    /* ENC1_Z: */ {{}},
    /* CAN_R: */ {{{ODrive::GPIO_MODE_CAN_A, GPIO_AF9_CAN1}, {ODrive::GPIO_MODE_I2C_A, GPIO_AF4_I2C1}}},
    /* CAN_D: */ {{{ODrive::GPIO_MODE_CAN_A, GPIO_AF9_CAN1}, {ODrive::GPIO_MODE_I2C_A, GPIO_AF4_I2C1}}},
};

#if HW_VERSION_MINOR < 3
template<> const int BoardSupportPackage::uart_tx_gpios[] = {};
template<> const int BoardSupportPackage::uart_rx_gpios[] = {};
#else
template<> const int BoardSupportPackage::uart_tx_gpios[] = {1, 3};
template<> const int BoardSupportPackage::uart_rx_gpios[] = {2, 4};
#endif

template<> const int BoardSupportPackage::inc_enc_a_gpios[] = {9, 12};
template<> const int BoardSupportPackage::inc_enc_b_gpios[] = {10, 13};
uint32_t inc_enc_af[] = {GPIO_AF2_TIM3, GPIO_AF2_TIM4};

// The SPI pins are hardwired and not part of the GPIO numbering scheme.
template<> const int BoardSupportPackage::spi_miso_gpios[] = {-1};
template<> const int BoardSupportPackage::spi_mosi_gpios[] = {-1};
template<> const int BoardSupportPackage::spi_sck_gpios[] = {-1};

template<> const int BoardSupportPackage::can_r_gpios[] = {15};
template<> const int BoardSupportPackage::can_d_gpios[] = {16};


/* DMA Streams ---------------------------------------------------------------*/

Stm32DmaStreamRef spi_rx_dma = {DMA1_Stream0};
Stm32DmaStreamRef spi_tx_dma = {DMA1_Stream7};
Stm32DmaStreamRef uart_a_rx_dma = {DMA1_Stream2};
Stm32DmaStreamRef uart_a_tx_dma = {DMA1_Stream4};
Stm32DmaStreamRef uart_b_rx_dma = {DMA1_Stream5};
Stm32DmaStreamRef uart_b_tx_dma = {DMA1_Stream6};


/* ADCs ----------------------------------------------------------------------*/

#if (HW_VERSION_MINOR == 1) || (HW_VERSION_MINOR == 2)
std::array<int, 3> adc_gpios = {2, 3, 4};
//std::array<int, 3> gpio_adc_channels = {5, 4, 3};
#elif (HW_VERSION_MINOR == 3) || (HW_VERSION_MINOR == 4)
std::array<int, 4> adc_gpios = {1, 2, 3, 4};
//std::array<int, 4> gpio_adc_channels = {0, 1, 2, 3};
#elif (HW_VERSION_MINOR == 5) || (HW_VERSION_MINOR == 6)
std::array<int, 5> adc_gpios = {1, 2, 3, 4, 5};
//std::array<int, 5> gpio_adc_channels = {0, 1, 2, 3, 14};
#else
#error "unknown ADC channels"
#endif

constexpr float kAdcFullScale = static_cast<float>(1UL << 12UL);

#if HW_VERSION_VOLTAGE >= 48
#define VBUS_S_DIVIDER_RATIO 19.0f
#elif HW_VERSION_VOLTAGE == 24
#define VBUS_S_DIVIDER_RATIO 11.0f
#else
#error "unknown board voltage"
#endif


/* Communication interfaces --------------------------------------------------*/

Stm32Spi spi = {SPI3, spi_rx_dma, spi_tx_dma};
Stm32SpiArbiter spi_arbiter{&spi};

#if HW_VERSION_MINOR < 3
Stm32Usart uart_impl[] = {};
template<> const std::array<Stm32Usart*, board.UART_COUNT> BoardSupportPackage::uarts = {};
uint32_t uart_af[] = {};
#else
Stm32Usart uart_impl[] = {
    {UART4, uart_a_rx_dma, uart_a_tx_dma},
    {USART2, uart_b_rx_dma, uart_b_tx_dma},
};
template<> const std::array<Stm32Usart*, board.UART_COUNT> BoardSupportPackage::uarts = {&uart_impl[0], &uart_impl[1]};
uint32_t uart_af[] = {GPIO_AF8_UART4, GPIO_AF7_USART2};
#endif


DEFINE_STM32_CAN(can_a, CAN1);

template<> const std::array<CanBusBase*, 1> BoardSupportPackage::can_busses = {&can_a};

#if HW_VERSION_MINOR <= 2
int pwm_gpios[] = {};
#else
int pwm_gpios[] = {1, 2, 3, 4};
#endif

Stm32PwmInput pwm_inputs[] = {
#if HW_VERSION_MINOR > 2
    {&htim5, {BoardSupportPackage::gpios[pwm_gpios[0]], BoardSupportPackage::gpios[pwm_gpios[1]], BoardSupportPackage::gpios[pwm_gpios[2]], BoardSupportPackage::gpios[pwm_gpios[3]]}},
#endif
};

extern PCD_HandleTypeDef hpcd_USB_OTG_FS; // defined in usbd_conf.c
USBD_HandleTypeDef usb_dev_handle;


/* Onboard devices -----------------------------------------------------------*/

Drv8301 m0_gate_driver{
    &spi_arbiter,
    drv0_ncs_gpio, // nCS
    {}, // EN pin (shared between both motors, therefore we actuate it outside of the drv8301 driver)
    drv_nfault_gpio // nFAULT pin (shared between both motors)
};

Drv8301 m1_gate_driver{
    &spi_arbiter,
    drv1_ncs_gpio, // nCS
    {}, // EN pin (shared between both motors, therefore we actuate it outside of the drv8301 driver)
    drv_nfault_gpio // nFAULT pin (shared between both motors)
};

const std::array<float, 4> fet_thermistor_poly_coeffs =
    {363.93910201f, -462.15369634f, 307.55129571f, -27.72569531f};

Motor motors[AXIS_COUNT] = {
    {
        &htim1, // timer
        0b110, // current_sensor_mask
        1.0f / SHUNT_RESISTANCE, // shunt_conductance [S]
        m0_gate_driver, // gate_driver
        m0_gate_driver, // opamp
        &board.motor_fet_temperatures[0],
    },
    {
        &htim8, // timer
        0b110, // current_sensor_mask
        1.0f / SHUNT_RESISTANCE, // shunt_conductance [S]
        m1_gate_driver, // gate_driver
        m1_gate_driver, // opamp
        &board.motor_fet_temperatures[1],
    }
};

Encoder encoders[AXIS_COUNT] = {
    {
        &htim3, // timer
        board.gpios[11], // index_gpio
        board.gpios[9], // hallA_gpio
        board.gpios[10], // hallB_gpio
        board.gpios[11], // hallC_gpio
        &spi_arbiter // spi_arbiter
    },
    {
        &htim4, // timer
        board.gpios[14], // index_gpio
        board.gpios[12], // hallA_gpio
        board.gpios[13], // hallB_gpio
        board.gpios[14], // hallC_gpio
        &spi_arbiter // spi_arbiter
    }
};

// TODO: this has no hardware dependency and should be allocated depending on config
Endstop endstops[2 * AXIS_COUNT];
MechanicalBrake mechanical_brakes[AXIS_COUNT];

SensorlessEstimator sensorless_estimators[AXIS_COUNT];
Controller controllers[AXIS_COUNT];
TrapezoidalTrajectory trap[AXIS_COUNT];

std::array<Axis, AXIS_COUNT> axes{{
    {
        0, // axis_num
        1, // step_gpio_pin
        2, // dir_gpio_pin
        (osPriority)(osPriorityHigh + (osPriority)1), // thread_priority
        encoders[0], // encoder
        sensorless_estimators[0], // sensorless_estimator
        controllers[0], // controller
        motors[0], // motor
        trap[0], // trap
        endstops[0], endstops[1], // min_endstop, max_endstop
        mechanical_brakes[0], // mechanical brake
    },
    {
        1, // axis_num
#if HW_VERSION_MAJOR == 3 && HW_VERSION_MINOR >= 5
        7, // step_gpio_pin
        8, // dir_gpio_pin
#else
        3, // step_gpio_pin
        4, // dir_gpio_pin
#endif
        osPriorityHigh, // thread_priority
        encoders[1], // encoder
        sensorless_estimators[1], // sensorless_estimator
        controllers[1], // controller
        motors[1], // motor
        trap[1], // trap
        endstops[2], endstops[3], // min_endstop, max_endstop
        mechanical_brakes[1], // mechanical brake
    },
}};

Stm32BasicPwmOutput<TIM_APB1_PERIOD_CLOCKS, TIM_APB1_DEADTIME_CLOCKS> brake_resistor_output_impl{TIM2->CCR3, TIM2->CCR4};
PwmOutputGroup<1>& brake_resistor_output = brake_resistor_output_impl;

template<> PwmOutputGroup<1>* BoardSupportPackage::fan_output = nullptr;


/* Misc Variables ------------------------------------------------------------*/

volatile uint32_t& board_control_loop_counter = TIM13->CNT;
uint32_t board_control_loop_counter_period = CONTROL_TIMER_PERIOD_TICKS / 2; // TIM13 is on a clock that's only half as fast as TIM1

constexpr size_t ADC_CHANNEL_COUNT = adc_gpios.size() + AXIS_COUNT; // Sample all ADC-capable GPIOs and one FET thermistor per axis.
std::array<uint16_t, ADC_CHANNEL_COUNT> adc_measurements_ = {};

BoardSupportPackage board;


static bool check_board_version(const uint8_t* otp_ptr) {
    return (otp_ptr[3] == HW_VERSION_MAJOR) &&
           (otp_ptr[4] == HW_VERSION_MINOR) &&
           (otp_ptr[5] == HW_VERSION_VOLTAGE);
}

template<> bool BoardSupportPackage::init() {
    // Reset of all peripherals, Initializes the Flash interface and the Systick.
    HAL_Init();

    // Configure the system clock
    SystemClock_Config();

    // If the OTP is pristine, use the fake-otp in RAM instead
    const uint8_t* otp_ptr = (const uint8_t*)FLASH_OTP_BASE;
    if (*otp_ptr == 0xff) {
        otp_ptr = fake_otp;
    }

    // Ensure that the board version for which this firmware is compiled matches
    // the board we're running on.
    if (!check_board_version(otp_ptr)) {
        for (;;);
    }

    // Init DMA interrupts

    nvic.enable_with_prio(spi_rx_dma.get_irqn(), 4);
    nvic.enable_with_prio(spi_tx_dma.get_irqn(), 3);

    // Init internal GPIOs (external GPIOs are initialized in config())

    vbus_s_gpio.config(GPIO_MODE_ANALOG, GPIO_NOPULL);
    aux_fet_temp_gpio.config(GPIO_MODE_ANALOG, GPIO_NOPULL);
    for (Stm32Gpio gpio: fet_thermistor_gpios) {
        gpio.config(GPIO_MODE_ANALOG, GPIO_NOPULL);
    }
    m0_sob_gpio.config(GPIO_MODE_ANALOG, GPIO_NOPULL);
    m0_soc_gpio.config(GPIO_MODE_ANALOG, GPIO_NOPULL);
    m1_sob_gpio.config(GPIO_MODE_ANALOG, GPIO_NOPULL);
    m1_soc_gpio.config(GPIO_MODE_ANALOG, GPIO_NOPULL);

    drv0_ncs_gpio.enable_clock();
    drv0_ncs_gpio.write(true);
    drv0_ncs_gpio.config(GPIO_MODE_OUTPUT_PP, GPIO_NOPULL);
    drv1_ncs_gpio.enable_clock();
    drv1_ncs_gpio.write(true);
    drv1_ncs_gpio.config(GPIO_MODE_OUTPUT_PP, GPIO_NOPULL);
    drv_nfault_gpio.config(GPIO_MODE_INPUT, GPIO_PULLUP);
    drv_en_gate_gpio.write(false);
    drv_en_gate_gpio.config(GPIO_MODE_OUTPUT_PP, GPIO_NOPULL);

    spi_miso_gpio.config(GPIO_MODE_AF_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_VERY_HIGH, GPIO_AF6_SPI3);
    spi_mosi_gpio.config(GPIO_MODE_AF_PP, GPIO_PULLDOWN, GPIO_SPEED_FREQ_VERY_HIGH, GPIO_AF6_SPI3);
    spi_clk_gpio.config(GPIO_MODE_AF_PP, GPIO_PULLDOWN, GPIO_SPEED_FREQ_VERY_HIGH, GPIO_AF6_SPI3);

    // Call CubeMX-generated peripheral initialization functions

    MX_ADC1_Init();
    MX_ADC2_Init();
    MX_TIM1_Init();
    MX_TIM8_Init();
    MX_TIM3_Init();
    MX_TIM4_Init();
    MX_ADC3_Init();
    MX_TIM2_Init();
    MX_TIM5_Init();
    MX_TIM13_Init();

    // Init USB peripheral control driver
    hpcd_USB_OTG_FS.pData = &usb_dev_handle;
    usb_dev_handle.pData = &hpcd_USB_OTG_FS;
    
    hpcd_USB_OTG_FS.Instance = USB_OTG_FS;
    hpcd_USB_OTG_FS.Init.dev_endpoints = 6;
    hpcd_USB_OTG_FS.Init.speed = PCD_SPEED_FULL;
    hpcd_USB_OTG_FS.Init.dma_enable = DISABLE;
    hpcd_USB_OTG_FS.Init.ep0_mps = DEP0CTL_MPS_64;
    hpcd_USB_OTG_FS.Init.phy_itface = PCD_PHY_EMBEDDED;
    hpcd_USB_OTG_FS.Init.Sof_enable = DISABLE;
    hpcd_USB_OTG_FS.Init.low_power_enable = DISABLE;
    hpcd_USB_OTG_FS.Init.lpm_enable = DISABLE;
    hpcd_USB_OTG_FS.Init.vbus_sensing_enable = DISABLE;
    hpcd_USB_OTG_FS.Init.use_dedicated_ep1 = DISABLE;
    if (HAL_PCD_Init(&hpcd_USB_OTG_FS) != HAL_OK) {
        return false;
    }

    HAL_PCDEx_SetRxFiFo(&hpcd_USB_OTG_FS, 0x80);
    HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_FS, 0, 0x40);
    HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_FS, 1, 0x40); // CDC IN endpoint
    HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_FS, 3, 0x40); // ODrive IN endpoint

    // External interrupt lines are individually enabled in stm32_gpio.cpp
    nvic.enable_with_prio(EXTI0_IRQn, 1);
    nvic.enable_with_prio(EXTI1_IRQn, 1);
    nvic.enable_with_prio(EXTI2_IRQn, 1);
    nvic.enable_with_prio(EXTI3_IRQn, 1);
    nvic.enable_with_prio(EXTI4_IRQn, 1);
    nvic.enable_with_prio(EXTI9_5_IRQn, 1);
    nvic.enable_with_prio(EXTI15_10_IRQn, 1);

    nvic.enable_with_prio(ControlLoop_IRQn, 5);
    nvic.enable_with_prio(TIM8_UP_TIM13_IRQn, 0);

    if (!spi.init()) {
        return false;
    }

    // Ensure that debug halting of the core doesn't leave the motor PWM running
    __HAL_DBGMCU_FREEZE_TIM1();
    __HAL_DBGMCU_FREEZE_TIM8();
    __HAL_DBGMCU_FREEZE_TIM13();

    // Start brake resistor PWM in floating output configuration
    htim2.Instance->CCR3 = 0;
    htim2.Instance->CCR4 = TIM_APB1_PERIOD_CLOCKS + 1;
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4);

    // Enable ADC
    __HAL_ADC_ENABLE(&hadc1);
    __HAL_ADC_ENABLE(&hadc2);
    __HAL_ADC_ENABLE(&hadc3);

    // Configure ADC1 regular sequence to sample GPIO ADCs and the FET
    // thermistors using DMA. The sequence will be triggered by TIM8 TRGO.

    std::array<int, ADC_CHANNEL_COUNT> channels;
    size_t i = 0;

    for (int gpio: adc_gpios) {
        channels[i++] = Stm32Adc::channel_from_gpio(ADC1, board.gpios[gpio]);
    }
    for (Stm32Gpio gpio: fet_thermistor_gpios) {
        channels[i++] = Stm32Adc::channel_from_gpio(ADC1, gpio);
    }

    Stm32Adc{&hadc1}.set_regular_sequence(channels.data(), channels.size());
    HAL_ADC_Start_DMA(&hadc1, reinterpret_cast<uint32_t*>(adc_measurements_.data()), ADC_CHANNEL_COUNT);

    if (!nvm_impl.init()) {
        return false;
    }

    return true;
}

template<> bool BoardSupportPackage::config(const BoardConfig& config) {
    if (!validate_gpios(config)) {
        return false;
    }

    // Cannot disable SPI because it's used by gate drivers
    if (!config.spi_config[0].enabled) {
        return false;
    }

    // Cannot enable CAN without enabling the corresponding GPIOs
    if (config.can_config[0].enabled && (config.can_config[0].r_gpio != can_r_gpios[0] || config.can_config[0].d_gpio != can_d_gpios[0])) {
        return false;
    }

    for (size_t i = 0; i < BoardTraits::_GPIO_COUNT; ++i) {
        switch (config.gpio_modes[i]) {
            case kAnalogInput: gpios[i].config(GPIO_MODE_ANALOG, GPIO_NOPULL); break;
            case kPwmInput: break; // initialized below
            case kDigitalInputNoPull: gpios[i].config(GPIO_MODE_INPUT, GPIO_NOPULL); break;
            case kDigitalInputPullUp: gpios[i].config(GPIO_MODE_INPUT, GPIO_PULLUP); break;
            case kDigitalInputPullDown: gpios[i].config(GPIO_MODE_INPUT, GPIO_PULLDOWN); break;
            case kDigitalOutput: gpios[i].config(GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_LOW); break;

            // Switch to Hi-Z for now. Will be overridden later.
            case kAlternateFunction: gpios[i].config(GPIO_MODE_ANALOG, GPIO_NOPULL); break;
            default: return false; // Should not happen because of preceding validate_gpio()
        }
    }

    std::array<bool, 4> enable_arr[sizeof(pwm_inputs) / sizeof(pwm_inputs[0])];

    for (size_t i = 0; i < sizeof(pwm_gpios) / sizeof(pwm_gpios[0]); ++i) {
        bool enable = pwm_gpios[i] >= 0 && config.gpio_modes[pwm_gpios[i]] == kPwmInput;
        enable_arr[i / 4][i % 4] = enable;
        if (enable) {
            gpios[pwm_gpios[i]].config(GPIO_MODE_AF_PP, GPIO_PULLDOWN, GPIO_SPEED_FREQ_LOW, GPIO_AF2_TIM5);
        }
    }

    for (size_t i = 0; i < sizeof(pwm_inputs) / sizeof(pwm_inputs[0]); ++i) {
        if (std::any_of(std::begin(enable_arr[i]), std::end(enable_arr[i]), [](auto& en) {return en;})) {
            nvic.enable_with_prio(Stm32Timer{pwm_inputs[i].get_timer()}.get_irqn(), 1);
            pwm_inputs[i].init(enable_arr[i]);
        }
    }

    for (size_t i = 0; i < BoardTraits::UART_COUNT; ++i) {
        if (config.uart_config[i].enabled) {
            nvic.enable_with_prio(Stm32DmaStreamRef{(DMA_Stream_TypeDef*)uart_impl[i].hdma_rx_.Instance}.get_irqn(), 10);
            nvic.enable_with_prio(Stm32DmaStreamRef{(DMA_Stream_TypeDef*)uart_impl[i].hdma_tx_.Instance}.get_irqn(), 10);
            nvic.enable_with_prio(uart_impl[i].get_irqn(), 10);

            if (!uart_impl[i].init(config.uart_config[i].baudrate)) {
                return false; // TODO: continue startup in degraded state
            }

            if (config.uart_config[i].tx_gpio >= 0) {
                gpios[uart_tx_gpios[i]].config(GPIO_MODE_AF_PP, GPIO_PULLDOWN, GPIO_SPEED_FREQ_VERY_HIGH, uart_af[i]);
            }
            if (config.uart_config[i].rx_gpio >= 0) {
                gpios[uart_rx_gpios[i]].config(GPIO_MODE_AF_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_VERY_HIGH, uart_af[i]);
            }
        }
    }

    if (config.can_config[0].enabled) {
        gpios[can_r_gpios[0]].config(GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_VERY_HIGH, GPIO_AF9_CAN1);
        gpios[can_d_gpios[0]].config(GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_VERY_HIGH, GPIO_AF9_CAN1);
        
        nvic.enable_with_prio(CAN1_TX_IRQn, 9);
        nvic.enable_with_prio(CAN1_RX0_IRQn, 9);
        nvic.enable_with_prio(CAN1_RX1_IRQn, 9);
        nvic.enable_with_prio(CAN1_SCE_IRQn, 9);

        if (!can_a.init({
            .Prescaler = 8,
            .Mode = CAN_MODE_NORMAL,
            .SyncJumpWidth = CAN_SJW_4TQ,
            .TimeSeg1 = CAN_BS1_16TQ,
            .TimeSeg2 = CAN_BS2_4TQ,
            .TimeTriggeredMode = DISABLE,
            .AutoBusOff = ENABLE,
            .AutoWakeUp = ENABLE,
            .AutoRetransmission = ENABLE,
            .ReceiveFifoLocked = DISABLE,
            .TransmitFifoPriority = DISABLE,
        }, 2000000UL)) {
            return false; // TODO: continue in degraded mode
        }
    }

    for (size_t i = 0; i < INC_ENC_COUNT; ++i) {
        if (config.inc_enc_config[i].enabled) {
            if (config.inc_enc_config[i].a_gpio >= 0) {
                gpios[inc_enc_a_gpios[i]].config(GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_LOW, inc_enc_af[i]);
            }
            if (config.inc_enc_config[i].b_gpio >= 0) {
                gpios[inc_enc_b_gpios[i]].config(GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_LOW, inc_enc_af[i]);
            }
        }
    }

    // Reset both DRV chips. The enable pin also controls the SPI interface, not
    // only the driver stages.
    drv_en_gate_gpio.write(false);
    delay_us(40); // mimumum pull-down time for full reset: 20us
    drv_en_gate_gpio.write(true);
    delay_us(20000); // mimumum pull-down time for full reset: 20us

    return true;
}

void start_timers() {
    CRITICAL_SECTION() {
        // Temporarily disable ADC triggers so they don't trigger as a side
        // effect of starting the timers.
        hadc1.Instance->CR2 &= ~(ADC_CR2_EXTEN | ADC_CR2_JEXTEN);
        hadc2.Instance->CR2 &= ~(ADC_CR2_EXTEN | ADC_CR2_JEXTEN);
        hadc3.Instance->CR2 &= ~(ADC_CR2_EXTEN | ADC_CR2_JEXTEN);

        /*
        * Synchronize TIM1, TIM8 and TIM13 such that:
        *  1. The triangle waveform of TIM1 leads the triangle waveform of TIM8 by a
        *     90° phase shift.
        *  2. Each TIM13 reload coincides with a TIM1 lower update event.
        */
        Stm32Timer::start_synchronously<3>(
            {&htim1, &htim8, &htim13},
            {TIM1_INIT_COUNT, 0, board_control_loop_counter}
        );

        hadc1.Instance->CR2 |= (ADC_EXTERNALTRIGCONVEDGE_RISING | ADC_EXTERNALTRIGINJECCONVEDGE_RISING);
        hadc2.Instance->CR2 |= (ADC_EXTERNALTRIGCONVEDGE_RISING | ADC_EXTERNALTRIGINJECCONVEDGE_RISING);
        hadc3.Instance->CR2 |= (ADC_EXTERNALTRIGCONVEDGE_RISING | ADC_EXTERNALTRIGINJECCONVEDGE_RISING);

        __HAL_ADC_CLEAR_FLAG(&hadc1, ADC_FLAG_JEOC);
        __HAL_ADC_CLEAR_FLAG(&hadc2, ADC_FLAG_JEOC);
        __HAL_ADC_CLEAR_FLAG(&hadc3, ADC_FLAG_JEOC);
        __HAL_ADC_CLEAR_FLAG(&hadc1, ADC_FLAG_EOC);
        __HAL_ADC_CLEAR_FLAG(&hadc2, ADC_FLAG_EOC);
        __HAL_ADC_CLEAR_FLAG(&hadc3, ADC_FLAG_EOC);
        __HAL_ADC_CLEAR_FLAG(&hadc1, ADC_FLAG_OVR);
        __HAL_ADC_CLEAR_FLAG(&hadc2, ADC_FLAG_OVR);
        __HAL_ADC_CLEAR_FLAG(&hadc3, ADC_FLAG_OVR);
        
        __HAL_TIM_CLEAR_IT(&htim8, TIM_IT_UPDATE);
        __HAL_TIM_ENABLE_IT(&htim8, TIM_IT_UPDATE);
    }
}

// Linear range of the DRV8301 opamp output: 0.3V...5.7V. We set the upper limit
// to 3.0V so that it's symmetric around the center point of 1.65V.
#define CURRENT_SENSE_MIN_VOLT  0.3f
#define CURRENT_SENSE_MAX_VOLT  3.0f

static constexpr auto CURRENT_ADC_LOWER_BOUND = (uint32_t)(kAdcFullScale * CURRENT_SENSE_MIN_VOLT / board.kAdcMaxVoltage);
static constexpr auto CURRENT_ADC_UPPER_BOUND = (uint32_t)(kAdcFullScale * CURRENT_SENSE_MAX_VOLT / board.kAdcMaxVoltage);

std::optional<float> phase_current_from_adcval(uint32_t ADCValue, float rev_gain) {
    // Make sure the measurements don't come too close to the current sensor's hardware limitations
    if (ADCValue < CURRENT_ADC_LOWER_BOUND || ADCValue > CURRENT_ADC_UPPER_BOUND) {
        motors[0].error_ |= Motor::ERROR_CURRENT_SENSE_SATURATION; // TODO make multi-axis
        return std::nullopt;
    }

    return ((float)ADCValue - kAdcFullScale / 2)
         * (board.kAdcMaxVoltage / kAdcFullScale)
         * rev_gain
         * (1.0f / SHUNT_RESISTANCE);
}

static bool fetch_and_reset_adcs(
        std::optional<Iph_ABC_t>* current0,
        std::optional<Iph_ABC_t>* current1) {
    bool all_adcs_done = (ADC1->SR & ADC_SR_JEOC) == ADC_SR_JEOC
        && (ADC2->SR & (ADC_SR_EOC | ADC_SR_JEOC)) == (ADC_SR_EOC | ADC_SR_JEOC)
        && (ADC3->SR & (ADC_SR_EOC | ADC_SR_JEOC)) == (ADC_SR_EOC | ADC_SR_JEOC);
    if (!all_adcs_done) {
        return false;
    }

    board.vbus_voltage = ADC1->JDR1 * (board.kAdcMaxVoltage * VBUS_S_DIVIDER_RATIO / kAdcFullScale);

    if (m0_gate_driver.is_ready()) {
        std::optional<float> phB = phase_current_from_adcval(ADC2->JDR1, motors[0].phase_current_rev_gain_);
        std::optional<float> phC = phase_current_from_adcval(ADC3->JDR1, motors[0].phase_current_rev_gain_);
        if (phB.has_value() && phC.has_value()) {
            *current0 = {-*phB - *phC, *phB, *phC};
        }
    }

    if (m1_gate_driver.is_ready()) {
        std::optional<float> phB = phase_current_from_adcval(ADC2->DR, motors[1].phase_current_rev_gain_);
        std::optional<float> phC = phase_current_from_adcval(ADC3->DR, motors[1].phase_current_rev_gain_);
        if (phB.has_value() && phC.has_value()) {
            *current1 = {-*phB - *phC, *phB, *phC};
        }
    }
    
    ADC1->SR = ~(ADC_SR_JEOC);
    ADC2->SR = ~(ADC_SR_EOC | ADC_SR_JEOC | ADC_SR_OVR);
    ADC3->SR = ~(ADC_SR_EOC | ADC_SR_JEOC | ADC_SR_OVR);

    return true;
}

extern "C" {

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) {
    HAL_SPI_TxRxCpltCallback(hspi);
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi) {
    HAL_SPI_TxRxCpltCallback(hspi);
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi) {
    if (hspi == &spi.hspi_) {
        spi_arbiter.on_complete();
    }
}

#if HW_VERSION_MINOR >= 3
void TIM5_IRQHandler(void) {
    COUNT_IRQ(TIM5_IRQn);
    pwm_inputs[0].on_capture();
}
#endif

volatile uint32_t timestamp_ = 0;
volatile bool counting_down_ = false;

void TIM8_UP_TIM13_IRQHandler(void) {
    COUNT_IRQ(TIM8_UP_TIM13_IRQn);
    
    // Entry into this function happens at 21-23 clock cycles after the timer
    // update event.
    __HAL_TIM_CLEAR_IT(&htim8, TIM_IT_UPDATE);

    // If the corresponding timer is counting up, we just sampled in SVM vector 0, i.e. real current
    // If we are counting down, we just sampled in SVM vector 7, with zero current
    bool counting_down = TIM8->CR1 & TIM_CR1_DIR;

    bool timer_update_missed = (counting_down_ == counting_down);
    if (timer_update_missed) {
        motors[0].disarm_with_error(Motor::ERROR_TIMER_UPDATE_MISSED);
        motors[1].disarm_with_error(Motor::ERROR_TIMER_UPDATE_MISSED);
        return;
    }
    counting_down_ = counting_down;

    timestamp_ += TIM_1_8_PERIOD_CLOCKS * (TIM_1_8_RCR + 1);

    if (!counting_down) {
        // If TIM8 is counting up run sampling handlers and kick off control
        // tasks.
        TaskTimer::enabled = odrv.task_timers_armed_;
        for (size_t i = 0; i < sizeof(pwm_gpios) / sizeof(pwm_gpios[0]); ++i) {
            board.gpio_pwm_values[pwm_gpios[i]] = pwm_inputs[i / 4].pwm_values_[i % 4];
        }
        odrv.sampling_cb();
        NVIC->STIR = ControlLoop_IRQn;
    } else {
        // Tentatively reset all PWM outputs to 50% duty cycles. If the control
        // loop handler finishes in time then these values will be overridden
        // before they go into effect.
        // TODO: reset bits in CCER instead
        TIM1->CCR1 =
        TIM1->CCR2 =
        TIM1->CCR3 =
        TIM8->CCR1 =
        TIM8->CCR2 =
        TIM8->CCR3 =
            TIM_1_8_PERIOD_CLOCKS / 2;
    }
}

void ControlLoop_IRQHandler(void) {
    COUNT_IRQ(ControlLoop_IRQn);
    uint32_t timestamp = timestamp_;

    // Ensure that all the ADCs are done
    std::optional<Iph_ABC_t> current0;
    std::optional<Iph_ABC_t> current1;

    if (!fetch_and_reset_adcs(&current0, &current1)) {
        motors[0].disarm_with_error(Motor::ERROR_BAD_TIMING);
        motors[1].disarm_with_error(Motor::ERROR_BAD_TIMING);
    }

    // Check if ADC1 (DMA) is done
    uint32_t tcif = __HAL_DMA_GET_TC_FLAG_INDEX(hadc1.DMA_Handle);
    if (!__HAL_DMA_GET_FLAG(hadc1.DMA_Handle, tcif)) {
        motors[0].disarm_with_error(Motor::ERROR_BAD_TIMING);
        motors[1].disarm_with_error(Motor::ERROR_BAD_TIMING);
    }
    __HAL_DMA_CLEAR_FLAG(hadc1.DMA_Handle, tcif);

    // Convert GPIO ADCs samples to voltages (actual sampling is triggered by
    // TIM8 and done using DMA).
    for (size_t i = 0; i < adc_gpios.size(); ++i) {
        board.gpio_adc_values[adc_gpios[i]] = adc_measurements_[i] /
            kAdcFullScale;
    }

    // Convert onboard FET thermistor ADC samples to temperatures
    for (size_t i = 0; i < AXIS_COUNT; ++i) {
        board.motor_fet_temperatures[i] = horner_poly_eval(
            adc_measurements_[adc_gpios.size() + i] / kAdcFullScale,
            fet_thermistor_poly_coeffs.data(), fet_thermistor_poly_coeffs.size());
    }

    // If the motor FETs are not switching then we can't measure the current
    // because for this we need the low side FET to conduct.
    // So for now we guess the current to be 0 (this is not correct shortly after
    // disarming and when the motor spins fast in idle). Passing an invalid
    // current reading would create problems with starting FOC.
    if (!(TIM1->BDTR & TIM_BDTR_MOE_Msk)) {
        current0 = {0.0f, 0.0f};
    }
    if (!(TIM8->BDTR & TIM_BDTR_MOE_Msk)) {
        current1 = {0.0f, 0.0f};
    }

    motors[0].current_meas_cb(timestamp - TIM1_INIT_COUNT, current0);
    motors[1].current_meas_cb(timestamp, current1);

    odrv.control_loop_cb(timestamp);

    // By this time the ADCs for both M0 and M1 should have fired again. But
    // let's wait for them just to be sure.
    MEASURE_TIME(odrv.task_times_.dc_calib_wait) {
        while (!(ADC2->SR & ADC_SR_EOC));
    }

    if (!fetch_and_reset_adcs(&current0, &current1)) {
        motors[0].disarm_with_error(Motor::ERROR_BAD_TIMING);
        motors[1].disarm_with_error(Motor::ERROR_BAD_TIMING);
    }

    motors[0].dc_calib_cb(timestamp + TIM_1_8_PERIOD_CLOCKS * (TIM_1_8_RCR + 1) - TIM1_INIT_COUNT, current0);
    motors[1].dc_calib_cb(timestamp + TIM_1_8_PERIOD_CLOCKS * (TIM_1_8_RCR + 1), current1);

    motors[0].pwm_update_cb(timestamp + 3 * TIM_1_8_PERIOD_CLOCKS * (TIM_1_8_RCR + 1) - TIM1_INIT_COUNT);
    motors[1].pwm_update_cb(timestamp + 3 * TIM_1_8_PERIOD_CLOCKS * (TIM_1_8_RCR + 1));

    // TODO: move to main control loop. For now we put it here because the motor
    // PWM update refreshes the power estimate and the brake res update must
    // happen after that.
    odrv.brake_resistor_.update();

    // The brake resistor PWM is not latched on TIM1 update events but will take
    // effect immediately. This means that the timestamp given here is a bit too
    // late.
    brake_resistor_output_impl.update(timestamp + 3 * TIM_1_8_PERIOD_CLOCKS * (TIM_1_8_RCR + 1) - TIM1_INIT_COUNT);

    // If we did everything right, the TIM8 update handler should have been
    // called exactly once between the start of this function and now.

    if (timestamp_ != timestamp + TIM_1_8_PERIOD_CLOCKS * (TIM_1_8_RCR + 1)) {
        motors[0].disarm_with_error(Motor::ERROR_CONTROL_DEADLINE_MISSED);
        motors[1].disarm_with_error(Motor::ERROR_CONTROL_DEADLINE_MISSED);
    }

    odrv.task_timers_armed_ = odrv.task_timers_armed_ && !TaskTimer::enabled;
    TaskTimer::enabled = false;
}

/* I2C support currently not maintained
void I2C1_EV_IRQHandler(void) {
    COUNT_IRQ(I2C1_EV_IRQn);
    HAL_I2C_EV_IRQHandler(&hi2c1);
}

void I2C1_ER_IRQHandler(void) {
    COUNT_IRQ(I2C1_ER_IRQn);
    HAL_I2C_ER_IRQHandler(&hi2c1);
}*/

#if HW_VERSION_MINOR >= 3

void DMA1_Stream2_IRQHandler(void) {
    COUNT_IRQ(DMA1_Stream2_IRQn);
    HAL_DMA_IRQHandler(&uart_impl[0].hdma_rx_);
}

void DMA1_Stream4_IRQHandler(void) {
    COUNT_IRQ(DMA1_Stream4_IRQn);
    HAL_DMA_IRQHandler(&uart_impl[0].hdma_tx_);
}

void DMA1_Stream5_IRQHandler(void) {
    COUNT_IRQ(DMA1_Stream5_IRQn);
    HAL_DMA_IRQHandler(&uart_impl[1].hdma_rx_);
}

void DMA1_Stream6_IRQHandler(void) {
    COUNT_IRQ(DMA1_Stream6_IRQn);
    HAL_DMA_IRQHandler(&uart_impl[1].hdma_tx_);
}

void UART4_IRQHandler(void) {
    COUNT_IRQ(UART4_IRQn);
    HAL_UART_IRQHandler(uart_impl[0]);
}

void USART2_IRQHandler(void) {
    COUNT_IRQ(USART2_IRQn);
    HAL_UART_IRQHandler(uart_impl[1]);
}

#endif

void DMA1_Stream0_IRQHandler(void) {
    COUNT_IRQ(DMA1_Stream0_IRQn);
    HAL_DMA_IRQHandler(&spi.hdma_rx_);
}

void DMA1_Stream7_IRQHandler(void) {
    COUNT_IRQ(DMA1_Stream7_IRQn);
    HAL_DMA_IRQHandler(&spi.hdma_tx_);
}

void SPI3_IRQHandler(void) {
    COUNT_IRQ(SPI3_IRQn);
    HAL_SPI_IRQHandler(spi);
}

void OTG_FS_IRQHandler(void) {
    COUNT_IRQ(OTG_FS_IRQn);
    HAL_PCD_IRQHandler(&hpcd_USB_OTG_FS);
}

}
