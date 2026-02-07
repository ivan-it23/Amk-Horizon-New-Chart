
#pragma pack(push, 1)
const double mG = (1000.0 * 180.0 )/ 3.1415;
const float mV = 4000.0f / pow(2.0f, 24.0f);

struct _4Tx_DATA {
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

struct _5Tx_DATA {// 240 byte
				uint32_t signature;
				uint32_t condition;
				uint32_t frame;
				float temperature;
				float rho_smt[2][5];// УЭС, рассчитанные на контроллере [400, 2000][T1-T5]
				float phase_smt[2][5];// симметризованные фазы [400, 2000][T1-T5]
				float AM_RX_1[2][5];// амплитуды на первом приемнике [400, 2000][T1-T5]
				float ZERO_AM_RX_1[2]; // амплитуды на первом приемнике [400, 2000] при молчащих передатчиках
				float AM_RX_2[2][5];// амплитуды на втором приемнике[400, 2000][T1 - T5]
				float ZERO_AM_RX_2[2];// амплитуды на втором приемнике [400, 2000] при молчащих передатчиках
				float DELTA_PH[2][5];// сырая разница фаз
				float ZERO_dPH[2];// разница фаз молчащих передатчиков;
	};

struct	METROLOGY_4Tx {
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



struct	METROLOGY_5Tx {
	uint32_t signature;
	uint32_t serial;
	uint16_t L1[5];//
	uint16_t L2[5];//
	uint16_t F[2];//
	int16_t Air_zz[10];//
	int16_t Air_zz_amt[10];//
	uint32_t D_sonde_mm;
	uint8_t service[164];//на будущее до 240 байт !!!!
};

#pragma pack(pop)

_4Tx_DATA  _4Tx_data = {0.0,};
METROLOGY_4Tx METRO_4Tx ;

_5Tx_DATA  _5Tx_data = {0.0,};
METROLOGY_5Tx METRO_5Tx ;


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

  // для старых ЛВД
	void send_out_(uint8_t CMD, uint8_t* DATA, uint8_t length) {
		uint8_t packet [255] = {0,};
		for (int i = 0; i < length ; i++) {
			packet[i+13] = DATA[i];
		}
		uint16_t CRC_res = CRC16(packet, length + 13);
		packet[length + 13] = CRC_res;
		packet[length + 14] = CRC_res >> 8;
		Serial.write(packet, length + 15);
	}
	//метрология для старых ЛВД
  void send_out_metro(uint8_t CMD, uint8_t* DATA, uint8_t length) {
			uint8_t packet [255];
			packet[0] = CMD;  
			for (int i = 0; i < length ; i++) {
				packet[i+1] = DATA[i];
			}
			uint16_t CRC_res = CRC16(packet, length + 1);
			packet[length + 1] = CRC_res;
			packet[length + 2] = CRC_res >> 8;
		  Serial.write(packet, length + 3);
		}

//для пяти байтовой команды
void send_out(uint8_t CMD, uint8_t* DATA, uint16_t length) {
	uint8_t packet [1000];
	packet[0] = CMD;
	//записываем длину пакета
	uint16_t lenght_to_send = length + 5;
	memcpy((uint8_t*)(&packet[1]), &(lenght_to_send), 2);
	//записываем сам пакет
	for (int i = 3; i < length + 3; i++) {
		packet[i] = DATA[i - 3];
	}
	//считаем и записываем CRC
	uint16_t CRC_res = CRC16(packet, length + 3);
	packet[length + 3] = CRC_res;
	packet[length + 4] = CRC_res >> 8;
  Serial.write(packet, length + 5);
	
}


int svitch_pin = 5;

void setup() {
 pinMode(svitch_pin, INPUT);	
 pinMode(13, OUTPUT);	
 Serial.begin(125000);
 //Serial.setTimeout(2000);  

  METRO_4Tx.signature = 0x3143add4;
	METRO_4Tx.serial = 0x0002012D;
	METRO_4Tx.L1[0] = 314; METRO_4Tx.L2[0] = 566;
	METRO_4Tx.L1[1] = 694; METRO_4Tx.L2[1] = 946;
	METRO_4Tx.L1[2] = 1074; METRO_4Tx.L2[2] = 1326;
	METRO_4Tx.L1[3] = 1454; METRO_4Tx.L2[3] = 1706;
	METRO_4Tx.L1[4] = 314; METRO_4Tx.L2[4] = 566;
	METRO_4Tx.L1[5] = 694; METRO_4Tx.L2[5] = 946;
	METRO_4Tx.L1[6] = 1074; METRO_4Tx.L2[6] = 1326;
	METRO_4Tx.L1[7] = 1454; METRO_4Tx.L2[7] = 1706;
	METRO_4Tx.F[0] = 401; METRO_4Tx.F[1] = 401; METRO_4Tx.F[2] = 401; METRO_4Tx.F[3] = 401;
	METRO_4Tx.F[4] = 2001; METRO_4Tx.F[5] = 2001; METRO_4Tx.F[6] = 2001; METRO_4Tx.F[7] = 2001;
	METRO_4Tx.Air_zz[0] = 00; METRO_4Tx.Air_zz[1] = 00; METRO_4Tx.Air_zz[2] = 00; METRO_4Tx.Air_zz[3] = 00;
	METRO_4Tx.Air_zz[4] = 00; METRO_4Tx.Air_zz[5] = 00; METRO_4Tx.Air_zz[6] = 00; METRO_4Tx.Air_zz[7] = 00;
	METRO_4Tx.D_sonde_mm = 121;


  METRO_5Tx.signature = 0x24DD9;
	METRO_5Tx.serial = 0x0002012D;
	METRO_5Tx.L1[0] = 314; METRO_5Tx.L2[0] = 566;
	METRO_5Tx.L1[1] = 694; METRO_5Tx.L2[1] = 946;
	METRO_5Tx.L1[2] = 1074; METRO_5Tx.L2[2] = 1326;
	METRO_5Tx.L1[3] = 1454; METRO_5Tx.L2[3] = 1706;
	METRO_5Tx.L1[4] = 314; METRO_5Tx.L2[4] = 566;
	METRO_5Tx.F[0] = 401; 
	METRO_5Tx.F[1] = 2001; 
	METRO_5Tx.Air_zz[0] = 00; METRO_5Tx.Air_zz[1] = 00; METRO_5Tx.Air_zz[2] = 00; METRO_5Tx.Air_zz[3] = 00;METRO_5Tx.Air_zz[4] = 00;
	METRO_5Tx.Air_zz[5] = 00; METRO_5Tx.Air_zz[6] = 00; METRO_5Tx.Air_zz[7] = 00; METRO_5Tx.Air_zz[8] = 00;METRO_5Tx.Air_zz[9] = 00;
	METRO_5Tx.D_sonde_mm = 121;


}
uint32_t now;
uint8_t buff[5] = {0,};
void loop() {
	  digitalWrite(13, digitalRead(svitch_pin));
// для пятибайтовой команды
    if( digitalRead(svitch_pin) == LOW){
      if (Serial.available() > 4) {
        for(int i = 0; i < 5; i++){
       		buff[i] = Serial.read();
	    	}
	      delay(10);
  	    while(Serial.available()){ Serial.read();}

  			if(buff[0] == 0x16 && buff[1] == 0x05 && buff[2] == 0x00 && buff[3] == 0x93 && buff[4] == 0x54){
    			send_out(0x16, (uint8_t*)(&METRO_5Tx), sizeof(uint32_t));
				}
 
  			if(buff[0] == 0x11 && buff[1] == 0x05 && buff[2] == 0x00 && buff[3] == 0x22 && buff[4] == 0x95){
    			//digitalWrite(13, !digitalRead(13));// toggle
    
	  			_5Tx_data.signature = 0x24DD9;
					for(int freq = 0; freq < 2; freq++){
      			for(int Tx = 0; Tx < 5; Tx++){
      				_5Tx_data.rho_smt[freq][Tx] = (Tx+1)*10 + freq;
      				_5Tx_data.phase_smt[freq][Tx] = 100*(Tx+1)/mG;
      				_5Tx_data.AM_RX_1[freq][Tx] = 1000*(100+Tx + _5Tx_data.frame);
      				_5Tx_data.AM_RX_2[freq][Tx] = 1000*(200+Tx + _5Tx_data.frame);     
      			} 
					}

    			_5Tx_data.DELTA_PH[0][0] = (50.0 + _5Tx_data.frame)/mG;
    			_5Tx_data.DELTA_PH[0][1] = -(60.0 + _5Tx_data.frame)/mG;
    			_5Tx_data.DELTA_PH[0][2] = (70.0 + _5Tx_data.frame)/mG;
    			_5Tx_data.DELTA_PH[0][3] = -(80.0 + _5Tx_data.frame)/mG;
    			_5Tx_data.DELTA_PH[0][4] = (90.0 + _5Tx_data.frame)/mG;
    
					_5Tx_data.DELTA_PH[1][0] = (3.0 + _5Tx_data.frame)/mG;
    			_5Tx_data.DELTA_PH[1][1] = -(5.0 + _5Tx_data.frame)/mG;
    			_5Tx_data.DELTA_PH[1][2] = (7.0 + _5Tx_data.frame)/mG;
    			_5Tx_data.DELTA_PH[1][3] = -(10.0 + _5Tx_data.frame)/mG;
    			_5Tx_data.DELTA_PH[1][4] = (12.0 + _5Tx_data.frame)/mG;

    			_5Tx_data.frame ++;
    			_5Tx_data.temperature ++;
					if(_5Tx_data.temperature == 130) _5Tx_data.temperature = 0;

    			send_out(0x11, (uint8_t*)(&_5Tx_data), sizeof(_5Tx_DATA));
    
  			}

  			if(buff[0] == 0x13 && buff[1] == 0x05 && buff[2] == 0x00 && buff[3] == 0x83 && buff[4] == 0x55){
	  			send_out(0x13, (uint8_t*)(&METRO_5Tx), sizeof(METROLOGY_5Tx));
				}

		 
  		}


		}

// для старых ЛВД
    if( digitalRead(svitch_pin) == HIGH){
      if (Serial.available() > 3) {
        for(int i = 0; i < 4; i++){
          buff[i] = Serial.read();
		    }
		  	delay(10);
  	  	while(Serial.available()){ 
					Serial.read();
				}

 		  	if(buff[0]== 0x67 && buff[1] == 0xfc && buff[2] == 0x2b && buff[3] == 0xc1){  
    			_4Tx_data.signature = 0x3143add4;
					for(int Tx = 0; Tx < 4; Tx++){
      			_4Tx_data.PH_smt[Tx] = (30+3*Tx)/mG;
      			_4Tx_data.Ro_smt[Tx] = 10*(Tx+1);
      			_4Tx_data.AM_RX_1[Tx] = 5*(Tx + _4Tx_data.frame)/mV;
      			_4Tx_data.AM_RX_2[Tx] = 5*(Tx + _4Tx_data.frame)/mV;
      			_4Tx_data.PH_RX_1[Tx] = 0;
    			}
    			for(int Tx = 4; Tx < 8; Tx++){
      			_4Tx_data.PH_smt[Tx] = (10+Tx)/mG;
      			_4Tx_data.Ro_smt[Tx] = 10*(Tx+1);
      			_4Tx_data.AM_RX_1[Tx] = 6*(Tx + _4Tx_data.frame)/mV;
      			_4Tx_data.AM_RX_2[Tx] = 6*(Tx + _4Tx_data.frame)/mV;
      			_4Tx_data.PH_RX_1[Tx] = 0;
    			}
    			_4Tx_data.PH_RX_2[0] = (50.0 + _4Tx_data.frame)/mG;
    			_4Tx_data.PH_RX_2[1] = -(60.0 + _4Tx_data.frame)/mG;
    			_4Tx_data.PH_RX_2[2] = (70.0 + _4Tx_data.frame)/mG;
    			_4Tx_data.PH_RX_2[3] = -(80.0 + _4Tx_data.frame)/mG;
    			_4Tx_data.PH_RX_2[4] = (3.0 + _4Tx_data.frame)/mG;
    			_4Tx_data.PH_RX_2[5] = -(5.0 + _4Tx_data.frame)/mG;
    			_4Tx_data.PH_RX_2[6] = (7.0 + _4Tx_data.frame)/mG;
    			_4Tx_data.PH_RX_2[7] = -(10.0 + _4Tx_data.frame)/mG;
    			_4Tx_data.frame ++;
    			_4Tx_data.temperature ++;

    			send_out_(0x67, (uint8_t*)(&_4Tx_data), sizeof(_4Tx_DATA));
  			}

 				if(buff[0]==0x65 || buff[1]==0x65 || buff[2]==0x65 || buff[3]==0x65){
    			send_out_metro(0x65, (uint8_t*)(&METRO_4Tx), sizeof(METROLOGY_4Tx));
  			} 
  
   		}
		}
		
	}

   
   
  


