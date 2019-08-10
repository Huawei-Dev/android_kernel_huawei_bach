/******************************************************************************
 * @file cm_cp_test.c
 *
 * cm_cp_test.c
 *
 * @version 0.0.1
 * @authors btok
 *
 *****************************************************************************//*
 * Copyright (2014), Cypress Semiconductor Corporation. All rights reserved.
 *
 * This software, associated documentation and materials ("Software") is owned
 * by Cypress Semiconductor Corporation ("Cypress") and is protected by and
 * subject to worldwide patent protection (United States and foreign), United
 * States copyright laws and international treaty provisions. Therefore, unless
 * otherwise specified in a separate license agreement between you and Cypress,
 * this Software must be treated like any other copyrighted material.
 * Reproduction, modification, translation, compilation, or representation of
 * this Software in any other form (e.g., paper, magnetic, optical, silicon) is
 * prohibited without Cypress's express written permission.
 *
 * Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 * Cypress reserves the right to make changes to the Software without notice.
 * Cypress does not assume any liability arising out of the application or use
 * of Software or any product or circuit described in the Software. Cypress does
 * not authorize its products for use as critical components in any products
 * where a malfunction or failure may reasonably be expected to result in
 * significant injury or death ("High Risk Product"). By including Cypress's
 * product in a High Risk Product, the manufacturer of such system or
 * application assumes all risk of such use and in doing so indemnifies Cypress
 * against all liability.
 *
 * Use of this Software may be limited by and subject to the applicable Cypress
 * software license agreement.
 *****************************************************************************/

#include <linux/device.h>
#include "pip.h"
#include "lookup.h"
#include "parameter.h"
#include "configuration.h"
#include "result.h"
#include "cm_cp_test.h"
#include "../cyttsp5_regs.h"
#include "../cyttsp5_core.h"

#define IDAC_LSB_DEFAULT	80
#define MTX_SUM_DEFAULT		1
#define CLK_DEFAULT		48
#define VREF_DEFAULT     2

#define CONVERSION_CONST	1000
#define MAX_READ_LENGTH     100

//#define ARRAY_SIZE(x)		sizeof(x)/sizeof(x[0])

#define ABS(x)			(((x) < 0) ? -(x) : (x))

//#define LOG_IS_NOT_SHOW_RAW_DATA

static struct rx_attenuator_lookup rx_attenuator_lookup_table[] =
    {RX_ATTENUATOR_LOOKUP_TABLE};

static int8_t mtx_sum_lookup_table[] =
    {MTX_SUM_LOOKUP_TABLE};

static int selfcap_signal_swing_lookup_table[] =
    {SELFCAP_SIGNAL_SWING_LOOKUP_TABLE};

uint8_t g_family_type = 0;

static int rx_attenuator_lookup(uint8_t index, int *value)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(rx_attenuator_lookup_table); i++)
        if (index == rx_attenuator_lookup_table[i].index) {
            *value = rx_attenuator_lookup_table[i].value;
            return 0;
        }

    return -EINVAL;
}

static int mtx_sum_lookup(uint8_t mtx_order, int *mtx_sum)
{
    if (IS_MTX_ORDER_VALID(mtx_order)) {
        *mtx_sum = (int)mtx_sum_lookup_table[
                GET_MTX_SUM_OFFSET(mtx_order)];
        return 0;
    }

    return -EINVAL;
};

static int selfcap_signal_swing_lookup(uint8_t ref_scale, uint8_t rxdac,
		int *vref_low, int *vref_mid, int *vref_high)
{
	if (IS_REF_SCALE_VALID(ref_scale) && IS_RXDAC_VALID(rxdac)) {
        *vref_low = selfcap_signal_swing_lookup_table[
                GET_VREF_LOW_OFFSET(ref_scale, rxdac)];
        *vref_mid = selfcap_signal_swing_lookup_table[
                GET_VREF_MID_OFFSET(ref_scale, rxdac)];

        *vref_high = selfcap_signal_swing_lookup_table[
                GET_VREF_HIGH_OFFSET(ref_scale, rxdac)];

        return 0;
    }

    return -EINVAL;
}

static int get_configuration_parameter(enum parameter_id id,
        enum parameter_type *parameter_type,
        union parameter_value *parameter_value)
{
    uint16_t parameter_address;
    uint16_t parameter_size;
    uint32_t parameter_mask;
    const union parameter_value *enumerated_value;
    uint16_t row_number;
    uint16_t row_offset; 
    uint16_t read_length;
    uint8_t data[ROW_SIZE];
    uint32_t value;
    int ret;
    int i;

    tp_log_debug("%s,id:%d\n", __func__, id);
    ret = parameter_get_info(id, &parameter_address, &parameter_size,
    		&parameter_mask, parameter_type);
    if (ret) {
        tp_log_err("%s:Unable to get parameter info!\n",__func__);
        goto exit;
    }

    row_number = parameter_address / ROW_SIZE;
    row_offset = parameter_address % ROW_SIZE;

    ret = pip_read_data_block(row_number, row_offset + parameter_size,
            CONFIG_BLOCK_ID, &read_length, data);
    if (ret) {
        tp_log_err("%s:Unable to read data block!\n",__func__);
        goto exit;
    }
#ifdef LOG_IS_NOT_SHOW_RAW_DATA
    tp_log_info("%s: data[0..%d]:\n", __func__, read_length - 1);
    for (i = 0; i < read_length; i++)
    {
        tp_log_info("%02X ", data[i]);
    }
    tp_log_info("\n");
#endif

    value = data[row_offset];
    if (parameter_size == 2 || parameter_size == 4)
        value += data[row_offset + 1] << 8;
    if (parameter_size == 4) {
        value += data[row_offset + 2] << 16;
        value += data[row_offset + 3] << 24;
    }

    if (parameter_mask) {
        value &= parameter_mask;
        while ((parameter_mask & 0x01) == 0) {
            value >>= 1;
            parameter_mask >>= 1;
        }
    }

    ret = parameter_get_enumerated_value(id, (int)value,
            &enumerated_value);
    if (ret) {
        tp_log_err("%s:Unable to get parameter enumerated value!\n",__func__);
        goto exit;
    }

    if (enumerated_value)
        memcpy(parameter_value, enumerated_value,
            sizeof(union parameter_value));
    else if (!enumerated_value && *parameter_type == INTEGER)
        parameter_value->integer = (int32_t)value;
    else {
        tp_log_err("%s:Unable to get parameter value!\n",__func__);
        ret = -EINVAL;
    }
    i=0;
exit:
    return ret;
}

/*
 * Wrapper function for pip_retrieve_scan to call it multiple times
 * to gather data if necessary
 */
static int retrieve_panel_scan(uint16_t read_offset, uint16_t read_length,
        uint8_t data_id, uint16_t *actual_read_length,
        uint8_t *data_format, uint8_t *data)
{
    uint16_t actual_read_len;
	uint16_t myread = 0;
    uint16_t total_read_len = 0;
    int ret = 0;

    while (read_length > 0) {
        if (read_length > MAX_READ_LENGTH){
            myread = MAX_READ_LENGTH;
        } else {
            myread = read_length;
        }
        ret = pip_retrieve_panel_scan(read_offset, myread,
                data_id, &actual_read_len, data_format, data);
        if (ret) {
            tp_log_err("%s:Unable to retrieve panel scan!\n",__func__);
            goto exit;
        }
        //mdelay(5000);
        if (actual_read_len == 0)
            break;
        tp_log_debug("%s: read_offset: %d\n", __func__, read_offset);
        tp_log_debug("%s: actual_read_len: %d\n", __func__,
                actual_read_len);
        tp_log_debug("%s: read_length: %d\n", __func__, read_length);
        tp_log_debug("%s: data_format: %02X\n", __func__,
                *data_format);

        read_offset += actual_read_len;
        total_read_len += actual_read_len;

        data += actual_read_len * GET_ELEMENT_SIZE(*data_format);

        read_length -= actual_read_len;
    }

    *actual_read_length = total_read_len;

exit:
    return ret;
}

static int32_t get_element(uint8_t element_size, uint8_t sign, uint8_t *data)
{
    if (element_size == 1) {
        if (sign == SIGN_UNSIGNED){
            return *data;
        }else{
            return (int8_t)*data;
        }
    } else if (element_size == 2) {
        if (sign == SIGN_UNSIGNED){
            return get_unaligned_le16(data);
        }else{
            return (int16_t)get_unaligned_le16(data);
        }
    } else if (element_size == 4) {
        if (sign == SIGN_UNSIGNED){
            return get_unaligned_le32(data);
        }else{
            return (int)get_unaligned_le32(data);
        }
    }

    return 0;
}

static int retrieve_panel_raw_data(uint8_t data_id, uint16_t read_offset,
        uint16_t read_length, uint8_t *data_format, int32_t *raw_data)
{
    uint16_t actual_read_length;
    uint8_t element_size;
    uint8_t sign;
    uint8_t *data;
    int ret;
    int i;
    
    data = kzalloc(read_length * sizeof(uint32_t), GFP_KERNEL);
    if (!data)
        return -ENOMEM;
    
    ret = retrieve_panel_scan(read_offset, read_length, data_id,
            &actual_read_length, data_format, data);
    if (ret) {
        tp_log_err("%s:Unable to retrieve panel raw data!\n",__func__);
        goto free;
    }

    if (read_length != actual_read_length) {
        ret = -EINVAL;
        goto free;
    }

    element_size = GET_ELEMENT_SIZE(*data_format);
    if (element_size != 1 && element_size != 2 && element_size != 4) {
        ret = -EINVAL;
        goto free;
    }

    sign = GET_SIGN(*data_format);

    for (i = 0; i < read_length; i++)
        raw_data[i] = get_element(element_size, sign,
                    &data[i << (element_size / 2)]);

free:
    kfree(data);

    return ret;
}


static int calculate_cm_gen6(int raw_data, int tx_period_mutual,
		int balancing_target_mutual, int scaling_factor_mutual,
		int idac_lsb, int idac_mutual, int rx_atten_mutual,
		int gidac_mult, int clk, int vtx, int mtx_sum)
{
	long long i_bias = gidac_mult * idac_lsb * idac_mutual;
	long long adc = (((long long)tx_period_mutual * balancing_target_mutual*scaling_factor_mutual *idac_mutual * rx_atten_mutual) -
						50000 * raw_data) / (scaling_factor_mutual *idac_mutual);

	return i_bias * adc / (clk * vtx * mtx_sum*1000);  //when bypass mode,vtx*1000
}


static int get_cm_uniformity_test_results_gen6(int vdda, uint16_t tx_num,
        uint16_t rx_num,  uint16_t button_num, bool skip_cm_button,
        int32_t **sensor_raw_data,
        int **cm_sensor_data, 
        int *cm_sensor_average)
{
    union parameter_value parameter_value;
    enum parameter_type parameter_type;
    uint16_t read_length;
    uint16_t sensor_element_num = 0;
    uint8_t data_format = 0;
    int8_t idac_mutual;
    int rx_atten_mutual;
    uint32_t tx_period_mutual;
    char* vdda_mode;
    uint32_t scaling_factor_mutual;
    uint32_t balancing_target_mutual;
    uint32_t gidac_mult;
    int vtx;
    int idac_lsb = IDAC_LSB_DEFAULT;
    int mtx_sum = MTX_SUM_DEFAULT;
    int clk = CLK_DEFAULT; 
    uint8_t data[IDAC_AND_RX_ATTENUATOR_CALIBRATION_DATA_LENGTH];
    int ret;
    int i;
    int j;

    sensor_element_num = rx_num * tx_num;
    tp_log_info("%s, get_cm_uniformity_test_results_gen6 called\n", __func__);
    *sensor_raw_data = kzalloc(sensor_element_num * sizeof(int32_t), GFP_KERNEL);
    *cm_sensor_data = kzalloc(sensor_element_num * sizeof(int),GFP_KERNEL);

    if (!*sensor_raw_data || !*cm_sensor_data) {
        ret = -ENOMEM;
        goto exit;
    }

    tp_log_info("%s: Set FORCE_SINGLE_TX to 1\n", __func__);

    /* Step 1: Set force single TX */
    ret = pip_set_parameter(FORCE_SINGLE_TX, 1, 0x01);
    if (ret) {
        tp_log_info("%s:Unable to set FORCE_SINGLE_TX parameter!\n",__func__);
        goto exit;
    }

    /*workaround for CDT193384*/
    ret = pip_resume_scanning();
    if (ret) {
        tp_log_info("%s:Unable to resume panel scan!\n",__func__);
        goto restore_multi_tx;
    }

    ret = pip_suspend_scanning();
    if (ret) {
        tp_log_info("%s:Unable to suspend panel scan!\n",__func__);
        goto restore_multi_tx;
    }
    /*end workaround for CDT193384*/

    tp_log_info("%s: Perform calibrate IDACs\n", __func__);

    /* Step 2: Perform calibration */
    ret = pip_calibrate_idacs(0);
    if (ret) {
        tp_log_info("%s:Unable to calibrate IDACs!\n",__func__);
        goto restore_multi_tx;
    }
    if(button_num > 0)
    {
        ret = pip_calibrate_idacs(1);
        if (ret) {
            tp_log_info("%s Unable to calibrate button IDACs!\n",__func__);
            goto restore_multi_tx;
        }
    }

    tp_log_info("%s: Get Mutual IDAC and RX Attenuator values\n",__func__);

    /* Step 3: Get Mutual IDAC and RX Attenuator values */
    ret = pip_retrieve_data_structure(0,
            IDAC_AND_RX_ATTENUATOR_CALIBRATION_DATA_LENGTH,
            IDAC_AND_RX_ATTENUATOR_CALIBRATION_DATA_ID,
            &read_length, &data_format, data);
    if (ret) {
        tp_log_info("%s:Unable to retrieve data structure!\n",__func__);
        goto restore_multi_tx;
    }

    ret = rx_attenuator_lookup(data[RX_ATTENUATOR_MUTUAL_INDEX],
            &rx_atten_mutual);
    if (ret) {
        tp_log_info("%s:Invalid RX Attenuator Index!\n",__func__);
        goto restore_multi_tx;
    }

    idac_mutual = (int8_t)data[IDAC_MUTUAL_INDEX];

    tp_log_info("%s: idac_mutual: %d\n", __func__, idac_mutual);
    tp_log_info("%s: rx_atten_mutual: %d\n", __func__, rx_atten_mutual);

    /* step 3 Get CDC:VDDA_MODE */
    ret = get_configuration_parameter(VDDA_MODE, &parameter_type,
                &parameter_value);
    if (ret || parameter_type != STRING) {
        tp_log_err("%s:Unable to get vdda_mode!\n",__func__);
        goto restore_multi_tx;
    }
    vdda_mode = parameter_value.string;
    tp_log_info("%s: vdda_mode: %s\n", __func__, vdda_mode);

    if (!strcmp(vdda_mode, VDDA_MODE_BYPASS)) {
        if (vdda != 0)
            vtx = vdda;
        else {
            tp_log_err("%s:VDDA cannot be zero when VDDA_MODE is bypass!\n",__func__);
            ret = -EINVAL;
            goto restore_multi_tx;
        }
    } else if (!strcmp(vdda_mode, VDDA_MODE_PUMP)) {
        /* Get CDC:TX_PUMP_VOLTAGE */
        ret = get_configuration_parameter(TX_PUMP_VOLTAGE,
                &parameter_type, &parameter_value);
        if (ret || parameter_type != FLOAT) {
            tp_log_err("%s:Unable to get tx_pump_voltage!\n", __func__);
            goto restore_multi_tx;
        }
        vtx = parameter_value.flt;
    } else {
        tp_log_err("%s:Invalid VDDA_MODE: %s!\n",__func__, vdda_mode);
        ret = -EINVAL;
        goto restore_multi_tx;
    }

    tp_log_info("%s: vtx: %d\n", __func__, vtx);

    /* Get CDC:TX_PERIOD_MUTUAL */
    ret = get_configuration_parameter(TX_PERIOD_MUTUAL, &parameter_type,
                &parameter_value);
    if (ret || parameter_type != INTEGER) {
        tp_log_err("%s:Unable to get tx_period_mutual!\n",__func__);
        goto restore_multi_tx;
    }

    tx_period_mutual = parameter_value.integer;
    tp_log_info("%s: tx_period_mutual: %d\n", __func__, tx_period_mutual);

    /* Get CDC:SCALING_FACTOR_MUTUAL */
    ret = get_configuration_parameter(SCALING_FACTOR_MUTUAL,
                &parameter_type, &parameter_value);
    if (ret || parameter_type != INTEGER) {
        tp_log_err("%s:Unable to get scaling_factor_mutual!\n", __func__);
        goto restore_multi_tx;
    }

    scaling_factor_mutual = parameter_value.integer;
    tp_log_info("%s: scaling_factor_mutual: %d\n", __func__,
            scaling_factor_mutual);

    /* Get Calibration:BALANCING_TARGET_MUTUAL */
    ret = get_configuration_parameter(BALANCING_TARGET_MUTUAL,
                &parameter_type, &parameter_value);
    if (ret || parameter_type != INTEGER) {
        tp_log_info("%s:Unable to get balancing_target_mutual!\n",__func__);
        goto restore_multi_tx;
    }

    balancing_target_mutual = parameter_value.integer;
    tp_log_info("%s: balancing_target_mutual: %d\n", __func__,
            balancing_target_mutual);

    /* Get CDC:INFRA_CTRL:GIDAC_MULT */
    ret = get_configuration_parameter(GIDAC_MULT,
                &parameter_type, &parameter_value);
    if (ret || parameter_type != INTEGER) {
        tp_log_err("%s:Unable to get gidac_mult!\n",__func__);
        goto restore_multi_tx;
    }

    gidac_mult = parameter_value.integer;

    tp_log_info("%s: gidac_mult:%d\n", __func__, gidac_mult);

    /* Step 4a: Execute Panel Scan */
    ret = pip_execute_panel_scan();
    if (ret) {
        tp_log_err("%s:Unable to execute panel scan!\n",__func__);
        goto restore_multi_tx;
    }

    /* Step 4b: Retrieve Panel Scan raw data */
    ret = retrieve_panel_raw_data(MUTUAL_CAP_RAW_DATA_ID, 0,
            sensor_element_num, &data_format, *sensor_raw_data);
    if (ret) {
        tp_log_err("%s:Unable to retrieve panel raw data!\n",__func__);
        goto restore_multi_tx;
    }

#ifdef LOG_IS_NOT_SHOW_RAW_DATA
    tp_log_info("%s: sensor_raw_data[0..%d]:\n", __func__,
            sensor_element_num - 1);
    for (i = 0; i < tx_num; i++) {
        for (j = 0; j < rx_num; j++)
        {
            tp_log_info("%5d ",
                (*sensor_raw_data)[i * rx_num + j]);
        }
        tp_log_info("\n");
    }
#endif

    /* Step 5 and 6: Calculate Cm_sensor and Cm_sensor_ave */
    *cm_sensor_average = 0;
    for (i = 0; i < sensor_element_num; i++) {
        (*cm_sensor_data)[i] = calculate_cm_gen6((*sensor_raw_data)[i],
            tx_period_mutual, balancing_target_mutual,
            scaling_factor_mutual, idac_lsb, idac_mutual,
            rx_atten_mutual, gidac_mult, clk, vtx, mtx_sum);
        *cm_sensor_average += (*cm_sensor_data)[i];
    }
    *cm_sensor_average /= sensor_element_num;

#ifdef LOG_IS_NOT_SHOW_RAW_DATA
    tp_log_info("%s: cm_sensor_data[0..%d]:\n", __func__,
        sensor_element_num - 1);
    for (i = 0; i < tx_num; i++) {
        for (j = 0; j < rx_num; j++)
        {
            tp_log_info("%d ",
                (*cm_sensor_data)[i * rx_num + j]);
        }
        tp_log_info("\n");
    }
#endif

    tp_log_info("%s: cm_sensor_average: %d\n", __func__,
            *cm_sensor_average);

restore_multi_tx:
	tp_log_info("%s: Set FORCE_SINGLE_TX to 0\n", __func__);

	/* Step 13: Set force single TX */
	ret = pip_set_parameter(FORCE_SINGLE_TX, 1, 0x00);
	if (ret) {
		tp_log_err("%s:Unable to set FORCE_SINGLE_TX parameter!\n",__func__);
		goto exit;
	}

	tp_log_info("%s: Perform calibrate IDACs\n", __func__);

	/* Step 14: Perform calibration */
	ret = pip_calibrate_idacs(0);
	if (ret) {
		tp_log_err("%s:Unable to calibrate mutual IDACs!\n", __func__);
		goto exit;
	}
	if(button_num > 0)
	{
		ret = pip_calibrate_idacs(1);
		if (ret) {
			tp_log_err("%s:Unable to calibrate button IDACs!\n", __func__);
			goto exit;
		}
	}

exit:
	if (ret) {
		kfree(*cm_sensor_data);
		kfree(*sensor_raw_data);
		*cm_sensor_data = NULL;
		*sensor_raw_data = NULL;
	}
	j=0;
	return ret;
}

static int calculate_cm_calibration_gen6(int tx_period_mutual,
		int balancing_target_mutual, int idac_lsb,
		int idac_mutual, int rx_atten_mutual, int gidac_mult,
		int clk, int vtx, int mtx_sum)
{
	int i_bias = gidac_mult * idac_lsb * idac_mutual;
	int t_cal = tx_period_mutual * balancing_target_mutual /clk;

	return ((long long)i_bias * t_cal * rx_atten_mutual )/ (vtx * mtx_sum*1000); //when bypass mode,vtx*1000
}

static int get_cm_calibration_check_test_results_gen6(int vdda,
		bool skip_cm_button,int *cm_sensor_calibration)
{
	union parameter_value parameter_value;
	enum parameter_type parameter_type;
	uint16_t read_length;
	uint8_t data_format;
	int8_t idac_mutual;
	int rx_atten_mutual;
	int mtx_sum;
	char *vdda_mode;
	uint32_t tx_period_mutual;
	uint32_t mtx_order;
	uint32_t balancing_target_mutual;
	uint32_t gidac_mult;
	int vtx;
	int idac_lsb = IDAC_LSB_DEFAULT;
	int clk = CLK_DEFAULT; 
	uint8_t data[IDAC_AND_RX_ATTENUATOR_CALIBRATION_DATA_LENGTH];
	int ret;

	tp_log_info("%s: Get Mutual IDAC and RX Attenuator values\n",
			__func__);

	/* Step 1: Get Mutual IDAC and RX Attenuator values */
	ret = pip_retrieve_data_structure(0,
			IDAC_AND_RX_ATTENUATOR_CALIBRATION_DATA_LENGTH,
			IDAC_AND_RX_ATTENUATOR_CALIBRATION_DATA_ID,
			&read_length, &data_format, data);
	if (ret) {
		tp_log_err("%s:Unable to retrieve data structure!\n",__func__);
		goto exit;
	}

	ret = rx_attenuator_lookup(data[RX_ATTENUATOR_MUTUAL_INDEX],
			&rx_atten_mutual);
	if (ret) {
		tp_log_err("%s:Invalid RX Attenuator Index!\n",__func__);
		goto exit;
	}

	idac_mutual = (int8_t)data[IDAC_MUTUAL_INDEX];

	tp_log_info("%s: idac_mutual: %d\n", __func__, idac_mutual);
	tp_log_info("%s: rx_atten_mutual: %d\n", __func__, rx_atten_mutual);

	/* Get CDC:VDDA_MODE */
	ret = get_configuration_parameter(VDDA_MODE, &parameter_type,
				&parameter_value);
	if (ret || parameter_type != STRING) {
		tp_log_err("%s:Unable to get vdda_mode!\n",__func__);
		goto exit;
	}

	vdda_mode = parameter_value.string;
	tp_log_info("%s: vdda_mode: %s\n", __func__, vdda_mode);

	if (!strcmp(vdda_mode, VDDA_MODE_BYPASS)) {
		if (vdda != 0)
			vtx = vdda;
		else {
			tp_log_err("%s:VDDA cannot be zero when VDDA_MODE is bypass!\n",__func__);
			ret = -EINVAL;
			goto exit;
		}
	} else if (!strcmp(vdda_mode, VDDA_MODE_PUMP)) {
		/* Get CDC:TX_PUMP_VOLTAGE */
		ret = get_configuration_parameter(TX_PUMP_VOLTAGE,
				&parameter_type, &parameter_value);
		if (ret || parameter_type != FLOAT) {
			tp_log_err("%s:Unable to get tx_pump_voltage!\n",__func__);
			goto exit;
		}
		vtx = parameter_value.flt;
	} else {
		tp_log_err("%s:Invalid VDDA_MODE: %s!\n",__func__, vdda_mode);
		ret = -EINVAL;
		goto exit;
	}

	tp_log_info("%s: vtx: %d\n", __func__, vtx);

	/* Get CDC:TX_PERIOD_MUTUAL */
	ret = get_configuration_parameter(TX_PERIOD_MUTUAL, &parameter_type,
				&parameter_value);
	if (ret || parameter_type != INTEGER) {
		tp_log_err("%s:Unable to get tx_period_mutual!\n",__func__);
		goto exit;
	}

	tx_period_mutual = parameter_value.integer;
	tp_log_info("%s: tx_period_mutual: %d\n", __func__, tx_period_mutual);

	/* Get CDC:MTX_ORDER */
	ret = get_configuration_parameter(MTX_ORDER, &parameter_type,
				&parameter_value);
	if (ret || parameter_type != INTEGER) {
		tp_log_err("%s:Unable to get mtx_order!\n",__func__);
		goto exit;
	}

	mtx_order = parameter_value.integer;
	tp_log_info("%s: mtx_order: %d\n", __func__,  mtx_order);

	ret = mtx_sum_lookup(mtx_order, &mtx_sum);
	if (ret) {
		tp_log_err("%s:Invalid MTX Order Index!\n",__func__);
		goto exit;
	}

	tp_log_info("%s: mtx_sum: %d\n", __func__, mtx_sum);

	/* Get Calibration:BALANCING_TARGET_MUTUAL */
	ret = get_configuration_parameter(BALANCING_TARGET_MUTUAL,
				&parameter_type, &parameter_value);
	if (ret || parameter_type != INTEGER) {
		tp_log_err("%s:Unable to get balancing_target_mutual!\n",__func__);
		goto exit;
	}

	balancing_target_mutual = parameter_value.integer;
	tp_log_info("%s: balancing_target_mutual: %d\n", __func__,
			balancing_target_mutual);

	/* Get CDC:INFRA_CTRL:GIDAC_MULT */
	ret = get_configuration_parameter(GIDAC_MULT,
				&parameter_type, &parameter_value);
	if (ret || parameter_type != INTEGER) {
		tp_log_err("%s:Unable to get gidac_mult!\n",__func__);
		goto exit;
	}

	gidac_mult = parameter_value.integer;
	tp_log_info("%s: gidac_mult: %d\n", __func__, gidac_mult);

	*cm_sensor_calibration = calculate_cm_calibration_gen6(tx_period_mutual,
			balancing_target_mutual, idac_lsb, idac_mutual,
			rx_atten_mutual, gidac_mult, clk, vtx, mtx_sum);

	tp_log_info("%s: cm_sensor_calibration: %d\n", __func__,
			*cm_sensor_calibration);

exit:

	return ret;
}

static int validate_cm_test_results_gen6(struct configuration *configuration,
		struct result *result, uint32_t tx_num, uint32_t rx_num,
		uint32_t button_num, bool skip_cm_button,
		int *cm_sensor_data, 
		int cm_sensor_calibration, 
		int cm_sensor_average, 
		int cm_sensor_delta, 
		int **cm_sensor_column_delta, int **cm_sensor_row_delta,
		bool *pass)
{
	uint32_t sensor_num = tx_num * rx_num;
	uint32_t configuration_sensor_num;
	uint32_t configuration_button_num;
	int ret = 0;
	int i, j;

	*cm_sensor_column_delta = kzalloc(rx_num * tx_num *sizeof(float), GFP_KERNEL);
	*cm_sensor_row_delta = kzalloc(tx_num * rx_num  * sizeof(float), GFP_KERNEL);
	if (!*cm_sensor_column_delta || !*cm_sensor_row_delta) {
		ret = -ENOMEM;
		goto exit;
	}

	configuration_sensor_num =
			configuration->cm_min_max_table_sensor_size / 2;
	configuration_button_num =
			configuration->cm_min_max_table_button_size / 2;

	if (configuration_sensor_num != sensor_num) {
		tp_log_err("%s: Configuration and Device number of sensors mismatch! (Configuration:%d, Device:%d)\n",
				__func__, configuration_sensor_num,
				sensor_num);
		ret = -EINVAL;
		goto exit;
	}

	if (!skip_cm_button && (button_num != configuration_button_num)) {
		tp_log_err("%s: Configuration and Device number of buttons mismatch! (Configuration:%d, Device:%d)\n",
				__func__, configuration_button_num,
				button_num);
		ret = -EINVAL;
		goto exit;
	}

	/* Check each sensor Cm data for min/max values */
	result->cm_sensor_validation_pass = true;
	for (i = 0; i < sensor_num; i++) {
		int row=i % rx_num;
		int col=i / rx_num;
		int cm_sensor_min =
			configuration->cm_min_max_table_sensor[(row*tx_num+col)*2];
		int cm_sensor_max =
			configuration->cm_min_max_table_sensor[(row*tx_num+col)*2 + 1];
		if ((cm_sensor_data[i] < cm_sensor_min)
				|| (cm_sensor_data[i] > cm_sensor_max)) {
			tp_log_err("%s: Sensor[%d,%d]:%d (%d,%d)\n",
					"Cm sensor min/max test",
					row,col, 
					(int)cm_sensor_data[i],
					cm_sensor_min, cm_sensor_max);
			result->cm_sensor_validation_pass = false;
		}
	}

	/* Check each row Cm data with neighbor for difference */
	result->cm_sensor_row_delta_pass = true;
	for (i = 0; i < tx_num; i++) {
		for (j = 1; j < rx_num; j++) {
			int cm_sensor_row_diff =
				ABS((int)cm_sensor_data[i * rx_num + j] -
					(int)cm_sensor_data[i * rx_num + j - 1]);
			(*cm_sensor_row_delta)[i * rx_num + j - 1] =
					cm_sensor_row_diff;
			if (cm_sensor_row_diff
					> configuration->cm_range_limit_row) {
				tp_log_err("%s: Sensor[%d,%d]:%d (%d)\n",
					"Cm sensor row range limit test",
					j, i,
					cm_sensor_row_diff,
					configuration->cm_range_limit_row);
				result->cm_sensor_row_delta_pass = false;
			}
		}
	}

	/* Check each column Cm data with neighbor for difference */
	result->cm_sensor_col_delta_pass = true;
	for (i = 1; i < tx_num; i++) {
		for (j = 0; j < rx_num; j++) {
			int cm_sensor_col_diff =
				ABS((int)cm_sensor_data[i * rx_num + j] -
					(int)cm_sensor_data[(i - 1) * rx_num + j]);
			(*cm_sensor_column_delta)[(i - 1) * rx_num + j] =
					cm_sensor_col_diff;
			if (cm_sensor_col_diff
					> configuration->cm_range_limit_col) {
				tp_log_err("%s: Sensor[%d,%d]:%d (%d)\n",
					"Cm sensor column range limit test",
					j, i,
					cm_sensor_col_diff,
					configuration->cm_range_limit_col);
				result->cm_sensor_col_delta_pass = false;
			}
		}
	}

	/* Check sensor calculated Cm for min/max values */
	result->cm_sensor_calibration_pass = true;
	if (cm_sensor_calibration < configuration->cm_min_limit_cal
			|| cm_sensor_calibration >
				configuration->cm_max_limit_cal) {
		tp_log_err("%s: Cm_cal:%d (%d,%d)\n",
				"Cm sensor Cm_cal min/max test",
				cm_sensor_calibration,
				configuration->cm_min_limit_cal,
				configuration->cm_max_limit_cal);
		result->cm_sensor_calibration_pass = false;
	}

	/* Check sensor Cm delta for range limit */
	result->cm_sensor_delta_pass = true;
	if (cm_sensor_delta > configuration->cm_max_delta_sensor_percent) {
		tp_log_err("%s: Cm_sensor_delta:%d (%d)\n",
 				"Cm sensor delta range limit test",
				cm_sensor_delta,
				configuration->cm_max_delta_sensor_percent);
		result->cm_sensor_delta_pass = false;
	}

	result->cm_test_pass = result->cm_sensor_validation_pass
			& result->cm_sensor_row_delta_pass
			& result->cm_sensor_col_delta_pass
			& result->cm_sensor_calibration_pass
			& result->cm_sensor_delta_pass;
	tp_log_info("cm_test_pass=%d \n", result->cm_test_pass);
exit:
	if (ret) {
		kfree(*cm_sensor_row_delta);
		kfree(*cm_sensor_column_delta);
		*cm_sensor_row_delta = NULL;
		*cm_sensor_column_delta = NULL;
	} else if (pass)
		*pass = result->cm_test_pass;
	return ret;
}


static int calculate_cp_calibration_gen6(int tx_period_self,
		int balancing_target_self, int idac_lsb, int idac_self,
		int rx_atten_self, int gidac_mult, int clk,
		int v_swing)
{
	int i_bias = gidac_mult * idac_lsb * idac_self;
	int t_cal = tx_period_self * balancing_target_self* CONVERSION_CONST / (1000 * clk);

	return i_bias * t_cal * rx_atten_self / v_swing;
}

static int calculate_cp_gen6(int raw_data, int tx_period_self,
		int balancing_target_self, int scaling_factor_self,
		int idac_lsb, int idac_self, int rx_atten_self,
		int gidac_mult, int clk, int v_swing)
{
	int i_bias = gidac_mult * idac_lsb * idac_self;
	int adc = tx_period_self * balancing_target_self / 100 +
				500 * raw_data / (scaling_factor_self * idac_self *rx_atten_self);

	return i_bias * adc * rx_atten_self / (clk * v_swing/100);
}

static int get_cp_calibration_check_test_results_gen6(
		uint16_t tx_num, uint16_t rx_num, uint16_t button_num,
		bool skip_cp_sensor, bool skip_cp_button,
		int32_t **sensor_tx_raw_data, int32_t **sensor_rx_raw_data,
		int **cp_sensor_tx_data,
		int **cp_sensor_rx_data,
		int *cp_sensor_tx_average, int *cp_sensor_rx_average,
		int *cp_sensor_rx_calibration,
		int *cp_sensor_tx_calibration)
{
	union parameter_value parameter_value;
	enum parameter_type parameter_type;
	uint16_t read_length;
	uint8_t data_format;
	int8_t idac_self_rx;
	int8_t idac_self_tx;
	uint8_t rxdac;
	uint8_t ref_scale;
	int rx_atten_self_rx;
	int rx_atten_self_tx;
	int vref_low;
	int vref_mid;
	int vref_high;
	int v_swing;
	uint32_t tx_period_self;
	uint32_t scaling_factor_self;
	uint32_t balancing_target_self;
	uint32_t gidac_mult;
	int idac_lsb = IDAC_LSB_DEFAULT;
	int clk = CLK_DEFAULT; 
	uint8_t data[IDAC_AND_RX_ATTENUATOR_CALIBRATION_DATA_LENGTH];
	int ret;
	int i;

	/* Get CDC:REFGEN_CTL:RXDAC */
	ret = get_configuration_parameter(RXDAC, &parameter_type,
			&parameter_value);
	if (ret || parameter_type != INTEGER) {
		tp_log_err("%s:Unable to get rxdac!\n",__func__);
		goto exit;
	}

	rxdac = parameter_value.integer;
	tp_log_info("%s: rxdac: %d\n", __func__, rxdac);

	ret = get_configuration_parameter(REF_SCALE, &parameter_type,
			&parameter_value);
	if (ret || parameter_type != INTEGER) {
		tp_log_err("%s:Unable to get rxdac!\n",__func__);
		goto exit;
	}

	ref_scale = parameter_value.integer;
	tp_log_info("%s: ref_scale: %d\n", __func__, ref_scale);

	ret = selfcap_signal_swing_lookup(ref_scale, rxdac,
			&vref_low, &vref_mid, &vref_high);
	if (ret) {
		tp_log_err("%s:Invalid ref_scale or rxdac!\n",__func__);
		goto exit;
	}

	tp_log_info("%s: vref_low: %d\n", __func__, vref_low);
	tp_log_info("%s: vref_mid: %d\n", __func__, vref_mid);
	tp_log_info("%s: vref_high: %d\n", __func__, vref_high);

	v_swing = vref_high - vref_low;

	tp_log_info("%s: v_swing: %d\n", __func__, v_swing);

	/* Get CDC:INFRA_CTRL:GIDAC_MULT */
	ret = get_configuration_parameter(GIDAC_MULT, &parameter_type,
			&parameter_value);
	if (ret || parameter_type != INTEGER) {
		tp_log_err("%s:Unable to get infra_ctrl!\n",__func__);
		goto exit;
	}

	gidac_mult = parameter_value.integer;
	tp_log_info("%s: gidac_mult: %d\n", __func__, gidac_mult);
	/* Step 1: Perform calibration */
	ret = pip_calibrate_idacs(0);
	if (ret) {
		tp_log_err("%s:Unable to calibrate self IDACs!\n", __func__);
		goto exit;
	}
	if(button_num > 0)
	{
		ret = pip_calibrate_idacs(1);
		if (ret) {
			tp_log_err("%s:Unable to calibrate button IDACs!\n", __func__);
			goto exit;
		}
	}

	tp_log_info("%s: Get Self IDAC and RX Attenuator values\n", __func__);

	/* Step 2: Get Self IDAC and RX Attenuator Self RX values */
	ret = pip_retrieve_data_structure(0,
			IDAC_AND_RX_ATTENUATOR_CALIBRATION_DATA_LENGTH,
			IDAC_AND_RX_ATTENUATOR_CALIBRATION_DATA_ID,
			&read_length, &data_format, data);
	if (ret) {
		tp_log_err("%s:Unable to retrieve data structure!\n",__func__);
		goto exit;
	}

	/* Step 4a: Execute Panel Scan */
	ret = pip_execute_panel_scan();
	if (ret) {
		tp_log_err("%s:Unable to execute panel scan!\n",__func__);
		goto exit;
	}

	if (skip_cp_sensor)
		goto process_cp_button;

	/* Allocate sensor rx and tx buffers */
	*sensor_tx_raw_data = kzalloc(tx_num * sizeof(int32_t), GFP_KERNEL);
	*sensor_rx_raw_data = kzalloc(rx_num * sizeof(int32_t), GFP_KERNEL);
	*cp_sensor_tx_data = kzalloc(tx_num * sizeof(float), GFP_KERNEL);
	*cp_sensor_rx_data = kzalloc(rx_num * sizeof(float), GFP_KERNEL);
	if (!*sensor_tx_raw_data || !*sensor_rx_raw_data
			|| !*cp_sensor_tx_data || !*cp_sensor_rx_data) {
		ret = -ENOMEM;
		goto free_buffers;
	}

	/* Step 4b: Retrieve Panel Scan raw data */
	ret = retrieve_panel_raw_data(SELF_CAP_RAW_DATA_ID, 0, rx_num,
			&data_format, *sensor_rx_raw_data);
	if (ret) {
		tp_log_err("%s:Unable to retrieve panel raw data!\n",__func__);
		goto free_buffers;
	}

	ret = retrieve_panel_raw_data(SELF_CAP_RAW_DATA_ID, rx_num, tx_num,
			&data_format, *sensor_tx_raw_data);
	if (ret) {
		tp_log_err("%s:Unable to retrieve panel raw data!\n",__func__);
		goto free_buffers;
	}

	ret = rx_attenuator_lookup(data[RX_ATTENUATOR_SELF_RX_INDEX],
			&rx_atten_self_rx);
	if (ret) {
		tp_log_err("%s:Invalid RX Attenuator Index!\n",__func__);
		goto free_buffers;
	}

	idac_self_rx = (int8_t)data[IDAC_SELF_RX_INDEX];

	ret = rx_attenuator_lookup(data[RX_ATTENUATOR_SELF_TX_INDEX],
			&rx_atten_self_tx);
	if (ret) {
		tp_log_err("%s:Invalid RX Attenuator Index!\n",__func__);
		goto free_buffers;
	}

	idac_self_tx = (int8_t)data[IDAC_SELF_TX_INDEX];

	tp_log_info("%s: idac_self_rx: %d\n", __func__, idac_self_rx);
	tp_log_info("%s: rx_atten_self_rx: %d\n", __func__, rx_atten_self_rx);
	tp_log_info("%s: idac_self_tx: %d\n", __func__, idac_self_tx);
	tp_log_info("%s: rx_atten_self_tx: %d\n", __func__, rx_atten_self_tx);

	/* Get CDC:TX_PERIOD_SELF */
	ret = get_configuration_parameter(TX_PERIOD_SELF, &parameter_type,
				&parameter_value);
	if (ret || parameter_type != INTEGER) {
		tp_log_err("%s:Unable to get tx_period_self!\n",__func__);
		goto free_buffers;
	}

	tx_period_self = parameter_value.integer;
	tp_log_info("%s: tx_period_self: %d\n", __func__, tx_period_self);

	/* Get CDC:SCALING_FACTOR_SELF */
	ret = get_configuration_parameter(SCALING_FACTOR_SELF, &parameter_type,
				&parameter_value);
	if (ret || parameter_type != INTEGER) {
		tp_log_err("%s:Unable to get scaling_factor_self!\n",__func__);
		goto free_buffers;
	}

	scaling_factor_self = parameter_value.integer;
	tp_log_info("%s: scaling_factor_self: %d\n", __func__,
			scaling_factor_self);

	/* Get Calibration:BALANCING_TARGET_SELF */
	ret = get_configuration_parameter(BALANCING_TARGET_SELF,
				&parameter_type, &parameter_value);
	if (ret || parameter_type != INTEGER) {
		tp_log_err("%s:Unable to get balancing_target_self!\n",__func__);
		goto free_buffers;
	}

	balancing_target_self = parameter_value.integer;
	tp_log_info("%s: balancing_target_self: %d\n", __func__,
			balancing_target_self);

	*cp_sensor_rx_calibration = calculate_cp_calibration_gen6(tx_period_self,
			balancing_target_self, idac_lsb, idac_self_rx,
			rx_atten_self_rx, gidac_mult, clk, v_swing);

	tp_log_info("%s: cp_sensor_rx_calibration: %d\n", __func__,
			*cp_sensor_rx_calibration);

	*cp_sensor_tx_calibration = calculate_cp_calibration_gen6(tx_period_self,
			balancing_target_self, idac_lsb, idac_self_tx,
			rx_atten_self_tx, gidac_mult, clk, v_swing);

	tp_log_info("%s: cp_sensor_tx_calibration: %d\n", __func__,
			*cp_sensor_tx_calibration);

#ifdef LOG_IS_NOT_SHOW_RAW_DATA
	tp_log_info("%s: sensor_rx_raw_data[0..%d]:\n", __func__, rx_num - 1);
	for (i = 0; i < rx_num; i++){
		tp_log_info("%5d ", (*sensor_rx_raw_data)[i]);}
	tp_log_info("\n");
#endif
	*cp_sensor_rx_average = 0;
	for (i = 0; i < rx_num; i++) {
		(*cp_sensor_rx_data)[i] = calculate_cp_gen6((*sensor_rx_raw_data)[i],
				tx_period_self, balancing_target_self,
				scaling_factor_self, idac_lsb, idac_self_rx,
				rx_atten_self_rx, gidac_mult, clk, v_swing);
		*cp_sensor_rx_average += (*cp_sensor_rx_data)[i];
	}
	*cp_sensor_rx_average /= rx_num;
#ifdef LOG_IS_NOT_SHOW_RAW_DATA
	tp_log_info("%s: cp_sensor_rx_data[0..%d]:\n", __func__, rx_num - 1);
	for (i = 0; i < rx_num; i++){
		tp_log_info("%d ", (*cp_sensor_rx_data)[i]);}
	tp_log_info("\n");
#endif
	tp_log_info("%s: cp_sensor_rx_average: %d\n", __func__,
			*cp_sensor_rx_average);
#ifdef LOG_IS_NOT_SHOW_RAW_DATA
	tp_log_info("%s: sensor_tx_raw_data[0..%d]:\n", __func__, tx_num - 1);
	for (i = 0; i < tx_num; i++)
	{
		tp_log_info("%5d ", (*sensor_tx_raw_data)[i]);
	}

#endif
	*cp_sensor_tx_average = 0;
	for (i = 0; i < tx_num; i++) {
		(*cp_sensor_tx_data)[i] = calculate_cp_gen6((*sensor_tx_raw_data)[i],
				tx_period_self, balancing_target_self,
				scaling_factor_self, idac_lsb, idac_self_tx,
				rx_atten_self_tx, gidac_mult, clk, v_swing);
		*cp_sensor_tx_average += (*cp_sensor_tx_data)[i];
	}
	*cp_sensor_tx_average /= tx_num;
#ifdef LOG_IS_NOT_SHOW_RAW_DATA
	tp_log_info("%s: cp_sensor_tx_data[0..%d]:\n", __func__, tx_num - 1);
	for (i = 0; i < tx_num; i++)
	{
		tp_log_info("%d ", (*cp_sensor_tx_data)[i]);
	}
	tp_log_info("\n");
#endif
	tp_log_info("%s: cp_sensor_tx_average: %d\n", __func__,
			*cp_sensor_tx_average);

process_cp_button:
	if (skip_cp_button)
		goto exit;
free_buffers:
	if (ret) {
		if (!skip_cp_sensor) {
			kfree(*sensor_rx_raw_data);
			kfree(*sensor_tx_raw_data);
			kfree(*cp_sensor_rx_data);
			kfree(*cp_sensor_tx_data);
			*sensor_rx_raw_data = NULL;
			*sensor_tx_raw_data = NULL;
			*cp_sensor_rx_data = NULL;
			*cp_sensor_tx_data = NULL;
		}
	}

exit:
	return ret;
}

static int validate_cp_test_results_gen6(struct configuration *configuration,
		struct result *result, uint32_t tx_num, uint32_t rx_num,
		bool skip_cp_sensor,
		int *cp_sensor_rx_data, int *cp_sensor_tx_data,
		int cp_sensor_rx_calibration,
		int cp_sensor_tx_calibration,
		int cp_sensor_rx_average, int cp_sensor_tx_average,
		int cp_sensor_rx_delta,
		int cp_sensor_tx_delta,
		bool *pass)
{
	uint32_t i=0;
	uint32_t configuration_rx_num;
	uint32_t configuration_tx_num;
	result->cp_test_pass = true;
	configuration_rx_num=configuration->cp_min_max_table_rx_size/2;
	configuration_tx_num=configuration->cp_min_max_table_tx_size/2;

	/* Check Sensor Cp delta for range limit */
	result->cp_sensor_delta_pass = true;
	if ((cp_sensor_rx_delta > configuration->cp_max_delta_sensor_rx_percent)
			|| (cp_sensor_tx_delta >
				configuration->cp_max_delta_sensor_tx_percent)) {
		tp_log_err("%s: Cp_sensor_rx_delta:%d(%d) Cp_sensor_tx_delta:%d (%d)\n",
				"Cp sensor delta range limit test",
				cp_sensor_rx_delta,
				configuration->cp_max_delta_sensor_rx_percent,
				cp_sensor_tx_delta,
				configuration->cp_max_delta_sensor_tx_percent);
		result->cp_sensor_delta_pass = false;
	}

	/* Check sensor Cp rx for min/max values */
	result->cp_rx_validation_pass = true;
	for (i = 0; i < configuration_rx_num; i++) {
		int cp_rx_min =
			configuration->cp_min_max_table_rx[i * 2];
		int cp_rx_max =
			configuration->cp_min_max_table_rx[i * 2 + 1];
		if ((cp_sensor_rx_data[i] <= cp_rx_min)
				|| (cp_sensor_rx_data[i] >= cp_rx_max)) {
			tp_log_err("%s: Cp Rx[%d]:%d (%d,%d)\n",
					"Cp Rx min/max test",
					i,
					(int)cp_sensor_rx_data[i],
					cp_rx_min, cp_rx_max);
			result->cp_rx_validation_pass = false;
		}
	}
	/* Check sensor Cp tx for min/max values */
	result->cp_tx_validation_pass = true;
	for (i = 0; i < configuration_tx_num; i++) {
		int cp_tx_min =
			configuration->cp_min_max_table_tx[i * 2];
		int cp_tx_max =
			configuration->cp_min_max_table_tx[i * 2 + 1];
		if ((cp_sensor_tx_data[i] <= cp_tx_min)
				|| (cp_sensor_tx_data[i] >= cp_tx_max)) {
			tp_log_err("%s: Cp Tx[%d]:%d(%d,%d)\n",
					"Cp Tx min/max test",
					i,
					(int)cp_sensor_tx_data[i],
					cp_tx_min, cp_tx_max);
			result->cp_tx_validation_pass = false;
		}
	}
		
	result->cp_test_pass &= result->cp_sensor_delta_pass
			& result->cp_rx_validation_pass
			& result->cp_tx_validation_pass;

	if (pass)
		*pass = result->cp_test_pass;

	return 0;
}

static int get_extra_parameter(uint32_t* sensor_assignment,uint32_t * config_ver,uint32_t * revision_ctrl,uint32_t * device_id_high,uint32_t * device_id_low){
    union parameter_value parameter_value;
    enum parameter_type parameter_type;
    struct pip_sysinfo sysinfo;
    int i;
    int ret;
    *device_id_high=0;
    *device_id_low=0;
    /* Get Device Setup:SENSOR_ASSIGNMENT */
    ret = get_configuration_parameter(SENSOR_ASSIGNMENT, &parameter_type,
                &parameter_value);
    if (ret || parameter_type != STRING) {
        tp_log_err("%s:Unable to get sensor assignment!\n",__func__);
        goto exit;
    }
    *sensor_assignment = !strcmp(parameter_value.string,RX_IS_Y);
    tp_log_info("%s: sensor_assignment: %d\n", __func__, *sensor_assignment);
    
    /* Get config version and revision control version */
    ret=pip_get_system_information(&sysinfo);
    if(ret){
        tp_log_err("%s:Unable to get system information!\n",__func__);
        goto exit;
    }
    *revision_ctrl=sysinfo.cy_data.fw_revctrl;
    *config_ver =sysinfo.cy_data.fw_ver_conf;
    tp_log_info("%s: config_ver: %d\n", __func__, *config_ver);
    tp_log_info("%s: revision_ctrl: %d\n", __func__, *revision_ctrl);
    for(i=0;i<4;i++){
        *device_id_low=(*device_id_low<<8)+sysinfo.cy_data.mfg_id[i];
    }
    for(i=4;i<8;i++){
        *device_id_high=(*device_id_high<<8)+sysinfo.cy_data.mfg_id[i];
    }
    tp_log_info("%s: device_id_low: 0x%x\n", __func__, *device_id_low);
    tp_log_info("%s: device_id_high: 0x%x\n", __func__, *device_id_high);
    return 0;
exit:
    return ret;
}
static int check_tests(uint32_t *tx_num, uint32_t *rx_num, uint32_t *button_num,
        bool *skip_cm_button, bool *skip_cp_sensor,
        bool *skip_cp_button)
{
    union parameter_value parameter_value;
    enum parameter_type parameter_type;
    uint32_t act_lft_en_enabled;
    uint32_t bl_h2o_rjct_enabled;
    char *scanning_mode_button;
    int ret;

    /* Get CDC:TX_NUM */
    ret = get_configuration_parameter(TX_NUM, &parameter_type,
                &parameter_value);
    if (ret || parameter_type != INTEGER) {
        tp_log_err("%s:Unable to get tx_num!\n",__func__);
        goto exit;
    }

    *tx_num = parameter_value.integer;
    tp_log_info("%s: tx_num: %d\n", __func__, *tx_num);

    /* Get CDC:RX_NUM */
    ret = get_configuration_parameter(RX_NUM, &parameter_type,
                &parameter_value);
    if (ret || parameter_type != INTEGER) {
        tp_log_err("%s:Unable to get rx_num!\n",__func__);
        goto exit;
    }

    *rx_num = parameter_value.integer;
    tp_log_info("%s: rx_num: %d\n", __func__, *rx_num);

    /* Get CDC:BUTTON_NUM */
    ret = get_configuration_parameter(BUTTON_NUM, &parameter_type,
                &parameter_value);
    if (ret || parameter_type != INTEGER) {
        tp_log_err("%s:Unable to get button_num!\n",__func__);
        goto exit;
    }

    *button_num = parameter_value.integer;
    tp_log_info("%s: button_num: %d\n", __func__, *button_num);

    /* Get FingerTracking:ACT_LFT_EN */
    ret = get_configuration_parameter(ACT_LFT_EN, &parameter_type,
                &parameter_value);
    if (ret || parameter_type != STRING) {
        tp_log_err("%s:Unable to get act_lft_en!\n",__func__);
        goto exit;
    }

    act_lft_en_enabled = !strcmp(parameter_value.string,
                ACT_LFT_EN_ENABLED);
    tp_log_info("%s: act_lft_en: %d\n", __func__, act_lft_en_enabled);

    /* Get ScanFiltering:BL_H20_RJCT */
    ret = get_configuration_parameter(BL_H20_RJCT, &parameter_type,
                &parameter_value);
    if (ret || parameter_type != STRING) {
        tp_log_err("%s:Unable to get bl_h2o_rjct!\n",__func__);
        goto exit;
    }

    bl_h2o_rjct_enabled = !strcmp(parameter_value.string,
                BL_H20_RJCT_ENABLED);
    tp_log_info("%s: bl_h2o_rjct: %d\n", __func__, bl_h2o_rjct_enabled);

    /* Get CDC:SCANNING_MODE_BUTTON */
    ret = get_configuration_parameter(SCANNING_MODE_BUTTON,
                &parameter_type, &parameter_value);
    if (ret < 0|| parameter_type != STRING) {
        tp_log_err("%s:Unable to get scanning_mode_button! ret = %d\n",__func__, ret);
        goto exit;
    }
    scanning_mode_button = parameter_value.string;
    tp_log_info("%s: scanning_mode_button: %s\n", __func__,
            scanning_mode_button);

    *skip_cm_button = false;
    *skip_cp_button = false;
    *skip_cp_sensor = false;

    if (*button_num == 0) {
        *skip_cm_button = true;
        *skip_cp_button = true;
    } else if (!strcmp(scanning_mode_button,
                SCANNING_MODE_BUTTON_MUTUAL))
        *skip_cp_button = true;
    else if (!strcmp(scanning_mode_button, SCANNING_MODE_BUTTON_SELF))
        *skip_cm_button = true;

    if (!act_lft_en_enabled && !bl_h2o_rjct_enabled)
        *skip_cp_sensor = true;

exit:
    return ret;
}

int cm_cp_test_run(char *device_path, struct file *parameter_file,
        struct file *configuration_file, struct seq_file *result_file, int vdda,
        bool run_cm_test, bool run_cp_test, bool *cm_test_pass,
        bool *cp_test_pass)
{
    struct configuration *configuration;
    struct result *result;
    //char tmp_buf[5];
    uint32_t tx_num, rx_num, button_num;
    uint32_t sensor_assignment=0;//if tx is column, then sensor_assignment is 1;
    uint32_t config_ver,revision_ctrl,device_id_high,device_id_low;
    bool skip_cm_button, skip_cp_button, skip_cp_sensor;
    int32_t *cm_sensor_raw_data = NULL;
    int *cm_sensor_data = NULL;
    int *cm_sensor_column_delta = NULL, *cm_sensor_row_delta = NULL;
    struct gd_sensor *cm_gradient_col = NULL, *cm_gradient_row = NULL;
    int cm_sensor_average;
    int cm_sensor_calibration;
    int cm_sensor_delta;
    int32_t *cp_sensor_rx_raw_data = NULL, *cp_sensor_tx_raw_data = NULL;
    int *cp_sensor_rx_data = NULL, *cp_sensor_tx_data = NULL;
    int cp_sensor_rx_calibration, cp_sensor_tx_calibration;
    int cp_sensor_rx_average, cp_sensor_tx_average;
    int cp_sensor_rx_delta, cp_sensor_tx_delta;
    int ret = 0;
    tp_log_info("%s: begin\n" ,__func__ );
    
    configuration = kzalloc(sizeof(struct configuration), GFP_KERNEL);
    result = kzalloc(sizeof(struct result), GFP_KERNEL);
    if (!configuration || !result) {
        ret = -ENOMEM;
        tp_log_err("%s: malloc configuration result fail\n" ,__func__ );
        seq_printf(result_file, "2F -software_reason");
        goto exit;
    }

    memset(configuration, 0, sizeof(struct configuration));
    memset(result, 0, sizeof(struct result));

    ret = parameter_init(parameter_file);
    if (ret) {
        tp_log_err("%s: Fail initing parameters!\n" ,__func__ );
        seq_printf(result_file, "2F -software_reason");
        goto exit;
    }

    ret = configuration_get(configuration_file, configuration);
    if (ret) {
        tp_log_err("%s: Fail getting configuration\n" ,__func__ );
        seq_printf(result_file, "2F -software_reason");
        goto parameter_exit;
    }

    ret = pip_init(device_path, HID_DESCRIPTOR_REGISTER);
    if (ret) {
        tp_log_err("%s: Unable to init pip!\n" ,__func__ );
        seq_printf(result_file, "2F -software_reason");
        goto parameter_exit;
    }

    /* Suspend scanning */
    ret = pip_suspend_scanning();
    if (ret) {
        tp_log_err("%s: Unable to suspend scanning!\n" ,__func__ );
        seq_printf(result_file, "2F -software_reason");
        goto pip_exit;
    }

    ret = check_tests(&tx_num, &rx_num, &button_num,
            &skip_cm_button, &skip_cp_sensor, &skip_cp_button);
    if (ret) {
        tp_log_err("%s: Unable to check_tests !\n" ,__func__ );
        seq_printf(result_file, "2F -software_reason");
        goto resume_scanning;
    }
    /*add get_extra_parameter item*/
    ret=get_extra_parameter(&sensor_assignment,&config_ver,&revision_ctrl,&device_id_high,&device_id_low);
    if (ret) {
        tp_log_err("%s:Unable to get_extra_parameter\n",__func__);
        seq_printf(result_file, "2F -software_reason");
    }
    else{
        tp_log_info("%s: get_extra_parameter pass\n", __func__);
        result->sensor_assignment=sensor_assignment;
        result->config_ver=config_ver;
        result->revision_ctrl=revision_ctrl;
        result->device_id_high=device_id_high;
        result->device_id_low=device_id_low;
    }
    /*add short test item*/
    result->short_test_pass=true;
    ret = pip_short_test();
    if (ret) {
        result->short_test_pass=false;
        tp_log_err("%s:Unable to do short test\n",__func__);
        goto resume_scanning;
    }
    else{
        tp_log_info("%s: short tets pass\n", __func__);
    }
    seq_printf(result_file, "2P-");
    if (run_cp_test && skip_cp_sensor)
        tp_log_info("%s:Cp sensor test is skipped\n",__func__);
    if (run_cp_test && skip_cp_button)
        tp_log_info("%s:Cp button test is skipped\n",__func__);
    if (run_cm_test && skip_cm_button)
        tp_log_info("%s:Cm button test is skipped\n",__func__);

    if (run_cm_test) {
        ret = get_cm_uniformity_test_results_gen6(vdda, tx_num, rx_num,
                button_num, skip_cm_button,
                &cm_sensor_raw_data,
                &cm_sensor_data, 
                &cm_sensor_average);
        if (ret) {
            seq_printf(result_file, "3F -software_reason");
            tp_log_err("%s:Unable to run Cm uniformity test gen6!\n",__func__);
            goto err_get_cm_test_results;
        }

        ret = get_cm_calibration_check_test_results_gen6(vdda,
                skip_cm_button, &cm_sensor_calibration);
        if (ret) {
            seq_printf(result_file, "3F -software_reason");
            tp_log_err("%s:Unable to run Cm calibration check test gen6!\n",__func__);
            goto free_buffers;
        }
	cm_sensor_delta = ABS((cm_sensor_average -
				cm_sensor_calibration)*100/cm_sensor_average);
	tp_log_info("%s: cm_sensor_delta: %d\n", __func__,
			cm_sensor_delta);
	ret = validate_cm_test_results_gen6(configuration, result, tx_num,
			rx_num, button_num, skip_cm_button,
			cm_sensor_data,
			cm_sensor_calibration,
			cm_sensor_average,
			cm_sensor_delta,
			&cm_sensor_column_delta, &cm_sensor_row_delta,
            cm_test_pass);
        if (ret) {
            seq_printf(result_file, "3F -software_reason");
			tp_log_err("%s:Fail validating Cm test results!\n",__func__);
			goto free_buffers;
        }

        result->cm_sensor_data = cm_sensor_data;
        result->cm_sensor_raw_data = cm_sensor_raw_data;
		result->cm_sensor_column_delta = cm_sensor_column_delta;
		result->cm_sensor_row_delta = cm_sensor_row_delta;
		result->cm_sensor_calibration = cm_sensor_calibration;
		result->cm_sensor_average = cm_sensor_average;
		result->cm_sensor_delta = cm_sensor_delta;
    }
    
    if (run_cp_test) {
		ret = get_cp_calibration_check_test_results_gen6(tx_num, rx_num,
				button_num, skip_cp_sensor, skip_cp_button,
	       	    &cp_sensor_tx_raw_data, &cp_sensor_rx_raw_data,
				&cp_sensor_tx_data,
				&cp_sensor_rx_data, 
				&cp_sensor_tx_average, &cp_sensor_rx_average,
				&cp_sensor_rx_calibration,
				&cp_sensor_tx_calibration);
        if (ret) {
            seq_printf(result_file, "4F -software_reason");
            tp_log_err("%s:Unable to run Cp calibration check test!\n",__func__);
			goto free_buffers;
        }

        if (!skip_cp_sensor) {
			cp_sensor_rx_delta = ABS((cp_sensor_rx_calibration -
						cp_sensor_rx_average) *100/
						cp_sensor_rx_average);
			cp_sensor_tx_delta = ABS((cp_sensor_tx_calibration -
						cp_sensor_tx_average)*100 /
						cp_sensor_tx_average);
			tp_log_info("cp_sensor_rx_calibration: %d,cp_sensor_rx_average:%d\n", cp_sensor_rx_calibration,cp_sensor_rx_average);
			tp_log_info("cp_sensor_tx_calibration: %d,cp_sensor_tx_average:%d\n", cp_sensor_tx_calibration,cp_sensor_tx_average);
			tp_log_info("%s: cp_sensor_rx_delta: %d\n", __func__,
					cp_sensor_rx_delta);
			tp_log_info("%s: cp_sensor_tx_delta: %d\n", __func__,
					cp_sensor_tx_delta);
            result->cp_sensor_rx_data = cp_sensor_rx_data;
            result->cp_sensor_tx_data = cp_sensor_tx_data;
            result->cp_sensor_rx_raw_data = cp_sensor_rx_raw_data;
            result->cp_sensor_tx_raw_data = cp_sensor_tx_raw_data;
			result->cp_sensor_rx_delta = cp_sensor_rx_delta;
			result->cp_sensor_tx_delta = cp_sensor_tx_delta;
            result->cp_sensor_rx_average =
				cp_sensor_rx_average;
            result->cp_sensor_tx_average =
					cp_sensor_tx_average;
			result->cp_sensor_rx_calibration=
				cp_sensor_rx_calibration;
			result->cp_sensor_tx_calibration=
				cp_sensor_tx_calibration;
        }
    	ret = validate_cp_test_results_gen6(configuration, result, tx_num,
		    	rx_num, skip_cp_sensor,
		    	cp_sensor_rx_data,
                cp_sensor_tx_data,
		    	cp_sensor_rx_calibration,
		    	cp_sensor_tx_calibration,
			    cp_sensor_rx_average, cp_sensor_tx_average,
		    	cp_sensor_rx_delta,
		    	cp_sensor_tx_delta,
                cp_test_pass);
        if (ret) {
            seq_printf(result_file, "7F -software_reason");
            tp_log_err("%s:Fail validating Cp test results!\n",__func__);
            goto free_buffers;
        }


    result->tx_num = tx_num;
    result->rx_num = rx_num;
    result->button_num = button_num;

    result->cm_test_run = run_cm_test;
    result->cp_test_run = run_cp_test;
    result->test_summary= result->cm_test_pass && result->cp_test_pass;

    tp_log_info("%s,get over\n", __func__);
}
    ret = result_save(result_file, configuration, result);
    if (ret) {
        tp_log_err("%s:Fail saving result\n",__func__);
        goto free_buffers;
    }

free_buffers:

    if (run_cp_test) {
        if (!skip_cp_sensor) {
            kfree(cp_sensor_rx_raw_data);
            cp_sensor_rx_raw_data = NULL;

            kfree(cp_sensor_tx_raw_data);
            cp_sensor_tx_raw_data = NULL;

            kfree(cp_sensor_rx_data);
            cp_sensor_rx_data = NULL;

            kfree(cp_sensor_tx_data);
            cp_sensor_tx_data = NULL;
        }
    }
err_get_cm_test_results:    
    if (run_cm_test) {
        kfree(cm_sensor_raw_data);
        cm_sensor_raw_data = NULL;

        kfree(cm_sensor_data);
        cm_sensor_data = NULL;

        kfree(cm_gradient_col);
        cm_gradient_col = NULL;

        kfree(cm_gradient_row);
        cm_gradient_row = NULL;
        
    }

resume_scanning:
    if (pip_resume_scanning()) {
        tp_log_err("%s:Unable to resume scanning!\n",__func__);
        goto pip_exit;
    }

pip_exit:
    pip_exit();

parameter_exit:
    parameter_exit();

exit:
    kfree(result);
    result = NULL;

    kfree(configuration);
    configuration = NULL;

    return ret;
}

#define CY_MEM_FREE(x) \
	if (NULL != x) \
	do { \
		kfree(x); \
		x = NULL; \
	} while(0)

static void calculate_gradient_row(struct gd_sensor *gd_sensor_row_head,
            uint16_t row_num, int exclude_row_edge,int exclude_col_edge) {
    int i = 0;
    int cm_min_cur = 0;
    int cm_max_cur = 0;
    int cm_ave_cur = 0;
    int cm_ave_next = 0;
    int cm_ave_prev = 0;
    struct gd_sensor *p = gd_sensor_row_head;
    if( exclude_row_edge) {
        for(i = 0; i < row_num; i++) {
            if(!exclude_col_edge ) {
                cm_ave_cur = (p + i)->cm_ave;
                cm_min_cur =  (p + i)->cm_min;
                cm_max_cur =  (p + i)->cm_max;
                if(i < ( row_num-1)) {
                    cm_ave_next = (p + i+1)->cm_ave;
                }
                if(i > 0) {
                    cm_ave_prev = (p + i-1)->cm_ave;
                }
            } else {
                cm_ave_cur = (p + i)->cm_ave_exclude_edge;
                cm_min_cur =  (p + i)->cm_min_exclude_edge;
                cm_max_cur =  (p + i)->cm_max_exclude_edge;
                if(i < ( row_num-1)) {
                     cm_ave_next = (p + i+1)->cm_ave_exclude_edge;
                }
                if(i > 0) {
                    cm_ave_prev = (p + i-1)->cm_ave_exclude_edge;
                }
            }
            if(cm_ave_cur == 0) {
                cm_ave_cur = 1;
            }
            if((i == 0) || (i == (row_num-1))) {
                (p + i)->gradient_val = (cm_max_cur -cm_min_cur) *100/ cm_ave_cur;
            } else if(i==1){
                (p + i)->gradient_val = (cm_max_cur -cm_min_cur + ABS(cm_ave_cur -cm_ave_next )) *100/ cm_ave_cur;
            } else {
                (p + i)->gradient_val = (cm_max_cur -cm_min_cur + ABS(cm_ave_cur -cm_ave_prev )) *100/ cm_ave_cur;
            }
        }
    } else if(!exclude_row_edge) {
        for(i = 0; i < row_num; i++) {
                if(!exclude_col_edge ) {
                    cm_ave_cur = (p + i)->cm_ave;
                    cm_min_cur =  (p + i)->cm_min;
                    cm_max_cur =  (p + i)->cm_max;
                    if(i < ( row_num-1)) {
                        cm_ave_next = (p + i+1)->cm_ave;
                    }
                    if(i > 0) {
                        cm_ave_prev = (p + i-1)->cm_ave;
                    }
                } else {
                    cm_ave_cur = (p + i)->cm_ave_exclude_edge;
                    cm_min_cur =  (p + i)->cm_min_exclude_edge;
                    cm_max_cur =  (p + i)->cm_max_exclude_edge;
                    if(i < ( row_num-1)) {
                         cm_ave_next = (p + i+1)->cm_ave_exclude_edge;
                    }
                    if(i > 0) {
                        cm_ave_prev = (p + i-1)->cm_ave_exclude_edge;
                    }
                }
                if(cm_ave_cur == 0) {
                    cm_ave_cur = 1;
                }
                 if(i <= 1) {
                    (p + i)->gradient_val = (cm_max_cur -cm_min_cur + ABS(cm_ave_cur -cm_ave_next )) *100/ cm_ave_cur;
                } else {
                    (p + i)->gradient_val = (cm_max_cur -cm_min_cur + ABS(cm_ave_cur -cm_ave_prev )) *100/ cm_ave_cur;
                }
            }
    }
}
static void calculate_gradient_col(struct gd_sensor *gd_sensor_row_head,
            uint16_t col_num, int  exclude_row_edge,int  exclude_col_edge) {
    int i = 0;
    int cm_min_cur = 0;
    int cm_max_cur = 0;
    int cm_ave_cur = 0;
    int cm_ave_next = 0;
    int cm_ave_prev = 0;
    struct gd_sensor *p = gd_sensor_row_head;
    if( !exclude_col_edge) {
        for(i = 0; i < col_num; i++) {
            if(!exclude_row_edge ) {
                cm_ave_cur = (p + i)->cm_ave;
                cm_min_cur =  (p + i)->cm_min;
                cm_max_cur =  (p + i)->cm_max;
                if(i < ( col_num-1)) {
                    cm_ave_next = (p + i+1)->cm_ave;
                }
                if(i > 0) {
                    cm_ave_prev = (p + i-1)->cm_ave;
                }
            } else {
                cm_ave_cur = (p + i)->cm_ave_exclude_edge;
                cm_min_cur =  (p + i)->cm_min_exclude_edge;
                cm_max_cur =  (p + i)->cm_max_exclude_edge;
                if(i < ( col_num-1)) {
                     cm_ave_next = (p + i+1)->cm_ave_exclude_edge;
                }
                if(i > 0) {
                    cm_ave_prev = (p + i-1)->cm_ave_exclude_edge;
                }
            }
            if(cm_ave_cur == 0) {
                cm_ave_cur = 1;
            }
            if(i <= 1){
                (p + i)->gradient_val = (cm_max_cur -cm_min_cur + ABS(cm_ave_cur -cm_ave_next )) *100/ cm_ave_cur;
            } else {
                (p + i)->gradient_val = (cm_max_cur -cm_min_cur + ABS(cm_ave_cur -cm_ave_prev )) *100/ cm_ave_cur;
            }
        }
    } else if(exclude_col_edge) {
        for(i = 0; i < col_num; i++) {
                if(!exclude_row_edge ) {
                    cm_ave_cur = (p + i)->cm_ave;
                    cm_min_cur =  (p + i)->cm_min;
                    cm_max_cur =  (p + i)->cm_max;
                    if(i < ( col_num-1)) {
                         cm_ave_next = (p + i+1)->cm_ave;
                    }
                    if(i > 0) {
                        cm_ave_prev = (p + i-1)->cm_ave;
                    }
                } else {
                    cm_ave_cur = (p + i)->cm_ave_exclude_edge;
                    cm_min_cur =  (p + i)->cm_min_exclude_edge;
                    cm_max_cur =  (p + i)->cm_max_exclude_edge;
                    if(i < ( col_num-1)) {
                        cm_ave_next =  (p + i+1)->cm_ave_exclude_edge;
                    }
                    if(i > 0) {
                        cm_ave_prev = (p + i-1)->cm_ave_exclude_edge;
                    }
                }
                if(cm_ave_cur == 0) {
                    cm_ave_cur = 1;
                }

                if((i == 0) || (i == (col_num -1))) {
                    (p + i)->gradient_val = (cm_max_cur -cm_min_cur ) *100/ cm_ave_cur;
                }
                 else if(i == 1){
                    (p + i)->gradient_val = (cm_max_cur -cm_min_cur + ABS(cm_ave_cur -cm_ave_next )) *100/ cm_ave_cur;
                } else {
                    (p + i)->gradient_val = (cm_max_cur -cm_min_cur + ABS(cm_ave_cur -cm_ave_prev )) *100/ cm_ave_cur;
                }
            }
    }

}



static void fill_gd_sensor_table(struct gd_sensor * head, int index, uint16_t cm_max, uint16_t cm_min,
    int cm_ave, uint16_t cm_max_exclude_edge, uint16_t cm_min_exclude_edge, int cm_ave_exclude_edge) {
        (head + index)->cm_max = cm_max;
        (head + index)->cm_min = cm_min;
        (head + index)->cm_ave = cm_ave;
        (head + index)->cm_ave_exclude_edge= cm_ave_exclude_edge;
        (head + index)->cm_max_exclude_edge= cm_max_exclude_edge;
        (head + index)->cm_min_exclude_edge= cm_min_exclude_edge;
}

static int sysinfo_get_ttda(struct device *dev,
							uint32_t *tx_num, uint32_t *rx_num,
							struct result *result)
{
	struct cyttsp5_core_data *cd = dev_get_drvdata(dev);
	if (!cd) {
		tp_log_err("%s: cd is null \n", __func__);
		return -EINVAL;
	}
	*tx_num = cd->sysinfo.sensing_conf_data.tx_num;
	*rx_num = cd->sysinfo.sensing_conf_data.rx_num;

	result->config_ver = cd->sysinfo.cydata.fw_ver_conf;
	result->rx_num = *rx_num;
	result->tx_num = *tx_num;

	tp_log_info("%s:tx_num = %d, rx_num = %d, fw_ver_conf = 0x%02x\n", __func__,
					*tx_num, *rx_num, result->config_ver);
	return 0;
}

static int do_calibration_tdda(struct device *dev, bool set_single)
{
	u8 status = 0;
	int rc = 0;
	int res = 0;
	struct cyttsp5_core_commands *cyttsp_cmd = cyttsp5_get_commands();

	if (set_single) {
		/*force single tx*/
		rc = cyttsp_cmd->nonhid_cmd->set_param(dev, 0, 0x1F, 1);
		if (rc) {
			res = rc;
			tp_log_err("%s:force single tx failed", __func__);
		}
	} else {
		/*force multi tx*/
		rc = cyttsp_cmd->nonhid_cmd->set_param(dev, 0, 0x1F, 0);
		if (rc) {
			res = rc;
			tp_log_err("%s:force multi tx failed", __func__);
		}
	}

	/*suspend_scanning */
	rc = cyttsp_cmd->nonhid_cmd->suspend_scanning(dev, 0);
	if (rc) {
		res = rc;
		tp_log_err("%s:suspend_scanning failed", __func__);
	}

	/*do calibration*/
	tp_log_debug("%s:do calibration in single tx mode", __func__);
	rc = cyttsp_cmd->nonhid_cmd->calibrate_idacs(dev,0, 0, &status);
	if (rc < 0) {
		res = rc;
		tp_log_err("%s: Error on calibrate idacs for mutual r=%d\n",
					__func__, rc);
	}

	rc = cyttsp_cmd->nonhid_cmd->calibrate_idacs(dev, 0, 2, &status);
	if (rc < 0) {
		res = rc;
		tp_log_err("%s: Error on calibrate idacs for self r=%d\n",
					__func__, rc);
	}
	/*resume_scanning */
	rc = cyttsp_cmd->nonhid_cmd->resume_scanning(dev, 0);
	if (rc) {
		res = rc;
		tp_log_err("%s:resume_scanning failed", __func__);
	}

	return res;
}
static int short_test_ttda(struct device *dev, struct result *result)
{
	int rc = 0;
	struct cyttsp5_device_access_data *dad;

	dad = cyttsp5_get_device_access_data(dev);

	rc = cyttsp5_run_and_get_selftest_result_noprint(
			dev, CY_ST_ID_AUTOSHORTS, PIP_CMD_MAX_LENGTH, false);
	if (rc) {
		tp_log_err("%s:execute short fail\n", __func__);
		return rc;
	}

	if (dad->ic_buf[1] != 0)
		result->short_test_pass = false;
	else
		result->short_test_pass = true;

	return rc;
}
static int get_cm_test_data_ttda(struct device *dev,
					int tx_num, int rx_num,
					int **cm_sensor_data, int *cm_sensor_average,
					int *cm_sensor_calibration, int *cm_sensor_delta)
{
	int rc = 0;
	int i = 0;
	int cm_average_data = 0;
	int actual_read_length = 0;
	/*
	 * read data format is:
	 * cm_data[0]_low,cm_data[0]_high...
	 * cm_data[rx_num * tx_num]_low,cm_data[rx_num * tx_num]_low,
	 * cm_cal_low,cm_cal_high
	 * so need to read (rx_num * tx_num + 1) * 2 Bytes
	 */
	int read_length = (rx_num * tx_num + 1) * 2;
	struct cyttsp5_device_access_data *dad = cyttsp5_get_device_access_data(dev);
	if (!dad) {
		tp_log_err("%s:get access data fail\n", __func__);
		return -EINVAL;
	}

	(*cm_sensor_data) = kzalloc((tx_num * rx_num) * sizeof(int), GFP_KERNEL);
	if (!*cm_sensor_data) {
		tp_log_err("%s:cm_sensor_data alloc fail\n", __func__);
		rc = -ENOMEM;
		goto exit;
	}

	rc = cyttsp5_run_and_get_selftest_result_noprint(
							dev, CY_ST_ID_CM_PANEL, read_length, true);
	if (rc) {
		tp_log_err("%s:Get CM Panel fail\n", __func__);
		goto exit;
	}

	actual_read_length = get_unaligned_le16(&dad->ic_buf[4]);
	tp_log_info("%s:actual_read_length=%d, (tx_num * rx_num)=%d\n", __func__,
					actual_read_length, (tx_num * rx_num));

	for (i = 0; i < (tx_num * rx_num); i++) {
		(*cm_sensor_data)[i] =
			10*(dad->ic_buf[CM_PANEL_DATA_OFFSET+i*2] + 256
			* dad->ic_buf[CM_PANEL_DATA_OFFSET+i*2+1]);
		tp_log_debug("cm_sensor_data[%d]=%d\n", i, (*cm_sensor_data)[i]);
		cm_average_data += (*cm_sensor_data)[i];
	}

	cm_average_data /= (tx_num * rx_num);
	*cm_sensor_average = cm_average_data;
	*cm_sensor_calibration =
				10*(dad->ic_buf[CM_PANEL_DATA_OFFSET+i*2]
				+ 256 * dad->ic_buf[CM_PANEL_DATA_OFFSET+i*2+1]);
	/*multiple 1000 to increate accuracy*/
	*cm_sensor_delta = ABS((cm_average_data - *cm_sensor_calibration) * 100 /
								cm_average_data);
	return 0;
exit:
	CY_MEM_FREE(*cm_sensor_data);
	return rc;

}
static int validate_cm_test_result_ttda_gen5(struct configuration *configuration,
        struct result *result, uint32_t tx_num, uint32_t rx_num,int *cm_sensor_data,
        struct gd_sensor **gd_sensor_col, struct gd_sensor **gd_sensor_row,
        bool *pass)
{
    int ret = 0;
    int index = 0;
    int i, j;
    uint32_t configuration_sensor_num;
    int sensor_data_min;
    int sensor_data_max;
    int sensor_num;
    int cm_max ;
    int cm_min ;
    int cm_ave ;
    int cm_max_exclude_edge ;
    int cm_min_exclude_edge ;
    int cm_ave_exclude_edge ;
    int cm_data ;

	if(!configuration || !result || !cm_sensor_data ) {
        tp_log_err("%s, param invalid\n", __func__);
        ret = -EINVAL;
        goto exit;
    }
    sensor_num = tx_num *rx_num;
    tp_log_info("%s, sensor_num= %d\n", __func__, sensor_num);

    configuration_sensor_num =
                configuration->cm_min_max_table_sensor_size / 2;

    tp_log_debug("%s, configuration_sensor_num= %d\n", __func__, configuration_sensor_num);
    if (configuration_sensor_num != sensor_num) {
        tp_log_err("%s: Configuration and Device number of sensors mismatch! (Configuration:%d, Device:%d)\n",
            __func__, configuration_sensor_num,
                sensor_num);
            ret = -EINVAL;
            goto exit;
    }


    /*max min test*/
    result->cm_sensor_validation_pass = true;
    for(i = 0; i < tx_num; i++) {
        for(j = 0; j < rx_num; j++) {
            index = i * rx_num + j;
            sensor_data_min = configuration->cm_min_max_table_sensor[2 * index];
            sensor_data_max = configuration->cm_min_max_table_sensor[2 * index + 1];
            if(cm_sensor_data[index] < sensor_data_min||
                cm_sensor_data[index] > sensor_data_max) {

                tp_log_err("%s, cm_sensor min_max failed,sensor[%d][%d] = %d, overflow [%d, %d]\n",
                    __func__ , i, j, cm_sensor_data[index], sensor_data_min, sensor_data_max);
                result->cm_sensor_validation_pass = false;
            }
        }
    }

    tp_log_info("cm_sensor_validation_pass: %d\n", result->cm_sensor_validation_pass);

    *gd_sensor_col = (struct gd_sensor *)kzalloc(tx_num * sizeof(struct gd_sensor), GFP_KERNEL);
    if(*gd_sensor_col == NULL) {
        tp_log_err("failed to malloc for gd_sensor_col\n");
        goto exit;
    }
    *gd_sensor_row =  (struct gd_sensor *)kzalloc(rx_num * sizeof(struct gd_sensor), GFP_KERNEL);
    if(*gd_sensor_row == NULL) {
        tp_log_err("failed to malloc for gd_sensor_row\n");
        goto exit;
    }
    for (j = 0; j < rx_num; j++) {
        /*re-initialize for a new row*/
        cm_max = (cm_sensor_data)[j];
        cm_min =  cm_max;
        cm_ave = 0;
        cm_max_exclude_edge = (cm_sensor_data)[rx_num + j];
        cm_min_exclude_edge =  cm_max_exclude_edge;
        cm_ave_exclude_edge = 0;
        //DEBUG_PRINTF("wuyu initialization:cm_max=%d, cm_max_exclude_edge=%d\n ", cm_max, cm_max_exclude_edge);
        for (i = 0; i < tx_num; i++) {
            cm_data = (cm_sensor_data)[i * rx_num + j];
            if (cm_data > cm_max)
                cm_max = cm_data;
            if ( cm_data < cm_min)
                cm_min = cm_data;
            cm_ave += cm_data;
            /*calculate exclude edge data*/
            if((i >  0) && (i < (tx_num-1))) {
                if (cm_data > cm_max_exclude_edge)
                    cm_max_exclude_edge = cm_data;
                if ( cm_data < cm_min_exclude_edge)
                    cm_min_exclude_edge = cm_data;
                cm_ave_exclude_edge += cm_data;
            }
        }
        cm_ave /= tx_num;
        cm_ave_exclude_edge /= (tx_num-2);
        fill_gd_sensor_table(*gd_sensor_row, j, cm_max, cm_min, cm_ave,
            cm_max_exclude_edge, cm_min_exclude_edge, cm_ave_exclude_edge);
    }

    result->cm_sensor_row_delta_pass = true;
    calculate_gradient_row(*gd_sensor_row, rx_num,
            configuration->cm_excluding_row_edge, configuration->cm_excluding_col_edge);
    for (i = 0;i < configuration->cm_gradient_check_row_size; i++) {
        if((*gd_sensor_row + i)->gradient_val > configuration->cm_gradient_check_row[i]
             || (*gd_sensor_row + i)->gradient_val < 0) {
            tp_log_err("%s cm_max_table_gradient_row_percent[%d]:%d, gradient_val:%d\n", __func__, i, configuration->cm_gradient_check_row[i], (*gd_sensor_row + i)->gradient_val );
            tp_log_debug("%s:row[%d] cm_max:%d cm_min:%d cm_ave:%d cm_max_exclude_edge:%d  cm_min_exclude_edge:%d cm_ave_exclude_edge:%d gradient_val=%d \n",
            __func__, i,
            (*gd_sensor_row + i)->cm_max,
            (*gd_sensor_row + i)->cm_min,
            (*gd_sensor_row + i)->cm_ave,
            (*gd_sensor_row + i)->cm_max_exclude_edge,
            (*gd_sensor_row + i)->cm_min_exclude_edge,
            (*gd_sensor_row + i)->cm_ave_exclude_edge,
            (*gd_sensor_row + i)->gradient_val);
            result->cm_sensor_row_delta_pass = false;
        }
    }
    printk("gd_sensor_row:\n");
    for (i = 0;i < configuration->cm_gradient_check_row_size; i++) {
        printk("%6d", (*gd_sensor_row + i)->gradient_val);
    }
    printk("\n");

    for (i = 0; i < tx_num; i++) {
        /*re-initialize for a new col*/
        cm_max = (cm_sensor_data)[i * rx_num];
        cm_min = cm_max;
        cm_ave = 0;
        cm_max_exclude_edge = (cm_sensor_data)[i * rx_num + 1];
        cm_min_exclude_edge = cm_max_exclude_edge;
        cm_ave_exclude_edge = 0;

        for (j = 0; j < rx_num; j++) {
            cm_data = (cm_sensor_data)[i * rx_num + j];
            if (cm_data > cm_max)
                cm_max = cm_data;
            if ( cm_data < cm_min)
                cm_min = cm_data;
            cm_ave += cm_data;
            /*calculate exclude edge data*/
            if((j > 0) && (j < (rx_num-1))) {
                if (cm_data > cm_max_exclude_edge)
                    cm_max_exclude_edge = cm_data;
                if ( cm_data < cm_min_exclude_edge)
                    cm_min_exclude_edge = cm_data;
                cm_ave_exclude_edge += cm_data;
            }
        }
        cm_ave /= rx_num;
        cm_ave_exclude_edge /= (rx_num-2);
        fill_gd_sensor_table(*gd_sensor_col, i, cm_max, cm_min, cm_ave,
            cm_max_exclude_edge, cm_min_exclude_edge, cm_ave_exclude_edge);
    }

    result->cm_sensor_col_delta_pass = true;
    calculate_gradient_col(*gd_sensor_col, tx_num,
            configuration->cm_excluding_row_edge, configuration->cm_excluding_col_edge);
    for (i = 0;i < configuration->cm_gradient_check_col_size; i++) {
        if((*gd_sensor_col + i)->gradient_val > configuration->cm_gradient_check_col[i]
            || (*gd_sensor_col + i)->gradient_val < 0) {
            tp_log_err("%s cm_max_table_gradient_col_percent[%d]:%d, gradient_val:%d\n", __func__, i, configuration->cm_gradient_check_col[i], (*gd_sensor_col + i)->gradient_val );
            tp_log_debug("%s:col[%d] cm_max:%d cm_min:%d cm_ave:%d cm_max_exclude_edge:%d  cm_min_exclude_edge:%d cm_ave_exclude_edge:%d gradient_val=%d \n",
            __func__, i,
            (*gd_sensor_col + i)->cm_max,
            (*gd_sensor_col + i)->cm_min,
            (*gd_sensor_col + i)->cm_ave,
            (*gd_sensor_col + i)->cm_max_exclude_edge,
            (*gd_sensor_col + i)->cm_min_exclude_edge,
            (*gd_sensor_col + i)->cm_ave_exclude_edge,
            (*gd_sensor_col + i)->gradient_val);
            result->cm_sensor_col_delta_pass = false;
        }
    }
    printk("gd_sensor_col:\n");
    for (i = 0;i < configuration->cm_gradient_check_col_size; i++) {
        printk("%6d", (*gd_sensor_col + i)->gradient_val);
    }
    printk("\n");

    result->cm_test_pass = result->cm_sensor_validation_pass &&
                           result->cm_sensor_col_delta_pass &&
                           result->cm_sensor_row_delta_pass;

    *pass = result->cm_test_pass;
    tp_log_info("cm_test:%d, result cm_test:%d\n", *pass, result->cm_test_pass);

exit:
    return ret;

}
static int get_cp_test_data_ttda(struct device *dev,
					uint16_t tx_num, uint16_t rx_num,
					int **cp_sensor_tx_data,
					int **cp_sensor_rx_data,
					int *cp_sensor_tx_average,
					int *cp_sensor_rx_average,
					int **cp_sensor_rx_calibration,
					int **cp_sensor_tx_calibration,
					int **cp_sensor_rx_detla,
					int **cp_sensor_tx_detla)
{
	int ret = 0;
	long long cp_tx_ave_data_panel = 0;
	long long cp_rx_ave_data_panel = 0;
	struct cyttsp5_device_access_data *dad = cyttsp5_get_device_access_data(dev);
	int i = 0;
	*cp_sensor_tx_data = kzalloc(tx_num * sizeof(int), GFP_KERNEL);
	*cp_sensor_rx_data = kzalloc(rx_num * sizeof(int), GFP_KERNEL);
	*cp_sensor_tx_calibration = kzalloc(tx_num * sizeof(int), GFP_KERNEL);
	*cp_sensor_rx_calibration = kzalloc(rx_num * sizeof(int), GFP_KERNEL);
	*cp_sensor_tx_detla = kzalloc(tx_num * sizeof(int), GFP_KERNEL);
	*cp_sensor_rx_detla = kzalloc(rx_num * sizeof(int), GFP_KERNEL);

	if (!*cp_sensor_tx_data || !*cp_sensor_rx_data ||
		!*cp_sensor_tx_calibration || !*cp_sensor_rx_calibration) {
			tp_log_err("%s: memory alloc fail\n", __func__);
			ret = -ENOMEM;
			goto exit;
	}
	memset(dad->ic_buf, 0 ,CY_MAX_PRBUF_SIZE);
	ret = cyttsp5_run_and_get_selftest_result_noprint(dev,
					CY_ST_ID_CP_PANEL, (rx_num + tx_num) * 4, true);
	if (ret) {
		tp_log_err("%s:Get CP Panel not supported\n", __func__);
		goto exit;
	}

	/*Get cp_tx_data_panel*/
	for (i = 0; i < tx_num; i++) {
		(*cp_sensor_tx_data)[i] =
				(10 * (dad->ic_buf[CP_PANEL_DATA_OFFSET+i*2]
				+ 256 * dad->ic_buf[CP_PANEL_DATA_OFFSET+i*2+1]))/10;
		tp_log_debug("cp_sensor_tx_data[%d]=%d\n", i, (*cp_sensor_tx_data)[i]);
		cp_tx_ave_data_panel += (*cp_sensor_tx_data)[i];
	}
	cp_tx_ave_data_panel /= tx_num;
	*cp_sensor_tx_average = cp_tx_ave_data_panel;

	/*Get cp_tx_cal_data_panel*/
	for (i = 0; i < tx_num; i++) {
		(*cp_sensor_tx_calibration)[i] =
			10*(dad->ic_buf[CP_PANEL_DATA_OFFSET+tx_num*2+i*2]
			+ 256 * dad->ic_buf[CP_PANEL_DATA_OFFSET+tx_num*2+i*2+1]);
		tp_log_debug("cp_sensor_tx_calibration[%d]=%d\n",
						i, (*cp_sensor_tx_calibration)[i]);
	}

	/*get cp_sensor_tx_delta,using the first sensor cal value for temp */
	/*multiple 1000 to increase accuracy*/
	for (i = 0; i < tx_num;  i++) {
		(*cp_sensor_tx_detla)[i]= ABS(((*cp_sensor_tx_calibration)[i]
				- (*cp_sensor_tx_data)[i]) * 100 / (*cp_sensor_tx_data)[i]);
		tp_log_debug("cp_sensor_tx_detla[%d]=%d\n",
						i, (*cp_sensor_tx_detla)[i]);
	}

	/*Get cp_rx_data_panel*/
	for (i = 0; i < rx_num;  i++) {
		(*cp_sensor_rx_data)[i] =
			(10*(dad->ic_buf[CP_PANEL_DATA_OFFSET+tx_num*4+i*2] +
			256 * dad->ic_buf[CP_PANEL_DATA_OFFSET+tx_num*4+i*2+1]))/10;
		tp_log_debug("%s:cp_sensor_rx_data[%d]=%d\n", __func__ , i,
						(*cp_sensor_rx_data)[i]);
		cp_rx_ave_data_panel += (*cp_sensor_rx_data)[i];
	}
	cp_rx_ave_data_panel /= rx_num;
	*cp_sensor_rx_average = cp_rx_ave_data_panel;

	/*Get cp_rx_cal_data_panel*/
	for (i = 0; i < rx_num; i++) {
		(*cp_sensor_rx_calibration)[i] =
			10 * (dad->ic_buf[CP_PANEL_DATA_OFFSET+tx_num*4+rx_num*2+i*2] +
			256 * dad->ic_buf[CP_PANEL_DATA_OFFSET+tx_num*4+rx_num*2+i*2+1]);
		tp_log_debug("cp_sensor_rx_calibration[%d]=%d\n",
						i, (*cp_sensor_rx_calibration)[i]);
	}

	/*get cp_sensor_rx_delta,using the first sensor cal value for temp */
	/*multiple 1000 to increase accuracy*/
	for (i = 0; i < rx_num; i++) {
		(*cp_sensor_rx_detla)[i] = ABS(((*cp_sensor_rx_calibration)[i]
					- (*cp_sensor_rx_data)[i]) * 100 / (*cp_sensor_rx_data)[i]);
		tp_log_debug("cp_sensor_rx_detla[%d]=%d\n",
						i, (*cp_sensor_rx_detla)[i]);
	}

	return 0;

exit:
	CY_MEM_FREE(*cp_sensor_tx_data);
	CY_MEM_FREE(*cp_sensor_rx_data);
	CY_MEM_FREE(*cp_sensor_tx_calibration);
	CY_MEM_FREE(*cp_sensor_rx_calibration);
	CY_MEM_FREE(*cp_sensor_rx_detla);
	CY_MEM_FREE(*cp_sensor_tx_detla);
	return ret;
}

static int validate_cp_test_result_ttda_gen5(int tx_num, int rx_num, struct configuration* configuration,
                int *cp_sensor_rx_data, int *cp_sensor_tx_data,struct result* result,
                bool *cp_test_pass)

{
    int ret = 0;
    int i;
    uint32_t configuration_rx_sensor_num;
    uint32_t configuration_tx_sensor_num;
    int max_rx_sensor, min_rx_sensor;
    int max_tx_sensor, min_tx_sensor;
	int sensor_num = 0;
    if(!configuration || !result || !cp_sensor_rx_data ||!cp_sensor_tx_data) {
        tp_log_err("%s, param invalid\n", __func__);
        ret = -EINVAL;
        goto exit;
    }

    sensor_num = tx_num  + rx_num;
    tp_log_info("%s, sensor_num= %d\n", __func__, sensor_num);

    configuration_rx_sensor_num =
                configuration->cp_min_max_table_rx_size / 2;
    configuration_tx_sensor_num =
                configuration->cp_min_max_table_tx_size / 2;

    tp_log_info("%s, configuration_rx_num:%d , tx_num: %d\n",
            __func__, configuration_rx_sensor_num, configuration_tx_sensor_num);
    if (configuration_rx_sensor_num != rx_num || configuration_tx_sensor_num != tx_num) {
        tp_log_err("%s: Configuration and Device number of sensors mismatch!\n", __func__);
        tp_log_err("%s:configuration:rx:%d, tx:%d Device:rx:%d, tx:%d\n",
            __func__, configuration_rx_sensor_num , configuration_tx_sensor_num,
                rx_num, tx_num );
        ret = -EINVAL;
        goto exit;
    }

    /*rx min_max check*/
    result->cp_rx_validation_pass = true;
    for(i = 0; i < rx_num; i++){
        min_rx_sensor = configuration->cp_min_max_table_rx[2*i];
        max_rx_sensor = configuration->cp_min_max_table_rx[2*i + 1];
        if(cp_sensor_rx_data[i] > max_rx_sensor ||
            cp_sensor_rx_data[i] < min_rx_sensor) {
            tp_log_err("%s, rx[%d],cp_data[%d],over range[%d, %d]\n",
                __func__, i, cp_sensor_rx_data[i],
                min_rx_sensor, max_rx_sensor);
            result->cp_rx_validation_pass = false;
        }
    }

    /*tx min_max check*/
    result->cp_tx_validation_pass = true;
    for(i = 0; i < tx_num; i++){
        min_tx_sensor = configuration->cp_min_max_table_tx[2*i];
        max_tx_sensor = configuration->cp_min_max_table_tx[2*i + 1];
        if(cp_sensor_tx_data[i] > max_tx_sensor ||
            cp_sensor_tx_data[i] < min_tx_sensor) {
            tp_log_err("%s, tx[%d],cp_data[%d],over range[%d, %d]\n",
                __func__, i, cp_sensor_tx_data[i],
                min_tx_sensor, max_tx_sensor);
            result->cp_tx_validation_pass = false;
        }
    }

exit:
    result->cp_test_pass = result->cp_tx_validation_pass &&
                           result->cp_rx_validation_pass;
    *cp_test_pass = result->cp_test_pass;
    tp_log_info("cp_test:%d\n", *cp_test_pass);
    return ret;
}
int cm_cp_test_run_ttda(struct device *dev,
			struct file *configuration_file,
			struct seq_file *result_file,  bool run_cm_test,
			bool run_cp_test, bool *cm_test_pass, bool *cp_test_pass)
{
	struct configuration *configuration = NULL;
	struct result *result = NULL;
	uint32_t tx_num, rx_num;
	int *cm_sensor_data = NULL;
	int *cm_sensor_column_delta = NULL, *cm_sensor_row_delta = NULL;
	int cm_sensor_average = 0;
	int cm_sensor_calibration = 0;
	int cm_sensor_delta = 0;
	int *cp_sensor_rx_data = NULL, *cp_sensor_tx_data = NULL;
	int *cp_sensor_rx_calibration = NULL, *cp_sensor_tx_calibration = NULL;
	int cp_sensor_rx_average = 0, cp_sensor_tx_average = 0;
	int *cp_sensor_rx_delta = NULL, *cp_sensor_tx_delta = NULL;
	int ret = 0;
	struct gd_sensor *cm_gradient_col = NULL, *cm_gradient_row = NULL;

	tp_log_info("%s:test begin\n", __func__);
	if(!dev) {
		tp_log_err("%s:dev is null\n", __func__);
		seq_printf(result_file, "1F-");
		goto software_reason_exit;
	}

	configuration = kzalloc(sizeof(struct configuration), GFP_KERNEL);
	result = kzalloc(sizeof(struct result), GFP_KERNEL);
	if (!configuration || !result) {
		ret = -ENOMEM;
		tp_log_err("%s: kzalloc configuration result fail\n", __func__);
		seq_printf(result_file, "1F-");
		goto software_reason_exit;
	}
	ret  = sysinfo_get_ttda(dev, &tx_num, &rx_num, result);
	if (ret || !tx_num || !rx_num) {
		tp_log_err("%s:sysinfo get fail\n", __func__);
		seq_printf(result_file, "1F-");
		goto software_reason_exit;
	}

	ret = configuration_get(configuration_file, configuration);
	if (ret) {
		tp_log_err("%s: Fail getting configuration\n", __func__);
		seq_printf(result_file, "1F-");
		goto software_reason_exit;
	}

	g_family_type = (uint8_t)configuration->family_type;
	tp_log_info("%s: g_family_type:%d\n", __func__,g_family_type);

	ret = do_calibration_tdda(dev, true);
	if (ret) {
		tp_log_err("%s: calibration fail\n", __func__);
		seq_printf(result_file, "1F-");
		goto software_reason_exit;
	}

	ret = short_test_ttda(dev, result);
	if (ret) {
		tp_log_err("%s: short test fail cause of software reason\n", __func__);
		seq_printf(result_file, "1F-");
		goto software_reason_exit;
	} else if (!result->short_test_pass) {
		tp_log_err("%s: short test fail cause of panel reason\n", __func__);
		seq_printf(result_file, "1F-");
	} else {
		tp_log_info("%s: short test pass\n", __func__);
	}

	ret = get_cm_test_data_ttda(dev, tx_num, rx_num,
					&cm_sensor_data, &cm_sensor_average,
					&cm_sensor_calibration, &cm_sensor_delta);
	if (ret) {
		tp_log_err("%s: get cm data fail, ret = %d\n", __func__, ret);
		seq_printf(result_file, "2F-");
		goto software_reason_exit;
	} else {
		tp_log_info("%s:get cm data succ\n", __func__);
	}

	ret = validate_cm_test_result_ttda_gen5(configuration,
		result,tx_num,rx_num,cm_sensor_data,&cm_gradient_col,
		&cm_gradient_row,cm_test_pass);
	if (ret) {
		tp_log_err("%s:validate cm test result fail, ret = %d\n", __func__, ret);
		seq_printf(result_file, "3F-");
		goto software_reason_exit;
	} else {
		tp_log_info("%s:validate cm test result succ\n", __func__);
	}
	result->cm_sensor_data = cm_sensor_data;
	result->cm_sensor_raw_data = NULL;
	result->cm_gradient_col = cm_gradient_col;
	result->cm_gradient_row = cm_gradient_row;


	ret = get_cp_test_data_ttda(dev,
					tx_num, rx_num,
					&cp_sensor_tx_data,	&cp_sensor_rx_data,
					&cp_sensor_tx_average, &cp_sensor_rx_average,
					&cp_sensor_rx_calibration, &cp_sensor_tx_calibration,
					&cp_sensor_rx_delta, &cp_sensor_tx_delta);
	if (ret) {
		tp_log_err("%s:get cp data fail, ret = %d\n", __func__, ret);
		seq_printf(result_file, "4F-");
		goto software_reason_exit;
	} else {
		tp_log_info("%s:get cp data succ\n", __func__);
	}

	ret = validate_cp_test_result_ttda_gen5(tx_num, rx_num,
					configuration, cp_sensor_rx_data, cp_sensor_tx_data,
					result,cp_test_pass);
	if (ret) {
		tp_log_err("%s:validate cp test result fail, ret = %d\n", __func__, ret);
		seq_printf(result_file, "5F-");
		goto software_reason_exit;
	} else {
		tp_log_info("%s:validate cp test result succ\n", __func__);
	}

	result->cp_sensor_rx_data = cp_sensor_rx_data;
	result->cp_sensor_tx_data = cp_sensor_tx_data;
	result->cp_sensor_rx_average = cp_sensor_rx_average;
	result->cp_sensor_tx_average = cp_sensor_tx_average;
	result->cp_sensor_rx_raw_data = NULL;
	result->cp_sensor_tx_raw_data = NULL;

	result->tx_num = tx_num;
	result->rx_num = rx_num;
	result->cm_test_run = run_cm_test;
	result->cp_test_run = run_cp_test;
	result->button_num = 0;
	result->test_summary = result->cm_test_pass && result->cp_test_pass;

	ret = result_save(result_file, configuration, result); // gen5 ttda

	if (ret)
		tp_log_err("%s:Fail saving result\n", __func__);

	ret = do_calibration_tdda(dev, false);
	if (ret) {
		tp_log_err("%s:calibration fail\n", __func__);
		seq_printf(result_file, "1F-");
		goto software_reason_exit;
	}
	goto exit;

software_reason_exit:
	seq_printf(result_file, "software_reason\n");
exit:
	CY_MEM_FREE(configuration);
	CY_MEM_FREE(result);
	CY_MEM_FREE(cm_sensor_data);
	CY_MEM_FREE(cm_sensor_column_delta);
	CY_MEM_FREE(cm_sensor_row_delta);
	CY_MEM_FREE(cp_sensor_rx_delta);
	CY_MEM_FREE(cp_sensor_tx_delta);
	CY_MEM_FREE(cp_sensor_rx_calibration);
	CY_MEM_FREE(cp_sensor_tx_calibration);
	CY_MEM_FREE(cp_sensor_rx_data);
	CY_MEM_FREE(cp_sensor_tx_data);
	CY_MEM_FREE(cm_gradient_col);
	CY_MEM_FREE(cm_gradient_row);
	tp_log_info("%s: test end\n", __func__);
	return 0;
}

