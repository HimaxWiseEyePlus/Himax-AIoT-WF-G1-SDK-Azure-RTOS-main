
/* **** DO NOT EDIT - this file is generated by ARChitect2 ****
 *
 * Description: Header file declaring the compiler extensions for eia components 
 */

#ifndef _io_config_H_
#define _io_config_H_


#define DMAC_DMA0_DONE (20)
#define DMAC_DMA1_DONE (21)
#define DMAC_DMA2_DONE (22)
#define DMAC_DMA3_DONE (23)
#define DMAC_DMA4_DONE (24)
#define DMAC_DMA5_DONE (25)
#define DMAC_DMA6_DONE (26)
#define DMAC_DMA7_DONE (27)
#define DMAC_DMA8_DONE (28)
#define DMAC_DMA9_DONE (29)
#define DMAC_DMA10_DONE (30)
#define DMAC_DMA11_DONE (31)
#define DMAC_DMA12_DONE (32)
#define DMAC_DMA13_DONE (33)
#define DMAC_DMA14_DONE (34)
#define DMAC_DMA15_DONE (35)
#define DMAC_DMA0_ERR (36)
#define DMAC_DMA1_ERR (37)
#define DMAC_DMA2_ERR (38)
#define DMAC_DMA3_ERR (39)
#define DMAC_DMA4_ERR (40)
#define DMAC_DMA5_ERR (41)
#define DMAC_DMA6_ERR (42)
#define DMAC_DMA7_ERR (43)
#define DMAC_DMA8_ERR (44)
#define DMAC_DMA9_ERR (45)
#define DMAC_DMA10_ERR (46)
#define DMAC_DMA11_ERR (47)
#define DMAC_DMA12_ERR (48)
#define DMAC_DMA13_ERR (49)
#define DMAC_DMA14_ERR (50)
#define DMAC_DMA15_ERR (51)
#define DMAC_INT_BASE (DMAC_DMA0_DONE)
#define DMAC_ERR_BASE (DMAC_DMA0_ERR)
#define DMAC_CHAN_CNT (16)
#define DMAC_MEM_CHAN_CNT (16)
#define DMAC_AUX_CHAN_CNT (0)
#define IO_I2C_MST0_PRESENT
#define IO_I2C_MST0_FS (16)
#define IO_I2C_MST0_INT_ERR (19)
#define IO_I2C_MST0_INT_RX_AVAIL (52)
#define IO_I2C_MST0_INT_TX_REQ (53)
#define IO_I2C_MST0_INT_STOP_DET (54)
#define IO_I2C_MST1_PRESENT
#define IO_I2C_MST1_FS (16)
#define IO_I2C_MST1_DMA_TX (0)
#define IO_I2C_MST1_DMA_RX (1)
#define IO_I2C_MST1_INT_ERR (55)
#define IO_I2C_MST1_INT_RX_AVAIL (56)
#define IO_I2C_MST1_INT_TX_REQ (57)
#define IO_I2C_MST1_INT_STOP_DET (58)
#define IO_I2C_MST2_PRESENT
#define IO_I2C_MST2_FS (16)
#define IO_I2C_MST2_INT_ERR (59)
#define IO_I2C_MST2_INT_RX_AVAIL (60)
#define IO_I2C_MST2_INT_TX_REQ (61)
#define IO_I2C_MST2_INT_STOP_DET (62)
#define IO_I2C_SLV0_PRESENT
#define IO_I2C_SLV0_FS (16)
#define IO_I2C_SLV0_DMA_TX (2)
#define IO_I2C_SLV0_DMA_RX (3)
#define IO_I2C_SLV0_INT_ERR (63)
#define IO_I2C_SLV0_INT_RX_AVAIL (64)
#define IO_I2C_SLV0_INT_TX_REQ (65)
#define IO_I2C_SLV0_INT_RD_REQ (66)
#define IO_I2C_SLV0_INT_STOP_DET (67)
#define IO_I2C_SLV0_INT_RESTART_DET (68)
#define IO_PDM_RX0_PRESENT
#define IO_PDM_RX0_CH (1)
#define IO_PDM_RX0_FS (8)
#define IO_PDM_RX0_NS (4)
#define IO_PDM_RX0_DS (2)
#define IO_PDM_RX0_DMA (4)
#define IO_PDM_RX0_INT_RX_AVAIL (69)
#define IO_PDM_RX0_INT_ERR (70)
#define IO_PDM_RX0_INT_CLP (71)
#define IO_SPI_SLV0_PRESENT
#define IO_SPI_SLV0_FS (32)
#define IO_SPI_SLV0_MAX_XFER_SIZE (32)
#define IO_SPI_SLV0_DMA_TX (5)
#define IO_SPI_SLV0_DMA_RX (6)
#define IO_SPI_SLV0_INT_ERR (72)
#define IO_SPI_SLV0_INT_RX_AVAIL (73)
#define IO_SPI_SLV0_INT_TX_REQ (74)
#define IO_SPI_SLV0_INT_IDLE (75)
#define IO_UART0_PRESENT
#define IO_UART0_FS (16)
#define IO_UART0_DMA_TX (7)
#define IO_UART0_DMA_RX (8)
#define IO_UART0_INTR (76)
#define IO_UART1_PRESENT
#define IO_UART1_FS (16)
#define IO_UART1_DMA_TX (9)
#define IO_UART1_DMA_RX (10)
#define IO_UART1_INTR (77)
#define IO_I2S_TX_MST0_DMA (14)
#define IO_I2S_RX_MST0_DMA (15)
#define IO_SPI_MST0_PRESENT
#define IO_SPI_MST0_FS (32)
#define IO_SPI_MST0_DMA_TX (12)
#define IO_SPI_MST0_DMA_RX (13)
#define IO_SPI_MST0_INT (100)

#endif

