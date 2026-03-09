#include "windows.h"
//#include "stdafx.h"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <complex.h> 
#include <variant>

#include <string> 
#include "variable.h"
using namespace std;

float  Ro_0 = 0.01;  //минимальное значение ус для расчета
float  Ro_max = 10000;  //максимальное значение ус для расчета
float epsilon_ABS = 0.0000005; // точность ,до которой возможны измерения амплитуды 
float epsilon_ARG = 0.0000005; // точность ,до которой возможны измерения фазы
float delta;
float  ro_0 = Ro_0;
float  ro_max = Ro_max;


//
ALL_DATA read_blok_all_data(ifstream &fin, int n) {
	ALL_DATA all_data_;
	fin.seekg(n * sizeof(ALL_DATA));
	fin.read((char*)&all_data_, sizeof(ALL_DATA));
	return all_data_;
}

CARTOGRAF_DATA_1FREQ read_blok_cartograf_data_1freq_Oleg(ifstream &fin, int n) {
	int pre = 11;
	int post = 21;
	CARTOGRAF_DATA_1FREQ cartograf_data_1freq;
	uint8_t *buff = new uint8_t[12];
	fin.seekg(n * (sizeof(CARTOGRAF_DATA_1FREQ) + pre + post));
	fin.read((char*)buff, pre);
	fin.read((char*)&cartograf_data_1freq, sizeof(CARTOGRAF_DATA_1FREQ));
	fin.read((char*)buff, post);
	return cartograf_data_1freq;
}

_4Tx_DATA read_blok_4Tx_data(ifstream &fin, int n) {
	_4Tx_DATA _4Tx_data;
	fin.seekg(n * sizeof(_4Tx_DATA));
	fin.read((char*)&_4Tx_data, sizeof(_4Tx_DATA));
	return _4Tx_data;
}

_4Tx_DATA read_blok_4Tx_data_Oleg(ifstream &fin, int n) {
	_4Tx_DATA _4Tx_data;
	uint8_t *buff = new uint8_t[12];
	fin.seekg(n * (sizeof(_4Tx_DATA) + 11));
	fin.read((char*)buff, 11);
	fin.read((char*)&_4Tx_data, sizeof(_4Tx_DATA));
	return _4Tx_data;
}

//определение типа прибора и количества кадров в файле /////////////////////////////////////
void define_sonde_type(DATA_FILE_PARAM *param) {
	int signature_for_pallete;
	uint32_t buff;
	int pre;
	int post;
	uint32_t sign;
	uint32_t struct_size;

	if (param->extension == ".bin" || param->extension == ".biv") {
		pre = 0; post = 0;
	}
	if (param->extension == ".DEV") {
		pre = 11;
		post = 21;
	}
	cout << " pre " << pre << " post " << post << endl;
	
	param->fin->seekg(pre);
	param->fin->read((char*)&sign, sizeof(uint32_t));
	param->fin->seekg(0, ios::end); //указатель на конец файла
	buff = sign << 12;	buff = buff >> 12;
	signature_for_pallete = buff / 1000;
	cout << "signature_for_pallete " << signature_for_pallete << endl;
	//signature_for_pallete 
	//1)1-autonom 2-lwd 3-kartograf
	//2)N_Tx 3,4,5
	//3)modification
	// если файл картографа сформирован в режиме информации
	if (signature_for_pallete == CARTOGRAPH_ && (param->extension == ".bin" || param->extension == ".biv")) {
		struct_size = sizeof(ALL_DATA);
		cout << "CARTOGRAPH_ .bin " << struct_size << " bytes struct size" << endl;
	}
	//если файл картографа получен из памяти
	if (signature_for_pallete == CARTOGRAPH_ && param->extension == ".DEV") {
		struct_size = (sizeof(CARTOGRAF_DATA_1FREQ)) + pre + post;
		cout << "CARTOGRAPH_ .DEV " << struct_size << " bytes struct size" << endl;
	}
	// если файл LWD_4Tx сформирован в режиме информации
	if (signature_for_pallete == LWD_4Tx_ && (param->extension == ".bin" || param->extension == ".biv")) {
		struct_size = sizeof(_4Tx_DATA);
		cout << "LWD_4Tx_ .bin " << struct_size << " bytes struct size" << endl;
	}
	//если файл LWD_4Tx получен из памяти
	if (signature_for_pallete == LWD_4Tx_ &&  param->extension == ".DEV") {
		struct_size = (sizeof(_4Tx_DATA)) + pre;
		cout << "LWD_4Tx_ .DEV " << struct_size << " bytes struct size" << endl;
	}
	// если файл AUTONOM_5Tx_ или AUTONOM_5Tx_SDR_ сформирован в режиме информации
	if ((signature_for_pallete == AUTONOM_5Tx_ || signature_for_pallete == AUTONOM_5Tx_SDR_) && (param->extension == ".bin" || param->extension == ".biv")) {
		struct_size = sizeof(_4Tx_DATA);
		cout << "AUTONOM_5Tx_ .bin " << struct_size << " bytes struct size" << endl;
	}
	//если файл AUTONOM_5Tx_ или AUTONOM_5Tx_SDR получен из памяти
	if ((signature_for_pallete == AUTONOM_5Tx_ || signature_for_pallete == AUTONOM_5Tx_SDR_) && param->extension == ".DEV") {
		struct_size = (sizeof(_4Tx_DATA)) + pre;
		cout << "AUTONOM_5Tx_ .DEV " << struct_size << " bytes struct size" << endl;
	}

	param->sonde_type = signature_for_pallete;
	param->signature = sign;
	//!!Убрать!!!!! пропуск нулевого кадра п подсчете длины файла в кадрах!!!!!!!!!
	if (signature_for_pallete == CARTOGRAPH_ && param->extension == ".DEV")  
		param->file_frames = (int(param->fin->tellg()) - struct_size)/ (2*struct_size);
	else
		param->file_frames = int(param->fin->tellg()) / struct_size;
	cout << "file_frames " << std::dec << param->file_frames << endl;

}

//читает из файла блок данных
variant<_4Tx_DATA, _5Tx_DATA, ALL_DATA > read_blok(DATA_FILE_PARAM *param, int n) {
	variant<_4Tx_DATA, _5Tx_DATA, ALL_DATA > result;
	// если файл картографа сформирован в режиме информации
	if (param->sonde_type == CARTOGRAPH_ && (param->extension == ".bin" || param->extension == ".biv")) {
		ALL_DATA all_data;
		param->fin->seekg(n * sizeof(ALL_DATA));
		param->fin->read((char*)&all_data, sizeof(ALL_DATA));
		result = all_data;
		//cout << "CARTOGRAPH " << param->extension << endl;
	}
	//если файл картографа получен из памяти
	if (param->sonde_type == CARTOGRAPH_ && param->extension == ".DEV") {
		ALL_DATA all_data;

		int pre = 11;
		int post = 21;
		CARTOGRAF_DATA_1FREQ cartograf_data_400;
		CARTOGRAF_DATA_1FREQ cartograf_data_2000;
		uint8_t *buff = new uint8_t[12];
		param->fin->seekg((2*n + 1) * (sizeof(CARTOGRAF_DATA_1FREQ) + pre + post));
		param->fin->read((char*)buff, pre);
		param->fin->read((char*)&cartograf_data_400, sizeof(CARTOGRAF_DATA_1FREQ));
		param->fin->read((char*)buff, post);
		param->fin->read((char*)buff, pre);
		param->fin->read((char*)&cartograf_data_2000, sizeof(CARTOGRAF_DATA_1FREQ));
		param->fin->read((char*)buff, post);

		all_data.data[0] = cartograf_data_400.data;
		all_data.raw_data[0] = cartograf_data_400.raw_data;
		all_data.data[1] = cartograf_data_2000.data;
		all_data.raw_data[1] = cartograf_data_2000.raw_data;
		result = all_data;
		//cout << "CARTOGRAPH " << param->extension << endl;
	}

	// если файл LWD_4Tx_ сформирован в режиме информации
	if (param->sonde_type == LWD_4Tx_ && (param->extension == ".bin" || param->extension == ".biv")) {
		_4Tx_DATA _4Tx_data;
		param->fin->seekg(n * sizeof(_4Tx_DATA));
		param->fin->read((char*)&_4Tx_data, sizeof(_4Tx_DATA));
		result = _4Tx_data;
		//cout << "LWD_4Tx " << param->extension << endl;
	}
	// если файл LWD_4Tx_ получен из памяти
	if (param->sonde_type == LWD_4Tx_ && param->extension == ".DEV") {
		_4Tx_DATA _4Tx_data;
		uint8_t *buff = new uint8_t[12];
		param->fin->seekg(n * (sizeof(_4Tx_DATA) + 11));
		param->fin->read((char*)buff, 11);
		param->fin->read((char*)&_4Tx_data, sizeof(_4Tx_DATA));
		result = _4Tx_data;
		//cout << "LWD_4Tx " << param->extension << endl;
	}

	// если файл AUTONOM_5Tx_ или AUTONOM_5Tx_SDR сформирован в режиме информации
	if ((param->sonde_type == AUTONOM_5Tx_ || param->sonde_type == AUTONOM_5Tx_SDR_) && (param->extension == ".bin" || param->extension == ".biv")) {
		_5Tx_DATA _5Tx_data;
		param->fin->seekg(n * sizeof(_5Tx_DATA));
		param->fin->read((char*)&_5Tx_data, sizeof(_5Tx_DATA));
		result = _5Tx_data;
		//cout << "AUTONOM_5Tx " << param->extension << endl;
	}
	// если файл AUTONOM_5Tx_ или AUTONOM_5Tx_SDR получен из памяти
	if ((param->sonde_type == AUTONOM_5Tx_ || param->sonde_type == AUTONOM_5Tx_SDR_) && param->extension == ".DEV") {
		_5Tx_DATA _5Tx_data;
		uint8_t *buff = new uint8_t[12];
		param->fin->seekg(n * (sizeof(_5Tx_DATA) + 11));
		param->fin->read((char*)buff, 11);
		param->fin->read((char*)&_5Tx_data, sizeof(_5Tx_DATA));
		result = _5Tx_data;
		//cout << "AUTONOM_5Tx " << param->extension << endl;
	}
	


	return result;
}

// переводит в GP_DATA для отображения на графиках ненаправленных измерений
GP_DATA transform_data(variant<_4Tx_DATA, _5Tx_DATA, ALL_DATA > sonde_data, ALL_DATA* all_data) {
	GP_DATA gp_data;
	//LWD_4Tx_ 241 CARTOGRAPH_ 351  AUTONOM_5Tx_ 151  AUTONOM_5Tx_SDR_ 152
	
	visit([&](auto&& current ) {
		using T = decay_t<decltype(current)>; 
		if constexpr (is_same_v<T, _4Tx_DATA>) {
			gp_data.signature = current.signature;
			gp_data.condition = current.condition;
			gp_data.frame = current.frame;
			gp_data.temperature = current.temperature;
			int N_Tx = 4;
			for (int freq = 0; freq < 2; freq++) {
				gp_data.ZERO_AM_RX_1[freq] = current.ZERO_AM_RX_1[freq];
				gp_data.ZERO_AM_RX_2[freq] = current.ZERO_AM_RX_2[freq];
				for (int Tx = 0; Tx < N_Tx; Tx++) {
					gp_data.phase_smt[freq][Tx] = current.phase_smt[freq][Tx];
					gp_data.rho_smt[freq][Tx] = current.rho_smt[freq][Tx];
					gp_data.AM_RX_1[freq][Tx] = current.AM_RX_1[freq][Tx];
					gp_data.AM_RX_2[freq][Tx] = current.AM_RX_2[freq][Tx];
					
					float signal = current.PH_RX_2[freq][Tx] - current.PH_RX_1[freq][Tx];
					if (fabs(signal) < 40) {
						while (signal >= PI) { signal -= 2 * PI; }
						while (signal <= -PI) { signal += 2 * PI; }
					}
					else { signal = 0.0; }
					gp_data.DELTA_PH[freq][Tx] = signal;
				}
			}
			
			//cout << "LWD_4Tx" << hex << current.signature << dec << endl;
		}
		
		else if constexpr (is_same_v<T, _5Tx_DATA>) {		
			gp_data.signature = current.signature;
			gp_data.condition = current.condition;
			gp_data.frame = current.frame;
			gp_data.temperature = current.temperature;
			int N_Tx = 5;
			for (int freq = 0; freq < 2; freq++) {
				gp_data.ZERO_AM_RX_1[freq] = current.ZERO_AM_RX_1[freq];
				gp_data.ZERO_AM_RX_2[freq] = current.ZERO_AM_RX_2[freq];
				gp_data.ZERO_dPH[freq] = current.ZERO_dPH[freq];
				for (int Tx = 0; Tx < N_Tx; Tx++) {
					gp_data.phase_smt[freq][Tx] = current.phase_smt[freq][Tx];
					gp_data.rho_smt[freq][Tx] = current.rho_smt[freq][Tx];
					gp_data.AM_RX_1[freq][Tx] = current.AM_RX_1[freq][Tx];
					gp_data.AM_RX_2[freq][Tx] = current.AM_RX_2[freq][Tx];
					gp_data.DELTA_PH[freq][Tx] = current.DELTA_PH[freq][Tx];					
				}
			}

			//cout << "AUTONOM_5Tx" << hex << current.signature << dec << endl;

		}
		
		else if constexpr (is_same_v<T, ALL_DATA>) {
			*all_data = current;
			gp_data.signature = current.data[0].signature;
			gp_data.condition = current.data[0].R_zz.condition;
			gp_data.frame = current.data[0].frame;
			gp_data.temperature = current.data[0].R_zz.temperature;
			int N_Tx = 4;
			for (int freq = 0; freq < 2; freq++) {
				gp_data.ZERO_AM_RX_1[freq] = abs(current.data[freq].R_zz.Tx_0[0]);
				gp_data.ZERO_AM_RX_2[freq] = abs(current.data[freq].R_zz.Tx_0[1]);
				for (int Tx = 0; Tx < N_Tx; Tx++) {
					gp_data.phase_smt[freq][Tx] = current.data[freq].phase_smt[Tx];
					gp_data.rho_smt[freq][Tx] = current.data[freq].rho_smt[Tx];
					gp_data.AM_RX_1[freq][Tx] = abs(current.data[freq].R_zz.Rzz1[Tx]);
					gp_data.AM_RX_2[freq][Tx] = abs(current.data[freq].R_zz.Rzz2[Tx]);
					
					float signal = arg(current.data[freq].R_zz.Rzz2[Tx]) - arg(current.data[freq].R_zz.Rzz1[Tx]);
					if (fabs(signal) < 40) {
						while (signal >= PI) signal -= 2 * PI;
						while (signal <= -PI) signal += 2 * PI;
					}
					else { signal = 0.0; }
					gp_data.DELTA_PH[freq][Tx] = signal;
				}
			}
			//cout << "CARTOGRAPH" << endl;
		}
	}, sonde_data);

	return gp_data;
}

_4Tx_DATA transform_GP_to_4Tx_data(GP_DATA *gp_data) {
	_4Tx_DATA _4tx_data;

	_4tx_data.signature = gp_data->signature;
	_4tx_data.condition = gp_data->condition;
	_4tx_data.frame = gp_data->frame;
	_4tx_data.temperature = gp_data->temperature;

	int N_Tx = 4;
	for (int freq = 0; freq < 2; freq++) {
		_4tx_data.ZERO_AM_RX_1[freq] = gp_data->ZERO_AM_RX_1[freq];
		_4tx_data.ZERO_AM_RX_2[freq] = gp_data->ZERO_AM_RX_2[freq];
		_4tx_data.ZERO_PH_RX_1[freq] = 0.0f;
		_4tx_data.ZERO_PH_RX_2[freq] = gp_data->ZERO_dPH[freq];
		
		for (int Tx = 0; Tx < N_Tx; Tx++) {
			_4tx_data.rho_smt[freq][Tx] = gp_data->rho_smt[freq][Tx];
			_4tx_data.phase_smt[freq][Tx] = gp_data->phase_smt[freq][Tx];
			
			_4tx_data.AM_RX_1[freq][Tx] = gp_data->AM_RX_1[freq][Tx];
			_4tx_data.AM_RX_2[freq][Tx] = gp_data->AM_RX_2[freq][Tx];
			
			_4tx_data.PH_RX_1[freq][Tx] = 0.0f;
			_4tx_data.PH_RX_2[freq][Tx] = gp_data->DELTA_PH[freq][Tx];

		}
	}


	return _4tx_data;
}

// переводит в METROLOGY_GP для отображения на графиках ненаправленных измерений
METROLOGY_GP transform_metro(variant<METROLOGY_CARTOGRAPH, METROLOGY_4Tx, METROLOGY_5Tx > current_metro_struct ) {
	
	METROLOGY_GP gp_metro;

	visit([&](auto&& current) {
		using T = decay_t<decltype(current)>;
		if constexpr (is_same_v<T, METROLOGY_CARTOGRAPH>) {
			gp_metro.signature = current.signature;
			gp_metro.serial = current.serial;
			gp_metro.D_sonde_mm = current.D_sonde_mm;
			int N_Tx = 4;
			for (int freq = 0; freq < 2; freq++) {
				gp_metro.F[freq] = current.F[freq];
				for (int Tx = 0; Tx < N_Tx; Tx++) {
					gp_metro.Air_zz[freq][Tx] = current.Air_zz[freq][Tx];
					gp_metro.Air_zz_amt[freq][Tx] = current.Air_zz_amt[freq][Tx];
				}			
			}
			for (int Tx = 0; Tx < N_Tx; Tx++) {
				gp_metro.L1[Tx] = current.L1[Tx];
				gp_metro.L2[Tx] = current.L2[Tx];
			}
			cout << "METROLOGY_CARTOGRAPH " << endl;
		}

		else if constexpr (is_same_v<T, METROLOGY_4Tx>) {
			gp_metro.signature = current.signature;
			gp_metro.serial = current.serial;
			gp_metro.D_sonde_mm = current.D_sonde_mm;
			int N_Tx = 4;
			for (int Tx = 0; Tx < N_Tx; Tx++) {
				gp_metro.L1[Tx] = current.L1[Tx];
				gp_metro.L2[Tx] = current.L2[Tx];
			}
			for (int freq = 0; freq < 2; freq++) {	
				gp_metro.F[freq] = current.F[4*freq];
				for (int Tx = 0; Tx < N_Tx; Tx++) {
					gp_metro.Air_zz[freq][Tx] = current.Air_zz[Tx + 4 * freq];
					gp_metro.Air_zz_amt[freq][Tx] = current.Air_zz_amt[Tx + 4 * freq];
				}
			}	
			cout << "METROLOGY_4Tx " << endl;
		}

		else if constexpr (is_same_v<T, METROLOGY_5Tx>) {
			gp_metro.signature = current.signature;
			gp_metro.serial = current.serial;
			gp_metro.D_sonde_mm = current.D_sonde_mm;
			int N_Tx = 5;
			for (int freq = 0; freq < 2; freq++) {
				gp_metro.F[freq] = current.F[freq];
				for (int Tx = 0; Tx < N_Tx; Tx++) {
					gp_metro.Air_zz[freq][Tx] = current.Air_zz[freq][Tx];
					gp_metro.Air_zz_amt[freq][Tx] = current.Air_zz_amt[freq][Tx];
				}
			}
			for (int Tx = 0; Tx < N_Tx; Tx++) {
				gp_metro.L1[Tx] = current.L1[Tx];
				gp_metro.L2[Tx] = current.L2[Tx];
			}
			cout << "METROLOGY_5Tx " << endl;
			
		}
	}, current_metro_struct);

	return gp_metro;
	
}

variant<METROLOGY_CARTOGRAPH, METROLOGY_4Tx, METROLOGY_5Tx > 
    read_metro_from_tool(System::IO::Ports::SerialPort^ COM, uint32_t sonde_type, bool *read_status) {
	variant<METROLOGY_CARTOGRAPH, METROLOGY_4Tx, METROLOGY_5Tx >  metrology_struct;
	ID id;
	bool stop = false;
	bool read_ok = false;
	uint8_t D0 = 0;
	try {
		Sleep(100);
		COM->ReadTimeout = 2000;
		COM->WriteTimeout = 2000;
		COM->BaudRate = 125000;
		COM->Open();
	}
	catch (...) { stop = 1; }

	if (!stop) {

		if (sonde_type == CARTOGRAPH_ || sonde_type == AUTONOM_5Tx_ || sonde_type == AUTONOM_5Tx_SDR_) {
			send_to_tool(COM, CMD_GET_METROLOGY, &D0, 0);
			Sleep(500);
			uint8_t CMD; uint16_t readed_length = 0; uint8_t DATA_IN[1024] = { 0, };
			int result_receive_from_sonde = receive_from_tool(COM, CMD, DATA_IN, &readed_length);
			if (result_receive_from_sonde == 0) {
			//uint32_t signature = *(uint32_t*)(DATA_IN);//убрать 2 при переходе на receive_from_tool
				id = get_sonde_id(*(uint32_t*)(DATA_IN));
				//cout << "id.type_0 " << std::dec << id.type << endl;
                if (id.type == CARTOGRAPH_) {
					METROLOGY_CARTOGRAPH metro;
					memcpy(&metro, DATA_IN, sizeof(METROLOGY_CARTOGRAPH));
					metrology_struct = metro;
					read_ok = true;
				}				
				else if (id.type == AUTONOM_5Tx_ || id.type == AUTONOM_5Tx_SDR_) {
					METROLOGY_5Tx metro;
					memcpy(&metro, DATA_IN, sizeof(METROLOGY_5Tx));
					metrology_struct = metro;
					read_ok = true;
				}												
			}
		}

		if (sonde_type == LWD_4Tx_) {
			cout << "read metro from tool LWD_4Tx " << endl;
			METROLOGY_4Tx metro_4Tx;
			memset((char*)&metro_4Tx, 0x00, sizeof(metro_4Tx));
			uint8_t DATA[3];
			DATA[0] = 0x00; DATA[1] = 0x04; DATA[2] = 240;
			send_to_tool_(COM, CMD_READ_EE, DATA, 3);
			Sleep(150);
			uint8_t CMD; uint8_t DATA_IN[256] = {0,};
			int result_receive_from_sonde = receive_from_tool_(COM, CMD, DATA_IN, 240);
			if (result_receive_from_sonde == 0) {
				memcpy(&metro_4Tx, DATA_IN, 240);
				metrology_struct = metro_4Tx;
				read_ok = true;
			}
		}

	}
	
	else cout << "not open " << endl;

	*read_status = read_ok;
	COM->Close(); 

	return metrology_struct;

}

void write_metro_to_tool(System::IO::Ports::SerialPort^ COM,  
	variant<METROLOGY_CARTOGRAPH, METROLOGY_4Tx, METROLOGY_5Tx> current_metro_struct, bool *write_status) {
	bool stop = false;
	bool write_ok = false;
	uint8_t D0 = 0;
	try {
		Sleep(100);
		COM->ReadTimeout = 2000;
		COM->WriteTimeout = 2000;
		COM->BaudRate = 125000;
		COM->Open();
	}

	catch (...) { stop = 1; }

	if (!stop) {
		uint32_t sonde_type;

		uint16_t current_metro_struct_size = 0;

		visit([&](auto&& current) {
			using T = decay_t<decltype(current)>;
			if constexpr (is_same_v<T, METROLOGY_CARTOGRAPH>) { sonde_type = CARTOGRAPH_; current_metro_struct_size = sizeof(current); }
			else if constexpr (is_same_v<T, METROLOGY_4Tx>) { sonde_type = LWD_4Tx_; current_metro_struct_size = sizeof(current);}
			else if constexpr (is_same_v<T, METROLOGY_5Tx>) { sonde_type = AUTONOM_5Tx_; current_metro_struct_size = sizeof(current);}
		}, current_metro_struct);

		if (sonde_type == CARTOGRAPH_ || sonde_type == AUTONOM_5Tx_) {
			uint8_t DATA_OUT[1024];
			memcpy(DATA_OUT, &current_metro_struct, current_metro_struct_size );
			send_to_tool(COM, CMD_SET_METROLOGY, DATA_OUT, current_metro_struct_size);
			Sleep(500);
			uint8_t CMD; uint16_t income_length = 0;
			if (receive_from_tool(COM, CMD, &D0, &income_length) == 0) {
				write_ok = true;
			}		
		}
		
		if (sonde_type == LWD_4Tx_) {
			uint8_t DATA_OUT[242];
			DATA_OUT[0] = 0x00; DATA_OUT[1] = 0x04;
			memcpy(DATA_OUT + 2, &current_metro_struct, 240);
			send_to_tool_(COM, CMD_WRITE_EE, DATA_OUT, 242);
			Sleep(150);
			uint8_t CMD; uint8_t DATA_IN;
			if (receive_from_tool_(COM, CMD, &DATA_IN, 0) == 0) {
				write_ok = true;
			}
		}
		
		*write_status = write_ok;
		COM->Close();
	}

}

// переводит температуру в градусы цельсия
float temp_deg(unsigned long adc_value) {
	float temp_deg;
	float v_in = 3300 * adc_value / pow(2, 10);
	if (v_in < 1500) temp_deg = (v_in - 500) / 10;
	else if (v_in >= 1500 && v_in < 1752.5) temp_deg = (v_in - 1500) / 10.1 + 100;
	else if (v_in >= 1752.5) temp_deg = (v_in - 1752.5) / 10.6 + 125;
	return temp_deg;
}

///////////////////////////////////////////////////////////////////////////////////////////////
// прямая задача
// n - номер передатчика 0-Т1, 1-Т2, 2-Т3, 3-Т4. freq - номер частоты 0-400кГц, 1-2000кГц
// расчитывает разницу фаз от УЭС для одной частоты от одного передатчика
// требуемые длины и частоту берет из структуры метрологии для данного передатчика 
float SIGNAL(struct METROLOGY_GP *metrology, uint16_t n, uint16_t freq, float ro) {
	float L1 = metrology->L1[n] / 1000.0;
	float L2 = metrology->L2[n] / 1000.0;
	float omegamu0sigma = (0.0078957 * metrology->F[freq]) / ro;
	complex<float> ik = 1if * sqrt(1if * omegamu0sigma);
	complex<float> SGN = exp(ik*(L2 - L1)) * ((1.0f - ik * L2) / (1.0f - ik * L1));
	return arg(SGN);
}


//обратная задача МПП "золотое сечение"
// рассчитывает симметризированные УЭС для одной частоты от одного передатчика
// из структуры Data берет симметризованные фазы и возвращает в нее рассчитанные УЭС
// структуру Data требуется подставить для нужной частоты!
int32_t RO_ARG(struct METROLOGY_GP *metrology, struct GP_DATA *Data, uint16_t n, uint16_t freq) {
	float Ro0;
	float epsilon_ARG = 0.0000005; // точность  фазы
	float epsilon_INF = 0.000000005; // точность  фазы
	float  Ro_0 = 0.01, Ro_max = 7000.0;  //мин. и мах. значение ус для расчета
	bool stop_epsl_ARG = false;
	bool stop_epsl_INF = false;
	float  ro_0 = Ro_0;
	float  ro_max = Ro_max;

	do {
		float X1 = ro_0 + 0.382*(ro_max - ro_0);
		float X2 = ro_max - 0.382*(ro_max - ro_0);
		float A = Data->phase_smt[freq][n] - SIGNAL(metrology, n, freq, X1);//
		float B = Data->phase_smt[freq][n] - SIGNAL(metrology, n, freq, X2);//
		if (fabs(A) > fabs(B)) { ro_0 = X1; }
		else { ro_max = X2; }
		if (fabs(A) < epsilon_ARG || fabs(B) < epsilon_ARG) { stop_epsl_ARG = true; }
		else if (fabs(A - B) < epsilon_INF) { stop_epsl_INF = true; }

	} while (stop_epsl_ARG == false && stop_epsl_INF == false);
	if (stop_epsl_ARG == true) Ro0 = (ro_0 + ro_max) / 2;
	if (stop_epsl_INF == true) Ro0 = 7200;//-100
	ro_0 = Ro_0;
	ro_max = Ro_max;
	Data->rho_smt[freq][n] = Ro0;
	return  0;
}


// определяет работоспособность передатчиков на одной частоте, основвываясь на амплитудах сигналов
// от этих передатчиков на дальних от них ненаправленных приемниках, и сравненим с хранящимися в
// структуре метрологии. 
// структуру Data требуется подставить для нужной частоты!
uint16_t get_condition(struct GP_DATA *Data, struct METROLOGY_GP *metro, uint16_t freq, float mV )
{
	float Tx_amp_val = 0.0;
	uint16_t condition = 0;
	//Tx1:
	Tx_amp_val = (Data->AM_RX_2[freq][T1])*mV; //Tx1->Rx2
	if (Tx_amp_val > metro->Air_zz_amt[freq][T1])
		condition |= (1 << (3 - T1));
	//Tx2:
	Tx_amp_val = (Data->AM_RX_1[freq][T2])*mV; //Tx2->Rx1
	if (Tx_amp_val > metro->Air_zz_amt[freq][T2])
		condition |= (1 << (3 - T2));
	//Tx3:
	Tx_amp_val = (Data->AM_RX_2[freq][T3])*mV; //Tx3->Rx2
	if (Tx_amp_val > metro->Air_zz_amt[freq][T3])
		condition |= (1 << (3 - T3));
	//Tx4:
	Tx_amp_val = (Data->AM_RX_1[freq][T4])*mV; //Tx4->Rx1
	if (Tx_amp_val > metro->Air_zz_amt[freq][T4])
		condition |= (1 << (3 - T4));

	return condition;
}

//определяет, как симметризировать, в зависимости от числа работающих передатчиков
uint16_t formula_simmetry(float K[4][4], uint16_t condition) {
	uint16_t result;
	//condition = condition >> 4;
	if (condition == 0b00001111) {// работают все 4 передатчика
		K[T1][T1] = +0.75;  K[T1][T2] = +0.50; K[T1][T3] = -0.25; K[T1][T4] = +0.00;
		K[T2][T1] = +0.25;  K[T2][T2] = +0.50; K[T2][T3] = +0.25; K[T2][T4] = +0.00;
		K[T3][T1] = +0.00;  K[T3][T2] = +0.25; K[T3][T3] = +0.50; K[T3][T4] = +0.25;
		K[T4][T1] = +0.00;  K[T4][T2] = -0.25; K[T4][T3] = +0.50; K[T4][T4] = +0.75;
		result = 0b00001111;
	}
	else if (condition == 0b00000111) {// работают 2,3,4 передатчики
		K[T1][T1] = +0.00; K[T1][T2] = +1.25; K[T1][T3] = +0.50; K[T1][T4] = -0.75;
		K[T2][T1] = +0.00; K[T2][T2] = +0.75; K[T2][T3] = +0.50; K[T2][T4] = -0.25;
		K[T3][T1] = +0.00; K[T3][T2] = +0.25; K[T3][T3] = +0.50; K[T3][T4] = +0.25;
		K[T4][T1] = +0.00; K[T4][T2] = -0.25; K[T4][T3] = +0.50; K[T4][T4] = +0.75;
		result = 0b00000111;
	}
	else if (condition == 0b00001110) {// работают 1,2,3 передатчики
		K[T1][T1] = +0.75; K[T1][T2] = +0.50; K[T1][T3] = -0.25; K[T1][T4] = +0.00;
		K[T2][T1] = +0.25; K[T2][T2] = +0.50; K[T2][T3] = +0.25; K[T2][T4] = +0.00;
		K[T3][T1] = -0.25; K[T3][T2] = +0.50; K[T3][T3] = +0.75; K[T3][T4] = +0.00;
		K[T4][T1] = -0.75; K[T4][T2] = +0.50; K[T4][T3] = +1.25; K[T4][T4] = +0.00;
		result = 0b00001110;
	}
	else if (condition == 0b00001011) {// работают 1,3,4 передатчики
		K[T1][T1] = +1.25; K[T1][T2] = 0.00; K[T1][T3] = -0.75; K[T1][T4] = 0.50;
		K[T2][T1] = +0.75; K[T2][T2] = 0.00; K[T2][T3] = -0.25; K[T2][T4] = 0.50;
		K[T3][T1] = +0.25; K[T3][T2] = 0.00; K[T3][T3] = +0.25; K[T3][T4] = 0.50;
		K[T4][T1] = -0.25; K[T4][T2] = 0.00; K[T4][T3] = +0.75; K[T4][T4] = 0.50;
		result = 0b00001011;
	}
	else if (condition == 0b00001101) {// работают 1,2,4 передатчики
		K[T1][T1] = +0.50; K[T1][T2] = +0.75; K[T1][T3] = +0.00; K[T1][T4] = -0.25;
		K[T2][T1] = +0.50; K[T2][T2] = +0.25; K[T2][T3] = +0.00; K[T2][T4] = +0.25;
		K[T3][T1] = +0.50; K[T3][T2] = -0.25; K[T3][T3] = +0.00; K[T3][T4] = +0.75;
		K[T4][T1] = +0.50; K[T4][T2] = -0.75; K[T4][T3] = +0.00; K[T4][T4] = +1.25;
		result = 0b00001101;
	}
	else if (condition == 0b00000000) {// выводим несимметризованные значения для всех передатчиков
		K[T1][T1] = 1.00; K[T1][T2] = 0.00; K[T1][T3] = 0.00; K[T1][T4] = 0.00;
		K[T2][T1] = 0.00; K[T2][T2] = 1.00; K[T2][T3] = 0.00; K[T2][T4] = 0.00;
		K[T3][T1] = 0.00; K[T3][T2] = 0.00; K[T3][T3] = 1.00; K[T3][T4] = 0.00;
		K[T4][T1] = 0.00; K[T4][T2] = 0.00; K[T4][T3] = 0.00; K[T4][T4] = 1.00;
		result = 0b00000000;
	}

	else {// работает меньше трех передатчиков
		//находим рабочие передатчики и для них выводим несимметризованные значения, для нерабочих фаза равна 0
		bool k4 = (condition >> 0) & 1u;
		bool k3 = (condition >> 1) & 1u;
		bool k2 = (condition >> 2) & 1u;
		bool k1 = (condition >> 3) & 1u;
		K[T1][T1] = 1.00*k1; K[T1][T2] = 0.00; K[T1][T3] = 0.00; K[T1][T4] = 0.00;
		K[T2][T1] = 0.00; K[T2][T2] = 1.00*k2; K[T2][T3] = 0.00; K[T2][T4] = 0.00;
		K[T3][T1] = 0.00; K[T3][T2] = 0.00; K[T3][T3] = 1.00*k3; K[T3][T4] = 0.00;
		K[T4][T1] = 0.00; K[T4][T2] = 0.00; K[T4][T3] = 0.00; K[T4][T4] = 1.00*k4;
		result = 0b00000000;
	}
	return result;
}

// переворачивает фазы, нормирует к воздуху и симметризует по формуле для одной частоты
// структуру Data требуется подставить для нужной частоты!
uint16_t simmetry(struct GP_DATA *Data, struct METROLOGY_GP *metro, uint16_t freq, uint16_t condition) {
	//если работаем без проверки аварийности - принудительно выставляем avaria = 0b00001111
	uint16_t avaria = condition;
	float K[4][4] = { 0.0, };
	float d_PH[4] = { 0.0, };
	//из сырых данных ненаправленных измерений получаем разницу фаз, нормированную к +-PI
	for (int Tx = 0; Tx < 4; Tx++) {
		d_PH[Tx] = Data->DELTA_PH[freq][Tx];
	}

	// нормализуем сигнал к воздуху
	//Z1 повернут T1
	for (int n = 0; n < 4; n++) {
		//n 0 1 2 3
		//T 1 2 3 4
		//  0 1 0 1
		if (n & 1)  d_PH[n] -= ((float)(metro->Air_zz[freq][n])) / 57297.0; // условие = 1 T2, T4
		else  d_PH[n] = -d_PH[n] + ((float)(metro->Air_zz[freq][n])) / 57297.0;// условие = 0 T1, T3
	}

	//рассчитываем коэф. симметризации в зависимости от работоспособности передатчиков
	//производим симметризацию и записываем в структуру Data
	uint16_t formula = formula_simmetry(K, avaria);
	for (int n = 0; n < 4; n++) {
		Data->phase_smt[freq][n] = K[n][0] * d_PH[0] + K[n][1] * d_PH[1] + K[n][2] * d_PH[2] + K[n][3] * d_PH[3];
	}
	return formula;
}


/*
void simmetry_5(PHASE *Phase_in, PHASE_5 *Phase_smt) {
	// вычисляем сигнал для 5 ти виртуальных симметризированных зондов
	//1.150 + 0.500 - 0.650 + 0.000
    Phase_smt->Phase[0] = 1.150 * Phase_in->Phase[T1_400] + 0.500 * Phase_in->Phase[T2_400] - 0.650 * Phase_in->Phase[T3_400] + 0.000 * Phase_in->Phase[T4_400];
	//0.675 + 0.500 - 0.175 + 0.000
	Phase_smt->Phase[1] = 0.675 * Phase_in->Phase[T1_400] + 0.500 * Phase_in->Phase[T2_400] - 0.175 * Phase_in->Phase[T3_400] + 0.000 * Phase_in->Phase[T4_400];
	//0.200 + 0.500 + 0.300 + 0.000
	Phase_smt->Phase[2] = 0.200 * Phase_in->Phase[T1_400] + 0.500 * Phase_in->Phase[T2_400] + 0.300 * Phase_in->Phase[T3_400] + 0.000 * Phase_in->Phase[T4_400];
	//0.000 + 0.225 + 0.500 + 0.275
	Phase_smt->Phase[3] = 0.000 * Phase_in->Phase[T1_400] + 0.225 * Phase_in->Phase[T2_400] + 0.500 * Phase_in->Phase[T3_400] + 0.275 * Phase_in->Phase[T4_400];
	//0.000 - 0.250 + 0.500 + 0.750 
	Phase_smt->Phase[4] = 0.000 * Phase_in->Phase[T1_400] - 0.250 * Phase_in->Phase[T2_400] + 0.500 * Phase_in->Phase[T3_400] + 0.750 * Phase_in->Phase[T4_400];
	
	//1.025 + 0.500 - 0.525 + 0.000
	Phase_smt->Phase[5] = 1.025 * Phase_in->Phase[T1_2000] + 0.500 * Phase_in->Phase[T2_2000] - 0.525 * Phase_in->Phase[T3_2000] + 0.000 * Phase_in->Phase[T4_2000];
	//0.550 + 0.500 - 0.050 + 0.000
	Phase_smt->Phase[6] = 0.550 * Phase_in->Phase[T1_2000] + 0.500 * Phase_in->Phase[T2_2000] - 0.050 * Phase_in->Phase[T3_2000] + 0.000 * Phase_in->Phase[T4_2000];
	//0.075 + 0.500 + 0.425 + 0.000
	Phase_smt->Phase[7] = 0.075 * Phase_in->Phase[T1_2000] + 0.500 * Phase_in->Phase[T2_2000] + 0.475 * Phase_in->Phase[T3_2000] + 0.000 * Phase_in->Phase[T4_2000];
	//0.000 + 0.100 + 0.500 + 0.400
	Phase_smt->Phase[8] = 0.000 * Phase_in->Phase[T1_2000] + 0.100 * Phase_in->Phase[T2_2000] + 0.500 * Phase_in->Phase[T3_2000] + 0.400 * Phase_in->Phase[T4_2000];
	//0.000 - 0.375 + 0.500 + 0.875 
	Phase_smt->Phase[9] = 0.000 * Phase_in->Phase[T1_2000] - 0.375 * Phase_in->Phase[T2_2000] + 0.500 * Phase_in->Phase[T3_2000] + 0.875 * Phase_in->Phase[T4_2000];
}
*/

uint16_t CRC16(const BYTE *nData, uint16_t wLength)
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

	BYTE nTemp;
	uint16_t wCRCWord = 0xFFFF;

	while (wLength--){
		nTemp = *nData++ ^ wCRCWord;
		wCRCWord >>= 8;
		wCRCWord ^= wCRCTable[nTemp];
	}
	return wCRCWord;
}

int signum(float val) {
	if (val > 0.0f)
		return 1;
	else if (val < 0.0f)
		return -1;
	else if (val == 0.0f)
		return 0;
}

ID get_sonde_id(uint32_t signature) {
	ID tool;
	uint32_t buff;
	buff = signature << 12;
	buff = buff >> 12;
	tool.type_ = buff / 100000;
	tool.N_Tx = (buff % 100000) / 10000;
	tool.mod = (buff % 10000) / 1000;
	tool.number = (buff % 1000);
	tool.type = buff / 1000;
	return tool;
}

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

//#define DEBUG_

//rph -random phase
void calk_geo_rph(RAW_DATA *raw, GEO *result, complex <float> fourier_k[8][16],  int N_rx) {

	for (int Tx = 0; Tx < 4; Tx++) {
		// буфера
		//MSD Mean square deviation
		float amp[16];
		float amp_2[16]; 
		complex <float> harm[8];
		complex <float> harm_amp[8];
		float amp_zz = 0;
		float amp_zx = 0;
		float arg_zx_zz;
		complex <float> Vzx_Vzz = 0;
		float buff[16];
		float buff_msd[16];
		float dv = 0.0f;
		memset(harm_amp, 0x00, sizeof(harm_amp));
		memset(harm, 0x00, sizeof(harm));
		memset(buff, 0x00, sizeof(buff));
		//memset(buff, 0x00, sizeof(amp_2));
		//расчитываем квадраты амплитуд сигнала на наклонной антенне для 16 ти секторов
		for (int n = 0; n < 16; n++) {			
			amp_2[n] = pow(raw->R_zx[N_rx][Tx].sector[Re][n] * mV, 2) + pow(raw->R_zx[N_rx][Tx].sector[Im][n] * mV, 2);
			amp[n] = sqrt(amp_2[n]);
			result->amp_2[N_rx][Tx][n] = amp_2[n];
			buff_msd[n] = sqrt(amp_2[n]);			
			buff[n] = amp_2[n];
		}
		//находим нулевую, первую и вторую гармоники квадрата амплитуды сигнала на наклонной антенне от угла тулфейса
		for (int k = 0; k < 8; k++) {
			for (int n = 0; n < 16; n++) {
				harm[k] += amp_2[n] * fourier_k[k][n];
				harm_amp[k] += amp[n] * fourier_k[k][n];
			}
		}
		harm[0] /= 2;
		harm_amp[0] /= 2;

		//выводим гармоники для апроксимации
		for (int k = 0; k < 8; k++) {
			result->harm[N_rx][Tx][k] = harm[k];
			result->harm_amp[N_rx][Tx][k] = harm_amp[k];
		}
			
		//определяем амплитуду , амплитуду Vzx и разницу фаз  Vzx и Vzz
		amp_zz = sqrt(abs(harm[0]) - abs(harm[2]));
		amp_zx = sqrt(2.0f * abs(harm[2]));

		//если фаза первой гармоники arg(harm[1]=arg(harm[2]/2+180 - то abs(harm[1]) нужно заменить на (-1)*abs(harm[1]) !!
		//коэфф при cos(phi_k - phi_0) может быть отрицательным, мы это увидим по перевороту фазы 1 - ой гармоники
		float K = 1.0f;
		if (fabs(arg(harm[1]) - (arg(harm[2]) / 2.0f) - PI) < 0.1) {
			//K = -1.0f;
		}
		float cos_arg_zx_zz = K * abs(harm[1]) / (2.0f*sqrt(2.0f*abs(harm[2])*(abs(harm[0]) - abs(harm[2]))));
		//if (cos_arg_zx_zz >= 1.0f)cos_arg_zx_zz = 1.0f 
			//- cos_arg_zx_zz
		//	;
		//arg_zx_zz = 0.0f;
		arg_zx_zz = acos(cos_arg_zx_zz);
		//arg_zx_zz =(cos_arg_zx_zz);
		//arg_zx_zz = (arg(harm[1]) - arg(harm[2]) / 2.0f)*57.3;
		//arg_zx_zz = (arg(harm_amp[1]) / 2.0f)*57.3;
		//arg_zx_zz = (arg(harm[2]) / 2.0f)*57.3;
		//arg_zx_zz = (abs(harm_amp[1]) / abs(harm_amp[2])) ;
		//arg_zx_zz = sqrt(abs(harm[2]) / abs(harm[4]));

		result->amp_zz[N_rx][Tx] = amp_zz;
		result->amp_zx[N_rx][Tx] = amp_zx;
		result->arg_zx_zz[N_rx][Tx] = arg_zx_zz;
		result->amp_xx[N_rx][Tx] = sqrt(2.0f * abs(harm[4]));
		result->amp_xx[N_rx][Tx] = abs(harm_amp[2]);
		//result->amp_xx[N_rx][Tx] = abs(harm[3])/abs(harm[2]);
		//result->betta_z[N_rx][Tx] = amp_zz / amp_zx;

		// получаем соотношение Vzx/Vzz в комплексном виде
		Vzx_Vzz = exp(1if *arg_zx_zz)*(amp_zx / amp_zz);

		// получаем геосигнал в комплексном виде
		result->geo[N_rx][Tx] = (1.0f - Vzx_Vzz) / (1.0f + Vzx_Vzz);		
		//угол на пласт - фаза первой гармоники
		result->border_angle[N_rx][Tx] = arg(harm[1])*57.3;
		//result->border_angle[N_rx][Tx] = sqrt(2.0f*(abs(harm[4])));
	
#ifdef DEBUG_
		for (int k = 0; k < 3; k++)
			cout << " abs harm " << abs(harm[k]) << " arg harm " << arg(harm[k]) << endl;
		cout << " amp_zz " << amp_zz << " amp_zx " << amp_zx << " arg_zx_zz " << arg_zx_zz << endl;
		cout << " Vzx_Vzz " << Vzx_Vzz << endl;
		cout << " geo  " << result->Geo[Tx] << endl;
#endif  DEBUG_

		// расчитываем dv - мусор Фурье
		// из каждой точки буфера вычитаем модельный сигнал, 
		//представляющий сумму амплитуд нулевой, первой и второй гармоник, на cos toolface
		for (int n = 0; n < 16; n++) {
			buff[n] -= (abs(harm[0]) + 
				        abs(harm[1])*cos(     PI*n / 8.0f - arg(harm[1])) + 
				        abs(harm[2])*cos(2.0f*PI*n / 8.0f - arg(harm[2])));
		}
		for (int n = 0; n < 16; n++) {
			buff[n] *= buff[n];
		}
		for (int n = 0; n < 16; n++) {
			dv += buff[n];
		}
		dv /= 16;
		dv = sqrt(dv);
		//результат мусора фурье от прямой компоненты сырого сигнала  пишем в Re
		result->dv[N_rx][Tx] = dv;

       //считаем СКО 
	   //среднее
		float summ = 0;
		for (int n = 0; n < 16; n++) {
			summ += buff_msd[n];
		}
		summ /= 16.0f;
		// сумма квадратов разностей текущего и среднего
		float summ_2 = 0;
		for (int n = 0; n < 16; n++) {
			summ_2 += pow((buff_msd[n] - summ), 2);
		}
		//
		summ_2 /= 16.0f;
		summ_2 = sqrt(summ_2);
		result->msd[N_rx][Tx] = summ_2;

	}

}

//sp -stable phase

void calk_geo_sph(RAW_DATA *raw, GEO *result, complex <float> fourier_k[8][16], int N_rx) {
	for (int Tx = 0; Tx < 4; Tx++) {
		// буфера
		//MSD Mean square deviation
		float RE[16];
		float IM[16];
		complex <float> harm_I[8];
		complex <float> harm_Q[8];
		complex <float> Vzz;
		complex <float> Vzx;
		complex <float> Vxx;
		
		float buff_I[16];
		float buff_Q[16];
		float buff_msd[16];
		float dv_I = 0.0f;
		float dv_Q = 0.0f;
		memset(harm_I, 0x00, sizeof(harm_I));
		memset(harm_Q, 0x00, sizeof(harm_Q));
		memset(buff_I, 0x00, sizeof(buff_I));
		memset(buff_Q, 0x00, sizeof(buff_Q));

		//забираем I и Q сигнала на наклонной антене для 16 ти секторов
		for (int n = 0; n < 16; n++) {					
			RE[n] = raw->R_zx[N_rx][Tx].sector[Re][n] * mV;
			IM[n] = raw->R_zx[N_rx][Tx].sector[Im][n] * mV;
			buff_msd[n] = sqrt(pow(RE[n], 2) + pow(IM[n], 2));			
			buff_I[n] = RE[n];
			buff_Q[n] = IM[n];
		}
		//находим нулевую, первую и вторую I и Q сигнала на наклонной антенне от угла тулфейса
		for (int k = 0; k < 8; k++) {
			for (int n = 0; n < 16; n++) {
				harm_I[k] += RE[n] * fourier_k[k][n];
				harm_Q[k] += IM[n] * fourier_k[k][n];
			}
		}
		harm_I[0] /= 2;
		harm_Q[0] /= 2;


		//выводим гармоники для апроксимации
		for (int k = 0; k < 8; k++) {
			result->harm_I[N_rx][Tx][k] = harm_I[k];
			result->harm_Q[N_rx][Tx][k] = harm_Q[k];
		}
		Vxx =  abs(harm_I[2]) +  1if * abs(harm_Q[2]) ;

		//определяем Vzz ,  Vzx и Vxx в комплексном виде и разницу фаз  Vzx и Vzz
		Vzz = abs(harm_I[0]) + 1if*abs(harm_Q[0]);
		//Vzx = { abs(harm_I[1])*signum(cos(arg(harm_I[1]))),  abs(harm_Q[1])*signum(cos(arg(harm_Q[1]))) };	
		Vzx = abs(harm_I[1]) + 1if*abs(harm_Q[1]) ;
		
		// выводим 
		result->amp_zz[N_rx][Tx] = abs(Vzz);
		result->amp_zx[N_rx][Tx] = abs(Vzx);
		result->amp_xx[N_rx][Tx] = abs(Vxx);
		result->arg_zx_zz[N_rx][Tx] = arg(Vzx / Vzz);
		result->betta_z[N_rx][Tx] = abs(Vzz) / abs(Vzx);

		// получаем геосигнал в комплексном виде
		result->geo[N_rx][Tx] = (Vzz - Vzx) / (Vzz + Vzx);
		//угол на пласт - фаза первой гармоники от I- исправить
		result->border_angle[N_rx][Tx] = arg(harm_I[1]);



#ifdef DEBUG_
		for (int k = 0; k < 3; k++)
			cout << " abs harm " << abs(harm[k]) << " arg harm " << arg(harm[k]) << endl;
		cout << " amp_zz " << amp_zz << " amp_zx " << amp_zx << " arg_zx_zz " << arg_zx_zz << endl;
		cout << " Vzx_Vzz " << Vzx_Vzz << endl;
		cout << " geo  " << result->Geo[Tx] << endl;
#endif  DEBUG_

		//расчитываем dv - мусор Фурье
		// из каждой точки буфера вычитаем модельный сигнал, 
		//представляющий сумму амплитуд нулевой, и первой  гармоник, на cos toolface
		for (int k = 0; k < 16; k++) {	
			buff_I[k] -= (abs(harm_I[0]) + abs(harm_I[1]) * cos(PI*k / 8 - arg(harm_I[1])));
			buff_Q[k] -= (abs(harm_Q[0]) + abs(harm_Q[1]) * cos(PI*k / 8 - arg(harm_Q[1])));
		}
		for (int k = 0; k < 16; k++) {
			buff_I[k] *= buff_I[k];
			buff_Q[k] *= buff_Q[k];
		}
		for (int k = 0; k < 16; k++) {
			dv_I += buff_I[k];
			dv_Q += buff_Q[k];
		}
		dv_I /= 16;		
		dv_Q /= 16;
		
		//результат мусора фурье 		
		result->dv[N_rx][Tx] = sqrt(dv_I +dv_Q);

		//считаем СКО 
		//среднее
		float summ = 0;
		for (int n = 0; n < 16; n++) {
			summ += buff_msd[n];
		}
		summ /= 16.0f;
		// сумма квадратов разностей текущего и среднего
		float summ_2 = 0;
		for (int n = 0; n < 16; n++) {
			summ_2 += pow((buff_msd[n] - summ), 2);
		}
		//
		summ_2 /= 16.0f;
		summ_2 = sqrt(summ_2);
		result->msd[N_rx][Tx] = summ_2;

	}

}

/*
void raw_data_phase_norm(direct_RX_zx *ant_sgn, direct_RX_zx *ref, direct_RX_zx *result ) {
	result->Rzx_Tx0_IQ[0] = ant_sgn->Rzx_Tx0_IQ[0];
	result->Rzx_Tx0_IQ[1] = ant_sgn->Rzx_Tx0_IQ[1];
	result->temperature = ant_sgn->temperature;
	result->condition = ant_sgn->condition;
	// для пяти передатчиков, номера которых берем из массива TX[4], в зависимости от кN_rx
	for (int Tx = 0; Tx < 5; Tx++) {
		complex <float> sgn[16];
		complex <float> ref_sgn[16];
		complex <float> norm_sgn[16];
		for (int n = 0; n < 16; n++) {
			complex <float> _Imaginary_I = 1if;
			//представляем референсный сигнал в комплексном виде
			ref_sgn[n] = ref->Rzx_I[Tx][n] + _Imaginary_I * ref->Rzx_Q[Tx][n];
			//представляем сигнал в комплексном виде			
			sgn[n] = ant_sgn->Rzx_I[Tx][n] + _Imaginary_I * ant_sgn->Rzx_Q[Tx][n];
			// получаем нормированный сигнал
			norm_sgn[n] = abs(ref_sgn[n])*(sgn[n] / ref_sgn[n]);
			//norm_sgn[n] = sgn[n];
			// распихиваем
			result->Rzx_I[Tx][n] = real(norm_sgn[n]);
			result->Rzx_Q[Tx][n] = imag(norm_sgn[n]);
		}
	}
}
*/

//функции для передачи в телесистему////////////////////////////////////////////////////////////
// вспомогательная функция распаковки УЭС из восьмибитного слова
float bits_code_to_ro(uint16_t code) {
	float percent = 2.0f;
	float percent_add = 0.0000593f;
	float Ro_0 = 0.3f;
	return float(Ro_0 * pow(((1.0f + percent / 100.0f) + percent_add * code), code));
}

//вспомогательная функция сжатия УЭС в восьмибитное слово
uint16_t ro_to_bits_code(float ro) {
	uint16_t result = 0;
	float percent = 2.0f;
	float percent_add = 0.0000593f;
	float Ro_0 = 0.3f;

	if (ro < Ro_0) return 0;

	for (int n = 0; n < 255; n++) {
		float ro_min = Ro_0 * pow(((1.0f + percent / 100.0f) + percent_add * n), n);
		float ro_max = Ro_0 * pow(((1.0f + percent / 100.0f) + percent_add * (n + 1)), n + 1);

		if (ro >= ro_min && ro <= ro_max) {
			if (ro <= (ro_min + ro_max) / 2.0f)
				result = n;
			else if (ro > (ro_min + ro_max) / 2.0f)
				result = n + 1;
		}
		if (n == 254 && ro >= ro_max) {
			result = n + 1;
		}
	}
	return result;
}

void set_mean_bits(REAP_CONST *reap, TO_PACK *pack) {
	//заполняем количество значащих бит 
	for (int n = 0; n < 4; n++)
		pack->G[n].bits = log2(reap->ATT_grad[_400_kGz][n] + 1);
	for (int n = 4; n < 8; n++)
		pack->G[n].bits = log2(reap->PH_grad[_400_kGz][n - 4] + 1);
	for (int n = 8; n < 12; n++)
		pack->G[n].bits = 8;

	for (int n = 12; n < 16; n++)
		pack->G[n].bits = log2(reap->ATT_grad[_2000_kGz][n - 12] + 1);
	for (int n = 16; n < 20; n++)
		pack->G[n].bits = log2(reap->PH_grad[_2000_kGz][n - 16] + 1);
	for (int n = 20; n < 24; n++)
		pack->G[n].bits = 8;
}

//перевод из структуры данных картографа в сжатую форму 
void compress(REAP_CONST *reap, DATA *data, COMPRESSED_1freq *c_data) {
	enum Right_Left { LEFT, RIGHT };
	c_data->frame = (float)(data->frame);
	uint8_t freq;
	if (data->dds_freq == 401) freq = 0;
	if (data->dds_freq == 2000)freq = 1;
	c_data->dds_freq = freq;
	for (int Tx = 0; Tx < 4; Tx++) {
		float ATT = data->ATT_dB_geo_signal_smt[Tx];
		float PH = data->PH_deg_geo_signal_smt[Tx];
		// reap
		c_data->GA[Tx].data = round((ATT + reap->ATT_max[freq][Tx])*(reap->ATT_grad[freq][Tx] / (2.0f*reap->ATT_max[freq][Tx])));
		c_data->GP[Tx].data = round((PH + reap->PH_max[freq][Tx])*(reap->PH_grad[freq][Tx] / (2.0f*reap->PH_max[freq][Tx])));
		c_data->Ro[Tx].data = ro_to_bits_code((float)data->rho_smt[Tx]);
		//заполняем количество значащих бит 
		c_data->GA[Tx].bits = log2(reap->ATT_grad[freq][Tx] + 1);
		c_data->GP[Tx].bits = log2(reap->PH_grad[freq][Tx] + 1);
		c_data->Ro[Tx].bits = 8;
	}
}

//перевод из сжатой формы в распакованную
void decompress(REAP_CONST *reap, COMPRESSED_1freq *c_data, DECOMPRESSED_1freq *d_data) {
	uint8_t freq = c_data->dds_freq;
	if (freq == 0) d_data->dds_freq = 401000;
	if (freq == 1) d_data->dds_freq = 2000000;
	for (int Tx = 0; Tx < 4; Tx++) {

		d_data->GA[Tx] = (float)c_data->GA[Tx].bits*((c_data->GA[Tx].data * 2.0f * reap->ATT_max[freq][Tx]) / reap->ATT_grad[freq][Tx] - reap->ATT_max[freq][Tx]);
		d_data->GP[Tx] = (float)c_data->GP[Tx].bits*((c_data->GP[Tx].data * 2.0f * reap->PH_max[freq][Tx]) / reap->PH_grad[freq][Tx] - reap->PH_max[freq][Tx]);
		d_data->Ro[Tx] = (float)c_data->Ro[Tx].bits*bits_code_to_ro(c_data->Ro[Tx].data);
	}
}

void out_compressed_data(DATA *data, METROLOGY_CARTOGRAPH *metro) {
	memset((char*)&data[0].out_arr[0], 0x00, 32);
	memset((char*)&data[1].out_arr[0], 0x00, 32);
	struct TO_PACK to_pack;
	struct COMPRESSED_1freq c_data[2];
	bool work_400 = false; bool work_2000 = false;
	if ((metro->select_key >> Work_400) & 1) {
		compress(&metro->reap, &data[_400_kGz], &c_data[_400_kGz]);
		std::memcpy(&to_pack, &c_data[_400_kGz].GA[0], sizeof(COMPRESSED_1freq) - 8);
		work_400 = true;
	}
	if ((metro->select_key >> Work_2000) & 1) {
		compress(&metro->reap, &data[_2000_kGz], &c_data[_2000_kGz]);
		std::memcpy(&to_pack.G[12], &c_data[_2000_kGz].GA[0], sizeof(COMPRESSED_1freq) - 8);
		work_2000 = true;
	}

	uint16_t n_0 = 0;
	uint16_t n_end = 0;
	uint16_t write_arr_freq_idx = 0;
	if (work_400 == true && work_2000 == true) { n_0 = 0; n_end = 24; write_arr_freq_idx = _2000_kGz; }//работают обе частоты
	else if (work_400 == true && work_2000 == false) { n_0 = 0; n_end = 12; write_arr_freq_idx = _400_kGz; }//работает только 400
	else if (work_400 == false && work_2000 == true) { n_0 = 12; n_end = 24; write_arr_freq_idx = _2000_kGz; }//работает только 2000
	else { n_0 = 0; n_end = 0; write_arr_freq_idx = 0; }

	//cout << "n_0 " << (int)n_0 << " n_end " << (int)n_end << " freq_idx " << (int)write_arr_freq_idx << endl;
	uint16_t bit_cntr = 0;
	uint16_t byte_cntr = 0;
	data->all_bit_cntr = 0;
	for (int n = n_0; n < n_end; n++) {
		if ((metro->select_key >> n) & 1) {
			PACKED packed;
			std::memcpy(&packed, &to_pack.G[n], sizeof(PACKED));
			std::cout << n << " Trans G " << bitset<16>(to_pack.G[n].data) << " G_bits " << (int)(to_pack.G[n].bits) << endl;
			if (n == 11)std::cout << endl;
			for (uint16_t n = 0; n < packed.bits; n++) {
				uint16_t bit = (packed.data >> n) & 1;
				if (bit == 1) {
					data[write_arr_freq_idx].out_arr[byte_cntr] |= (1 << bit_cntr);
				}
				data[write_arr_freq_idx].all_bit_cntr++;
				bit_cntr++;
				if (bit_cntr == 16) {
					bit_cntr = 0;
					byte_cntr++;
				}
			}
		}
	}
}

void receive_compressed_data(DECOMPRESSED_1freq *d_data, METROLOGY_CARTOGRAPH *metro, uint8_t* in_arr) {
	uint8_t bit_cntr = 0;
	uint8_t byte_cntr = 0;
	uint32_t all_bit_cntr = 0;

	struct TO_PACK from_pack;
	memset((char*)&from_pack, 0x00, sizeof(TO_PACK));
	set_mean_bits(&metro->reap, &from_pack);

	bool work_400 = false; bool work_2000 = false;
	if ((metro->select_key >> Work_400) & 1) {
		work_400 = true;
	}
	if ((metro->select_key >> Work_2000) & 1) {
		work_2000 = true;
	}

	uint8_t n_0 = 0;
	uint8_t n_end = 0;
	if (work_400 == true && work_2000 == true) { n_0 = 0; n_end = 24; }
	else if (work_400 == true && work_2000 == false) { n_0 = 0; n_end = 12; }
	else if (work_400 == false && work_2000 == true) { n_0 = 12; n_end = 24; }
	else { n_0 = 0; n_end = 0; }

	for (int curve = n_0; curve < n_end; curve++) {
		if ((metro->select_key >> curve) & 1) {
			uint8_t buff[2] = { 0, };
			uint8_t buff_bit_cntr = 0;
			uint8_t buff_byte_cntr = 0;
			//std::cout << from_pack.G[curve].bits << " ";
			for (uint8_t n = 0; n < from_pack.G[curve].bits; n++) {
				uint8_t bit = (in_arr[byte_cntr] >> bit_cntr) & 1;
				//std::cout << (int)bit << "" ;
				if (bit == 1) {
					buff[buff_byte_cntr] |= (1 << buff_bit_cntr);
				}
				buff_bit_cntr++;
				if (buff_bit_cntr == 8) {
					buff_bit_cntr = 0;
					buff_byte_cntr++;
				}
				all_bit_cntr++;
				bit_cntr++;
				if (bit_cntr == 8) {
					bit_cntr = 0;
					byte_cntr++;
				}
			}
			//std::cout << " ";
			//std::cout << endl;	
			uint16_t result = (buff[1] << 8);
			result += buff[0];
			from_pack.G[curve].data = result;
			from_pack.G[curve].bits = 1;//для актуализации распакованных данных умножаем на 1 в функции декомпресс
			//std::cout << curve << " packed.data " << bitset<8>(buff[1]) << bitset<8>(buff[0]) << endl;
			//std::cout << curve << " shift " << bitset<16>(result)  << endl;
		}
		else from_pack.G[curve].bits = 0;////для обнуления распакованных данных умножаем на 0 в функции декомпресс
	}

	COMPRESSED_1freq c_data[2];
	c_data[0].dds_freq = 0;
	c_data[1].dds_freq = 1;

	if (work_400 == true) {
		std::memcpy(&c_data[0].GA[0], &from_pack.G[0], 12 * sizeof(PACKED));
		decompress(&metro->reap, &c_data[0], &d_data[0]);
	}
	if (work_2000 == true) {
		std::memcpy(&c_data[1].GA[0], &from_pack.G[12], 12 * sizeof(PACKED));
		decompress(&metro->reap, &c_data[1], &d_data[1]);
	}

}

///////////////////////////////////////////////////////////////
void calck_geo_smt(DATA*data) {
	enum Right_Left { LEFT, RIGHT };
	//LEFT или RIGHT пока не выбрали
	for (int Tx = 0; Tx < 4; Tx++) {
		data->ATT_dB_geo_signal_smt[Tx] = -20.0f*log10(abs(data->R_zx[LEFT].Geo[Tx]));
		data->PH_deg_geo_signal_smt[Tx] = 57.290f*(arg(data->R_zx[LEFT].Geo[Tx]));;
	}
}


///old///////////////////////////////////////////////////////////////////////////////
int receive_from_tool_(System::IO::Ports::SerialPort^ COM, uint8_t CMD, uint8_t* DATA, uint16_t length) {
	bool console_debug = false;
	int  stop = 0;
	int readed = 0;
	uint8_t *packet = new uint8_t[length + 3];
	cli::array<Byte>^ packet_to_receive = gcnew cli::array<Byte>(length + 3);

	for (int i = 0; i < (length + 3); i++) {
		packet[i] = 0xff;
		packet_to_receive[i] = 0xff;
	}

	try {
		readed = COM->Read(packet_to_receive, 0, length + 3);
	}
	catch (...) { stop = 1; }

	if (!stop) {
		cout << length + 3 << " readed " << readed << endl;
		Sleep(20);
		for (int i = 0; i < length + 3; i++)
			packet[i] = packet_to_receive[i];

		COM->DiscardInBuffer();
		COM->DiscardOutBuffer();

		if (CRC16(packet, length + 1) == *(uint16_t *) &(packet[length + 1])) {
			CMD = packet[0];
			for (int i = 0; i < length; i++)
				DATA[i] = packet_to_receive[i + 1];

			if (console_debug == true) {
				Console::Write("receive_from_tool_ok ");
				for (int i = 0; i < length; i++) {
					Console::Write("{0:x}", DATA[i]); Console::Write(" ");
				}
				Console::WriteLine("");
			}

			delete packet;
			delete packet_to_receive;
			return 0;
		}


		else {
			if (console_debug == true) {
				Console::Write("error ");
				for (int i = 0; i < length + 3; i++) {
					Console::Write(i); Console::Write(" "); Console::Write("{0:x}", packet_to_receive[i]); Console::Write(" ");
				}
				Console::WriteLine("");
			}
			cout << "crc no valid" << endl;
			COM->DiscardInBuffer();
			COM->DiscardOutBuffer();
			delete packet;
			delete packet_to_receive;
			return 2;
		}

	}

	else {
		stop = 0;
		cout << "timeout" << endl;
		COM->DiscardInBuffer();
		COM->DiscardOutBuffer();
		delete packet;
		delete packet_to_receive;
		return 1;
	}

}

int send_to_tool_(System::IO::Ports::SerialPort^ COM, uint8_t CMD, uint8_t* DATA, uint16_t length) {
	bool console_debug = false;
	bool stop = false;
	uint8_t *packet = new uint8_t[length + 3];
	cli::array<Byte>^ packet_to_send = gcnew cli::array<Byte>(length + 3);
	packet[0] = CMD;
	packet_to_send[0] = CMD;
	for (int i = 1; i < length + 1; i++) {
		packet[i] = DATA[i - 1];
		packet_to_send[i] = DATA[i - 1];
	}
	uint16_t CRC_res = CRC16(packet, length + 1);
	packet_to_send[length + 1] = CRC_res;
	packet_to_send[length + 2] = CRC_res >> 8;

	COM->DiscardInBuffer();
	COM->DiscardOutBuffer();
	//COM->Dispose();
	try {
		COM->Write(packet_to_send, 0, length + 3);
	}
	catch (...) { stop = 1; }
	if (!stop) {
		if (console_debug == true) {
			Console::Write("send_to_tool ");
			for (int i = 0; i < length + 3; i++) {
				Console::Write("{0:x}", packet_to_send[i]); Console::Write(" ");
			}
			Console::Write(" CRC ");
			Console::Write("{0:x}", packet_to_send[length + 2]);
			Console::Write("{0:x}", packet_to_send[length + 1]);
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

//Для Сергея трансляция данных из структуры картографа в структуру обычного индукционника.
void transform_data(DATA* data, GP_DATA* gp_data) {
	gp_data->signature = data[0].signature;
	gp_data->condition = data[0].R_zz.condition;
	gp_data->frame = data[0].frame;
	gp_data->temperature = data[0].R_zz.temperature;
	int N_Tx = 4;
	for (int freq = 0; freq < 2; freq++) {
		gp_data->ZERO_AM_RX_1[freq] = abs(data[freq].R_zz.Tx_0[0]);
		gp_data->ZERO_AM_RX_2[freq] = abs(data[freq].R_zz.Tx_0[1]);
		for (int Tx = 0; Tx < N_Tx; Tx++) {
			gp_data->phase_smt[freq][Tx] = data[freq].phase_smt[Tx];
			gp_data->rho_smt[freq][Tx] = data[freq].rho_smt[Tx];
			gp_data->AM_RX_1[freq][Tx] = abs(data[freq].R_zz.Rzz1[Tx]);
			gp_data->AM_RX_2[freq][Tx] = abs(data[freq].R_zz.Rzz2[Tx]);

			float signal = arg(data[freq].R_zz.Rzz2[Tx]) - arg(data[freq].R_zz.Rzz1[Tx]);
			if (fabs(signal) < 40) {
				while (signal >= PI) signal -= 2 * PI;
				while (signal <= -PI) signal += 2 * PI;
			}
			else { signal = 0.0; }
			gp_data->DELTA_PH[freq][Tx] = signal;
		}
	}
}
