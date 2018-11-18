#include "ilitek_ts.h"

struct kobject *android_touch_kobj = NULL;
struct kobject *glove_kobj = NULL;
void ilitek_touch_node_deinit(void);
void ilitek_touch_node_init(void);
extern struct i2c_data i2c;
extern char firmware_version_for_roi[50];
extern void ilitek_reset(int);
static int i2c_connect = 1;
static int short_test_result = 0;
static int open_test_result = 0;
static int allnode_test_result = 0;
static int Tx_Rx_delta_test_result = 0;
static int p2p_noise_test_result = 0;
static int change_rate_test_result = 0;
#define RAW_DATA_SIZE PAGE_SIZE
static int ilitek_ready_into_test(void) {
	int ret = 0;
	uint8_t cmd[2] = {0};
	uint8_t buf[32] = {0};
	struct i2c_msg msgs_cmd[] = {
		{.addr = i2c.client->addr, .flags = 0, .len = 2, .buf = cmd,},
	};
	struct i2c_msg msgs_ret[] = {
		{.addr = i2c.client->addr, .flags = I2C_M_RD, .len = 3, .buf = buf,}
	};

	cmd[0] = 0x10;
	msgs_cmd[0].len = 1;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		i2c_connect = 0;
		tp_log_err("%s:%d err, ret %d\n", __func__,__LINE__,ret);
		return ret;
	}
	msgs_ret[0].len = 3;
	ret = ilitek_i2c_transfer(i2c.client, msgs_ret, 1);
	if(ret < 0){
		tp_log_err("%s, i2c read error, ret %d\n", __func__, ret);
		return ret;
	}
	if (buf[1] < 0x80) {
		tp_log_info("ilitek_ready_into_test IC is not ready\n");
	}
	cmd[0] = 0xF6;
	cmd[1] = 0x13;
	msgs_cmd[0].len = 2;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		tp_log_err("%s:%d err, ret %d\n", __func__,__LINE__,ret);
		return ret;
	}

	cmd[0] = 0x13;
	msgs_cmd[0].len = 1;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		tp_log_err("%s:%d err, ret %d\n", __func__,__LINE__,ret);
		return ret;
	}
	msgs_ret[0].len = 2;
	ret = ilitek_i2c_transfer(i2c.client, msgs_ret, 1);
	if(ret < 0){
		tp_log_err("%s:%d err, ret %d\n", __func__,__LINE__,ret);
		return ret;
	}
	return 0;
}

static int ilitek_into_testmode(void) {
	int ret = 0;
	uint8_t cmd[2] = {0};
	struct i2c_msg msgs_cmd[] = {
		{.addr = i2c.client->addr, .flags = 0, .len = 2, .buf = cmd,},
	};
	cmd[0] = 0xF0;
	cmd[1] = 0x01;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		tp_log_err("%s,  err, ret %d\n", __func__, ret);
		return ret;
	}
	return 0;
}
#if 1
int ilitek_into_glove_mode(bool status) {
	int ret = 0;
	uint8_t cmd[2] = {0};
	struct i2c_msg msgs_cmd[] = {
		{.addr = i2c.client->addr, .flags = 0, .len = 2, .buf = cmd,},
	};
	tp_log_info("%s, status = %d\n", __func__, status);
	cmd[0] = 0x06;
	if (status) {
		cmd[1] = 0x01;
	}
	else {
		cmd[1] = 0x00;
	}
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		tp_log_info("%s,  err, ret %d\n", __func__, ret);
		return ret;
	}
	return 0;
}
#endif
#if 1
int ilitek_into_hall_halfmode(bool status) {
	int ret = 0;
	uint8_t cmd[12] = {0};
	struct i2c_msg msgs_cmd[] = {
		{.addr = i2c.client->addr, .flags = 0, .len = 2, .buf = cmd,},
	};
	tp_log_info("ilitek_into_hall_halfmode status = %d\n", status);
	if (status) {
		msgs_cmd[0].len = 9;
		cmd[0] = 0x0d;
		cmd[1] = i2c.hall_x0 & 0xFF;
		cmd[2] = (i2c.hall_x0 >> 8) & 0xFF;
		cmd[3] = i2c.hall_y0 & 0xFF;
		cmd[4] = (i2c.hall_y0 >> 8) & 0xFF;
		cmd[5] = i2c.hall_x1 & 0xFF;
		cmd[6] = (i2c.hall_x1 >> 8) & 0xFF;
		cmd[7] = i2c.hall_y1 & 0xFF;
		cmd[8] = (i2c.hall_y1 >> 8) & 0xFF;
		tp_log_info("ilitek_into_hall_halfmode 0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x\n", cmd[1], cmd[2], cmd[3], cmd[4], cmd[5], cmd[6], cmd[7], cmd[8]);
		ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
		if(ret < 0){
			tp_log_err("%s,  set cover x, y err, ret %d\n", __func__, ret);
			return ret;
		}
		msgs_cmd[0].len = 2;
		cmd[0] = 0x0C;
		cmd[1] = 0x00;
		ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
		if(ret < 0){
			tp_log_err("%s,  err, ret %d\n", __func__, ret);
			return ret;
		}
	}
	else {
		cmd[0] = 0x0C;
		cmd[1] = 0x01;
		ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
		if(ret < 0){
			tp_log_err("%s,  err, ret %d\n", __func__, ret);
			return ret;
		}
	}
	return 0;
}
#endif

#if 1
int ilitek_into_fingersense_mode(bool status) {
	int ret = 0;
	uint8_t cmd[2] = {0};
	struct i2c_msg msgs_cmd[] = {
		{.addr = i2c.client->addr, .flags = 0, .len = 2, .buf = cmd,},
	};
	tp_log_info("%s, status = %d\n", __func__, status);
	cmd[0] = 0x0F;
	if (status) {
		cmd[1] = 0x01;
	}
	else {
		cmd[1] = 0x00;
	}
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		tp_log_info("%s,  err, ret %d\n", __func__, ret);
		return ret;
	}
	return 0;
}
#endif

static int ilitek_gesture_disable_sense_start(void) {
	int ret = 0;
	uint8_t cmd[2] = {0};
	struct i2c_msg msgs_cmd[] = {
		{.addr = i2c.client->addr, .flags = 0, .len = 2, .buf = cmd,},
	};

	cmd[0] = 0x0A;
	cmd[1] = 0x00;
	msgs_cmd[0].len = 2;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	mdelay(10);
	cmd[0] = 0x01;
	cmd[1] = 0x01;
	msgs_cmd[0].len = 2;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	mdelay(10);
	return 0;
}

static int ilitek_into_normalmode(void) {
	int ret = 0;
	uint8_t cmd[2] = {0};
	struct i2c_msg msgs_cmd[] = {
		{.addr = i2c.client->addr, .flags = 0, .len = 2, .buf = cmd,},
	};
	cmd[0] = 0xF0;
	cmd[1] = 0x00;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		tp_log_err("%s,  err, ret %d\n", __func__, ret);
		return ret;
	}
	return 0;
}

static int ilitek_short_test(int *short_data1, int *short_data2) {
	int ret = 0, newMaxSize = 32, i = 0, j = 0, index = 0;
	uint8_t cmd[4] = {0};
	uint8_t * buf_recv = kzalloc(sizeof(uint8_t) * i2c.y_ch * i2c.x_ch * 2 + 32, GFP_KERNEL);

	struct i2c_msg msgs_cmd[] = {
		{.addr = i2c.client->addr, .flags = 0, .len = 2, .buf = cmd,},
	};
	struct i2c_msg msgs_ret[] = {
		{.addr = i2c.client->addr, .flags = I2C_M_RD, .len = 3, .buf = buf_recv,}
	};

	int test_32 = (i2c.x_ch * 2) / (newMaxSize - 2);
	if ((i2c.x_ch * 2) % (newMaxSize - 2) != 0) {
		test_32 += 1;
	}

	tp_log_info("ilitek_short_test test_32 = %d\n", test_32);

	short_test_result = 0;
	cmd[0] = 0xF1;
	cmd[1] = 0x04;
	cmd[2] = 0x00;
	msgs_cmd[0].len = 3;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		tp_log_err("%s:%d err, ret %d\n", __func__,__LINE__,ret);
	}

	msleep(1);
	for (i =0; i < 300; i++ ) {
		ret = ilitek_poll_int();
		tp_log_info("%s:ilitek interrupt status = %d\n", __func__,ret);
		if (ret == 1) {
			break;
		}
		else {
			msleep(5);
		}
	}

	cmd[0] = 0xF6;
	cmd[1] = 0xF2;
	msgs_cmd[0].len = 2;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		tp_log_err("%s:%d err, ret %d\n", __func__,__LINE__,ret);
	}

	cmd[0] = 0xF2;
	msgs_cmd[0].len = 1;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		tp_log_err("%s:%d err, ret %d\n", __func__,__LINE__,ret);
	}

	for(i = 0; i < test_32; i++){
		if ((i2c.x_ch * 2)%(newMaxSize - 2) != 0 && i == test_32 - 1) {
			msgs_ret[0].len = (i2c.x_ch * 2)%(newMaxSize - 2) + 2;
			msgs_ret[0].buf = buf_recv + newMaxSize*i;
			ret = ilitek_i2c_transfer(i2c.client, msgs_ret, 1);
			if(ret < 0){
				tp_log_err("%s:%d err,2c read error ret %d\n", __func__,__LINE__,ret);
			}
		}
		else {
			msgs_ret[0].len = newMaxSize;
			msgs_ret[0].buf = buf_recv + newMaxSize*i;
			ret = ilitek_i2c_transfer(i2c.client, msgs_ret, 1);
			if(ret < 0){
				tp_log_err("%s:%d err,2c read error ret %d\n", __func__,__LINE__,ret);
			}
		}
	}
	j = 0;
	for(i = 0; i < test_32; i++) {
		if (j == i2c.x_ch * 2) {
			break;
		}
		for(index = 2; index < newMaxSize; index++) {
			if (j < i2c.x_ch) {
				short_data1[j] = buf_recv[i * newMaxSize + index];
			}
			else {
				short_data2[j - i2c.x_ch] = buf_recv[i * newMaxSize + index];
			}
			j++;
			if (j == i2c.x_ch * 2) {
				break;
			}
		}
	}

	for (i = 0; i < i2c.x_ch; i++) {
		if (abs(short_data1[i] - short_data2[i]) > i2c.ilitek_short_threshold) {
			short_test_result = -1;
		}
	}
	if (buf_recv) {
		kfree(buf_recv);
		buf_recv = NULL;
	}
	return short_test_result;
}

static int ilitek_open_test(int *open_data) {
	int ret = 0, newMaxSize = 32, i = 0, j = 0, index = 0;
	uint8_t cmd[4] = {0};
	uint8_t * buf_recv = kzalloc(sizeof(uint8_t) * i2c.y_ch * i2c.x_ch * 2 + 32, GFP_KERNEL);

	struct i2c_msg msgs_cmd[] = {
		{.addr = i2c.client->addr, .flags = 0, .len = 2, .buf = cmd,},
	};
	struct i2c_msg msgs_ret[] = {
		{.addr = i2c.client->addr, .flags = I2C_M_RD, .len = 3, .buf = buf_recv,}
	};

	int test_32 = (i2c.y_ch * i2c.x_ch * 2) / (newMaxSize - 2);
	if ((i2c.y_ch * i2c.x_ch * 2) % (newMaxSize - 2) != 0) {
		test_32 += 1;
	}
	open_test_result = 0;

	cmd[0] = 0xF1;
	cmd[1] = 0x06;
	cmd[2] = 0x00;
	msgs_cmd[0].len = 3;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		tp_log_err("%s:%d err, ret %d\n", __func__,__LINE__,ret);
	}

	msleep(1);
	for (i =0; i < 300; i++ ) {
		ret = ilitek_poll_int();
		tp_log_info("%s:ilitek interrupt status = %d\n", __func__,ret);
		if (ret == 1) {
			break;
		}
		else {
			msleep(5);
		}
	}

	cmd[0] = 0xF6;
	cmd[1] = 0xF2;
	msgs_cmd[0].len = 2;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		tp_log_err("%s:%d err, ret %d\n", __func__,__LINE__,ret);
	}

	cmd[0] = 0xF2;
	msgs_cmd[0].len = 1;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		tp_log_err("%s:%d err, ret %d\n", __func__,__LINE__,ret);
	}

	tp_log_info("ilitek_open_test test_32 = %d\n", test_32);
	for(i = 0; i < test_32; i++){
		if ((i2c.y_ch * i2c.x_ch * 2)%(newMaxSize - 2) != 0 && i == test_32 - 1) {
			msgs_ret[0].len = (i2c.y_ch * i2c.x_ch * 2)%(newMaxSize - 2) + 2;
			msgs_ret[0].buf = buf_recv + newMaxSize*i;
			ret = ilitek_i2c_transfer(i2c.client, msgs_ret, 1);
			if(ret < 0){
				tp_log_err("%s:%d err,i2c read error ret %d\n", __func__,__LINE__,ret);
			}
		}
		else {
			msgs_ret[0].len = newMaxSize;
			msgs_ret[0].buf = buf_recv + newMaxSize*i;
			ret = ilitek_i2c_transfer(i2c.client, msgs_ret, 1);
			if(ret < 0){
				tp_log_err("%s:%d err,i2c read error ret %d\n", __func__,__LINE__,ret);
			}
		}
	}
	j = 0;
	for(i = 0; i < test_32; i++) {
		if (j == i2c.y_ch * i2c.x_ch) {
			break;
		}
		for(index = 2; index < newMaxSize; index += 2) {
			open_data[j] = ((buf_recv[i * newMaxSize + index + 1] << 8) + buf_recv[i * newMaxSize + index]);
			if (((buf_recv[i * newMaxSize + index + 1] << 8) + buf_recv[i * newMaxSize + index]) < i2c.ilitek_open_threshold) {
				open_test_result = -1;
			}
			j++;
			if(j % i2c.x_ch == 0) {
			}
			if (j == i2c.y_ch * i2c.x_ch) {
				break;
			}
		}
	}

	if (buf_recv) {
		kfree(buf_recv);
		buf_recv = NULL;
	}
	return open_test_result;
}

static int ilitek_Tx_Rx_test(int *allnode_data, int *allnode_Tx_delta_data, int *allnode_Rx_delta_data) {
	int i = 0;
	int j = 0;
	int index = 0;
	tp_log_info("ilitek allnode Tx delta\n");
#if 0
	printk("ilitek_tx_cap_max\n");
	for (i = 0; i < (i2c.y_ch - 1); i++) {
		for (j = 0; j < i2c.x_ch; j++) {
			printk("%d,", i2c.ilitek_tx_cap_max[index]);
			index++;
		}
		printk("\n");
	}

	index = 0;
	printk("ilitek_rx_cap_max\n");
	for (i = 0; i < i2c.y_ch; i++) {
		for (j = 0; j < (i2c.x_ch - 1); j++) {
			printk("%d,", i2c.ilitek_rx_cap_max[index]);
			index++;
		}
		printk("\n");
	}
#endif
	Tx_Rx_delta_test_result = 0;
	index = 0;
	for (i = 0; i < (i2c.y_ch - 1); i++) {
		for (j = 0; j < i2c.x_ch; j++) {
			allnode_Tx_delta_data[index] = abs(allnode_data[i * i2c.x_ch + j] - allnode_data[(i + 1) * i2c.x_ch + j]);
			if (allnode_Tx_delta_data[index] > i2c.ilitek_tx_cap_max[index]) {
				tp_log_err("%s:%d err,Tx_Rx_delta_test_result error allnode_Tx_delta_data[%d] = %d,  i2c.ilitek_tx_cap_max[%d] = %d\n",
					__func__,__LINE__, index , allnode_Tx_delta_data[index], index, i2c.ilitek_tx_cap_max[index]);
				Tx_Rx_delta_test_result = -1;
			}
			index++;
		}
	}

	index = 0;
	for (i = 0; i < i2c.y_ch; i++) {
		for (j = 0; j < (i2c.x_ch - 1); j++) {
			allnode_Rx_delta_data[index] = abs(allnode_data[i * i2c.x_ch + j + 1] - allnode_data[i * i2c.x_ch + j]);
			if (allnode_Rx_delta_data[index] > i2c.ilitek_rx_cap_max[index]) {
				tp_log_err("%s:%d err,Tx_Rx_delta_test_result error allnode_Rx_delta_data[%d] = %d,i2c.ilitek_rx_cap_max[%d] = %d\n",
					__func__,__LINE__, index , allnode_Rx_delta_data[index], index, i2c.ilitek_rx_cap_max[index]);
				Tx_Rx_delta_test_result = -1;
			}
			index++;
		}
	}
	return Tx_Rx_delta_test_result;
}

static int ilitek_p2p_test(int *allnode_p2p_data) {
	int ret = 0, newMaxSize = 32, i = 0, j = 0, index = 0;
	int frame_count = 0;
	uint8_t cmd[4] = {0};
	uint8_t * buf_recv = kzalloc(sizeof(uint8_t) * i2c.y_ch * i2c.x_ch * 2 + 32, GFP_KERNEL);
	int * allnode_data_min = kzalloc(sizeof(int) *( i2c.y_ch * i2c.x_ch + 32), GFP_KERNEL);
	int * allnode_data_max = kzalloc(sizeof(int) *( i2c.y_ch * i2c.x_ch + 32), GFP_KERNEL);
	struct i2c_msg msgs_cmd[] = {
		{.addr = i2c.client->addr, .flags = 0, .len = 2, .buf = cmd,},
	};
	struct i2c_msg msgs_ret[] = {
		{.addr = i2c.client->addr, .flags = I2C_M_RD, .len = 3, .buf = buf_recv,}
	};

	int test_32 = (i2c.y_ch * i2c.x_ch * 2) / (newMaxSize - 2);
	if ((i2c.y_ch * i2c.x_ch * 2) % (newMaxSize - 2) != 0) {
		test_32 += 1;
	}
	tp_log_info("ilitek_p2p_test\n");

	p2p_noise_test_result = 0;
	if(NULL == allnode_data_min||NULL == allnode_data_max || NULL == buf_recv) {
		tp_log_err("%s, invalid parameter NULL\n", __func__);
		return -ENOMEM;
	}

	for (i = 0; i < i2c.y_ch * i2c.x_ch; i++) {
		allnode_data_min[i] = 36255;
		allnode_data_max[i] = 0;
	}
	for (frame_count = 0; frame_count < 10; frame_count++) {
		j = 0;
		cmd[0] = 0xF1;
		cmd[1] = 0x03;
		cmd[2] = 0x00;
		msgs_cmd[0].len = 3;
		ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
		if(ret < 0){
			tp_log_err("%s,	err, ret %d\n", __func__, ret);
		}
		msleep(1);
		for (i =0; i < 300; i++ ) {
			ret = ilitek_poll_int();
			tp_log_info("%s ilitek interrupt status = %d frame_count  = %d\n", __func__, ret, frame_count);
			if (ret == 1) {
				break;
			}
			else {
				msleep(5);
			}
		}
		cmd[0] = 0xF6;
		cmd[1] = 0xF2;
		msgs_cmd[0].len = 2;
		ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
		if(ret < 0){
			tp_log_err("%s,	err, ret %d\n", __func__, ret);
		}
		cmd[0] = 0xF2;
		msgs_cmd[0].len = 1;
		ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
		if(ret < 0){
			tp_log_err("%s,	err, ret %d\n", __func__, ret);
		}
		for(i = 0; i < test_32; i++){
			if ((i2c.y_ch * i2c.x_ch * 2)%(newMaxSize - 2) != 0 && i == test_32 - 1) {
				msgs_ret[0].len = (i2c.y_ch * i2c.x_ch * 2)%(newMaxSize - 2) + 2;
				msgs_ret[0].buf = buf_recv + newMaxSize*i;
				ret = ilitek_i2c_transfer(i2c.client, msgs_ret, 1);
				if(ret < 0){
					tp_log_err("%s, i2c read error, ret %d\n", __func__, ret);
				}
			}
			else {
				msgs_ret[0].len = newMaxSize;
				msgs_ret[0].buf = buf_recv + newMaxSize*i;
				ret = ilitek_i2c_transfer(i2c.client, msgs_ret, 1);
				if(ret < 0){
					tp_log_err("%s, i2c read error, ret %d\n", __func__, ret);
				}
			}
		}
		for(i = 0; i < test_32; i++) {
			if (j == i2c.y_ch * i2c.x_ch) {
				break;
			}
			for(index = 2; index < newMaxSize; index += 2) {
				if (allnode_data_max[j] < ((buf_recv[i * newMaxSize + index + 1] << 8) + buf_recv[i * newMaxSize + index])) {
					allnode_data_max[j] = ((buf_recv[i * newMaxSize + index + 1] << 8) + buf_recv[i * newMaxSize + index]);
				}
				if (allnode_data_min[j] > ((buf_recv[i * newMaxSize + index + 1] << 8) + buf_recv[i * newMaxSize + index])) {
					allnode_data_min[j] = ((buf_recv[i * newMaxSize + index + 1] << 8) + buf_recv[i * newMaxSize + index]);
				}
				j++;
				if (j == i2c.y_ch * i2c.x_ch) {
					break;
				}
			}
		}
	}

	index = 0;
	for (i = 0; i < i2c.y_ch * i2c.x_ch; i++) {
		allnode_p2p_data[index] = allnode_data_max[i] - allnode_data_min[i];
		if (allnode_p2p_data[index] > i2c.ilitek_deltarawimage_max) {
		tp_log_err("%s:%d err,p2p_noise_test_result error allnode_p2p_data[%d] = %d,i2c.ilitek_deltarawimage_max=%d \n",
			__func__,__LINE__, index , allnode_p2p_data[index], i2c.ilitek_deltarawimage_max);
			p2p_noise_test_result = -1;
		}
		index++;
	}

	if (allnode_data_min) {
		kfree(allnode_data_min);
		allnode_data_min = NULL;
	}
	if (allnode_data_max) {
		kfree(allnode_data_max);
		allnode_data_max = NULL;
	}
	if (buf_recv) {
		kfree(buf_recv);
		buf_recv = NULL;
	}
	return p2p_noise_test_result;
}

static int ilitek_allnode_test(int *allnode_data) {
	int ret = 0, newMaxSize = 32, i = 0, j = 0, index = 0;
	uint8_t cmd[4] = {0};
	uint8_t * buf_recv = kzalloc(sizeof(uint8_t) * i2c.y_ch * i2c.x_allnode_ch * 2 + 32, GFP_KERNEL);
	struct i2c_msg msgs_cmd[] = {
		{.addr = i2c.client->addr, .flags = 0, .len = 2, .buf = cmd,},
	};
	struct i2c_msg msgs_ret[] = {
		{.addr = i2c.client->addr, .flags = I2C_M_RD, .len = 3, .buf = buf_recv,}
	};

	int test_32 = (i2c.y_ch * i2c.x_ch * 2) / (newMaxSize - 2);
	if ((i2c.y_ch * i2c.x_ch * 2) % (newMaxSize - 2) != 0) {
		test_32 += 1;
	}

	if(NULL == buf_recv) {
		tp_log_err("%s, invalid parameter NULL\n", __func__);
		return -ENOMEM;
	}

	tp_log_info("ilitek_allnode_test test_32 = %d\n", test_32);
#if 0
	printk("ilitek_full_raw_max_cap\n");
	for (i = 0; i < i2c.y_ch; i++) {
		for (j = 0; j < i2c.x_ch; j++) {
			printk("%d,", i2c.ilitek_full_raw_max_cap[index]);
			index++;
		}
		printk("\n");
	}
	printk("ilitek_full_raw_min_cap\n");
	index = 0;
	for (i = 0; i < i2c.y_ch; i++) {
		for (j = 0; j < i2c.x_ch; j++) {
			printk("%d,", i2c.ilitek_full_raw_min_cap[index]);
			index++;
		}
		printk("\n");
	}
#endif
	allnode_test_result = 0;
	cmd[0] = 0xF1;
	cmd[1] = 0x05;
	msgs_cmd[0].len = 2;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		tp_log_err("%s:%d err,i2c  error ret %d\n", __func__,__LINE__,ret);
	}
	msleep(1);

	for (i =0; i < 300; i++ ) {
		ret = ilitek_poll_int();
		tp_log_info("%s:ilitek interrupt status = %d\n", __func__,ret);
		if (ret == 1) {
			break;
		}
		else {
			msleep(5);
		}
	}

	cmd[0] = 0xF6;
	cmd[1] = 0xF2;
	msgs_cmd[0].len = 2;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		tp_log_err("%s:%d err,i2c  error ret %d\n", __func__,__LINE__,ret);
	}

	cmd[0] = 0xF2;
	msgs_cmd[0].len = 1;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		tp_log_err("%s:%d err,i2c  error ret %d\n", __func__,__LINE__,ret);
	}

	for(i = 0; i < test_32; i++){
		if ((i2c.y_ch * i2c.x_ch * 2)%(newMaxSize - 2) != 0 && i == test_32 - 1) {
			msgs_ret[0].len = (i2c.y_ch * i2c.x_ch * 2)%(newMaxSize - 2) + 2;
			msgs_ret[0].buf = buf_recv + newMaxSize*i;
			ret = ilitek_i2c_transfer(i2c.client, msgs_ret, 1);
			if(ret < 0){
				tp_log_err("%s:%d err,i2c read error ret %d\n", __func__,__LINE__,ret);
			}
		}
		else {
			msgs_ret[0].len = newMaxSize;
			msgs_ret[0].buf = buf_recv + newMaxSize*i;
			ret = ilitek_i2c_transfer(i2c.client, msgs_ret, 1);
			if(ret < 0){
				tp_log_err("%s:%d err,i2c read error ret %d\n", __func__,__LINE__,ret);
			}
		}
	}
	j = 0;
	for(i = 0; i < test_32; i++) {
		if (j == i2c.y_ch * i2c.x_ch) {
			break;
		}
		for(index = 2; index < newMaxSize; index += 2) {
			allnode_data[j] = ((buf_recv[i * newMaxSize + index + 1] << 8) + buf_recv[i * newMaxSize + index]);
			if ( (allnode_data[j] < i2c.ilitek_full_raw_min_cap[j]) ||(allnode_data[j] > i2c.ilitek_full_raw_max_cap[j]) ) {
				tp_log_err("%s:%d err,allnode_test_result error allnode_data[%d] = %d, i2c.ilitek_full_raw_min_cap[%d] = %d i2c.ilitek_full_raw_max_cap[%d] = %d\n",
					__func__,__LINE__, j , allnode_data[j], j, i2c.ilitek_full_raw_min_cap[j], j, i2c.ilitek_full_raw_max_cap[j]);
				allnode_test_result = -1;
			}
			j++;

			if (j == i2c.y_ch * i2c.x_ch) {
				break;
			}
		}
	}

	if (buf_recv) {
		kfree(buf_recv);
		buf_recv = NULL;
	}

	return allnode_test_result;
}

static int ilitek_change_report_rate_test(void)
{
	uint8_t cmd[4] = {0};
	uint8_t buf[4] = {0};
	int ret = 0;
	int i = 0;
	struct i2c_msg msgs_cmd[] = {
		{.addr = i2c.client->addr, .flags = 0, .len = 2, .buf = cmd,},
	};
	struct i2c_msg msgs_ret[] = {
		{.addr = i2c.client->addr, .flags = I2C_M_RD, .len = 1, .buf = buf,}
	};
	cmd[0] = SENSOR_TEST_SET_CDC_INITIAL;
	cmd[1] = SENSOR_TEST_TRCRQ_TRCST_TESET;
	cmd[2] = SENSOR_TEST_COMMAND;
	msgs_cmd[0].len = 3;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		tp_log_err("%s:%d i2c transfer error ret=%d\n", __func__,__LINE__,ret);
	}

	msleep(1);
	for (i =0; i < POLLINT_INT_TIMES; i++ ) {
		ret = ilitek_poll_int();
		tp_log_info("%s:ilitek interrupt status = %d\n", __func__,ret);
		if (ret == 1) {
			break;
		}
		else {
			msleep(5);
		}
	}
	cmd[0] = SENSOR_TEST_TEAD_DATA_SELECT_CONTROL;
	cmd[1] = SENSOR_TEST_GET_CDC_RAW_DATA;
	msgs_cmd[0].len = 2;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		tp_log_err("%s:%d i2c transfer error ret=%d\n", __func__,__LINE__,ret);
	}

	cmd[0] = SENSOR_TEST_GET_CDC_RAW_DATA;
	msgs_cmd[0].len = 1;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		tp_log_err("%s:%d i2c transfer error ret=%d\n", __func__,__LINE__,ret);
	}
	msgs_ret[0].len = 1;
	ret = ilitek_i2c_transfer(i2c.client, msgs_ret, 1);
	if(ret < 0){
		tp_log_err("%s:%d i2c transfer error ret=%d\n", __func__,__LINE__,ret);
	}
	tp_log_info("%s:change_rate_test_result status = 0x%x\n", __func__,buf[0]);

	if (buf[0] == CHANGE_REPORT_RATE_RESULT_SUCCESS) {
		change_rate_test_result = CHANGE_REPORT_RATE_SUCCESS;
	}
	else if (buf[0] == CHANGE_REPORT_RATE_RESULT_60HZ_FAIL) {
		change_rate_test_result = CHANGE_REPORT_RATE_60HZ_FAIL;
	}
	else if (buf[0] == CHANGE_REPORT_RATE_RESULT_120HZ_FAIL) {
		change_rate_test_result = CHANGE_REPORT_RATE_120HZ_FAIL;
	}
	else {
		change_rate_test_result = CHANGE_REPORT_RATE_FAIL;
	}
	return change_rate_test_result;
}

 int ilitek_rawdata_proc_show(struct seq_file *m, void *v)
{
	int i = 0;
	int j = 0;
	int ret = -1;
	unsigned char buf[64] = {0};

	int * short_data1 = kzalloc(sizeof(int) * (i2c.x_ch + 32), GFP_KERNEL);
	int * short_data2 = kzalloc(sizeof(int) * (i2c.x_ch + 32), GFP_KERNEL);
	int * open_data = kzalloc(sizeof(int) *( i2c.y_ch * i2c.x_ch + 32), GFP_KERNEL);
	int * allnode_data = kzalloc(sizeof(int) * (i2c.y_ch * i2c.x_ch + 32), GFP_KERNEL);
	int * allnode_p2p_data = kzalloc(sizeof(int) * (i2c.y_ch * i2c.x_ch + 32), GFP_KERNEL);
	int * allnode_Tx_delta_data = kzalloc(sizeof(int) *( i2c.y_ch * i2c.x_ch + 32), GFP_KERNEL);
	int * allnode_Rx_delta_data = kzalloc(sizeof(int) *( i2c.y_ch * i2c.x_ch + 32), GFP_KERNEL);

	if(NULL == short_data1||NULL == short_data2||NULL == open_data
		||NULL == allnode_data || NULL == allnode_p2p_data
		||NULL == allnode_Tx_delta_data||NULL == allnode_Rx_delta_data){
		tp_log_err("%s, invalid parameter NULL\n", __func__);
		return -ENOMEM;
	}

	if (m->size <= RAW_DATA_SIZE) {
		m->count = m->size;
		return 0;
	}
	i = 0;
	j = 0;
	disable_irq(i2c.client->irq);
	ret = ilitek_ready_into_test();
	if (ret){
		tp_log_err("%s, i2c transfer error, ret = %d\n", __func__, ret);
		seq_printf(m, "%s", "0F-");
		ret = ilitek_into_normalmode();
		ilitek_reset(i2c.reset_gpio);
		enable_irq(i2c.client->irq);
		return ret;
	}

	ret = ilitek_into_testmode();
	mdelay(30);
	ilitek_gesture_disable_sense_start();
	ret = ilitek_allnode_test(allnode_data);
	if (ret){
		tp_log_err("%s, allnode test failed, ret = %d\n", __func__, ret);
		seq_printf(m, "%s", "0P-1F-");
	}
	else{
		seq_printf(m, "%s", "0P-1P-");
	}

	ret = ilitek_Tx_Rx_test(allnode_data, allnode_Tx_delta_data, allnode_Rx_delta_data);
	if (ret){
		tp_log_err("%s, TRX test failed, ret = %d\n", __func__, ret);
		seq_printf(m, "%s", "2F-");
	}
	else{
		seq_printf(m, "%s", "2P-");
	}

	ret = ilitek_p2p_test(allnode_p2p_data);
	if (ret){
		tp_log_err("%s, noise test failed, ret = %d\n", __func__, ret);
		seq_printf(m, "%s", "3F-");
	}
	else{
		seq_printf(m, "%s", "3P-");
	}

	ret = ilitek_open_test(open_data);
	if (ret){
		tp_log_err("%s, open test failed, ret = %d\n", __func__, ret);
		seq_printf(m, "%s", "4F-");
	}
	else{
		seq_printf(m, "%s", "4P-");
	}

	ret = ilitek_short_test(short_data1, short_data2);
	if (ret){
		tp_log_err("%s, short test failed, ret = %d\n", __func__, ret);
		seq_printf(m, "%s", "5F-");
	}
	else{
		seq_printf(m, "%s", "5P-");
	}

	ret = ilitek_change_report_rate_test();
	if (ret) {
		tp_log_err("%s, change report rate test failed, ret = %d\n", __func__, ret);
		seq_printf(m, "%s", "8F-");
	}
	else{
		seq_printf(m, "%s", "8P-");
	}

	ret = ilitek_into_normalmode();
	ilitek_reset(i2c.reset_gpio);
	for (ret = 0; ret < 30; ret++ ) {
		j = ilitek_poll_int();
		tp_log_info("ilitek int status = %d\n", j);
		if (j == 0) {
			break;
		}
		else {
			msleep(5);
		}
	}
	if (ret >= 30) {
		tp_log_err("ilitek reset but int not pull low\n");
	}
	buf[0] = 0x10;
	ilitek_i2c_write_and_read(i2c.client, buf, 1, 0, buf, 3);
	tp_log_info("%s, 0x10 cmd read data :%X %X %X\n", __func__, buf[0], buf[1], buf[2]);
	enable_irq(i2c.client->irq);

	seq_printf(m, "%s", "\n");
	seq_printf(m, "%s", "RawImageData:\n");
	for (j = 0; j < i2c.y_ch * i2c.x_ch; j++) {
		seq_printf(m, "%d,", allnode_data[j]);
		i++;
		if((i) % i2c.x_ch == 0) {
			seq_printf(m, "%s", "\n");
		}
	}

	i = 0;
	seq_printf(m, "%s", "Tx To Tx\n");
	for (j = 0; j <( i2c.y_ch - 1) * i2c.x_ch; j++) {
		seq_printf(m, "%d,", allnode_Tx_delta_data[j]);
		i++;
		if((i) % i2c.x_ch == 0) {
			seq_printf(m, "%s", "\n");
		}
	}

	i = 0;
	seq_printf(m, "%s", "Rx To Rx\n");
	for (j = 0; j < i2c.y_ch *( i2c.x_ch - 1); j++) {
		seq_printf(m, "%d,", allnode_Rx_delta_data[j]);
		i++;
		if((i) % ( i2c.x_ch - 1) == 0) {
			seq_printf(m, "%s", "\n");
		}
	}

	i = 0;
	seq_printf(m, "%s", "rawdata noise\n");
	for (j = 0; j < i2c.y_ch * i2c.x_ch; j++) {
		seq_printf(m, "%d,", allnode_p2p_data[j]);
		i++;
		if((i) % i2c.x_ch == 0) {
			seq_printf(m, "%s", "\n");
		}
	}

	seq_printf(m, "%s", "short test\n");
	for (j = 0; j < i2c.x_ch; j++) {
		seq_printf(m, "%d,", short_data1[j]);
	}
	seq_printf(m, "%s", "\n");
	for (j = 0; j < i2c.x_ch; j++) {
		seq_printf(m, "%d,", short_data2[j]);
	}
	seq_printf(m, "%s", "\n");

	if (short_data1) {
		kfree(short_data1);
		short_data1 = NULL;
	}
	if (short_data2) {
		kfree(short_data2);
		short_data2 = NULL;
	}
	if (open_data) {
		kfree(open_data);
		open_data = NULL;
	}
	if (allnode_data) {
		kfree(allnode_data);
		allnode_data = NULL;
	}
	if (allnode_p2p_data) {
		kfree(allnode_p2p_data);
		allnode_p2p_data = NULL;
	}
	if (allnode_Rx_delta_data) {
		kfree(allnode_Rx_delta_data);
		allnode_Rx_delta_data = NULL;
	}
	if (allnode_Tx_delta_data) {
		kfree(allnode_Tx_delta_data);
		allnode_Tx_delta_data = NULL;
	}
	return 0;
}

 int ilitek_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, ilitek_rawdata_proc_show, NULL);
}

 const struct file_operations ilitek_proc_fops = {
	.open = ilitek_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
static ssize_t hw_touch_test_type_show(struct kobject *dev,
		struct kobj_attribute *attr, char *buf)
{

	return snprintf(buf, PAGE_SIZE - 1,
		"%s:%s\n",
		NORMALIZE_TP_CAPACITANCE_TEST,
		TP_JUDGE_SYNAPTICS_RESULT);
}

static struct kobj_attribute hw_touch_test_type = {
	.attr = {.name = "tp_capacitance_test_type", .mode = 0444},
	.show = hw_touch_test_type_show,
	.store = NULL,
};
#if 1
static ssize_t ts_roi_enable_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
	int error = 0;
	int value = 0;
	tp_log_info("%s:", __func__);

	if (i2c.firmware_updating){
		tp_log_err("%s: tp fw is updating,return\n", __func__);
		return -EINVAL;
	}
	error = sscanf(buf, "%d", &value);
	if (value < 0) {
		tp_log_err("%s: sscanf value %d is invalid\n", __func__, value);
		return -EINVAL;
	}
	if (error <= 0) {
		tp_log_err("sscanf return invalid :%d\n", error);
		error = -EINVAL;
		goto out;
	}

	if (value) {
		ilitek_into_fingersense_mode(true);
	}
	else {
		ilitek_into_fingersense_mode(false);
	}
	i2c.ilitek_roi_enabled = value;
	tp_log_debug("sscanf value is %u\n", i2c.ilitek_roi_enabled);

out:
	tp_log_debug("roi_enable_store done\n");
	return count;
}

static ssize_t ts_roi_enable_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return snprintf(buf, sizeof(i2c.ilitek_roi_enabled), "%d\n", i2c.ilitek_roi_enabled);
}


static ssize_t ts_roi_data_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	if (i2c.firmware_updating) {
		tp_log_err("%s: tp fw is updating,return\n", __func__);
		return 0;
	}
	mutex_lock(&(i2c.roi_mutex));
	memcpy(buf, i2c.ilitek_roi_data, ROI_DATA_LENGTH);
	mutex_unlock(&(i2c.roi_mutex));
	return ROI_DATA_LENGTH;
}

static ssize_t ts_roi_data_debug_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int cnt = 0;
	int count = 0;
	int i, j = 0;
	short roi_data_16[ROI_DATA_LENGTH / 2] = { 0 };
	unsigned char *roi_data_p = NULL;

	if (i2c.firmware_updating) {
		tp_log_err("%s: tp fw is updating,return\n", __func__);
		return 0;
	}

	mutex_lock(&(i2c.roi_mutex));
	roi_data_p = i2c.ilitek_roi_data;
	mutex_unlock(&(i2c.roi_mutex));

	for (i = 0; i < ROI_DATA_LENGTH; i += 2, j++) {
		roi_data_16[j] = roi_data_p[i] | (roi_data_p[i + 1] << 8);
		cnt = snprintf(buf, PAGE_SIZE - count, "%4d", roi_data_16[j]);
		buf += cnt;
		count += cnt;

		if ((j + 1) % 7 == 0) {
			cnt = snprintf(buf, PAGE_SIZE - count, "\n");
			buf += cnt;
			count += cnt;
		}
	}
	snprintf(buf, PAGE_SIZE - count, "\n");
	count++;

	return count;
}

static ssize_t touch_chip_info_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE - 1, "ilitek-CANN-jdi-%s",firmware_version_for_roi);
}

static DEVICE_ATTR(roi_data,S_IRUGO | S_IWUSR | S_IWGRP,ts_roi_data_show,NULL);
static DEVICE_ATTR(roi_data_debug,S_IRUGO | S_IWUSR | S_IWGRP,ts_roi_data_debug_show,NULL);
static DEVICE_ATTR(roi_enable,S_IRUGO | S_IWUSR | S_IWGRP,ts_roi_enable_show,ts_roi_enable_store);
static DEVICE_ATTR(touch_chip_info,S_IRUGO | S_IWUSR | S_IWGRP,touch_chip_info_show,NULL);

static struct attribute *ts_attributes[] = {
	&dev_attr_roi_enable.attr,
	&dev_attr_roi_data.attr,
	&dev_attr_roi_data_debug.attr,
	&dev_attr_touch_chip_info.attr,
	NULL
};
static const struct attribute_group ts_attr_group = {
	.attrs = ts_attributes,
};
#endif
#if 1
static ssize_t hw_glove_func_show(struct kobject *dev,
		struct kobj_attribute *attr, char *buf)
{

	tp_log_info("%s:", __func__);
	return snprintf(buf, sizeof(i2c.glove_status), "%d\n", i2c.glove_status);
}

static ssize_t hw_glove_func_store(struct kobject *dev,
		struct kobj_attribute *attr, const char *buf, size_t size)
{
	int ret = 0;
	tp_log_info("%s:", __func__);
	if (i2c.firmware_updating) {
		tp_log_err("%s: tp fw is updating,return\n", __func__);
		return size;
	}
	ret = kstrtoint(buf, 10, &i2c.glove_status);
	if (ret) {
		tp_log_err("%s kstrtoint error\n", __func__);
		return ret;
	}
	if (0 == i2c.glove_status) {
		ilitek_into_glove_mode(false);
	}
	else if (1 == i2c.glove_status) {
		ilitek_into_glove_mode(true);
	}

	return size;
}
static struct kobj_attribute glove_func = {
	.attr = {.name = "signal_disparity", .mode = (S_IRUGO | S_IWUSR | S_IWGRP)},
	.show = hw_glove_func_show,
	.store = hw_glove_func_store,
};
#endif
#if 1
static ssize_t hw_holster_func_store(struct kobject *dev,
		struct kobj_attribute *attr, const char *buf, size_t size)
{
	int ret = 0;
	if (i2c.firmware_updating) {
		tp_log_err("%s: tp fw is updating,return\n", __func__);
		return -EINVAL;
	}
	ret = sscanf(buf, "%d %d %d %d %d",&i2c.hall_status, &i2c.hall_x0, &i2c.hall_y0, &i2c.hall_x1, &i2c.hall_y1);
	if (!ret) {
		tp_log_err("sscanf return invaild :%d\n", ret);
		ret = -EINVAL;
		goto out;
	}
	tp_log_info("%s:sscanf value is enable=%d, (%d,%d), (%d,%d)\n",__func__, i2c.hall_status, i2c.hall_x0, i2c.hall_y0, i2c.hall_x1, i2c.hall_y1);

	if (i2c.hall_status && ((i2c.hall_x0 < 0) || (i2c.hall_y0 < 0) || (i2c.hall_x1 <= i2c.hall_x0) || (i2c.hall_y1 <= i2c.hall_y0))) {
		tp_log_err("%s:invalid value is %d (%d,%d), (%d,%d)\n",__func__, i2c.hall_status, i2c.hall_x0, i2c.hall_y0, i2c.hall_x1, i2c.hall_y1);
		ret = -EINVAL;
		return ret;
	}
	if (i2c.hall_status) {
		ret = ilitek_into_hall_halfmode(true);
	}
	else {
		ret = ilitek_into_hall_halfmode(false);
	}

out:
	if (ret < 0) {
		return ret;
	}
	return size;
}

static ssize_t hw_holster_func_show(struct kobject *dev,
		struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf,PAGE_SIZE,"%d %d %d %d %d\n", i2c.hall_status, i2c.hall_x0,i2c.hall_y0, i2c.hall_x1, i2c.hall_y1);
}
static struct kobj_attribute holster_func = {
	.attr = {.name = "holster_touch_window", .mode = (S_IRUGO | S_IWUSR | S_IWGRP)},
	.show = hw_holster_func_show,
	.store = hw_holster_func_store,
};
#endif

void ilitek_touch_node_init(void)
{
	int ret ;
	struct proc_dir_entry *proc_entry;
#if 1
	struct platform_device *touch_dev;
#endif
	android_touch_kobj = kobject_create_and_add("touchscreen", NULL) ;
	if (android_touch_kobj == NULL) {
		tp_log_err("%s: kobject_create_and_add failed\n", __func__);
		return;
	}

	ret = sysfs_create_file(android_touch_kobj, &hw_touch_test_type.attr);
	if (ret) {
		tp_log_err("%s: ilitek_mmi_test create file error\n", __func__);
	}
#if 1
	ret = sysfs_create_file(android_touch_kobj, &holster_func.attr);
	if (ret){
		tp_log_err("%s: holster_func create file error\n", __func__);
	}
#endif
	proc_entry = proc_mkdir("touchscreen", NULL);
	if (!proc_entry) {
		tp_log_err("%s:%d Error, failed to creat procfs.\n",__func__, __LINE__);
	}

	if (!proc_create("tp_capacitance_data", 0, proc_entry, &ilitek_proc_fops)) {
		tp_log_err("%s:%d Error, failed to creat procfs.\n",__func__, __LINE__);
		remove_proc_entry("tp_capacitance_data", proc_entry);
	}
#if 1
	touch_dev = platform_device_alloc("huawei_touch", -1);
	if (!touch_dev) {
		tp_log_err("touch_dev platform device malloc failed\n");
		return ;
	}

	ret = platform_device_add(touch_dev);
	if (ret) {
		tp_log_err("touch_dev platform device add failed :%d\n", ret);
		platform_device_put(touch_dev);
		return ;
	}

	ret = sysfs_create_group(&touch_dev->dev.kobj, &ts_attr_group);
	if (ret) {
		tp_log_err("can't create ts's sysfs\n");
		return ;
	}
#endif
#if 1
	glove_kobj = kobject_create_and_add("glove_func", android_touch_kobj) ;
	if (glove_kobj == NULL) {
		tp_log_err("%s: kobject_create_and_add touchscreen\n", __func__);
	}

	ret = sysfs_create_file(glove_kobj, &glove_func.attr);
	if (ret){
		tp_log_err("%s: glove_func create file error\n", __func__);
	}
#endif
	return;
}

void ilitek_touch_node_deinit(void)
{
	kobject_del(android_touch_kobj);
}

