/*
 * MPU6050.c
 *
 *  Created on: Jan 2, 2023
 *      Author: Arjanit
 */


#include "MPU6050.h"
#include "main.h"
//extern I2C_HandleTypeDef hi2c

extern UART_HandleTypeDef huart2;


uint32_t Ax = 0;
uint32_t Ay = 0;
uint32_t Az = 0;


void MPU6050_init(mpu6050_t *obj, I2C_HandleTypeDef *hi2c, UART_HandleTypeDef *UART)
{

	obj->_i2c = hi2c;
	obj->_uart = UART;
	obj->interrupt_flag = 0;
	memset(obj->_X_data, 0, sizeof(int32_t));
	memset(obj->_Y_data, 0, sizeof(int32_t));
	memset(obj->_Z_data, 0,sizeof(int32_t));


	mpu6050_power_management(obj);
	MPU6050_SMPL_DIV(obj);

	MPU6050_FIFO_EN_Config(obj, ACCEL_FIFO_EN); //you can enable here gyro temperature also
	MPU6050_FIFO_EN_DATA(obj); // you can enable fifo data



}


//void MPU6050_data_out_ACCEL(uint32_t data_x, uint32_t data_y, uint32_t data_z)
//{
//
//	char msg[80];
////	sprintf(msg,"")
//	sprintf(msg,"data_x: %.2f\n data_y:%.2f\n data_z:%.2f",data_x,data_y,data_z);
//	HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 50);
//
//}



void MPU6050_write(mpu6050_t *obj, uint8_t reg, uint8_t * buf, uint16_t buflen)
{
	uint8_t *payload = (uint8_t *)malloc((buflen+1) * sizeof(uint8_t));
	*payload = reg;

	if(buf != NULL && buflen != 0)
	{
		memcpy(payload+1,buf,buflen);

	}

	HAL_I2C_Master_Transmit(obj->_i2c, MPU6050_ADDRESS << 1, payload, buflen+1, 50);

}



void MPU6050_read(mpu6050_t *obj, uint8_t reg, uint8_t *buf, uint16_t buflen)
{
//	uint8_t data = 0;
	uint8_t reg_address = reg;

	HAL_I2C_Master_Transmit(obj->_i2c, MPU6050_ADDRESS<<1, &reg_address, 1, 50);
	HAL_I2C_Master_Receive(obj->_i2c, (MPU6050_ADDRESS << 1)|1, buf, buflen, 50);

}



//void MPU6050_PWR_Config(mpu6050_t * obj)
//{
//	uint8_t data = 0x00;
//
//	MPU6050_write(obj, PWR_MGMT_1, &data, 1);
//
//}




void MPU6050_SMPL_DIV(mpu6050_t * obj)
{
	uint8_t data = 0x07;

	MPU6050_write(obj, SMPLRT_DIV, &data, 1);

}



void MPU6050_ACCEL_CFG(mpu6050_t * obj,afs_sel_t afs_select)
{
	uint8_t data;

	MPU6050_read(obj, ACCEL_CONFIG, &data, 1);

	data = ((data & 0xE7) | afs_select << 3);

	MPU6050_write(obj, ACCEL_CONFIG, &data, 1);

}

void MPU6050_GYRO_CFG(mpu6050_t * obj,fs_sel_t fs_select)
{
	uint8_t data;
	MPU6050_read(obj, GYRO_CONFIG, &data, 1);
	data=((data & 0xE7) | fs_select << 3);
	MPU6050_write(obj, GYRO_CONFIG, &data, 1);

}



void MPU6050_FIFO_EN_Config(mpu6050_t *obj, fifo_en_t fifo_en)
{

	uint8_t config;
	MPU6050_read(obj, FIFO_EN, &config, 1);
	config = ((config & 0xBF) | 0x01 << fifo_en);// enabling fifo for the accel data. By using the fifo_en check header you can use fifo for temp or gyro.
	MPU6050_write(obj, FIFO_EN, &config, 1);

//	testing point here if i2c does not write to the register.
//	uint8_t test_data;
//
//	MPU6050_read(obj, FIFO_EN, &test_data, 1);


}


uint8_t mpu6050_on_interrupt(mpu6050_t * obj)
{
	obj->interrupt_flag = 1;

}

uint8_t mpu6050_has_interrupt(mpu6050_t *obj)
{
	return obj->interrupt_flag;

}



void MPU6050_FIFO_EN_DATA(mpu6050_t *obj)
{
	uint8_t config;

	MPU6050_read(obj, USER_CTRL, &config, 1);

	config = ((config & 0xBF) | 0x01 << 6);

	MPU6050_write(obj, USER_CTRL, &config, 1);




}

uint8_t mpu6050_interrupt_handler(mpu6050_t *obj)
{
	uint8_t reg[2] = {0x00};

//	MPU6050_read(obj, INT_STATUS, &reg, 2);

	if((reg[0]>>4) & 0x01){
		//fifo overflow
		MPU6050_fifo_reset(obj);
	}
	if((reg[0]>>3) & 0x01){
		// data ready
	}




}

void mpu6050_power_management(mpu6050_t * obj)
{

	//if turn on 1 we will start the sensor

	uint8_t config = 0x00;

	MPU6050_write(obj, PWR_MGMT_1, &config, 1);

}

__weak void print_data( int16_t x_data,int16_t y_data,int16_t z_data){
	UNUSED(x_data);
	UNUSED(y_data);
	UNUSED(z_data);
}

void read_fifo(mpu6050_t * obj)
{
	float Ax,Ay,Az;

	uint8_t config[2];
	uint8_t user_ct = 0x00;

	MPU6050_read(obj, FIFO_COUNTH, &config[0], 1);


	if(config[0] != 0){

	for(uint8_t i =0; i<config[0];i++)
	{
		uint8_t samples[6];
		uint8_t conf;
		MPU6050_read(obj, FIFO_R_W, &samples, 6);
//		obj->_X_data = (int16_t)(samples[0] << 8 | samples[1]);
//		obj->_Y_data = (int16_t)(samples[2] << 8 | samples[3]);
//		obj->_Z_data = (int16_t)(samples[4] << 8 | samples[5]);
		int16_t z_data = (int16_t)(samples[2] << 8 | samples[3]);
		int16_t y_data = (int16_t)(samples[0] << 8 | samples[1]);
		int16_t x_data = (int16_t)(samples[4] << 8 | samples[5]);



		obj->_Z_data[i] = z_data;
		obj->_Y_data[i] = y_data;
		obj->_X_data[i] = x_data;



			Ax = (x_data /16384.0)*1000; // dont use float use int instead
			Ay = (y_data/16384.0)*1000;
			Az = (z_data/16384.0)*1000;

			print_data(Ax, Ay, Az);



	}

	}
	else{
		HAL_UART_Transmit(obj->_uart, (uint8_t*)"FIFO full", strlen("FIFO full"), 100);
	}

}





void MPU6050_READ_ACCEL_DATA(mpu6050_t * obj)
{
	int8_t data[6];

	uint8_t check;
	char msg[80];


//	MPU6050_read(obj, ACCEL_XOUT_H, &data, 6);
	MPU6050_read(obj, WHO_AM_I, &check, 1);

	if(check == 104){


	MPU6050_read(obj,ACCEL_XOUT_H,&data,6);


//	obj->_X_data = (int16_t)(data[0] << 8 | data[1]);
//	obj->_Y_data = (int16_t)(data[2] << 8 | data[3]);
//	obj->_Z_data = (int16_t)(data[4] << 8 | data[5]);
//
//	Ax = (obj->_X_data/16384.0)*1000; // dont use float because the lib will take processing power and flash storage :)
//	Ay = (obj->_Y_data/16384.0)*1000;
//	Az = (obj->_Z_data/16384.0)*1000;

	sprintf(msg,"\nX:%f\nY:%f\nZ:%f\n",Ax,Ay,Az);

//	MPU6050_data_out_ACCEL(Ax,Ay,Az);
	HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), 50);
	HAL_UART_Transmit(&huart2, (uint8_t *)"----------------------------------", strlen("----------------------------------"), 50);
	}
	else{
		HAL_UART_Transmit(&huart2, (uint8_t*)"NO data was given", strlen("NO data was given"), 100);
	}
}


