/*
 * hx_drv_lsm9ds1.c
 *
 *  Created on: 2020/05/14
 *      Author: 903730
 */

#include <stdio.h>
#include <string.h>
#include "embARC.h"
#include "embARC_debug.h"

#include "apexextensions.h"
#include "hx_drv_lsm9ds1.h"
#include "board_config.h"
#include "hx_drv_timer.h"

#define HX_IMU_IIC_M_ID         SS_IIC_0_ID     /* ToDo: board_config.h   #define BOARD_IMU_SENSOR_IIC_ID     USE_SS_IIC_0 */
#define HX_IMU_I2C_ADDR_BYTE    1               /**< I2C Master Register address length*/
#define HX_IMU_I2C_DATA_BYTE    1               /**< I2C Master Register value length*/

#define HX_IMU_I2C_RETRY_TIME  3  /**< If I2C Master set fail, maximum retry time for imu setting*/

/** I2C Device Address 7 bit format **/
#define LSM9DS1_IMU_I2C_ADDR    0x6A            // 7 bit format
//#define LSM9DS1_IMU_I2C_ADDR    0x6B            // 7 bit format
#define LSM9DS1_IMU_ID          0x68            // Device Identification (Who am I)
#define LSM9DS1_WHO_AM_I        0x0F
#define LSM9DS1_CTRL_REG1_G     0x10
#define LSM9DS1_STATUS_REG      0x17
#define LSM9DS1_OUT_X_G         0x18
#define LSM9DS1_CTRL_REG6_XL    0x20
#define LSM9DS1_CTRL_REG8       0x22
#define LSM9DS1_OUT_X_XL        0x28
#define LSM9DS1_FIFO_SRC        0x2F

// magnetometer
#define LSM9DS1_MAG_I2C_ADDR    0x1C            // 7 bit format
//#define LSM9DS1_MAG_I2C_ADDR    0x1E            // 7 bit format
#define LSM9DS1_MAG_ID          0x3D            // Device Identification (Who am I)
#define LSM9DS1_CTRL_REG1_M     0x20
#define LSM9DS1_CTRL_REG2_M     0x21
#define LSM9DS1_CTRL_REG3_M     0x22
#define LSM9DS1_STATUS_REG_M    0x27
#define LSM9DS1_OUT_X_L_M       0x28

uint8_t continuousMode = false;

HX_DRV_IMU_ERROR_E hx_drv_imu_set_reg(uint8_t slv_addr, uint8_t addr, uint8_t val)
{
    uint8_t regAddr[HX_IMU_I2C_ADDR_BYTE]  = {addr};
    uint8_t wBuffer[HX_IMU_I2C_DATA_BYTE] = {val};
    int32_t retI2C = 0;

    for(int i = 0;i<HX_IMU_I2C_RETRY_TIME;i++) {
      retI2C = hx_drv_i2cm_write_data(HX_IMU_IIC_M_ID, slv_addr, regAddr, HX_IMU_I2C_ADDR_BYTE, wBuffer, HX_IMU_I2C_DATA_BYTE);
      if(retI2C == E_OK)
        return HX_DRV_IMU_PASS;
    }

    return HX_DRV_IMU_ERROR;
}

HX_DRV_IMU_ERROR_E hx_drv_imu_get_reg(uint8_t slv_addr, uint8_t addr, uint8_t *val)
{
    uint8_t regAddr[HX_IMU_I2C_ADDR_BYTE]  = {addr};
    uint8_t rBuffer[HX_IMU_I2C_DATA_BYTE] = {0x00};
    int32_t retI2C = 0;

    *val = 0;

    retI2C = hx_drv_i2cm_writeread(HX_IMU_IIC_M_ID, slv_addr, regAddr, HX_IMU_I2C_ADDR_BYTE, rBuffer, HX_IMU_I2C_DATA_BYTE);
    if(retI2C < E_OK)
    {
        //EMBARC_PRINTF("hx_drv_imu_get_reg(0x%x, 0x%x) fail \n\n", slv_addr, addr);
        return HX_DRV_IMU_ERROR;
    }

   	*val = rBuffer[0];

    return HX_DRV_IMU_PASS;
}

void hx_drv_imu_setContinuousMode()
{
    // Enable FIFO 
    hx_drv_imu_set_reg(LSM9DS1_IMU_I2C_ADDR, 0x23, 0x02);
    
    // Set continuous mode
    hx_drv_imu_set_reg(LSM9DS1_IMU_I2C_ADDR, 0x2E, 0xC0);

    continuousMode = true;
}

void hx_drv_imu_setOneShotMode()
{
    // Disable FIFO 
    hx_drv_imu_set_reg(LSM9DS1_IMU_I2C_ADDR, 0x23, 0x00);

    // Disable continuous mode
    hx_drv_imu_set_reg(LSM9DS1_IMU_I2C_ADDR, 0x2E, 0x00);
    continuousMode = false;
}

float hx_drv_imu_accelerationSampleRate()
{
    return 119.0F;
}

uint8_t hx_drv_imu_accelerationAvailableCount()
{
    if (continuousMode)
    {
        // continuousMode: Enable FIFO
        // Read FIFO_SRC. If any of the rightmost 8 bits have a value, there is data.

        uint8_t un_read_num = 0;
        if(hx_drv_imu_get_reg(LSM9DS1_IMU_I2C_ADDR, LSM9DS1_FIFO_SRC, &un_read_num) == HX_DRV_IMU_ERROR)
        {
        	 //EMBARC_PRINTF("hx_drv_imu_accelerationAvailable - get data error\n");
            return 0;
        }
        else
        {
        	//EMBARC_PRINTF("FIFO_SRC:0x%x, un_read_num:%d \n", un_read_num, (un_read_num & 0x3F));

        	un_read_num = un_read_num & 0x3F;

        	if(un_read_num!=0)
            {
                //EMBARC_PRINTF("FIFO_SRC:0x%x, un_read_num:%d \n", un_read_num, (un_read_num & 0x3F));
                return un_read_num;
            }
            else
            {
            	//EMBARC_PRINTF("hx_drv_imu_accelerationAvailable - no data\n");
            	return 0;
            }

        }
    }

    return 0;
}

HX_DRV_IMU_ERROR_E hx_drv_imu_accelerationAvailable()
{
    if (continuousMode)
    {
        // continuousMode: Enable FIFO
        // Read FIFO_SRC. If any of the rightmost 8 bits have a value, there is data.

        uint8_t un_read_num = 0;
        if(hx_drv_imu_get_reg(LSM9DS1_IMU_I2C_ADDR, LSM9DS1_FIFO_SRC, &un_read_num) == HX_DRV_IMU_ERROR)
        {
        	 //EMBARC_PRINTF("hx_drv_imu_accelerationAvailable - get data error\n");
            return HX_DRV_IMU_ERROR;
        }
        else
        {
        	//EMBARC_PRINTF("FIFO_SRC:0x%x, un_read_num:%d \n", un_read_num, (un_read_num & 0x3F));

        	un_read_num = un_read_num & 0x3F;

        	if(un_read_num!=0)
            {
                //EMBARC_PRINTF("FIFO_SRC:0x%x, un_read_num:%d \n", un_read_num, (un_read_num & 0x3F));
                return HX_DRV_IMU_MORE_DATA;
            }
            else
            {
            	//EMBARC_PRINTF("hx_drv_imu_accelerationAvailable - no data\n");
            	return HX_DRV_IMU_NO_DATA;
            }

        }
    }
    else
    {
        // OneShotMode: Disable FIFO

        uint8_t valid = 0;
        if(hx_drv_imu_get_reg(LSM9DS1_IMU_I2C_ADDR, LSM9DS1_STATUS_REG, &valid) == HX_DRV_IMU_ERROR)
        {
            return HX_DRV_IMU_ERROR;
        }
        else
        {
            if(valid & 0x01)
            {
                //EMBARC_PRINTF("accelerationAvailable \n");
                return HX_DRV_IMU_MORE_DATA;
            }
            else
            {
            	//EMBARC_PRINTF("one shot mode - no data \n");
            	return HX_DRV_IMU_NO_DATA;
            }

        }
    }

    return HX_DRV_IMU_PASS;
}


HX_DRV_IMU_ERROR_E hx_drv_imu_initial()
{
    uint8_t dev_id = 0xFF;

    // reset
    hx_drv_imu_set_reg(LSM9DS1_IMU_I2C_ADDR, LSM9DS1_CTRL_REG8, 0x05);
    hx_drv_imu_set_reg(LSM9DS1_MAG_I2C_ADDR, LSM9DS1_CTRL_REG2_M, 0x0c);

    board_delay_cycle(10* BOARD_SYS_TIMER_MS_CONV);

    if(hx_drv_imu_get_reg(LSM9DS1_IMU_I2C_ADDR, LSM9DS1_WHO_AM_I, &dev_id) == HX_DRV_IMU_ERROR)
    {
        return HX_DRV_IMU_ERROR;
    }
    else
    {
        if(dev_id == LSM9DS1_IMU_ID)
        {
            //EMBARC_PRINTF("IMU_DEV_ID = 0x%x (PASS) \n", LSM9DS1_IMU_ID);
        }
        else
        {
            //EMBARC_PRINTF("IMU_DEV_ID = 0x%x (FAIL) \n", LSM9DS1_IMU_ID);
            return HX_DRV_IMU_ERROR;
        }
    }

    if(hx_drv_imu_get_reg(LSM9DS1_MAG_I2C_ADDR, LSM9DS1_WHO_AM_I, &dev_id) == HX_DRV_IMU_ERROR)
    {
        return HX_DRV_IMU_ERROR;
    }
    else
    {
        if(dev_id == LSM9DS1_MAG_ID)
        {
            //EMBARC_PRINTF("MAG_DEV_ID = 0x%x (PASS) \n", LSM9DS1_MAG_ID);
        }
        else
        {
            //EMBARC_PRINTF("MAG_DEV_ID = 0x%x (FAIL) \n", LSM9DS1_MAG_ID);
            return HX_DRV_IMU_ERROR;
        }
    }

    hx_drv_imu_set_reg(LSM9DS1_IMU_I2C_ADDR, LSM9DS1_CTRL_REG1_G, 0x78);    // 119 Hz, 2000 dps, 16 Hz BW
    hx_drv_imu_set_reg(LSM9DS1_IMU_I2C_ADDR, LSM9DS1_CTRL_REG6_XL, 0x70);   // 119 Hz, 4G

    hx_drv_imu_set_reg(LSM9DS1_MAG_I2C_ADDR, LSM9DS1_CTRL_REG1_M, 0xb4);    // Temperature compensation enable, medium performance, 20 Hz
    hx_drv_imu_set_reg(LSM9DS1_MAG_I2C_ADDR, LSM9DS1_CTRL_REG2_M, 0x00);    // 4 Gauss
    hx_drv_imu_set_reg(LSM9DS1_MAG_I2C_ADDR, LSM9DS1_CTRL_REG3_M, 0x00);    // Continuous conversion mode

    hx_drv_imu_setContinuousMode();
    hx_drv_imu_accelerationSampleRate();

    return HX_DRV_IMU_PASS;
}

HX_DRV_IMU_ERROR_E hx_drv_imu_receive(float *x, float *y, float *z)
{
    uint8_t regAddr[HX_IMU_I2C_ADDR_BYTE]  = {LSM9DS1_OUT_X_XL};
    uint8_t rBuffer[6] = {0x00};
    int32_t retI2C = 0;
    int16_t gx_raw, gy_raw, gz_raw;

    retI2C = hx_drv_i2cm_writeread(HX_IMU_IIC_M_ID, LSM9DS1_IMU_I2C_ADDR, regAddr, HX_IMU_I2C_ADDR_BYTE, rBuffer, 6);
    if(retI2C < E_OK)
    {
        //EMBARC_PRINTF("hx_drv_imu_receive fail \n");
        return HX_DRV_IMU_ERROR;
    }
    else
    {
        //EMBARC_PRINTF("hx_drv_imu_receive(%d, %d, %d, %d, %d, %d) \n", rBuffer[0], rBuffer[1], rBuffer[2], rBuffer[3], rBuffer[4], rBuffer[5]);
    }

    //SCALE_4G:
    gx_raw = rBuffer[0] | (rBuffer[1] << 8);
    gy_raw = rBuffer[2] | (rBuffer[3] << 8);
    gz_raw = rBuffer[4] | (rBuffer[5] << 8);

   	//*x = (float)gx_raw * 4.0 / 32768.0;
    //*y = (float)gy_raw * 4.0 / 32768.0;
    //*z = (float)gz_raw * 4.0 / 32768.0;
    *x = (float)gx_raw/8192.0;
    *y = (float)gy_raw/8192.0;
    *z = (float)gz_raw/8192.0;

    //EMBARC_PRINTF("g_raw(%d, %d, %d)\n", gx_raw, gy_raw, gx_raw);

    return HX_DRV_IMU_PASS;
}


