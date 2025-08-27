"""
Read only param:

<q1:parameter uniqueID="UID_OBJ_2123">
<description lang="en">TMC9660 REGISTER_READ_ONLY</description>
<DINT />
<q1:defaultValue value="0" />
<q1:property name="CO_storageGroup" value="APP_SHADOW" />
</q1:parameter>

Read write but not persist param:

<q1:parameter uniqueID="UID_OBJ_2124" access="readWriteOutput">
<description lang="en">TMC9660 REGISTER_READ_ONLY</description>
<DINT />
<q1:defaultValue value="0" />
<q1:property name="CO_storageGroup" value="APP_SHADOW" />
</q1:parameter>

Read write persist param:

<q1:parameter uniqueID="UID_OBJ_2125" access="readWriteOutput">
<description lang="en">TMC9660 REGISTER_RWE</description>
<DINT />
<q1:defaultValue value="0" />
<q1:property name="CO_storageGroup" value="PERSIST_TMC" />
</q1:parameter>
"""
part1 = ""


"""
Read only param:

<CANopenObject index="2123" name="REGISTER_READ_ONLY" objectType="7" PDOmapping="no" uniqueIDRef="UID_OBJ_2123" />

Read write but not persist param:

<CANopenObject index="2124" name="REGISTER_READ_WRITE" objectType="7" PDOmapping="RPDO" uniqueIDRef="UID_OBJ_2124" />

Read write persist param:

<CANopenObject index="2122" name="TMC9660 SUPPLY_VOLTAGE" objectType="7" PDOmapping="TPDO" uniqueIDRef="UID_OBJ_2122" />
"""
part2 = ""


def Parameter(id, name, rw, range, default, desc, enum=set(), unit=None):
	global part1, part2
	assert rw in ["R", "RW", "RWE"], f"{rw=} not valid"

	storageGroup = {
		"R": "APP_SHADOW",
		"RW": "APP_SHADOW",
		"RWE": "PERSIST_TMC"
	}[rw]

	part1 += f"""
<q1:parameter uniqueID="UID_OBJ_{0x2000 + id:04X}" """
	if rw == "RW" or rw == "RWE":
		part1 += 'access="readWriteOutput"'
	part1 += f""">
	<description lang="en">TMC9660 register {name}
{desc}
Min: {range[0]}; Max: {range[1]}; Default: {default}"""
	if unit:
		part1 += f"; Unit: {unit}"
	if enum:
		part1 += "\nValues:\n"
		part1 += "\n".join(f"  {name} = {val}" for val, name in enum.items())
	part1 += f"""
	</description>
	<DINT />
	<q1:defaultValue value="{default}" />
	<q1:property name="CO_storageGroup" value="{storageGroup}" />
</q1:parameter>"""

	pdo = {
		"R": "TPDO",
		"RW": "optional",
		"RWE": "optional"
	}[rw]

	part2 += f"""
	<CANopenObject index="{0x2000 + id:04X}" name="{name}" objectType="7" PDOmapping="{pdo}" uniqueIDRef="UID_OBJ_{0x2000 + id:04X}" />"""

Parameter(id=0, name='MOTOR_TYPE', rw='RWE', range=(0, 3), default=0, desc='Selected motor type. PWM must be turned off to change this.', enum={0: 'NO_MOTOR', 1: 'DC_MOTOR', 2: 'STEPPER_MOTOR', 3: 'BLDC_MOTOR'})
Parameter(id=1, name='MOTOR_POLE_PAIRS', rw='RWE', range=(0, 127), default=1, desc='Pole pair count of the motor.')
Parameter(id=2, name='MOTOR_DIRECTION', rw='RWE', range=(0, 1), default=0, desc='Motor direction bit. Inverts the motor direction.', enum={0: 'NOT_INVERTED', 1: 'INVERTED'})
Parameter(id=3, name='MOTOR_PWM_FREQUENCY', rw='RWE', range=(10000, 100000), default=25000, desc='Set the frequency of the motor PWM.', unit='Hz')
Parameter(id=4, name='COMMUTATION_MODE', rw='RW', range=(0, 8), default=0, desc='Selected FOC operation mode depending on feedback used for commutation.', enum={0: 'SYSTEM_OFF', 1: 'SYSTEM_OFF_LOW_SIDE_FETS_ON', 2: 'SYSTEM_OFF_HIGH_SIDE_FETS_ON', 3: 'FOC_OPENLOOP_VOLTAGE_MODE', 4: 'FOC_OPENLOOP_CURRENT_MODE', 5: 'FOC_ABN', 6: 'FOC_HALL_SENSOR', 7: 'RESERVED', 8: 'FOC_SPI_ENC'})
Parameter(id=5, name='OUTPUT_VOLTAGE_LIMIT', rw='RWE', range=(0, 32767), default=8000, desc='PID UQ/UD output limit for circular limiter.')
Parameter(id=6, name='MAX_TORQUE', rw='RWE', range=(0, 65535), default=2000, desc='Maximum motor torque. *This value can be temporarily exceeded marginally due to the operation of the current regulator.', unit='mA')
Parameter(id=7, name='MAX_FLUX', rw='RWE', range=(0, 65535), default=2000, desc='Max. motor flux. *This value can be temporarily exceeded marginally due to the operation of the current regulator.', unit='mA')
Parameter(id=8, name='PWM_SWITCHING_SCHEME', rw='RWE', range=(0, 2), default=1, desc='PWM switching scheme.', enum={0: 'STANDARD', 1: 'SVPWM', 2: 'FLAT_BOTTOM'})
Parameter(id=9, name='IDLE_MOTOR_PWM_BEHAVIOR', rw='RWE', range=(0, 1), default=1, desc='Configure if the PWM should be off (high-z) or on (all motor phases same voltage) in commutation mode "System Off".', enum={0: 'PWM_ON_WHEN_MOTOR_IDLE', 1: 'PWM_OFF_WHEN_MOTOR_IDLE'})
Parameter(id=12, name='ADC_SHUNT_TYPE', rw='RWE', range=(0, 4), default=4, desc='Shunt type used for ADC measurements.', enum={0: 'INLINE_UVW', 1: 'INLINE_VW', 2: 'INLINE_UW', 3: 'INLINE_UV', 4: 'BOTTOM_SHUNTS'})
Parameter(id=13, name='ADC_I0_RAW', rw='R', range=(-32768, 32767), default=0, desc='Raw ADC measurement of the ADC I0 shunt.')
Parameter(id=14, name='ADC_I1_RAW', rw='R', range=(-32768, 32767), default=0, desc='Raw ADC measurement of the ADC I1 shunt.')
Parameter(id=15, name='ADC_I2_RAW', rw='R', range=(-32768, 32767), default=0, desc='Raw ADC measurement of the ADC I2 shunt.')
Parameter(id=16, name='ADC_I3_RAW', rw='R', range=(-32768, 32767), default=0, desc='Raw ADC measurement of the ADC I3 shunt.')
Parameter(id=17, name='CSA_GAIN_ADC_I0_TO_ADC_I2', rw='RWE', range=(0, 4), default=1, desc='Current sense amplifier gain for ADC I0, I1 and I2.', enum={0: 'GAIN_5X', 1: 'GAIN_10X', 2: 'GAIN_20X', 3: 'GAIN_40X', 4: 'GAIN_1X_BYPASS_CSA'})
Parameter(id=18, name='CSA_GAIN_ADC_I3', rw='RWE', range=(0, 4), default=1, desc='Current sense amplifier gain for ADC I3.', enum={0: 'GAIN_5X', 1: 'GAIN_10X', 2: 'GAIN_20X', 3: 'GAIN_40X', 4: 'GAIN_1X_BYPASS_CSA'})
Parameter(id=19, name='CSA_FILTER_ADC_I0_TO_ADC_I2', rw='RWE', range=(0, 3), default=0, desc='Current sense amplifier filter for ADC I0, I1 and I2.', enum={0: 'T_0_55_MICROSEC', 1: 'T_0_75_MICROSEC', 2: 'T_1_0_MICROSEC', 3: 'T_1_35_MICROSEC'})
Parameter(id=20, name='CSA_FILTER_ADC_I3', rw='RWE', range=(0, 3), default=0, desc='Current sense amplifier filter for ADC I3.', enum={0: 'T_0_55_MICROSEC', 1: 'T_0_75_MICROSEC', 2: 'T_1_0_MICROSEC', 3: 'T_1_35_MICROSEC'})
Parameter(id=21, name='CURRENT_SCALING_FACTOR', rw='RWE', range=(1, 65535), default=520, desc='Current scaling factor converting internal units to real- world units.')
Parameter(id=22, name='PHASE_UX1_ADC_MAPPING', rw='RWE', range=(0, 3), default=0, desc='Mapping ADC to UX1.', enum={0: 'ADC_I0', 1: 'ADC_I1', 2: 'ADC_I2', 3: 'ADC_I3'})
Parameter(id=23, name='PHASE_VX2_ADC_MAPPING', rw='RWE', range=(0, 3), default=1, desc='Mapping ADC to VX2.', enum={0: 'ADC_I0', 1: 'ADC_I1', 2: 'ADC_I2', 3: 'ADC_I3'})
Parameter(id=24, name='PHASE_WY1_ADC_MAPPING', rw='RWE', range=(0, 3), default=2, desc='Mapping ADC to WY1.', enum={0: 'ADC_I0', 1: 'ADC_I1', 2: 'ADC_I2', 3: 'ADC_I3'})
Parameter(id=25, name='PHASE_Y2_ADC_MAPPING', rw='RWE', range=(0, 3), default=3, desc='Mapping ADC to Y2.', enum={0: 'ADC_I0', 1: 'ADC_I1', 2: 'ADC_I2', 3: 'ADC_I3'})
Parameter(id=26, name='ADC_I0_SCALE', rw='RWE', range=(1, 32767), default=1024, desc='Scaling applied to ADC I0.')
Parameter(id=27, name='ADC_I1_SCALE', rw='RWE', range=(1, 32767), default=1024, desc='Scaling applied to ADC I1.')
Parameter(id=28, name='ADC_I2_SCALE', rw='RWE', range=(1, 32767), default=1024, desc='Scaling applied to ADC I2.')
Parameter(id=29, name='ADC_I3_SCALE', rw='RWE', range=(1, 32767), default=1024, desc='Scaling applied to ADC I3.')
Parameter(id=30, name='ADC_I0_INVERTED', rw='RWE', range=(0, 1), default=1, desc='Invert the reading of ADC I0.', enum={0: 'NOT_INVERTED', 1: 'INVERTED'})
Parameter(id=31, name='ADC_I1_INVERTED', rw='RWE', range=(0, 1), default=1, desc='Invert the reading of ADC I1.', enum={0: 'NOT_INVERTED', 1: 'INVERTED'})
Parameter(id=32, name='ADC_I2_INVERTED', rw='RWE', range=(0, 1), default=1, desc='Invert the reading of ADC I2.', enum={0: 'NOT_INVERTED', 1: 'INVERTED'})
Parameter(id=33, name='ADC_I3_INVERTED', rw='RWE', range=(0, 1), default=1, desc='Invert the reading of ADC I3.', enum={0: 'NOT_INVERTED', 1: 'INVERTED'})
Parameter(id=34, name='ADC_I0_OFFSET', rw='RWE', range=(-32768, 32767), default=0, desc='Offset applied to ADC I0 measurement.')
Parameter(id=35, name='ADC_I1_OFFSET', rw='RWE', range=(-32768, 32767), default=0, desc='Offset applied to ADC I1 measurement.')
Parameter(id=36, name='ADC_I2_OFFSET', rw='RWE', range=(-32768, 32767), default=0, desc='Offset applied to ADC I2 measurement.')
Parameter(id=37, name='ADC_I3_OFFSET', rw='RWE', range=(-32768, 32767), default=0, desc='Offset applied to ADC I3 measurement.')
Parameter(id=38, name='ADC_I0', rw='R', range=(-32768, 32767), default=0, desc='Scaled and offset compensated ADC I0 measurement.')
Parameter(id=39, name='ADC_I1', rw='R', range=(-32768, 32767), default=0, desc='Scaled and offset compensated ADC I1 measurement.')
Parameter(id=40, name='ADC_I2', rw='R', range=(-32768, 32767), default=0, desc='Scaled and offset compensated ADC I2 measurement.')
Parameter(id=41, name='ADC_I3', rw='R', range=(-32768, 32767), default=0, desc='Scaled and offset compensated ADC I3 measurement.')
Parameter(id=45, name='OPENLOOP_ANGLE', rw='R', range=(-32768, 32767), default=0, desc='Phi_e calculated by the ramper hardware. Used for commutation in openloop modes.')
Parameter(id=46, name='OPENLOOP_CURRENT', rw='RWE', range=(0, 65535), default=1000, desc='Openloop current applied in openloop, current mode.', unit='mA')
Parameter(id=47, name='OPENLOOP_VOLTAGE', rw='RWE', range=(0, 16383), default=0, desc='Openloop voltage applied in openloop, voltage mode.')
Parameter(id=50, name='ACCELERATION_FF_GAIN', rw='RWE', range=(0, 65535), default=8, desc='Gain applied to acceleration feedforward.')
Parameter(id=51, name='ACCELERATION_FF_SHIFT', rw='RWE', range=(0, 6), default=4, desc='Shift applied to acceleration feedforward.', enum={0: 'NO_SHIFT', 1: 'SHIFT_4_BIT', 2: 'SHIFT_8_BIT', 3: 'SHIFT_12_BIT', 4: 'SHIFT_16_BIT', 5: 'SHIFT_20_BIT', 6: 'SHIFT_24_BIT'})
Parameter(id=52, name='RAMP_ENABLE', rw='RWE', range=(0, 1), default=0, desc='Enable the application of acceleration and deceleration ramps.', enum={0: 'DISABLED', 1: 'ENABLED'})
Parameter(id=53, name='DIRECT_VELOCITY_MODE', rw='RWE', range=(0, 1), default=1, desc='Specify the control loop structure for velocity mode. Directly regulating the velocity or regulating on a constantly calculated target position.', enum={0: 'DISABLED', 1: 'ENABLED'})
Parameter(id=54, name='RAMP_AMAX', rw='RWE', range=(1, 8388607), default=1000, desc='Acceleration in top part of eight-point ramp.', unit='internal')
Parameter(id=55, name='RAMP_A1', rw='RWE', range=(1, 8388607), default=8000, desc='First acceleration in eight-point ramp.', unit='internal')
Parameter(id=56, name='RAMP_A2', rw='RWE', range=(1, 8388607), default=4000, desc='Second acceleration in eight-point ramp.', unit='internal')
Parameter(id=57, name='RAMP_DMAX', rw='RWE', range=(1, 8388607), default=1000, desc='Deceleration in top part of eight-point ramp.', unit='internal')
Parameter(id=58, name='RAMP_D1', rw='RWE', range=(1, 8388607), default=8000, desc='Second deceleration in eight-point ramp.', unit='internal')
Parameter(id=59, name='RAMP_D2', rw='RWE', range=(1, 8388607), default=8000, desc='First deceleration in eight-point ramp.', unit='internal')
Parameter(id=60, name='RAMP_VMAX', rw='RWE', range=(0, 134217727), default=134217727, desc='Maximum velocity of eight-point ramp.', unit='internal')
Parameter(id=61, name='RAMP_V1', rw='RWE', range=(0, 134217727), default=0, desc='Velocity threshold to switch from A1/D1 to A2/D2.', unit='internal')
Parameter(id=62, name='RAMP_V2', rw='RWE', range=(0, 134217727), default=0, desc='Velocity threshold to switch from A2/D2 to AMAX/DMAX.', unit='internal')
Parameter(id=63, name='RAMP_VSTART', rw='RWE', range=(0, 8388607), default=0, desc='Start velocity of ramp.', unit='internal')
Parameter(id=64, name='RAMP_VSTOP', rw='RWE', range=(1, 8388607), default=1, desc='Stop velocity of ramp. Needs to be greater than 0.', unit='internal')
Parameter(id=65, name='RAMP_TVMAX', rw='RWE', range=(0, 65535), default=0, desc='Minimum time at VMAX to start deceleration.', unit='internal')
Parameter(id=66, name='RAMP_TZEROWAIT', rw='RWE', range=(0, 65535), default=0, desc='Wait time at end of ramp to signal stop.', unit='internal')
Parameter(id=67, name='ACCELERATION_FEEDFORWARD_ENABLE', rw='RWE', range=(0, 1), default=0, desc='Enable the acceleration feedforward feature.', enum={0: 'DISABLED', 1: 'ENABLED'})
Parameter(id=68, name='VELOCITY_FEEDFORWARD_ENABLE', rw='RWE', range=(0, 1), default=0, desc='Enable the velocity feedforward feature.', enum={0: 'DISABLED', 1: 'ENABLED'})
Parameter(id=69, name='RAMP_VELOCITY', rw='R', range=(-134217727, 134217727), default=0, desc='Target velocity calculated by ramp controller.')
Parameter(id=70, name='RAMP_POSITION', rw='R', range=(-2147483648, 2147483647), default=0, desc='Target position calculated by ramp controller.')
Parameter(id=74, name='HALL_PHI_E', rw='R', range=(-32768, 32767), default=0, desc='Phi_e calculated from hall feedback.')
Parameter(id=75, name='HALL_SECTOR_OFFSET', rw='RWE', range=(0, 5), default=0, desc='Hall sensor 60-degree/sector offset composed of 120 offset (order) and 180 degree offset (polarity).', enum={0: 'DEG_0', 1: 'DEG_60', 2: 'DEG_120', 3: 'DEG_180', 4: 'DEG_240', 5: 'DEG_300'})
Parameter(id=76, name='HALL_FILTER_LENGTH', rw='RWE', range=(0, 255), default=0, desc='Filter length of the hall sensor input signal filters.')
Parameter(id=77, name='HALL_POSITION_0_OFFSET', rw='RWE', range=(-32768, 32767), default=0, desc='Hall offset compensation for 0 degree hall position.')
Parameter(id=78, name='HALL_POSITION_60_OFFSET', rw='RWE', range=(-32768, 32767), default=10922, desc='Hall offset compensation for 60 degree hall position.')
Parameter(id=79, name='HALL_POSITION_120_OFFSET', rw='RWE', range=(-32768, 32767), default=21845, desc='Hall offset compensation for 120 degree hall position.')
Parameter(id=80, name='HALL_POSITION_180_OFFSET', rw='RWE', range=(-32768, 32767), default=-32768, desc='Hall offset compensation for 180 degree hall position.')
Parameter(id=81, name='HALL_POSITION_240_OFFSET', rw='RWE', range=(-32768, 32767), default=-21846, desc='Hall offset compensation for 240 degree hall position.')
Parameter(id=82, name='HALL_POSITION_300_OFFSET', rw='RWE', range=(-32768, 32767), default=-10923, desc='Hall offset compensation for 300 degree hall position.')
Parameter(id=83, name='HALL_INVERT_DIRECTION', rw='RWE', range=(0, 1), default=0, desc='Invert the hall angle direction.', enum={0: 'NOT_INVERTED', 1: 'INVERTED'})
Parameter(id=84, name='HALL_EXTRAPOLATION_ENABLE', rw='RWE', range=(0, 1), default=0, desc='Enable the hall extrapolation to generate a higher resolution position signal. The extrapolation is only active at speeds higher than 60 rpm.', enum={0: 'DISABLED', 1: 'ENABLED'})
Parameter(id=85, name='HALL_PHI_E_OFFSET', rw='RWE', range=(-32768, 32767), default=0, desc='Use this parameter to compensate hall sensor mounting tolerances.')
Parameter(id=89, name='ABN_1_PHI_E', rw='R', range=(-32768, 32767), default=0, desc='Phi_e calculated from abn feedback.')
Parameter(id=90, name='ABN_1_STEPS', rw='RWE', range=(0, 16777215), default=65536, desc='ABN 1 encoder steps per rotation (CPR).')
Parameter(id=91, name='ABN_1_DIRECTION', rw='RWE', range=(0, 1), default=0, desc='ABN 1 encoder rotation direction.', enum={0: 'NOT_INVERTED', 1: 'INVERTED'})
Parameter(id=92, name='ABN_1_INIT_METHOD', rw='RWE', range=(0, 3), default=0, desc="Select an ABN encoder initialization method that fits best to your motor's sensors. Forces the rotor into phi_e zero using the open loop current but actively swings the rotor. Forces the rotor into phi_e 90 degree position and then into zero position using the open loop current. Turns the motor slightly in hall commutation mode until a hall signal change gives a new absolut position, which then the ABN phi_e is aligned to. Turns the motor slightly in open loop commutation mode until a N-channel is reached and gives an absolut position, which then the ABN phi_e is aligned to.", enum={0: 'FORCED_PHI_E_ZERO_WITH_ACTIVE_SWING', 1: 'FORCED_PHI_E_90_ZERO', 2: 'USE_HALL', 3: 'USE_N_CHANNEL_OFFSET'})
Parameter(id=93, name='ABN_1_INIT_STATE', rw='R', range=(0, 3), default=0, desc='Actual state of ABN encoder initialization.', enum={0: 'IDLE', 1: 'BUSY', 2: 'WAIT', 3: 'DONE'})
Parameter(id=94, name='ABN_1_INIT_DELAY', rw='RWE', range=(1000, 10000), default=1000, desc='When one of the "Forced phi_e" initialization methods is used, this value defines the wait time until the phi_e ABN angle is set to zero. This parameter should be set in a way, that the motor has stopped mechanical oscillations after the specified time.', unit='ms')
Parameter(id=95, name='ABN_1_INIT_VELOCITY', rw='RWE', range=(-200000, 200000), default=5, desc='Init velocity for ABN encoder initialization with N- channel offset.')
Parameter(id=96, name='ABN_1_N_CHANNEL_PHI_E_OFFSET', rw='RWE', range=(-32768, 32767), default=0, desc='Offset between phi_e zero and the ABN encoders index pulse position. This value is updated asynchronously on any ABN initialization other than the "Use-N channel offset" method. The value can then be used for the "Use N-channel offset" based initialization.')
Parameter(id=97, name='ABN_1_N_CHANNEL_INVERTED', rw='RWE', range=(0, 1), default=0, desc='ABN 1 encoder N-channel is inverted.', enum={0: 'ACTIVE_HIGH', 1: 'ACTIVE_LOW'})
Parameter(id=98, name='ABN_1_N_CHANNEL_FILTERING', rw='RWE', range=(0, 4), default=0, desc='ABN 1 encoder N-channel filtering. Useful for imprecise encoders with the index pulses lasting multiple A/B steps.', enum={0: 'FILTERING_OFF', 1: 'N_EVENT_ON_A_HIGH_B_HIGH', 2: 'N_EVENT_ON_A_HIGH_B_LOW', 3: 'N_EVENT_ON_A_LOW_B_HIGH', 4: 'N_EVENT_ON_A_LOW_B_LOW'})
Parameter(id=99, name='ABN_1_CLEAR_ON_NEXT_NULL', rw='RW', range=(0, 1), default=0, desc='Clear the actual position on the next ABN 1 encoder N- channel event.', enum={0: 'DISABLED', 1: 'ENABLED'})
Parameter(id=100, name='ABN_1_VALUE', rw='R', range=(0, 16777215), default=0, desc='Raw ABN encoder internal counter value.')
Parameter(id=104, name='TARGET_TORQUE', rw='RW', range=(-32768, 32767), default=0, desc='Target torque value. Write to activate torque regulation.', unit='mA')
Parameter(id=105, name='ACTUAL_TORQUE', rw='R', range=(-32767, 32768), default=0, desc='Actual motor torque value.', unit='mA')
Parameter(id=106, name='TARGET_FLUX', rw='RW', range=(-10000, 10000), default=0, desc='Target flux value.', unit='mA')
Parameter(id=107, name='ACTUAL_FLUX', rw='R', range=(-2147483648, 2147483647), default=0, desc='Actual motor flux value.', unit='mA')
Parameter(id=108, name='TORQUE_OFFSET', rw='RW', range=(-4700, 4700), default=0, desc='Offset applied to torque value.', unit='mA] (peak)')
Parameter(id=109, name='TORQUE_P', rw='RWE', range=(0, 32767), default=50, desc='P parameter for torque PI regulator. Also controls flux P parameter unless separate torque/flux loops are enabled.')
Parameter(id=110, name='TORQUE_I', rw='RWE', range=(0, 32767), default=100, desc='I parameter for torque PI regulator. Also controls flux I parameter unless separate torque/flux loops are enabled.')
Parameter(id=111, name='FLUX_P', rw='RWE', range=(0, 32767), default=50, desc='P parameter for flux PI regulator. Only available when separated torque/flux loops are enabled.')
Parameter(id=112, name='FLUX_I', rw='RWE', range=(0, 32767), default=100, desc='I parameter for flux PI regulator. Only available when separated torque/flux loops are enabled.')
Parameter(id=113, name='SEPARATE_TORQUE_FLUX_PI_PARAMTERS', rw='RWE', range=(0, 1), default=0, desc='Enable to configure separate PI values for the torque and flux current control loops.', enum={0: 'TORQUE_FLUX_PI_COMBINED', 1: 'TORQUE_FLUX_PI_SEPARATED'})
Parameter(id=114, name='CURRENT_NORM_P', rw='RWE', range=(0, 1), default=0, desc='P parameter normalization format for current PI regulator.', enum={0: 'SHIFT_8_BIT', 1: 'SHIFT_16_BIT'})
Parameter(id=115, name='CURRENT_NORM_I', rw='RWE', range=(0, 1), default=1, desc='I parameter normalization format for current PI regulator.', enum={0: 'SHIFT_8_BIT', 1: 'SHIFT_16_BIT'})
Parameter(id=116, name='TORQUE_PI_ERROR', rw='R', range=(-2147483648, 2147483647), default=0, desc='Torque PI regulator error.')
Parameter(id=117, name='FLUX_PI_ERROR', rw='R', range=(-2147483648, 2147483647), default=0, desc='Flux PI regulator error.')
Parameter(id=118, name='TORQUE_PI_INTEGRATOR', rw='R', range=(-2147483648, 2147483647), default=0, desc='Integrated error of torque PI regulator.')
Parameter(id=119, name='FLUX_PI_INTEGRATOR', rw='R', range=(-2147483648, 2147483647), default=0, desc='Integrated error of flux PI regulator.')
Parameter(id=120, name='FLUX_OFFSET', rw='RW', range=(-4700, 4700), default=0, desc='Offset applied to flux value.', unit='mA] (peak)')
Parameter(id=123, name='VELOCITY_SENSOR_SELECTION', rw='RWE', range=(0, 4), default=0, desc='Feedback source for the velocity PI regulator.', enum={0: 'SAME_AS_COMMUTATION', 1: 'DIGITAL_HALL', 2: 'ABN1_ENCODER', 3: 'ABN2_ENCODER', 4: 'SPI_ENCODER'})
Parameter(id=124, name='TARGET_VELOCITY', rw='RW', range=(-134217728, 134217727), default=0, desc='Target velocity value. Write to activate velocity regulation.')
Parameter(id=125, name='ACTUAL_VELOCITY', rw='R', range=(-2147483648, 2147483647), default=0, desc='Actual velocity value.')
Parameter(id=126, name='VELOCITY_OFFSET', rw='RW', range=(-200000, 200000), default=0, desc='Offset applied to velocity value.', unit='rpm')
Parameter(id=127, name='VELOCITY_P', rw='RWE', range=(0, 32767), default=800, desc='P parameter for velocity PI regulator.')
Parameter(id=128, name='VELOCITY_I', rw='RWE', range=(0, 32767), default=1, desc='I parameter for velocity PI regulator.')
Parameter(id=129, name='VELOCITY_NORM_P', rw='RWE', range=(0, 3), default=2, desc='P parameter normalization format for velocity PI regulator.', enum={0: 'NO_SHIFT', 1: 'SHIFT_8_BIT', 2: 'SHIFT_16_BIT', 3: 'SHIFT_24_BIT'})
Parameter(id=130, name='VELOCITY_NORM_I', rw='RWE', range=(0, 3), default=2, desc='I parameter normalization format for velocity PI regulator.', enum={0: 'SHIFT_8_BIT', 1: 'SHIFT_16_BIT', 2: 'SHIFT_24_BIT', 3: 'SHIFT_32_BIT'})
Parameter(id=131, name='VELOCITY_PI_INTEGRATOR', rw='R', range=(-2147483648, 2147483647), default=0, desc='Integrated error of velocity PI regulator.')
Parameter(id=132, name='VELOCITY_PI_ERROR', rw='R', range=(-2147483648, 2147483647), default=0, desc='Velocity PI regulator error.')
Parameter(id=133, name='VELOCITY_SCALING_FACTOR', rw='RWE', range=(1, 2047), default=1, desc='Scaling factor to convert internal velocity to real-world units.')
Parameter(id=134, name='STOP_ON_VELOCITY_DEVIATION', rw='RW', range=(0, 200000), default=0, desc='Maximum of velocity deviation tolerated before stop event is triggered (if activated).')
Parameter(id=135, name='VELOCITY_LOOP_DOWNSAMPLING', rw='RWE', range=(0, 127), default=5, desc='Downsampling factor for velocity controller.')
Parameter(id=136, name='VELOCITY_REACHED_THRESHOLD', rw='RWE', range=(0, 2000000000), default=0, desc='Deviation between target and actual velocity below which the velocity reached flag goes active and latches. If a new target velocity is set the flag is reset.')
Parameter(id=137, name='VELOCITY_METER_SWITCH_THRESHOLD', rw='RWE', range=(0, 134217727), default=2000, desc='Velocity threshold switching from period to frequency velocity meter.')
Parameter(id=138, name='VELOCITY_METER_SWITCH_HYSTERESIS', rw='RWE', range=(0, 65535), default=500, desc='Velocity hysteresis for switching back from frequency to period velocity meter.')
Parameter(id=139, name='VELOCITY_METER_MODE', rw='R', range=(0, 2), default=0, desc='Currently used velocity meter mode. Measurement of velocity by time measurement between position changes. Velocity Meter running at PWM frequency. Calculates the velocity using the difference of the angle in one clock cycle. Measurement of velocity by software.', enum={0: 'PERIOD_METER', 1: 'FREQUENCY_METER', 2: 'SOFTWARE_METER'})
Parameter(id=142, name='POSITION_SENSOR_SELECTION', rw='RWE', range=(0, 4), default=0, desc='Feedback source for the position PI regulator.', enum={0: 'SAME_AS_COMMUTATION', 1: 'DIGITAL_HALL', 2: 'ABN1_ENCODER', 3: 'ABN2_ENCODER', 4: 'SPI_ENCODER'})
Parameter(id=143, name='TARGET_POSITION', rw='RW', range=(-2147483648, 2147483647), default=0, desc='Target position value. Write to activate position regulation.')
Parameter(id=144, name='ACTUAL_POSITION', rw='RW', range=(-2147483648, 2147483647), default=0, desc='Actual position value.')
Parameter(id=145, name='POSITION_SCALING_FACTOR', rw='RWE', range=(1024, 65535), default=1024, desc='Scaling factor to convert internal position to real-world units.')
Parameter(id=146, name='POSITION_P', rw='RWE', range=(0, 32767), default=5, desc='P parameter for position PI regulator.')
Parameter(id=147, name='POSITION_I', rw='RWE', range=(0, 32767), default=0, desc='I parameter for position PI regulator.')
Parameter(id=148, name='POSITION_NORM_P', rw='RWE', range=(0, 3), default=1, desc='P parameter normalization format for position PI regulator.', enum={0: 'NO_SHIFT', 1: 'SHIFT_8_BIT', 2: 'SHIFT_16_BIT', 3: 'SHIFT_24_BIT'})
Parameter(id=149, name='POSITION_NORM_I', rw='RWE', range=(0, 3), default=1, desc='I parameter normalization format for position PI regulator.', enum={0: 'SHIFT_8_BIT', 1: 'SHIFT_16_BIT', 2: 'SHIFT_24_BIT', 3: 'SHIFT_32_BIT'})
Parameter(id=150, name='POSITION_PI_INTEGRATOR', rw='R', range=(-2147483648, 2147483647), default=0, desc='Integrated error of position PI regulator.')
Parameter(id=151, name='POSITION_PI_ERROR', rw='R', range=(-2147483648, 2147483647), default=0, desc='Error of position PI regulator.')
Parameter(id=152, name='STOP_ON_POSITION_DEVIATION', rw='RWE', range=(0, 2147483647), default=0, desc='Maximum of position deviation tolerated before stop event is triggered (if activated).')
Parameter(id=153, name='POSITION_LOOP_DOWNSAMPLING', rw='RWE', range=(0, 127), default=0, desc='Downsampling factor for position controller.')
Parameter(id=154, name='LATCH_POSITION', rw='R', range=(-2147483648, 2147483647), default=0, desc='Position switch latched.')
Parameter(id=155, name='POSITION_LIMIT_LOW', rw='RWE', range=(-2147483648, 2147483647), default=-2147483648, desc='Position limit low.')
Parameter(id=156, name='POSITION_LIMIT_HIGH', rw='RWE', range=(-2147483648, 2147483647), default=2147483647, desc='Position limit high.')
Parameter(id=157, name='POSITION_REACHED_THRESHOLD', rw='RWE', range=(0, 2000000000), default=0, desc='Deviation between target and actual position below which the position reached flag goes active and latches. If a new target position is set the flag is reset.')
Parameter(id=161, name='REFERENCE_SWITCH_ENABLE', rw='RWE', range=(0, 7), default=0, desc='Bitwise enable for stopping when reference switch input is triggered.', enum={4: 'STOP_ON_H', 2: 'STOP_ON_R', 1: 'STOP_ON_L', 0: 'NO_STOP_ON_SWITCH_TRIGGERED', 3: 'STOP_ON_R_AND_L', 5: 'STOP_ON_H_AND_L', 6: 'STOP_ON_H_AND_R', 7: 'STOP_ON_H_R_AND_L'})
Parameter(id=162, name='REFERENCE_SWITCH_POLARITY_AND_SWAP', rw='RWE', range=(0, 15), default=0, desc='Bitwise configuration of reference switch configuration. Options to swap left and right input and invert switch polarity.', enum={8: 'L_R_SWAPPED_L_INVERTED', 4: 'H_INVERTED', 2: 'R_INVERTED', 1: 'L_INVERTED', 0: 'NOT_SWAPPED_NOT_INVERTED', 3: 'R_AND_L_INVERTED', 5: 'H_AND_L_INVERTED', 6: 'H_AND_R_INVERTED', 7: 'H_R_AND_L_INVERTED', 9: 'L_R_SWAPPED_R_INVERTED', 10: 'L_R_SWAPPED_R_AND_L_INVERTED', 11: 'L_R_SWAPPED_H_INVERTED', 12: 'L_R_SWAPPED_H_AND_L_INVERTED', 13: 'L_R_SWAPPED', 14: 'L_R_SWAPPED_H_AND_R_INVERTED', 15: 'L_R_SWAPPED_H_R_AND_L_INVERTED'})
Parameter(id=163, name='REFERENCE_SWITCH_LATCH_SETTINGS', rw='RWE', range=(0, 15), default=0, desc='Bitwise configuration of reference switch latch configuration. Writing position to latch position parameter.', enum={8: 'H_FALLING_EDGE', 4: 'H_RISING_EDGE', 2: 'L_R_FALLING_EDGE', 1: 'L_R_RISING_EDGE', 0: 'NO_TRIGGER', 3: 'L_R_BOTH_EDGES', 5: 'H_L_R_RISING_EDGE', 6: 'H_RISING_L_R_FALLING_EDGE', 7: 'H_RISING_L_R_BOTH_EDGES', 9: 'H_FALLING_L_R_RISING_EDGE', 10: 'H_L_R_FALLING_EDGE', 11: 'H_FALLING_L_R_BOTH_EDGES', 12: 'H_BOTH_EDGES', 13: 'H_BOTH_L_R_RISING_EDGE', 14: 'H_BOTH_L_R_FALLING_EDGE', 15: 'H_L_R_BOTH_EDGES'})
Parameter(id=164, name='EVENT_STOP_SETTINGS', rw='RWE', range=(0, 7), default=0, desc='Bitwise configuration of stop configuration. parameter "Stop on velocity deviation". the parameter "Stop on position deviation". condition rises. Doing a soft and not a hard stop.', enum={4: 'STOP_ON_VEL_DEVIATION', 2: 'STOP_ON_POS_DEVIATION', 1: 'DO_SOFT_STOP', 0: 'DO_HARD_STOP', 3: 'STOP_ON_POS_DEVIATION_SOFT_STOP', 5: 'STOP_ON_VEL_DEVIATION_SOFT_STOP', 6: 'STOP_ON_POS_VEL_DEVIATION', 7: 'STOP_ON_POS_VEL_DEVIATION_SOFT_STOP'})
Parameter(id=165, name='REFERENCE_SWITCH_SEARCH_MODE', rw='RWE', range=(1, 8), default=0, desc='Determine the reference switch search sequence. Search for left limit switch. Search for right limit switch then search for left limit switch. Search for right limit switch then approach left limit switch from both sides. Approach left limit switch from both sides. Search for home switch in negative direction, turn around if left end switch detected. Search for home switch in positive direction, turn around if right end switch detected. Search for home switch in negative direction, ignore end switch. Search for home switch in positive direction, ignore end switch.', enum={1: 'LEFT_SWITCH', 2: 'RIGHT_SWITCH_LEFT_SWITCH', 3: 'RIGHT_SWITCH_LEFT_SWITCH_BOTH_SIDES', 4: 'LEFT_SWITCH_BOTH_SIDES', 5: 'HOME_SWITCH_NEG_DIR_LEFT_END_SWITCH', 6: 'HOME_SWITCH_POS_DIR_RIGHT_END_SWITCH', 7: 'HOME_SWITCH_NEG_DIR_IGNORE_END_SWITCH', 8: 'HOME_SWITCH_POS_DIR_IGNORE_END_SWITCH'})
Parameter(id=166, name='REFERENCE_SWITCH_SEARCH_SPEED', rw='RWE', range=(-134217728, 134217727), default=0, desc='Speed used for the reference switch search sequence.')
Parameter(id=167, name='REFERENCE_SWITCH_SPEED', rw='RWE', range=(-134217728, 134217727), default=0, desc='Lower speed used e.g. for positioning the motor at a reference switch position.')
Parameter(id=168, name='RIGHT_LIMIT_SWITCH_POSITION', rw='R', range=(-2147483648, 2147483647), default=0, desc='Right limit switch position.')
Parameter(id=169, name='HOME_SWITCH_POSITION', rw='R', range=(-2147483648, 2147483647), default=0, desc='Home switch position.')
Parameter(id=170, name='LAST_REFERENCE_POSITION', rw='R', range=(-2147483648, 2147483647), default=0, desc='Last reference position.')
Parameter(id=174, name='ABN_2_STEPS', rw='RWE', range=(0, 16777215), default=1024, desc='ABN 2 encoder steps per rotation (CPR).')
Parameter(id=175, name='ABN_2_DIRECTION', rw='RWE', range=(0, 1), default=0, desc='ABN 2 encoder rotation direction.', enum={0: 'NORMAL', 1: 'INVERTED'})
Parameter(id=176, name='ABN_2_GEAR_RATIO', rw='RWE', range=(1, 255), default=1, desc='ABN 2 encoder gear ratio.')
Parameter(id=177, name='ABN_2_ENABLE', rw='RWE', range=(0, 1), default=0, desc='Enable the ABN 2 encoder. Disabling resets the counted steps.', enum={0: 'DISABLED', 1: 'ENABLED'})
Parameter(id=178, name='ABN_2_VALUE', rw='R', range=(0, 4294967295), default=0, desc='Raw ABN2 encoder internal counter value.')
Parameter(id=181, name='SPI_ENCODE_CS_SETTLE_DELAY_TIME', rw='RWE', range=(0, 6375), default=0, desc='Add a delay from CS going low to first SCLK edge.', unit='ns')
Parameter(id=182, name='SPI_ENCODER_CS_IDLE_DELAY_TIME', rw='RWE', range=(0, 102), default=0, desc='Extend CS idle time between SPI message frames.', unit='us')
Parameter(id=183, name='SPI_ENCODER_MAIN_TRANSFER_CMD_SIZE', rw='RWE', range=(1, 16), default=1, desc='Size of the first SPI transfer frame.')
Parameter(id=184, name='SPI_ENCODER_SECONDARY_TRANSFER_CMD_SIZE', rw='RWE', range=(0, 15), default=0, desc='Size of the optional secondary SPI transfer frame. If set to zero, no secondary SPI transfer.')
Parameter(id=185, name='SPI_ENCODER_TRANSFER_DATA_3_0', rw='RWE', range=(0, 4294967295), default=0, desc='Used to set the transmit data and read out the received data.')
Parameter(id=186, name='SPI_ENCODER_TRANSFER_DATA_7_4', rw='RWE', range=(0, 4294967295), default=0, desc='Used to set the transmit data and read out the received data.')
Parameter(id=187, name='SPI_ENCODER_TRANSFER_DATA_11_8', rw='RWE', range=(0, 4294967295), default=0, desc='Used to set the transmit data and read out the received data.')
Parameter(id=188, name='SPI_ENCODER_TRANSFER_DATA_15_12', rw='RWE', range=(0, 4294967295), default=0, desc='Used to set the transmit data and read out the received data.')
Parameter(id=189, name='SPI_ENCODER_TRANSFER', rw='RWE', range=(0, 2), default=0, desc='SPI interface setting, polarity and phase.', enum={0: 'OFF', 1: 'TRIGGER_SINGLE_TRANSFER', 2: 'CONTINUOUS_POSITION_COUNTER_READ'})
Parameter(id=190, name='SPI_ENCODER_POSITION_COUNTER_MASK', rw='RWE', range=(0, 4294967295), default=0, desc='Mask to be used to collect the position counter value from the continuous received data.')
Parameter(id=191, name='SPI_ENCODER_POSITION_COUNTER_SHIFT', rw='RWE', range=(0, 127), default=0, desc='Right bit shift for the position counter value before mask is applied.')
Parameter(id=192, name='SPI_ENCODER_POSITION_COUNTER_VALUE', rw='R', range=(0, 4294967295), default=0, desc='Actual SPI encoder position value.')
Parameter(id=193, name='SPI_ENCODER_COMMUTATION_ANGLE', rw='R', range=(-32768, 32767), default=0, desc='Actual absolute encoder angle value.')
Parameter(id=194, name='SPI_ENCODER_INITIALIZATION_METHOD', rw='RWE', range=(0, 2), default=0, desc='Select the used absolute encoder initialization mode Forces the rotor into PHI_E zero using the open loop current but actively swings the rotor. Forces the rotor into PHI_E 90 degree position and then into zero position using the open loop current.', enum={0: 'FORCED_PHI_E_ZERO_WITH_ACTIVE_SWING', 1: 'FORCED_PHI_E_90_ZERO', 2: 'USE_OFFSET'})
Parameter(id=195, name='SPI_ENCODER_DIRECTION', rw='RWE', range=(0, 1), default=0, desc='SPI encoder direction.', enum={0: 'NOT_INVERTED', 1: 'INVERTED'})
Parameter(id=196, name='SPI_ENCODER_OFFSET', rw='RWE', range=(0, 4294967295), default=0, desc='This value represents the internal commutation offset. (0...max. encoder steps per rotation).')
Parameter(id=197, name='SPI_LUT_CORRECTION_ENABLE', rw='RWE', range=(0, 1), default=0, desc='Enable the lookup table based encoder correction.', enum={0: 'DISABLED', 1: 'ENABLED'})
Parameter(id=198, name='SPI_LUT_ADDRESS_SELECT', rw='RW', range=(0, 255), default=0, desc='Address to read or write the lookup table.')
Parameter(id=199, name='SPI_LUT_DATA', rw='RW', range=(-128, 127), default=0, desc='Data to read or write to a lookup table address.')
Parameter(id=201, name='SPI_LUT_COMMON_SHIFT_FACTOR', rw='RW', range=(0, 4), default=0, desc='All LUT table entries are multiplied with 2^SHIFT_FACTOR to compensate for larger erros if needed.')
Parameter(id=205, name='STEP_DIR_STEP_DIVIDER_SHIFT', rw='RWE', range=(0, 10), default=0, desc='Configure step/dir to use between 1 and 1024 microsteps per full step.', enum={0: 'STEP_MODE_FULL', 1: 'STEP_MODE_HALF', 2: 'STEP_MODE_QUARTER', 3: 'STEP_MODE_1_8TH', 4: 'STEP_MODE_1_16TH', 5: 'STEP_MODE_1_32ND', 6: 'STEP_MODE_1_64TH', 7: 'STEP_MODE_1_128TH', 8: 'STEP_MODE_1_256TH', 9: 'STEP_MODE_1_512TH', 10: 'STEP_MODE_1_1024TH'})
Parameter(id=206, name='STEP_DIR_ENABLE', rw='RW', range=(0, 1), default=0, desc='Enable the Step/Dir input functionality.', enum={0: 'DISABLED', 1: 'ENABLED'})
Parameter(id=207, name='STEP_DIR_EXTRAPOLATION_ENABLE', rw='RW', range=(0, 1), default=0, desc='Enable the Step/Dir extrapolation feature.', enum={0: 'DISABLED', 1: 'ENABLED'})
Parameter(id=208, name='STEP_DIR_STEP_SIGNAL_TIMEOUT_LIMIT', rw='RW', range=(1, 2000), default=1000, desc='Step signal timeout limit.', unit='ms')
Parameter(id=209, name='STEP_DIR_MAXIMUM_EXTRAPOLATION_VELOCITY', rw='RW', range=(0, 2147483647), default=2147483647, desc='Maximum velocity up to which extrapolation will be used.', unit='eRPM')
Parameter(id=212, name='BRAKE_CHOPPER_ENABLE', rw='RWE', range=(0, 1), default=0, desc='Enable the brake chopper functionality.', enum={0: 'DISABLED', 1: 'ENABLED'})
Parameter(id=213, name='BRAKE_CHOPPER_VOLTAGE_LIMIT', rw='RWE', range=(50, 1000), default=260, desc='If the brake chopper is enabled and supply voltage exceeds this value, the brake chopper output will be activated.', unit='0.1V')
Parameter(id=214, name='BRAKE_CHOPPER_HYSTERESIS', rw='RWE', range=(0, 50), default=5, desc='An activated brake chopper will be deactivated if the actual supply voltage is lower than BRAKE_CHOPPER_VOLTAGE_LIMIT - BRAKE_CHOPPER_HYSTERESIS.', unit='0.1V')
Parameter(id=216, name='RELEASE_BRAKE', rw='RWE', range=(0, 1), default=0, desc='Release the external brake by applying a PWM signal.', enum={0: 'BRAKE_PWM_DEACTIVATED', 1: 'BRAKE_PWM_ACTIVATED'})
Parameter(id=217, name='BRAKE_RELEASING_DUTY_CYCLE', rw='RWE', range=(0, 99), default=75, desc='Set the duty cycle of the first PWM phase for releasing the brake.', unit='%')
Parameter(id=218, name='BRAKE_HOLDING_DUTY_CYCLE', rw='RWE', range=(0, 99), default=11, desc='Set the duty cycle of the second PWM phase to hold the brake.', unit='%')
Parameter(id=219, name='BRAKE_RELEASING_DURATION', rw='RWE', range=(0, 65535), default=80, desc='Set the duration the brake PWM uses the first duty cycle.', unit='ms')
Parameter(id=221, name='INVERT_BRAKE_OUTPUT', rw='RWE', range=(0, 1), default=0, desc='Invert the brake output.', enum={0: 'NORMAL', 1: 'INVERTED'})
Parameter(id=224, name='THERMAL_WINDING_TIME_CONSTANT_1', rw='RWE', range=(1000, 60000), default=3000, desc='Thermal winding time constant for the used motor. Used for IIT monitoring. Setting a new value restarts the IIT monitoring.', unit='ms')
Parameter(id=225, name='IIT_LIMIT_1', rw='RWE', range=(0, 4294967295), default=4294967295, desc='An actual IIT sum that exceeds this limit leads to trigger the IIT_1_EXCEEDED.', unit='A^2*ms')
Parameter(id=226, name='IIT_SUM_1', rw='R', range=(0, 4294967295), default=0, desc='Actual sum of the IIT monitor 1.', unit='A^2*ms')
Parameter(id=227, name='THERMAL_WINDING_TIME_CONSTANT_2', rw='RWE', range=(1000, 60000), default=6000, desc='Thermal winding time constant for the used motor. Used for IIT monitoring. Setting a new value restarts the IIT monitoring.', unit='ms')
Parameter(id=228, name='IIT_LIMIT_2', rw='RWE', range=(0, 4294967295), default=4294967295, desc='An actual IIT sum that exceeds this limit leads to trigger the IIT_2_EXCEEDED.', unit='A^2*ms')
Parameter(id=229, name='IIT_SUM_2', rw='R', range=(0, 4294967295), default=0, desc='Actual sum of the IIT monitor 2.', unit='A^2*ms')
Parameter(id=230, name='RESET_IIT_SUMS', rw='RW', range=(0, 0), default=0, desc='Reset both IIT sums.')
Parameter(id=231, name='ACTUAL_TOTAL_MOTOR_CURRENT', rw='R', range=(0, 65535), default=0, desc='Total current through the motor windings.', unit='mA')
Parameter(id=233, name='PWM_L_OUTPUT_POLARITY', rw='RWE', range=(0, 1), default=0, desc='PWM_L output polarity.', enum={0: 'ACTIVE_HIGH', 1: 'ACTIVE_LOW'})
Parameter(id=234, name='PWM_H_OUTPUT_POLARITY', rw='RWE', range=(0, 1), default=0, desc='PWM_H output polarity.', enum={0: 'ACTIVE_HIGH', 1: 'ACTIVE_LOW'})
Parameter(id=235, name='BREAK_BEFORE_MAKE_TIME_LOW_UVW', rw='RWE', range=(0, 255), default=0, desc='Break before make time for the low side gates of the UVW phases. Applied before switching from high to low.', unit='8.33ns')
Parameter(id=236, name='BREAK_BEFORE_MAKE_TIME_HIGH_UVW', rw='RWE', range=(0, 255), default=0, desc='Break before make time for the high side gates of the UVW phases. Applied before switching from low to high.', unit='8.33ns')
Parameter(id=237, name='BREAK_BEFORE_MAKE_TIME_LOW_Y2', rw='RWE', range=(0, 255), default=0, desc='Break before make time for the low side gate of the Y2 phase. Applied before switching from high to low.', unit='8.33ns')
Parameter(id=238, name='BREAK_BEFORE_MAKE_TIME_HIGH_Y2', rw='RWE', range=(0, 255), default=0, desc='Break before make time for the high side gate of the Y2 phase. Applied before switching from low to high.', unit='8.33ns')
Parameter(id=239, name='USE_ADAPTIVE_DRIVE_TIME_UVW', rw='RWE', range=(0, 1), default=1, desc='If enabled, the discharge cycle of the low and high side gates for the UVW phases will be shortened by monitoring the gate voltages. If enabled, the value on T_DRIVE_SINK will act as an upper bound instead of a fixed time.', enum={0: 'DISABLED', 1: 'ENABLED'})
Parameter(id=240, name='USE_ADAPTIVE_DRIVE_TIME_Y2', rw='RWE', range=(0, 1), default=1, desc='If enabled, the discharge cycle of the low and high side gates for the Y2 phase will be shortened by monitoring the gate voltages. If enabled, the value on T_DRIVE_SINK will act as an upper bound instead of a fixed time.', enum={0: 'DISABLED', 1: 'ENABLED'})
Parameter(id=241, name='DRIVE_TIME_SINK_UVW', rw='RWE', range=(0, 255), default=255, desc='Discharge time for the low and high side gates of the UVW phases. During this time, the full sink current will be applied. The applied time is defined as (1s / 120MHz) * (2 * DRIVE_TIME_SINK_UVW + 3).')
Parameter(id=242, name='DRIVE_TIME_SOURCE_UVW', rw='RWE', range=(0, 255), default=255, desc='Charge time for the low and high side gates of the UVW phases. During this time, the full source current will be applied. The applied time is defined as (1s / 120MHz) * (2 * DRIVE_TIME_SOURCE_UVW + 3).')
Parameter(id=243, name='DRIVE_TIME_SINK_Y2', rw='RWE', range=(0, 255), default=255, desc='Discharge time for the low and high side gates of the Y2 phase. During this time, the full sink current will be applied. The applied time is defined as (1s / 120MHz) * (2 * DRIVE_TIME_SINK_Y2 + 3).')
Parameter(id=244, name='DRIVE_TIME_SOURCE_Y2', rw='RWE', range=(0, 255), default=255, desc='Charge time for the low and high side gates of the Y2 phase. During this time, the full source current will be applied. The applied time is defined as (1s / 120MHz) * (2 * DRIVE_TIME_SOURCE_Y2 + 3).')
Parameter(id=245, name='UVW_SINK_CURRENT', rw='RWE', range=(0, 15), default=4, desc='Limit the maximum sink current for the low and high side gates of the UVW phases.', enum={0: 'CUR_50_MILLIAMP', 1: 'CUR_100_MILLIAMP', 2: 'CUR_160_MILLIAMP', 3: 'CUR_210_MILLIAMP', 4: 'CUR_270_MILLIAMP', 5: 'CUR_320_MILLIAMP', 6: 'CUR_380_MILLIAMP', 7: 'CUR_430_MILLIAMP', 8: 'CUR_580_MILLIAMP', 9: 'CUR_720_MILLIAMP', 10: 'CUR_860_MILLIAMP', 11: 'CUR_1000_MILLIAMP', 12: 'CUR_1250_MILLIAMP', 13: 'CUR_1510_MILLIAMP', 14: 'CUR_1770_MILLIAMP', 15: 'CUR_2000_MILLIAMP'})
Parameter(id=246, name='UVW_SOURCE_CURRENT', rw='RWE', range=(0, 15), default=4, desc='Limit the maximum source current for the low and high side gates of the UVW phases.', enum={0: 'CUR_25_MILLIAMP', 1: 'CUR_50_MILLIAMP', 2: 'CUR_80_MILLIAMP', 3: 'CUR_105_MILLIAMP', 4: 'CUR_135_MILLIAMP', 5: 'CUR_160_MILLIAMP', 6: 'CUR_190_MILLIAMP', 7: 'CUR_215_MILLIAMP', 8: 'CUR_290_MILLIAMP', 9: 'CUR_360_MILLIAMP', 10: 'CUR_430_MILLIAMP', 11: 'CUR_500_MILLIAMP', 12: 'CUR_625_MILLIAMP', 13: 'CUR_755_MILLIAMP', 14: 'CUR_855_MILLIAMP', 15: 'CUR_1000_MILLIAMP'})
Parameter(id=247, name='Y2_SINK_CURRENT', rw='RWE', range=(0, 15), default=4, desc='Limit the maximum sink current for the low and high side gates of the Y2 phase.', enum={0: 'CUR_50_MILLIAMP', 1: 'CUR_100_MILLIAMP', 2: 'CUR_160_MILLIAMP', 3: 'CUR_210_MILLIAMP', 4: 'CUR_270_MILLIAMP', 5: 'CUR_320_MILLIAMP', 6: 'CUR_380_MILLIAMP', 7: 'CUR_430_MILLIAMP', 8: 'CUR_580_MILLIAMP', 9: 'CUR_720_MILLIAMP', 10: 'CUR_860_MILLIAMP', 11: 'CUR_1000_MILLIAMP', 12: 'CUR_1250_MILLIAMP', 13: 'CUR_1510_MILLIAMP', 14: 'CUR_1770_MILLIAMP', 15: 'CUR_2000_MILLIAMP'})
Parameter(id=248, name='Y2_SOURCE_CURRENT', rw='RWE', range=(0, 15), default=4, desc='Limit the maximum source current for the low and high side gates of the Y2 phase.', enum={0: 'CUR_25_MILLIAMP', 1: 'CUR_50_MILLIAMP', 2: 'CUR_80_MILLIAMP', 3: 'CUR_105_MILLIAMP', 4: 'CUR_135_MILLIAMP', 5: 'CUR_160_MILLIAMP', 6: 'CUR_190_MILLIAMP', 7: 'CUR_215_MILLIAMP', 8: 'CUR_290_MILLIAMP', 9: 'CUR_360_MILLIAMP', 10: 'CUR_430_MILLIAMP', 11: 'CUR_500_MILLIAMP', 12: 'CUR_625_MILLIAMP', 13: 'CUR_755_MILLIAMP', 14: 'CUR_855_MILLIAMP', 15: 'CUR_1000_MILLIAMP'})
Parameter(id=249, name='BOOTSTRAP_CURRENT_LIMIT', rw='RWE', range=(0, 7), default=7, desc='Bootstrap current limit.', enum={0: 'CUR_45_MILLIAMP', 1: 'CUR_91_MILLIAMP', 2: 'CUR_141_MILLIAMP', 3: 'CUR_191_MILLIAMP', 4: 'CUR_267_MILLIAMP', 5: 'CUR_292_MILLIAMP', 6: 'CUR_341_MILLIAMP', 7: 'CUR_391_MILLIAMP'})
Parameter(id=250, name='UNDERVOLTAGE_PROTECTION_SUPPLY_LEVEL', rw='RWE', range=(0, 16), default=0, desc='Undervoltage protection level for VS (Supply voltage). 0 disables the comparator. 1-16 are mapped to 0-15 HW values, with the comparator enabled.')
Parameter(id=251, name='UNDERVOLTAGE_PROTECTION_VDRV_ENABLE', rw='RWE', range=(0, 1), default=1, desc='Enable the undervoltage protection for VDRV (Driver voltage).', enum={0: 'DISABLED', 1: 'ENABLED'})
Parameter(id=252, name='UNDERVOLTAGE_PROTECTION_BST_UVW_ENABLE', rw='RWE', range=(0, 1), default=1, desc='Enable the undervoltage protection on the bootstrap capacitor of the UVW phases.', enum={0: 'DISABLED', 1: 'ENABLED'})
Parameter(id=253, name='UNDERVOLTAGE_PROTECTION_BST_Y2_ENABLE', rw='RWE', range=(0, 1), default=1, desc='Enable the undervoltage protection on the bootstrap capacitor of the Y2 phase.', enum={0: 'DISABLED', 1: 'ENABLED'})
Parameter(id=254, name='OVERCURRENT_PROTECTION_UVW_LOW_SIDE_ENABLE', rw='RWE', range=(0, 1), default=1, desc='Enable the overcurrent protection on the low side of the UVW phases.', enum={0: 'DISABLED', 1: 'ENABLED'})
Parameter(id=255, name='OVERCURRENT_PROTECTION_UVW_HIGH_SIDE_ENABLE', rw='RWE', range=(0, 1), default=1, desc='Enable the overcurrent protection on the high side of the UVW phases.', enum={0: 'DISABLED', 1: 'ENABLED'})
Parameter(id=256, name='OVERCURRENT_PROTECTION_Y2_LOW_SIDE_ENABLE', rw='RWE', range=(0, 1), default=1, desc='Enable the overcurrent protection on the low side of the Y2 phase.', enum={0: 'DISABLED', 1: 'ENABLED'})
Parameter(id=257, name='OVERCURRENT_PROTECTION_Y2_HIGH_SIDE_ENABLE', rw='RWE', range=(0, 1), default=1, desc='Enable the overcurrent protection on the high side of the Y2 phase.', enum={0: 'DISABLED', 1: 'ENABLED'})
Parameter(id=258, name='OVERCURRENT_PROTECTION_UVW_LOW_SIDE_THRESHOLD', rw='RWE', range=(0, 15), default=0, desc='Overcurrent protection threshold for the low side of the UVW phases (uses second list if OVERCURRENT_PROTECTION_UVW_LOW_SIDE_USE_VD S=true).', enum={0: 'V_80_OR_63_MILLIVOLT', 1: 'V_165_OR_125_MILLIVOLT', 2: 'V_250_OR_187_MILLIVOLT', 3: 'V_330_OR_248_MILLIVOLT', 4: 'V_415_OR_312_MILLIVOLT', 5: 'V_500_OR_374_MILLIVOLT', 6: 'V_582_OR_434_MILLIVOLT', 7: 'V_660_OR_504_MILLIVOLT', 8: 'V_125_OR_705_MILLIVOLT', 9: 'V_250_OR_940_MILLIVOLT', 10: 'V_375_OR_1180_MILLIVOLT', 11: 'V_500_OR_1410_MILLIVOLT', 12: 'V_625_OR_1650_MILLIVOLT', 13: 'V_750_OR_1880_MILLIVOLT', 14: 'V_875_OR_2110_MILLIVOLT', 15: 'V_1000_OR_2350_MILLIVOLT'})
Parameter(id=259, name='OVERCURRENT_PROTECTION_UVW_HIGH_SIDE_THRESHOLD', rw='RWE', range=(0, 15), default=0, desc='Overcurrent protection threshold for the high side of the UVW phases.', enum={0: 'V_63_MILLIVOLT', 1: 'V_125_MILLIVOLT', 2: 'V_187_MILLIVOLT', 3: 'V_248_MILLIVOLT', 4: 'V_312_MILLIVOLT', 5: 'V_374_MILLIVOLT', 6: 'V_434_MILLIVOLT', 7: 'V_504_MILLIVOLT', 8: 'V_705_MILLIVOLT', 9: 'V_940_MILLIVOLT', 10: 'V_1180_MILLIVOLT', 11: 'V_1410_MILLIVOLT', 12: 'V_1650_MILLIVOLT', 13: 'V_1880_MILLIVOLT', 14: 'V_2110_MILLIVOLT', 15: 'V_2350_MILLIVOLT'})
Parameter(id=260, name='OVERCURRENT_PROTECTION_Y2_LOW_SIDE_THRESHOLD', rw='RWE', range=(0, 15), default=0, desc='Overcurrent protection threshold for the low side of the Y2 phase (uses second list if OVERCURRENT_PROTECTION_Y2_LOW_SIDE_USE_VDS= true).', enum={0: 'V_80_OR_63_MILLIVOLT', 1: 'V_165_OR_125_MILLIVOLT', 2: 'V_250_OR_187_MILLIVOLT', 3: 'V_330_OR_248_MILLIVOLT', 4: 'V_415_OR_312_MILLIVOLT', 5: 'V_500_OR_374_MILLIVOLT', 6: 'V_582_OR_434_MILLIVOLT', 7: 'V_660_OR_504_MILLIVOLT', 8: 'V_125_OR_705_MILLIVOLT', 9: 'V_250_OR_940_MILLIVOLT', 10: 'V_375_OR_1180_MILLIVOLT', 11: 'V_500_OR_1410_MILLIVOLT', 12: 'V_625_OR_1650_MILLIVOLT', 13: 'V_750_OR_1880_MILLIVOLT', 14: 'V_875_OR_2110_MILLIVOLT', 15: 'V_1000_OR_2350_MILLIVOLT'})
Parameter(id=261, name='OVERCURRENT_PROTECTION_Y2_HIGH_SIDE_THRESHOLD', rw='RWE', range=(0, 15), default=0, desc='Overcurrent protection threshold for the high side of the Y2 phase.', enum={0: 'V_63_MILLIVOLT', 1: 'V_125_MILLIVOLT', 2: 'V_187_MILLIVOLT', 3: 'V_248_MILLIVOLT', 4: 'V_312_MILLIVOLT', 5: 'V_374_MILLIVOLT', 6: 'V_434_MILLIVOLT', 7: 'V_504_MILLIVOLT', 8: 'V_705_MILLIVOLT', 9: 'V_940_MILLIVOLT', 10: 'V_1180_MILLIVOLT', 11: 'V_1410_MILLIVOLT', 12: 'V_1650_MILLIVOLT', 13: 'V_1880_MILLIVOLT', 14: 'V_2110_MILLIVOLT', 15: 'V_2350_MILLIVOLT'})
Parameter(id=262, name='OVERCURRENT_PROTECTION_UVW_LOW_SIDE_BLANKING', rw='RWE', range=(0, 7), default=2, desc='Overcurrent protection blanking time for the low side of the UVW phases.', enum={0: 'OFF', 1: 'T_0_25_MICROSEC', 2: 'T_0_5_MICROSEC', 3: 'T_1_MICROSEC', 4: 'T_2_MICROSEC', 5: 'T_4_MICROSEC', 6: 'T_6_MICROSEC', 7: 'T_8_MICROSEC'})
Parameter(id=263, name='OVERCURRENT_PROTECTION_UVW_HIGH_SIDE_BLANKING', rw='RWE', range=(0, 7), default=2, desc='Overcurrent protection blanking time for the high side of the UVW phases.', enum={0: 'OFF', 1: 'T_0_25_MICROSEC', 2: 'T_0_5_MICROSEC', 3: 'T_1_MICROSEC', 4: 'T_2_MICROSEC', 5: 'T_4_MICROSEC', 6: 'T_6_MICROSEC', 7: 'T_8_MICROSEC'})
Parameter(id=264, name='OVERCURRENT_PROTECTION_Y2_LOW_SIDE_BLANKING', rw='RWE', range=(0, 7), default=2, desc='Overcurrent protection blanking time for the low side of the Y2 phase.', enum={0: 'OFF', 1: 'T_0_25_MICROSEC', 2: 'T_0_5_MICROSEC', 3: 'T_1_MICROSEC', 4: 'T_2_MICROSEC', 5: 'T_4_MICROSEC', 6: 'T_6_MICROSEC', 7: 'T_8_MICROSEC'})
Parameter(id=265, name='OVERCURRENT_PROTECTION_Y2_HIGH_SIDE_BLANKING', rw='RWE', range=(0, 7), default=2, desc='Overcurrent protection blanking time for the high side of the Y2 phase.', enum={0: 'OFF', 1: 'T_0_25_MICROSEC', 2: 'T_0_5_MICROSEC', 3: 'T_1_MICROSEC', 4: 'T_2_MICROSEC', 5: 'T_4_MICROSEC', 6: 'T_6_MICROSEC', 7: 'T_8_MICROSEC'})
Parameter(id=266, name='OVERCURRENT_PROTECTION_UVW_LOW_SIDE_DEGLITCH', rw='RWE', range=(0, 7), default=6, desc='Overcurrent protection deglitch time for the low side of the UVW phases.', enum={0: 'OFF', 1: 'T_0_25_MICROSEC', 2: 'T_0_5_MICROSEC', 3: 'T_1_MICROSEC', 4: 'T_2_MICROSEC', 5: 'T_4_MICROSEC', 6: 'T_6_MICROSEC', 7: 'T_8_MICROSEC'})
Parameter(id=267, name='OVERCURRENT_PROTECTION_UVW_HIGH_SIDE_DEGLITCH', rw='RWE', range=(0, 7), default=6, desc='Overcurrent protection deglitch time for the high side of the UVW phases.', enum={0: 'OFF', 1: 'T_0_25_MICROSEC', 2: 'T_0_5_MICROSEC', 3: 'T_1_MICROSEC', 4: 'T_2_MICROSEC', 5: 'T_4_MICROSEC', 6: 'T_6_MICROSEC', 7: 'T_8_MICROSEC'})
Parameter(id=268, name='OVERCURRENT_PROTECTION_Y2_LOW_SIDE_DEGLITCH', rw='RWE', range=(0, 7), default=6, desc='Overcurrent protection deglitch time for the low side of the Y2 phase.', enum={0: 'OFF', 1: 'T_0_25_MICROSEC', 2: 'T_0_5_MICROSEC', 3: 'T_1_MICROSEC', 4: 'T_2_MICROSEC', 5: 'T_4_MICROSEC', 6: 'T_6_MICROSEC', 7: 'T_8_MICROSEC'})
Parameter(id=269, name='OVERCURRENT_PROTECTION_Y2_HIGH_SIDE_DEGLITCH', rw='RWE', range=(0, 7), default=6, desc='Overcurrent protection deglitch time for the high side of the Y2 phase.', enum={0: 'OFF', 1: 'T_0_25_MICROSEC', 2: 'T_0_5_MICROSEC', 3: 'T_1_MICROSEC', 4: 'T_2_MICROSEC', 5: 'T_4_MICROSEC', 6: 'T_6_MICROSEC', 7: 'T_8_MICROSEC'})
Parameter(id=270, name='OVERCURRENT_PROTECTION_UVW_LOW_SIDE_USE_VDS', rw='RWE', range=(0, 1), default=1, desc='Use the VDS measurement for the overcurrent protection on the low side of the UVW phases.', enum={0: 'DISABLED', 1: 'ENABLED'})
Parameter(id=271, name='OVERCURRENT_PROTECTION_Y2_LOW_SIDE_USE_VDS', rw='RWE', range=(0, 1), default=1, desc='Use the VDS measurement for the overcurrent protection on the low side of the Y2 phase.', enum={0: 'DISABLED', 1: 'ENABLED'})
Parameter(id=272, name='VGS_SHORT_ON_PROTECTION_UVW_LOW_SIDE_ENABLE', rw='RWE', range=(0, 1), default=1, desc='Enable the gate-source short protection for the ON transition of the low side of the UVW phases.', enum={0: 'DISABLED', 1: 'ENABLED'})
Parameter(id=273, name='VGS_SHORT_OFF_PROTECTION_UVW_LOW_SIDE_ENABLE', rw='RWE', range=(0, 1), default=1, desc='Enable the gate-source short protection for the OFF transition of the low side of the UVW phases.', enum={0: 'DISABLED', 1: 'ENABLED'})
Parameter(id=274, name='VGS_SHORT_ON_PROTECTION_UVW_HIGH_SIDE_ENABLE', rw='RWE', range=(0, 1), default=1, desc='Enable the gate-source short protection for the ON transition of the high side of the UVW phases.', enum={0: 'DISABLED', 1: 'ENABLED'})
Parameter(id=275, name='VGS_SHORT_OFF_PROTECTION_UVW_HIGH_SIDE_ENABLE', rw='RWE', range=(0, 1), default=1, desc='Enable the gate-source short protection for the OFF transition of the high side of the UVW phases.', enum={0: 'DISABLED', 1: 'ENABLED'})
Parameter(id=276, name='VGS_SHORT_ON_PROTECTION_Y2_LOW_SIDE_ENABLE', rw='RWE', range=(0, 1), default=1, desc='Enable the gate-source short protection for the ON transition of the low side of the Y2 phase.', enum={0: 'DISABLED', 1: 'ENABLED'})
Parameter(id=277, name='VGS_SHORT_OFF_PROTECTION_Y2_LOW_SIDE_ENABLE', rw='RWE', range=(0, 1), default=1, desc='Enable the gate-source short protection for the OFF transition of the low side of the Y2 phase.', enum={0: 'DISABLED', 1: 'ENABLED'})
Parameter(id=278, name='VGS_SHORT_ON_PROTECTION_Y2_HIGH_SIDE_ENABLE', rw='RWE', range=(0, 1), default=1, desc='Enable the gate-source short protection for the ON transition of the high side of the Y2 phase.', enum={0: 'DISABLED', 1: 'ENABLED'})
Parameter(id=279, name='VGS_SHORT_OFF_PROTECTION_Y2_HIGH_SIDE_ENABLE', rw='RWE', range=(0, 1), default=1, desc='Enable the gate-source short protection for the OFF transition of the high side of the Y2 phase.', enum={0: 'DISABLED', 1: 'ENABLED'})
Parameter(id=280, name='VGS_SHORT_PROTECTION_UVW_BLANKING', rw='RWE', range=(0, 3), default=1, desc='Gate-source short protection blanking time for the low and high sides of the UVW phases.', enum={0: 'OFF', 1: 'T_0_25_MICROSEC', 2: 'T_0_5_MICROSEC', 3: 'T_1_MICROSEC'})
Parameter(id=281, name='VGS_SHORT_PROTECTION_Y2_BLANKING', rw='RWE', range=(0, 3), default=1, desc='Gate-source short protection blanking time for the low and high sides of the Y2 phase.', enum={0: 'OFF', 1: 'T_0_25_MICROSEC', 2: 'T_0_5_MICROSEC', 3: 'T_1_MICROSEC'})
Parameter(id=282, name='VGS_SHORT_PROTECTION_UVW_DEGLITCH', rw='RWE', range=(0, 7), default=1, desc='Gate-source short protection deglitch time for the low and high sides of the UVW phases.', enum={0: 'OFF', 1: 'T_0_25_MICROSEC', 2: 'T_0_5_MICROSEC', 3: 'T_1_MICROSEC', 4: 'T_2_MICROSEC', 5: 'T_4_MICROSEC', 6: 'T_6_MICROSEC', 7: 'T_8_MICROSEC'})
Parameter(id=283, name='VGS_SHORT_PROTECTION_Y2_DEGLITCH', rw='RWE', range=(0, 7), default=1, desc='Gate-source short protection deglitch time for low and high sides of the Y2 phase.', enum={0: 'OFF', 1: 'T_0_25_MICROSEC', 2: 'T_0_5_MICROSEC', 3: 'T_1_MICROSEC', 4: 'T_2_MICROSEC', 5: 'T_4_MICROSEC', 6: 'T_6_MICROSEC', 7: 'T_8_MICROSEC'})
Parameter(id=286, name='GDRV_RETRY_BEHAVIOUR', rw='RWE', range=(0, 1), default=0, desc='This value defines the state the system goes to after a fault condition on a motor phase occurs. The system switches off and discharges the gates. The motor can spin freely. The system switches off and, if possible and depending on fault, enables the LS or HS gates, braking the motor electrically.', enum={0: 'OPEN_CIRCUIT', 1: 'ELECTRICAL_BRAKING'})
Parameter(id=287, name='DRIVE_FAULT_BEHAVIOUR', rw='RWE', range=(0, 3), default=0, desc='This value defines the state the system goes to after a fault condition on a motor phase occurs and all retries failed. The system switches off and discharges the LS and HS gates, letting the motor spin freely. The system switches off and, if possible and depending on fault, enables the LS or HS gates, braking the motor electrically. The system switches off, discharges the LS and HS gates, and, if correctly configured, engages the mechanical brake. The system switches off, if possible and depending on fault, enables the LS or HS gates, and, if correctly configured, engages the mechanical brake.', enum={0: 'OPEN_CIRCUIT', 1: 'ELECTRICAL_BRAKING', 2: 'MECHANICAL_BRAKING_AND_OPEN_CIRCUIT', 3: 'MECHANICAL_AND_ELECTRICAL_BRAKING'})
Parameter(id=288, name='FAULT_HANDLER_NUMBER_OF_RETRIES', rw='RWE', range=(0, 255), default=5, desc='Maximum number of retries that are performed for every fault that is detected.')
Parameter(id=289, name='GENERAL_STATUS_FLAGS', rw='R', range=(0, 0), default=None, desc='Actual status flags.', enum={1: 'REGULATION_STOPPED', 2: 'REGULATION_TORQUE', 4: 'REGULATION_VELOCITY', 8: 'REGULATION_POSITION', 16: 'CONFIG_STORED', 32: 'CONFIG_LOADED', 64: 'CONFIG_READ_ONLY', 128: 'TMCL_SCRIPT_READ_ONLY', 256: 'BRAKE_CHOPPER_ACTIVE', 512: 'POSITION_REACHED', 1024: 'VELOCITY_REACHED', 2048: 'ADC_OFFSET_CALIBRATED', 4096: 'RAMPER_LATCHED', 8192: 'RAMPER_EVENT_STOP_SWITCH', 16384: 'RAMPER_EVENT_STOP_DEVIATION', 32768: 'RAMPER_VELOCITY_REACHED', 65536: 'RAMPER_POSITION_REACHED', 131072: 'RAMPER_SECOND_MOVE', 262144: 'IIT_1_ACTIVE', 524288: 'IIT_2_ACTIVE', 1048576: 'REFSEARCH_FINISHED', 2097152: 'Y2_USED_FOR_BRAKING', 4194304: 'FLASH_STIMULUS_AVAILABLE', 8388608: 'STEPDIR_INPUT_AVAILABLE', 16777216: 'RIGHT_REF_SWITCH_AVAILABLE', 33554432: 'HOME_REF_SWITCH_AVAILABLE', 67108864: 'LEFT_REF_SWITCH_AVAILABLE', 134217728: 'ABN2_FEEDBACK_AVAILABLE', 268435456: 'HALL_FEEDBACK_AVAILABLE', 536870912: 'ABN1_FEEDBACK_AVAILABLE', 1073741824: 'SPI_FLASH_AVAILABLE', 2147483648: 'I2C_EEPROM_AVAILABLE'})
Parameter(id=290, name='SUPPLY_VOLTAGE', rw='R', range=(0, 1000), default=0, desc='The actual supply voltage.', unit='0.1V')
Parameter(id=291, name='SUPPLY_OVERVOLTAGE_WARNING_THRESHOLD', rw='RWE', range=(0, 1000), default=480, desc='The supply overvoltage warning  threshold.', unit='0.1V')
Parameter(id=292, name='SUPPLY_UNDERVOLTAGE_WARNING_THRESHOLD', rw='RWE', range=(0, 1000), default=50, desc='The supply undervoltage warning  threshold.', unit='0.1V')
Parameter(id=293, name='EXTERNAL_TEMPERATURE', rw='R', range=(0, 65535), default=0, desc='The actual temperature at the external temperature sensor.')
Parameter(id=294, name='EXTERNAL_TEMPERATURE_SHUTDOWN_THRESHOLD', rw='RWE', range=(0, 65535), default=65535, desc='The temperature threshold at which the motor driver is shut down.')
Parameter(id=295, name='EXTERNAL_TEMPERATURE_WARNING_THRESHOLD', rw='RWE', range=(0, 65535), default=65535, desc='The temperature threshold above which the warning flag is set.')
Parameter(id=296, name='CHIP_TEMPERATURE', rw='R', range=(0, 65535), default=0, desc='The actual temperature of the chip.')
Parameter(id=297, name='CHIP_TEMPERATURE_SHUTDOWN_THRESHOLD', rw='RWE', range=(0, 65535), default=65535, desc='The temperature threshold at which the motor driver is shut down.')
Parameter(id=298, name='CHIP_TEMPERATURE_WARNING_THRESHOLD', rw='RWE', range=(0, 65535), default=65535, desc='The temperature threshold above which the warning flag is set.')
Parameter(id=299, name='GENERAL_ERROR_FLAGS', rw='R', range=(0, 0), default=None, desc='Actual error flags.', enum={1: 'CONFIG_ERROR', 2: 'TMCL_SCRIPT_ERROR', 4: 'HOMESWITCH_NOT_FOUND', 32: 'HALL_ERROR', 512: 'WATCHDOG_EVENT', 8192: 'EXT_TEMP_EXCEEDED', 16384: 'CHIP_TEMP_EXCEEDED', 65536: 'ITT_1_EXCEEDED', 131072: 'ITT_2_EXCEEDED', 262144: 'EXT_TEMP_WARNING', 524288: 'SUPPLY_OVERVOLTAGE_WARNING', 1048576: 'SUPPLY_UNDERVOLTAGE_WARNING', 2097152: 'ADC_IN_OVERVOLTAGE', 4194304: 'FAULT_RETRY_HAPPEND', 8388608: 'FAULT_RETRIES_FAILED', 16777216: 'CHIP_TEMP_WARNING', 67108864: 'HEARTBEAT_STOPPED'})
Parameter(id=300, name='GDRV_ERROR_FLAGS', rw='R', range=(0, 0), default=None, desc='Gate driver error flags.', enum={1: 'U_LOW_SIDE_OVERCURRENT', 2: 'V_LOW_SIDE_OVERCURRENT', 4: 'W_LOW_SIDE_OVERCURRENT', 8: 'Y2_LOW_SIDE_OVERCURRENT', 16: 'U_LOW_SIDE_DISCHARGE_SHORT', 32: 'V_LOW_SIDE_DISCHARGE_SHORT', 64: 'W_LOW_SIDE_DISCHARGE_SHORT', 128: 'Y2_LOW_SIDE_DISCHARGE_SHORT', 256: 'U_LOW_SIDE_CHARGE_SHORT', 512: 'V_LOW_SIDE_CHARGE_SHORT', 1024: 'W_LOW_SIDE_CHARGE_SHORT', 2048: 'Y2_LOW_SIDE_CHARGE_SHORT', 4096: 'U_BOOTSTRAP_UNDERVOLTAGE', 8192: 'V_BOOTSTRAP_UNDERVOLTAGE', 16384: 'W_BOOTSTRAP_UNDERVOLTAGE', 32768: 'Y2_BOOTSTRAP_UNDERVOLTAGE', 65536: 'U_HIGH_SIDE_OVERCURRENT', 131072: 'V_HIGH_SIDE_OVERCURRENT', 262144: 'W_HIGH_SIDE_OVERCURRENT', 524288: 'Y2_HIGH_SIDE_OVERCURRENT', 1048576: 'U_HIGH_SIDE_DISCHARGE_SHORT', 2097152: 'V_HIGH_SIDE_DISCHARGE_SHORT', 4194304: 'W_HIGH_SIDE_DISCHARGE_SHORT', 8388608: 'Y2_HIGH_SIDE_DISCHARGE_SHORT', 16777216: 'U_HIGH_SIDE_CHARGE_SHORT', 33554432: 'V_HIGH_SIDE_CHARGE_SHORT', 67108864: 'W_HIGH_SIDE_CHARGE_SHORT', 134217728: 'Y2_HIGH_SIDE_CHARGE_SHORT', 536870912: 'GDRV_UNDERVOLTAGE', 1073741824: 'GDRV_LOW_VOLTAGE', 2147483648: 'GDRV_SUPPLY_UNDERVOLTAGE'})
Parameter(id=301, name='ADC_STATUS_FLAGS', rw='R', range=(0, 0), default=None, desc='ADC channel clipped. Flags do latch, write 1 to clear.', enum={1: 'I0_CLIPPED', 2: 'I1_CLIPPED', 4: 'I2_CLIPPED', 8: 'I3_CLIPPED', 16: 'U0_CLIPPED', 32: 'U1_CLIPPED', 64: 'U2_CLIPPED', 128: 'U3_CLIPPED', 256: 'AIN0_CLIPPED', 512: 'AIN1_CLIPPED', 1024: 'AIN2_CLIPPED', 2048: 'AIN3_CLIPPED', 4096: 'VM_CLIPPED', 8192: 'TEMP_CLIPPED'})
Parameter(id=304, name='MCC_INPUTS_RAW', rw='R', range=(0, 32767), default=0, desc='Raw inputs for ABN, hall, reference switches, driver enabled, hall filtered and ABN2 or Step/Dir.')
Parameter(id=305, name='FOC_VOLTAGE_UX', rw='R', range=(-32768, 32767), default=0, desc='Interim result of the FOC for phase U (X in case of motor type is a stepper motor).')
Parameter(id=306, name='FOC_VOLTAGE_WY', rw='R', range=(-32768, 32767), default=0, desc='Interim result of the FOC for phase W (Y in case of motor type is a stepper motor).')
Parameter(id=307, name='FOC_VOLTAGE_V', rw='R', range=(-32768, 32767), default=0, desc='Interim result of the FOC for phase V (BLDC motor only).')
Parameter(id=308, name='FIELDWEAKENING_I', rw='RWE', range=(0, 32767), default=0, desc='I parameter for field weakening controller.')
Parameter(id=310, name='FIELDWEAKENING_VOLTAGE_THRESHOLD', rw='RWE', range=(0, 32767), default=32767, desc='Maximum motor voltage allowed for field weakening.')
Parameter(id=311, name='FOC_CURRENT_UX', rw='R', range=(-32768, 32767), default=0, desc='Interim measurement of the FOC for phase UX.')
Parameter(id=312, name='FOC_CURRENT_V', rw='R', range=(-32768, 32767), default=0, desc='Interim measurement of the FOC for phase V.')
Parameter(id=313, name='FOC_CURRENT_WY', rw='R', range=(-32768, 32767), default=0, desc='Interim measurement of the FOC for phase WY.')
Parameter(id=314, name='FOC_VOLTAGE_UQ', rw='R', range=(-32768, 32767), default=0, desc='Interim measurement of the FOC for Uq.')
Parameter(id=315, name='FOC_CURRENT_IQ', rw='R', range=(-32768, 32767), default=0, desc='Interim measurement of the FOC for Iq.')
Parameter(id=318, name='TARGET_TORQUE_BIQUAD_FILTER_ENABLE', rw='RWE', range=(0, 1), default=0, desc='Enable the target torque biquad filter.', enum={0: 'DISABLED', 1: 'ENABLED'})
Parameter(id=319, name='TARGET_TORQUE_BIQUAD_FILTER_ACOEFF_1', rw='RWE', range=(-2147483648, 2147483647), default=0, desc='Target torque biquad filter aCoeff_1.')
Parameter(id=320, name='TARGET_TORQUE_BIQUAD_FILTER_ACOEFF_2', rw='RWE', range=(-2147483648, 2147483647), default=0, desc='Target torque biquad filter aCoeff_2.')
Parameter(id=321, name='TARGET_TORQUE_BIQUAD_FILTER_BCOEFF_0', rw='RWE', range=(-2147483648, 2147483647), default=1048576, desc='Target torque biquad filter bCoeff_0.')
Parameter(id=322, name='TARGET_TORQUE_BIQUAD_FILTER_BCOEFF_1', rw='RWE', range=(-2147483648, 2147483647), default=0, desc='Target torque biquad filter bCoeff_1.')
Parameter(id=323, name='TARGET_TORQUE_BIQUAD_FILTER_BCOEFF_2', rw='RWE', range=(-2147483648, 2147483647), default=0, desc='Target torque biquad filter bCoeff_2.')
Parameter(id=324, name='ACTUAL_VELOCITY_BIQUAD_FILTER_ENABLE', rw='RWE', range=(0, 1), default=1, desc='Enable the actual velocity biquad filter.', enum={0: 'DISABLED', 1: 'ENABLED'})
Parameter(id=325, name='ACTUAL_VELOCITY_BIQUAD_FILTER_ACOEFF_1', rw='RWE', range=(-2147483648, 2147483647), default=1849195, desc='Actual velocity biquad filter aCoeff_1.')
Parameter(id=326, name='ACTUAL_VELOCITY_BIQUAD_FILTER_ACOEFF_2', rw='RWE', range=(-2147483648, 2147483647), default=15961938, desc='Actual velocity biquad filter aCoeff_2.')
Parameter(id=327, name='ACTUAL_VELOCITY_BIQUAD_FILTER_BCOEFF_0', rw='RWE', range=(-2147483648, 2147483647), default=3665, desc='Actual velocity biquad filter bCoeff_0.')
Parameter(id=328, name='ACTUAL_VELOCITY_BIQUAD_FILTER_BCOEFF_1', rw='RWE', range=(-2147483648, 2147483647), default=7329, desc='Actual velocity biquad filter bCoeff_1.')
Parameter(id=329, name='ACTUAL_VELOCITY_BIQUAD_FILTER_BCOEFF_2', rw='RWE', range=(-2147483648, 2147483647), default=3665, desc='Actual velocity biquad filter bCoeff_2.')
Parameter(id=330, name='TORQUE_FLUX_COMBINED_TARGET_VALUES', rw='R', range=(0, 4294967295), default=0, desc='Raw (unscaled) torque and flux target values combined into one 32 bit value. This is only used for simplified compact measurement during tuning operations.')
Parameter(id=331, name='TORQUE_FLUX_COMBINED_ACTUAL_VALUES', rw='R', range=(0, 4294967295), default=0, desc='Raw (unscaled) torque and flux actual values combined into one 32 bit value. This is only used for simplified compact measurement during tuning operations.')
Parameter(id=332, name='VOLTAGE_D_Q_COMBINED_ACTUAL_VALUES', rw='R', range=(0, 4294967295), default=0, desc='Raw (unscaled) voltage actual values combined into one 32 bit value. This is only used for simplified compact measurement during tuning operations.')
Parameter(id=333, name='INTEGRATED_ACTUAL_TORQUE_VALUE', rw='R', range=(0, 4294967295), default=0, desc='Periodically summed up actual torque value. This is only used for simplified measurement with low measurement frequency during tuning operations.')
Parameter(id=334, name='INTEGRATED_ACTUAL_VELOCITY_VALUE', rw='R', range=(0, 4294967295), default=0, desc='Periodically summed up actual velocity value. This is only used for simplified measurement with low measurement frequency during tuning operations.')

print(part1)

print("\n\n\n")

print(part2)
