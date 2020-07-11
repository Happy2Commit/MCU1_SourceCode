

/*
 * PB14 -->SPI2_MISO
 * PB15 -->SPI2_MOSI
 * PB13 -->SPI2_SCLK
 * PB12 -->SPI2_NSS
 * ALT FUNC MODE -->5
 */
#include <string.h>
#include "stm32f407xx.h"

//command codes
#define COMMAND_LED_CTRL			0x50
#define COMMAND_SENSOR_READ			0x51
#define COMMAND_LED_READ			0x52
#define COMMAND_PRINT				0x53
#define COMMAND_ID_READ				0x54

//feedback code
#define NACK 0xA5
#define ACK 0xF5

#define LED_ON     1
#define LED_OFF    0

//arduino analog pins
#define ANALOG_PIN0   0
#define ANALOG_PIN1   1
#define ANALOG_PIN2   2
#define ANALOG_PIN3   3
#define ANALOG_PIN4   4

//arduino led
#define LED_PIN		9
void SPI2_GPIOInits(void)
{
	GPIO_Handle_t SPIPins;
	SPIPins.pGPIOx = GPIOB;
	SPIPins.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_ALTFN;
	SPIPins.GPIO_PinConfig.GPIO_PinAltFunMode = AF5;
	SPIPins.GPIO_PinConfig.GPIO_PinOPType = GPIO_OP_TYPE_PP;
	SPIPins.GPIO_PinConfig.GPIO_PinPuPdControl = GPIO_PIN_PU;
	SPIPins.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_FAST;

	//SCLK
	SPIPins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_13;
	GPIO_Init(&SPIPins);

	//MISO
	SPIPins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_14;
	GPIO_Init(&SPIPins);

	//MOSI
	SPIPins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_15;
	GPIO_Init(&SPIPins);

	//NSS
	SPIPins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_12;
	GPIO_Init(&SPIPins);
}

void SPI2_Inits(void)
{
	SPI_Handle_t SPI2handle;
	SPI2handle.pSPIx = SPI2;
	SPI2handle.SPIConfig.SPI_DeviceMode = SPI_DEVICE_MODE_MASTER;
	SPI2handle.SPIConfig.SPI_SclkSpeed = SPI_SCLK_SPEED_DIV8;
	SPI2handle.SPIConfig.SPI_BusConfig = SPI_BUS_CONFIG_FD;
	SPI2handle.SPIConfig.SPI_DFF = SPI_DFF_8BITS;
	SPI2handle.SPIConfig.SPI_CPOL = SPI_CPOL_LOW;
	SPI2handle.SPIConfig.SPI_CPHA = SPI_CPHA_LOW;
	SPI2handle.SPIConfig.SPI_SSM = SPI_SSM_DI;//Hardware slave management

	SPI_Init(&SPI2handle);
}

void GPIO_ButtonInit()
{
	GPIO_Handle_t GPIOUSR;	//USR button which is connected to PA0

	GPIOUSR.pGPIOx = GPIOA;
	GPIOUSR.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_0;
	GPIOUSR.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_IN;
	GPIOUSR.GPIO_PinConfig.GPIO_PinPuPdControl = GPIO_NO_PUPD;
	GPIOUSR.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_FAST;

	GPIO_Init(&GPIOUSR);
}

uint8_t SPI_Verifyresponse(uint8_t feedback)
{
	if (feedback == ACK)
	{
		//ack
		return 1;
	}
	return 0;
}
void delay (void)
{
	for (uint32_t i = 0; i<500000/2;i++);
}
int main(void)
{
	uint8_t dummy_write = 0xff;
	uint8_t dummy_read;
	uint8_t count = 2;

	GPIO_ButtonInit();//initialising user button
	SPI2_GPIOInits();//initialising GPIO pins to behave as SPI2
	SPI2_Inits();//initialising SPI peripheral parameters

	/*
	 * making SSOE 1 enables NSS output (pulls it to zero) when SPE is set (Peripheral enabled)
	 * ie when SPE =1 , NSS Pulled to low and when SPE = 0 (Peripheral disabled) then NSSS pulled to high
	 */
	SPI_SSOEConfig(SPI2, ENABLE);

	while(1)
	{

		while (!GPIO_ReadFromInputPin(GPIOA, GPIO_PIN_NO_0));
		delay();	//to avoid button de-bouncing issues add some delay

		SPI_PeripheralControl(SPI2, ENABLE);	//enables SPI2 peripheral

		//1. CMD_LED_CTRL	<pin no(1)>		<value(1)>
		uint8_t commandcode = COMMAND_LED_CTRL;
		uint8_t fdbackcode = 0;
		uint8_t args[2];

		//Send commandcode to check if its valid
		SPI_SendData(SPI2, &commandcode, 1);

		/*dummy read - clear off garbage value received due to above transmission. As every transmission causes a reception too.
		This function below resets the RXNE bit*/
		SPI_ReceiveData(SPI2, &dummy_read, 1);

		delay();

		//send some dummy byte to receive ACK or NACK
		SPI_SendData(SPI2, &dummy_write, 1);

		//receive data (ACK or UNACK)
		SPI_ReceiveData(SPI2, &fdbackcode, 1);

		//check if its ACK or NACK
		if (SPI_Verifyresponse(fdbackcode))
		{
			//send arguments - LED PIN NO. and LED On command
			args[0] = LED_PIN;
			if (count%2 == 0)
			{
				args[1] = LED_ON;
			}else
			{
				args[1] = LED_OFF;
			}
			count++;

			SPI_SendData(SPI2, &args[0], 1);
			SPI_ReceiveData(SPI2, &dummy_read, 1);

			delay();
			SPI_SendData(SPI2, &args[1], 1);

		}


		while(SPI_GetFlagStatus(SPI2, SPI_BUSY_FLAG));

		SPI_PeripheralControl(SPI2, DISABLE);	//disable the peripheral

	}

	return 0;
}

