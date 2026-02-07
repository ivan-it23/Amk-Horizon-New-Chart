int send_to_tool(System::IO::Ports::SerialPort^ COM, uint8_t CMD, uint8_t* DATA, uint16_t length) {
	bool console_debug = false;
	bool stop = false;
	uint8_t *packet = new uint8_t[length + 5];
	cli::array<Byte>^ packet_to_send = gcnew cli::array<Byte>(length + 5);
	//команда
	packet[0] = CMD;
	packet_to_send[0] = CMD;
	//записываем длину пакета
	uint16_t lenght_to_send = length + 5;
	memcpy(&packet[1], &(lenght_to_send), 2);
	packet_to_send[1] = packet[1];
	packet_to_send[2] = packet[2];
	//записываем сам пакет
	for (int i = 3; i < length + 3; i++) {
		packet[i] = DATA[i - 3];
		packet_to_send[i] = DATA[i - 3];
	}
	//считаем и записываем CRC
	uint16_t CRC_res = CRC16(packet, length + 3);
	packet_to_send[length + 3] = CRC_res;
	packet_to_send[length + 4] = CRC_res >> 8;

	COM->DiscardInBuffer();
	COM->DiscardOutBuffer();
	//COM->Dispose();
	try {
		COM->Write(packet_to_send, 0, length + 5);
	}
	catch (...) { stop = 1; }
	if (!stop) {
		if (console_debug == true) {
			Console::Write("send_to_tool ");
			for (int i = 0; i < length + 5; i++) {
				Console::Write("{0:x}", packet_to_send[i]); Console::Write(" ");
			}
			Console::Write(" CRC ");
			Console::Write("{0:x}", packet_to_send[length + 4]);
			Console::Write("{0:x}", packet_to_send[length + 3]);
			Console::WriteLine("");
		}


		//Sleep(100);
		delete packet;
		delete packet_to_send;
		return 0;
	}
	else {
		if (console_debug == true)
			Console::Write("error ");
	}
	return 1;
}

int receive_from_tool(System::IO::Ports::SerialPort^ COM, uint8_t CMD, uint8_t* DATA, uint16_t *readed_length) {
	bool console_debug = false;
	int  stop = 0; int rest_length_for_read = 0;  int buff_length = 2048;
	uint8_t *packet = new uint8_t[buff_length];
	cli::array<Byte>^ packet_to_receive = gcnew cli::array<Byte>(buff_length);

	for (int i = 0; i < (buff_length); i++) {
		packet[i] = 0xff;
		packet_to_receive[i] = 0xff;
	}
	//читаем первые три байта
	try {
		COM->Read(packet_to_receive, 0, 3);
	}
	catch (...) { stop = 1; }
	if (!stop) {
		for (int i = 0; i < 3; i++)
			packet[i] = packet_to_receive[i];
		//читаем полную длину пакета и определяем  оставшуюся длину = полная - 3
		rest_length_for_read = *(uint16_t *) &(packet[1]) - 3;
		if (console_debug == true)
			cout << " rest_length_for_read " << rest_length_for_read << endl;
	}

	else {
		cout << " first 3 byte no answer " << endl;
		stop = 0;
		COM->DiscardInBuffer();
		COM->DiscardOutBuffer();
		readed_length = 0;
		delete packet;
		delete packet_to_receive;
		return 1;
	}

	// читаем оставшиеся байты rest_length_for_read
	Sleep(300);
	int readed = 0;
	try {
		readed = COM->Read(packet_to_receive, 3, rest_length_for_read);

	}
	catch (...) { stop = 1; }

	if (!stop) {

		//заполняем оставшиеся 
		for (int i = 3; i < rest_length_for_read + 3; i++)
			packet[i] = packet_to_receive[i];

		COM->DiscardInBuffer();
		COM->DiscardOutBuffer();

		// если CRC от полной длины - последние два байта CRC = самoй CRC и прочитано все
		if (CRC16(packet, rest_length_for_read + 1) == *(uint16_t *) &(packet[rest_length_for_read + 1]) && rest_length_for_read == readed) {
			CMD = packet[0];

			for (int i = 0; i < rest_length_for_read - 2; i++)
				DATA[i] = packet_to_receive[i + 3];
			// длина пакета без первых трех и CRC 
			*readed_length = readed - 2;

			if (console_debug == true) {
				cout << " readed_length " << *readed_length << endl;
				Console::Write("receive_from_tool_ok ");
				for (int i = 0; i < rest_length_for_read + 3; i++) {
					Console::Write("{0:x}", packet_to_receive[i]); Console::Write(" ");
				}
				Console::WriteLine("");
			}

			delete packet;
			delete packet_to_receive;
			return 0;

		}
		// если не совпало CRC или прочитан не весь пакет
		else {

			if (console_debug == true) {
				Console::Write("error ");
				for (int i = 0; i < rest_length_for_read + 3; i++) {
					Console::Write("{0:x}", packet_to_receive[i]); Console::Write(" ");
				}
				Console::WriteLine("");
				cout << "crc no valid" << endl;
			}

			cout << "crc no valid" << endl;
			COM->DiscardInBuffer();
			COM->DiscardOutBuffer();
			readed_length = 0;
			delete packet;
			delete packet_to_receive;
			return 2;

		}
	}

	else {
		stop = 0;

		if (console_debug == true)
			cout << "timeout" << endl;

		COM->DiscardInBuffer();
		COM->DiscardOutBuffer();
		readed_length = 0;
		delete packet;
		delete packet_to_receive;
		return 1;
	}

}