#pragma pack(push, 1)
const double mG = (1000.0 * 180.0 )/ 3.1415;

struct AllData_P {
	uint32_t signature;
	uint32_t condition;
	float Ro_smt[8];
	float PH_smt[8];
	uint32_t frame;
	float temperature;
	float AM_RX_1[10];
	float AM_RX_2[10];
	float PH_RX_1[10];
	float PH_RX_2[10];
};

struct	METROLOGY {
	uint32_t signature;
	uint32_t serial;
	uint16_t L1[8];//
	uint16_t L2[8];//
	uint16_t F[8];//
	int16_t Air_zz[8];//
	uint16_t PhaseOffsets[4];//сдвиг между квадратурами
	uint32_t D_sonde_mm;
	int16_t Air_zz_amt[8];//
	uint8_t service[140];//на будущее до 256 байт !!!!
};

#pragma pack(pop)




AllData_P  petr = {0.0,};
METROLOGY METRO ;
	

uint16_t CRC16(const uint8_t *nData, uint16_t wLength)
		{
			static const uint16_t wCRCTable[] = {
			0X0000, 0XC0C1, 0XC181, 0X0140, 0XC301, 0X03C0, 0X0280, 0XC241,
			0XC601, 0X06C0, 0X0780, 0XC741, 0X0500, 0XC5C1, 0XC481, 0X0440,
			0XCC01, 0X0CC0, 0X0D80, 0XCD41, 0X0F00, 0XCFC1, 0XCE81, 0X0E40,
			0X0A00, 0XCAC1, 0XCB81, 0X0B40, 0XC901, 0X09C0, 0X0880, 0XC841,
			0XD801, 0X18C0, 0X1980, 0XD941, 0X1B00, 0XDBC1, 0XDA81, 0X1A40,
			0X1E00, 0XDEC1, 0XDF81, 0X1F40, 0XDD01, 0X1DC0, 0X1C80, 0XDC41,
			0X1400, 0XD4C1, 0XD581, 0X1540, 0XD701, 0X17C0, 0X1680, 0XD641,
			0XD201, 0X12C0, 0X1380, 0XD341, 0X1100, 0XD1C1, 0XD081, 0X1040,
			0XF001, 0X30C0, 0X3180, 0XF141, 0X3300, 0XF3C1, 0XF281, 0X3240,
			0X3600, 0XF6C1, 0XF781, 0X3740, 0XF501, 0X35C0, 0X3480, 0XF441,
			0X3C00, 0XFCC1, 0XFD81, 0X3D40, 0XFF01, 0X3FC0, 0X3E80, 0XFE41,
			0XFA01, 0X3AC0, 0X3B80, 0XFB41, 0X3900, 0XF9C1, 0XF881, 0X3840,
			0X2800, 0XE8C1, 0XE981, 0X2940, 0XEB01, 0X2BC0, 0X2A80, 0XEA41,
			0XEE01, 0X2EC0, 0X2F80, 0XEF41, 0X2D00, 0XEDC1, 0XEC81, 0X2C40,
			0XE401, 0X24C0, 0X2580, 0XE541, 0X2700, 0XE7C1, 0XE681, 0X2640,
			0X2200, 0XE2C1, 0XE381, 0X2340, 0XE101, 0X21C0, 0X2080, 0XE041,
			0XA001, 0X60C0, 0X6180, 0XA141, 0X6300, 0XA3C1, 0XA281, 0X6240,
			0X6600, 0XA6C1, 0XA781, 0X6740, 0XA501, 0X65C0, 0X6480, 0XA441,
			0X6C00, 0XACC1, 0XAD81, 0X6D40, 0XAF01, 0X6FC0, 0X6E80, 0XAE41,
			0XAA01, 0X6AC0, 0X6B80, 0XAB41, 0X6900, 0XA9C1, 0XA881, 0X6840,
			0X7800, 0XB8C1, 0XB981, 0X7940, 0XBB01, 0X7BC0, 0X7A80, 0XBA41,
			0XBE01, 0X7EC0, 0X7F80, 0XBF41, 0X7D00, 0XBDC1, 0XBC81, 0X7C40,
			0XB401, 0X74C0, 0X7580, 0XB541, 0X7700, 0XB7C1, 0XB681, 0X7640,
			0X7200, 0XB2C1, 0XB381, 0X7340, 0XB101, 0X71C0, 0X7080, 0XB041,
			0X5000, 0X90C1, 0X9181, 0X5140, 0X9301, 0X53C0, 0X5280, 0X9241,
			0X9601, 0X56C0, 0X5780, 0X9741, 0X5500, 0X95C1, 0X9481, 0X5440,
			0X9C01, 0X5CC0, 0X5D80, 0X9D41, 0X5F00, 0X9FC1, 0X9E81, 0X5E40,
			0X5A00, 0X9AC1, 0X9B81, 0X5B40, 0X9901, 0X59C0, 0X5880, 0X9841,
			0X8801, 0X48C0, 0X4980, 0X8941, 0X4B00, 0X8BC1, 0X8A81, 0X4A40,
			0X4E00, 0X8EC1, 0X8F81, 0X4F40, 0X8D01, 0X4DC0, 0X4C80, 0X8C41,
			0X4400, 0X84C1, 0X8581, 0X4540, 0X8701, 0X47C0, 0X4680, 0X8641,
			0X8201, 0X42C0, 0X4380, 0X8341, 0X4100, 0X81C1, 0X8081, 0X4040 };

			uint8_t nTemp;
			uint16_t wCRCWord = 0xFFFF;

			while (wLength--)
			{
				nTemp = *nData++ ^ wCRCWord;
				wCRCWord >>= 8;
				wCRCWord ^= wCRCTable[nTemp];
			}
			
			return wCRCWord;

		}


	int send_out(uint8_t CMD, uint8_t* DATA, uint8_t length) {
			bool stop = false;
			uint8_t packet [length + 15];
			uint8_t packet_to_send [length + 15];
			packet[0] = CMD; packet[1] = 0; packet[2] = 0; packet[3] = 0; packet[4] = 0; packet[5] = 0; packet[6] = 0;  packet[7] = 0; packet[8] = 0; packet[9] = 0; packet[10] = 0; packet[11] = 0; packet[12] = 0; 
			packet_to_send[0] = CMD;packet_to_send[1] = 0;packet_to_send[2] = 0;packet_to_send[3] = 0;packet_to_send[4] = 0;packet_to_send[5] = 0;packet_to_send[6] = 0;packet_to_send[7] = 0;packet_to_send[8] = 0;packet_to_send[9] = 0;packet_to_send[10] = 0;packet_to_send[11] = 0;packet_to_send[12] = 0;

			for (int i = 0; i < length ; i++) {
				packet[i+13] = DATA[i];
				packet_to_send[i+13] = DATA[i];
			}
			uint16_t CRC_res = CRC16(packet, length + 13);
			packet_to_send[length + 13] = CRC_res;
			packet_to_send[length + 14] = CRC_res >> 8;
		  Serial.write(packet_to_send, length + 15);
			return 0;
		}


int send_out_metro(uint8_t CMD, uint8_t* DATA, uint8_t length) {
			bool stop = false;
			uint8_t packet [length + 3];
			uint8_t packet_to_send [length + 3];
			packet[0] = CMD; 
      packet_to_send[0] = CMD;
			for (int i = 0; i < length ; i++) {
				packet[i+1] = DATA[i];
				packet_to_send[i+1] = DATA[i];
			}
			uint16_t CRC_res = CRC16(packet, length + 1);
			packet_to_send[length + 1] = CRC_res;
			packet_to_send[length + 2] = CRC_res >> 8;
		  Serial.write(packet_to_send, length + 3);
			return 0;
		}


void setup() {
 Serial.begin(125000);  
 petr.signature = 0x31444453;
 petr.frame = 0x30;
  for(int i = 0; i < 8; i++){
      petr.PH_smt[i] = 0.01*i;
      petr.Ro_smt[i] = 1.1*i;
      petr.AM_RX_1[i] = 50+i;
      petr.AM_RX_2[i] = 60+i;
      petr.PH_RX_1[i] = 0.1;
      petr.PH_RX_2[i] = 0.2;
  }

METRO.signature = 0x31443453;
	METRO.serial = 0x0002012D;
	METRO.L1[0] = 314; METRO.L2[0] = 566;
	METRO.L1[1] = 694; METRO.L2[1] = 946;
	METRO.L1[2] = 1074; METRO.L2[2] = 1326;
	METRO.L1[3] = 1454; METRO.L2[3] = 1706;
	METRO.L1[4] = 314; METRO.L2[4] = 566;
	METRO.L1[5] = 694; METRO.L2[5] = 946;
	METRO.L1[6] = 1074; METRO.L2[6] = 1326;
	METRO.L1[7] = 1454; METRO.L2[7] = 1706;
	METRO.F[0] = 401; METRO.F[1] = 401; METRO.F[2] = 401; METRO.F[3] = 401;
	METRO.F[4] = 2001; METRO.F[5] = 2001; METRO.F[6] = 2001; METRO.F[7] = 2001;
	METRO.Air_zz[0] = 00; METRO.Air_zz[1] = 00; METRO.Air_zz[2] = 00; METRO.Air_zz[3] = 00;
	METRO.Air_zz[4] = 00; METRO.Air_zz[5] = 00; METRO.Air_zz[6] = 00; METRO.Air_zz[7] = 00;
	METRO.D_sonde_mm = 121;



}
uint32_t now;
char buff[4] = {0,};
void loop() {
 if (Serial.available() > 3) {
   Serial.readBytes(buff, 4);
 // while(Serial.available()){ Serial.read();}
  if(buff[0]==0x67 || buff[1]==0x67 || buff[2]==0x67 || buff[2]==0x67){
    buff[0] = 0; buff[1] = 0; buff[2] = 0; buff[3] = 0;
    

    petr.signature = 0x31443454;
    for(int i = 0; i < 8; i++){
      petr.PH_smt[i] = 100/mG;
      petr.Ro_smt[i] = 10*(i+1);
      petr.AM_RX_1[i] = 100*(100+i + petr.frame);
      petr.AM_RX_2[i] = 100*(200+i + petr.frame);
      // PH_RX_1 теперь заполняется ниже с разными значениями для каждого канала
    }
    // Разные значения для каждого передатчика, чтобы видеть все 4 кривые
    petr.PH_RX_2[0] = -100.0/mG;  // T1 @ 400 Гц → -100 mdeg
    petr.PH_RX_2[1] = -50.0/mG;   // T2 @ 400 Гц → -50 mdeg
    petr.PH_RX_2[2] = 50.0/mG;    // T3 @ 400 Гц → +50 mdeg
    petr.PH_RX_2[3] = 100.0/mG;   // T4 @ 400 Гц → +100 mdeg
    petr.PH_RX_2[4] = -200.0/mG;  // T1 @ 2000 Гц → -200 mdeg
    petr.PH_RX_2[5] = -100.0/mG;  // T2 @ 2000 Гц → -100 mdeg
    petr.PH_RX_2[6] = 100.0/mG;   // T3 @ 2000 Гц → +100 mdeg
    petr.PH_RX_2[7] = 200.0/mG;   // T4 @ 2000 Гц → +200 mdeg
    
    // Также заполним PH_RX_1 разными значениями
    petr.PH_RX_1[0] = -80.0/mG;   // T1 @ 400 Гц
    petr.PH_RX_1[1] = -40.0/mG;   // T2 @ 400 Гц
    petr.PH_RX_1[2] = 40.0/mG;    // T3 @ 400 Гц
    petr.PH_RX_1[3] = 80.0/mG;    // T4 @ 400 Гц
    petr.PH_RX_1[4] = -160.0/mG;  // T1 @ 2000 Гц
    petr.PH_RX_1[5] = -80.0/mG;   // T2 @ 2000 Гц
    petr.PH_RX_1[6] = 80.0/mG;    // T3 @ 2000 Гц
    petr.PH_RX_1[7] = 160.0/mG;   // T4 @ 2000 Гц
    
    // Данные для "молчащего" передатчика TX0 (индексы 8-9)
    petr.AM_RX_1[8] = 50 + petr.frame;   // TX0 @ 400 Гц, RX1
    petr.AM_RX_2[8] = 60 + petr.frame;   // TX0 @ 400 Гц, RX2
    petr.AM_RX_1[9] = 55 + petr.frame;   // TX0 @ 2000 Гц, RX1
    petr.AM_RX_2[9] = 65 + petr.frame;   // TX0 @ 2000 Гц, RX2
    petr.PH_RX_1[8] = 10.0/mG;           // TX0 @ 400 Гц, RX1
    petr.PH_RX_2[8] = 20.0/mG;           // TX0 @ 400 Гц, RX2
    petr.PH_RX_1[9] = 15.0/mG;           // TX0 @ 2000 Гц, RX1
    petr.PH_RX_2[9] = 25.0/mG;           // TX0 @ 2000 Гц, RX2
    petr.frame ++;
    send_out(0x67, (char*)(&petr), sizeof(AllData_P));
  }

 
   if(buff[0]==0x65 || buff[1]==0x65 || buff[2]==0x65 || buff[2]==0x65){
    buff[0] = 0; buff[1] = 0; buff[2] = 0; buff[3] = 0;
    

   
    send_out_metro(0x65, (char*)(&METRO), sizeof(METROLOGY));
  }
  

 }

   
   
  

}
