#pragma once
#include <fstream>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <complex> 
#include <string> 
#include "variable.h"
#include "function.h"
#include <typeinfo.h>
#include <string>
#include <sstream>
#include <msclr\marshal_cppstd.h>
//#include "winuser.h"
//#include <windows.h>
extern bool busy;
extern bool busy_read_from_sonde;
extern bool metro_is_readed_from_sonde;
extern bool sonde_on_off;
extern string Com_Name;
extern bool metro_check;
extern unsigned int baudrate;
//extern METROLOGY_CARTOGRAPH metrology_struct;
extern METROLOGY_GP metrology_GP_struct;
extern const char* Metrology;
extern bool metro_form_is_open;
extern int sonde_type;
extern REAP_CONST reap;
extern uint32_t global_selected_key;

namespace AIRTEMP10 {

	using namespace std;
	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;

	/// <summary>
	/// Сводка для MyForm1
	/// </summary>
	public ref class MyForm1 : public System::Windows::Forms::Form
	{
	public:

		MyForm1(void)
		{
			InitializeComponent();
			checkBox_read_only->Checked = true;
			comboBoxTBTindex->SelectedIndex = 0;
			comboBox_work_type->SelectedIndex = 0;
			//checkBox_raw->Checked = true;
			time_t timestamp;
			time(&timestamp);
			Timestamp = timestamp;
			//cout << Timestamp << " timestamp " << timestamp << endl; 
			sells_load(sonde_type);		

			


			//
			//TODO: добавьте код конструктора
			//
		}

	protected:
		/// <summary>
		/// Освободить все используемые ресурсы.
		/// </summary>
		~MyForm1()
		{
			if (components)
			{
				delete components;
			}
		}
	private: System::Windows::Forms::TableLayoutPanel^  tableLayoutPanel1;
	protected:
	private: System::Windows::Forms::Label^  labelT0;
	private: System::Windows::Forms::TextBox^  textBoxL1T1;
	private: System::Windows::Forms::TextBox^  textBoxL1T2;
	private: System::Windows::Forms::TextBox^  textBoxL1T3;
	private: System::Windows::Forms::TextBox^  textBoxL1T4;
	private: System::Windows::Forms::Label^  label1;
	private: System::Windows::Forms::Label^  label2;
	private: System::Windows::Forms::Label^  label3;
	private: System::Windows::Forms::Label^  label8;
	private: System::Windows::Forms::Label^  label9;
	private: System::Windows::Forms::Label^  label10;
	private: System::Windows::Forms::Label^  label11;
	private: System::Windows::Forms::TextBox^  textBoxL2T1;
	private: System::Windows::Forms::TextBox^  textBoxL2T2;
	private: System::Windows::Forms::TextBox^  textBoxL2T3;
	private: System::Windows::Forms::TextBox^  textBoxL2T4;
	private: System::Windows::Forms::Label^  label58;
	private: System::Windows::Forms::Label^  label60;
	private: System::Windows::Forms::TextBox^  textBoxL1T5;
	private: System::Windows::Forms::TextBox^  textBoxL2T5;
	private: System::Windows::Forms::Label^  label24;
	private: System::Windows::Forms::Label^  label25;
	private: System::Windows::Forms::Label^  label26;
	private: System::Windows::Forms::Label^  label27;
	private: System::Windows::Forms::Label^  label65;
	private: System::Windows::Forms::Label^  label28;
	private: System::Windows::Forms::Label^  label29;
	private: System::Windows::Forms::Label^  label30;
	private: System::Windows::Forms::Label^  label31;
	private: System::Windows::Forms::Label^  label64;
	private: System::Windows::Forms::TextBox^  textBoxAIRT1;
	private: System::Windows::Forms::TextBox^  textBoxAIRT2;
	private: System::Windows::Forms::TextBox^  textBoxAIRT3;
	private: System::Windows::Forms::TextBox^  textBoxAIRT4;
	private: System::Windows::Forms::TextBox^  textBoxAIRT5;
	private: System::Windows::Forms::TextBox^  textBoxAIRT6;
	private: System::Windows::Forms::TextBox^  textBoxAIRT7;
	private: System::Windows::Forms::TextBox^  textBoxAIRT8;
	private: System::Windows::Forms::TextBox^  textBoxAIRT9;
	private: System::Windows::Forms::TextBox^  textBoxAIRT10;
	private: System::Windows::Forms::Label^  label33;
	private: System::Windows::Forms::Label^  label34;
	private: System::Windows::Forms::Label^  label35;
	private: System::Windows::Forms::Label^  label36;
	private: System::Windows::Forms::Label^  label48;
	private: System::Windows::Forms::Label^  label49;
	private: System::Windows::Forms::Label^  label50;
	private: System::Windows::Forms::Label^  label51;
	private: System::Windows::Forms::Label^  label4;
	private: System::Windows::Forms::Label^  label5;
	private: System::Windows::Forms::TextBox^  textBox_min1;
	private: System::Windows::Forms::TextBox^  textBox_min2;
	private: System::Windows::Forms::TextBox^  textBox_min3;
	private: System::Windows::Forms::TextBox^  textBox_min5;
	private: System::Windows::Forms::TextBox^  textBox_min6;
	private: System::Windows::Forms::TextBox^  textBox_min7;
	private: System::Windows::Forms::TextBox^  textBox_min8;
	private: System::Windows::Forms::TextBox^  textBox_min9;
	private: System::Windows::Forms::TextBox^  textBox_min10;
	private: System::Windows::Forms::TextBox^  textBox_min4;
	private: System::Windows::Forms::Label^  label17;
	private: System::Windows::Forms::Label^  label16;
	private: System::Windows::Forms::TextBox^  textBoxFT2;
	private: System::Windows::Forms::TextBox^  textBoxFT1;
	private: System::Windows::Forms::Panel^  panel_inc;

	private: System::Windows::Forms::CheckBox^  checkBox_interupt;
	private: System::Windows::Forms::CheckBox^  checkBox_auto_delta;
	private: System::Windows::Forms::TextBox^  textBox_K_Delta;
	private: System::Windows::Forms::TextBox^  textBox_history_angle;
	private: System::Windows::Forms::TextBox^  textBox_APS_DELTA;
	private: System::Windows::Forms::TextBox^  textBox_K_predict;
	private: System::Windows::Forms::TextBox^  textBox_W_MAX;
	private: System::Windows::Forms::TextBox^  textBox_W_MIN;
	private: System::Windows::Forms::TextBox^  textBoxK;
	private: System::Windows::Forms::TextBox^  textBoxN;
	private: System::Windows::Forms::TextBox^  textBoxMLD;
	private: System::Windows::Forms::TextBox^  textBoxMA;
	private: System::Windows::Forms::Label^  label56;
	private: System::Windows::Forms::Label^  label55;
	private: System::Windows::Forms::Label^  label54;
	private: System::Windows::Forms::Label^  label53;
	private: System::Windows::Forms::Label^  label52;
	private: System::Windows::Forms::Label^  label;
	private: System::Windows::Forms::Label^  label4N;
	private: System::Windows::Forms::Label^  label4MLD;
	private: System::Windows::Forms::Label^  label46;
	private: System::Windows::Forms::Label^  label45;
	private: System::Windows::Forms::Panel^  panel9;
	private: System::Windows::Forms::Label^  label41;
	private: System::Windows::Forms::TextBox^  textBox_sign_for_pallete;
	private: System::Windows::Forms::Label^  label67;
	private: System::Windows::Forms::TextBox^  textBox_number;
	private: System::Windows::Forms::Label^  label66;
	private: System::Windows::Forms::ComboBox^  comboBox_mod;
	private: System::Windows::Forms::Label^  label38;
	private: System::Windows::Forms::ComboBox^  comboBox_type;
	private: System::Windows::Forms::Label^  label44;
	private: System::Windows::Forms::TextBox^  textBoxSer;
	private: System::Windows::Forms::TextBox^  textBox_sign32;
	private: System::Windows::Forms::Label^  label39;
	private: System::Windows::Forms::ComboBox^  comboBox_N_Tx;
	private: System::Windows::Forms::Label^  label37;
	private: System::Windows::Forms::OpenFileDialog^  openFileDialog1;
	private: System::Windows::Forms::SaveFileDialog^  saveFileDialog1;
	private: System::Windows::Forms::CheckBox^  checkBox_read_only;




	private: System::Windows::Forms::TableLayoutPanel^  tableLayoutPanel3;

	private: System::Windows::Forms::TextBox^  textBoxT6;
	private: System::Windows::Forms::TextBox^  textBoxT5;
	private: System::Windows::Forms::TextBox^  textBoxT3;
	private: System::Windows::Forms::TextBox^  textBoxT2;
	private: System::Windows::Forms::TextBox^  textBoxT1;
	private: System::Windows::Forms::TextBox^  textBoxT4;
	private: System::Windows::Forms::Button^  button3;
	private: System::Windows::Forms::Button^  button1;
	private: System::Windows::Forms::Button^  button4;
	private: System::Windows::Forms::Button^  button5;
	private: System::Windows::Forms::Label^  label7;
	private: System::Windows::Forms::Label^  label12;
	private: System::Windows::Forms::TextBox^  textBox_order;




	private: System::Windows::Forms::TextBox^  textBoxT7;
	private: System::Windows::Forms::Label^  label13;
	private: System::Windows::Forms::Label^  label14;
	private: System::Windows::Forms::Label^  label15;
	private: System::Windows::Forms::ComboBox^  comboBoxTBTindex;

	private: System::Windows::Forms::Label^  label18;
	private: System::Windows::Forms::Label^  label19;
	private: System::Windows::Forms::TextBox^  textBox_MinRotSpeed;
	private: System::Windows::Forms::CheckBox^  checkBox_debug_session;


	private: System::Windows::Forms::TextBox^  textBox_unix_epoh;

	private: System::Windows::Forms::Label^  label20;
	private: System::Windows::Forms::Button^  button_set_of_preset_from_sonde;
	private: System::Windows::Forms::Button^  button_set_of_preset_to_sonde;
private: System::Windows::Forms::Label^  label_set_of_persets;



	private: System::Windows::Forms::Label^  label22;
	private: System::Windows::Forms::TableLayoutPanel^  tableLayoutPanel2;
	private: System::Windows::Forms::TextBox^  textBoxT5S3;

	private: System::Windows::Forms::TextBox^  textBoxT5S2;

	private: System::Windows::Forms::TextBox^  textBoxT5S1;

	private: System::Windows::Forms::TextBox^  textBoxT4S3;

	private: System::Windows::Forms::TextBox^  textBoxT4S2;

	private: System::Windows::Forms::TextBox^  textBoxT4S1;

	private: System::Windows::Forms::TextBox^  textBoxT3S3;

	private: System::Windows::Forms::TextBox^  textBoxT3S2;

	private: System::Windows::Forms::TextBox^  textBoxT3S1;

	private: System::Windows::Forms::TextBox^  textBoxT2S3;

	private: System::Windows::Forms::TextBox^  textBoxT2S2;

	private: System::Windows::Forms::TextBox^  textBoxT2S1;

	private: System::Windows::Forms::TextBox^  textBoxT1S3;

	private: System::Windows::Forms::TextBox^  textBoxT1S2;

	private: System::Windows::Forms::TextBox^  textBoxT1S1;
	private: System::Windows::Forms::Label^  label23;


	private: System::Windows::Forms::TextBox^  textBox_rotation;



private: System::IO::Ports::SerialPort^  serialPort1;
private: System::Windows::Forms::Button^  button_set_air;
private: System::Windows::Forms::Panel^  panel_cartograph_sets;
private: System::Windows::Forms::Label^  label_rot;
private: System::Windows::Forms::Panel^  panel1;
private: System::Windows::Forms::Panel^  panel2;
private: System::Windows::Forms::Panel^  panel3;
private: System::Windows::Forms::CheckBox^  checkBox11;
private: System::Windows::Forms::CheckBox^  checkBox10;
private: System::Windows::Forms::CheckBox^  checkBox9;
private: System::Windows::Forms::CheckBox^  checkBox8;
private: System::Windows::Forms::CheckBox^  checkBox7;
private: System::Windows::Forms::CheckBox^  checkBox6;
private: System::Windows::Forms::CheckBox^  checkBox5;
private: System::Windows::Forms::CheckBox^  checkBox4;
private: System::Windows::Forms::CheckBox^  checkBox3;
private: System::Windows::Forms::CheckBox^  checkBox2;
private: System::Windows::Forms::CheckBox^  checkBox1;
private: System::Windows::Forms::CheckBox^  checkBox0;
private: System::Windows::Forms::CheckBox^  checkBox12;
private: System::Windows::Forms::CheckBox^  checkBox13;
private: System::Windows::Forms::CheckBox^  checkBox14;
private: System::Windows::Forms::CheckBox^  checkBox15;
private: System::Windows::Forms::CheckBox^  checkBox16;
private: System::Windows::Forms::CheckBox^  checkBox17;
private: System::Windows::Forms::CheckBox^  checkBox18;
private: System::Windows::Forms::CheckBox^  checkBox19;
private: System::Windows::Forms::CheckBox^  checkBox20;
private: System::Windows::Forms::CheckBox^  checkBox21;
private: System::Windows::Forms::CheckBox^  checkBox22;
private: System::Windows::Forms::CheckBox^  checkBox23;
private: System::Windows::Forms::TableLayoutPanel^  tableLayoutPanel_compress;
private: System::Windows::Forms::CheckBox^  checkBox25;
private: System::Windows::Forms::CheckBox^  checkBox24;
private: System::Windows::Forms::Button^  button_set_out;
private: System::Windows::Forms::ComboBox^  comboBox_work_type;
private: System::Windows::Forms::Label^  label6;





	private: System::ComponentModel::IContainer^  components;

	private:
		/// <summary>
		/// Обязательная переменная конструктора.
		/// </summary>


#pragma region Windows Form Designer generated code
		/// <summary>
		/// Требуемый метод для поддержки конструктора — не изменяйте 
		/// содержимое этого метода с помощью редактора кода.
		/// </summary>
		void InitializeComponent(void)
		{
			this->components = (gcnew System::ComponentModel::Container());
			this->tableLayoutPanel1 = (gcnew System::Windows::Forms::TableLayoutPanel());
			this->labelT0 = (gcnew System::Windows::Forms::Label());
			this->textBoxL1T1 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxL1T2 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxL1T3 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxL1T4 = (gcnew System::Windows::Forms::TextBox());
			this->label1 = (gcnew System::Windows::Forms::Label());
			this->label2 = (gcnew System::Windows::Forms::Label());
			this->label3 = (gcnew System::Windows::Forms::Label());
			this->label8 = (gcnew System::Windows::Forms::Label());
			this->label9 = (gcnew System::Windows::Forms::Label());
			this->label10 = (gcnew System::Windows::Forms::Label());
			this->label11 = (gcnew System::Windows::Forms::Label());
			this->textBoxL2T1 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxL2T2 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxL2T3 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxL2T4 = (gcnew System::Windows::Forms::TextBox());
			this->label58 = (gcnew System::Windows::Forms::Label());
			this->label60 = (gcnew System::Windows::Forms::Label());
			this->textBoxL1T5 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxL2T5 = (gcnew System::Windows::Forms::TextBox());
			this->label24 = (gcnew System::Windows::Forms::Label());
			this->label25 = (gcnew System::Windows::Forms::Label());
			this->label26 = (gcnew System::Windows::Forms::Label());
			this->label27 = (gcnew System::Windows::Forms::Label());
			this->label65 = (gcnew System::Windows::Forms::Label());
			this->label28 = (gcnew System::Windows::Forms::Label());
			this->label29 = (gcnew System::Windows::Forms::Label());
			this->label30 = (gcnew System::Windows::Forms::Label());
			this->label31 = (gcnew System::Windows::Forms::Label());
			this->label64 = (gcnew System::Windows::Forms::Label());
			this->textBoxAIRT1 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxAIRT2 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxAIRT3 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxAIRT4 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxAIRT5 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxAIRT6 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxAIRT7 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxAIRT8 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxAIRT9 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxAIRT10 = (gcnew System::Windows::Forms::TextBox());
			this->label33 = (gcnew System::Windows::Forms::Label());
			this->label34 = (gcnew System::Windows::Forms::Label());
			this->label35 = (gcnew System::Windows::Forms::Label());
			this->label36 = (gcnew System::Windows::Forms::Label());
			this->label48 = (gcnew System::Windows::Forms::Label());
			this->label49 = (gcnew System::Windows::Forms::Label());
			this->label50 = (gcnew System::Windows::Forms::Label());
			this->label51 = (gcnew System::Windows::Forms::Label());
			this->label4 = (gcnew System::Windows::Forms::Label());
			this->label5 = (gcnew System::Windows::Forms::Label());
			this->textBox_min1 = (gcnew System::Windows::Forms::TextBox());
			this->textBox_min2 = (gcnew System::Windows::Forms::TextBox());
			this->textBox_min3 = (gcnew System::Windows::Forms::TextBox());
			this->textBox_min5 = (gcnew System::Windows::Forms::TextBox());
			this->textBox_min6 = (gcnew System::Windows::Forms::TextBox());
			this->textBox_min7 = (gcnew System::Windows::Forms::TextBox());
			this->textBox_min8 = (gcnew System::Windows::Forms::TextBox());
			this->textBox_min9 = (gcnew System::Windows::Forms::TextBox());
			this->textBox_min10 = (gcnew System::Windows::Forms::TextBox());
			this->textBox_min4 = (gcnew System::Windows::Forms::TextBox());
			this->label17 = (gcnew System::Windows::Forms::Label());
			this->label16 = (gcnew System::Windows::Forms::Label());
			this->textBoxFT2 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxFT1 = (gcnew System::Windows::Forms::TextBox());
			this->panel_inc = (gcnew System::Windows::Forms::Panel());
			this->checkBox_interupt = (gcnew System::Windows::Forms::CheckBox());
			this->checkBox_auto_delta = (gcnew System::Windows::Forms::CheckBox());
			this->textBox_K_Delta = (gcnew System::Windows::Forms::TextBox());
			this->textBox_history_angle = (gcnew System::Windows::Forms::TextBox());
			this->textBox_APS_DELTA = (gcnew System::Windows::Forms::TextBox());
			this->textBox_K_predict = (gcnew System::Windows::Forms::TextBox());
			this->textBox_W_MAX = (gcnew System::Windows::Forms::TextBox());
			this->textBox_W_MIN = (gcnew System::Windows::Forms::TextBox());
			this->textBoxK = (gcnew System::Windows::Forms::TextBox());
			this->textBoxN = (gcnew System::Windows::Forms::TextBox());
			this->textBoxMLD = (gcnew System::Windows::Forms::TextBox());
			this->textBoxMA = (gcnew System::Windows::Forms::TextBox());
			this->label56 = (gcnew System::Windows::Forms::Label());
			this->label55 = (gcnew System::Windows::Forms::Label());
			this->label54 = (gcnew System::Windows::Forms::Label());
			this->label53 = (gcnew System::Windows::Forms::Label());
			this->label52 = (gcnew System::Windows::Forms::Label());
			this->label = (gcnew System::Windows::Forms::Label());
			this->label4N = (gcnew System::Windows::Forms::Label());
			this->label4MLD = (gcnew System::Windows::Forms::Label());
			this->label46 = (gcnew System::Windows::Forms::Label());
			this->label45 = (gcnew System::Windows::Forms::Label());
			this->panel9 = (gcnew System::Windows::Forms::Panel());
			this->comboBox_work_type = (gcnew System::Windows::Forms::ComboBox());
			this->label6 = (gcnew System::Windows::Forms::Label());
			this->label41 = (gcnew System::Windows::Forms::Label());
			this->textBox_sign_for_pallete = (gcnew System::Windows::Forms::TextBox());
			this->label67 = (gcnew System::Windows::Forms::Label());
			this->textBox_number = (gcnew System::Windows::Forms::TextBox());
			this->label66 = (gcnew System::Windows::Forms::Label());
			this->comboBox_mod = (gcnew System::Windows::Forms::ComboBox());
			this->label38 = (gcnew System::Windows::Forms::Label());
			this->comboBox_type = (gcnew System::Windows::Forms::ComboBox());
			this->label44 = (gcnew System::Windows::Forms::Label());
			this->textBoxSer = (gcnew System::Windows::Forms::TextBox());
			this->textBox_sign32 = (gcnew System::Windows::Forms::TextBox());
			this->label39 = (gcnew System::Windows::Forms::Label());
			this->comboBox_N_Tx = (gcnew System::Windows::Forms::ComboBox());
			this->label37 = (gcnew System::Windows::Forms::Label());
			this->openFileDialog1 = (gcnew System::Windows::Forms::OpenFileDialog());
			this->saveFileDialog1 = (gcnew System::Windows::Forms::SaveFileDialog());
			this->checkBox_read_only = (gcnew System::Windows::Forms::CheckBox());
			this->tableLayoutPanel3 = (gcnew System::Windows::Forms::TableLayoutPanel());
			this->textBoxT7 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxT6 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxT5 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxT3 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxT2 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxT4 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxT1 = (gcnew System::Windows::Forms::TextBox());
			this->button3 = (gcnew System::Windows::Forms::Button());
			this->button1 = (gcnew System::Windows::Forms::Button());
			this->button4 = (gcnew System::Windows::Forms::Button());
			this->button5 = (gcnew System::Windows::Forms::Button());
			this->label7 = (gcnew System::Windows::Forms::Label());
			this->label12 = (gcnew System::Windows::Forms::Label());
			this->textBox_order = (gcnew System::Windows::Forms::TextBox());
			this->label13 = (gcnew System::Windows::Forms::Label());
			this->label14 = (gcnew System::Windows::Forms::Label());
			this->label15 = (gcnew System::Windows::Forms::Label());
			this->comboBoxTBTindex = (gcnew System::Windows::Forms::ComboBox());
			this->label18 = (gcnew System::Windows::Forms::Label());
			this->label19 = (gcnew System::Windows::Forms::Label());
			this->textBox_MinRotSpeed = (gcnew System::Windows::Forms::TextBox());
			this->checkBox_debug_session = (gcnew System::Windows::Forms::CheckBox());
			this->textBox_unix_epoh = (gcnew System::Windows::Forms::TextBox());
			this->label20 = (gcnew System::Windows::Forms::Label());
			this->button_set_of_preset_from_sonde = (gcnew System::Windows::Forms::Button());
			this->button_set_of_preset_to_sonde = (gcnew System::Windows::Forms::Button());
			this->label_set_of_persets = (gcnew System::Windows::Forms::Label());
			this->label22 = (gcnew System::Windows::Forms::Label());
			this->tableLayoutPanel2 = (gcnew System::Windows::Forms::TableLayoutPanel());
			this->textBoxT5S1 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxT4S1 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxT3S1 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxT2S1 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxT1S1 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxT5S2 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxT4S2 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxT2S2 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxT1S2 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxT5S3 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxT4S3 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxT3S3 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxT2S3 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxT1S3 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxT3S2 = (gcnew System::Windows::Forms::TextBox());
			this->label23 = (gcnew System::Windows::Forms::Label());
			this->textBox_rotation = (gcnew System::Windows::Forms::TextBox());
			this->serialPort1 = (gcnew System::IO::Ports::SerialPort(this->components));
			this->button_set_air = (gcnew System::Windows::Forms::Button());
			this->panel_cartograph_sets = (gcnew System::Windows::Forms::Panel());
			this->panel1 = (gcnew System::Windows::Forms::Panel());
			this->label_rot = (gcnew System::Windows::Forms::Label());
			this->panel2 = (gcnew System::Windows::Forms::Panel());
			this->panel3 = (gcnew System::Windows::Forms::Panel());
			this->checkBox11 = (gcnew System::Windows::Forms::CheckBox());
			this->checkBox10 = (gcnew System::Windows::Forms::CheckBox());
			this->checkBox9 = (gcnew System::Windows::Forms::CheckBox());
			this->checkBox8 = (gcnew System::Windows::Forms::CheckBox());
			this->checkBox7 = (gcnew System::Windows::Forms::CheckBox());
			this->checkBox6 = (gcnew System::Windows::Forms::CheckBox());
			this->checkBox5 = (gcnew System::Windows::Forms::CheckBox());
			this->checkBox4 = (gcnew System::Windows::Forms::CheckBox());
			this->checkBox3 = (gcnew System::Windows::Forms::CheckBox());
			this->checkBox2 = (gcnew System::Windows::Forms::CheckBox());
			this->checkBox1 = (gcnew System::Windows::Forms::CheckBox());
			this->checkBox0 = (gcnew System::Windows::Forms::CheckBox());
			this->checkBox12 = (gcnew System::Windows::Forms::CheckBox());
			this->checkBox13 = (gcnew System::Windows::Forms::CheckBox());
			this->checkBox14 = (gcnew System::Windows::Forms::CheckBox());
			this->checkBox15 = (gcnew System::Windows::Forms::CheckBox());
			this->checkBox16 = (gcnew System::Windows::Forms::CheckBox());
			this->checkBox17 = (gcnew System::Windows::Forms::CheckBox());
			this->checkBox18 = (gcnew System::Windows::Forms::CheckBox());
			this->checkBox19 = (gcnew System::Windows::Forms::CheckBox());
			this->checkBox20 = (gcnew System::Windows::Forms::CheckBox());
			this->checkBox21 = (gcnew System::Windows::Forms::CheckBox());
			this->checkBox22 = (gcnew System::Windows::Forms::CheckBox());
			this->checkBox23 = (gcnew System::Windows::Forms::CheckBox());
			this->tableLayoutPanel_compress = (gcnew System::Windows::Forms::TableLayoutPanel());
			this->checkBox25 = (gcnew System::Windows::Forms::CheckBox());
			this->checkBox24 = (gcnew System::Windows::Forms::CheckBox());
			this->button_set_out = (gcnew System::Windows::Forms::Button());
			this->tableLayoutPanel1->SuspendLayout();
			this->panel_inc->SuspendLayout();
			this->panel9->SuspendLayout();
			this->tableLayoutPanel3->SuspendLayout();
			this->tableLayoutPanel2->SuspendLayout();
			this->panel_cartograph_sets->SuspendLayout();
			this->panel1->SuspendLayout();
			this->panel2->SuspendLayout();
			this->panel3->SuspendLayout();
			this->tableLayoutPanel_compress->SuspendLayout();
			this->SuspendLayout();
			// 
			// tableLayoutPanel1
			// 
			this->tableLayoutPanel1->CellBorderStyle = System::Windows::Forms::TableLayoutPanelCellBorderStyle::Single;
			this->tableLayoutPanel1->ColumnCount = 8;
			this->tableLayoutPanel1->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Absolute,
				107)));
			this->tableLayoutPanel1->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Absolute,
				75)));
			this->tableLayoutPanel1->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Absolute,
				73)));
			this->tableLayoutPanel1->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Absolute,
				75)));
			this->tableLayoutPanel1->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Absolute,
				101)));
			this->tableLayoutPanel1->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Absolute,
				85)));
			this->tableLayoutPanel1->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Absolute,
				147)));
			this->tableLayoutPanel1->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Absolute,
				561)));
			this->tableLayoutPanel1->Controls->Add(this->labelT0, 0, 0);
			this->tableLayoutPanel1->Controls->Add(this->textBoxL1T1, 1, 0);
			this->tableLayoutPanel1->Controls->Add(this->textBoxL1T2, 1, 1);
			this->tableLayoutPanel1->Controls->Add(this->textBoxL1T3, 1, 2);
			this->tableLayoutPanel1->Controls->Add(this->textBoxL1T4, 1, 3);
			this->tableLayoutPanel1->Controls->Add(this->label1, 0, 1);
			this->tableLayoutPanel1->Controls->Add(this->label2, 0, 2);
			this->tableLayoutPanel1->Controls->Add(this->label3, 0, 3);
			this->tableLayoutPanel1->Controls->Add(this->label8, 2, 0);
			this->tableLayoutPanel1->Controls->Add(this->label9, 2, 1);
			this->tableLayoutPanel1->Controls->Add(this->label10, 2, 2);
			this->tableLayoutPanel1->Controls->Add(this->label11, 2, 3);
			this->tableLayoutPanel1->Controls->Add(this->textBoxL2T1, 3, 0);
			this->tableLayoutPanel1->Controls->Add(this->textBoxL2T2, 3, 1);
			this->tableLayoutPanel1->Controls->Add(this->textBoxL2T3, 3, 2);
			this->tableLayoutPanel1->Controls->Add(this->textBoxL2T4, 3, 3);
			this->tableLayoutPanel1->Controls->Add(this->label58, 0, 4);
			this->tableLayoutPanel1->Controls->Add(this->label60, 2, 4);
			this->tableLayoutPanel1->Controls->Add(this->textBoxL1T5, 1, 4);
			this->tableLayoutPanel1->Controls->Add(this->textBoxL2T5, 3, 4);
			this->tableLayoutPanel1->Controls->Add(this->label24, 4, 0);
			this->tableLayoutPanel1->Controls->Add(this->label25, 4, 1);
			this->tableLayoutPanel1->Controls->Add(this->label26, 4, 2);
			this->tableLayoutPanel1->Controls->Add(this->label27, 4, 3);
			this->tableLayoutPanel1->Controls->Add(this->label65, 4, 4);
			this->tableLayoutPanel1->Controls->Add(this->label28, 4, 5);
			this->tableLayoutPanel1->Controls->Add(this->label29, 4, 6);
			this->tableLayoutPanel1->Controls->Add(this->label30, 4, 7);
			this->tableLayoutPanel1->Controls->Add(this->label31, 4, 8);
			this->tableLayoutPanel1->Controls->Add(this->label64, 4, 9);
			this->tableLayoutPanel1->Controls->Add(this->textBoxAIRT1, 5, 0);
			this->tableLayoutPanel1->Controls->Add(this->textBoxAIRT2, 5, 1);
			this->tableLayoutPanel1->Controls->Add(this->textBoxAIRT3, 5, 2);
			this->tableLayoutPanel1->Controls->Add(this->textBoxAIRT4, 5, 3);
			this->tableLayoutPanel1->Controls->Add(this->textBoxAIRT5, 5, 4);
			this->tableLayoutPanel1->Controls->Add(this->textBoxAIRT6, 5, 5);
			this->tableLayoutPanel1->Controls->Add(this->textBoxAIRT7, 5, 6);
			this->tableLayoutPanel1->Controls->Add(this->textBoxAIRT8, 5, 7);
			this->tableLayoutPanel1->Controls->Add(this->textBoxAIRT9, 5, 8);
			this->tableLayoutPanel1->Controls->Add(this->textBoxAIRT10, 5, 9);
			this->tableLayoutPanel1->Controls->Add(this->label33, 6, 0);
			this->tableLayoutPanel1->Controls->Add(this->label34, 6, 1);
			this->tableLayoutPanel1->Controls->Add(this->label35, 6, 2);
			this->tableLayoutPanel1->Controls->Add(this->label36, 6, 3);
			this->tableLayoutPanel1->Controls->Add(this->label48, 6, 4);
			this->tableLayoutPanel1->Controls->Add(this->label49, 6, 5);
			this->tableLayoutPanel1->Controls->Add(this->label50, 6, 6);
			this->tableLayoutPanel1->Controls->Add(this->label51, 6, 7);
			this->tableLayoutPanel1->Controls->Add(this->label4, 6, 8);
			this->tableLayoutPanel1->Controls->Add(this->label5, 6, 9);
			this->tableLayoutPanel1->Controls->Add(this->textBox_min1, 7, 0);
			this->tableLayoutPanel1->Controls->Add(this->textBox_min2, 7, 1);
			this->tableLayoutPanel1->Controls->Add(this->textBox_min3, 7, 2);
			this->tableLayoutPanel1->Controls->Add(this->textBox_min5, 7, 4);
			this->tableLayoutPanel1->Controls->Add(this->textBox_min6, 7, 5);
			this->tableLayoutPanel1->Controls->Add(this->textBox_min7, 7, 6);
			this->tableLayoutPanel1->Controls->Add(this->textBox_min8, 7, 7);
			this->tableLayoutPanel1->Controls->Add(this->textBox_min9, 7, 8);
			this->tableLayoutPanel1->Controls->Add(this->textBox_min10, 7, 9);
			this->tableLayoutPanel1->Controls->Add(this->textBox_min4, 7, 3);
			this->tableLayoutPanel1->Controls->Add(this->label17, 0, 7);
			this->tableLayoutPanel1->Controls->Add(this->label16, 0, 6);
			this->tableLayoutPanel1->Controls->Add(this->textBoxFT2, 1, 7);
			this->tableLayoutPanel1->Controls->Add(this->textBoxFT1, 1, 6);
			this->tableLayoutPanel1->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->tableLayoutPanel1->Location = System::Drawing::Point(12, 42);
			this->tableLayoutPanel1->Margin = System::Windows::Forms::Padding(4, 2, 4, 2);
			this->tableLayoutPanel1->Name = L"tableLayoutPanel1";
			this->tableLayoutPanel1->RowCount = 10;
			this->tableLayoutPanel1->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Percent, 10)));
			this->tableLayoutPanel1->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Percent, 10)));
			this->tableLayoutPanel1->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Percent, 10)));
			this->tableLayoutPanel1->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Percent, 10)));
			this->tableLayoutPanel1->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Percent, 10)));
			this->tableLayoutPanel1->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Percent, 10)));
			this->tableLayoutPanel1->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Percent, 10)));
			this->tableLayoutPanel1->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Percent, 10)));
			this->tableLayoutPanel1->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Percent, 10)));
			this->tableLayoutPanel1->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Percent, 10)));
			this->tableLayoutPanel1->Size = System::Drawing::Size(761, 290);
			this->tableLayoutPanel1->TabIndex = 1;
			// 
			// labelT0
			// 
			this->labelT0->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->labelT0->AutoSize = true;
			this->labelT0->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->labelT0->Location = System::Drawing::Point(5, 1);
			this->labelT0->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->labelT0->Name = L"labelT0";
			this->labelT0->Size = System::Drawing::Size(99, 18);
			this->labelT0->TabIndex = 0;
			this->labelT0->Text = L"T1_L1_mm";
			// 
			// textBoxL1T1
			// 
			this->textBoxL1T1->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->textBoxL1T1->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->textBoxL1T1->Location = System::Drawing::Point(113, 3);
			this->textBoxL1T1->Margin = System::Windows::Forms::Padding(4, 2, 4, 2);
			this->textBoxL1T1->Name = L"textBoxL1T1";
			this->textBoxL1T1->Size = System::Drawing::Size(67, 24);
			this->textBoxL1T1->TabIndex = 1;
			// 
			// textBoxL1T2
			// 
			this->textBoxL1T2->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->textBoxL1T2->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->textBoxL1T2->Location = System::Drawing::Point(113, 31);
			this->textBoxL1T2->Margin = System::Windows::Forms::Padding(4, 2, 4, 2);
			this->textBoxL1T2->Name = L"textBoxL1T2";
			this->textBoxL1T2->Size = System::Drawing::Size(67, 24);
			this->textBoxL1T2->TabIndex = 2;
			// 
			// textBoxL1T3
			// 
			this->textBoxL1T3->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->textBoxL1T3->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->textBoxL1T3->Location = System::Drawing::Point(113, 59);
			this->textBoxL1T3->Margin = System::Windows::Forms::Padding(4, 2, 4, 2);
			this->textBoxL1T3->Name = L"textBoxL1T3";
			this->textBoxL1T3->Size = System::Drawing::Size(67, 24);
			this->textBoxL1T3->TabIndex = 3;
			// 
			// textBoxL1T4
			// 
			this->textBoxL1T4->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->textBoxL1T4->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->textBoxL1T4->Location = System::Drawing::Point(113, 87);
			this->textBoxL1T4->Margin = System::Windows::Forms::Padding(4, 2, 4, 2);
			this->textBoxL1T4->Name = L"textBoxL1T4";
			this->textBoxL1T4->Size = System::Drawing::Size(67, 24);
			this->textBoxL1T4->TabIndex = 4;
			// 
			// label1
			// 
			this->label1->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->label1->AutoSize = true;
			this->label1->Location = System::Drawing::Point(5, 29);
			this->label1->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label1->Name = L"label1";
			this->label1->Size = System::Drawing::Size(99, 18);
			this->label1->TabIndex = 9;
			this->label1->Text = L"T2 _L1_mm";
			// 
			// label2
			// 
			this->label2->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->label2->AutoSize = true;
			this->label2->Location = System::Drawing::Point(5, 57);
			this->label2->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label2->Name = L"label2";
			this->label2->Size = System::Drawing::Size(99, 18);
			this->label2->TabIndex = 10;
			this->label2->Text = L"T3_L1_mm";
			// 
			// label3
			// 
			this->label3->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->label3->AutoSize = true;
			this->label3->Location = System::Drawing::Point(5, 85);
			this->label3->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label3->Name = L"label3";
			this->label3->Size = System::Drawing::Size(99, 18);
			this->label3->TabIndex = 11;
			this->label3->Text = L"T4 _L1_mm";
			// 
			// label8
			// 
			this->label8->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->label8->AutoSize = true;
			this->label8->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->label8->Location = System::Drawing::Point(189, 1);
			this->label8->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label8->Name = L"label8";
			this->label8->Size = System::Drawing::Size(65, 18);
			this->label8->TabIndex = 16;
			this->label8->Text = L"L2_mm";
			// 
			// label9
			// 
			this->label9->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->label9->AutoSize = true;
			this->label9->Location = System::Drawing::Point(189, 29);
			this->label9->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label9->Name = L"label9";
			this->label9->Size = System::Drawing::Size(65, 18);
			this->label9->TabIndex = 17;
			this->label9->Text = L"L2_mm";
			// 
			// label10
			// 
			this->label10->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->label10->AutoSize = true;
			this->label10->Location = System::Drawing::Point(189, 57);
			this->label10->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label10->Name = L"label10";
			this->label10->Size = System::Drawing::Size(65, 18);
			this->label10->TabIndex = 18;
			this->label10->Text = L"L2_mm";
			// 
			// label11
			// 
			this->label11->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->label11->AutoSize = true;
			this->label11->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->label11->Location = System::Drawing::Point(189, 85);
			this->label11->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label11->Name = L"label11";
			this->label11->Size = System::Drawing::Size(65, 18);
			this->label11->TabIndex = 19;
			this->label11->Text = L"L2_mm";
			// 
			// textBoxL2T1
			// 
			this->textBoxL2T1->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->textBoxL2T1->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->textBoxL2T1->Location = System::Drawing::Point(263, 3);
			this->textBoxL2T1->Margin = System::Windows::Forms::Padding(4, 2, 4, 2);
			this->textBoxL2T1->Name = L"textBoxL2T1";
			this->textBoxL2T1->Size = System::Drawing::Size(67, 24);
			this->textBoxL2T1->TabIndex = 24;
			// 
			// textBoxL2T2
			// 
			this->textBoxL2T2->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->textBoxL2T2->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->textBoxL2T2->Location = System::Drawing::Point(263, 31);
			this->textBoxL2T2->Margin = System::Windows::Forms::Padding(4, 2, 4, 2);
			this->textBoxL2T2->Name = L"textBoxL2T2";
			this->textBoxL2T2->Size = System::Drawing::Size(67, 24);
			this->textBoxL2T2->TabIndex = 25;
			// 
			// textBoxL2T3
			// 
			this->textBoxL2T3->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->textBoxL2T3->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->textBoxL2T3->Location = System::Drawing::Point(263, 59);
			this->textBoxL2T3->Margin = System::Windows::Forms::Padding(4, 2, 4, 2);
			this->textBoxL2T3->Name = L"textBoxL2T3";
			this->textBoxL2T3->Size = System::Drawing::Size(67, 24);
			this->textBoxL2T3->TabIndex = 26;
			// 
			// textBoxL2T4
			// 
			this->textBoxL2T4->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->textBoxL2T4->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->textBoxL2T4->Location = System::Drawing::Point(263, 87);
			this->textBoxL2T4->Margin = System::Windows::Forms::Padding(4, 2, 4, 2);
			this->textBoxL2T4->Name = L"textBoxL2T4";
			this->textBoxL2T4->Size = System::Drawing::Size(67, 24);
			this->textBoxL2T4->TabIndex = 27;
			// 
			// label58
			// 
			this->label58->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->label58->AutoSize = true;
			this->label58->Location = System::Drawing::Point(5, 113);
			this->label58->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label58->Name = L"label58";
			this->label58->Size = System::Drawing::Size(99, 18);
			this->label58->TabIndex = 144;
			this->label58->Text = L"T5 _L1_mm";
			// 
			// label60
			// 
			this->label60->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->label60->AutoSize = true;
			this->label60->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->label60->Location = System::Drawing::Point(189, 113);
			this->label60->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label60->Name = L"label60";
			this->label60->Size = System::Drawing::Size(65, 18);
			this->label60->TabIndex = 146;
			this->label60->Text = L"L2_mm";
			// 
			// textBoxL1T5
			// 
			this->textBoxL1T5->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->textBoxL1T5->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->textBoxL1T5->Location = System::Drawing::Point(113, 115);
			this->textBoxL1T5->Margin = System::Windows::Forms::Padding(4, 2, 4, 2);
			this->textBoxL1T5->Name = L"textBoxL1T5";
			this->textBoxL1T5->Size = System::Drawing::Size(67, 24);
			this->textBoxL1T5->TabIndex = 152;
			// 
			// textBoxL2T5
			// 
			this->textBoxL2T5->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->textBoxL2T5->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->textBoxL2T5->Location = System::Drawing::Point(263, 115);
			this->textBoxL2T5->Margin = System::Windows::Forms::Padding(4, 2, 4, 2);
			this->textBoxL2T5->Name = L"textBoxL2T5";
			this->textBoxL2T5->Size = System::Drawing::Size(67, 24);
			this->textBoxL2T5->TabIndex = 153;
			// 
			// label24
			// 
			this->label24->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->label24->AutoSize = true;
			this->label24->Location = System::Drawing::Point(339, 1);
			this->label24->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label24->Name = L"label24";
			this->label24->Size = System::Drawing::Size(93, 18);
			this->label24->TabIndex = 48;
			this->label24->Text = L"AIR_1_F_1";
			// 
			// label25
			// 
			this->label25->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->label25->AutoSize = true;
			this->label25->Location = System::Drawing::Point(339, 29);
			this->label25->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label25->Name = L"label25";
			this->label25->Size = System::Drawing::Size(93, 18);
			this->label25->TabIndex = 49;
			this->label25->Text = L"AIR_2_F_1";
			// 
			// label26
			// 
			this->label26->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->label26->AutoSize = true;
			this->label26->Location = System::Drawing::Point(339, 57);
			this->label26->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label26->Name = L"label26";
			this->label26->Size = System::Drawing::Size(93, 18);
			this->label26->TabIndex = 50;
			this->label26->Text = L"AIR_3_F_1";
			// 
			// label27
			// 
			this->label27->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->label27->AutoSize = true;
			this->label27->Location = System::Drawing::Point(339, 85);
			this->label27->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label27->Name = L"label27";
			this->label27->Size = System::Drawing::Size(93, 18);
			this->label27->TabIndex = 51;
			this->label27->Text = L"AIR_3_F_1";
			// 
			// label65
			// 
			this->label65->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->label65->AutoSize = true;
			this->label65->Location = System::Drawing::Point(339, 113);
			this->label65->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label65->Name = L"label65";
			this->label65->Size = System::Drawing::Size(93, 18);
			this->label65->TabIndex = 151;
			this->label65->Text = L"AIR_4_F_1";
			// 
			// label28
			// 
			this->label28->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->label28->AutoSize = true;
			this->label28->Location = System::Drawing::Point(339, 141);
			this->label28->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label28->Name = L"label28";
			this->label28->Size = System::Drawing::Size(93, 18);
			this->label28->TabIndex = 52;
			this->label28->Text = L"AIR_1_F_2";
			// 
			// label29
			// 
			this->label29->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->label29->AutoSize = true;
			this->label29->Location = System::Drawing::Point(339, 169);
			this->label29->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label29->Name = L"label29";
			this->label29->Size = System::Drawing::Size(93, 18);
			this->label29->TabIndex = 53;
			this->label29->Text = L"AIR_2_F_2";
			// 
			// label30
			// 
			this->label30->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->label30->AutoSize = true;
			this->label30->Location = System::Drawing::Point(339, 197);
			this->label30->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label30->Name = L"label30";
			this->label30->Size = System::Drawing::Size(93, 18);
			this->label30->TabIndex = 54;
			this->label30->Text = L"AIR_3_F_2";
			// 
			// label31
			// 
			this->label31->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->label31->AutoSize = true;
			this->label31->Location = System::Drawing::Point(339, 225);
			this->label31->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label31->Name = L"label31";
			this->label31->Size = System::Drawing::Size(93, 18);
			this->label31->TabIndex = 55;
			this->label31->Text = L"AIR_4_F_2";
			// 
			// label64
			// 
			this->label64->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->label64->AutoSize = true;
			this->label64->Location = System::Drawing::Point(339, 253);
			this->label64->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label64->Name = L"label64";
			this->label64->Size = System::Drawing::Size(93, 18);
			this->label64->TabIndex = 150;
			this->label64->Text = L"AIR_5_F_2";
			// 
			// textBoxAIRT1
			// 
			this->textBoxAIRT1->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->textBoxAIRT1->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->textBoxAIRT1->Location = System::Drawing::Point(441, 3);
			this->textBoxAIRT1->Margin = System::Windows::Forms::Padding(4, 2, 4, 2);
			this->textBoxAIRT1->Name = L"textBoxAIRT1";
			this->textBoxAIRT1->Size = System::Drawing::Size(77, 24);
			this->textBoxAIRT1->TabIndex = 56;
			// 
			// textBoxAIRT2
			// 
			this->textBoxAIRT2->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->textBoxAIRT2->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->textBoxAIRT2->Location = System::Drawing::Point(441, 31);
			this->textBoxAIRT2->Margin = System::Windows::Forms::Padding(4, 2, 4, 2);
			this->textBoxAIRT2->Name = L"textBoxAIRT2";
			this->textBoxAIRT2->Size = System::Drawing::Size(77, 24);
			this->textBoxAIRT2->TabIndex = 57;
			// 
			// textBoxAIRT3
			// 
			this->textBoxAIRT3->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->textBoxAIRT3->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->textBoxAIRT3->Location = System::Drawing::Point(441, 59);
			this->textBoxAIRT3->Margin = System::Windows::Forms::Padding(4, 2, 4, 2);
			this->textBoxAIRT3->Name = L"textBoxAIRT3";
			this->textBoxAIRT3->Size = System::Drawing::Size(77, 24);
			this->textBoxAIRT3->TabIndex = 58;
			// 
			// textBoxAIRT4
			// 
			this->textBoxAIRT4->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->textBoxAIRT4->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->textBoxAIRT4->Location = System::Drawing::Point(441, 87);
			this->textBoxAIRT4->Margin = System::Windows::Forms::Padding(4, 2, 4, 2);
			this->textBoxAIRT4->Name = L"textBoxAIRT4";
			this->textBoxAIRT4->Size = System::Drawing::Size(77, 24);
			this->textBoxAIRT4->TabIndex = 59;
			// 
			// textBoxAIRT5
			// 
			this->textBoxAIRT5->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->textBoxAIRT5->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->textBoxAIRT5->Location = System::Drawing::Point(441, 115);
			this->textBoxAIRT5->Margin = System::Windows::Forms::Padding(4, 2, 4, 2);
			this->textBoxAIRT5->Name = L"textBoxAIRT5";
			this->textBoxAIRT5->Size = System::Drawing::Size(77, 24);
			this->textBoxAIRT5->TabIndex = 155;
			// 
			// textBoxAIRT6
			// 
			this->textBoxAIRT6->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->textBoxAIRT6->Location = System::Drawing::Point(441, 143);
			this->textBoxAIRT6->Margin = System::Windows::Forms::Padding(4, 2, 4, 2);
			this->textBoxAIRT6->Name = L"textBoxAIRT6";
			this->textBoxAIRT6->Size = System::Drawing::Size(77, 24);
			this->textBoxAIRT6->TabIndex = 41;
			// 
			// textBoxAIRT7
			// 
			this->textBoxAIRT7->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->textBoxAIRT7->Location = System::Drawing::Point(441, 171);
			this->textBoxAIRT7->Margin = System::Windows::Forms::Padding(4, 2, 4, 2);
			this->textBoxAIRT7->Name = L"textBoxAIRT7";
			this->textBoxAIRT7->Size = System::Drawing::Size(77, 24);
			this->textBoxAIRT7->TabIndex = 60;
			// 
			// textBoxAIRT8
			// 
			this->textBoxAIRT8->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->textBoxAIRT8->Location = System::Drawing::Point(441, 199);
			this->textBoxAIRT8->Margin = System::Windows::Forms::Padding(4, 2, 4, 2);
			this->textBoxAIRT8->Name = L"textBoxAIRT8";
			this->textBoxAIRT8->Size = System::Drawing::Size(77, 24);
			this->textBoxAIRT8->TabIndex = 61;
			// 
			// textBoxAIRT9
			// 
			this->textBoxAIRT9->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->textBoxAIRT9->Location = System::Drawing::Point(441, 227);
			this->textBoxAIRT9->Margin = System::Windows::Forms::Padding(4, 2, 4, 2);
			this->textBoxAIRT9->Name = L"textBoxAIRT9";
			this->textBoxAIRT9->Size = System::Drawing::Size(77, 24);
			this->textBoxAIRT9->TabIndex = 62;
			// 
			// textBoxAIRT10
			// 
			this->textBoxAIRT10->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->textBoxAIRT10->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->textBoxAIRT10->Location = System::Drawing::Point(441, 255);
			this->textBoxAIRT10->Margin = System::Windows::Forms::Padding(4, 2, 4, 2);
			this->textBoxAIRT10->Name = L"textBoxAIRT10";
			this->textBoxAIRT10->Size = System::Drawing::Size(77, 24);
			this->textBoxAIRT10->TabIndex = 159;
			// 
			// label33
			// 
			this->label33->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->label33->AutoSize = true;
			this->label33->Location = System::Drawing::Point(527, 1);
			this->label33->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label33->Name = L"label33";
			this->label33->Size = System::Drawing::Size(139, 18);
			this->label33->TabIndex = 66;
			this->label33->Text = L"MIN_R2_T1_F_1";
			// 
			// label34
			// 
			this->label34->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->label34->AutoSize = true;
			this->label34->Location = System::Drawing::Point(527, 29);
			this->label34->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label34->Name = L"label34";
			this->label34->Size = System::Drawing::Size(139, 18);
			this->label34->TabIndex = 67;
			this->label34->Text = L"MIN_R1_T2_F_1";
			// 
			// label35
			// 
			this->label35->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->label35->AutoSize = true;
			this->label35->Location = System::Drawing::Point(527, 57);
			this->label35->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label35->Name = L"label35";
			this->label35->Size = System::Drawing::Size(139, 18);
			this->label35->TabIndex = 68;
			this->label35->Text = L"MIN_R2_T3_F_1";
			// 
			// label36
			// 
			this->label36->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->label36->AutoSize = true;
			this->label36->Location = System::Drawing::Point(527, 85);
			this->label36->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label36->Name = L"label36";
			this->label36->Size = System::Drawing::Size(139, 18);
			this->label36->TabIndex = 69;
			this->label36->Text = L"MIN_R1_T4_F_1";
			// 
			// label48
			// 
			this->label48->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->label48->AutoSize = true;
			this->label48->Location = System::Drawing::Point(527, 113);
			this->label48->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label48->Name = L"label48";
			this->label48->Size = System::Drawing::Size(139, 18);
			this->label48->TabIndex = 70;
			this->label48->Text = L"MIN_R2_T5_F_1";
			// 
			// label49
			// 
			this->label49->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->label49->AutoSize = true;
			this->label49->Location = System::Drawing::Point(527, 141);
			this->label49->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label49->Name = L"label49";
			this->label49->Size = System::Drawing::Size(139, 18);
			this->label49->TabIndex = 71;
			this->label49->Text = L"MIN_R2_T1_F_2";
			// 
			// label50
			// 
			this->label50->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->label50->AutoSize = true;
			this->label50->Location = System::Drawing::Point(527, 169);
			this->label50->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label50->Name = L"label50";
			this->label50->Size = System::Drawing::Size(139, 18);
			this->label50->TabIndex = 72;
			this->label50->Text = L"MIN_R1_T2_F_2";
			// 
			// label51
			// 
			this->label51->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->label51->AutoSize = true;
			this->label51->Location = System::Drawing::Point(527, 197);
			this->label51->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label51->Name = L"label51";
			this->label51->Size = System::Drawing::Size(139, 18);
			this->label51->TabIndex = 73;
			this->label51->Text = L"MIN_R2_T3_F_2";
			// 
			// label4
			// 
			this->label4->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->label4->AutoSize = true;
			this->label4->Location = System::Drawing::Point(527, 225);
			this->label4->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label4->Name = L"label4";
			this->label4->Size = System::Drawing::Size(139, 18);
			this->label4->TabIndex = 160;
			this->label4->Text = L"MIN_R1_T4_F_2";
			// 
			// label5
			// 
			this->label5->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->label5->AutoSize = true;
			this->label5->Location = System::Drawing::Point(527, 253);
			this->label5->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label5->Name = L"label5";
			this->label5->Size = System::Drawing::Size(139, 18);
			this->label5->TabIndex = 161;
			this->label5->Text = L"MIN_R2_T5_F_2";
			// 
			// textBox_min1
			// 
			this->textBox_min1->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->textBox_min1->Location = System::Drawing::Point(675, 3);
			this->textBox_min1->Margin = System::Windows::Forms::Padding(4, 2, 4, 2);
			this->textBox_min1->Name = L"textBox_min1";
			this->textBox_min1->Size = System::Drawing::Size(80, 24);
			this->textBox_min1->TabIndex = 63;
			// 
			// textBox_min2
			// 
			this->textBox_min2->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->textBox_min2->Location = System::Drawing::Point(675, 31);
			this->textBox_min2->Margin = System::Windows::Forms::Padding(4, 2, 4, 2);
			this->textBox_min2->Name = L"textBox_min2";
			this->textBox_min2->Size = System::Drawing::Size(80, 24);
			this->textBox_min2->TabIndex = 64;
			// 
			// textBox_min3
			// 
			this->textBox_min3->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->textBox_min3->Location = System::Drawing::Point(675, 59);
			this->textBox_min3->Margin = System::Windows::Forms::Padding(4, 2, 4, 2);
			this->textBox_min3->Name = L"textBox_min3";
			this->textBox_min3->Size = System::Drawing::Size(80, 24);
			this->textBox_min3->TabIndex = 65;
			// 
			// textBox_min5
			// 
			this->textBox_min5->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->textBox_min5->Location = System::Drawing::Point(675, 115);
			this->textBox_min5->Margin = System::Windows::Forms::Padding(4, 2, 4, 2);
			this->textBox_min5->Name = L"textBox_min5";
			this->textBox_min5->Size = System::Drawing::Size(80, 24);
			this->textBox_min5->TabIndex = 75;
			// 
			// textBox_min6
			// 
			this->textBox_min6->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->textBox_min6->Location = System::Drawing::Point(675, 143);
			this->textBox_min6->Margin = System::Windows::Forms::Padding(4, 2, 4, 2);
			this->textBox_min6->Name = L"textBox_min6";
			this->textBox_min6->Size = System::Drawing::Size(80, 24);
			this->textBox_min6->TabIndex = 76;
			// 
			// textBox_min7
			// 
			this->textBox_min7->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->textBox_min7->Location = System::Drawing::Point(675, 171);
			this->textBox_min7->Margin = System::Windows::Forms::Padding(4, 2, 4, 2);
			this->textBox_min7->Name = L"textBox_min7";
			this->textBox_min7->Size = System::Drawing::Size(80, 24);
			this->textBox_min7->TabIndex = 77;
			// 
			// textBox_min8
			// 
			this->textBox_min8->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->textBox_min8->Location = System::Drawing::Point(675, 199);
			this->textBox_min8->Margin = System::Windows::Forms::Padding(4, 2, 4, 2);
			this->textBox_min8->Name = L"textBox_min8";
			this->textBox_min8->Size = System::Drawing::Size(80, 24);
			this->textBox_min8->TabIndex = 78;
			// 
			// textBox_min9
			// 
			this->textBox_min9->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->textBox_min9->Location = System::Drawing::Point(675, 227);
			this->textBox_min9->Margin = System::Windows::Forms::Padding(4, 2, 4, 2);
			this->textBox_min9->Name = L"textBox_min9";
			this->textBox_min9->Size = System::Drawing::Size(80, 24);
			this->textBox_min9->TabIndex = 112;
			// 
			// textBox_min10
			// 
			this->textBox_min10->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->textBox_min10->Location = System::Drawing::Point(675, 255);
			this->textBox_min10->Margin = System::Windows::Forms::Padding(4, 2, 4, 2);
			this->textBox_min10->Name = L"textBox_min10";
			this->textBox_min10->Size = System::Drawing::Size(80, 24);
			this->textBox_min10->TabIndex = 162;
			// 
			// textBox_min4
			// 
			this->textBox_min4->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->textBox_min4->Location = System::Drawing::Point(675, 87);
			this->textBox_min4->Margin = System::Windows::Forms::Padding(4, 2, 4, 2);
			this->textBox_min4->Name = L"textBox_min4";
			this->textBox_min4->Size = System::Drawing::Size(80, 24);
			this->textBox_min4->TabIndex = 74;
			// 
			// label17
			// 
			this->label17->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->label17->AutoSize = true;
			this->label17->Location = System::Drawing::Point(5, 197);
			this->label17->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label17->Name = L"label17";
			this->label17->Size = System::Drawing::Size(99, 18);
			this->label17->TabIndex = 33;
			this->label17->Text = L"F_2_kHz";
			// 
			// label16
			// 
			this->label16->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->label16->AutoSize = true;
			this->label16->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->label16->Location = System::Drawing::Point(5, 169);
			this->label16->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label16->Name = L"label16";
			this->label16->Size = System::Drawing::Size(99, 18);
			this->label16->TabIndex = 32;
			this->label16->Text = L"F_1_kHz";
			// 
			// textBoxFT2
			// 
			this->textBoxFT2->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->textBoxFT2->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->textBoxFT2->Location = System::Drawing::Point(113, 199);
			this->textBoxFT2->Margin = System::Windows::Forms::Padding(4, 2, 4, 2);
			this->textBoxFT2->Name = L"textBoxFT2";
			this->textBoxFT2->Size = System::Drawing::Size(67, 24);
			this->textBoxFT2->TabIndex = 41;
			// 
			// textBoxFT1
			// 
			this->textBoxFT1->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->textBoxFT1->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->textBoxFT1->Location = System::Drawing::Point(113, 171);
			this->textBoxFT1->Margin = System::Windows::Forms::Padding(4, 2, 4, 2);
			this->textBoxFT1->Name = L"textBoxFT1";
			this->textBoxFT1->Size = System::Drawing::Size(67, 24);
			this->textBoxFT1->TabIndex = 40;
			// 
			// panel_inc
			// 
			this->panel_inc->BorderStyle = System::Windows::Forms::BorderStyle::FixedSingle;
			this->panel_inc->Controls->Add(this->checkBox_interupt);
			this->panel_inc->Controls->Add(this->checkBox_auto_delta);
			this->panel_inc->Controls->Add(this->textBox_K_Delta);
			this->panel_inc->Controls->Add(this->textBox_history_angle);
			this->panel_inc->Controls->Add(this->textBox_APS_DELTA);
			this->panel_inc->Controls->Add(this->textBox_K_predict);
			this->panel_inc->Controls->Add(this->textBox_W_MAX);
			this->panel_inc->Controls->Add(this->textBox_W_MIN);
			this->panel_inc->Controls->Add(this->textBoxK);
			this->panel_inc->Controls->Add(this->textBoxN);
			this->panel_inc->Controls->Add(this->textBoxMLD);
			this->panel_inc->Controls->Add(this->textBoxMA);
			this->panel_inc->Controls->Add(this->label56);
			this->panel_inc->Controls->Add(this->label55);
			this->panel_inc->Controls->Add(this->label54);
			this->panel_inc->Controls->Add(this->label53);
			this->panel_inc->Controls->Add(this->label52);
			this->panel_inc->Controls->Add(this->label);
			this->panel_inc->Controls->Add(this->label4N);
			this->panel_inc->Controls->Add(this->label4MLD);
			this->panel_inc->Controls->Add(this->label46);
			this->panel_inc->Controls->Add(this->label45);
			this->panel_inc->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->panel_inc->Location = System::Drawing::Point(13, 336);
			this->panel_inc->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->panel_inc->Name = L"panel_inc";
			this->panel_inc->Size = System::Drawing::Size(759, 110);
			this->panel_inc->TabIndex = 20;
			// 
			// checkBox_interupt
			// 
			this->checkBox_interupt->AutoSize = true;
			this->checkBox_interupt->Location = System::Drawing::Point(345, 43);
			this->checkBox_interupt->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->checkBox_interupt->Name = L"checkBox_interupt";
			this->checkBox_interupt->Size = System::Drawing::Size(205, 22);
			this->checkBox_interupt->TabIndex = 30;
			this->checkBox_interupt->Text = L"IRQ_BY_ANGLE_PATH";
			this->checkBox_interupt->UseVisualStyleBackColor = true;
			// 
			// checkBox_auto_delta
			// 
			this->checkBox_auto_delta->AutoSize = true;
			this->checkBox_auto_delta->Location = System::Drawing::Point(201, 42);
			this->checkBox_auto_delta->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->checkBox_auto_delta->Name = L"checkBox_auto_delta";
			this->checkBox_auto_delta->Size = System::Drawing::Size(132, 22);
			this->checkBox_auto_delta->TabIndex = 29;
			this->checkBox_auto_delta->Text = L"AUTO DELTA";
			this->checkBox_auto_delta->UseVisualStyleBackColor = true;
			// 
			// textBox_K_Delta
			// 
			this->textBox_K_Delta->Location = System::Drawing::Point(683, 73);
			this->textBox_K_Delta->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->textBox_K_Delta->Name = L"textBox_K_Delta";
			this->textBox_K_Delta->Size = System::Drawing::Size(49, 24);
			this->textBox_K_Delta->TabIndex = 28;
			// 
			// textBox_history_angle
			// 
			this->textBox_history_angle->Location = System::Drawing::Point(115, 39);
			this->textBox_history_angle->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->textBox_history_angle->Name = L"textBox_history_angle";
			this->textBox_history_angle->Size = System::Drawing::Size(71, 24);
			this->textBox_history_angle->TabIndex = 27;
			// 
			// textBox_APS_DELTA
			// 
			this->textBox_APS_DELTA->Location = System::Drawing::Point(669, 6);
			this->textBox_APS_DELTA->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->textBox_APS_DELTA->Name = L"textBox_APS_DELTA";
			this->textBox_APS_DELTA->Size = System::Drawing::Size(63, 24);
			this->textBox_APS_DELTA->TabIndex = 26;
			// 
			// textBox_K_predict
			// 
			this->textBox_K_predict->Location = System::Drawing::Point(427, 6);
			this->textBox_K_predict->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->textBox_K_predict->Name = L"textBox_K_predict";
			this->textBox_K_predict->Size = System::Drawing::Size(71, 24);
			this->textBox_K_predict->TabIndex = 25;
			// 
			// textBox_W_MAX
			// 
			this->textBox_W_MAX->Location = System::Drawing::Point(236, 6);
			this->textBox_W_MAX->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->textBox_W_MAX->Name = L"textBox_W_MAX";
			this->textBox_W_MAX->Size = System::Drawing::Size(71, 24);
			this->textBox_W_MAX->TabIndex = 24;
			// 
			// textBox_W_MIN
			// 
			this->textBox_W_MIN->Location = System::Drawing::Point(73, 6);
			this->textBox_W_MIN->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->textBox_W_MIN->Name = L"textBox_W_MIN";
			this->textBox_W_MIN->Size = System::Drawing::Size(71, 24);
			this->textBox_W_MIN->TabIndex = 23;
			// 
			// textBoxK
			// 
			this->textBoxK->Location = System::Drawing::Point(683, 39);
			this->textBoxK->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->textBoxK->Name = L"textBoxK";
			this->textBoxK->Size = System::Drawing::Size(49, 24);
			this->textBoxK->TabIndex = 20;
			// 
			// textBoxN
			// 
			this->textBoxN->Location = System::Drawing::Point(592, 39);
			this->textBoxN->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->textBoxN->Name = L"textBoxN";
			this->textBoxN->Size = System::Drawing::Size(41, 24);
			this->textBoxN->TabIndex = 19;
			// 
			// textBoxMLD
			// 
			this->textBoxMLD->Location = System::Drawing::Point(519, 73);
			this->textBoxMLD->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->textBoxMLD->Name = L"textBoxMLD";
			this->textBoxMLD->Size = System::Drawing::Size(49, 24);
			this->textBoxMLD->TabIndex = 18;
			// 
			// textBoxMA
			// 
			this->textBoxMA->Location = System::Drawing::Point(241, 73);
			this->textBoxMA->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->textBoxMA->Name = L"textBoxMA";
			this->textBoxMA->Size = System::Drawing::Size(49, 24);
			this->textBoxMA->TabIndex = 17;
			// 
			// label56
			// 
			this->label56->AutoSize = true;
			this->label56->Location = System::Drawing::Point(605, 76);
			this->label56->Name = L"label56";
			this->label56->Size = System::Drawing::Size(64, 18);
			this->label56->TabIndex = 16;
			this->label56->Text = L"K_delta";
			// 
			// label55
			// 
			this->label55->AutoSize = true;
			this->label55->Location = System::Drawing::Point(3, 41);
			this->label55->Name = L"label55";
			this->label55->Size = System::Drawing::Size(108, 18);
			this->label55->TabIndex = 15;
			this->label55->Text = L"history_angle";
			// 
			// label54
			// 
			this->label54->AutoSize = true;
			this->label54->Location = System::Drawing::Point(528, 9);
			this->label54->Name = L"label54";
			this->label54->Size = System::Drawing::Size(101, 18);
			this->label54->TabIndex = 14;
			this->label54->Text = L"APS_DELTA";
			// 
			// label53
			// 
			this->label53->AutoSize = true;
			this->label53->Location = System::Drawing::Point(333, 9);
			this->label53->Name = L"label53";
			this->label53->Size = System::Drawing::Size(79, 18);
			this->label53->TabIndex = 13;
			this->label53->Text = L"K_predict";
			// 
			// label52
			// 
			this->label52->AutoSize = true;
			this->label52->Location = System::Drawing::Point(165, 9);
			this->label52->Name = L"label52";
			this->label52->Size = System::Drawing::Size(68, 18);
			this->label52->TabIndex = 12;
			this->label52->Text = L"W_MAX";
			// 
			// label
			// 
			this->label->AutoSize = true;
			this->label->Location = System::Drawing::Point(5, 9);
			this->label->Name = L"label";
			this->label->Size = System::Drawing::Size(63, 18);
			this->label->TabIndex = 11;
			this->label->Text = L"W_MIN";
			// 
			// label4N
			// 
			this->label4N->AutoSize = true;
			this->label4N->Location = System::Drawing::Point(657, 43);
			this->label4N->Name = L"label4N";
			this->label4N->Size = System::Drawing::Size(19, 18);
			this->label4N->TabIndex = 8;
			this->label4N->Text = L"K";
			// 
			// label4MLD
			// 
			this->label4MLD->AutoSize = true;
			this->label4MLD->Location = System::Drawing::Point(568, 43);
			this->label4MLD->Name = L"label4MLD";
			this->label4MLD->Size = System::Drawing::Size(20, 18);
			this->label4MLD->TabIndex = 7;
			this->label4MLD->Text = L"N";
			// 
			// label46
			// 
			this->label46->AutoSize = true;
			this->label46->Location = System::Drawing::Point(317, 76);
			this->label46->Name = L"label46";
			this->label46->Size = System::Drawing::Size(196, 18);
			this->label46->TabIndex = 6;
			this->label46->Text = L"Окно средне линейного";
			// 
			// label45
			// 
			this->label45->AutoSize = true;
			this->label45->Location = System::Drawing::Point(0, 76);
			this->label45->Name = L"label45";
			this->label45->Size = System::Drawing::Size(234, 18);
			this->label45->TabIndex = 5;
			this->label45->Text = L"Окно скользящего среднего";
			// 
			// panel9
			// 
			this->panel9->BorderStyle = System::Windows::Forms::BorderStyle::FixedSingle;
			this->panel9->Controls->Add(this->comboBox_work_type);
			this->panel9->Controls->Add(this->label6);
			this->panel9->Controls->Add(this->label41);
			this->panel9->Controls->Add(this->textBox_sign_for_pallete);
			this->panel9->Controls->Add(this->label67);
			this->panel9->Controls->Add(this->textBox_number);
			this->panel9->Controls->Add(this->label66);
			this->panel9->Controls->Add(this->comboBox_mod);
			this->panel9->Controls->Add(this->label38);
			this->panel9->Controls->Add(this->comboBox_type);
			this->panel9->Controls->Add(this->label44);
			this->panel9->Controls->Add(this->textBoxSer);
			this->panel9->Controls->Add(this->textBox_sign32);
			this->panel9->Controls->Add(this->label39);
			this->panel9->Controls->Add(this->comboBox_N_Tx);
			this->panel9->Controls->Add(this->label37);
			this->panel9->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->panel9->Location = System::Drawing::Point(13, 702);
			this->panel9->Margin = System::Windows::Forms::Padding(4, 2, 4, 2);
			this->panel9->Name = L"panel9";
			this->panel9->Size = System::Drawing::Size(759, 179);
			this->panel9->TabIndex = 21;
			// 
			// comboBox_work_type
			// 
			this->comboBox_work_type->FormattingEnabled = true;
			this->comboBox_work_type->Items->AddRange(gcnew cli::array< System::Object^  >(2) { L"CARTOGRAF", L"LWD_4Tx" });
			this->comboBox_work_type->Location = System::Drawing::Point(119, 62);
			this->comboBox_work_type->Margin = System::Windows::Forms::Padding(4, 2, 4, 2);
			this->comboBox_work_type->Name = L"comboBox_work_type";
			this->comboBox_work_type->Size = System::Drawing::Size(147, 26);
			this->comboBox_work_type->TabIndex = 166;
			this->comboBox_work_type->SelectedIndexChanged += gcnew System::EventHandler(this, &MyForm1::comboBox_work_type_SelectedIndexChanged);
			// 
			// label6
			// 
			this->label6->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->label6->AutoSize = true;
			this->label6->Location = System::Drawing::Point(5, 67);
			this->label6->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label6->Name = L"label6";
			this->label6->Size = System::Drawing::Size(107, 18);
			this->label6->TabIndex = 164;
			this->label6->Text = L"WORK TYPE";
			// 
			// label41
			// 
			this->label41->AutoSize = true;
			this->label41->Location = System::Drawing::Point(527, 100);
			this->label41->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label41->Name = L"label41";
			this->label41->Size = System::Drawing::Size(157, 18);
			this->label41->TabIndex = 32;
			this->label41->Text = L"signature for pallete";
			// 
			// textBox_sign_for_pallete
			// 
			this->textBox_sign_for_pallete->Location = System::Drawing::Point(528, 125);
			this->textBox_sign_for_pallete->Margin = System::Windows::Forms::Padding(4, 2, 4, 2);
			this->textBox_sign_for_pallete->Name = L"textBox_sign_for_pallete";
			this->textBox_sign_for_pallete->ReadOnly = true;
			this->textBox_sign_for_pallete->Size = System::Drawing::Size(132, 24);
			this->textBox_sign_for_pallete->TabIndex = 31;
			// 
			// label67
			// 
			this->label67->AutoSize = true;
			this->label67->Location = System::Drawing::Point(391, 2);
			this->label67->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label67->Name = L"label67";
			this->label67->Size = System::Drawing::Size(57, 18);
			this->label67->TabIndex = 27;
			this->label67->Text = L"номер";
			// 
			// textBox_number
			// 
			this->textBox_number->Location = System::Drawing::Point(380, 26);
			this->textBox_number->Margin = System::Windows::Forms::Padding(4, 2, 4, 2);
			this->textBox_number->Name = L"textBox_number";
			this->textBox_number->Size = System::Drawing::Size(84, 24);
			this->textBox_number->TabIndex = 26;
			// 
			// label66
			// 
			this->label66->AutoSize = true;
			this->label66->Location = System::Drawing::Point(308, 0);
			this->label66->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label66->Name = L"label66";
			this->label66->Size = System::Drawing::Size(40, 18);
			this->label66->TabIndex = 25;
			this->label66->Text = L"мод";
			// 
			// comboBox_mod
			// 
			this->comboBox_mod->FormattingEnabled = true;
			this->comboBox_mod->Items->AddRange(gcnew cli::array< System::Object^  >(9) {
				L"1", L"2", L"3", L"4", L"5", L"6", L"7", L"8",
					L"9"
			});
			this->comboBox_mod->Location = System::Drawing::Point(291, 25);
			this->comboBox_mod->Margin = System::Windows::Forms::Padding(4, 2, 4, 2);
			this->comboBox_mod->Name = L"comboBox_mod";
			this->comboBox_mod->Size = System::Drawing::Size(72, 26);
			this->comboBox_mod->TabIndex = 24;
			// 
			// label38
			// 
			this->label38->AutoSize = true;
			this->label38->Location = System::Drawing::Point(4, 0);
			this->label38->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label38->Name = L"label38";
			this->label38->Size = System::Drawing::Size(42, 18);
			this->label38->TabIndex = 23;
			this->label38->Text = L"ТИП";
			// 
			// comboBox_type
			// 
			this->comboBox_type->FormattingEnabled = true;
			this->comboBox_type->Items->AddRange(gcnew cli::array< System::Object^  >(3) { L"АВТОНОМ", L"LWD", L"КАРТОГРАФ" });
			this->comboBox_type->Location = System::Drawing::Point(-3, 25);
			this->comboBox_type->Margin = System::Windows::Forms::Padding(4, 2, 4, 2);
			this->comboBox_type->Name = L"comboBox_type";
			this->comboBox_type->Size = System::Drawing::Size(197, 26);
			this->comboBox_type->TabIndex = 22;
			// 
			// label44
			// 
			this->label44->AutoSize = true;
			this->label44->Location = System::Drawing::Point(44, 100);
			this->label44->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label44->Name = L"label44";
			this->label44->Size = System::Drawing::Size(222, 18);
			this->label44->TabIndex = 15;
			this->label44->Text = L"signature32 разр. бинарный";
			// 
			// textBoxSer
			// 
			this->textBoxSer->Location = System::Drawing::Point(491, 26);
			this->textBoxSer->Margin = System::Windows::Forms::Padding(4, 2, 4, 2);
			this->textBoxSer->Name = L"textBoxSer";
			this->textBoxSer->Size = System::Drawing::Size(221, 24);
			this->textBoxSer->TabIndex = 2;
			// 
			// textBox_sign32
			// 
			this->textBox_sign32->Location = System::Drawing::Point(4, 125);
			this->textBox_sign32->Margin = System::Windows::Forms::Padding(4, 2, 4, 2);
			this->textBox_sign32->Name = L"textBox_sign32";
			this->textBox_sign32->Size = System::Drawing::Size(491, 24);
			this->textBox_sign32->TabIndex = 10;
			// 
			// label39
			// 
			this->label39->AutoSize = true;
			this->label39->Location = System::Drawing::Point(515, 2);
			this->label39->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label39->Name = L"label39";
			this->label39->Size = System::Drawing::Size(170, 18);
			this->label39->TabIndex = 2;
			this->label39->Text = L"Серийный на кожухе";
			// 
			// comboBox_N_Tx
			// 
			this->comboBox_N_Tx->FormattingEnabled = true;
			this->comboBox_N_Tx->Items->AddRange(gcnew cli::array< System::Object^  >(3) { L"3 Tx", L"4 Tx", L"5 Tx" });
			this->comboBox_N_Tx->Location = System::Drawing::Point(208, 25);
			this->comboBox_N_Tx->Margin = System::Windows::Forms::Padding(4, 2, 4, 2);
			this->comboBox_N_Tx->Name = L"comboBox_N_Tx";
			this->comboBox_N_Tx->Size = System::Drawing::Size(72, 26);
			this->comboBox_N_Tx->TabIndex = 8;
			// 
			// label37
			// 
			this->label37->AutoSize = true;
			this->label37->Location = System::Drawing::Point(219, 2);
			this->label37->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label37->Name = L"label37";
			this->label37->Size = System::Drawing::Size(47, 18);
			this->label37->TabIndex = 6;
			this->label37->Text = L"N_Tx";
			// 
			// openFileDialog1
			// 
			this->openFileDialog1->FileName = L"openFileDialog1";
			// 
			// checkBox_read_only
			// 
			this->checkBox_read_only->AutoSize = true;
			this->checkBox_read_only->Location = System::Drawing::Point(13, 14);
			this->checkBox_read_only->Margin = System::Windows::Forms::Padding(4, 2, 4, 2);
			this->checkBox_read_only->Name = L"checkBox_read_only";
			this->checkBox_read_only->Size = System::Drawing::Size(125, 21);
			this->checkBox_read_only->TabIndex = 22;
			this->checkBox_read_only->Text = L"L_F_read_only";
			this->checkBox_read_only->UseVisualStyleBackColor = true;
			this->checkBox_read_only->CheckedChanged += gcnew System::EventHandler(this, &MyForm1::checkBox_read_only_CheckedChanged);
			// 
			// tableLayoutPanel3
			// 
			this->tableLayoutPanel3->ColumnCount = 1;
			this->tableLayoutPanel3->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Absolute,
				73)));
			this->tableLayoutPanel3->Controls->Add(this->textBoxT7, 0, 6);
			this->tableLayoutPanel3->Controls->Add(this->textBoxT6, 0, 5);
			this->tableLayoutPanel3->Controls->Add(this->textBoxT5, 0, 4);
			this->tableLayoutPanel3->Controls->Add(this->textBoxT3, 0, 2);
			this->tableLayoutPanel3->Controls->Add(this->textBoxT2, 0, 1);
			this->tableLayoutPanel3->Controls->Add(this->textBoxT4, 0, 3);
			this->tableLayoutPanel3->Controls->Add(this->textBoxT1, 0, 0);
			this->tableLayoutPanel3->Location = System::Drawing::Point(104, 25);
			this->tableLayoutPanel3->Margin = System::Windows::Forms::Padding(4);
			this->tableLayoutPanel3->Name = L"tableLayoutPanel3";
			this->tableLayoutPanel3->RowCount = 7;
			this->tableLayoutPanel3->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Absolute, 31)));
			this->tableLayoutPanel3->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Absolute, 31)));
			this->tableLayoutPanel3->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Absolute, 31)));
			this->tableLayoutPanel3->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Absolute, 31)));
			this->tableLayoutPanel3->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Absolute, 31)));
			this->tableLayoutPanel3->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Absolute, 31)));
			this->tableLayoutPanel3->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Absolute, 31)));
			this->tableLayoutPanel3->Size = System::Drawing::Size(61, 218);
			this->tableLayoutPanel3->TabIndex = 72;
			// 
			// textBoxT7
			// 
			this->textBoxT7->Location = System::Drawing::Point(4, 190);
			this->textBoxT7->Margin = System::Windows::Forms::Padding(4);
			this->textBoxT7->Name = L"textBoxT7";
			this->textBoxT7->Size = System::Drawing::Size(49, 22);
			this->textBoxT7->TabIndex = 6;
			// 
			// textBoxT6
			// 
			this->textBoxT6->Location = System::Drawing::Point(4, 159);
			this->textBoxT6->Margin = System::Windows::Forms::Padding(4);
			this->textBoxT6->Name = L"textBoxT6";
			this->textBoxT6->Size = System::Drawing::Size(49, 22);
			this->textBoxT6->TabIndex = 5;
			// 
			// textBoxT5
			// 
			this->textBoxT5->Location = System::Drawing::Point(4, 128);
			this->textBoxT5->Margin = System::Windows::Forms::Padding(4);
			this->textBoxT5->Name = L"textBoxT5";
			this->textBoxT5->Size = System::Drawing::Size(49, 22);
			this->textBoxT5->TabIndex = 4;
			// 
			// textBoxT3
			// 
			this->textBoxT3->Location = System::Drawing::Point(4, 66);
			this->textBoxT3->Margin = System::Windows::Forms::Padding(4);
			this->textBoxT3->Name = L"textBoxT3";
			this->textBoxT3->Size = System::Drawing::Size(49, 22);
			this->textBoxT3->TabIndex = 2;
			// 
			// textBoxT2
			// 
			this->textBoxT2->Location = System::Drawing::Point(4, 35);
			this->textBoxT2->Margin = System::Windows::Forms::Padding(4);
			this->textBoxT2->Name = L"textBoxT2";
			this->textBoxT2->Size = System::Drawing::Size(49, 22);
			this->textBoxT2->TabIndex = 1;
			// 
			// textBoxT4
			// 
			this->textBoxT4->Location = System::Drawing::Point(4, 97);
			this->textBoxT4->Margin = System::Windows::Forms::Padding(4);
			this->textBoxT4->Name = L"textBoxT4";
			this->textBoxT4->Size = System::Drawing::Size(49, 22);
			this->textBoxT4->TabIndex = 3;
			// 
			// textBoxT1
			// 
			this->textBoxT1->Location = System::Drawing::Point(4, 4);
			this->textBoxT1->Margin = System::Windows::Forms::Padding(4);
			this->textBoxT1->Name = L"textBoxT1";
			this->textBoxT1->Size = System::Drawing::Size(49, 22);
			this->textBoxT1->TabIndex = 0;
			// 
			// button3
			// 
			this->button3->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->button3->Location = System::Drawing::Point(4, 36);
			this->button3->Margin = System::Windows::Forms::Padding(4, 2, 4, 2);
			this->button3->Name = L"button3";
			this->button3->Size = System::Drawing::Size(233, 37);
			this->button3->TabIndex = 73;
			this->button3->Text = L"ОТКРЫТЬ_ФАЙЛ";
			this->button3->UseVisualStyleBackColor = true;
			this->button3->Click += gcnew System::EventHandler(this, &MyForm1::buttonOpen_file_Click);
			// 
			// button1
			// 
			this->button1->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->button1->Location = System::Drawing::Point(4, 78);
			this->button1->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->button1->Name = L"button1";
			this->button1->Size = System::Drawing::Size(233, 41);
			this->button1->TabIndex = 74;
			this->button1->Text = L"ЗАПИСАТЬ_В_ФАЙЛ";
			this->button1->UseVisualStyleBackColor = true;
			this->button1->Click += gcnew System::EventHandler(this, &MyForm1::buttonWrite_tofile_Click);
			// 
			// button4
			// 
			this->button4->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->button4->Location = System::Drawing::Point(4, 135);
			this->button4->Margin = System::Windows::Forms::Padding(4, 2, 4, 2);
			this->button4->Name = L"button4";
			this->button4->Size = System::Drawing::Size(233, 36);
			this->button4->TabIndex = 75;
			this->button4->Text = L"ПРОЧИТАТЬ_ИЗ_ЗОНДА";
			this->button4->UseVisualStyleBackColor = true;
			this->button4->Click += gcnew System::EventHandler(this, &MyForm1::buttonRead_from_sonde_Click);
			// 
			// button5
			// 
			this->button5->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->button5->Location = System::Drawing::Point(4, 178);
			this->button5->Margin = System::Windows::Forms::Padding(4, 2, 4, 2);
			this->button5->Name = L"button5";
			this->button5->Size = System::Drawing::Size(233, 37);
			this->button5->TabIndex = 76;
			this->button5->Text = L"ЗАПИСАТЬ_В_ЗОНД";
			this->button5->UseVisualStyleBackColor = true;
			this->button5->Click += gcnew System::EventHandler(this, &MyForm1::buttonWrite_to_sonde_Click);
			// 
			// label7
			// 
			this->label7->AutoSize = true;
			this->label7->Location = System::Drawing::Point(103, 6);
			this->label7->Margin = System::Windows::Forms::Padding(5, 0, 5, 0);
			this->label7->Name = L"label7";
			this->label7->Size = System::Drawing::Size(89, 17);
			this->label7->TabIndex = 77;
			this->label7->Text = L"TIME_UNITS";
			// 
			// label12
			// 
			this->label12->AutoSize = true;
			this->label12->Location = System::Drawing::Point(173, 126);
			this->label12->Margin = System::Windows::Forms::Padding(5, 0, 5, 0);
			this->label12->Name = L"label12";
			this->label12->Size = System::Drawing::Size(47, 17);
			this->label12->TabIndex = 78;
			this->label12->Text = L"TURN";
			// 
			// textBox_order
			// 
			this->textBox_order->Location = System::Drawing::Point(3, 121);
			this->textBox_order->Margin = System::Windows::Forms::Padding(4);
			this->textBox_order->Name = L"textBox_order";
			this->textBox_order->Size = System::Drawing::Size(93, 22);
			this->textBox_order->TabIndex = 79;
			// 
			// label13
			// 
			this->label13->AutoSize = true;
			this->label13->Location = System::Drawing::Point(29, 0);
			this->label13->Margin = System::Windows::Forms::Padding(29, 0, 5, 0);
			this->label13->Name = L"label13";
			this->label13->Size = System::Drawing::Size(47, 17);
			this->label13->TabIndex = 83;
			this->label13->Text = L"SET 1";
			// 
			// label14
			// 
			this->label14->AutoSize = true;
			this->label14->Location = System::Drawing::Point(128, 0);
			this->label14->Margin = System::Windows::Forms::Padding(29, 0, 5, 0);
			this->label14->Name = L"label14";
			this->label14->Size = System::Drawing::Size(47, 17);
			this->label14->TabIndex = 84;
			this->label14->Text = L"SET 2";
			// 
			// label15
			// 
			this->label15->AutoSize = true;
			this->label15->Location = System::Drawing::Point(228, 0);
			this->label15->Margin = System::Windows::Forms::Padding(29, 0, 5, 0);
			this->label15->Name = L"label15";
			this->label15->Size = System::Drawing::Size(47, 17);
			this->label15->TabIndex = 85;
			this->label15->Text = L"SET 3";
			// 
			// comboBoxTBTindex
			// 
			this->comboBoxTBTindex->FormattingEnabled = true;
			this->comboBoxTBTindex->Items->AddRange(gcnew cli::array< System::Object^  >(3) { L"SET 1", L"SET 2", L"SET 3" });
			this->comboBoxTBTindex->Location = System::Drawing::Point(7, 21);
			this->comboBoxTBTindex->Margin = System::Windows::Forms::Padding(4);
			this->comboBoxTBTindex->Name = L"comboBoxTBTindex";
			this->comboBoxTBTindex->Size = System::Drawing::Size(80, 24);
			this->comboBoxTBTindex->TabIndex = 86;
			// 
			// label18
			// 
			this->label18->AutoSize = true;
			this->label18->Location = System::Drawing::Point(4, 0);
			this->label18->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label18->Name = L"label18";
			this->label18->Size = System::Drawing::Size(68, 17);
			this->label18->TabIndex = 87;
			this->label18->Text = L"TBTIndex";
			// 
			// label19
			// 
			this->label19->AutoSize = true;
			this->label19->Location = System::Drawing::Point(4, 52);
			this->label19->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label19->Name = L"label19";
			this->label19->Size = System::Drawing::Size(142, 17);
			this->label19->TabIndex = 88;
			this->label19->Text = L"MinRotSpeed об.мин";
			// 
			// textBox_MinRotSpeed
			// 
			this->textBox_MinRotSpeed->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->textBox_MinRotSpeed->Location = System::Drawing::Point(4, 71);
			this->textBox_MinRotSpeed->Margin = System::Windows::Forms::Padding(4, 2, 4, 2);
			this->textBox_MinRotSpeed->Name = L"textBox_MinRotSpeed";
			this->textBox_MinRotSpeed->Size = System::Drawing::Size(139, 24);
			this->textBox_MinRotSpeed->TabIndex = 160;
			// 
			// checkBox_debug_session
			// 
			this->checkBox_debug_session->AutoSize = true;
			this->checkBox_debug_session->Location = System::Drawing::Point(8, 165);
			this->checkBox_debug_session->Margin = System::Windows::Forms::Padding(4);
			this->checkBox_debug_session->Name = L"checkBox_debug_session";
			this->checkBox_debug_session->Size = System::Drawing::Size(122, 21);
			this->checkBox_debug_session->TabIndex = 162;
			this->checkBox_debug_session->Text = L"DebugSession";
			this->checkBox_debug_session->UseVisualStyleBackColor = true;
			// 
			// textBox_unix_epoh
			// 
			this->textBox_unix_epoh->Location = System::Drawing::Point(7, 129);
			this->textBox_unix_epoh->Margin = System::Windows::Forms::Padding(4);
			this->textBox_unix_epoh->Name = L"textBox_unix_epoh";
			this->textBox_unix_epoh->Size = System::Drawing::Size(136, 22);
			this->textBox_unix_epoh->TabIndex = 163;
			// 
			// label20
			// 
			this->label20->AutoSize = true;
			this->label20->Location = System::Drawing::Point(5, 110);
			this->label20->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label20->Name = L"label20";
			this->label20->Size = System::Drawing::Size(119, 17);
			this->label20->TabIndex = 164;
			this->label20->Text = L"UNIX EPOH TIME";
			// 
			// button_set_of_preset_from_sonde
			// 
			this->button_set_of_preset_from_sonde->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9, System::Drawing::FontStyle::Bold,
				System::Drawing::GraphicsUnit::Point, static_cast<System::Byte>(204)));
			this->button_set_of_preset_from_sonde->Location = System::Drawing::Point(4, 32);
			this->button_set_of_preset_from_sonde->Margin = System::Windows::Forms::Padding(4, 2, 4, 2);
			this->button_set_of_preset_from_sonde->Name = L"button_set_of_preset_from_sonde";
			this->button_set_of_preset_from_sonde->Size = System::Drawing::Size(233, 36);
			this->button_set_of_preset_from_sonde->TabIndex = 165;
			this->button_set_of_preset_from_sonde->Text = L"ПРОЧИТАТЬ_ИЗ_ЗОНДА";
			this->button_set_of_preset_from_sonde->UseVisualStyleBackColor = true;
			this->button_set_of_preset_from_sonde->Click += gcnew System::EventHandler(this, &MyForm1::button_set_of_preset_from_sonde_Click);
			// 
			// button_set_of_preset_to_sonde
			// 
			this->button_set_of_preset_to_sonde->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9, System::Drawing::FontStyle::Bold,
				System::Drawing::GraphicsUnit::Point, static_cast<System::Byte>(204)));
			this->button_set_of_preset_to_sonde->Location = System::Drawing::Point(4, 74);
			this->button_set_of_preset_to_sonde->Margin = System::Windows::Forms::Padding(4, 2, 4, 2);
			this->button_set_of_preset_to_sonde->Name = L"button_set_of_preset_to_sonde";
			this->button_set_of_preset_to_sonde->Size = System::Drawing::Size(233, 37);
			this->button_set_of_preset_to_sonde->TabIndex = 166;
			this->button_set_of_preset_to_sonde->Text = L"ЗАПИСАТЬ_В_ЗОНД";
			this->button_set_of_preset_to_sonde->UseVisualStyleBackColor = true;
			this->button_set_of_preset_to_sonde->Click += gcnew System::EventHandler(this, &MyForm1::button_set_of_preset_to_sonde_Click);
			// 
			// label_set_of_persets
			// 
			this->label_set_of_persets->AutoSize = true;
			this->label_set_of_persets->Location = System::Drawing::Point(53, 14);
			this->label_set_of_persets->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label_set_of_persets->Name = L"label_set_of_persets";
			this->label_set_of_persets->Size = System::Drawing::Size(130, 17);
			this->label_set_of_persets->TabIndex = 167;
			this->label_set_of_persets->Text = L"SET OF PRESETS ";
			// 
			// label22
			// 
			this->label22->AutoSize = true;
			this->label22->Location = System::Drawing::Point(67, 10);
			this->label22->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label22->Name = L"label22";
			this->label22->Size = System::Drawing::Size(97, 17);
			this->label22->TabIndex = 168;
			this->label22->Text = L"METROLOGY";
			// 
			// tableLayoutPanel2
			// 
			this->tableLayoutPanel2->ColumnCount = 3;
			this->tableLayoutPanel2->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Absolute,
				99)));
			this->tableLayoutPanel2->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Absolute,
				100)));
			this->tableLayoutPanel2->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Absolute,
				209)));
			this->tableLayoutPanel2->Controls->Add(this->textBoxT5S1, 0, 5);
			this->tableLayoutPanel2->Controls->Add(this->textBoxT4S1, 0, 4);
			this->tableLayoutPanel2->Controls->Add(this->textBoxT3S1, 0, 3);
			this->tableLayoutPanel2->Controls->Add(this->textBoxT2S1, 0, 2);
			this->tableLayoutPanel2->Controls->Add(this->textBoxT1S1, 0, 1);
			this->tableLayoutPanel2->Controls->Add(this->textBoxT5S2, 1, 5);
			this->tableLayoutPanel2->Controls->Add(this->textBoxT4S2, 1, 4);
			this->tableLayoutPanel2->Controls->Add(this->label13, 0, 0);
			this->tableLayoutPanel2->Controls->Add(this->textBoxT2S2, 1, 2);
			this->tableLayoutPanel2->Controls->Add(this->label14, 1, 0);
			this->tableLayoutPanel2->Controls->Add(this->textBoxT1S2, 1, 1);
			this->tableLayoutPanel2->Controls->Add(this->textBoxT5S3, 2, 5);
			this->tableLayoutPanel2->Controls->Add(this->textBoxT4S3, 2, 4);
			this->tableLayoutPanel2->Controls->Add(this->textBoxT3S3, 2, 3);
			this->tableLayoutPanel2->Controls->Add(this->label15, 2, 0);
			this->tableLayoutPanel2->Controls->Add(this->textBoxT2S3, 2, 2);
			this->tableLayoutPanel2->Controls->Add(this->textBoxT1S3, 2, 1);
			this->tableLayoutPanel2->Controls->Add(this->textBoxT3S2, 1, 3);
			this->tableLayoutPanel2->Location = System::Drawing::Point(221, 37);
			this->tableLayoutPanel2->Margin = System::Windows::Forms::Padding(4);
			this->tableLayoutPanel2->Name = L"tableLayoutPanel2";
			this->tableLayoutPanel2->RowCount = 6;
			this->tableLayoutPanel2->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Percent, 16.66667F)));
			this->tableLayoutPanel2->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Percent, 16.66667F)));
			this->tableLayoutPanel2->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Percent, 16.66667F)));
			this->tableLayoutPanel2->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Percent, 16.66667F)));
			this->tableLayoutPanel2->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Percent, 16.66667F)));
			this->tableLayoutPanel2->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Percent, 16.66667F)));
			this->tableLayoutPanel2->Size = System::Drawing::Size(301, 172);
			this->tableLayoutPanel2->TabIndex = 169;
			// 
			// textBoxT5S1
			// 
			this->textBoxT5S1->Location = System::Drawing::Point(4, 144);
			this->textBoxT5S1->Margin = System::Windows::Forms::Padding(4);
			this->textBoxT5S1->Name = L"textBoxT5S1";
			this->textBoxT5S1->Size = System::Drawing::Size(89, 22);
			this->textBoxT5S1->TabIndex = 178;
			// 
			// textBoxT4S1
			// 
			this->textBoxT4S1->Location = System::Drawing::Point(4, 116);
			this->textBoxT4S1->Margin = System::Windows::Forms::Padding(4);
			this->textBoxT4S1->Name = L"textBoxT4S1";
			this->textBoxT4S1->Size = System::Drawing::Size(89, 22);
			this->textBoxT4S1->TabIndex = 175;
			// 
			// textBoxT3S1
			// 
			this->textBoxT3S1->Location = System::Drawing::Point(4, 88);
			this->textBoxT3S1->Margin = System::Windows::Forms::Padding(4);
			this->textBoxT3S1->Name = L"textBoxT3S1";
			this->textBoxT3S1->Size = System::Drawing::Size(89, 22);
			this->textBoxT3S1->TabIndex = 172;
			// 
			// textBoxT2S1
			// 
			this->textBoxT2S1->Location = System::Drawing::Point(4, 60);
			this->textBoxT2S1->Margin = System::Windows::Forms::Padding(4);
			this->textBoxT2S1->Name = L"textBoxT2S1";
			this->textBoxT2S1->Size = System::Drawing::Size(89, 22);
			this->textBoxT2S1->TabIndex = 170;
			// 
			// textBoxT1S1
			// 
			this->textBoxT1S1->Location = System::Drawing::Point(4, 32);
			this->textBoxT1S1->Margin = System::Windows::Forms::Padding(4);
			this->textBoxT1S1->Name = L"textBoxT1S1";
			this->textBoxT1S1->Size = System::Drawing::Size(89, 22);
			this->textBoxT1S1->TabIndex = 80;
			// 
			// textBoxT5S2
			// 
			this->textBoxT5S2->Location = System::Drawing::Point(103, 144);
			this->textBoxT5S2->Margin = System::Windows::Forms::Padding(4);
			this->textBoxT5S2->Name = L"textBoxT5S2";
			this->textBoxT5S2->Size = System::Drawing::Size(89, 22);
			this->textBoxT5S2->TabIndex = 179;
			// 
			// textBoxT4S2
			// 
			this->textBoxT4S2->Location = System::Drawing::Point(103, 116);
			this->textBoxT4S2->Margin = System::Windows::Forms::Padding(4);
			this->textBoxT4S2->Name = L"textBoxT4S2";
			this->textBoxT4S2->Size = System::Drawing::Size(89, 22);
			this->textBoxT4S2->TabIndex = 176;
			// 
			// textBoxT2S2
			// 
			this->textBoxT2S2->Location = System::Drawing::Point(103, 60);
			this->textBoxT2S2->Margin = System::Windows::Forms::Padding(4);
			this->textBoxT2S2->Name = L"textBoxT2S2";
			this->textBoxT2S2->Size = System::Drawing::Size(89, 22);
			this->textBoxT2S2->TabIndex = 171;
			// 
			// textBoxT1S2
			// 
			this->textBoxT1S2->Location = System::Drawing::Point(103, 32);
			this->textBoxT1S2->Margin = System::Windows::Forms::Padding(4);
			this->textBoxT1S2->Name = L"textBoxT1S2";
			this->textBoxT1S2->Size = System::Drawing::Size(89, 22);
			this->textBoxT1S2->TabIndex = 81;
			// 
			// textBoxT5S3
			// 
			this->textBoxT5S3->Location = System::Drawing::Point(203, 144);
			this->textBoxT5S3->Margin = System::Windows::Forms::Padding(4);
			this->textBoxT5S3->Name = L"textBoxT5S3";
			this->textBoxT5S3->Size = System::Drawing::Size(89, 22);
			this->textBoxT5S3->TabIndex = 180;
			// 
			// textBoxT4S3
			// 
			this->textBoxT4S3->Location = System::Drawing::Point(203, 116);
			this->textBoxT4S3->Margin = System::Windows::Forms::Padding(4);
			this->textBoxT4S3->Name = L"textBoxT4S3";
			this->textBoxT4S3->Size = System::Drawing::Size(89, 22);
			this->textBoxT4S3->TabIndex = 177;
			// 
			// textBoxT3S3
			// 
			this->textBoxT3S3->Location = System::Drawing::Point(203, 88);
			this->textBoxT3S3->Margin = System::Windows::Forms::Padding(4);
			this->textBoxT3S3->Name = L"textBoxT3S3";
			this->textBoxT3S3->Size = System::Drawing::Size(89, 22);
			this->textBoxT3S3->TabIndex = 174;
			// 
			// textBoxT2S3
			// 
			this->textBoxT2S3->Location = System::Drawing::Point(203, 60);
			this->textBoxT2S3->Margin = System::Windows::Forms::Padding(4);
			this->textBoxT2S3->Name = L"textBoxT2S3";
			this->textBoxT2S3->Size = System::Drawing::Size(89, 22);
			this->textBoxT2S3->TabIndex = 170;
			// 
			// textBoxT1S3
			// 
			this->textBoxT1S3->Location = System::Drawing::Point(203, 32);
			this->textBoxT1S3->Margin = System::Windows::Forms::Padding(4);
			this->textBoxT1S3->Name = L"textBoxT1S3";
			this->textBoxT1S3->Size = System::Drawing::Size(89, 22);
			this->textBoxT1S3->TabIndex = 82;
			// 
			// textBoxT3S2
			// 
			this->textBoxT3S2->Location = System::Drawing::Point(103, 88);
			this->textBoxT3S2->Margin = System::Windows::Forms::Padding(4);
			this->textBoxT3S2->Name = L"textBoxT3S2";
			this->textBoxT3S2->Size = System::Drawing::Size(89, 22);
			this->textBoxT3S2->TabIndex = 173;
			// 
			// label23
			// 
			this->label23->AutoSize = true;
			this->label23->Location = System::Drawing::Point(11, 95);
			this->label23->Margin = System::Windows::Forms::Padding(5, 0, 5, 0);
			this->label23->Name = L"label23";
			this->label23->Size = System::Drawing::Size(61, 17);
			this->label23->TabIndex = 170;
			this->label23->Text = L"Tx order";
			// 
			// textBox_rotation
			// 
			this->textBox_rotation->Location = System::Drawing::Point(89, 203);
			this->textBox_rotation->Margin = System::Windows::Forms::Padding(4);
			this->textBox_rotation->Name = L"textBox_rotation";
			this->textBox_rotation->Size = System::Drawing::Size(55, 22);
			this->textBox_rotation->TabIndex = 173;
			this->textBox_rotation->Text = L"0";
			this->textBox_rotation->KeyPress += gcnew System::Windows::Forms::KeyPressEventHandler(this, &MyForm1::digit);
			// 
			// button_set_air
			// 
			this->button_set_air->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->button_set_air->Location = System::Drawing::Point(807, 870);
			this->button_set_air->Margin = System::Windows::Forms::Padding(4, 2, 4, 2);
			this->button_set_air->Name = L"button_set_air";
			this->button_set_air->Size = System::Drawing::Size(233, 37);
			this->button_set_air->TabIndex = 175;
			this->button_set_air->Text = L"ЗАПИСАТЬ ВОЗДУХ";
			this->button_set_air->UseVisualStyleBackColor = true;
			this->button_set_air->Click += gcnew System::EventHandler(this, &MyForm1::button_set_air_Click);
			// 
			// panel_cartograph_sets
			// 
			this->panel_cartograph_sets->Controls->Add(this->panel1);
			this->panel_cartograph_sets->Controls->Add(this->label23);
			this->panel_cartograph_sets->Controls->Add(this->label7);
			this->panel_cartograph_sets->Controls->Add(this->tableLayoutPanel3);
			this->panel_cartograph_sets->Controls->Add(this->textBox_order);
			this->panel_cartograph_sets->Controls->Add(this->tableLayoutPanel2);
			this->panel_cartograph_sets->Controls->Add(this->label12);
			this->panel_cartograph_sets->Location = System::Drawing::Point(12, 450);
			this->panel_cartograph_sets->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->panel_cartograph_sets->Name = L"panel_cartograph_sets";
			this->panel_cartograph_sets->Size = System::Drawing::Size(761, 247);
			this->panel_cartograph_sets->TabIndex = 176;
			// 
			// panel1
			// 
			this->panel1->Controls->Add(this->label18);
			this->panel1->Controls->Add(this->label_rot);
			this->panel1->Controls->Add(this->comboBoxTBTindex);
			this->panel1->Controls->Add(this->textBox_rotation);
			this->panel1->Controls->Add(this->label19);
			this->panel1->Controls->Add(this->textBox_MinRotSpeed);
			this->panel1->Controls->Add(this->label20);
			this->panel1->Controls->Add(this->textBox_unix_epoh);
			this->panel1->Controls->Add(this->checkBox_debug_session);
			this->panel1->Location = System::Drawing::Point(528, 2);
			this->panel1->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->panel1->Name = L"panel1";
			this->panel1->Size = System::Drawing::Size(161, 234);
			this->panel1->TabIndex = 175;
			// 
			// label_rot
			// 
			this->label_rot->AutoSize = true;
			this->label_rot->Location = System::Drawing::Point(5, 206);
			this->label_rot->Margin = System::Windows::Forms::Padding(5, 0, 5, 0);
			this->label_rot->Name = L"label_rot";
			this->label_rot->Size = System::Drawing::Size(80, 17);
			this->label_rot->TabIndex = 174;
			this->label_rot->Text = L"ROT EMUL";
			// 
			// panel2
			// 
			this->panel2->Controls->Add(this->label22);
			this->panel2->Controls->Add(this->button3);
			this->panel2->Controls->Add(this->button1);
			this->panel2->Controls->Add(this->button4);
			this->panel2->Controls->Add(this->button5);
			this->panel2->Location = System::Drawing::Point(803, 497);
			this->panel2->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->panel2->Name = L"panel2";
			this->panel2->Size = System::Drawing::Size(245, 225);
			this->panel2->TabIndex = 177;
			// 
			// panel3
			// 
			this->panel3->Controls->Add(this->label_set_of_persets);
			this->panel3->Controls->Add(this->button_set_of_preset_from_sonde);
			this->panel3->Controls->Add(this->button_set_of_preset_to_sonde);
			this->panel3->Location = System::Drawing::Point(803, 729);
			this->panel3->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->panel3->Name = L"panel3";
			this->panel3->Size = System::Drawing::Size(245, 114);
			this->panel3->TabIndex = 178;
			// 
			// checkBox11
			// 
			this->checkBox11->AutoSize = true;
			this->checkBox11->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9.75F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->checkBox11->Location = System::Drawing::Point(3, 394);
			this->checkBox11->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->checkBox11->Name = L"checkBox11";
			this->checkBox11->Size = System::Drawing::Size(75, 24);
			this->checkBox11->TabIndex = 11;
			this->checkBox11->Text = L"RoL4";
			this->checkBox11->UseVisualStyleBackColor = true;
			this->checkBox11->CheckedChanged += gcnew System::EventHandler(this, &MyForm1::checkBox_curve_CheckedChanged);
			// 
			// checkBox10
			// 
			this->checkBox10->AutoSize = true;
			this->checkBox10->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9.75F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->checkBox10->Location = System::Drawing::Point(3, 366);
			this->checkBox10->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->checkBox10->Name = L"checkBox10";
			this->checkBox10->Size = System::Drawing::Size(75, 24);
			this->checkBox10->TabIndex = 10;
			this->checkBox10->Text = L"RoL3";
			this->checkBox10->UseVisualStyleBackColor = true;
			this->checkBox10->CheckedChanged += gcnew System::EventHandler(this, &MyForm1::checkBox_curve_CheckedChanged);
			// 
			// checkBox9
			// 
			this->checkBox9->AutoSize = true;
			this->checkBox9->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9.75F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->checkBox9->Location = System::Drawing::Point(3, 338);
			this->checkBox9->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->checkBox9->Name = L"checkBox9";
			this->checkBox9->Size = System::Drawing::Size(75, 24);
			this->checkBox9->TabIndex = 9;
			this->checkBox9->Text = L"RoL2";
			this->checkBox9->UseVisualStyleBackColor = true;
			this->checkBox9->CheckedChanged += gcnew System::EventHandler(this, &MyForm1::checkBox_curve_CheckedChanged);
			// 
			// checkBox8
			// 
			this->checkBox8->AutoSize = true;
			this->checkBox8->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9.75F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->checkBox8->Location = System::Drawing::Point(3, 310);
			this->checkBox8->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->checkBox8->Name = L"checkBox8";
			this->checkBox8->Size = System::Drawing::Size(75, 24);
			this->checkBox8->TabIndex = 8;
			this->checkBox8->Text = L"RoL1";
			this->checkBox8->UseVisualStyleBackColor = true;
			this->checkBox8->CheckedChanged += gcnew System::EventHandler(this, &MyForm1::checkBox_curve_CheckedChanged);
			// 
			// checkBox7
			// 
			this->checkBox7->AutoSize = true;
			this->checkBox7->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9.75F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->checkBox7->Location = System::Drawing::Point(3, 254);
			this->checkBox7->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->checkBox7->Name = L"checkBox7";
			this->checkBox7->Size = System::Drawing::Size(78, 24);
			this->checkBox7->TabIndex = 7;
			this->checkBox7->Text = L"GPL4";
			this->checkBox7->UseVisualStyleBackColor = true;
			this->checkBox7->CheckedChanged += gcnew System::EventHandler(this, &MyForm1::checkBox_curve_CheckedChanged);
			// 
			// checkBox6
			// 
			this->checkBox6->AutoSize = true;
			this->checkBox6->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9.75F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->checkBox6->Location = System::Drawing::Point(3, 226);
			this->checkBox6->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->checkBox6->Name = L"checkBox6";
			this->checkBox6->Size = System::Drawing::Size(78, 24);
			this->checkBox6->TabIndex = 6;
			this->checkBox6->Text = L"GPL3";
			this->checkBox6->UseVisualStyleBackColor = true;
			this->checkBox6->CheckedChanged += gcnew System::EventHandler(this, &MyForm1::checkBox_curve_CheckedChanged);
			// 
			// checkBox5
			// 
			this->checkBox5->AutoSize = true;
			this->checkBox5->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9.75F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->checkBox5->Location = System::Drawing::Point(3, 198);
			this->checkBox5->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->checkBox5->Name = L"checkBox5";
			this->checkBox5->Size = System::Drawing::Size(78, 24);
			this->checkBox5->TabIndex = 5;
			this->checkBox5->Text = L"GPL2";
			this->checkBox5->UseVisualStyleBackColor = true;
			this->checkBox5->CheckedChanged += gcnew System::EventHandler(this, &MyForm1::checkBox_curve_CheckedChanged);
			// 
			// checkBox4
			// 
			this->checkBox4->AutoSize = true;
			this->checkBox4->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9.75F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->checkBox4->Location = System::Drawing::Point(3, 170);
			this->checkBox4->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->checkBox4->Name = L"checkBox4";
			this->checkBox4->Size = System::Drawing::Size(78, 24);
			this->checkBox4->TabIndex = 4;
			this->checkBox4->Text = L"GPL1";
			this->checkBox4->UseVisualStyleBackColor = true;
			this->checkBox4->CheckedChanged += gcnew System::EventHandler(this, &MyForm1::checkBox_curve_CheckedChanged);
			// 
			// checkBox3
			// 
			this->checkBox3->AutoSize = true;
			this->checkBox3->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9.75F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->checkBox3->Location = System::Drawing::Point(3, 114);
			this->checkBox3->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->checkBox3->Name = L"checkBox3";
			this->checkBox3->Size = System::Drawing::Size(78, 24);
			this->checkBox3->TabIndex = 3;
			this->checkBox3->Text = L"GAL4";
			this->checkBox3->UseVisualStyleBackColor = true;
			this->checkBox3->CheckedChanged += gcnew System::EventHandler(this, &MyForm1::checkBox_curve_CheckedChanged);
			// 
			// checkBox2
			// 
			this->checkBox2->AutoSize = true;
			this->checkBox2->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9.75F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->checkBox2->Location = System::Drawing::Point(3, 86);
			this->checkBox2->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->checkBox2->Name = L"checkBox2";
			this->checkBox2->Size = System::Drawing::Size(78, 24);
			this->checkBox2->TabIndex = 2;
			this->checkBox2->Text = L"GAL3";
			this->checkBox2->UseVisualStyleBackColor = true;
			this->checkBox2->CheckedChanged += gcnew System::EventHandler(this, &MyForm1::checkBox_curve_CheckedChanged);
			// 
			// checkBox1
			// 
			this->checkBox1->AutoSize = true;
			this->checkBox1->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9.75F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->checkBox1->Location = System::Drawing::Point(3, 58);
			this->checkBox1->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->checkBox1->Name = L"checkBox1";
			this->checkBox1->Size = System::Drawing::Size(78, 24);
			this->checkBox1->TabIndex = 1;
			this->checkBox1->Text = L"GAL2";
			this->checkBox1->UseVisualStyleBackColor = true;
			this->checkBox1->CheckedChanged += gcnew System::EventHandler(this, &MyForm1::checkBox_curve_CheckedChanged);
			// 
			// checkBox0
			// 
			this->checkBox0->AutoSize = true;
			this->checkBox0->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9.75F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->checkBox0->Location = System::Drawing::Point(3, 30);
			this->checkBox0->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->checkBox0->Name = L"checkBox0";
			this->checkBox0->Size = System::Drawing::Size(78, 24);
			this->checkBox0->TabIndex = 0;
			this->checkBox0->Text = L"GAL1";
			this->checkBox0->UseVisualStyleBackColor = true;
			this->checkBox0->CheckedChanged += gcnew System::EventHandler(this, &MyForm1::checkBox_curve_CheckedChanged);
			// 
			// checkBox12
			// 
			this->checkBox12->AutoSize = true;
			this->checkBox12->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9.75F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->checkBox12->Location = System::Drawing::Point(146, 30);
			this->checkBox12->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->checkBox12->Name = L"checkBox12";
			this->checkBox12->Size = System::Drawing::Size(81, 24);
			this->checkBox12->TabIndex = 12;
			this->checkBox12->Text = L"GAH1";
			this->checkBox12->UseVisualStyleBackColor = true;
			this->checkBox12->CheckedChanged += gcnew System::EventHandler(this, &MyForm1::checkBox_curve_CheckedChanged);
			// 
			// checkBox13
			// 
			this->checkBox13->AutoSize = true;
			this->checkBox13->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9.75F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->checkBox13->Location = System::Drawing::Point(146, 58);
			this->checkBox13->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->checkBox13->Name = L"checkBox13";
			this->checkBox13->Size = System::Drawing::Size(81, 24);
			this->checkBox13->TabIndex = 13;
			this->checkBox13->Text = L"GAH2";
			this->checkBox13->UseVisualStyleBackColor = true;
			this->checkBox13->CheckedChanged += gcnew System::EventHandler(this, &MyForm1::checkBox_curve_CheckedChanged);
			// 
			// checkBox14
			// 
			this->checkBox14->AutoSize = true;
			this->checkBox14->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9.75F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->checkBox14->Location = System::Drawing::Point(146, 86);
			this->checkBox14->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->checkBox14->Name = L"checkBox14";
			this->checkBox14->Size = System::Drawing::Size(81, 24);
			this->checkBox14->TabIndex = 14;
			this->checkBox14->Text = L"GAH3";
			this->checkBox14->UseVisualStyleBackColor = true;
			this->checkBox14->CheckedChanged += gcnew System::EventHandler(this, &MyForm1::checkBox_curve_CheckedChanged);
			// 
			// checkBox15
			// 
			this->checkBox15->AutoSize = true;
			this->checkBox15->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9.75F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->checkBox15->Location = System::Drawing::Point(146, 114);
			this->checkBox15->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->checkBox15->Name = L"checkBox15";
			this->checkBox15->Size = System::Drawing::Size(81, 24);
			this->checkBox15->TabIndex = 15;
			this->checkBox15->Text = L"GAH4";
			this->checkBox15->UseVisualStyleBackColor = true;
			this->checkBox15->CheckedChanged += gcnew System::EventHandler(this, &MyForm1::checkBox_curve_CheckedChanged);
			// 
			// checkBox16
			// 
			this->checkBox16->AutoSize = true;
			this->checkBox16->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9.75F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->checkBox16->Location = System::Drawing::Point(146, 170);
			this->checkBox16->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->checkBox16->Name = L"checkBox16";
			this->checkBox16->Size = System::Drawing::Size(81, 24);
			this->checkBox16->TabIndex = 16;
			this->checkBox16->Text = L"GPH1";
			this->checkBox16->UseVisualStyleBackColor = true;
			this->checkBox16->CheckedChanged += gcnew System::EventHandler(this, &MyForm1::checkBox_curve_CheckedChanged);
			// 
			// checkBox17
			// 
			this->checkBox17->AutoSize = true;
			this->checkBox17->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9.75F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->checkBox17->Location = System::Drawing::Point(146, 198);
			this->checkBox17->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->checkBox17->Name = L"checkBox17";
			this->checkBox17->Size = System::Drawing::Size(81, 24);
			this->checkBox17->TabIndex = 17;
			this->checkBox17->Text = L"GPH2";
			this->checkBox17->UseVisualStyleBackColor = true;
			this->checkBox17->CheckedChanged += gcnew System::EventHandler(this, &MyForm1::checkBox_curve_CheckedChanged);
			// 
			// checkBox18
			// 
			this->checkBox18->AutoSize = true;
			this->checkBox18->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9.75F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->checkBox18->Location = System::Drawing::Point(146, 226);
			this->checkBox18->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->checkBox18->Name = L"checkBox18";
			this->checkBox18->Size = System::Drawing::Size(81, 24);
			this->checkBox18->TabIndex = 18;
			this->checkBox18->Text = L"GPH3";
			this->checkBox18->UseVisualStyleBackColor = true;
			this->checkBox18->CheckedChanged += gcnew System::EventHandler(this, &MyForm1::checkBox_curve_CheckedChanged);
			// 
			// checkBox19
			// 
			this->checkBox19->AutoSize = true;
			this->checkBox19->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9.75F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->checkBox19->Location = System::Drawing::Point(146, 254);
			this->checkBox19->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->checkBox19->Name = L"checkBox19";
			this->checkBox19->Size = System::Drawing::Size(81, 24);
			this->checkBox19->TabIndex = 19;
			this->checkBox19->Text = L"GPH4";
			this->checkBox19->UseVisualStyleBackColor = true;
			this->checkBox19->CheckedChanged += gcnew System::EventHandler(this, &MyForm1::checkBox_curve_CheckedChanged);
			// 
			// checkBox20
			// 
			this->checkBox20->AutoSize = true;
			this->checkBox20->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9.75F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->checkBox20->Location = System::Drawing::Point(146, 310);
			this->checkBox20->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->checkBox20->Name = L"checkBox20";
			this->checkBox20->Size = System::Drawing::Size(78, 24);
			this->checkBox20->TabIndex = 20;
			this->checkBox20->Text = L"RoH1";
			this->checkBox20->UseVisualStyleBackColor = true;
			this->checkBox20->CheckedChanged += gcnew System::EventHandler(this, &MyForm1::checkBox_curve_CheckedChanged);
			// 
			// checkBox21
			// 
			this->checkBox21->AutoSize = true;
			this->checkBox21->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9.75F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->checkBox21->Location = System::Drawing::Point(146, 338);
			this->checkBox21->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->checkBox21->Name = L"checkBox21";
			this->checkBox21->Size = System::Drawing::Size(78, 24);
			this->checkBox21->TabIndex = 21;
			this->checkBox21->Text = L"RoH2";
			this->checkBox21->UseVisualStyleBackColor = true;
			this->checkBox21->CheckedChanged += gcnew System::EventHandler(this, &MyForm1::checkBox_curve_CheckedChanged);
			// 
			// checkBox22
			// 
			this->checkBox22->AutoSize = true;
			this->checkBox22->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9.75F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->checkBox22->Location = System::Drawing::Point(146, 366);
			this->checkBox22->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->checkBox22->Name = L"checkBox22";
			this->checkBox22->Size = System::Drawing::Size(78, 24);
			this->checkBox22->TabIndex = 22;
			this->checkBox22->Text = L"RoH3";
			this->checkBox22->UseVisualStyleBackColor = true;
			this->checkBox22->CheckedChanged += gcnew System::EventHandler(this, &MyForm1::checkBox_curve_CheckedChanged);
			// 
			// checkBox23
			// 
			this->checkBox23->AutoSize = true;
			this->checkBox23->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9.75F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->checkBox23->Location = System::Drawing::Point(146, 394);
			this->checkBox23->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->checkBox23->Name = L"checkBox23";
			this->checkBox23->Size = System::Drawing::Size(78, 24);
			this->checkBox23->TabIndex = 23;
			this->checkBox23->Text = L"RoH4";
			this->checkBox23->UseVisualStyleBackColor = true;
			this->checkBox23->CheckedChanged += gcnew System::EventHandler(this, &MyForm1::checkBox_curve_CheckedChanged);
			// 
			// tableLayoutPanel_compress
			// 
			this->tableLayoutPanel_compress->ColumnCount = 2;
			this->tableLayoutPanel_compress->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Percent,
				51.1194F)));
			this->tableLayoutPanel_compress->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Percent,
				48.8806F)));
			this->tableLayoutPanel_compress->Controls->Add(this->checkBox23, 1, 14);
			this->tableLayoutPanel_compress->Controls->Add(this->checkBox22, 1, 13);
			this->tableLayoutPanel_compress->Controls->Add(this->checkBox21, 1, 12);
			this->tableLayoutPanel_compress->Controls->Add(this->checkBox20, 1, 11);
			this->tableLayoutPanel_compress->Controls->Add(this->checkBox19, 1, 9);
			this->tableLayoutPanel_compress->Controls->Add(this->checkBox17, 1, 7);
			this->tableLayoutPanel_compress->Controls->Add(this->checkBox14, 1, 3);
			this->tableLayoutPanel_compress->Controls->Add(this->checkBox13, 1, 2);
			this->tableLayoutPanel_compress->Controls->Add(this->checkBox11, 0, 14);
			this->tableLayoutPanel_compress->Controls->Add(this->checkBox18, 1, 8);
			this->tableLayoutPanel_compress->Controls->Add(this->checkBox25, 1, 0);
			this->tableLayoutPanel_compress->Controls->Add(this->checkBox10, 0, 13);
			this->tableLayoutPanel_compress->Controls->Add(this->checkBox24, 0, 0);
			this->tableLayoutPanel_compress->Controls->Add(this->checkBox16, 1, 6);
			this->tableLayoutPanel_compress->Controls->Add(this->checkBox9, 0, 12);
			this->tableLayoutPanel_compress->Controls->Add(this->checkBox15, 1, 4);
			this->tableLayoutPanel_compress->Controls->Add(this->checkBox0, 0, 1);
			this->tableLayoutPanel_compress->Controls->Add(this->checkBox7, 0, 9);
			this->tableLayoutPanel_compress->Controls->Add(this->checkBox1, 0, 2);
			this->tableLayoutPanel_compress->Controls->Add(this->checkBox6, 0, 8);
			this->tableLayoutPanel_compress->Controls->Add(this->checkBox2, 0, 3);
			this->tableLayoutPanel_compress->Controls->Add(this->checkBox5, 0, 7);
			this->tableLayoutPanel_compress->Controls->Add(this->checkBox3, 0, 4);
			this->tableLayoutPanel_compress->Controls->Add(this->checkBox4, 0, 6);
			this->tableLayoutPanel_compress->Controls->Add(this->checkBox8, 0, 11);
			this->tableLayoutPanel_compress->Controls->Add(this->checkBox12, 1, 1);
			this->tableLayoutPanel_compress->Location = System::Drawing::Point(784, 22);
			this->tableLayoutPanel_compress->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->tableLayoutPanel_compress->Name = L"tableLayoutPanel_compress";
			this->tableLayoutPanel_compress->RowCount = 15;
			this->tableLayoutPanel_compress->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Absolute,
				28)));
			this->tableLayoutPanel_compress->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Absolute,
				28)));
			this->tableLayoutPanel_compress->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Absolute,
				28)));
			this->tableLayoutPanel_compress->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Absolute,
				28)));
			this->tableLayoutPanel_compress->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Absolute,
				28)));
			this->tableLayoutPanel_compress->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Absolute,
				28)));
			this->tableLayoutPanel_compress->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Absolute,
				28)));
			this->tableLayoutPanel_compress->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Absolute,
				28)));
			this->tableLayoutPanel_compress->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Absolute,
				28)));
			this->tableLayoutPanel_compress->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Absolute,
				28)));
			this->tableLayoutPanel_compress->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Absolute,
				28)));
			this->tableLayoutPanel_compress->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Absolute,
				28)));
			this->tableLayoutPanel_compress->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Absolute,
				28)));
			this->tableLayoutPanel_compress->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Absolute,
				28)));
			this->tableLayoutPanel_compress->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Absolute,
				28)));
			this->tableLayoutPanel_compress->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Absolute,
				20)));
			this->tableLayoutPanel_compress->Size = System::Drawing::Size(281, 418);
			this->tableLayoutPanel_compress->TabIndex = 180;
			// 
			// checkBox25
			// 
			this->checkBox25->AutoSize = true;
			this->checkBox25->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9.75F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->checkBox25->Location = System::Drawing::Point(146, 2);
			this->checkBox25->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->checkBox25->Name = L"checkBox25";
			this->checkBox25->Size = System::Drawing::Size(132, 24);
			this->checkBox25->TabIndex = 182;
			this->checkBox25->Text = L"WORK 2000";
			this->checkBox25->UseVisualStyleBackColor = true;
			// 
			// checkBox24
			// 
			this->checkBox24->AutoSize = true;
			this->checkBox24->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9.75F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->checkBox24->Location = System::Drawing::Point(3, 2);
			this->checkBox24->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->checkBox24->Name = L"checkBox24";
			this->checkBox24->Size = System::Drawing::Size(123, 24);
			this->checkBox24->TabIndex = 181;
			this->checkBox24->Text = L"WORK 400";
			this->checkBox24->UseVisualStyleBackColor = true;
			// 
			// button_set_out
			// 
			this->button_set_out->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->button_set_out->Location = System::Drawing::Point(807, 455);
			this->button_set_out->Margin = System::Windows::Forms::Padding(4, 2, 4, 2);
			this->button_set_out->Name = L"button_set_out";
			this->button_set_out->Size = System::Drawing::Size(233, 37);
			this->button_set_out->TabIndex = 181;
			this->button_set_out->Text = L"ЗАПИСАТЬ КОМАНДУ";
			this->button_set_out->UseVisualStyleBackColor = true;
			this->button_set_out->Click += gcnew System::EventHandler(this, &MyForm1::button_set_out_Click);
			// 
			// MyForm1
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(8, 16);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(1245, 939);
			this->Controls->Add(this->button_set_out);
			this->Controls->Add(this->tableLayoutPanel_compress);
			this->Controls->Add(this->panel3);
			this->Controls->Add(this->panel2);
			this->Controls->Add(this->panel_cartograph_sets);
			this->Controls->Add(this->button_set_air);
			this->Controls->Add(this->checkBox_read_only);
			this->Controls->Add(this->panel9);
			this->Controls->Add(this->panel_inc);
			this->Controls->Add(this->tableLayoutPanel1);
			this->Margin = System::Windows::Forms::Padding(4);
			this->Name = L"MyForm1";
			this->Text = L"WORK_WITH_METROLOGY_FILE";
			this->FormClosing += gcnew System::Windows::Forms::FormClosingEventHandler(this, &MyForm1::MyForm1_FormClosing);
			this->Load += gcnew System::EventHandler(this, &MyForm1::MyForm1_Load);
			this->tableLayoutPanel1->ResumeLayout(false);
			this->tableLayoutPanel1->PerformLayout();
			this->panel_inc->ResumeLayout(false);
			this->panel_inc->PerformLayout();
			this->panel9->ResumeLayout(false);
			this->panel9->PerformLayout();
			this->tableLayoutPanel3->ResumeLayout(false);
			this->tableLayoutPanel3->PerformLayout();
			this->tableLayoutPanel2->ResumeLayout(false);
			this->tableLayoutPanel2->PerformLayout();
			this->panel_cartograph_sets->ResumeLayout(false);
			this->panel_cartograph_sets->PerformLayout();
			this->panel1->ResumeLayout(false);
			this->panel1->PerformLayout();
			this->panel2->ResumeLayout(false);
			this->panel2->PerformLayout();
			this->panel3->ResumeLayout(false);
			this->panel3->PerformLayout();
			this->tableLayoutPanel_compress->ResumeLayout(false);
			this->tableLayoutPanel_compress->PerformLayout();
			this->ResumeLayout(false);
			this->PerformLayout();

		}
#pragma endregion

		
	void sells_load(uint32_t sonde_type) {
		if (sonde_type == LWD_4Tx_ || sonde_type == AUTONOM_5Tx_ || sonde_type == AUTONOM_5Tx_SDR_) {
			panel_inc->Visible = false;
			button_set_of_preset_from_sonde->Visible = false;
			button_set_of_preset_to_sonde->Visible = false;
			label_set_of_persets->Visible = false;
			panel_cartograph_sets->Visible = false;
			comboBox_work_type->Visible = false;
			label6->Visible = false;
		}
		else if (sonde_type == CARTOGRAPH_ || sonde_type == CARTOGRAPH_LWD_4Tx_) {
			panel_inc->Visible = true;
			button_set_of_preset_from_sonde->Visible = true;
			button_set_of_preset_to_sonde->Visible = true;
			label_set_of_persets->Visible = true;
			panel_cartograph_sets->Visible = true;
			comboBox_work_type->Visible = true;
			label6->Visible = true;
		}
	}

	void fill_the_form(variant<METROLOGY_CARTOGRAPH, METROLOGY_4Tx, METROLOGY_5Tx > current_metro_struct, uint32_t sonde_type) {
		unsigned int Sonde_serial;
		if (sonde_type == CARTOGRAPH_ || CARTOGRAPH_LWD_4Tx_) {
			METROLOGY_CARTOGRAPH metro = get <METROLOGY_CARTOGRAPH>(current_metro_struct);
			uint32_t buff = metro.signature << 12;
			buff = buff >> 12;
			uint32_t tool_type = buff / 100000;
			uint32_t tool_N_Tx = (buff % 100000) / 10000;
			uint32_t tool_mod = (buff % 10000) / 1000;
			uint32_t tool_number = (buff % 1000);
			uint32_t signature_for_pallete = buff / 1000;
			textBox_sign_for_pallete->Text = signature_for_pallete.ToString();
			cout<< metro.signature << " buff " << buff << " tool_type " << tool_type << " tool_N_Tx " << tool_N_Tx << " tool_mod " << tool_mod << " tool_number " << tool_number << " signature_for_pallete " << signature_for_pallete << endl;
			if ((tool_type == 1 || tool_type == 2 || tool_type == 3) && (tool_N_Tx == 3 || tool_N_Tx == 4 || tool_N_Tx == 5)) {
				comboBox_type->SelectedIndex = tool_type - 1;
				comboBox_N_Tx->SelectedIndex = tool_N_Tx - 3;
				comboBox_mod->SelectedIndex = tool_mod - 1;
				textBox_number->Text = tool_number.ToString();
				std::stringstream ss;
				ss << std::bitset<32>(metro.signature);
				textBox_sign32->Text = gcnew String((ss.str()).c_str());
			}
			else MessageBox::Show("сигнатура старого образца1");
			textBoxSer->Text = (metro.serial).ToString();
			Sonde_serial = metro.serial;
			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			for (int Tx = 0; Tx < 4; Tx++) {
				TextBox^ TEXTBOX_L1 = (TextBox^)this->tableLayoutPanel1->Controls["textBoxL1T" + (Tx + 1).ToString()];
				TEXTBOX_L1->Text = metro.L1[Tx].ToString();
				TextBox^ TEXTBOX_L2 = (TextBox^)this->tableLayoutPanel1->Controls["textBoxL2T" + (Tx + 1).ToString()];
				TEXTBOX_L2->Text = metro.L2[Tx].ToString();
			}
			for (int freq = 0; freq < 2; freq++) {
				TextBox^ TEXTBOX_F = (TextBox^)this->tableLayoutPanel1->Controls["textBoxFT" + (freq + 1).ToString()];
				TEXTBOX_F->Text = metro.F[freq].ToString();
			}

			for (int Tx_air = 0; Tx_air < 10; Tx_air++) {
				TextBox^ TEXTBOX_AIR = (TextBox^)this->tableLayoutPanel1->Controls["textBoxAIRT" + (Tx_air + 1).ToString()];
				TextBox^ TEXTBOX_MIN = (TextBox^)this->tableLayoutPanel1->Controls["textBox_min" + (Tx_air + 1).ToString()];
				if (Tx_air < 5) {
					TEXTBOX_AIR->Text = metro.Air_zz[_400_kGz][Tx_air].ToString();
					TEXTBOX_MIN->Text = metro.Air_zz_amt[_400_kGz][Tx_air].ToString();
				}
				else {
					TEXTBOX_AIR->Text = metro.Air_zz[_2000_kGz][Tx_air - 5].ToString();
					TEXTBOX_MIN->Text = metro.Air_zz_amt[_2000_kGz][Tx_air - 5].ToString();
				}
			}
			for (int i = 0; i < 7; i++) {
				TextBox^ TEXTBOX_TIME = (TextBox^)this->tableLayoutPanel3->Controls["textBoxT" + (i + 1).ToString()];
				TEXTBOX_TIME->Text = metro.turn_presets.tx_work_time_units[i].ToString();
			}

			textBox_unix_epoh->Text = metro.unix_data.ToString();
			
			//// выбор режима работы CARTOGRAPH или LWD_4Tx_
			comboBox_work_type->SelectedIndex = metro.work_type;
			//для инклинометра///////////////////////
			textBoxMA->Text = metro.inc_set.MA_WINDOW_SIZE.ToString();
			textBoxMLD->Text = metro.inc_set.MLD_WINDOW_SIZE.ToString();
			textBoxN->Text = metro.inc_set.N.ToString();
			textBoxK->Text = metro.inc_set.K.ToString();
			textBox_W_MIN->Text = (30 * metro.inc_set.W_MIN / PI).ToString("F1");
			textBox_W_MAX->Text = (30 * metro.inc_set.W_MAX / PI).ToString("F1");
			textBox_K_predict->Text = metro.inc_set.K_predict.ToString();
			textBox_APS_DELTA->Text = (57.296*metro.inc_set.APS_DELTA).ToString("F1");
			textBox_K_Delta->Text = metro.inc_set.K_delta.ToString();
			textBox_history_angle->Text = (57.296*metro.inc_set.history_angle).ToString();
			checkBox_auto_delta->Checked = metro.inc_set.auto_delta;
			checkBox_interupt->Checked = metro.inc_set.INTERRUPT_BY_ANGLE_PATH;

			//сеты пресетов
			for (int k = 0; k < 3; k++) {
				for (int j = 0; j < 5; j++) {
					TextBox^ TEXT_BOX = (TextBox^)this->tableLayoutPanel2->Controls["textBoxT" + (j + 1) + "S" + (k + 1).ToString()];
					uint32_t number = 0;
					for (int i = 0; i < 6; i++) {
						number += metro.turn_presets.Tx_by_turn[k][j][i] * pow(10, 5 - i);
					}
					TEXT_BOX->Text = number.ToString();
				}
			}

			//работа на частотах и выбор сигналов для телесистемы
			uint8_t bit = 0;
			for (int n = 0; n < 24; n++) {
				CheckBox^ CHECKBOX_CURVE;
				CHECKBOX_CURVE = (CheckBox^)tableLayoutPanel_compress->Controls["checkBox" + n];
				bit = (metro.select_key >> n) & 1;
				if (bit == 1) 
					CHECKBOX_CURVE->Checked = true;
				else
					CHECKBOX_CURVE->Checked = false;
				
			}
			bit = (metro.select_key >> Work_400) & 1;			
			if (bit == 1)
				checkBox24->Checked = true;
			else
				checkBox24->Checked = false;

			bit = (metro.select_key >> Work_2000) & 1;
			if (bit == 1)
				checkBox25->Checked = true;
			else
			
			checkBox25->Checked = false;
			//////////////////////////////////////////
			global_selected_key = metro.select_key;
            //////////////////////////////////////////

			uint32_t number = 0;
			for (int i = 0; i < 6; i++) {
				number += metro.turn_presets.Tx[i] * pow(10, 5 - i);
			}
			textBox_order->Text = number.ToString();
		}
		
		if (sonde_type == LWD_4Tx_) {
			METROLOGY_4Tx metro = get <METROLOGY_4Tx>(current_metro_struct);
			uint32_t buff = metro.signature << 12;
			buff = buff >> 12;
			uint32_t tool_type = buff / 100000;
			uint32_t tool_N_Tx = (buff % 100000) / 10000;
			uint32_t tool_mod = (buff % 10000) / 1000;
			uint32_t tool_number = (buff % 1000);
			uint32_t signature_for_pallete = buff / 1000;
			textBox_sign_for_pallete->Text = signature_for_pallete.ToString();
			//cout<< metro.signature << " buff " << buff << " tool_type " << tool_type << " tool_N_Tx " << tool_N_Tx << " tool_mod " << tool_mod << " tool_number " << tool_number << " signature_for_pallete " << signature_for_pallete << endl;
			if ((tool_type == 1 || tool_type == 2 || tool_type == 3) && (tool_N_Tx == 3 || tool_N_Tx == 4 || tool_N_Tx == 5)) {
				comboBox_type->SelectedIndex = tool_type - 1;
				comboBox_N_Tx->SelectedIndex = tool_N_Tx - 3;
				comboBox_mod->SelectedIndex = tool_mod - 1;
				textBox_number->Text = tool_number.ToString();
				std::stringstream ss;
				ss << std::bitset<32>(metro.signature);
				textBox_sign32->Text = gcnew String((ss.str()).c_str());
			}
			else MessageBox::Show("сигнатура старого образца");
			textBoxSer->Text = (metro.serial).ToString();
			Sonde_serial = metro.serial;
			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			for (int Tx = 0; Tx < 4; Tx++) {
				TextBox^ TEXTBOX_L1 = (TextBox^)this->tableLayoutPanel1->Controls["textBoxL1T" + (Tx + 1).ToString()];
				TEXTBOX_L1->Text = metro.L1[Tx].ToString();
				TextBox^ TEXTBOX_L2 = (TextBox^)this->tableLayoutPanel1->Controls["textBoxL2T" + (Tx + 1).ToString()];
				TEXTBOX_L2->Text = metro.L2[Tx].ToString();
			}
			for (int freq = 0; freq < 2; freq++) {
				TextBox^ TEXTBOX_F = (TextBox^)this->tableLayoutPanel1->Controls["textBoxFT" + (freq + 1).ToString()];
				TEXTBOX_F->Text = metro.F[4*freq].ToString();
			}
			for (int Tx_air = 0; Tx_air < 10; Tx_air++) {
				TextBox^ TEXTBOX_AIR = (TextBox^)this->tableLayoutPanel1->Controls["textBoxAIRT" + (Tx_air + 1).ToString()];
				TextBox^ TEXTBOX_MIN = (TextBox^)this->tableLayoutPanel1->Controls["textBox_min" + (Tx_air + 1).ToString()];
				if (Tx_air < 4) {
					TEXTBOX_AIR->Text = metro.Air_zz[Tx_air].ToString();
					TEXTBOX_MIN->Text = metro.Air_zz_amt[Tx_air].ToString();
				}
				else if(Tx_air > 4 && Tx_air < 9) {
					TEXTBOX_AIR->Text = metro.Air_zz[Tx_air - 1].ToString();
					TEXTBOX_MIN->Text = metro.Air_zz_amt[Tx_air - 1].ToString();
				}
			}
		
		}
		
		if (sonde_type == AUTONOM_5Tx_ || sonde_type == AUTONOM_5Tx_SDR_) {
			METROLOGY_5Tx metro = get <METROLOGY_5Tx>(current_metro_struct);
			uint32_t buff = metro.signature << 12;
			buff = buff >> 12;
			uint32_t tool_type = buff / 100000;
			uint32_t tool_N_Tx = (buff % 100000) / 10000;
			uint32_t tool_mod = (buff % 10000) / 1000;
			uint32_t tool_number = (buff % 1000);
			uint32_t signature_for_pallete = buff / 1000;
			textBox_sign_for_pallete->Text = signature_for_pallete.ToString();
			//cout<< metro.signature << " buff " << buff << " tool_type " << tool_type << " tool_N_Tx " << tool_N_Tx << " tool_mod " << tool_mod << " tool_number " << tool_number << " signature_for_pallete " << signature_for_pallete << endl;
			if ((tool_type == 1 || tool_type == 2 || tool_type == 3) && (tool_N_Tx == 3 || tool_N_Tx == 4 || tool_N_Tx == 5)) {
				comboBox_type->SelectedIndex = tool_type - 1;
				comboBox_N_Tx->SelectedIndex = tool_N_Tx - 3;
				comboBox_mod->SelectedIndex = tool_mod - 1;
				textBox_number->Text = tool_number.ToString();
				std::stringstream ss;
				ss << std::bitset<32>(metro.signature);
				textBox_sign32->Text = gcnew String((ss.str()).c_str());
			}
			else MessageBox::Show("сигнатура старого образца");
			textBoxSer->Text = (metro.serial).ToString();
			Sonde_serial = metro.serial;
			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			for (int Tx = 0; Tx < 5; Tx++) {
				TextBox^ TEXTBOX_L1 = (TextBox^)this->tableLayoutPanel1->Controls["textBoxL1T" + (Tx + 1).ToString()];
				TEXTBOX_L1->Text = metro.L1[Tx].ToString();
				TextBox^ TEXTBOX_L2 = (TextBox^)this->tableLayoutPanel1->Controls["textBoxL2T" + (Tx + 1).ToString()];
				TEXTBOX_L2->Text = metro.L2[Tx].ToString();
			}
			for (int freq = 0; freq < 2; freq++) {
				TextBox^ TEXTBOX_F = (TextBox^)this->tableLayoutPanel1->Controls["textBoxFT" + (freq + 1).ToString()];
				TEXTBOX_F->Text = metro.F[freq].ToString();
			}
			for (int Tx_air = 0; Tx_air < 10; Tx_air++) {
				TextBox^ TEXTBOX_AIR = (TextBox^)this->tableLayoutPanel1->Controls["textBoxAIRT" + (Tx_air + 1).ToString()];
				TextBox^ TEXTBOX_MIN = (TextBox^)this->tableLayoutPanel1->Controls["textBox_min" + (Tx_air + 1).ToString()];
				
				if(Tx_air < 5) {
					TEXTBOX_AIR->Text = metro.Air_zz[_400_kGz][Tx_air].ToString();
					TEXTBOX_MIN->Text = metro.Air_zz_amt[_400_kGz][Tx_air].ToString();
				}
		        else {
			    TEXTBOX_AIR->Text = metro.Air_zz[_2000_kGz][Tx_air - 5].ToString();
			    TEXTBOX_MIN->Text = metro.Air_zz_amt[_2000_kGz][Tx_air - 5].ToString();
		        }
				
				
			}

		}

	}

	variant<METROLOGY_CARTOGRAPH, METROLOGY_4Tx, METROLOGY_5Tx > fill_the_struct() {
		variant<METROLOGY_CARTOGRAPH, METROLOGY_4Tx, METROLOGY_5Tx> current_metro_struct;
		unsigned int Sonde_serial;
		if (sonde_type == CARTOGRAPH_ || CARTOGRAPH_LWD_4Tx_) {
			METROLOGY_CARTOGRAPH metro;
			memset((char*)&metro, 0x00, sizeof(METROLOGY_CARTOGRAPH));
			uint32_t tool_number = 0;
			if (textBox_number->Text != "")  tool_number = System::Int64::Parse(textBox_number->Text);

			// выбор режима работы CARTOGRAPH или LWD_4Tx_
			metro.work_type = comboBox_work_type->SelectedIndex;
			
			uint32_t buff = metro.signature >> 20;
			buff = buff << 20;
			uint32_t buff1 =
				(comboBox_type->SelectedIndex + 1) * 100000 +
				(comboBox_N_Tx->SelectedIndex + 3) * 10000 +
				(comboBox_mod->SelectedIndex + 1) * 1000 + tool_number;

			buff = buff | buff1;
			metro.signature = buff;
			if (textBoxSer->Text != "")Sonde_serial = System::Int32::Parse(textBoxSer->Text);
			metro.serial = Sonde_serial;
			for (int Tx = 0; Tx < 4; Tx++) {
				TextBox^ TEXTBOX_L1 = (TextBox^)this->tableLayoutPanel1->Controls["textBoxL1T" + (Tx + 1).ToString()];
				if (TEXTBOX_L1->Text != "") metro.L1[Tx] = System::Int64::Parse(TEXTBOX_L1->Text);
				TextBox^ TEXTBOX_L2 = (TextBox^)this->tableLayoutPanel1->Controls["textBoxL2T" + (Tx + 1).ToString()];
				if (TEXTBOX_L2->Text != "") metro.L2[Tx] = System::Int64::Parse(TEXTBOX_L2->Text);
			}
			for (int freq = 0; freq < 2; freq++) {
				TextBox^ TEXTBOX_F = (TextBox^)this->tableLayoutPanel1->Controls["textBoxFT" + (freq + 1).ToString()];
				if (TEXTBOX_F->Text != "") metro.F[freq] = System::Int64::Parse(TEXTBOX_F->Text);
			}

			for (int Tx_air = 0; Tx_air < 10; Tx_air++) {
				TextBox^ TEXTBOX_AIR = (TextBox^)this->tableLayoutPanel1->Controls["textBoxAIRT" + (Tx_air + 1).ToString()];
				TextBox^ TEXTBOX_MIN = (TextBox^)this->tableLayoutPanel1->Controls["textBox_min" + (Tx_air + 1).ToString()];
				if (Tx_air < 5) {
					if (TEXTBOX_AIR->Text != "") metro.Air_zz[_400_kGz][Tx_air] = System::Int64::Parse(TEXTBOX_AIR->Text);
					if (TEXTBOX_MIN->Text != "") metro.Air_zz_amt[_400_kGz][Tx_air] = System::Int64::Parse(TEXTBOX_MIN->Text);
				}
				else {
					if (TEXTBOX_AIR->Text != "") metro.Air_zz[_2000_kGz][Tx_air - 5] = System::Int64::Parse(TEXTBOX_AIR->Text);
					if (TEXTBOX_MIN->Text != "") metro.Air_zz_amt[_2000_kGz][Tx_air - 5] = System::Int64::Parse(TEXTBOX_MIN->Text);
				}
			}

			for (int i = 0; i < 7; i++) {
				TextBox^ TEXTBOX_TIME = (TextBox^)this->tableLayoutPanel3->Controls["textBoxT" + (i + 1).ToString()];
				if (TEXTBOX_TIME->Text != "") metro.turn_presets.tx_work_time_units[i] = System::Int64::Parse(TEXTBOX_TIME->Text);
			}

			//сеты пресетов
			for (int k = 0; k < 3; k++) {
				for (int j = 0; j < 5; j++) {
					TextBox^ TEXT_BOX = (TextBox^)this->tableLayoutPanel2->Controls["textBoxT" + (j + 1) + "S" + (k + 1).ToString()];
					uint32_t number = 0;
					if (TEXT_BOX->Text != "") number = System::Int64::Parse(TEXT_BOX->Text);
					for (int i = 0; i < 6; i++) {
						metro.turn_presets.Tx_by_turn[k][j][i] = (number / (int)pow(10, 5 - i)) % 10;
					}
				}
			}

			uint32_t number = 0;
			if (textBox_order->Text != "") number = System::Int64::Parse(textBox_order->Text);
			for (int i = 0; i < 6; i++) {
				metro.turn_presets.Tx[i] = (number / (int)pow(10, 5 - i)) % 10;
			}

			metro.unix_data = Timestamp;
			
			//для инклинометра///////////////////////
			if (textBoxMA->Text != "") metro.inc_set.MA_WINDOW_SIZE = System::Int32::Parse(textBoxMA->Text);
			if (textBoxMLD->Text != "") metro.inc_set.MLD_WINDOW_SIZE = System::Int32::Parse(textBoxMLD->Text);
			if (textBoxN->Text != "") metro.inc_set.N = System::Int32::Parse(textBoxN->Text);
			if (textBoxK->Text != "") metro.inc_set.K = System::Int32::Parse(textBoxK->Text);
			if (textBox_W_MIN->Text != "") metro.inc_set.W_MIN = (PI / 30) * System::Double::Parse(textBox_W_MIN->Text);
			if (textBox_W_MAX->Text != "") metro.inc_set.W_MAX = (PI / 30) * System::Double::Parse(textBox_W_MAX->Text);
			if (textBox_K_predict->Text != "") metro.inc_set.K_predict = System::Double::Parse(textBox_K_predict->Text);
			if (textBox_APS_DELTA->Text != "") metro.inc_set.APS_DELTA = System::Double::Parse(textBox_APS_DELTA->Text) / 57.296;
			if (textBox_K_Delta->Text != "") metro.inc_set.K_delta = System::Double::Parse(textBox_K_Delta->Text);
			if (textBox_history_angle->Text != "") metro.inc_set.history_angle = System::Double::Parse(textBox_history_angle->Text) / 57.296;
			metro.inc_set.auto_delta = checkBox_auto_delta->Checked;
			metro.inc_set.INTERRUPT_BY_ANGLE_PATH = checkBox_interupt->Checked;
			
			//работа на частотах и выбор сигналов для телесистемы
			metro.reap = reap;
			for (int n = 0; n < 24; n++) {
				CheckBox^ CHECKBOX_CURVE;
				CHECKBOX_CURVE = (CheckBox^)tableLayoutPanel_compress->Controls["checkBox" + n];
				if (CHECKBOX_CURVE->Checked == true)
					metro.select_key |= (1 << n);
			}
			if (checkBox24->Checked == true)
				metro.select_key |= (1 << Work_400);
			if (checkBox25->Checked == true)
				metro.select_key |= (1 << Work_2000);
			
			cout << " select_key " << bitset<32>(metro.select_key) << endl;

			current_metro_struct = metro;
			global_selected_key = metro.select_key;
		}

		if (sonde_type == LWD_4Tx_) {
			METROLOGY_4Tx metro;
			memset((char*)&metro, 0x00, sizeof(METROLOGY_4Tx));
			uint32_t tool_number = 0;
			if (textBox_number->Text != "")  tool_number = System::Int64::Parse(textBox_number->Text);

			uint32_t buff = metro.signature >> 20;
			buff = buff << 20;
			uint32_t buff1 =
				(comboBox_type->SelectedIndex + 1) * 100000 +
				(comboBox_N_Tx->SelectedIndex + 3) * 10000 +
				(comboBox_mod->SelectedIndex + 1) * 1000 + tool_number;

			buff = buff | buff1;
			metro.signature = buff;
			if (textBoxSer->Text != "")Sonde_serial = System::Int32::Parse(textBoxSer->Text);
			metro.serial = Sonde_serial;
			
			for (int Tx = 0; Tx < 4; Tx++) {
				TextBox^ TEXTBOX_L1 = (TextBox^)this->tableLayoutPanel1->Controls["textBoxL1T" + (Tx + 1).ToString()];
				if (TEXTBOX_L1->Text != "") { 
					metro.L1[Tx] = System::Int64::Parse(TEXTBOX_L1->Text);
					metro.L1[Tx + 4] = System::Int64::Parse(TEXTBOX_L1->Text);
				}
				TextBox^ TEXTBOX_L2 = (TextBox^)this->tableLayoutPanel1->Controls["textBoxL2T" + (Tx + 1).ToString()];
				if (TEXTBOX_L2->Text != "") {
					metro.L2[Tx] = System::Int64::Parse(TEXTBOX_L2->Text);
					metro.L2[Tx + 4] = System::Int64::Parse(TEXTBOX_L2->Text);
				}
			}
			for (int freq = 0; freq < 2; freq++) {
				TextBox^ TEXTBOX_F = (TextBox^)this->tableLayoutPanel1->Controls["textBoxFT" + (freq + 1).ToString()];
				for (int Tx = 0; Tx < 4; Tx++)
				    if (TEXTBOX_F->Text != "") metro.F[Tx + 4*freq] = System::Int64::Parse(TEXTBOX_F->Text);
			}

			for (int Tx_air = 0; Tx_air < 10; Tx_air++) {
				TextBox^ TEXTBOX_AIR = (TextBox^)this->tableLayoutPanel1->Controls["textBoxAIRT" + (Tx_air + 1).ToString()];
				TextBox^ TEXTBOX_MIN = (TextBox^)this->tableLayoutPanel1->Controls["textBox_min" + (Tx_air + 1).ToString()];
				if (Tx_air < 4) {
					if (TEXTBOX_AIR->Text != "") metro.Air_zz[Tx_air] = System::Int64::Parse(TEXTBOX_AIR->Text);
					if (TEXTBOX_MIN->Text != "") metro.Air_zz_amt[Tx_air] = System::Int64::Parse(TEXTBOX_MIN->Text);
				}
				else if (Tx_air > 4 && Tx_air < 9) {
					if (TEXTBOX_AIR->Text != "") metro.Air_zz[Tx_air - 1] = System::Int64::Parse(TEXTBOX_AIR->Text);
					if (TEXTBOX_MIN->Text != "") metro.Air_zz_amt[Tx_air - 1] = System::Int64::Parse(TEXTBOX_MIN->Text);
				}
			}

			current_metro_struct = metro;
		}

		if (sonde_type == AUTONOM_5Tx_ || sonde_type == AUTONOM_5Tx_SDR_) {
			METROLOGY_5Tx metro;
			memset((char*)&metro, 0x00, sizeof(METROLOGY_5Tx));
			uint32_t tool_number = 0;
			if (textBox_number->Text != "")  tool_number = System::Int64::Parse(textBox_number->Text);

			uint32_t buff = metro.signature >> 20;
			buff = buff << 20;
			uint32_t buff1 =
				(comboBox_type->SelectedIndex + 1) * 100000 +
				(comboBox_N_Tx->SelectedIndex + 3) * 10000 +
				(comboBox_mod->SelectedIndex + 1) * 1000 + tool_number;

			buff = buff | buff1;
			metro.signature = buff;
			if (textBoxSer->Text != "")Sonde_serial = System::Int32::Parse(textBoxSer->Text);
			metro.serial = Sonde_serial;

			for (int Tx = 0; Tx < 5; Tx++) {
				TextBox^ TEXTBOX_L1 = (TextBox^)this->tableLayoutPanel1->Controls["textBoxL1T" + (Tx + 1).ToString()];
				if (TEXTBOX_L1->Text != "") metro.L1[Tx] = System::Int64::Parse(TEXTBOX_L1->Text);					
				TextBox^ TEXTBOX_L2 = (TextBox^)this->tableLayoutPanel1->Controls["textBoxL2T" + (Tx + 1).ToString()];
				if (TEXTBOX_L2->Text != "") metro.L2[Tx] = System::Int64::Parse(TEXTBOX_L2->Text);
				
			}
			for (int freq = 0; freq < 2; freq++) {
				TextBox^ TEXTBOX_F = (TextBox^)this->tableLayoutPanel1->Controls["textBoxFT" + (freq + 1).ToString()];				
					if (TEXTBOX_F->Text != "") metro.F[freq] = System::Int64::Parse(TEXTBOX_F->Text);
			}

			for (int Tx_air = 0; Tx_air < 10; Tx_air++) {
				TextBox^ TEXTBOX_AIR = (TextBox^)this->tableLayoutPanel1->Controls["textBoxAIRT" + (Tx_air + 1).ToString()];
				TextBox^ TEXTBOX_MIN = (TextBox^)this->tableLayoutPanel1->Controls["textBox_min" + (Tx_air + 1).ToString()];
				if (Tx_air < 5) {
					if (TEXTBOX_AIR->Text != "") metro.Air_zz[_400_kGz][Tx_air] = System::Int64::Parse(TEXTBOX_AIR->Text);
					if (TEXTBOX_MIN->Text != "") metro.Air_zz_amt[_400_kGz][Tx_air] = System::Int64::Parse(TEXTBOX_MIN->Text);
				}
				else {
					if (TEXTBOX_AIR->Text != "") metro.Air_zz[_2000_kGz][Tx_air - 5] = System::Int64::Parse(TEXTBOX_AIR->Text);
					if (TEXTBOX_MIN->Text != "") metro.Air_zz_amt[_2000_kGz][Tx_air - 5] = System::Int64::Parse(TEXTBOX_MIN->Text);
				}
			}

			current_metro_struct = metro;

		}

		return current_metro_struct;
	}

	uint64_t Timestamp;

	private: System::Void buttonOpen_file_Click(System::Object^  sender, System::EventArgs^  e) {
		ifstream metro_in;
		openFileDialog1->Filter = "text files (*.bin)|*.bin";
		if (openFileDialog1->ShowDialog() == System::Windows::Forms::DialogResult::OK) {
			Metrology = (char*)Marshal::StringToHGlobalAnsi(openFileDialog1->FileName).ToPointer();
			Text = L"DATA1 " + L" METRO " + gcnew String(Metrology);
		}

		uint32_t first_read_signature = 0;
		metro_in.open(Metrology, ios::binary);
		metro_in.read((char*)&first_read_signature, sizeof(first_read_signature));
		uint32_t buff = first_read_signature << 12;
		buff = buff >> 12;
	    sonde_type = buff / 1000;// изменяет глобальный sonde_type !!!!!!!
		cout  << "sonde_type "<< sonde_type <<endl;
		metro_in.close();

		sells_load(sonde_type);

		metro_in.open(Metrology, ios::binary);
		if (sonde_type == CARTOGRAPH_) {
			METROLOGY_CARTOGRAPH current_metro_struct;
			metro_in.read((char*)&current_metro_struct, sizeof(current_metro_struct));
			metrology_GP_struct = transform_metro(current_metro_struct);
			fill_the_form(current_metro_struct, sonde_type);
		}
		if (sonde_type == LWD_4Tx_) {
			METROLOGY_4Tx current_metro_struct;
			metro_in.read((char*)&current_metro_struct, sizeof(current_metro_struct));
			metrology_GP_struct = transform_metro(current_metro_struct);
			fill_the_form(current_metro_struct, sonde_type);
		}
		if (sonde_type == AUTONOM_5Tx_ || sonde_type == AUTONOM_5Tx_SDR_) {
			METROLOGY_5Tx current_metro_struct;
			metro_in.read((char*)&current_metro_struct, sizeof(current_metro_struct));
			metrology_GP_struct = transform_metro(current_metro_struct);
			fill_the_form(current_metro_struct, sonde_type);
		}
		metro_in.close();
		metro_is_readed_from_sonde = true;
		//cout << "metrology_GP_struct.signature " << metrology_GP_struct.signature << endl;
	}

	private: System::Void buttonWrite_tofile_Click(System::Object^  sender, System::EventArgs^  e) {
		const char* Metrology_out = "Metrology.bin";
		unsigned int Sonde_serial;
		ofstream metro_out;
		
		variant<METROLOGY_CARTOGRAPH, METROLOGY_4Tx, METROLOGY_5Tx > current_metro_struct;
		current_metro_struct = fill_the_struct();

		saveFileDialog1->Filter = "text files (*.bin)|*.bin";
		if (saveFileDialog1->ShowDialog() == System::Windows::Forms::DialogResult::OK) {
			Metrology_out = (char*)Marshal::StringToHGlobalAnsi(saveFileDialog1->FileName).ToPointer();
			Text = L" METRO OUT " + gcnew String(Metrology_out);
		}
		else MessageBox::Show("Не удалось записать файл");

		metro_out.open(Metrology_out, ios::binary);
		metro_out.write((char*)&current_metro_struct, sizeof(current_metro_struct));
		metro_out.close();

	}

	private: System::Void buttonRead_from_sonde_Click(System::Object^  sender, System::EventArgs^  e) {
		unsigned int Sonde_serial;
		sonde_on_off = true;
		if (sonde_on_off == true) {
			metro_is_readed_from_sonde = false;
			metro_check = true;
			while (busy_read_from_sonde == true) {;}
			Sleep(2000);
			serialPort1->PortName = gcnew String(Com_Name.c_str());
			//Console::Write("serial metro "); Console::WriteLine(serialPort1->PortName);
			//Console::Write("sonde_type "); Console::WriteLine(sonde_type);
			variant<METROLOGY_CARTOGRAPH, METROLOGY_4Tx, METROLOGY_5Tx > current_metrology_struct;
			bool read_status;
			current_metrology_struct = read_metro_from_tool(serialPort1,  sonde_type, &read_status);
			if (read_status == true) {
				metrology_GP_struct = transform_metro(current_metrology_struct);
				fill_the_form(current_metrology_struct, sonde_type);
				MessageBox::Show("ФАЙЛ МЕТРОЛОГИИ  СЧИТАН ИЗ ПРИБОРА!");
				metro_is_readed_from_sonde = true;
			}

			else {
				MessageBox::Show("ФАЙЛ МЕТРОЛОГИИ НЕ БЫЛ СЧИТАН !!!");
				metro_is_readed_from_sonde = true;
            }
		   metro_check = false;
		}
		else {
			metro_is_readed_from_sonde = false;
			MessageBox::Show("выберите порт!");
		}
	}

	private: System::Void buttonWrite_to_sonde_Click(System::Object^  sender, System::EventArgs^  e) {
		unsigned int Sonde_serial;
		bool write_status;

		//if (MessageBox::Show("если вы не разработчик, нажмите 'Нет' ", "", MessageBoxButtons::YesNo) == System::Windows::Forms::DialogResult::Yes){}

		System::Windows::Forms::DialogResult choice = MessageBox::Show(
			this,
			"УВЕРЕНЫ?",
			"Confirm",
			MessageBoxButtons::OKCancel,
			MessageBoxIcon::Warning,
			MessageBoxDefaultButton::Button2);   // make Cancel the safe default

		switch (choice)
		{
		case System::Windows::Forms::DialogResult::OK:
			//подтвердили ///////////////////////////////////////////////////////////////
			sonde_on_off = true;
			if (sonde_on_off == true) {
				variant<METROLOGY_CARTOGRAPH, METROLOGY_4Tx, METROLOGY_5Tx > current_metro_struct;
				current_metro_struct = fill_the_struct();
				metro_check = true;
				//ждем окончания очередного цикла 
				while (busy_read_from_sonde == true) {
					;
				}
				Sleep(2000);
				serialPort1->PortName = gcnew String(Com_Name.c_str());
				
				write_metro_to_tool(serialPort1, current_metro_struct, &write_status);
				if (write_status == true) {
					MessageBox::Show("ФАЙЛ МЕТРОЛОГИИ ЗАПИСАН В ПРИБОР !");
				}
				else {
					MessageBox::Show("ФАЙЛ МЕТРОЛОГИИ НЕ БЫЛ ЗАПИСАН В ПРИБОР !");
				}
				metro_check = false;
			}
			else MessageBox::Show("Выберите порт!");
			break;
			//////////////////////////////////////////////////////////
		default:
			// не подтвердили
			return;
		}

	}

	private: System::Void button_set_of_preset_from_sonde_Click(System::Object^  sender, System::EventArgs^  e) {
		uint8_t D0 = 0;
		unsigned int Sonde_serial;
		sonde_on_off = true;
		if (sonde_on_off == true) {
			extern SET_OF_PRESETS set_of_presets;
			metro_check = true;
			
			while (busy_read_from_sonde == true) {
				;
			}
			Sleep(2000);
			bool stop = false;
			try {
				serialPort1->PortName = gcnew String(Com_Name.c_str());
				Sleep(50);
				serialPort1->ReadTimeout = 2100;
				serialPort1->WriteTimeout = 2100;
				serialPort1->BaudRate = baudrate;
				serialPort1->Open();
			}
			catch (...) { stop = 1; }

			if (!stop) {
				send_to_tool(serialPort1, CMD_GET_PRESETS, &D0, 0);
				Sleep(250);
				
				uint8_t CMD; uint16_t readed_length = 0; uint8_t DATA_IN[sizeof(SET_OF_PRESETS)] = { 0, };
				int result_receive_from_sonde = receive_from_tool(serialPort1, CMD, DATA_IN, &readed_length);

				if (result_receive_from_sonde == 0){
					memcpy(&set_of_presets, DATA_IN , sizeof(SET_OF_PRESETS));
					serialPort1->Close();
					/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
					if (set_of_presets.TBTIndex < 3)
						comboBoxTBTindex->SelectedIndex = set_of_presets.TBTIndex;
					textBox_MinRotSpeed->Text = set_of_presets.MinRotSpeed.ToString();
					textBox_unix_epoh->Text = set_of_presets.DateTime.ToString();
					checkBox_debug_session->Checked = set_of_presets.DebugSession;
					textBox_rotation->Text = set_of_presets.RotSpeed.ToString();

					Sleep(500);
					MessageBox::Show("ФАЙЛ ПРЕСЕТОВ  СЧИТАН ИЗ ПРИБОРА!");
					metro_check = false;
					//
				}
				else {
					cout << "result_receive_from_sonde " << result_receive_from_sonde << endl;
					stop = 0;
					MessageBox::Show("ФАЙЛ ПРЕСЕТОВ НЕ БЫЛ СЧИТАН !!!");
					metro_check = false;
					serialPort1->Close();
				}
			}
			else {
				stop = 0;
				MessageBox::Show("ФАЙЛ ПРЕСЕТОВ НЕ БЫЛ СЧИТАН !!!");
				metro_check = false;
				serialPort1->Close();
			}

		}
		else MessageBox::Show("выберите порт!");



	}

	private: System::Void button_set_of_preset_to_sonde_Click(System::Object^  sender, System::EventArgs^  e) {
		uint8_t D0 = 0;
		unsigned int Sonde_serial;
		sonde_on_off = true;
		if (sonde_on_off == true ) {
			extern SET_OF_PRESETS set_of_presets;
			///////////////////////////////////////////////////////////////////////
			set_of_presets.Signature = 0x5AA5;
			set_of_presets.TBTIndex = comboBoxTBTindex->SelectedIndex;
			set_of_presets.MinRotSpeed = System::Int32::Parse(textBox_MinRotSpeed->Text);
			set_of_presets.DateTime = System::Int64::Parse(textBox_unix_epoh->Text);
			set_of_presets.DateTime = Timestamp;
			set_of_presets.DebugSession = checkBox_debug_session->Checked;

			if (textBox_rotation->Text != "") set_of_presets.RotSpeed = System::Int64::Parse(textBox_rotation->Text);
			else set_of_presets.RotSpeed = 0;

			bool stop = false;
			metro_check = true;
			while (busy_read_from_sonde == true) {
				;
			}
			Sleep(2000);
			try {
				serialPort1->PortName = gcnew String(Com_Name.c_str());
				Sleep(50);
				serialPort1->ReadTimeout = 2100;
				serialPort1->WriteTimeout = 2100;
				serialPort1->BaudRate = baudrate;
				serialPort1->Open();
			}
			catch (...) { stop = 1; }

			if (!stop) {
				uint8_t DATA_OUT[sizeof(SET_OF_PRESETS)];
				memcpy(DATA_OUT , &set_of_presets, sizeof(SET_OF_PRESETS));
				send_to_tool(serialPort1, CMD_SET_PRESETS, DATA_OUT, sizeof(SET_OF_PRESETS) );

				for (int i = 0; i < sizeof(SET_OF_PRESETS) + 2; i++) {
					Console::Write("{0:x}", DATA_OUT[i] + " ");
					
				}
				Console::WriteLine("");


				Sleep(150);
				uint8_t CMD; uint16_t readed_length = 0;
				if (receive_from_tool(serialPort1, CMD, &D0, &readed_length) == 0) {
					MessageBox::Show("ФАЙЛ ПРЕСЕТОВ  ЗАПИСАН В ПРИБОР !");
				}
				else {
					MessageBox::Show("ФАЙЛ ПРЕСЕТОВ НЕ БЫЛ ЗАПИСАН В ПРИБОР !");				
				}
			}

			else {
				stop = 0;
				MessageBox::Show("ФАЙЛ ПРЕСЕТОВ НЕ БЫЛ ЗАПИСАН В ПРИБОР !!!");				
			}
			metro_check = false;
			serialPort1->Close();
		}
		else MessageBox::Show("Выберите порт!");
	}

	private: System::Void digit2(System::Object^  sender, System::Windows::Forms::KeyPressEventArgs^  e) {
		if (!Char::IsDigit(e->KeyChar) && !Char::IsControl(e->KeyChar)) e->Handled = true;
		if ((e->KeyChar) == '-') e->Handled = false; if ((e->KeyChar) == ',') e->Handled = false;
	}

	private: System::Void digit(System::Object^  sender, System::Windows::Forms::KeyPressEventArgs^  e) {
		if (!Char::IsDigit(e->KeyChar) && !Char::IsControl(e->KeyChar)) e->Handled = true;
	}

	private: System::Void checkBox_read_only_CheckedChanged(System::Object^  sender, System::EventArgs^  e) {

		if (checkBox_read_only->Checked == true) {
			for (int i = 0; i < 5; i++) {
				TextBox^ TEXTBOX_L1 = (TextBox^)this->tableLayoutPanel1->Controls["textBoxL1T" + (i + 1).ToString()];
				TEXTBOX_L1->ReadOnly = true;
				TextBox^ TEXTBOX_L2 = (TextBox^)this->tableLayoutPanel1->Controls["textBoxL2T" + (i + 1).ToString()];
				TEXTBOX_L2->ReadOnly = true;
			}
			for (int i = 0; i < 2; i++) {
				TextBox^ TEXTBOX_F = (TextBox^)this->tableLayoutPanel1->Controls["textBoxFT" + (i + 1).ToString()];
				TEXTBOX_F->ReadOnly = true;
			}

		}


		if (checkBox_read_only->Checked == false) {
			for (int i = 0; i < 5; i++) {
				TextBox^ TEXTBOX_L1 = (TextBox^)this->tableLayoutPanel1->Controls["textBoxL1T" + (i + 1).ToString()];
				TEXTBOX_L1->ReadOnly = false;
				TextBox^ TEXTBOX_L2 = (TextBox^)this->tableLayoutPanel1->Controls["textBoxL2T" + (i + 1).ToString()];
				TEXTBOX_L2->ReadOnly = false;
			}
			for (int i = 0; i < 2; i++) {
				TextBox^ TEXTBOX_F = (TextBox^)this->tableLayoutPanel1->Controls["textBoxFT" + (i + 1).ToString()];
				TEXTBOX_F->ReadOnly = false;
			}

		}

	}

	private: System::Void button_set_air_Click(System::Object^  sender, System::EventArgs^  e) {
		for (int i = 0; i < 10; i++) {
			TextBox^ TEXTBOX_AIR = (TextBox^)this->tableLayoutPanel1->Controls["textBoxAIRT" + (i + 1).ToString()];
			TextBox^ TEXTBOX_MIN = (TextBox^)this->tableLayoutPanel1->Controls["textBox_min" + (i + 1).ToString()];
			int air_zero = 0;
			TEXTBOX_AIR->Text = air_zero.ToString();
			//TEXTBOX_MIN->Text = metrology_struct.Air_zz_amt[0][i].ToString();		
		}

		button5->PerformClick();
	}

private: System::Void MyForm1_FormClosing(System::Object^  sender, System::Windows::Forms::FormClosingEventArgs^  e) {
	metro_form_is_open = false;
}

private: System::Void MyForm1_Load(System::Object^  sender, System::EventArgs^  e) {
	 sells_load(sonde_type);
}

private: System::Void button_set_out_Click(System::Object^  sender, System::EventArgs^  e) {
	uint8_t D0 = 0;
	uint32_t select_key = 0x00;
	for (int n = 0; n < 24; n++) {
		CheckBox^ CHECKBOX_CURVE;
		CHECKBOX_CURVE = (CheckBox^)tableLayoutPanel_compress->Controls["checkBox" + n];
		if (CHECKBOX_CURVE->Checked == true)
			select_key |= (1 << n);
	}
	if (checkBox24->Checked == true)
		select_key |= (1 << Work_400);
	if (checkBox25->Checked == true)
		select_key |= (1 << Work_2000);
	cout << " select_key " << bitset<32>(select_key) << endl;


	if (sonde_on_off == true) {
		

		bool stop = false;
		metro_check = true;
		while (busy_read_from_sonde == true) {
			;
		}
		Sleep(2000);
		try {
			serialPort1->PortName = gcnew String(Com_Name.c_str());
			Sleep(50);
			serialPort1->ReadTimeout = 2100;
			serialPort1->WriteTimeout = 2100;
			serialPort1->BaudRate = baudrate;
			serialPort1->Open();
		}
		catch (...) { stop = 1; }

		if (!stop) {
			uint8_t DATA_OUT[sizeof(uint32_t) + 2];
			memcpy(DATA_OUT, &select_key, sizeof(uint32_t));
			send_to_tool(serialPort1, CMD_SET_TELESYSTEM, DATA_OUT, sizeof(uint32_t));

			for (int i = 0; i < sizeof(uint32_t) + 2; i++) {
				Console::Write("{0:x}", DATA_OUT[i] + " ");

			}
			Console::WriteLine("");


			Sleep(150);
			uint8_t CMD; uint16_t readed_length = 0;
			if (receive_from_tool(serialPort1, CMD, &D0, &readed_length) == 0) {
				MessageBox::Show("НАСТРОЙКИ ЗАПИСАНЫ В ПРИБОР !");
			}
			else {
				MessageBox::Show("НАСТРОЙКИ НЕ БЫЛИ ЗАПИСАНЫ В ПРИБОР !");
			}
		}

		else {
			stop = 0;
			MessageBox::Show("НАСТРОЙКИ НЕ БЫЛИ ЗАПИСАНЫ В ПРИБОР !!!");
		}
		metro_check = false;
		serialPort1->Close();
	}
	else MessageBox::Show("Выберите порт!");




}

private: System::Void checkBox_curve_CheckedChanged(System::Object^  sender, System::EventArgs^  e) {
	TO_PACK pack = {0,};
	uint32_t all_bits = 0;
	
	set_mean_bits(&reap, &pack);
	
	for (int n = 0; n < 24; n++) {
		CheckBox^ CHECKBOX_CURVE;
		CHECKBOX_CURVE = (CheckBox^)tableLayoutPanel_compress->Controls["checkBox" + n];
		if (CHECKBOX_CURVE->Checked == true)
			all_bits += pack.G[n].bits;
	}


	uint32_t bytes = all_bits / 8;
	uint32_t rest = all_bits % 8;
	if (rest > 0)bytes++;

	//cout << " total bits " << all_bits << " rest " << 8 - rest << " total bytes " << bytes << endl;
}



private: System::Void comboBox_work_type_SelectedIndexChanged(System::Object^  sender, System::EventArgs^  e) {
	//
	if (comboBox_work_type->SelectedIndex == 1)comboBox_mod->SelectedIndex = 8;
}
};
}
