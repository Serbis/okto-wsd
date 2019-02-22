/*
 * NRF24.c
 *
 *  Created on: 5 янв. 2019 г.
 *      Author: serbis
 */

#include "include/NRF24.h"

uint32_t a_p1_targ = 0xFFFFFFFF;
uint32_t a_p1_self = 0xAAAAAAA1;

uint32_t a_p2_targ = 0xFFFFFFFF;
uint32_t a_p2_self = 0xAAAAAAA2;

uint32_t a_p3_targ = 0xFFFFFFFF;
uint32_t a_p3_self = 0xAAAAAAA3;

uint32_t a_p4_targ = 0xFFFFFFFF;
uint32_t a_p4_self = 0xAAAAAAA4;

uint32_t a_p5_targ = 0xFFFFFFFF;
uint32_t a_p5_self = 0xAAAAAAA5;


#if defined(HW_PI) || defined(HW_STM32)

#ifdef HW_PI
    #include <stdio.h>
    #include <stdlib.h>
	#include <pthread.h>

	pthread_mutex_t *rf_mutex;
#endif


//------------------------------------------------


//uint8_t TX_ADDRESS[TX_ADR_WIDTH] = { 0xb1, 0xb2, 0xb3, 0xb4 };
uint8_t RX_BUF[TX_PLOAD_WIDTH] = { 0 };


uint8_t nrf_recvb = 0;

//--------------------------------------------------

uint8_t NRF24_ReadReg(uint8_t addr)
{
	uint16_t dt = 0;
    dt |= addr;

	CS_ON;
    spi_transmit(NRF_spid, (uint8_t*) &dt, 2, 1000);
	CS_OFF;
    dt >>= 8;
	return dt;
}

//------------------------------------------------

void NRF24_WriteReg(uint8_t addr, uint8_t dt)
{
    addr |= W_REGISTER;
    uint16_t bf = 0;
    bf |= dt;
    bf <<= 8;
    bf |= addr;

	CS_ON;
    spi_transmit(NRF_spid, (uint8_t*) &bf, 2, 1000);
	CS_OFF;
}

//------------------------------------------------

void NRF24_Read_Buf(uint8_t addr,uint8_t *pBuf,uint8_t bytes)
{
    #ifdef HW_STM32
    uint8_t *bf = (uint8_t*) pmalloc(bytes + 1);
    #endif

    #ifdef HW_PI
    uint8_t *bf = (uint8_t*) malloc(bytes + 1);
    #endif

    bf[0] = addr;

	CS_ON;
    spi_transmit(NRF_spid, bf, bytes + 1, 1000);
	CS_OFF;

    memcpy(pBuf, bf + 1, bytes);

    #ifdef HW_STM32
    pfree(bf);
    #endif

    #ifdef HW_PI
    free(bf);
    #endif
}

//------------------------------------------------

void NRF24_Write_Buf(uint8_t addr,uint8_t *pBuf,uint8_t bytes)
{
	addr |= W_REGISTER;

    #ifdef HW_STM32
    uint8_t *bf = (uint8_t*) pmalloc(bytes + 1);
    #endif

    #ifdef HW_PI
    uint8_t *bf = (uint8_t*) malloc(bytes + 1);
    #endif

    memcpy(bf + 1, pBuf, bytes);
    bf[0] = addr;

	CS_ON;
    spi_transmit(NRF_spid, bf, bytes + 1, 1000);
	CS_OFF;

    #ifdef HW_STM32
    pfree(bf);
    #endif

    #ifdef HW_PI
    free(bf);
    #endif
}

//------------------------------------------------

void NRF24_FlushRX(void)
{
	uint8_t dt = FLUSH_RX;
	CS_ON;
	spi_transmit(NRF_spid, (uint8_t*) &dt, 1, 1000);
	delayMs(1);
	CS_OFF;
}

//------------------------------------------------

void NRF24_FlushTX(void)
{
	uint8_t dt = FLUSH_TX;
	CS_ON;
	spi_transmit(NRF_spid, (uint8_t*) &dt, 1, 1000);
	delayMs(1);
	CS_OFF;
}

//------------------------------------------------

void NRF24_RX_Mode(void)
{
  NRF24_Write_Buf(RX_ADDR_P1, (uint8_t*) &a_p1_targ, TX_ADR_WIDTH);

  uint8_t regval = 0x00;
  regval = NRF24_ReadReg(CONFIG);

  //разбудим модуль и переведём его в режим приёмника, включив биты PWR_UP и PRIM_RX
  regval |= (1 << PWR_UP) | (1 << PRIM_RX);

  NRF24_WriteReg(CONFIG, regval);
  CE_SET;
  delayMs(150); //Задержка минимум 130 мкс

  NRF24_FlushRX();
  NRF24_FlushTX();
}

//------------------------------------------------

void NRF24_TX_Mode(uint8_t pipe)
{
  uint32_t self = 0;
  if (pipe == 1) self = a_p1_self;
  else if (pipe == 2) self = a_p2_self;
  else if (pipe == 3) self = a_p3_self;
  else if (pipe == 4) self = a_p4_self;
  else self = a_p5_self;

  NRF24_Write_Buf(TX_ADDR, (uint8_t*) &self, TX_ADR_WIDTH);
  NRF24_Write_Buf(RX_ADDR_P0, (uint8_t*) &self, TX_ADR_WIDTH);
  //NRF24_Write_Buf(TX_ADDR, TX_ADDRESS, TX_ADR_WIDTH);
  CE_RESET;
  NRF24_FlushRX();
  NRF24_FlushTX();
}

//------------------------------------------------

void NRF24_Transmit(uint8_t addr,uint8_t *pBuf,uint8_t bytes)
{
  CE_RESET;
  NRF24_Write_Buf(addr, pBuf, bytes);
  CE_SET;
}

//------------------------------------------------

uint8_t NRF24_Send(uint8_t *pBuf, uint8_t pipe)
{
  uint8_t status=0x00, regval=0x00;

  pthread_mutex_lock(rf_mutex);
  NRF24_TX_Mode(pipe);


  regval = NRF24_ReadReg(CONFIG);

  //если модуль ушел в спящий режим, то разбудим его, включив бит PWR_UP и выключив PRIM_RX
  regval |= (1 << PWR_UP);
  regval &= ~(1 << PRIM_RX);
  NRF24_WriteReg(CONFIG, regval);
  delayMs(150); //Задержка минимум 130 мкс

  //Отправим данные в воздух
  NRF24_Transmit(WR_TX_PLOAD, pBuf, TX_PLOAD_WIDTH);
  CE_SET;
  delayMs(15); //minimum 10us high pulse (Page 21)
  CE_RESET;

  while(read_irq_pin() == 1) {}

  status = NRF24_ReadReg(STATUS);
  if(status & TX_DS) { //tx_ds == 0x20
      NRF24_WriteReg(STATUS, 0x20);
  } else if(status&MAX_RT) {
    NRF24_WriteReg(STATUS, 0x10);
    NRF24_FlushTX();
  }

  regval = NRF24_ReadReg(OBSERVE_TX);



  NRF24_RX_Mode();
  pthread_mutex_unlock(rf_mutex);

  return regval;
}

//------------------------------------------------

int8_t NRF24_GetPipeByAddress(uint32_t addr) {
	if (a_p1_targ == addr)
		return 1;
	if (a_p2_targ == addr)
			return 2;
	if (a_p3_targ == addr)
			return 3;
	if (a_p4_targ == addr)
			return 4;
	if (a_p5_targ == addr)
			return 5;

	return -1;
}

//------------------------------------------------

uint32_t NRF24_GetAddrByPipe(uint8_t pipe) {
	if (pipe == 1)
		return a_p1_targ;
	if (pipe == 2)
		return a_p2_targ;
	if (pipe == 3)
		return a_p3_targ;
	if (pipe == 4)
		return a_p4_targ;
	if (pipe == 5)
		return a_p5_targ;

	return 0;
}

//------------------------------------------------

bool NRF24_available() {
	 uint8_t fs = NRF24_ReadReg(FIFO_STATUS);
	 if (!(fs & _BV(0)))
		 return true;

	 return false;
}

//------------------------------------------------

int NRF24_Receive(uint8_t *payload, uint32_t *addr)
{
	uint8_t status = NRF24_ReadReg(STATUS);
	NRF24_Read_Buf(RD_RX_PLOAD, RX_BUF, TX_PLOAD_WIDTH);
	NRF24_WriteReg(STATUS, status | 0x70);
	memcpy(payload, RX_BUF, TX_PLOAD_WIDTH);

	uint8_t pipe = status << 4;
	pipe = pipe >> 5;
	*addr = NRF24_GetAddrByPipe(pipe);

	return TX_PLOAD_WIDTH;
}

//------------------------------------------------

void NRF24_init()
{
	rf_mutex = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(rf_mutex, NULL);

	CE_RESET;
    delayMs(5000);
	NRF24_WriteReg(CONFIG, 0x0a);
	delayMs(5000);
	NRF24_WriteReg(EN_AA, 0x3F); // Enable all pipes
	NRF24_WriteReg(EN_RXADDR, 0x3F); // Enable all pipes
	NRF24_WriteReg(SETUP_AW, 0x02); // Setup address width=4 bytes
	NRF24_WriteReg(SETUP_RETR, 0x5F); // 1500us, 15 retrans
	NRF24_WriteReg(FEATURE, 0);
	NRF24_WriteReg(DYNPD, 0);
	NRF24_WriteReg(STATUS, 0x70); //Reset flags for IRQ
	NRF24_WriteReg(RF_CH, 76); // частота 2476 MHz
	NRF24_WriteReg(RF_SETUP, 0x06); //TX_PWR:0dBm, Datarate:1Mbps
	//NRF24_Write_Buf(TX_ADDR, TX_ADDRESS, TX_ADR_WIDTH);
	//NRF24_Write_Buf(RX_ADDR_P0, TX_ADDRESS, TX_ADR_WIDTH);
	NRF24_WriteReg(RX_PW_P0, TX_PLOAD_WIDTH);
	NRF24_WriteReg(RX_PW_P1, TX_PLOAD_WIDTH);
	NRF24_WriteReg(RX_PW_P2, TX_PLOAD_WIDTH);
	NRF24_WriteReg(RX_PW_P3, TX_PLOAD_WIDTH);
	NRF24_WriteReg(RX_PW_P4, TX_PLOAD_WIDTH);
	NRF24_WriteReg(RX_PW_P5, TX_PLOAD_WIDTH);
	/*NRF24_Write_Buf(RX_ADDR_P1, (uint8_t*) &a_p1_targ, TX_ADR_WIDTH);
	NRF24_Write_Buf(RX_ADDR_P2, (uint8_t*) &a_p2_targ, TX_ADR_WIDTH);
	NRF24_Write_Buf(RX_ADDR_P3, (uint8_t*) &a_p3_targ, TX_ADR_WIDTH);
	NRF24_Write_Buf(RX_ADDR_P4, (uint8_t*) &a_p4_targ, TX_ADR_WIDTH);
	NRF24_Write_Buf(RX_ADDR_P5, (uint8_t*) &a_p5_targ, TX_ADR_WIDTH);*/

	NRF24_RX_Mode();

	uint8_t value = NRF24_ReadReg(CONFIG);
	printf("CONFIG=0x%02X\n", value);
	fflush(stdout);
}

uint8_t NRF24_init_check()
{
	/*uint8_t value = 0;

    #ifdef HW_STM32
	uint8_t *pBuf = (uint8_t*) pmalloc(TX_ADR_WIDTH);
    #endif

    #ifdef HW_PI
	uint8_t *pBuf = (uint8_t*) malloc(TX_ADR_WIDTH);
    #endif

    pBuf[0] = 0;
    pBuf[1] = 0;
    pBuf[2] = 0;

    value = NRF24_ReadReg(CONFIG);
    #ifdef HW_PI_DEBUG
    printf("CONFIG=0x%02X\n", value);
    #else
    if (value != 0x0b) return 1; //Because RX mode is up un up
    #endif

	value = NRF24_ReadReg(EN_AA);
    #ifdef HW_PI_DEBUG
    printf("EN_AA=0x%02X\n", value);
    #else
    if (value != 0x02) return 2;
    #endif

	value = NRF24_ReadReg(EN_RXADDR);
    #ifdef HW_PI_DEBUG
    printf("EN_RXADDR=0x%02X\n", value);
    #else
    if (value != 0x02) return 3;
    #endif

	value = NRF24_ReadReg(SETUP_AW);
    #ifdef HW_PI_DEBUG
    printf("SETUP_AW=0x%02X\n", value);
    #else
    if (value != 0x01) return 4;
    #endif

    value = NRF24_ReadReg(SETUP_RETR);
    #ifdef HW_PI_DEBUG
    printf("SETUP_RETR=0x%02X\n", value);
    #else
    if (value != 0x5f) return 6;
    #endif

    value = NRF24_ReadReg(FEATURE);
    #ifdef HW_PI_DEBUG
    printf("FEATURE=0x%02X\n", value);
    #else
    if (value != 0x00) return 7;
    #endif

    value = NRF24_ReadReg(DYNPD);
    #ifdef HW_PI_DEBUG
    printf("DYNPD=0x%02X\n", value);
    #else
    if (value != 0x00) return 8;
    #endif*/

   /* value = NRF24_ReadReg(STATUS);
    #ifdef HW_PI_DEBUG
    printf("STATUS=0x%02X\n", value);
    #else
    if (value != 0x70) return 9;
    #endif*/

    /*value = NRF24_ReadReg(RF_CH);
    #ifdef HW_PI_DEBUG
    printf("RF_CH=0x%d\n", value);
    #else
    if (value != 76) return 10;
    #endif

    value = NRF24_ReadReg(RF_SETUP);
    #ifdef HW_PI_DEBUG
    printf("RF_SETUP=0x%02X\n", value);
    #else
    if (value != 0x06) return 11;
    #endif


    NRF24_Read_Buf(TX_ADDR, pBuf, TX_ADR_WIDTH);
    if (pBuf[0] == TX_ADDRESS[0] && pBuf[1] == TX_ADDRESS[1] && pBuf[2] == TX_ADDRESS[2]) {
        #ifdef HW_PI_DEBUG
        printf("TX_ADDR=EQ\n");
        #endif
    } else {
        #ifdef HW_PI_DEBUG
        printf("TX_ADDR=NOT_EQ\n");
        #else
        #ifdef HW_PI
        free(pBuf);
        #endif
        return 12;
        #endif
    }

    NRF24_Read_Buf(RX_ADDR_P1, pBuf, TX_ADR_WIDTH);
    if (pBuf[0] == TX_ADDRESS[0] && pBuf[1] == TX_ADDRESS[1] && pBuf[2] == TX_ADDRESS[2]) {
        #ifdef HW_PI_DEBUG
        printf("RX_ADDR_P1=EQ\n");
        #endif
    } else {
        #ifdef HW_PI_DEBUG
        printf("RX_ADDR_P1=NOT_EQ\n");
        #else
        #ifdef HW_PI
        free(pBuf);
        #endif
        return 13;
        #endif
    }

    value = NRF24_ReadReg(RX_PW_P1);
    #ifdef HW_PI_DEBUG
    printf("RX_PW_P1=0x%02X\n", value);
    #else
    if (value != TX_PLOAD_WIDTH) return 14;
    #endif

	#ifdef HW_PI_DEBUG
	free(pBuf);
	#endif
	#ifdef HW_STM32
	pfree(pBuf);
	#endif*/

	return 0;
}
#endif
