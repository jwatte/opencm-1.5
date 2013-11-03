/*
 * dxl.c
 *
 *  Created on: 2013. 4. 18.
 *      Author: in2storm
 */

#include "dxl.h"

#ifdef CM9_DEBUG
extern void TxDByteC(uint8 buf);
extern void TxDStringC(char *str);
extern void TxDHex8C(u16 bSentData);
extern void TxDHex16C(u16 wSentData);
extern void TxDHex32C(u32 lSentData);

void PrintBuffer(byte *bpPrintBuffer, byte bLength)
{
	byte bCount;
	if(bLength == 0)
	{
		if(gbpTxBuffer[2] == BROADCAST_ID)
		{
			TxDStringC("\r\n No Data[at Broadcast ID 0xFE]");
		}
		else
		{
			TxDStringC("\r\n No Data(Check ID, Operating Mode, Baud rate)");//TxDString("\r\n No Data(Check ID, Operating Mode, Baud rate)");
		}
	}
	for(bCount = 0; bCount < bLength; bCount++)
	{
		TxDHex8C(bpPrintBuffer[bCount]);
		TxDByteC(' ');
	}
	TxDStringC(" LEN:");//("(LEN:")
	TxDHex8C(bLength);
	TxDStringC("\r\n");
}
#endif

uint32 Dummy(uint32 tmp){
	return tmp;
}

void uDelay(uint32 uTime) {
	uint32 cnt, max;
	static uint32 tmp = 0;

	for( max=0; max < uTime; max++)
	{
		for( cnt=0; cnt < 10 ; cnt++ )
		{
			tmp +=Dummy(cnt);
		}
	}
	//tmpdly = tmp;
}
void nDelay(uint32 nTime) { //100ns
	uint32 cnt, max;
	cnt=0;
	static uint32 tmp = 0;

	for( max=0; max < nTime; max++)
	{
		//for( cnt=0; cnt < 10 ; cnt++ )
		//{
			tmp +=Dummy(cnt);
		//}
	}
	//tmpdly = tmp;
}

uint8 getTxRxStatus(void)
{
	return gbDXLtxrxStatus;
}
uint8 setDxlLibStatRtnLvl(uint8 num)
{
	gbDXLStatusReturnLevel = num;
	return gbDXLStatusReturnLevel;
}
uint8 setDxlLibNumTries(uint8 num)
{
	gbDXLNumberTxRxAttempts = num;
	return gbDXLNumberTxRxAttempts;
}

void clearBuffer256(void){
	gbDXLReadPointer = gbDXLWritePointer = 0;
}
byte checkNewArrive(void){
	if(gbDXLReadPointer != gbDXLWritePointer)
		return 1;
	else
		return 0;
}

void TxByteToDXL(byte bTxData){
	//USART_SendData(USART1,bTxData);
	//while( USART_GetFlagStatus(USART1, USART_FLAG_TC)==RESET );
	USART1->regs->DR = (bTxData & (u16)0x01FF);
		while( (USART1->regs->SR & ((u16)0x0040)) == RESET );
}
byte RxByteFromDXL(void){
	return gbpDXLDataBuffer[gbDXLReadPointer++];
}


/**
 * @brief communicate with dynamixel bus on CM-9XX series. After transmitting packets to the bus, also it receives status feedback packets.
 * @param bID dynamixel ID
 * @param bInst Instruction number, you can find it from dynamixel.h
 * @param bTxParaLen length of packet
 *
 */

byte txrx_Packet(byte bID, byte bInst, byte bTxParaLen){

	gbDXLtxrxStatus = 0;

	byte bTxLen, bRxLenEx, bTryCount;

	gbBusUsed = 1;


	for(bTryCount = 0; bTryCount < gbDXLNumberTxRxAttempts; bTryCount++)
	{
		gbDXLReadPointer = gbDXLWritePointer; //BufferClear050728
		bTxLen = tx_Packet(bID, bInst, bTxParaLen);

		if (bTxLen == (bTxParaLen+4+2))
		{
			gbDXLtxrxStatus = (1<<COMM_TXSUCCESS);
		}

		if(bInst == INST_PING)
		{
			if(bID == BROADCAST_ID)
			{
				gbRxLength = bRxLenEx = 255;
			}
			else
			{
				gbRxLength = bRxLenEx = 6;
			}
		}
		else if(bInst == INST_READ)
		{
			if (gbDXLStatusReturnLevel>0)
				gbRxLength = bRxLenEx = 6+gbpParameter[1];
			else
				gbRxLength = bRxLenEx = 0;
		}
		else if( bID == BROADCAST_ID )
		{
			gbRxLength = bRxLenEx = 0;
			break;
		}
		else
		{
			if (gbDXLStatusReturnLevel>1)
				gbRxLength = bRxLenEx = 6;
			else
				gbRxLength = bRxLenEx = 0;
		}


		if(bRxLenEx)//(gbpValidServo[bID] > 0x81 || bInst != INST_WRITE)) //ValidServo = 0x80|RETURN_LEVEL
		{
			gbRxLength = rx_Packet(bRxLenEx);
			//TxDStringC("gbRxLength = ");TxD_Dec_U8C(gbRxLength);TxDStringC("\r\n");
			//TxDStringC("bRxLenEx = ");TxD_Dec_U8C(bRxLenEx);TxDStringC("\r\n");
			//      if(gbRxLength != bRxLenEx) //&& bRxLenEx != 255) before Ver 1.11e
			if((gbRxLength != bRxLenEx) && (bRxLenEx != 255)) // after Ver 1.11f
			{
				unsigned long ulCounter;
				word wTimeoutCount;
				ulCounter = 0;
				wTimeoutCount = 0;
				//TxDByteC('0');//TxDStringC("\r\n TEST POINT 0");//TxDString("\r\n Err ID:0x");
				while(ulCounter++ < RX_TIMEOUT_COUNT2)
				{
					if(gbDXLReadPointer != gbDXLWritePointer) {
						gbDXLReadPointer = gbDXLWritePointer; //BufferClear
						ulCounter = 0;
						if(wTimeoutCount++ > 100 )
						{
							//uDelay(0);// porting ydh added
							break; //porting ydh 100->245 //;;;;;; min max �ڹٲ� found at Ver 1.11e
						}
						nDelay(NANO_TIME_DELAY);// porting ydh added 20120210.
					}
					//uDelay(0);// porting ydh added
					nDelay(NANO_TIME_DELAY);// porting ydh added

				}
				//TxDByteC('1');//TxDStringC("\r\n TEST POINT 1");//TxDString("\r\n Err ID:0x");
				gbDXLReadPointer = gbDXLWritePointer; //BufferClear
			}
			else
			{
				//TxDByteC('6');//TxDStringC("\r\n TEST POINT 6");//TxDString("\r\n Err ID:0x");
				break;
			}
		}
	}

	//TxDByteC('2');//TxDStringC("\r\n TEST POINT 2");//TxDString("\r\n Err ID:0x");
	gbBusUsed = 0;

	if((gbRxLength != bRxLenEx) && (gbpTxBuffer[2] != BROADCAST_ID))
	{
		//TxDByteC('3');//TxDStringC("\r\n TEST POINT 3");//TxDString("\r\n Err ID:0x");
#ifdef	PRINT_OUT_COMMUNICATION_ERROR_TO_USART2
		//TxDString("\r\n Err ID:0x");
		//TxDHex8(bID);
		//TxDString("\r\n ->[DXL]Err: ");
		PrintBuffer(gbpTxBuffer,bTxLen);
		//TxDString("\r\n <-[DXL]Err: ");
		PrintBuffer(gbpRxBuffer,gbRxLength);
#endif

#ifdef	PRINT_OUT_TRACE_ERROR_PRINT_TO_USART2
		//TxDString("\r\n {[ERROR:");TxD16Hex(0x8100);TxDByte(':');TxD16Hex(bID);TxDByte(':');TxD8Hex(bInst);TxDByte(']');TxDByte('}');
		//TxDByte(bID);TxDByte(' ');
		//TxDByte(bInst);TxDByte(' ');
		//TxDByte(gbpParameter[0]);TxDByte(' ');
		//TxDByte(gbpParameter[1]);TxDByte(' ');
#endif
		return 0;
	}

	//TxDString("\r\n TEST POINT 4");//TxDString("\r\n Err ID:0x");
#ifdef PRINT_OUT_PACKET_TO_USART2
	TxDStringC("\r\n ->[TX Buffer]: ");
	PrintBuffer(gbpTxBuffer,bTxLen);
	TxDStringC("\r\n <-[RX Buffer]: ");
	PrintBuffer(gbpRxBuffer,gbRxLength);
#endif

	gbDXLtxrxStatus = (1<<COMM_RXSUCCESS);

	//gbLengthForPacketMaking =0;
	return 1;
}


//������ �°� �޴� ���� : ��ſ����� ������ Length�� Ʋ�� ���ɼ��� ��ô ���� ����
byte rx_Packet(byte bRxLength){
	unsigned long ulCounter, ulTimeLimit;
	byte bCount, bLength, bChecksum;
	byte bTimeout;

	bTimeout = 0;
	if(bRxLength == 255) ulTimeLimit = RX_TIMEOUT_COUNT1;
	else ulTimeLimit = RX_TIMEOUT_COUNT2;
	for(bCount = 0; bCount < bRxLength; bCount++)
	{
		ulCounter = 0;
		while(gbDXLReadPointer == gbDXLWritePointer)
		{
			nDelay(NANO_TIME_DELAY); // porting ydh
			if(ulCounter++ > ulTimeLimit)
			{
				bTimeout = 1;

				break;
			}
			uDelay(0); //porting ydh added
		}
		if(bTimeout) break;
		gbpRxBuffer[bCount] = gbpDXLDataBuffer[gbDXLReadPointer++];
		//TxDStringC("gbpRxBuffer = ");TxDHex8C(gbpRxBuffer[bCount]);TxDStringC("\r\n");
	}


	bLength = bCount;
	bChecksum = 0;

	if( gbpTxBuffer[2] != BROADCAST_ID )
	{
		if(bTimeout && bRxLength != 255)
		{
#ifdef PRINT_OUT_COMMUNICATION_ERROR_TO_USART2
			TxDString("Rx Timeout");
			TxDByte(bLength);
#endif
			gbDXLtxrxStatus |= (1<<COMM_RXTIMEOUT);
			clearBuffer256();

			//return 0;
		}
		if(bLength > 3) //checking available length.
		{
			if(gbpRxBuffer[0] != 0xff || gbpRxBuffer[1] != 0xff )
			{
#ifdef PRINT_OUT_COMMUNICATION_ERROR_TO_USART2
				TxDStringC("Wrong Header");//[Wrong Header]
#endif
				gbDXLtxrxStatus |= (1<<COMM_RXCORRUPT);//RXHEADER);
				clearBuffer256();
				return 0;
			}
			if(gbpRxBuffer[2] != gbpTxBuffer[2] )
			{
#ifdef PRINT_OUT_COMMUNICATION_ERROR_TO_USART2
				TxDStringC("[Error:TxID != RxID]");
#endif
				gbDXLtxrxStatus |= (1<<COMM_RXCORRUPT);//RXID);
				clearBuffer256();
				return 0;
			}
			if(gbpRxBuffer[3] != bLength-4)
			{
#ifdef PRINT_OUT_COMMUNICATION_ERROR_TO_USART2
				TxDStringC("RxLength Error");
#endif
				gbDXLtxrxStatus |= (1<<COMM_RXCORRUPT);//RXLENGTH);
				clearBuffer256();
				return 0;
			}
			for(bCount = 2; bCount < bLength; bCount++) bChecksum += gbpRxBuffer[bCount];
			if(bChecksum != 0xff)
			{
#ifdef PRINT_OUT_COMMUNICATION_ERROR_TO_USART2
				TxDStringC("[RxChksum Error]");
#endif
				gbDXLtxrxStatus |= (1<<COMM_RXCORRUPT);//RXCHECKSUM);
				clearBuffer256();
				return 0;
			}
		}
	}

	return bLength;
}


byte tx_Packet(byte bID, byte bInstruction, byte bParameterLength){
    byte bCount,bCheckSum,bPacketLength;

	gbpTxBuffer[0] = 0xff;
    gbpTxBuffer[1] = 0xff;
    gbpTxBuffer[2] = bID;
    gbpTxBuffer[3] = bParameterLength+2; //Length(Paramter,Instruction,Checksum)

    gbpTxBuffer[4] = bInstruction;

    for(bCount = 0; bCount < bParameterLength; bCount++)
    {
        gbpTxBuffer[bCount+5] = gbpParameter[bCount];
    }
    bCheckSum = 0;
    bPacketLength = bParameterLength+4+2;

    for(bCount = 2; bCount < bPacketLength-1; bCount++) //except 0xff,checksum
    {
        bCheckSum += gbpTxBuffer[bCount];
    }

    gbpTxBuffer[bCount] = ~bCheckSum; //Writing Checksum with Bit Inversion

    DXL_TXD; // this define is declared in dxl.h


    for(bCount = 0; bCount < bPacketLength; bCount++)
    {
        TxByteToDXL(gbpTxBuffer[bCount]);
    }

    DXL_RXD;// this define is declared in dxl.h



    return(bPacketLength);
}


