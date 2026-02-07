#pragma pack(push, 1)

static const float mG = (1000.0f * 180.0f) / 3.1415f;

struct _5Tx_DATA { // 240 byte
	uint32_t signature;
	uint32_t condition;
	uint32_t frame;
	float temperature;
	float rho_smt[2][5];
	float phase_smt[2][5];
	float AM_RX_1[2][5];
	float ZERO_AM_RX_1[2];
	float AM_RX_2[2][5];
	float ZERO_AM_RX_2[2];
	float DELTA_PH[2][5];
	float ZERO_dPH[2];
};

struct METROLOGY_5Tx {
	uint32_t signature;
	uint32_t serial;
	uint16_t L1[5];
	uint16_t L2[5];
	uint16_t F[2];
	int16_t Air_zz[10];
	int16_t Air_zz_amt[10];
	uint32_t D_sonde_mm;
	uint8_t service[164];
};

#pragma pack(pop)

static _5Tx_DATA g_data = { 0 };
static METROLOGY_5Tx g_metro = { 0 };

static uint16_t CRC16(const uint8_t* nData, uint16_t wLength) {
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
		0X8201, 0X42C0, 0X4380, 0X8341, 0X4100, 0X81C1, 0X8081, 0X4040
	};

	uint16_t wCRCWord = 0xFFFF;
	while (wLength--) {
		uint8_t nTemp = *nData++ ^ wCRCWord;
		wCRCWord >>= 8;
		wCRCWord ^= wCRCTable[nTemp];
	}
	return wCRCWord;
}

static void send_out(uint8_t CMD, const uint8_t* DATA, uint16_t length) {
	uint8_t packet[300];
	packet[0] = CMD;
	uint16_t total_len = length + 5;
	memcpy((uint8_t*)(&packet[1]), &total_len, 2);
	for (uint16_t i = 0; i < length; i++) {
		packet[3 + i] = DATA[i];
	}
	uint16_t crc = CRC16(packet, length + 3);
	packet[3 + length] = crc;
	packet[4 + length] = crc >> 8;
	Serial.write(packet, total_len);
}

static bool read_packet(uint8_t* outCmd, uint8_t* outPayload, uint16_t* outPayloadLen) {
	if (Serial.available() < 5) {
		return false;
	}

	uint8_t header[3];
	Serial.readBytes(header, 3);
	uint16_t total_len = *(uint16_t*)&header[1];
	if (total_len < 5 || total_len > 300) {
		while (Serial.available()) Serial.read();
		return false;
	}

	uint16_t rest = total_len - 3;
	uint8_t buf[300];
	buf[0] = header[0];
	buf[1] = header[1];
	buf[2] = header[2];

	size_t got = Serial.readBytes((char*)&buf[3], rest);
	if (got != rest) {
		while (Serial.available()) Serial.read();
		return false;
	}

	uint16_t calc = CRC16(buf, total_len - 2);
	uint16_t recv = *(uint16_t*)&buf[total_len - 2];
	if (calc != recv) {
		while (Serial.available()) Serial.read();
		return false;
	}

	*outCmd = buf[0];
	uint16_t payload_len = total_len - 5;
	*outPayloadLen = payload_len;
	for (uint16_t i = 0; i < payload_len; i++) {
		outPayload[i] = buf[3 + i];
	}
	return true;
}

static void fill_next_frame() {
	g_data.signature = 0x24DD9;
	for (int freq = 0; freq < 2; freq++) {
		for (int tx = 0; tx < 5; tx++) {
			g_data.rho_smt[freq][tx] = (tx + 1) * 10.0f + freq;
			g_data.phase_smt[freq][tx] = 100.0f * (tx + 1) / mG;
			g_data.AM_RX_1[freq][tx] = 1000.0f * (100 + tx + g_data.frame);
			g_data.AM_RX_2[freq][tx] = 1000.0f * (200 + tx + g_data.frame);
			g_data.DELTA_PH[freq][tx] = ((freq == 0) ? 50.0f : 3.0f) / mG;
		}
	}

	g_data.DELTA_PH[0][0] = (50.0f + g_data.frame) / mG;
	g_data.DELTA_PH[0][1] = -(60.0f + g_data.frame) / mG;
	g_data.DELTA_PH[0][2] = (70.0f + g_data.frame) / mG;
	g_data.DELTA_PH[0][3] = -(80.0f + g_data.frame) / mG;
	g_data.DELTA_PH[0][4] = (90.0f + g_data.frame) / mG;

	g_data.DELTA_PH[1][0] = (3.0f + g_data.frame) / mG;
	g_data.DELTA_PH[1][1] = -(5.0f + g_data.frame) / mG;
	g_data.DELTA_PH[1][2] = (7.0f + g_data.frame) / mG;
	g_data.DELTA_PH[1][3] = -(10.0f + g_data.frame) / mG;
	g_data.DELTA_PH[1][4] = (12.0f + g_data.frame) / mG;

	g_data.ZERO_AM_RX_1[0] = 0;
	g_data.ZERO_AM_RX_1[1] = 0;
	g_data.ZERO_AM_RX_2[0] = 0;
	g_data.ZERO_AM_RX_2[1] = 0;
	g_data.ZERO_dPH[0] = 0;
	g_data.ZERO_dPH[1] = 0;

	g_data.frame++;
	g_data.temperature++;
	if (g_data.temperature == 130) g_data.temperature = 0;
}

void setup() {
	Serial.begin(125000);

	g_metro.signature = 0x24DD9;
	g_metro.serial = 0x0002012D;
	g_metro.L1[0] = 314; g_metro.L2[0] = 566;
	g_metro.L1[1] = 694; g_metro.L2[1] = 946;
	g_metro.L1[2] = 1074; g_metro.L2[2] = 1326;
	g_metro.L1[3] = 1454; g_metro.L2[3] = 1706;
	g_metro.L1[4] = 314; g_metro.L2[4] = 566;
	g_metro.F[0] = 401;
	g_metro.F[1] = 2001;
	for (int i = 0; i < 10; i++) {
		g_metro.Air_zz[i] = 0;
		g_metro.Air_zz_amt[i] = 0;
	}
	g_metro.D_sonde_mm = 121;
}

void loop() {
	uint8_t cmd = 0;
	uint8_t payload[260];
	uint16_t payload_len = 0;

	if (!read_packet(&cmd, payload, &payload_len)) {
		return;
	}

	delay(10);
	while (Serial.available()) Serial.read();

	if (cmd == 0x16 && payload_len == 0) {
		send_out(0x16, (const uint8_t*)(&g_data.signature), sizeof(uint32_t));
		return;
	}

	if (cmd == 0x11 && payload_len == 0) {
		fill_next_frame();
		send_out(0x11, (const uint8_t*)(&g_data), sizeof(_5Tx_DATA));
		return;
	}

	if (cmd == 0x13 && payload_len == 0) {
		send_out(0x13, (const uint8_t*)(&g_metro), sizeof(METROLOGY_5Tx));
		return;
	}

	if (cmd == 0x12 && payload_len == sizeof(METROLOGY_5Tx)) {
		send_out(0x12, nullptr, 0);
		return;
	}

	if (cmd == 0x17 && payload_len == sizeof(uint32_t)) {
		send_out(0x17, nullptr, 0);
		return;
	}
}
