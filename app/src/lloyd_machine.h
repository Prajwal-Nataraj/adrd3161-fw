/*
 * Lloyd Machine Header
 * Author: Prajwal Nataraj
 */

#ifndef LLOYD_MACHINE_H
#define LLOYD_MACHINE_H

/* Safety Doors, Limit Switches */
#define FRONT_DOOR_PIN  7
#define REAR_DOOR_PIN   8
#define FRONT_DOOR_BIT  2
#define REAR_DOOR_BIT   1

#define LIM_SW_UP_PIN   18      /* Swapped the pin numbers of LIM_SW_UP and LIM_SW_LO compared to hardware connection */
#define LIM_SW_LO_PIN   19      /* because LIM_SW_UP and LIM_SW_LO are swapped in ADI compared to AMC */
#define LIM_SW_UP_BIT   3       
#define LIM_SW_LO_BIT   4       

/* Fault bits */
#define DRIVE_RESET            BIT(0)
#define SHORT_CIRCUIT          BIT(2)
#define DRIVE_INTERNAL_ERROR   BIT(1)
#define OVER_CURRENT           BIT(3)
#define UNDER_VOLTAGE          BIT(4)
#define OVER_VOLTAGE           BIT(5)
#define OVER_TEMPERATURE       BIT(6)

/* Machine type */
#define LS1     (0x01)
#define LS5     (0x02)

/* typedef */
typedef struct {
    struct gpio_callback front_door_gpio_cb;
    struct gpio_callback rear_door_gpio_cb;
    struct gpio_callback ls_up_gpio_cb;
    struct gpio_callback ls_lo_gpio_cb;
}Lloyd_Int_CB_Struct;

/* Function prototypes */
void conf_LloydMachine(uint8_t);
int conf_LloydGPIOInputs(const struct device *);
int conf_LloydIntCallbacks(const struct device *, Lloyd_Int_CB_Struct *);
int conf_LloydInterrupts(const struct device *, Lloyd_Int_CB_Struct *);
void Set_LloydMD_Fault(uint32_t, uint32_t);

#endif /* LLOYD_MACHINE_H */