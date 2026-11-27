/*
 * Lloyd Machine Source
 * Author: Prajwal Nataraj
 */

/*---------------------- Includes ----------------------*/
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <canopennode.h>
#include "OD.h"
#include "tmc9660_params.h"
#include "lloyd_machine.h"

/*---------------------- Logging Module ----------------------*/
LOG_MODULE_REGISTER(LOG_LEVEL_INF);

/*---------------------- Static Variables ----------------------*/
static struct k_spinlock drv_dig_inputs_lock;

/*---------------------- Private Functions ----------------------*/
/**
 * @brief Set or clear bits in the driver digital inputs register
 * @param mask Bit mask indicating which bits to modify
 * @param set If true, set the bits; if false, clear the bits
 */
static void drv_dig_inputs_set_bit(uint32_t mask, bool set)
{
    k_spinlock_key_t key = k_spin_lock(&drv_dig_inputs_lock);
    uint32_t inputs = OD_RAM.x2200_drv_DigInputs;

    if (set) {
        inputs |= mask;
    } else {
        inputs &= ~mask;
    }

    OD_RAM.x2200_drv_DigInputs = inputs;
    k_spin_unlock(&drv_dig_inputs_lock, key);
}

/**
 * @brief Convert Analog Devices error flags to AMC fault flags
 *
 * @param gen_err_flags General error flags from the drive
 * @param gdrv_err_flags Gate driver error flags from the drive
 * @note The function updates the OD_RAM.x2200_drv_DigInputs register with the corresponding AMC fault flags.
 */
static void adi_to_amc_faults(uint32_t gen_err_flags,
			   uint32_t gdrv_err_flags)
{
	uint16_t amc_faults = 0;

	/* Drive Reset */
	if (gen_err_flags & (BIT(9)  |  /* Watchdog event */
			     BIT(22))) /* Fault retry happened */
	{
		amc_faults |= DRIVE_RESET;
	}

	/* Drive Internal Error */
	if (gen_err_flags & (BIT(1)  |  /* TMCL script error */
			     BIT(23))) /* Fault retries failed */
	{
		amc_faults |= DRIVE_INTERNAL_ERROR;
	}

	/* Short Circuit
	 * U/V only, W/Y2 excluded
	 */
	if (gdrv_err_flags & (
		BIT(4)  | BIT(5)  |  /* low-side discharge short */
		BIT(8)  | BIT(9)  |  /* low-side charge short */
		BIT(20) | BIT(21) |  /* high-side discharge short */
		BIT(24) | BIT(25)))  /* high-side charge short */
	{
		amc_faults |= SHORT_CIRCUIT;
	}

	/* Over Current
	 * U/V only, W/Y2 excluded
	 */
	if ((gdrv_err_flags & (
			BIT(0)  | BIT(1)  |   /* low-side overcurrent */
			BIT(16) | BIT(17))) || /* high-side overcurrent */
	    (gen_err_flags & (
			BIT(16) |           /* I²t limit 1 exceeded */
			BIT(17))))          /* I²t limit 2 exceeded */
	{
		amc_faults |= OVER_CURRENT;
	}

	/* Drive Under Voltage
	 * U/V only, W/Y2 excluded
	 */
	if ((gen_err_flags & BIT(20)) || /* Supply undervoltage warning */
	    (gdrv_err_flags & (
			BIT(12) | BIT(13) | /* U/V bootstrap undervoltage */
			BIT(29) |           /* Gate-driver undervoltage */
			BIT(30) |           /* Gate-driver low voltage */
			BIT(31))))          /* Gate-driver supply undervoltage */
	{
		amc_faults |= UNDER_VOLTAGE;
	}

	/* Drive Over Voltage */
    if (gen_err_flags & (
		BIT(19) | /* Supply overvoltage warning */
		BIT(21))) /* ADC input overvoltage */
	{
		amc_faults |= OVER_VOLTAGE;
	}

	/* Drive Over Temperature */
	if (gen_err_flags & (
		BIT(13) | /* External temperature exceeded */
		BIT(14))) /* Chip temperature exceeded */
	{
		amc_faults |= OVER_TEMPERATURE;
	}

	OD_RAM.x2200_drv_DigInputs |= (amc_faults << 16);
}

/**
 * @brief Callback for front door GPIO interrupt
 */
static void front_door_gpio_callback(const struct device *port,
                                     struct gpio_callback *cb,
                                     gpio_port_pins_t pins)
{
    if ((pins & BIT(FRONT_DOOR_PIN)) == 0U) {
        return;
    }

    int value = gpio_pin_get(port, FRONT_DOOR_PIN);
    if (value >= 0) {
        drv_dig_inputs_set_bit(BIT(FRONT_DOOR_BIT), value != 0);
    }
}

/**
 * @brief Callback for rear door GPIO interrupt
 */
static void rear_door_gpio_callback(const struct device *port,
                                    struct gpio_callback *cb,
                                    gpio_port_pins_t pins)
{
    if ((pins & BIT(REAR_DOOR_PIN)) == 0U) {
        return;
    }

    int value = gpio_pin_get(port, REAR_DOOR_PIN);
    if (value >= 0) {
        drv_dig_inputs_set_bit(BIT(REAR_DOOR_BIT), value != 0);
    }
}

/**
 * @brief Callback for limit switch up GPIO interrupt
 */
static void ls_up_gpio_callback(const struct device *port,
                                    struct gpio_callback *cb,
                                    gpio_port_pins_t pins)
{
    if ((pins & BIT(LIM_SW_UP_PIN)) == 0U) {
        return;
    }

    int value = gpio_pin_get(port, LIM_SW_UP_PIN);
    if (value >= 0) {
        drv_dig_inputs_set_bit(BIT(LIM_SW_UP_BIT), value != 0);
    }
}

/**
 * @brief Callback for limit switch low GPIO interrupt
 */
static void ls_lo_gpio_callback(const struct device *port,
                                     struct gpio_callback *cb,
                                     gpio_port_pins_t pins)
{
    if ((pins & BIT(LIM_SW_LO_PIN)) == 0U) {
        return;
    }

    int value = gpio_pin_get(port, LIM_SW_LO_PIN);
    if (value >= 0) {
        drv_dig_inputs_set_bit(BIT(LIM_SW_LO_BIT), value != 0);
    }
}

/*---------------------- Public Functions ----------------------*/
/**
 * @brief Apply INI parameters based on machine type
 * @param mType Machine type identifier
 */
void conf_LloydMachine(uint8_t mType)
{
    // Common INI parameters for all machine types
    OD_PERSIST_TMC.x2000_MOTOR_TYPE = 1;
    OD_PERSIST_TMC.x2002_MOTOR_DIRECTION = 2;
    OD_PERSIST_TMC.x2005_OUTPUT_VOLTAGE_LIMIT = 32767;
    OD_PERSIST_TMC.x2015_CURRENT_SCALING_FACTOR = 390;
    OD_PERSIST_TMC.x201F_ADC_I1_INVERTED = 0;
    OD_PERSIST_TMC.x2020_ADC_I2_INVERTED = 0;
    OD_PERSIST_TMC.x2034_RAMP_ENABLE = 1;
    OD_PERSIST_TMC.x2036_RAMP_AMAX = 4000;
    OD_PERSIST_TMC.x2037_RAMP_A1 = 2000;
    OD_PERSIST_TMC.x2039_RAMP_DMAX = 6000;
    OD_PERSIST_TMC.x203B_RAMP_D2 = 4000;
    OD_PERSIST_TMC.x203D_RAMP_V1 = 32000;
    OD_PERSIST_TMC.x203E_RAMP_V2 = 64000;
    OD_PERSIST_TMC.x205A_ABN_1_STEPS = 20000;
    OD_PERSIST_TMC.x207B_VELOCITY_SENSOR_SELECTION = 2;
    OD_PERSIST_TMC.x207F_VELOCITY_P = 50;
    OD_PERSIST_TMC.x2081_VELOCITY_NORM_P = 0;
    OD_PERSIST_TMC.x2082_VELOCITY_NORM_I = 0;
    OD_PERSIST_TMC.x2087_VELOCITY_LOOP_DOWNSAMPLING = 0;
    OD_PERSIST_TMC.x2089_VELOCITY_METER_SWITCH_THRESHOLD = 0;
    OD_PERSIST_TMC.x208E_POSITION_SENSOR_SELECTION = 2;
    OD_PERSIST_TMC.x20A1_REFERENCE_SWITCH_ENABLE = 3;
    OD_PERSIST_TMC.x20A4_EVENT_STOP_SETTINGS = 1;
    OD_PERSIST_TMC.x20AE_ABN_2_STEPS = 0;
    OD_PERSIST_TMC.x20B0_ABN_2_GEAR_RATIO = 0;
    OD_PERSIST_TMC.x20B7_SPI_ENCODER_MAIN_TRANSFER_CMD_SIZE = 0;
    OD_PERSIST_TMC.x20B9_SPI_ENCODER_TRANSFER_DATA_3_0 = 0;
    OD_PERSIST_TMC.x20BA_SPI_ENCODER_TRANSFER_DATA_7_4 = 0;
    OD_PERSIST_TMC.x20BB_SPI_ENCODER_TRANSFER_DATA_11_8 = 0;
    OD_PERSIST_TMC.x20BC_SPI_ENCODER_TRANSFER_DATA_15_12 = 0;
    OD_PERSIST_TMC.x20BE_SPI_ENCODER_POSITION_COUNTER_MASK = 0;
    OD_PERSIST_TMC.x20C4_SPI_ENCODER_OFFSET = 0;
    OD_PERSIST_TMC.x20D4_BRAKE_CHOPPER_ENABLE = 1;
    OD_PERSIST_TMC.x20D5_BRAKE_CHOPPER_VOLTAGE_LIMIT = 495;
    OD_PERSIST_TMC.x20DD_INVERT_BRAKE_OUTPUT = 1;
    OD_PERSIST_TMC.x20F1_DRIVE_TIME_SINK_UVW = 40;
    OD_PERSIST_TMC.x20F2_DRIVE_TIME_SOURCE_UVW = 40;
    OD_PERSIST_TMC.x20F3_DRIVE_TIME_SINK_Y2 = 40;
    OD_PERSIST_TMC.x20F4_DRIVE_TIME_SOURCE_Y2 = 40;
    OD_PERSIST_TMC.x20F5_UVW_SINK_CURRENT = 10;
    OD_PERSIST_TMC.x20F6_UVW_SOURCE_CURRENT = 5;
    OD_PERSIST_TMC.x20F7_Y2_SINK_CURRENT = 10;
    OD_PERSIST_TMC.x20F8_Y2_SOURCE_CURRENT = 5;
    OD_PERSIST_TMC.x2102_OVERCURRENT_PROTECTION_UVW_LOW_SIDE_THRESHOLD = 1;
    OD_PERSIST_TMC.x2103_OVERCURRENT_PROTECTION_UVW_HIGH_SIDE_THRESHOLD = 2;
    OD_PERSIST_TMC.x2104_OVERCURRENT_PROTECTION_Y2_LOW_SIDE_THRESHOLD = 1;
    OD_PERSIST_TMC.x2105_OVERCURRENT_PROTECTION_Y2_HIGH_SIDE_THRESHOLD = 4;
    OD_PERSIST_TMC.x210E_OVERCURRENT_PROTECTION_UVW_LOW_SIDE_USE_VDS = 0;
    OD_PERSIST_TMC.x210F_OVERCURRENT_PROTECTION_Y2_LOW_SIDE_USE_VDS = 0;
    OD_PERSIST_TMC.x2123_SUPPLY_OVERVOLTAGE_WARNING_THRESHOLD = 710;
    OD_PERSIST_TMC.x2124_SUPPLY_UNDERVOLTAGE_WARNING_THRESHOLD = 78;
    OD_PERSIST_TMC.x213E_TARGET_TORQUE_BIQUAD_FILTER_ENABLE = 1;

    if(mType == LS1)
    {    
        OD_PERSIST_TMC.x2006_MAX_TORQUE = 10000;
        OD_PERSIST_TMC.x2007_MAX_FLUX = 1690;
        OD_PERSIST_TMC.x2022_ADC_I0_OFFSET = -14;
        OD_PERSIST_TMC.x2023_ADC_I1_OFFSET = 8;
        OD_PERSIST_TMC.x2025_ADC_I3_OFFSET = 7;
        OD_PERSIST_TMC.x202E_OPENLOOP_CURRENT = 3000;
        OD_PERSIST_TMC.x206D_TORQUE_P = 500;
        OD_PERSIST_TMC.x206E_TORQUE_I = 300;
        OD_PERSIST_TMC.x206F_FLUX_P = 500;
        OD_PERSIST_TMC.x2070_FLUX_I = 300;
        OD_PERSIST_TMC.x2080_VELOCITY_I = 120;
        OD_PERSIST_TMC.x20E1_IIT_LIMIT_1 = 75000;
        OD_PERSIST_TMC.x20E4_IIT_LIMIT_2 = 150000;

        // Write persisted params to TMC and set up extensions for intercepting reads and writes
        tmc_params_init();
        LOG_INF("Initialization OK!");
    }
    else if(mType == LS5)
    {
        OD_PERSIST_TMC.x2006_MAX_TORQUE = 12000;
        OD_PERSIST_TMC.x2007_MAX_FLUX = 947;
        OD_PERSIST_TMC.x2022_ADC_I0_OFFSET = -3;
        OD_PERSIST_TMC.x2023_ADC_I1_OFFSET = 9;
        OD_PERSIST_TMC.x2024_ADC_I2_OFFSET = -4;
        OD_PERSIST_TMC.x2025_ADC_I3_OFFSET = 21;
        OD_PERSIST_TMC.x202E_OPENLOOP_CURRENT = 5000;
        OD_PERSIST_TMC.x206D_TORQUE_P = 1000;
        OD_PERSIST_TMC.x206E_TORQUE_I = 4000;
        OD_PERSIST_TMC.x206F_FLUX_P = 1000;
        OD_PERSIST_TMC.x2070_FLUX_I = 4000;
        OD_PERSIST_TMC.x2080_VELOCITY_I = 200;
        OD_PERSIST_TMC.x20E1_IIT_LIMIT_1 = -1;
        OD_PERSIST_TMC.x20E4_IIT_LIMIT_2 = -1;
        
        // Write persisted params to TMC and set up extensions for intercepting reads and writes
        tmc_params_init();
        LOG_INF("Initialization OK!");
    }

    else {
        LOG_ERR("Machine configuration failed");
    }
}

/**
 * @brief Configure Lloyd GPIO inputs
 * @param gpio Pointer to the GPIO device
 * @return 0 on success, negative error code on failure
 */
int conf_LloydGPIOInputs(const struct device *gpio)
{
    int err = -1;

    err = gpio_pin_configure(gpio, FRONT_DOOR_PIN, GPIO_INPUT);
    if(err < 0) {
        LOG_ERR("Could not initialize GPIO %d: %d", FRONT_DOOR_PIN, err);
        return err;
    }
    err = gpio_pin_configure(gpio, REAR_DOOR_PIN, GPIO_INPUT);
    if(err < 0) {
        LOG_ERR("Could not initialize GPIO %d: %d", REAR_DOOR_PIN, err);
        return err;
    }
    err = gpio_pin_configure(gpio, LIM_SW_UP_PIN, GPIO_INPUT);
    if(err < 0) {
        LOG_ERR("Could not initialize GPIO %d: %d", LIM_SW_UP_PIN, err);
        return err;
    }
    err = gpio_pin_configure(gpio, LIM_SW_LO_PIN, GPIO_INPUT);
    if(err < 0) {
        LOG_ERR("Could not initialize GPIO %d: %d", LIM_SW_LO_PIN, err);
        return err;
    }

    return 0;
}

int conf_LloydIntCallbacks(const struct device *gpio, Lloyd_Int_CB_Struct *lloyd_int_cb)
{
    int err = -1;

    gpio_init_callback(&lloyd_int_cb->front_door_gpio_cb, front_door_gpio_callback,
                       BIT(FRONT_DOOR_PIN));
    err = gpio_add_callback(gpio, &lloyd_int_cb->front_door_gpio_cb);
    if (err < 0) {
        LOG_ERR("Could not add front door GPIO callback: %d", err);
        return err;
    }

    gpio_init_callback(&lloyd_int_cb->rear_door_gpio_cb, rear_door_gpio_callback,
                       BIT(REAR_DOOR_PIN));
    err = gpio_add_callback(gpio, &lloyd_int_cb->rear_door_gpio_cb);
    if (err < 0) {
        LOG_ERR("Could not add rear door GPIO callback: %d", err);
        return err;
    }

    gpio_init_callback(&lloyd_int_cb->ls_up_gpio_cb, ls_up_gpio_callback,
                       BIT(LIM_SW_UP_PIN));
    err = gpio_add_callback(gpio, &lloyd_int_cb->ls_up_gpio_cb);
    if (err < 0) {
        LOG_ERR("Could not add LS UP GPIO callback: %d", err);
        return err;
    }

    gpio_init_callback(&lloyd_int_cb->ls_lo_gpio_cb, ls_lo_gpio_callback,
                       BIT(LIM_SW_LO_PIN));
    err = gpio_add_callback(gpio, &lloyd_int_cb->ls_lo_gpio_cb);
    if (err < 0) {
        LOG_ERR("Could not add LS LO GPIO callback: %d", err);
        return err;
    }

    return 0;
}

int conf_LloydInterrupts(const struct device *gpio, Lloyd_Int_CB_Struct *lloyd_int_cb)
{
    int err = -1;

    // Configure GPIO interrupts
    err = gpio_pin_interrupt_configure(gpio, FRONT_DOOR_PIN,
                                       GPIO_INT_EDGE_BOTH);
    if (err < 0) {
        LOG_ERR("Could not configure front door GPIO interrupt: %d", err);
        gpio_remove_callback(gpio, &lloyd_int_cb->front_door_gpio_cb);
        return err;
    }

    err = gpio_pin_interrupt_configure(gpio, REAR_DOOR_PIN,
                                       GPIO_INT_EDGE_BOTH);
    if (err < 0) {
        LOG_ERR("Could not configure rear door GPIO interrupt: %d", err);
        gpio_remove_callback(gpio, &lloyd_int_cb->rear_door_gpio_cb);
        return err;
    }
    
    err = gpio_pin_interrupt_configure(gpio, LIM_SW_UP_PIN,
                                       GPIO_INT_EDGE_FALLING);
    if (err < 0) {
        LOG_ERR("Could not configure LS UP GPIO interrupt: %d", err);
        gpio_remove_callback(gpio, &lloyd_int_cb->ls_up_gpio_cb);
        return err;
    }

    err = gpio_pin_interrupt_configure(gpio, LIM_SW_LO_PIN,
                                       GPIO_INT_EDGE_FALLING);
    if (err < 0) {
        LOG_ERR("Could not configure LS LO GPIO interrupt: %d", err);
        gpio_remove_callback(gpio, &lloyd_int_cb->ls_lo_gpio_cb);
        return err;
    }

    int FDvalue = gpio_pin_get(gpio, FRONT_DOOR_PIN);
    if (FDvalue < 0) {
        LOG_ERR("Could not read front door GPIO: %d", FDvalue);
        gpio_pin_interrupt_configure(gpio, FRONT_DOOR_PIN,
                                     GPIO_INT_DISABLE);
        gpio_remove_callback(gpio, &lloyd_int_cb->front_door_gpio_cb);
        return FDvalue;
    }
    drv_dig_inputs_set_bit(BIT(FRONT_DOOR_BIT), FDvalue != 0);

    // Read initial state of input pins
    int RDvalue = gpio_pin_get(gpio, REAR_DOOR_PIN);
    if (RDvalue < 0) {
        LOG_ERR("Could not read rear door GPIO: %d", RDvalue);
        gpio_pin_interrupt_configure(gpio, REAR_DOOR_PIN,
                                     GPIO_INT_DISABLE);
        gpio_remove_callback(gpio, &lloyd_int_cb->rear_door_gpio_cb);
        return RDvalue;
    }
    drv_dig_inputs_set_bit(BIT(REAR_DOOR_BIT), RDvalue != 0);

    int LSUPvalue = gpio_pin_get(gpio, LIM_SW_UP_PIN);
    if (LSUPvalue < 0) {
        LOG_ERR("Could not read LS UP GPIO: %d", LSUPvalue);
        gpio_pin_interrupt_configure(gpio, LIM_SW_UP_PIN,
                                     GPIO_INT_DISABLE);
        gpio_remove_callback(gpio, &lloyd_int_cb->ls_up_gpio_cb);
        return LSUPvalue;
    }
    drv_dig_inputs_set_bit(BIT(LIM_SW_UP_BIT), LSUPvalue != 0);

    int LSLOvalue = gpio_pin_get(gpio, LIM_SW_LO_PIN);
    if (LSLOvalue < 0) {
        LOG_ERR("Could not read LS LO GPIO: %d", LSLOvalue);
        gpio_pin_interrupt_configure(gpio, LIM_SW_LO_PIN,
                                     GPIO_INT_DISABLE);
        gpio_remove_callback(gpio, &lloyd_int_cb->ls_lo_gpio_cb);
        return LSLOvalue;
    }
    drv_dig_inputs_set_bit(BIT(LIM_SW_LO_BIT), LSLOvalue != 0);

    return 0;
}

/**
 * Set Lloyd Machine Fault
 * @param gen_err_flags General error flags
 * @param gdrv_err_flags GDRV error flags
 */
void Set_LloydMD_Fault(uint32_t gen_err_flags, uint32_t gdrv_err_flags)
{
    adi_to_amc_faults(gen_err_flags, gdrv_err_flags);
}

/************************** END OF FILE *****************************/