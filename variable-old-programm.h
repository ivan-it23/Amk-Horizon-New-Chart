#pragma once
#include "windows.h"
//#include "stdafx.h"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <complex> 
#include <string> 

#include <typeinfo.h>
#include "ShellAPI.h"
#include <msclr/marshal_cppstd.h>
#include <bitset>
#include <vector>
#include <time.h>
#include <variant>
#include "wininet.h"
#pragma comment (lib,"Wininet.lib")


using namespace std;
using namespace System;
using namespace System::Data;
using namespace System::IO::Ports;
using namespace System::Threading;
using namespace System::Runtime::InteropServices;

const float PI = 3.141592741f;
const float eps0 = 8.85f*1e-12f;
const float mu0 = 4.0f * PI*1e-7f;

const float mV = 0.00023842;
const float mG = 1000.0f * 180.0f / PI;
const float Grad = 180.0f / PI;


#define CARTOGRAPH_ 351
#define CARTOGRAPH_LWD_4Tx_ 359
#define AUTONOM_5Tx_ 151
#define	AUTONOM_5Tx_SDR_ 152
#define	LWD_4Tx_ 241
#define	LWD_4Tx_NEW 242

#define КEY (0x01 << 4)
// 0x01 получить кадр данных измерений 
#define CMD_GET_DATA 0x01 | КEY
// 0x02 чтение файла метрологии из EEPROM ЦК
#define CMD_GET_METROLOGY 0x03 | КEY
// 0x03 запись файла метрологии в EEPROM ЦК
#define CMD_SET_METROLOGY 0x02 | КEY
// 0x06 чтение сигнатуры  из EEPROM ЦК
#define CMD_GET_SIGNATURE 0x06 | КEY

// 0x05 чтение presets  из EEPROM ЦК
#define CMD_GET_PRESETS 0x05 | КEY
// 0x04 запись presets  в EEPROM ЦК
#define CMD_SET_PRESETS 0x04 | КEY
// 0x04 запись настроек передачи в телесистему  в EEPROM ЦК
#define CMD_SET_TELESYSTEM 0x07 | КEY

#define CMD_TIME_SYNC 0xF5
// 0x01 чтение флэш-памяти
#define CMD_ERAM 0x61 
#define CMD_TURBO 0xFD
#define CMD_WORK 0x67 
// 0x05 чтение EEPROM памяти (в EEPROM храним калибровки и т.д.)
#define CMD_READ_EE 0x65 
// 0x06 запись EEPROM памяти
#define CMD_WRITE_EE 0x66 

#define Work_400 24
#define Work_2000 25

enum T_ {
	T1, T2, T3, T4, T5
};

enum RE_IM {
	Re, Im,
};

enum Right_Left {
	Left_,
	Right_,
};

enum UP_DOWN {
	up_chart,
	down_chart,
};

enum UNDIR_DIRECT {
	undir,
	direct,
};

enum FREQ {
	_400_kGz,
	_2000_kGz,
};

enum WORK_MODE {
	 NOT,
	_400,
	_2000,
	BOTH
};
//////////////////////////////////////////////////////////////////////////////////////////////

#pragma pack(push, 1)
//структуры для сжатия и передачи в телесистему//////////////////////////////////

struct PACKED {
	uint16_t data;//упакованные данные
	//количество значащих младших бит, которые требуется забрать для передачи наверх
	//например, если data 0b00000011 10101010 10 значащих  бит, то bits = 10
	uint16_t bits;
};

struct COMPRESSED_1freq {
	uint32_t frame;// номер кадра со старта сесии
	uint32_t dds_freq;//частота в Герцах 1 значащий бит 0b00000000 - 400 кГц 0b00000001 - 2000 кГц 
	PACKED GA[4];//амплитуда геосигнала в дециБеллах в сжатом виде 	
	PACKED GP[4];//фаза геосигнала в дециБеллах в сжатом виде 
	PACKED Ro[4];//УЭС в сжатом виде 

};

struct TO_PACK {
	PACKED G[24];//амплитуда геосигнала в дециБеллах в сжатом виде 	
};

struct DECOMPRESSED_1freq {
	// номер кадра со старта сесии
	uint32_t frame;
	uint32_t dds_freq;//частота 
	float GA[4];//амплитуда геосигнала в дециБеллах распакованная 	
	float GP[4];//фаза геосигнала в дециБеллах  распакованная 
	float Ro[4];//УЭС распакованное 
};

// константы для сжатия данных для телесистемы 
//ATT_max и PH_max - пределы изменения амплитуд и фаз геосигналов
//ATT_grad и PH_grad - количество градаций для амплитуд и фаз геосигналов 
//[2][4] - две частоты четыре передатчика
struct REAP_CONST {
	float ATT_max[2][4];
	uint32_t ATT_grad[2][4];
	float PH_max[2][4];
	uint32_t PH_grad[2][4];
};
//////////////////////////////////////////////////////////////////////////////

// структуры для вывода данных в этой программе

struct INC_SET {
	uint32_t MA_WINDOW_SIZE; //32
	uint32_t MLD_WINDOW_SIZE; //32
	uint32_t N; //128
	uint32_t K; //128
	uint32_t auto_delta; //1
	uint32_t INTERRUPT_BY_ANGLE_PATH; // 0
	float W_MIN; //3.14
	float W_MAX; //18.84
	float K_predict; //0.5
	float APS_DELTA; //PI/360
	float history_angle; //(20*PI)/180
	float K_delta; //2.5
};

struct TURN_PRESETS {
	uint32_t Tx[6] = { 0,1,2,3,4,5 };// порядок работы передатчиков в режиме ненаправленных измерений
	uint32_t Tx_by_turn[3][5][6]; // порядок работы передатчиков в режиме направленных измерений для разных скоростей вращения буровой колонны
	uint32_t tx_work_time_units[7];// = { 5,5,6,7,8,9,0 };//время работы одного передатчика в единицах цикла опроса датчиков положения (1 мс)
};

struct	METROLOGY_CARTOGRAPH {
	uint32_t signature;
	uint32_t serial;
	uint32_t L1[4];//
	uint32_t L2[4];//
	uint32_t L_geo[5];//
	uint32_t F[2];//
	int32_t Air_zz[2][5];//воздух ненаправленных измерений
	int32_t Air_zz_amt[2][5];//минимальная амплитуда, определяющая работоспособность передатчика для ненаправленных измерений
	INC_SET inc_set;
	TURN_PRESETS turn_presets;
	uint32_t D_sonde_mm;//диаметр прибора	
	uint64_t unix_data;//дата в юникс эпох
	REAP_CONST reap;
	uint32_t select_key;// ключ, посылаемый по команде  24 младших бита отвечают за выбранные сигналы(каналы)
	uint32_t work_type;// если 0 - то картограф (по умолчанию), если 1- то обычный индукционник 
	uint32_t service[61];//на будущее до 1000 байт !!!!
};

struct	METROLOGY_CARTOGRAPH_new {
	uint32_t signature;
	uint32_t serial;
	uint32_t L1[5];//
	uint32_t L2[5];//
	uint32_t F[2];//
	int32_t Air_zz[2][5];//воздух ненаправленных измерений
	int32_t Air_zz_amt[2][5];//минимальная амплитуда, определяющая работоспособность передатчика для ненаправленных измерений
	uint32_t D_sonde_mm;//диаметр прибора
///здесь кончается общая часть////////////////////////////////////////////////////////////////////////////////////////	
	uint32_t L_geo[5];//
	INC_SET inc_set;
	TURN_PRESETS turn_presets;
	uint64_t unix_data;//дата в юникс эпох
	REAP_CONST reap;
	uint32_t select_key;// ключ, посылаемый по команде  24 младших бита отвечают за выбранные сигналы(каналы)
	uint32_t work_type;// если 0 - то картограф (по умолчанию), если 1- то обычный индукционник 
	uint32_t service[59];//на будущее до 1000 байт !!!!
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
	uint8_t service[140];//на будущее до 240 байт !!!!
};

struct	METROLOGY_5Tx {
	uint32_t signature;
	uint32_t serial;
	uint16_t L1[5];//
	uint16_t L2[5];//
	uint16_t F[2];//
	int16_t Air_zz[2][5];//
	int16_t Air_zz_amt[2][5];//
	uint32_t D_sonde_mm;
	uint8_t service[164];//на будущее до 240 байт !!!!
};

struct	METROLOGY_GP {
	uint32_t signature;
	uint32_t serial;
	uint16_t L1[5];//
	uint16_t L2[5];//
	uint16_t F[2];//
	int16_t Air_zz[2][5];//
	int16_t Air_zz_amt[2][5];//
	uint32_t D_sonde_mm;
	uint8_t service[164];//на будущее до 240 байт !!!!
};

struct Inclinometer {
	float Azm;
	float Zen;
	float Aps;
};

//direct_RX  
struct direct_RX {//128
	complex <float> Tx_0;//8
	complex <float> Geo[4];//32
	float amp_Vzz[4];//16
	float amp_Vzx[4];//16
	float ph_Vzx_Vzz[4];//16
	float dv[4];//16
	float border_angle[4];//16
	float temperature;//4
	uint32_t condition;//4
};

//undirect_RX  80
struct undirect_RX {
	complex <float> Tx_0[2];
	complex <float> Rzz1[4];
	complex <float> Rzz2[4];
	float temperature;
	uint32_t condition;
};

struct _16_sector_data { //RAW с напр. приемников (3,4) по номеру TX(1,2,4,5 или 1,2,3,5)
	float sector[2][16];//Re,Im
};

//raw_sector_data_all  1024 в uart
struct RAW_DATA {
	struct _16_sector_data R_zx[2][4];// Rx_Left, Rx_Right, Tx1234[4] 
};

//raw_sector_data_all  1024 в uart
struct RAW_DATA_ALEX {
	complex<float>  R_zx[2][2][4][16];// [freq] [Rx_Left, Rx_Right], [Tx1234[4]] 
	uint32_t start_sector[2];// [freq] 
};

//AllData_Direct_1DDS_1freq  
struct DATA { // всего 512 байт.
	// потребительская информация//////////////////////////////////
	uint32_t signature;//индетификатор типа прибора (для картографа 351 или 352,3,4,5-9)
	uint32_t frame;//номер кадра                                              //
	uint32_t dds_freq;// частотатв кГц
	float ATT_dB_geo_signal_smt[4];//затухание в дециБеллах направленные компенсированные измерения
	float PH_deg_geo_signal_smt[4];//фаза в градусах направленные компенсированные измерения 
	float rho_smt[4];//УЭС ,рассчитанные на компьютере
	float phase_smt[4];//калиборвенные фазы ,рассчитанные на компьютере
	uint8_t out_arr[32];//массив сжатых и побитово упакованных данных для отправки в телесистему
	uint32_t all_bit_cntr;//количество значащих бит в этом массиве
	uint32_t depth;//глубина, на которой были сделаны измерения(заполняется при привязке к глубине*)
	//информация для отладки////////////////////////////////////////
	struct direct_RX R_zx[2];// данные от двух направленных приемников [0]верхний(левый) и [1]нижний(правый)
	struct undirect_RX R_zz;// данные от центрального приемника                                      
	struct Inclinometer INC;// углы ABS, ZEN, AZM
	int32_t  start_sector; // начальный сектор направленных измерений 0-15
	float Wg; //угловая скорость  об.мин                                                   
	uint32_t Tx_sector_condition[8];//информация о пропущенных секторах
};
//* Важно!!! depth заполняется для каждой точки измерений возможно, что на одной и той же глубине прибор 
// находился продолжительное время или бывал несколько раз, тогда эта глубина будет прописана для нескольких точек.


//AllData_Direct_1DDS_1freq  
struct DATA_old { // всего 512 байт.
	uint32_t signature;                                          //
	uint32_t frame;                                              //
	uint32_t dds_freq;                                           //
	struct direct_RX R_zx[2];                                    //
	struct undirect_RX R_zz;                                     //
	complex <float> geo_signal_smt[4];                           //  
	float phase_smt[4];                                          //
	float rho_smt[4];                                            //
	float rho_up_smt[4];                                         // 
	float rho_down_smt[4];                                       // 
	struct Inclinometer INC;                                     //
	int32_t start_sector;                                       //
	float Wg;                                                   // 
	uint32_t error_msg;
	uint32_t depth;
	//uint16_t expService[16];                                     
	uint32_t Tx_sector_condition[8];//
};

struct ALL_DATA {
	DATA data[2];
	RAW_DATA raw_data[2];
};

struct CARTOGRAF_DATA_1FREQ {
	DATA data;
	RAW_DATA raw_data;
};

struct GEO { // два приемника, четыре передатчика                                        
	float amp_2[2][4][16];// два приемника, четыре передатчика 16 секторов
	float amp_zz[2][4];// два приемника, четыре передатчика
	float amp_zx[2][4];// два приемника, четыре передатчика
	float amp_xx[2][4];// два приемника, четыре передатчика
	float arg_zx_zz[2][4];// два приемника, четыре передатчика
	float betta_z[2][4];// два приемника, четыре передатчика
	complex <float> geo[2][4]; // два приемника, четыре передатчика
	complex <float> harm[2][4][8];// два приемника, четыре передатчика восемь гармоник;
	complex <float> harm_amp[2][4][8];// два приемника, четыре передатчика восемь гармоник Real;
	complex <float> harm_I[2][4][8];// два приемника, четыре передатчика восемь гармоник Real;
	complex <float> harm_Q[2][4][8];// два приемника, четыре передатчика восемь гармоник Real;
	float border_angle[2][4]; // два приемника, четыре передатчика ;
	float dv[2][4];// два приемника, четыре передатчика
	float msd[2][4];// два приемника, четыре передатчика
};

struct PH_SHIFT {
	float d_fi[2][5];
};

struct _4Tx_DATA {
	uint32_t signature;
	uint32_t condition;
	float rho_smt[2][4];
	float phase_smt[2][4];
	uint32_t frame;
	float temperature;

	float AM_RX_1[2][4];
	float ZERO_AM_RX_1[2];
	float AM_RX_2[2][4];
	float ZERO_AM_RX_2[2];
	
	float PH_RX_1[2][4];
	float ZERO_PH_RX_1[2];
	float PH_RX_2[2][4];
	float ZERO_PH_RX_2[2];
};

struct _5Tx_DATA {// 240 byte
	uint32_t signature;
	uint32_t condition;
	uint32_t frame;
	float temperature;
	float rho_smt[2][5];// УЭС, рассчитанные на контроллере [400, 2000][T1-T5]
	float phase_smt[2][5];// симметризованные фазы [400, 2000][T1-T5]
	float AM_RX_1[2][5];// амплитуды на первом приемнике [400, 2000][T1-T5]
	float ZERO_AM_RX_1[2]; 
	float AM_RX_2[2][5];// амплитуды на втором приемнике[400, 2000][T1 - T5]
	float ZERO_AM_RX_2[2];// амплитуды на втором приемнике [400, 2000] при молчащих передатчиках
	float DELTA_PH[2][5];// сырая разница фаз
	float ZERO_dPH[2];// разница фаз молчащих передатчиков;
};



struct GP_DATA {
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
	float DELTA_PH[2][5];// сырая разница фаз [400, 2000][T1 - T5]
	float ZERO_dPH[2];// разница фаз молчащих передатчиков;
	//uint8_t out_arr[8];
	//uint32_t selected_key;
};

struct GP_TOOL_DATA_5_TX {
	uint32_t signature;
	uint32_t condition;
	uint32_t frame;
	float temperature;
	float Ro_smt[10];// УЭС, рассчитанные на контроллере T1_400 T2_400 T3_400 T4_400 T5_400 T1_2000 T2_2000 T3_2000 T4_2000 T5_2000
	float PH_smt[10];// симметризованные фазы  T1_400 T2_400 T3_400 T4_400 T5_400 T1_2000 T2_2000 T3_2000 T4_2000 T5_2000
	float AM_RX_1[12]; // амплитуды на первом приемнике T1_400 T2_400 T3_400 T4_400 T5_400 T1_2000 T2_2000 T3_2000 T4_2000 T5_2000 Tsilet_400  Tsilent_2000
	float AM_RX_2[12]; // амплитуды на втором приемнике T1_400 T2_400 T3_400 T4_400 T5_400 T1_2000 T2_2000 T3_2000 T4_2000 T5_2000 Tsilet_400  Tsilent_2000
	float DELTA_PH[12];//разница фаз T1_400 T2_400 T3_400 T4_400 T5_400 T1_2000 T2_2000 T3_2000 T4_2000 T5_2000 Tsilent_400  Tsilent_2000
	

};

// CMD, SET_OF_PRESETS CRC
struct SET_OF_PRESETS {
	uint16_t Signature = 0x5AA5; // 0x5AA5 - для определения занятого места во флэше
	uint16_t TBTIndex;  // индекс в массиве Tx_by_turn - выводить
	uint16_t MinRotSpeed; // минимальная скорость вращения для направленных измерений (ниже - переходим на ненаправленные) - выводить
	uint8_t DebugSession; // отладочная сессия (да/нет) - выводить
	uint8_t RotSpeed; // скорость вращения при эмуляции
	uint64_t DateTime;// - выводить
};

// SERVICE уже не актуальна
struct SERVICE {
	uint16_t Signature = 0x1111; // 
	uint16_t periodic_start = 0; // 
	uint16_t RotSpeed; // cкорость вращения для направленных измерений (ниже - переходим на ненаправленные) - выводить
	uint16_t add; // 
	uint64_t add1;//
};


// у Пети структуры такие//////////////////////////////////////////////////////////////////////////////
struct direct_RX_zx {
	float Rzx_Tx0_IQ[2];//I и Q первое ДФТ от CH1_for_dft[] молчащие передатчики
	float Rzx_I[5][16];//I первое ДФТ от CH1_for_dft[] работающие передатчики 5шт 16ть секторов
	float Rzx_Q[5][16];//Q первое ДФТ от CH1_for_dft[] работающие передатчики 5шт 16ть секторов
	float temperature;
	uint32_t condition;
};

struct DATA_FILE_PARAM{
	ifstream *fin;
	const char* Data_file_name;//задаем
	string extension;//задаем
	uint32_t sonde_type;//задаем и получаем
	uint32_t signature; //получаем
	uint32_t file_frames;//получаем
};


struct ID {
	uint32_t type_;
	uint32_t N_Tx;
	uint32_t mod;
	uint32_t number;
	uint32_t type;
};


#pragma pack(pop)

float temp_deg(unsigned long adc_value);
////////////////////////////////////////////////////
float SIGNAL(struct METROLOGY_GP *metrology, uint16_t n, uint16_t freq, float ro);

int32_t RO_ARG(struct METROLOGY_GP *metrology, struct GP_DATA *Data, uint16_t n, uint16_t freq);

uint16_t get_condition(struct GP_DATA *Data, struct METROLOGY_GP *metro, uint16_t freq, float mV);

uint16_t simmetry(struct GP_DATA *Data, struct METROLOGY_GP *metro, uint16_t freq, uint16_t condition);

ALL_DATA read_blok_all_data(ifstream &fin, int n);

CARTOGRAF_DATA_1FREQ read_blok_cartograf_data_1freq_Oleg(ifstream &fin, int n);

_4Tx_DATA read_blok_4Tx_data(ifstream &fin, int n);

_4Tx_DATA read_blok_4Tx_data_Oleg(ifstream &fin, int n);

uint16_t CRC16(const BYTE *nData, uint16_t wLength);

int send_to_tool_(System::IO::Ports::SerialPort^ COM, uint8_t CMD, uint8_t* DATA, uint16_t length);

int send_to_tool(System::IO::Ports::SerialPort^ COM, uint8_t CMD, uint8_t* DATA, uint16_t length);

int receive_from_tool_(System::IO::Ports::SerialPort^ COM, uint8_t CMD, uint8_t* DATA, uint16_t length);

int receive_from_tool(System::IO::Ports::SerialPort^ COM, uint8_t CMD, uint8_t* DATA, uint16_t *income_length);

void define_sonde_type(DATA_FILE_PARAM *param);

int signum(float val);

void calk_geo_sph(RAW_DATA *raw, GEO *result, complex <float> fourier_k[8][16], int N_rx);

void calk_geo_rph(RAW_DATA *raw, GEO *result, complex<float> fourier_k[8][16], int N_rx);

GP_DATA transform_data(variant<_4Tx_DATA, _5Tx_DATA, ALL_DATA > sonde_data, ALL_DATA* all_data);

METROLOGY_GP transform_metro(variant<METROLOGY_CARTOGRAPH, METROLOGY_4Tx, METROLOGY_5Tx > metro_data);

variant<METROLOGY_CARTOGRAPH, METROLOGY_4Tx, METROLOGY_5Tx >
    read_metro_from_tool(System::IO::Ports::SerialPort^ COM, uint32_t sonde_type, bool *read_status);

void write_metro_to_tool(System::IO::Ports::SerialPort^ COM,
	variant<METROLOGY_CARTOGRAPH, METROLOGY_4Tx, METROLOGY_5Tx> current_metro_struct, bool *write_status);

variant<_4Tx_DATA, _5Tx_DATA, ALL_DATA > read_blok(DATA_FILE_PARAM *param, int n);

_4Tx_DATA transform_GP_to_4Tx_data(GP_DATA *gp_data);

//функции для передачи в телесистему////////////////////////////////////////////////////////////
// вспомогательная функция распаковки УЭС из восьмибитного слова
float bits_code_to_ro(uint16_t code);

//вспомогательная функция сжатия УЭС в восьмибитное слово
uint16_t ro_to_bits_code(float ro);

void set_mean_bits(REAP_CONST *reap, TO_PACK *pack);
//перевод из структуры данных картографа в сжатую форму 
void compress(REAP_CONST *reap, DATA *data, COMPRESSED_1freq *c_data);

//перевод из сжатой формы в распакованную
void decompress(REAP_CONST *reap, COMPRESSED_1freq *c_data, DECOMPRESSED_1freq *d_data);

void out_compressed_data(DATA *data, METROLOGY_CARTOGRAPH *metro);

void receive_compressed_data(DECOMPRESSED_1freq *d_data, METROLOGY_CARTOGRAPH *metro, uint8_t* in_arr);

///////////////////////////////////////////////////////////////

void calck_geo_smt(DATA*data);

ID get_sonde_id(uint32_t signature);
