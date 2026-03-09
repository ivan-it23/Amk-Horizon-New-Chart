#pragma once
#include "windows.h"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <limits>
#include <complex> 
#include <string> 
#include <string_view> 
#include "Param_Form.h"
#include <typeinfo.h>
#include <winuser.h>
#include <locale.h>
#include <vector>
#include "MetroForm.h"
#include <chrono> 
#include <tchar.h> 
#include <algorithm>    // std::sort
#include <urlmon.h>
#include <shellapi.h>
#include "ShellAPI.h"
#include <msclr/marshal_cppstd.h>
#include <cstdlib>
#pragma comment(lib,"Winmm.lib")
#pragma warning(disable:4244)
#pragma comment (lib, "urlmon.lib")
#pragma comment (lib, "Shell32.lib")

extern ALL_DATA all_data_buff_;
extern int sonde_type;
extern vector<ALL_DATA> all_data;
extern vector<_4Tx_DATA> _4Tx_data;
extern vector<_5Tx_DATA> _5Tx_data;
extern vector<GP_DATA> gp_data;
extern bool metro_is_readed_from_sonde;
extern METROLOGY_GP metrology_GP_struct;
extern bool sonde_on_off;
extern unsigned int surf_interval;
extern unsigned int Points;
extern unsigned int get_data_interval;
extern unsigned int baudrate;
extern bool check_update;
extern string local_file_to_ftp_upload;
extern string Com_Name;
extern string Dataout;
extern string Dataout_for_save;
extern string extension_for_save;
extern PH_SHIFT ph_shift;

extern uint32_t global_selected_key;

namespace AIRTEMP10 {

	using namespace System;
	using namespace System::Timers;
	using namespace System::IO;
	using namespace System::IO::Ports;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;
	using namespace System::Runtime::InteropServices;
	using namespace System::Windows::Forms::DataVisualization::Charting;
	using namespace System::Threading;

	using namespace std;
	using namespace System::IO::Ports;
	using namespace System::Windows;
	/// <summary>
	/// Сводка для MyForm

	/// </summary>
	public ref class MyForm : public System::Windows::Forms::Form
	{
	public:
		//AllData *Gleb;
		//DAT *S,*S1;
		//PHASE *Phase;
		//PHASE *Phase_smt;
		//bool PH_shift = true;


		int *index_freq = new int[2];
		int *index_rzx = new int[2];
		int *index_amp = new int[2];
		int *index_dir_undir = new int[2];
		int *undir_selected_index = new int[2];
		int *direct_selected_index = new int[2];

	private: System::Timers::Timer^ Timer_get_data_from_sonde;

	private: System::Windows::Forms::TextBox^  textBox5;

	private: System::Windows::Forms::DataVisualization::Charting::Chart^  chart1;
	private: System::Windows::Forms::TextBox^  textBox_cursor1;
	private: System::Windows::Forms::TextBox^  textBox_cursor2;

	private: System::Windows::Forms::TextBox^  textBox_min1;
	private: System::Windows::Forms::TextBox^  textBox_max1;

	private: System::Windows::Forms::ComboBox^  comboBox_choise_chart1;

	private: System::Windows::Forms::Button^  button5;
	private: System::Windows::Forms::ContextMenuStrip^  contextMenuStrip1;

	private: System::Windows::Forms::ComboBox^  comboBox_choise_chart2;
	private: System::Windows::Forms::TextBox^  textBox_min2;

	private: System::Windows::Forms::TextBox^  textBox_max2;

	private: System::Windows::Forms::Button^  button2;
	private: System::Windows::Forms::Timer^  timer_read_file;

	private: System::Windows::Forms::CheckBox^  checkBox1;
	private: System::Windows::Forms::CheckBox^  checkBox2;

	private: System::Windows::Forms::TableLayoutPanel^  tableLayoutPanel1;

	private: System::Windows::Forms::TableLayoutPanel^  tableLayoutPanel4;

	private: System::Windows::Forms::CheckBox^  checkBox8;

	private: System::IO::Ports::SerialPort^  serialPort1;

	private: System::Windows::Forms::TableLayoutPanel^  tableLayoutPanel10;

	private: System::Windows::Forms::SaveFileDialog^  saveFileDialog1;



	public:
		ofstream *fout;
		ofstream *AF;
	private: System::Windows::Forms::ToolStripMenuItem^  N_Raw_Data_point_1;
	public:

	public:

	public:


	private: System::Windows::Forms::ContextMenuStrip^  contextMenuStrip2;
	private: System::Windows::Forms::ToolStripMenuItem^  N_Raw_Data_point_2;
	private: System::Windows::Forms::Button^  button_400_2000_1;




	private:

	private: System::Windows::Forms::Button^  button_400_2000_2;

	private: System::Windows::Forms::ToolStripLabel^  toolStripLabel8;
	private: System::Windows::Forms::ToolStripSeparator^  toolStripSeparator31;
	private: System::Windows::Forms::ToolStripSeparator^  toolStripSeparator32;
	private: System::Windows::Forms::ToolStripLabel^  toolStripLabel9;
	private: System::Windows::Forms::ToolStripSeparator^  toolStripSeparator33;
	private: System::Windows::Forms::ToolStripTextBox^  toolStripTextBox_Ro_zp;
	private: System::Windows::Forms::ToolStripSeparator^  toolStripSeparator34;
	private: System::Windows::Forms::ToolStripLabel^  toolStripLabel10;
	private: System::Windows::Forms::ToolStripSeparator^  toolStripSeparator35;
	private: System::Windows::Forms::ToolStripTextBox^  toolStripTextBox_R_zp;
	private: System::Windows::Forms::ToolStripSeparator^  toolStripSeparator36;
	private: System::Windows::Forms::ToolStripButton^  прямая_задача;
	private: System::Windows::Forms::ToolStripSeparator^  toolStripSeparator37;
	private: System::Windows::Forms::ToolStripLabel^  toolStripLabel11;
	private: System::Windows::Forms::ToolStripSeparator^  toolStripSeparator38;
	private: System::Windows::Forms::ToolStripComboBox^  comboBox_axis;

	private: System::Windows::Forms::FolderBrowserDialog^  folderBrowserDialog1;
	private: System::Windows::Forms::CheckBox^  checkBox_sonde_calk;
	private: System::Windows::Forms::CheckBox^  checkBox_re_im;



	private: System::Windows::Forms::ProgressBar^  progressBar1;
	private: System::Windows::Forms::Label^  label_T;
	private: System::Windows::Forms::MenuStrip^  menuStrip1;
	private: System::Windows::Forms::ToolStripMenuItem^  файлToolStripMenuItem;
	private: System::Windows::Forms::ToolStripMenuItem^  открытьToolStripMenuItem;
	private: System::Windows::Forms::ToolStripMenuItem^  ToolStripMenuItem_data_file;
	private: System::Windows::Forms::ToolStripMenuItem^  ToolStripMenuItem_temporary_file;
	private: System::Windows::Forms::ToolStripMenuItem^  ToolStripMenuItem_save_file;

	private: System::Windows::Forms::ToolStripMenuItem^  toolStripMenuItem_delete;
	private: System::Windows::Forms::ToolStripMenuItem^  ToolStripMenuItem_parametrs;

	private: System::Windows::Forms::ToolStripSeparator^  toolStripSeparator1;

	private: System::Windows::Forms::ToolStripComboBox^  toolStripComboBox_sonde_type;

	private: System::Windows::Forms::ToolStrip^  toolStrip2;
	private: System::Windows::Forms::ToolStripSeparator^  toolStripSeparator15;
	private: System::Windows::Forms::ToolStripButton^  toolStripButton_on_off;
	private: System::Windows::Forms::ToolStripSeparator^  toolStripSeparator39;
	private: System::Windows::Forms::ToolStripButton^  buttonSURF;
	private: System::Windows::Forms::ToolStripSeparator^  toolStripSeparator40;
	private: System::Windows::Forms::ToolStripSeparator^  toolStripSeparator7;
	private: System::Windows::Forms::ToolStripButton^  toolStripButton3;
	private: System::Windows::Forms::ToolStripSeparator^  toolStripSeparator8;
	private: System::Windows::Forms::ToolStripSeparator^  toolStripSeparator41;
	private: System::Windows::Forms::ToolStripButton^  toolStripButton_metro_file;
	private: System::Windows::Forms::ToolStripSeparator^  toolStripSeparator11;

	private: System::Windows::Forms::ToolStripSeparator^  toolStripSeparator12;
	private: System::Windows::Forms::DataVisualization::Charting::Chart^  chart3;
	private: System::Windows::Forms::Button^  button_ZX_1;


	private: System::Windows::Forms::CheckBox^  checkBox9;
	public protected:
	private: System::Windows::Forms::Button^  button_ZX_2;
	private: System::Windows::Forms::TableLayoutPanel^  tableLayoutPanel5;

	private: System::Windows::Forms::CheckBox^  checkBoxT14;
	private: System::Windows::Forms::CheckBox^  checkBoxT13;
	private: System::Windows::Forms::CheckBox^  checkBoxT12;
	private: System::Windows::Forms::CheckBox^  checkBoxT11;
	private: System::Windows::Forms::Button^  button_back_1;

	public protected: System::Windows::Forms::Button^  button_re_im_1;
	private: System::Windows::Forms::Button^  button_forward_1;
	private: System::Windows::Forms::TableLayoutPanel^  tableLayoutPanel6;
	public protected:

	public protected:
	private:

	private:


	private: System::Windows::Forms::CheckBox^  checkBoxT23;

	private: System::Windows::Forms::CheckBox^  checkBoxT22;

	private: System::Windows::Forms::CheckBox^  checkBoxT21;


	private: System::Windows::Forms::Button^  button_back_2;
	public protected: System::Windows::Forms::Button^  button_re_im_2;
	private: System::Windows::Forms::Button^  button_forward_2;
	private: System::Windows::Forms::CheckBox^  checkBoxT24;
	private: System::Windows::Forms::DataVisualization::Charting::Chart^  chart2;
	private: System::Windows::Forms::DataVisualization::Charting::Chart^  chart4;
	private: System::Windows::Forms::Label^  label_dv1;
	private: System::Windows::Forms::Label^  label_dv2;
	private: System::Windows::Forms::Panel^  panel_Sonde_Picture;
	private: System::Windows::Forms::PictureBox^  pictureBoxD12;
	private: System::Windows::Forms::PictureBox^  pictureBoxD10;
	private: System::Windows::Forms::PictureBox^  pictureBoxD11;
	private: System::Windows::Forms::PictureBox^  pictureBoxD9;
	private: System::Windows::Forms::Label^  labelR3;
	private: System::Windows::Forms::Label^  labelR4;


	private: System::Windows::Forms::Label^  labelT4;
	private: System::Windows::Forms::Label^  labelT3;


	private: System::Windows::Forms::Label^  labelT5;
	private: System::Windows::Forms::Label^  labelR2;



	private: System::Windows::Forms::Label^  labelR1;

	private: System::Windows::Forms::Label^  labelT2;
	private: System::Windows::Forms::Label^  labelT1;
	private: System::Windows::Forms::PictureBox^  pictureBoxD5;
	private: System::Windows::Forms::PictureBox^  pictureBoxD6;
	private: System::Windows::Forms::PictureBox^  pictureBoxT5;

	private: System::Windows::Forms::PictureBox^  pictureBoxD7;
	private: System::Windows::Forms::PictureBox^  pictureBoxD8;
	private: System::Windows::Forms::PictureBox^  pictureBoxD3;
	private: System::Windows::Forms::PictureBox^  pictureBoxD4;
	private: System::Windows::Forms::PictureBox^  pictureBoxD1;
	private: System::Windows::Forms::PictureBox^  pictureBoxD2;
	private: System::Windows::Forms::PictureBox^  pictureBoxT2;

	private: System::Windows::Forms::PictureBox^  pictureBoxR4;
	private: System::Windows::Forms::PictureBox^  pictureBoxT4;


	private: System::Windows::Forms::PictureBox^  pictureBoxR2;

	private: System::Windows::Forms::PictureBox^  pictureBoxR1;

	private: System::Windows::Forms::PictureBox^  pictureBoxT1;
	private: System::Windows::Forms::PictureBox^  pictureBoxT3;



	private: System::Windows::Forms::PictureBox^  pictureBoxR3;
	private: System::Windows::Forms::Button^  button_direct2;
	private: System::Windows::Forms::TableLayoutPanel^  tableLayoutPanel3;

	private: System::Windows::Forms::Button^  button_direct1;
	private: System::Windows::Forms::ToolStripMenuItem^  сохрвнитьДляАлехToolStripMenuItem;
	private: System::Windows::Forms::CheckBox^  checkBox_add_file;
	private: System::Windows::Forms::CheckBox^  checkBox_stable_ph;
	private: System::Windows::Forms::TrackBar^  trackBar1;
	private: System::Windows::Forms::Panel^  panel1;
	private: System::Windows::Forms::Label^  label_ro;
	private: System::Windows::Forms::TextBox^  textBox_T5;
	private: System::Windows::Forms::TextBox^  textBox_T4;
	private: System::Windows::Forms::TextBox^  textBox_T3;
	private: System::Windows::Forms::TextBox^  textBox_T2;
	private: System::Windows::Forms::TextBox^  textBox_T1;
	private: System::Windows::Forms::ToolStripMenuItem^  toolStripMenuItemT1;
	private: System::Windows::Forms::ToolStripMenuItem^  toolStripMenuItemT2;
	private: System::Windows::Forms::ToolStripMenuItem^  toolStripMenuItemT3;
	private: System::Windows::Forms::ToolStripMenuItem^  toolStripMenuItemT4;
	private: System::Windows::Forms::ToolStripMenuItem^  toolStripMenuItemT5;
	private: System::Windows::Forms::ToolStripMenuItem^  toolStripMenuItemmin1;
	private: System::Windows::Forms::ToolStripMenuItem^  toolStripMenuItemmax1;
	private: System::Windows::Forms::ToolStripMenuItem^  toolStripMenuItemmin2;
	private: System::Windows::Forms::ToolStripMenuItem^  toolStripMenuItemmax2;
	private: System::Windows::Forms::PictureBox^  pictureBoxD13;









	public protected:
	private:

	private:

	public protected:

	public protected:
	private:


	public:

	public:

		ifstream *fin;

		MyForm(void)
		{

			InitializeComponent();

			//label_ro->Text = "Ro = infinity";
			//_controlfp(_CW_DEFAULT, 0xfffff);
			for (int i = 0; i < 8; i++) {
				//chart1->Series[0]->ToolTip = "X = #VALX{0.##}, Y = #VALY{0.##}"; 
				//chart2->Series[0]->ToolTip = "X = #VALX{0.##}, Y = #VALY{0.##}";
			}
			form_width = this->Width - 450;
			//toolStripComboBox_sonde_type->SelectedIndex = 1;

			CreateDirectory("C:\\EXP", NULL);
			checkBox_sonde_calk->Checked = false;
			comboBox_choise_chart1->SelectedIndex = 0;
			comboBox_choise_chart2->SelectedIndex = 0;
			progressBar1->Visible = false;

			checkBoxT11->Checked = true; checkBoxT12->Checked = true; checkBoxT13->Checked = true; checkBoxT14->Checked = true;
			checkBoxT21->Checked = true; checkBoxT22->Checked = true; checkBoxT23->Checked = true; checkBoxT24->Checked = true;

			// загружаем из файла установки 
			fout = new ofstream;

			ifstream conf_in;
			conf_in.open(ConfName, ios::in);
			if (conf_in.is_open()) {
				conf_in >> surf_interval;//1
				conf_in >> Points;//2
				conf_in >> get_data_interval;//3
				conf_in >> check_update;//4
				conf_in >> Com_Name;//5
				conf_in >> local_file_to_ftp_upload;	//6	
				conf_in >> index_dir_undir[up_chart];//7    0-undir; 1 - direct 
				conf_in >> index_dir_undir[down_chart];//8  0-undir; 1 - direct 	  
				conf_in >> index_rzx[up_chart];//9          0-left_; 1 - right_ 
				conf_in >> index_rzx[down_chart];//10       0-left_; 1 - right_ 
				conf_in >> index_freq[up_chart];//11        0-400; 1 - 2000 
				conf_in >> index_freq[down_chart];//12      0-400; 1 - 2000 
				conf_in >> undir_selected_index[up_chart];//13
				conf_in >> undir_selected_index[down_chart];//14
				conf_in >> direct_selected_index[up_chart];//15
				conf_in >> direct_selected_index[down_chart];//16
				int idx;
				conf_in >> idx;
				if (idx != 0 && idx != 1 && idx != 2 && idx != 3 && idx != 40)
					idx = 1;
				toolStripComboBox_sonde_type->SelectedIndex = idx;
				conf_in.close();

				for (int i = 0; i < 2; i++) {
					if (index_dir_undir[i] != 0 && index_dir_undir[i] != 1)
						index_dir_undir[i] = 0;

					if (index_rzx[i] != 0 && index_rzx[i] != 1) {
						index_rzx[i] = 0;
					}

					if (index_freq[i] != 0 && index_freq[i] != 1) {
						index_freq[i] = 0;
					}


					if (undir_selected_index[i] < 0 || undir_selected_index[i] > 11)
						undir_selected_index[i] = 0;
					if (direct_selected_index[i] < 0 || direct_selected_index[i] > 18)
						direct_selected_index[i] = 0;

				}

			}

			else {
				index_dir_undir[up_chart] = 0;
				index_dir_undir[down_chart] = 0;
				index_rzx[up_chart] = 0;
				index_rzx[down_chart] = 0;
				index_freq[up_chart] = 1;
				index_freq[down_chart] = 1;
				direct_selected_index[0] = 0;
				direct_selected_index[1] = 0;
				undir_selected_index[0] = 0;
				undir_selected_index[1] = 0;
				comboBox_choise_chart1->SelectedIndex = 0;
				comboBox_choise_chart2->SelectedIndex = 0;
			}

			comboBox_choise_chart1->Items->Clear();
			comboBox_choise_chart2->Items->Clear();
			//////////////////////////////////////////////////////////////////////////////////////////////////////////////
			// up_chart
			if (index_dir_undir[up_chart] == 0) {
				this->button_direct1->Text = "UNDIR 1";
				this->button_ZX_1->Visible = false;
				comboBox_choise_chart1->Items->AddRange(gcnew cli::array< System::Object^  >(12) {
					L"PH Z2-Z1",//1
						L"AM Z2/Z1",//2
						L"AM Z1",//3 
						L"AM Z2",//4
						L"AM Zero ZZ",//5	
						L"Temp RZZ",//6
						L"PH_SMT_from_computer",//7		
						L"УЭС_from_computer",//8
						L"PH_SMT_from_sonde",//9
						L"УЭС_from_sonde",//10		
						L"Погрешность УЭС",//11	
						"FRAME",//12						
				});
				//if(undir_selected_index[up_chart] >= 0 && undir_selected_index[up_chart] < 11)
				comboBox_choise_chart1->SelectedIndex = undir_selected_index[up_chart];
				//else
				//    comboBox_choise_chart1->SelectedIndex = 1;
				//cout << "UNDIR chart 1 " << up_chart << "  comboBox_choise_chart1->SelectedIndex " << comboBox_choise_chart1->SelectedIndex << endl;
			}

			else if (index_dir_undir[up_chart] == 1) {
				this->button_direct1->Text = "DIRECT 1";
				this->button_ZX_1->Visible = true;
				comboBox_choise_chart1->Items->AddRange(gcnew cli::array< System::Object^  >(18) {
					L"AM_GEO",//1
						L"PH_GEO",//2
						L"AM_V_ZX",//3				
						L"AM_V_ZZ",//4
						L"betta_z",//5
						L"PH_VZX / VZZ",//6
						L"AM_V_XX",//7
						L"dV",//8
						L"SNR",//9
						L"AM Zero ZX",//10
						L"AM_GEO_SMT",//11
						L"PH_GEO_SMT",//12
						L"BORDER_ANGLE",//13
						L"INC Abs; Zen; Azm",//14
						L"ANGULAR_VELOCITY",//15
						L"START_SECTOR",//16
						L"Temp RZX",//17
						L"FRAME",//18
				});
				//if (direct_selected_index[up_chart] >= 0 && direct_selected_index[up_chart] < 18)
				comboBox_choise_chart1->SelectedIndex = direct_selected_index[up_chart];
				//else
				//    comboBox_choise_chart1->SelectedIndex = 1;	
				//cout << "DIRECT chart 1 " << up_chart << "  comboBox_choise_chart1->SelectedIndex " << comboBox_choise_chart1->SelectedIndex << endl;
			}
			/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
						//down_chart
			if (index_dir_undir[down_chart] == 0) {
				this->button_direct2->Text = "UNDIR 2";
				this->button_ZX_2->Visible = false;
				comboBox_choise_chart2->Items->AddRange(gcnew cli::array< System::Object^  >(12) {
					L"PH Z2-Z1",//1
						L"AM Z2/Z1",//2
						L"AM Z1",//3 
						L"AM Z2",//4
						L"AM Zero ZZ",//5	
						L"Temp RZZ",//6
						L"PH_SMT_from_computer",//7
						L"УЭС_from_computer",//8
						L"PH_SMT_from_sonde",//9
						L"УЭС_from_sonde",//10	
						L"Погрешность УЭС",//11	
						"FRAME",//12					
				});
				//if (undir_selected_index[down_chart] >= 0 && undir_selected_index[down_chart] < 11)
				comboBox_choise_chart2->SelectedIndex = undir_selected_index[down_chart];
				//else
				//    comboBox_choise_chart2->SelectedIndex = 1;
				//cout << "UNDIR chart 2 " << down_chart << "  comboBox_choise_chart2->SelectedIndex " << comboBox_choise_chart2->SelectedIndex << endl;
			}

			else if (index_dir_undir[down_chart] == 1) {
				this->button_direct2->Text = "DIRECT 2";
				this->button_ZX_2->Visible = true;
				comboBox_choise_chart2->Items->AddRange(gcnew cli::array< System::Object^  >(18) {
					L"AM_GEO",//1
						L"PH_GEO",//2
						L"AM_V_ZX",//3				
						L"AM_V_ZZ",//4
						L"betta_z",//5
						L"PH_VZX / VZZ",//6
						L"AM_V_XX",//7
						L"dV",//8
						L"SNR",//9
						L"AM Zero ZX",//10
						L"AM_GEO_SMT",//11
						L"PH_GEO_SMT",//12
						L"BORDER_ANGLE",//13
						L"INC Abs; Zen; Azm",//14
						L"ANGULAR_VELOCITY",//15
						L"START_SECTOR",//16
						L"Temp RZX",//17
						L"FRAME",//18
				});
				//if (direct_selected_index[down_chart] >= 0 && direct_selected_index[down_chart] < 18)
				comboBox_choise_chart2->SelectedIndex = direct_selected_index[down_chart];
				//else
				//    comboBox_choise_chart2->SelectedIndex = 1;
				//cout << "DIRECT chart 2 " << down_chart << "  comboBox_choise_chart2->SelectedIndex " << comboBox_choise_chart2->SelectedIndex << endl;

			}
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


			if (index_rzx[up_chart] == Left_)this->button_ZX_1->Text = "R_Zx L";
			else this->button_ZX_1->Text = "R_Zx R";
			if (index_rzx[down_chart] == Left_)this->button_ZX_2->Text = "R_Zx L";
			else this->button_ZX_2->Text = "R_Zx R";

			if (index_freq[up_chart] == 0)this->button_400_2000_1->Text = "400";
			else this->button_400_2000_1->Text = "2000";
			if (index_freq[down_chart] == 0)this->button_400_2000_2->Text = "400";
			else this->button_400_2000_2->Text = "2000";

			index_amp[0] = 0;
			index_amp[1] = 0;

			min_n[0] = 0; min_n[1] = 0; max_n[0] = 0; max_n[1] = 0;

			n_raw_data_point[0] = 0;
			n_raw_data_point[1] = 0;
			cursor_x_position[0] = 0;
			cursor_x_position[1] = 0;

			for (int j = 0; j < 8; j++) {
				PictureBox^ PICTUREBOX = (PictureBox^)this->panel_Sonde_Picture->Controls["pictureBoxD" + (j + 1).ToString()];
				PICTUREBOX->Visible = false;
			}
			for (int j = 8; j < 12; j++) {
				PictureBox^ PICTUREBOX = (PictureBox^)this->panel_Sonde_Picture->Controls["pictureBoxD" + (j + 1).ToString()];
				PICTUREBOX->Visible = true;
			}


			pictureBoxR3->Visible = false; pictureBoxR4->Visible = false; pictureBoxT5->Visible = false;
			labelR3->Visible = false; labelR4->Visible = false; labelT5->Visible = false;

			Read_Port_Thread->Start();
			checkBox_stable_ph->Checked = true;
			trackBar1->Value = 11;
			N_Raw_Data_point_1->Visible = false;
			N_Raw_Data_point_2->Visible = false;
			toolStripMenuItemT1->Visible = false;
			toolStripMenuItemT2->Visible = false;
			toolStripMenuItemT3->Visible = false;
			toolStripMenuItemT4->Visible = false;
			toolStripMenuItemT5->Visible = false;
			pictureBoxD13->Visible = false;;
		}

	protected:
		/// <summary>
		/// Освободить все используемые ресурсы.
		/// </summary>
		~MyForm()
		{
			if (components)
			{
				delete components;
			}
		}
	private: System::Windows::Forms::OpenFileDialog^  openFileDialog;
	private: System::Windows::Forms::OpenFileDialog^  openFileDialog1;


	private: System::ComponentModel::IContainer^  components;

	protected:

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
			System::Windows::Forms::DataVisualization::Charting::ChartArea^  chartArea1 = (gcnew System::Windows::Forms::DataVisualization::Charting::ChartArea());
			System::Windows::Forms::DataVisualization::Charting::Legend^  legend1 = (gcnew System::Windows::Forms::DataVisualization::Charting::Legend());
			System::Windows::Forms::DataVisualization::Charting::Series^  series1 = (gcnew System::Windows::Forms::DataVisualization::Charting::Series());
			System::Windows::Forms::DataVisualization::Charting::Series^  series2 = (gcnew System::Windows::Forms::DataVisualization::Charting::Series());
			System::Windows::Forms::DataVisualization::Charting::Series^  series3 = (gcnew System::Windows::Forms::DataVisualization::Charting::Series());
			System::Windows::Forms::DataVisualization::Charting::Series^  series4 = (gcnew System::Windows::Forms::DataVisualization::Charting::Series());
			System::Windows::Forms::DataVisualization::Charting::Series^  series5 = (gcnew System::Windows::Forms::DataVisualization::Charting::Series());
			System::Windows::Forms::DataVisualization::Charting::Series^  series6 = (gcnew System::Windows::Forms::DataVisualization::Charting::Series());
			System::Windows::Forms::DataVisualization::Charting::Series^  series7 = (gcnew System::Windows::Forms::DataVisualization::Charting::Series());
			System::Windows::Forms::DataVisualization::Charting::Series^  series8 = (gcnew System::Windows::Forms::DataVisualization::Charting::Series());
			System::Windows::Forms::DataVisualization::Charting::Series^  series9 = (gcnew System::Windows::Forms::DataVisualization::Charting::Series());
			System::Windows::Forms::DataVisualization::Charting::Series^  series10 = (gcnew System::Windows::Forms::DataVisualization::Charting::Series());
			System::Windows::Forms::DataVisualization::Charting::Series^  series11 = (gcnew System::Windows::Forms::DataVisualization::Charting::Series());
			System::Windows::Forms::DataVisualization::Charting::ChartArea^  chartArea2 = (gcnew System::Windows::Forms::DataVisualization::Charting::ChartArea());
			System::Windows::Forms::DataVisualization::Charting::Legend^  legend2 = (gcnew System::Windows::Forms::DataVisualization::Charting::Legend());
			System::Windows::Forms::DataVisualization::Charting::Series^  series12 = (gcnew System::Windows::Forms::DataVisualization::Charting::Series());
			System::Windows::Forms::DataVisualization::Charting::Series^  series13 = (gcnew System::Windows::Forms::DataVisualization::Charting::Series());
			System::Windows::Forms::DataVisualization::Charting::Series^  series14 = (gcnew System::Windows::Forms::DataVisualization::Charting::Series());
			System::Windows::Forms::DataVisualization::Charting::Series^  series15 = (gcnew System::Windows::Forms::DataVisualization::Charting::Series());
			System::Windows::Forms::DataVisualization::Charting::Series^  series16 = (gcnew System::Windows::Forms::DataVisualization::Charting::Series());
			System::Windows::Forms::DataVisualization::Charting::Series^  series17 = (gcnew System::Windows::Forms::DataVisualization::Charting::Series());
			System::Windows::Forms::DataVisualization::Charting::Series^  series18 = (gcnew System::Windows::Forms::DataVisualization::Charting::Series());
			System::Windows::Forms::DataVisualization::Charting::Series^  series19 = (gcnew System::Windows::Forms::DataVisualization::Charting::Series());
			System::Windows::Forms::DataVisualization::Charting::Series^  series20 = (gcnew System::Windows::Forms::DataVisualization::Charting::Series());
			System::Windows::Forms::DataVisualization::Charting::Series^  series21 = (gcnew System::Windows::Forms::DataVisualization::Charting::Series());
			System::Windows::Forms::DataVisualization::Charting::Series^  series22 = (gcnew System::Windows::Forms::DataVisualization::Charting::Series());
			System::Windows::Forms::DataVisualization::Charting::Series^  series23 = (gcnew System::Windows::Forms::DataVisualization::Charting::Series());
			System::Windows::Forms::DataVisualization::Charting::ChartArea^  chartArea3 = (gcnew System::Windows::Forms::DataVisualization::Charting::ChartArea());
			System::Windows::Forms::DataVisualization::Charting::Legend^  legend3 = (gcnew System::Windows::Forms::DataVisualization::Charting::Legend());
			System::Windows::Forms::DataVisualization::Charting::Series^  series24 = (gcnew System::Windows::Forms::DataVisualization::Charting::Series());
			System::Windows::Forms::DataVisualization::Charting::Series^  series25 = (gcnew System::Windows::Forms::DataVisualization::Charting::Series());
			System::Windows::Forms::DataVisualization::Charting::Series^  series26 = (gcnew System::Windows::Forms::DataVisualization::Charting::Series());
			System::Windows::Forms::DataVisualization::Charting::Series^  series27 = (gcnew System::Windows::Forms::DataVisualization::Charting::Series());
			System::Windows::Forms::DataVisualization::Charting::Series^  series28 = (gcnew System::Windows::Forms::DataVisualization::Charting::Series());
			System::Windows::Forms::DataVisualization::Charting::Series^  series29 = (gcnew System::Windows::Forms::DataVisualization::Charting::Series());
			System::Windows::Forms::DataVisualization::Charting::Series^  series30 = (gcnew System::Windows::Forms::DataVisualization::Charting::Series());
			System::Windows::Forms::DataVisualization::Charting::Series^  series31 = (gcnew System::Windows::Forms::DataVisualization::Charting::Series());
			System::Windows::Forms::DataVisualization::Charting::Series^  series32 = (gcnew System::Windows::Forms::DataVisualization::Charting::Series());
			System::Windows::Forms::DataVisualization::Charting::Series^  series33 = (gcnew System::Windows::Forms::DataVisualization::Charting::Series());
			System::Windows::Forms::DataVisualization::Charting::Series^  series34 = (gcnew System::Windows::Forms::DataVisualization::Charting::Series());
			System::Windows::Forms::DataVisualization::Charting::Series^  series35 = (gcnew System::Windows::Forms::DataVisualization::Charting::Series());
			System::Windows::Forms::DataVisualization::Charting::ChartArea^  chartArea4 = (gcnew System::Windows::Forms::DataVisualization::Charting::ChartArea());
			System::Windows::Forms::DataVisualization::Charting::Legend^  legend4 = (gcnew System::Windows::Forms::DataVisualization::Charting::Legend());
			System::Windows::Forms::DataVisualization::Charting::Series^  series36 = (gcnew System::Windows::Forms::DataVisualization::Charting::Series());
			System::Windows::Forms::DataVisualization::Charting::Series^  series37 = (gcnew System::Windows::Forms::DataVisualization::Charting::Series());
			System::Windows::Forms::DataVisualization::Charting::Series^  series38 = (gcnew System::Windows::Forms::DataVisualization::Charting::Series());
			System::Windows::Forms::DataVisualization::Charting::Series^  series39 = (gcnew System::Windows::Forms::DataVisualization::Charting::Series());
			System::Windows::Forms::DataVisualization::Charting::Series^  series40 = (gcnew System::Windows::Forms::DataVisualization::Charting::Series());
			System::Windows::Forms::DataVisualization::Charting::Series^  series41 = (gcnew System::Windows::Forms::DataVisualization::Charting::Series());
			System::Windows::Forms::DataVisualization::Charting::Series^  series42 = (gcnew System::Windows::Forms::DataVisualization::Charting::Series());
			System::Windows::Forms::DataVisualization::Charting::Series^  series43 = (gcnew System::Windows::Forms::DataVisualization::Charting::Series());
			System::Windows::Forms::DataVisualization::Charting::Series^  series44 = (gcnew System::Windows::Forms::DataVisualization::Charting::Series());
			System::Windows::Forms::DataVisualization::Charting::Series^  series45 = (gcnew System::Windows::Forms::DataVisualization::Charting::Series());
			System::Windows::Forms::DataVisualization::Charting::Series^  series46 = (gcnew System::Windows::Forms::DataVisualization::Charting::Series());
			System::ComponentModel::ComponentResourceManager^  resources = (gcnew System::ComponentModel::ComponentResourceManager(MyForm::typeid));
			this->openFileDialog = (gcnew System::Windows::Forms::OpenFileDialog());
			this->openFileDialog1 = (gcnew System::Windows::Forms::OpenFileDialog());
			this->textBox5 = (gcnew System::Windows::Forms::TextBox());
			this->chart1 = (gcnew System::Windows::Forms::DataVisualization::Charting::Chart());
			this->contextMenuStrip1 = (gcnew System::Windows::Forms::ContextMenuStrip(this->components));
			this->N_Raw_Data_point_1 = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->toolStripMenuItemT1 = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->toolStripMenuItemT2 = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->toolStripMenuItemT3 = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->toolStripMenuItemT4 = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->toolStripMenuItemT5 = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->toolStripMenuItemmin1 = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->toolStripMenuItemmax1 = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->textBox_cursor1 = (gcnew System::Windows::Forms::TextBox());
			this->textBox_cursor2 = (gcnew System::Windows::Forms::TextBox());
			this->textBox_min1 = (gcnew System::Windows::Forms::TextBox());
			this->textBox_max1 = (gcnew System::Windows::Forms::TextBox());
			this->saveFileDialog1 = (gcnew System::Windows::Forms::SaveFileDialog());
			this->comboBox_choise_chart1 = (gcnew System::Windows::Forms::ComboBox());
			this->button5 = (gcnew System::Windows::Forms::Button());
			this->comboBox_choise_chart2 = (gcnew System::Windows::Forms::ComboBox());
			this->textBox_min2 = (gcnew System::Windows::Forms::TextBox());
			this->textBox_max2 = (gcnew System::Windows::Forms::TextBox());
			this->button2 = (gcnew System::Windows::Forms::Button());
			this->timer_read_file = (gcnew System::Windows::Forms::Timer(this->components));
			this->checkBox1 = (gcnew System::Windows::Forms::CheckBox());
			this->checkBox2 = (gcnew System::Windows::Forms::CheckBox());
			this->contextMenuStrip2 = (gcnew System::Windows::Forms::ContextMenuStrip(this->components));
			this->N_Raw_Data_point_2 = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->toolStripMenuItemmin2 = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->toolStripMenuItemmax2 = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->tableLayoutPanel1 = (gcnew System::Windows::Forms::TableLayoutPanel());
			this->panel_Sonde_Picture = (gcnew System::Windows::Forms::Panel());
			this->pictureBoxD13 = (gcnew System::Windows::Forms::PictureBox());
			this->pictureBoxD12 = (gcnew System::Windows::Forms::PictureBox());
			this->pictureBoxD10 = (gcnew System::Windows::Forms::PictureBox());
			this->pictureBoxD11 = (gcnew System::Windows::Forms::PictureBox());
			this->pictureBoxD9 = (gcnew System::Windows::Forms::PictureBox());
			this->labelR3 = (gcnew System::Windows::Forms::Label());
			this->labelR4 = (gcnew System::Windows::Forms::Label());
			this->labelT4 = (gcnew System::Windows::Forms::Label());
			this->labelT3 = (gcnew System::Windows::Forms::Label());
			this->labelT5 = (gcnew System::Windows::Forms::Label());
			this->labelR2 = (gcnew System::Windows::Forms::Label());
			this->labelR1 = (gcnew System::Windows::Forms::Label());
			this->labelT2 = (gcnew System::Windows::Forms::Label());
			this->labelT1 = (gcnew System::Windows::Forms::Label());
			this->pictureBoxD5 = (gcnew System::Windows::Forms::PictureBox());
			this->pictureBoxD6 = (gcnew System::Windows::Forms::PictureBox());
			this->pictureBoxT5 = (gcnew System::Windows::Forms::PictureBox());
			this->pictureBoxD7 = (gcnew System::Windows::Forms::PictureBox());
			this->pictureBoxD8 = (gcnew System::Windows::Forms::PictureBox());
			this->pictureBoxD3 = (gcnew System::Windows::Forms::PictureBox());
			this->pictureBoxD4 = (gcnew System::Windows::Forms::PictureBox());
			this->pictureBoxD1 = (gcnew System::Windows::Forms::PictureBox());
			this->pictureBoxD2 = (gcnew System::Windows::Forms::PictureBox());
			this->pictureBoxT2 = (gcnew System::Windows::Forms::PictureBox());
			this->pictureBoxR4 = (gcnew System::Windows::Forms::PictureBox());
			this->pictureBoxT4 = (gcnew System::Windows::Forms::PictureBox());
			this->pictureBoxR2 = (gcnew System::Windows::Forms::PictureBox());
			this->pictureBoxR1 = (gcnew System::Windows::Forms::PictureBox());
			this->pictureBoxT1 = (gcnew System::Windows::Forms::PictureBox());
			this->pictureBoxT3 = (gcnew System::Windows::Forms::PictureBox());
			this->pictureBoxR3 = (gcnew System::Windows::Forms::PictureBox());
			this->tableLayoutPanel5 = (gcnew System::Windows::Forms::TableLayoutPanel());
			this->checkBoxT13 = (gcnew System::Windows::Forms::CheckBox());
			this->checkBoxT12 = (gcnew System::Windows::Forms::CheckBox());
			this->checkBoxT11 = (gcnew System::Windows::Forms::CheckBox());
			this->button_back_1 = (gcnew System::Windows::Forms::Button());
			this->button_forward_1 = (gcnew System::Windows::Forms::Button());
			this->checkBoxT14 = (gcnew System::Windows::Forms::CheckBox());
			this->label_dv1 = (gcnew System::Windows::Forms::Label());
			this->button_re_im_1 = (gcnew System::Windows::Forms::Button());
			this->tableLayoutPanel4 = (gcnew System::Windows::Forms::TableLayoutPanel());
			this->button_direct2 = (gcnew System::Windows::Forms::Button());
			this->button_ZX_2 = (gcnew System::Windows::Forms::Button());
			this->button_400_2000_2 = (gcnew System::Windows::Forms::Button());
			this->checkBox9 = (gcnew System::Windows::Forms::CheckBox());
			this->tableLayoutPanel10 = (gcnew System::Windows::Forms::TableLayoutPanel());
			this->toolStrip2 = (gcnew System::Windows::Forms::ToolStrip());
			this->toolStripSeparator15 = (gcnew System::Windows::Forms::ToolStripSeparator());
			this->toolStripButton_on_off = (gcnew System::Windows::Forms::ToolStripButton());
			this->toolStripSeparator39 = (gcnew System::Windows::Forms::ToolStripSeparator());
			this->buttonSURF = (gcnew System::Windows::Forms::ToolStripButton());
			this->toolStripSeparator40 = (gcnew System::Windows::Forms::ToolStripSeparator());
			this->toolStripSeparator7 = (gcnew System::Windows::Forms::ToolStripSeparator());
			this->toolStripButton3 = (gcnew System::Windows::Forms::ToolStripButton());
			this->toolStripSeparator8 = (gcnew System::Windows::Forms::ToolStripSeparator());
			this->toolStripSeparator41 = (gcnew System::Windows::Forms::ToolStripSeparator());
			this->toolStripButton_metro_file = (gcnew System::Windows::Forms::ToolStripButton());
			this->toolStripSeparator11 = (gcnew System::Windows::Forms::ToolStripSeparator());
			this->toolStripSeparator12 = (gcnew System::Windows::Forms::ToolStripSeparator());
			this->checkBox_sonde_calk = (gcnew System::Windows::Forms::CheckBox());
			this->menuStrip1 = (gcnew System::Windows::Forms::MenuStrip());
			this->файлToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->открытьToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->ToolStripMenuItem_data_file = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->ToolStripMenuItem_temporary_file = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->ToolStripMenuItem_save_file = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->toolStripMenuItem_delete = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->ToolStripMenuItem_parametrs = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->toolStripSeparator1 = (gcnew System::Windows::Forms::ToolStripSeparator());
			this->сохрвнитьДляАлехToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->toolStripComboBox_sonde_type = (gcnew System::Windows::Forms::ToolStripComboBox());
			this->label_T = (gcnew System::Windows::Forms::Label());
			this->checkBox_add_file = (gcnew System::Windows::Forms::CheckBox());
			this->checkBox_re_im = (gcnew System::Windows::Forms::CheckBox());
			this->checkBox_stable_ph = (gcnew System::Windows::Forms::CheckBox());
			this->progressBar1 = (gcnew System::Windows::Forms::ProgressBar());
			this->tableLayoutPanel6 = (gcnew System::Windows::Forms::TableLayoutPanel());
			this->checkBoxT23 = (gcnew System::Windows::Forms::CheckBox());
			this->checkBoxT22 = (gcnew System::Windows::Forms::CheckBox());
			this->checkBoxT21 = (gcnew System::Windows::Forms::CheckBox());
			this->button_back_2 = (gcnew System::Windows::Forms::Button());
			this->button_re_im_2 = (gcnew System::Windows::Forms::Button());
			this->button_forward_2 = (gcnew System::Windows::Forms::Button());
			this->checkBoxT24 = (gcnew System::Windows::Forms::CheckBox());
			this->label_dv2 = (gcnew System::Windows::Forms::Label());
			this->chart3 = (gcnew System::Windows::Forms::DataVisualization::Charting::Chart());
			this->chart4 = (gcnew System::Windows::Forms::DataVisualization::Charting::Chart());
			this->chart2 = (gcnew System::Windows::Forms::DataVisualization::Charting::Chart());
			this->tableLayoutPanel3 = (gcnew System::Windows::Forms::TableLayoutPanel());
			this->checkBox8 = (gcnew System::Windows::Forms::CheckBox());
			this->button_direct1 = (gcnew System::Windows::Forms::Button());
			this->button_ZX_1 = (gcnew System::Windows::Forms::Button());
			this->button_400_2000_1 = (gcnew System::Windows::Forms::Button());
			this->panel1 = (gcnew System::Windows::Forms::Panel());
			this->textBox_T5 = (gcnew System::Windows::Forms::TextBox());
			this->textBox_T4 = (gcnew System::Windows::Forms::TextBox());
			this->textBox_T3 = (gcnew System::Windows::Forms::TextBox());
			this->textBox_T2 = (gcnew System::Windows::Forms::TextBox());
			this->textBox_T1 = (gcnew System::Windows::Forms::TextBox());
			this->label_ro = (gcnew System::Windows::Forms::Label());
			this->trackBar1 = (gcnew System::Windows::Forms::TrackBar());
			this->serialPort1 = (gcnew System::IO::Ports::SerialPort(this->components));
			this->toolStripLabel8 = (gcnew System::Windows::Forms::ToolStripLabel());
			this->toolStripSeparator31 = (gcnew System::Windows::Forms::ToolStripSeparator());
			this->toolStripSeparator32 = (gcnew System::Windows::Forms::ToolStripSeparator());
			this->toolStripLabel9 = (gcnew System::Windows::Forms::ToolStripLabel());
			this->toolStripSeparator33 = (gcnew System::Windows::Forms::ToolStripSeparator());
			this->toolStripTextBox_Ro_zp = (gcnew System::Windows::Forms::ToolStripTextBox());
			this->toolStripSeparator34 = (gcnew System::Windows::Forms::ToolStripSeparator());
			this->toolStripLabel10 = (gcnew System::Windows::Forms::ToolStripLabel());
			this->toolStripSeparator35 = (gcnew System::Windows::Forms::ToolStripSeparator());
			this->toolStripTextBox_R_zp = (gcnew System::Windows::Forms::ToolStripTextBox());
			this->toolStripSeparator36 = (gcnew System::Windows::Forms::ToolStripSeparator());
			this->прямая_задача = (gcnew System::Windows::Forms::ToolStripButton());
			this->toolStripSeparator37 = (gcnew System::Windows::Forms::ToolStripSeparator());
			this->toolStripLabel11 = (gcnew System::Windows::Forms::ToolStripLabel());
			this->toolStripSeparator38 = (gcnew System::Windows::Forms::ToolStripSeparator());
			this->comboBox_axis = (gcnew System::Windows::Forms::ToolStripComboBox());
			this->folderBrowserDialog1 = (gcnew System::Windows::Forms::FolderBrowserDialog());
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->chart1))->BeginInit();
			this->contextMenuStrip1->SuspendLayout();
			this->contextMenuStrip2->SuspendLayout();
			this->tableLayoutPanel1->SuspendLayout();
			this->panel_Sonde_Picture->SuspendLayout();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBoxD13))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBoxD12))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBoxD10))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBoxD11))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBoxD9))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBoxD5))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBoxD6))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBoxT5))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBoxD7))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBoxD8))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBoxD3))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBoxD4))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBoxD1))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBoxD2))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBoxT2))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBoxR4))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBoxT4))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBoxR2))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBoxR1))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBoxT1))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBoxT3))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBoxR3))->BeginInit();
			this->tableLayoutPanel5->SuspendLayout();
			this->tableLayoutPanel4->SuspendLayout();
			this->tableLayoutPanel10->SuspendLayout();
			this->toolStrip2->SuspendLayout();
			this->menuStrip1->SuspendLayout();
			this->tableLayoutPanel6->SuspendLayout();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->chart3))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->chart4))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->chart2))->BeginInit();
			this->tableLayoutPanel3->SuspendLayout();
			this->panel1->SuspendLayout();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->trackBar1))->BeginInit();
			this->SuspendLayout();
			// 
			// textBox5
			// 
			this->textBox5->BackColor = System::Drawing::SystemColors::Menu;
			this->textBox5->BorderStyle = System::Windows::Forms::BorderStyle::FixedSingle;
			this->textBox5->Cursor = System::Windows::Forms::Cursors::No;
			this->textBox5->Enabled = false;
			this->textBox5->Location = System::Drawing::Point(1232, 486);
			this->textBox5->Margin = System::Windows::Forms::Padding(2);
			this->textBox5->Name = L"textBox5";
			this->textBox5->ReadOnly = true;
			this->textBox5->Size = System::Drawing::Size(114, 22);
			this->textBox5->TabIndex = 1;
			this->textBox5->Text = L"GEOM not SET";
			this->textBox5->Visible = false;
			// 
			// chart1
			// 
			this->chart1->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Bottom)
				| System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->chart1->BorderlineColor = System::Drawing::Color::Black;
			this->chart1->BorderlineDashStyle = System::Windows::Forms::DataVisualization::Charting::ChartDashStyle::Solid;
			chartArea1->CursorX->IsUserEnabled = true;
			chartArea1->CursorX->IsUserSelectionEnabled = true;
			chartArea1->CursorY->Interval = 0.0001;
			chartArea1->CursorY->IsUserEnabled = true;
			chartArea1->CursorY->IsUserSelectionEnabled = true;
			chartArea1->Name = L"ChartArea1";
			this->chart1->ChartAreas->Add(chartArea1);
			this->chart1->ContextMenuStrip = this->contextMenuStrip1;
			legend1->Name = L"Legend1";
			legend1->Position->Auto = false;
			legend1->Position->Height = 5;
			legend1->Position->Width = 25;
			legend1->Position->X = 30;
			legend1->Position->Y = 1;
			this->chart1->Legends->Add(legend1);
			this->chart1->Location = System::Drawing::Point(3, 80);
			this->chart1->Margin = System::Windows::Forms::Padding(2);
			this->chart1->Name = L"chart1";
			series1->BorderWidth = 2;
			series1->ChartArea = L"ChartArea1";
			series1->ChartType = System::Windows::Forms::DataVisualization::Charting::SeriesChartType::Line;
			series1->Color = System::Drawing::Color::Green;
			series1->Legend = L"Legend1";
			series1->Name = L"T1";
			series2->BorderWidth = 2;
			series2->ChartArea = L"ChartArea1";
			series2->ChartType = System::Windows::Forms::DataVisualization::Charting::SeriesChartType::Line;
			series2->Color = System::Drawing::Color::DarkGoldenrod;
			series2->Legend = L"Legend1";
			series2->Name = L"T2";
			series3->BorderWidth = 2;
			series3->ChartArea = L"ChartArea1";
			series3->ChartType = System::Windows::Forms::DataVisualization::Charting::SeriesChartType::Line;
			series3->Color = System::Drawing::Color::Red;
			series3->Legend = L"Legend1";
			series3->Name = L"T3";
			series4->BorderWidth = 2;
			series4->ChartArea = L"ChartArea1";
			series4->ChartType = System::Windows::Forms::DataVisualization::Charting::SeriesChartType::Line;
			series4->Color = System::Drawing::Color::RoyalBlue;
			series4->Legend = L"Legend1";
			series4->Name = L"T4";
			series5->BorderWidth = 2;
			series5->ChartArea = L"ChartArea1";
			series5->ChartType = System::Windows::Forms::DataVisualization::Charting::SeriesChartType::Line;
			series5->Color = System::Drawing::Color::Magenta;
			series5->Legend = L"Legend1";
			series5->Name = L"T5";
			series6->BorderDashStyle = System::Windows::Forms::DataVisualization::Charting::ChartDashStyle::Dash;
			series6->BorderWidth = 2;
			series6->ChartArea = L"ChartArea1";
			series6->ChartType = System::Windows::Forms::DataVisualization::Charting::SeriesChartType::Line;
			series6->Color = System::Drawing::Color::Green;
			series6->IsVisibleInLegend = false;
			series6->Legend = L"Legend1";
			series6->Name = L"T1_";
			series7->BorderDashStyle = System::Windows::Forms::DataVisualization::Charting::ChartDashStyle::Dash;
			series7->BorderWidth = 2;
			series7->ChartArea = L"ChartArea1";
			series7->ChartType = System::Windows::Forms::DataVisualization::Charting::SeriesChartType::Line;
			series7->Color = System::Drawing::Color::DarkGoldenrod;
			series7->IsVisibleInLegend = false;
			series7->Legend = L"Legend1";
			series7->Name = L"T2_";
			series8->BorderDashStyle = System::Windows::Forms::DataVisualization::Charting::ChartDashStyle::Dash;
			series8->BorderWidth = 2;
			series8->ChartArea = L"ChartArea1";
			series8->ChartType = System::Windows::Forms::DataVisualization::Charting::SeriesChartType::Line;
			series8->Color = System::Drawing::Color::Red;
			series8->IsVisibleInLegend = false;
			series8->Legend = L"Legend1";
			series8->Name = L"T3_";
			series9->BorderDashStyle = System::Windows::Forms::DataVisualization::Charting::ChartDashStyle::Dash;
			series9->BorderWidth = 2;
			series9->ChartArea = L"ChartArea1";
			series9->ChartType = System::Windows::Forms::DataVisualization::Charting::SeriesChartType::Line;
			series9->Color = System::Drawing::Color::RoyalBlue;
			series9->IsVisibleInLegend = false;
			series9->Legend = L"Legend1";
			series9->Name = L"T4_";
			series10->BorderDashStyle = System::Windows::Forms::DataVisualization::Charting::ChartDashStyle::Dash;
			series10->BorderWidth = 2;
			series10->ChartArea = L"ChartArea1";
			series10->ChartType = System::Windows::Forms::DataVisualization::Charting::SeriesChartType::Line;
			series10->Color = System::Drawing::Color::Magenta;
			series10->Legend = L"Legend1";
			series10->Name = L"T5_";
			series11->BorderDashStyle = System::Windows::Forms::DataVisualization::Charting::ChartDashStyle::DashDot;
			series11->BorderWidth = 2;
			series11->ChartArea = L"ChartArea1";
			series11->ChartType = System::Windows::Forms::DataVisualization::Charting::SeriesChartType::Line;
			series11->Color = System::Drawing::Color::Black;
			series11->Legend = L"Legend1";
			series11->Name = L"zero";
			this->chart1->Series->Add(series1);
			this->chart1->Series->Add(series2);
			this->chart1->Series->Add(series3);
			this->chart1->Series->Add(series4);
			this->chart1->Series->Add(series5);
			this->chart1->Series->Add(series6);
			this->chart1->Series->Add(series7);
			this->chart1->Series->Add(series8);
			this->chart1->Series->Add(series9);
			this->chart1->Series->Add(series10);
			this->chart1->Series->Add(series11);
			this->chart1->Size = System::Drawing::Size(1392, 360);
			this->chart1->TabIndex = 9;
			this->chart1->Text = L"CCCC";
			this->chart1->TextAntiAliasingQuality = System::Windows::Forms::DataVisualization::Charting::TextAntiAliasingQuality::Normal;
			this->chart1->CursorPositionChanged += gcnew System::EventHandler<System::Windows::Forms::DataVisualization::Charting::CursorEventArgs^ >(this, &MyForm::cursor_position);
			// 
			// contextMenuStrip1
			// 
			this->contextMenuStrip1->ImageScalingSize = System::Drawing::Size(18, 18);
			this->contextMenuStrip1->Items->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(8) {
				this->N_Raw_Data_point_1,
					this->toolStripMenuItemT1, this->toolStripMenuItemT2, this->toolStripMenuItemT3, this->toolStripMenuItemT4, this->toolStripMenuItemT5,
					this->toolStripMenuItemmin1, this->toolStripMenuItemmax1
			});
			this->contextMenuStrip1->Name = L"contextMenuStrip1";
			this->contextMenuStrip1->Size = System::Drawing::Size(186, 196);
			// 
			// N_Raw_Data_point_1
			// 
			this->N_Raw_Data_point_1->Name = L"N_Raw_Data_point_1";
			this->N_Raw_Data_point_1->Size = System::Drawing::Size(185, 24);
			this->N_Raw_Data_point_1->Text = L"Raw_Data_point";
			this->N_Raw_Data_point_1->Click += gcnew System::EventHandler(this, &MyForm::N_Raw_Data_point_1_Click);
			// 
			// toolStripMenuItemT1
			// 
			this->toolStripMenuItemT1->Name = L"toolStripMenuItemT1";
			this->toolStripMenuItemT1->Size = System::Drawing::Size(185, 24);
			this->toolStripMenuItemT1->Text = L"T1";
			this->toolStripMenuItemT1->Click += gcnew System::EventHandler(this, &MyForm::toolStripMenuItemT1_Click);
			// 
			// toolStripMenuItemT2
			// 
			this->toolStripMenuItemT2->Name = L"toolStripMenuItemT2";
			this->toolStripMenuItemT2->Size = System::Drawing::Size(185, 24);
			this->toolStripMenuItemT2->Text = L"T2";
			this->toolStripMenuItemT2->Click += gcnew System::EventHandler(this, &MyForm::toolStripMenuItemT2_Click);
			// 
			// toolStripMenuItemT3
			// 
			this->toolStripMenuItemT3->Name = L"toolStripMenuItemT3";
			this->toolStripMenuItemT3->Size = System::Drawing::Size(185, 24);
			this->toolStripMenuItemT3->Text = L"T3";
			this->toolStripMenuItemT3->Click += gcnew System::EventHandler(this, &MyForm::toolStripMenuItemT3_Click);
			// 
			// toolStripMenuItemT4
			// 
			this->toolStripMenuItemT4->Name = L"toolStripMenuItemT4";
			this->toolStripMenuItemT4->Size = System::Drawing::Size(185, 24);
			this->toolStripMenuItemT4->Text = L"T4";
			this->toolStripMenuItemT4->Click += gcnew System::EventHandler(this, &MyForm::toolStripMenuItemT4_Click);
			// 
			// toolStripMenuItemT5
			// 
			this->toolStripMenuItemT5->Name = L"toolStripMenuItemT5";
			this->toolStripMenuItemT5->Size = System::Drawing::Size(185, 24);
			this->toolStripMenuItemT5->Text = L"T5";
			this->toolStripMenuItemT5->Click += gcnew System::EventHandler(this, &MyForm::toolStripMenuItemT5_Click);
			// 
			// toolStripMenuItemmin1
			// 
			this->toolStripMenuItemmin1->Name = L"toolStripMenuItemmin1";
			this->toolStripMenuItemmin1->Size = System::Drawing::Size(185, 24);
			this->toolStripMenuItemmin1->Text = L"min";
			this->toolStripMenuItemmin1->Click += gcnew System::EventHandler(this, &MyForm::toolStripMenuItemmin1_Click);
			// 
			// toolStripMenuItemmax1
			// 
			this->toolStripMenuItemmax1->Name = L"toolStripMenuItemmax1";
			this->toolStripMenuItemmax1->Size = System::Drawing::Size(185, 24);
			this->toolStripMenuItemmax1->Text = L"max";
			this->toolStripMenuItemmax1->Click += gcnew System::EventHandler(this, &MyForm::toolStripMenuItemmax1_Click);
			// 
			// textBox_cursor1
			// 
			this->textBox_cursor1->BackColor = System::Drawing::SystemColors::Window;
			this->textBox_cursor1->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 10.2F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->textBox_cursor1->Location = System::Drawing::Point(758, 0);
			this->textBox_cursor1->Margin = System::Windows::Forms::Padding(6, 0, 6, 0);
			this->textBox_cursor1->Name = L"textBox_cursor1";
			this->textBox_cursor1->ReadOnly = true;
			this->textBox_cursor1->Size = System::Drawing::Size(215, 27);
			this->textBox_cursor1->TabIndex = 10;
			// 
			// textBox_cursor2
			// 
			this->textBox_cursor2->BackColor = System::Drawing::SystemColors::Window;
			this->textBox_cursor2->Cursor = System::Windows::Forms::Cursors::IBeam;
			this->textBox_cursor2->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9.75F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->textBox_cursor2->Location = System::Drawing::Point(758, 0);
			this->textBox_cursor2->Margin = System::Windows::Forms::Padding(6, 0, 6, 0);
			this->textBox_cursor2->Name = L"textBox_cursor2";
			this->textBox_cursor2->ReadOnly = true;
			this->textBox_cursor2->Size = System::Drawing::Size(215, 26);
			this->textBox_cursor2->TabIndex = 11;
			// 
			// textBox_min1
			// 
			this->textBox_min1->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9.75F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->textBox_min1->Location = System::Drawing::Point(524, 0);
			this->textBox_min1->Margin = System::Windows::Forms::Padding(6, 0, 6, 0);
			this->textBox_min1->Name = L"textBox_min1";
			this->textBox_min1->Size = System::Drawing::Size(74, 26);
			this->textBox_min1->TabIndex = 15;
			this->textBox_min1->TextChanged += gcnew System::EventHandler(this, &MyForm::min_n_change);
			this->textBox_min1->KeyPress += gcnew System::Windows::Forms::KeyPressEventHandler(this, &MyForm::digit2);
			// 
			// textBox_max1
			// 
			this->textBox_max1->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9.75F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->textBox_max1->Location = System::Drawing::Point(610, 0);
			this->textBox_max1->Margin = System::Windows::Forms::Padding(6, 0, 6, 0);
			this->textBox_max1->Name = L"textBox_max1";
			this->textBox_max1->Size = System::Drawing::Size(74, 26);
			this->textBox_max1->TabIndex = 16;
			this->textBox_max1->TextChanged += gcnew System::EventHandler(this, &MyForm::max_n_change);
			this->textBox_max1->KeyPress += gcnew System::Windows::Forms::KeyPressEventHandler(this, &MyForm::digit2);
			// 
			// comboBox_choise_chart1
			// 
			this->comboBox_choise_chart1->DropDownHeight = 230;
			this->comboBox_choise_chart1->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->comboBox_choise_chart1->FormattingEnabled = true;
			this->comboBox_choise_chart1->IntegralHeight = false;
			this->comboBox_choise_chart1->Items->AddRange(gcnew cli::array< System::Object^  >(1) { L"PH Z2-Z1" });
			this->comboBox_choise_chart1->Location = System::Drawing::Point(92, 0);
			this->comboBox_choise_chart1->Margin = System::Windows::Forms::Padding(6, 0, 6, 0);
			this->comboBox_choise_chart1->Name = L"comboBox_choise_chart1";
			this->comboBox_choise_chart1->Size = System::Drawing::Size(249, 26);
			this->comboBox_choise_chart1->TabIndex = 22;
			this->comboBox_choise_chart1->SelectedIndexChanged += gcnew System::EventHandler(this, &MyForm::comboBox1_SelectedIndexChanged);
			// 
			// button5
			// 
			this->button5->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 10.2F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->button5->Location = System::Drawing::Point(696, 0);
			this->button5->Margin = System::Windows::Forms::Padding(6, 0, 6, 0);
			this->button5->Name = L"button5";
			this->button5->Size = System::Drawing::Size(50, 30);
			this->button5->TabIndex = 30;
			this->button5->Text = L">>";
			this->button5->UseVisualStyleBackColor = true;
			this->button5->Click += gcnew System::EventHandler(this, &MyForm::do_);
			// 
			// comboBox_choise_chart2
			// 
			this->comboBox_choise_chart2->DropDownHeight = 230;
			this->comboBox_choise_chart2->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->comboBox_choise_chart2->FormattingEnabled = true;
			this->comboBox_choise_chart2->IntegralHeight = false;
			this->comboBox_choise_chart2->Items->AddRange(gcnew cli::array< System::Object^  >(1) { L"PH Z2-Z1" });
			this->comboBox_choise_chart2->Location = System::Drawing::Point(92, 0);
			this->comboBox_choise_chart2->Margin = System::Windows::Forms::Padding(6, 0, 6, 0);
			this->comboBox_choise_chart2->Name = L"comboBox_choise_chart2";
			this->comboBox_choise_chart2->Size = System::Drawing::Size(249, 26);
			this->comboBox_choise_chart2->TabIndex = 32;
			this->comboBox_choise_chart2->SelectedIndexChanged += gcnew System::EventHandler(this, &MyForm::comboBox2_SelectedIndexChanged);
			// 
			// textBox_min2
			// 
			this->textBox_min2->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9.75F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->textBox_min2->Location = System::Drawing::Point(524, 0);
			this->textBox_min2->Margin = System::Windows::Forms::Padding(6, 0, 6, 0);
			this->textBox_min2->Name = L"textBox_min2";
			this->textBox_min2->Size = System::Drawing::Size(74, 26);
			this->textBox_min2->TabIndex = 34;
			this->textBox_min2->TextChanged += gcnew System::EventHandler(this, &MyForm::min_n2_change);
			this->textBox_min2->KeyPress += gcnew System::Windows::Forms::KeyPressEventHandler(this, &MyForm::digit2);
			// 
			// textBox_max2
			// 
			this->textBox_max2->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9.75F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->textBox_max2->Location = System::Drawing::Point(610, 0);
			this->textBox_max2->Margin = System::Windows::Forms::Padding(6, 0, 6, 0);
			this->textBox_max2->Name = L"textBox_max2";
			this->textBox_max2->Size = System::Drawing::Size(74, 26);
			this->textBox_max2->TabIndex = 35;
			this->textBox_max2->TextChanged += gcnew System::EventHandler(this, &MyForm::max_n2_change);
			this->textBox_max2->KeyPress += gcnew System::Windows::Forms::KeyPressEventHandler(this, &MyForm::digit2);
			// 
			// button2
			// 
			this->button2->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 10.2F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->button2->Location = System::Drawing::Point(696, 0);
			this->button2->Margin = System::Windows::Forms::Padding(6, 0, 6, 0);
			this->button2->Name = L"button2";
			this->button2->Size = System::Drawing::Size(50, 30);
			this->button2->TabIndex = 36;
			this->button2->Text = L">>";
			this->button2->UseVisualStyleBackColor = true;
			this->button2->Click += gcnew System::EventHandler(this, &MyForm::do_2);
			// 
			// timer_read_file
			// 
			this->timer_read_file->Interval = 2000;
			this->timer_read_file->Tick += gcnew System::EventHandler(this, &MyForm::timer_read_file_Tick);
			// 
			// checkBox1
			// 
			this->checkBox1->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9.75F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->checkBox1->Location = System::Drawing::Point(1065, 0);
			this->checkBox1->Margin = System::Windows::Forms::Padding(6, 0, 0, 0);
			this->checkBox1->Name = L"checkBox1";
			this->checkBox1->Size = System::Drawing::Size(72, 24);
			this->checkBox1->TabIndex = 55;
			this->checkBox1->Text = L"f(T)";
			this->checkBox1->UseVisualStyleBackColor = true;
			// 
			// checkBox2
			// 
			this->checkBox2->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9.75F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->checkBox2->Location = System::Drawing::Point(1065, 0);
			this->checkBox2->Margin = System::Windows::Forms::Padding(6, 0, 0, 0);
			this->checkBox2->Name = L"checkBox2";
			this->checkBox2->Size = System::Drawing::Size(72, 22);
			this->checkBox2->TabIndex = 56;
			this->checkBox2->Text = L"f(T)";
			this->checkBox2->UseVisualStyleBackColor = true;
			// 
			// contextMenuStrip2
			// 
			this->contextMenuStrip2->ImageScalingSize = System::Drawing::Size(20, 20);
			this->contextMenuStrip2->Items->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(3) {
				this->N_Raw_Data_point_2,
					this->toolStripMenuItemmin2, this->toolStripMenuItemmax2
			});
			this->contextMenuStrip2->Name = L"contextMenuStrip2";
			this->contextMenuStrip2->Size = System::Drawing::Size(186, 76);
			// 
			// N_Raw_Data_point_2
			// 
			this->N_Raw_Data_point_2->Name = L"N_Raw_Data_point_2";
			this->N_Raw_Data_point_2->Size = System::Drawing::Size(185, 24);
			this->N_Raw_Data_point_2->Text = L"Raw_Data_point";
			this->N_Raw_Data_point_2->Click += gcnew System::EventHandler(this, &MyForm::N_Raw_Data_point_2_Click);
			// 
			// toolStripMenuItemmin2
			// 
			this->toolStripMenuItemmin2->Name = L"toolStripMenuItemmin2";
			this->toolStripMenuItemmin2->Size = System::Drawing::Size(185, 24);
			this->toolStripMenuItemmin2->Text = L"min";
			this->toolStripMenuItemmin2->Click += gcnew System::EventHandler(this, &MyForm::toolStripMenuItemmin2_Click);
			// 
			// toolStripMenuItemmax2
			// 
			this->toolStripMenuItemmax2->Name = L"toolStripMenuItemmax2";
			this->toolStripMenuItemmax2->Size = System::Drawing::Size(185, 24);
			this->toolStripMenuItemmax2->Text = L"max";
			this->toolStripMenuItemmax2->Click += gcnew System::EventHandler(this, &MyForm::toolStripMenuItemmax2_Click);
			// 
			// tableLayoutPanel1
			// 
			this->tableLayoutPanel1->CellBorderStyle = System::Windows::Forms::TableLayoutPanelCellBorderStyle::Single;
			this->tableLayoutPanel1->ColumnCount = 2;
			this->tableLayoutPanel1->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Percent,
				100)));
			this->tableLayoutPanel1->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Absolute,
				503)));
			this->tableLayoutPanel1->Controls->Add(this->panel_Sonde_Picture, 0, 5);
			this->tableLayoutPanel1->Controls->Add(this->tableLayoutPanel5, 1, 1);
			this->tableLayoutPanel1->Controls->Add(this->tableLayoutPanel4, 0, 3);
			this->tableLayoutPanel1->Controls->Add(this->progressBar1, 1, 0);
			this->tableLayoutPanel1->Controls->Add(this->tableLayoutPanel6, 1, 3);
			this->tableLayoutPanel1->Controls->Add(this->chart3, 1, 2);
			this->tableLayoutPanel1->Controls->Add(this->chart1, 0, 2);
			this->tableLayoutPanel1->Controls->Add(this->chart4, 1, 4);
			this->tableLayoutPanel1->Controls->Add(this->chart2, 0, 4);
			this->tableLayoutPanel1->Controls->Add(this->tableLayoutPanel3, 0, 1);
			this->tableLayoutPanel1->Controls->Add(this->panel1, 0, 6);
			this->tableLayoutPanel1->Controls->Add(this->tableLayoutPanel10, 0, 0);
			this->tableLayoutPanel1->Dock = System::Windows::Forms::DockStyle::Fill;
			this->tableLayoutPanel1->Location = System::Drawing::Point(0, 0);
			this->tableLayoutPanel1->Margin = System::Windows::Forms::Padding(10);
			this->tableLayoutPanel1->Name = L"tableLayoutPanel1";
			this->tableLayoutPanel1->RowCount = 7;
			this->tableLayoutPanel1->RowStyles->Add((gcnew System::Windows::Forms::RowStyle()));
			this->tableLayoutPanel1->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Absolute, 40)));
			this->tableLayoutPanel1->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Percent, 50)));
			this->tableLayoutPanel1->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Absolute, 39)));
			this->tableLayoutPanel1->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Percent, 50)));
			this->tableLayoutPanel1->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Absolute, 121)));
			this->tableLayoutPanel1->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Absolute, 60)));
			this->tableLayoutPanel1->Size = System::Drawing::Size(1902, 1032);
			this->tableLayoutPanel1->TabIndex = 85;
			// 
			// panel_Sonde_Picture
			// 
			this->panel_Sonde_Picture->Anchor = System::Windows::Forms::AnchorStyles::Top;
			this->panel_Sonde_Picture->Controls->Add(this->pictureBoxD13);
			this->panel_Sonde_Picture->Controls->Add(this->pictureBoxD12);
			this->panel_Sonde_Picture->Controls->Add(this->pictureBoxD10);
			this->panel_Sonde_Picture->Controls->Add(this->pictureBoxD11);
			this->panel_Sonde_Picture->Controls->Add(this->pictureBoxD9);
			this->panel_Sonde_Picture->Controls->Add(this->labelR3);
			this->panel_Sonde_Picture->Controls->Add(this->labelR4);
			this->panel_Sonde_Picture->Controls->Add(this->labelT4);
			this->panel_Sonde_Picture->Controls->Add(this->labelT3);
			this->panel_Sonde_Picture->Controls->Add(this->labelT5);
			this->panel_Sonde_Picture->Controls->Add(this->labelR2);
			this->panel_Sonde_Picture->Controls->Add(this->labelR1);
			this->panel_Sonde_Picture->Controls->Add(this->labelT2);
			this->panel_Sonde_Picture->Controls->Add(this->labelT1);
			this->panel_Sonde_Picture->Controls->Add(this->pictureBoxD5);
			this->panel_Sonde_Picture->Controls->Add(this->pictureBoxD6);
			this->panel_Sonde_Picture->Controls->Add(this->pictureBoxT5);
			this->panel_Sonde_Picture->Controls->Add(this->pictureBoxD7);
			this->panel_Sonde_Picture->Controls->Add(this->pictureBoxD8);
			this->panel_Sonde_Picture->Controls->Add(this->pictureBoxD3);
			this->panel_Sonde_Picture->Controls->Add(this->pictureBoxD4);
			this->panel_Sonde_Picture->Controls->Add(this->pictureBoxD1);
			this->panel_Sonde_Picture->Controls->Add(this->pictureBoxD2);
			this->panel_Sonde_Picture->Controls->Add(this->pictureBoxT2);
			this->panel_Sonde_Picture->Controls->Add(this->pictureBoxR4);
			this->panel_Sonde_Picture->Controls->Add(this->pictureBoxT4);
			this->panel_Sonde_Picture->Controls->Add(this->pictureBoxR2);
			this->panel_Sonde_Picture->Controls->Add(this->pictureBoxR1);
			this->panel_Sonde_Picture->Controls->Add(this->pictureBoxT1);
			this->panel_Sonde_Picture->Controls->Add(this->pictureBoxT3);
			this->panel_Sonde_Picture->Controls->Add(this->pictureBoxR3);
			this->panel_Sonde_Picture->Location = System::Drawing::Point(149, 849);
			this->panel_Sonde_Picture->Margin = System::Windows::Forms::Padding(1);
			this->panel_Sonde_Picture->Name = L"panel_Sonde_Picture";
			this->panel_Sonde_Picture->Size = System::Drawing::Size(1100, 119);
			this->panel_Sonde_Picture->TabIndex = 109;
			// 
			// pictureBoxD13
			// 
			this->pictureBoxD13->BackColor = System::Drawing::Color::Magenta;
			this->pictureBoxD13->Location = System::Drawing::Point(15, 96);
			this->pictureBoxD13->Margin = System::Windows::Forms::Padding(0);
			this->pictureBoxD13->Name = L"pictureBoxD13";
			this->pictureBoxD13->Size = System::Drawing::Size(463, 6);
			this->pictureBoxD13->SizeMode = System::Windows::Forms::PictureBoxSizeMode::CenterImage;
			this->pictureBoxD13->TabIndex = 51;
			this->pictureBoxD13->TabStop = false;
			// 
			// pictureBoxD12
			// 
			this->pictureBoxD12->BackColor = System::Drawing::Color::RoyalBlue;
			this->pictureBoxD12->Location = System::Drawing::Point(479, 85);
			this->pictureBoxD12->Margin = System::Windows::Forms::Padding(0);
			this->pictureBoxD12->Name = L"pictureBoxD12";
			this->pictureBoxD12->Size = System::Drawing::Size(529, 6);
			this->pictureBoxD12->SizeMode = System::Windows::Forms::PictureBoxSizeMode::CenterImage;
			this->pictureBoxD12->TabIndex = 50;
			this->pictureBoxD12->TabStop = false;
			// 
			// pictureBoxD10
			// 
			this->pictureBoxD10->BackColor = System::Drawing::Color::DarkGoldenrod;
			this->pictureBoxD10->Location = System::Drawing::Point(479, 76);
			this->pictureBoxD10->Margin = System::Windows::Forms::Padding(0);
			this->pictureBoxD10->Name = L"pictureBoxD10";
			this->pictureBoxD10->Size = System::Drawing::Size(269, 6);
			this->pictureBoxD10->SizeMode = System::Windows::Forms::PictureBoxSizeMode::CenterImage;
			this->pictureBoxD10->TabIndex = 49;
			this->pictureBoxD10->TabStop = false;
			// 
			// pictureBoxD11
			// 
			this->pictureBoxD11->BackColor = System::Drawing::Color::Red;
			this->pictureBoxD11->Location = System::Drawing::Point(79, 85);
			this->pictureBoxD11->Margin = System::Windows::Forms::Padding(0);
			this->pictureBoxD11->Name = L"pictureBoxD11";
			this->pictureBoxD11->Size = System::Drawing::Size(399, 6);
			this->pictureBoxD11->SizeMode = System::Windows::Forms::PictureBoxSizeMode::CenterImage;
			this->pictureBoxD11->TabIndex = 48;
			this->pictureBoxD11->TabStop = false;
			// 
			// pictureBoxD9
			// 
			this->pictureBoxD9->BackColor = System::Drawing::Color::Green;
			this->pictureBoxD9->Location = System::Drawing::Point(340, 76);
			this->pictureBoxD9->Margin = System::Windows::Forms::Padding(0);
			this->pictureBoxD9->Name = L"pictureBoxD9";
			this->pictureBoxD9->Size = System::Drawing::Size(139, 6);
			this->pictureBoxD9->SizeMode = System::Windows::Forms::PictureBoxSizeMode::CenterImage;
			this->pictureBoxD9->TabIndex = 47;
			this->pictureBoxD9->TabStop = false;
			// 
			// labelR3
			// 
			this->labelR3->AutoSize = true;
			this->labelR3->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->labelR3->Location = System::Drawing::Point(5, 41);
			this->labelR3->Margin = System::Windows::Forms::Padding(5, 0, 5, 0);
			this->labelR3->Name = L"labelR3";
			this->labelR3->Size = System::Drawing::Size(29, 18);
			this->labelR3->TabIndex = 41;
			this->labelR3->Text = L"R3";
			// 
			// labelR4
			// 
			this->labelR4->AutoSize = true;
			this->labelR4->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->labelR4->Location = System::Drawing::Point(1052, 41);
			this->labelR4->Margin = System::Windows::Forms::Padding(5, 0, 5, 0);
			this->labelR4->Name = L"labelR4";
			this->labelR4->Size = System::Drawing::Size(29, 18);
			this->labelR4->TabIndex = 40;
			this->labelR4->Text = L"R4";
			// 
			// labelT4
			// 
			this->labelT4->AutoSize = true;
			this->labelT4->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->labelT4->Location = System::Drawing::Point(998, 41);
			this->labelT4->Margin = System::Windows::Forms::Padding(5, 0, 5, 0);
			this->labelT4->Name = L"labelT4";
			this->labelT4->Size = System::Drawing::Size(27, 18);
			this->labelT4->TabIndex = 39;
			this->labelT4->Text = L"T4";
			// 
			// labelT3
			// 
			this->labelT3->AutoSize = true;
			this->labelT3->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->labelT3->Location = System::Drawing::Point(61, 41);
			this->labelT3->Margin = System::Windows::Forms::Padding(5, 0, 5, 0);
			this->labelT3->Name = L"labelT3";
			this->labelT3->Size = System::Drawing::Size(27, 18);
			this->labelT3->TabIndex = 38;
			this->labelT3->Text = L"T3";
			// 
			// labelT5
			// 
			this->labelT5->AutoSize = true;
			this->labelT5->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->labelT5->Location = System::Drawing::Point(533, 41);
			this->labelT5->Margin = System::Windows::Forms::Padding(5, 0, 5, 0);
			this->labelT5->Name = L"labelT5";
			this->labelT5->Size = System::Drawing::Size(27, 18);
			this->labelT5->TabIndex = 37;
			this->labelT5->Text = L"T5";
			// 
			// labelR2
			// 
			this->labelR2->AutoSize = true;
			this->labelR2->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->labelR2->Location = System::Drawing::Point(491, 41);
			this->labelR2->Margin = System::Windows::Forms::Padding(5, 0, 5, 0);
			this->labelR2->Name = L"labelR2";
			this->labelR2->Size = System::Drawing::Size(29, 18);
			this->labelR2->TabIndex = 36;
			this->labelR2->Text = L"R2";
			// 
			// labelR1
			// 
			this->labelR1->AutoSize = true;
			this->labelR1->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->labelR1->Location = System::Drawing::Point(435, 41);
			this->labelR1->Margin = System::Windows::Forms::Padding(5, 0, 5, 0);
			this->labelR1->Name = L"labelR1";
			this->labelR1->Size = System::Drawing::Size(29, 18);
			this->labelR1->TabIndex = 35;
			this->labelR1->Text = L"R1";
			// 
			// labelT2
			// 
			this->labelT2->AutoSize = true;
			this->labelT2->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->labelT2->Location = System::Drawing::Point(732, 41);
			this->labelT2->Margin = System::Windows::Forms::Padding(5, 0, 5, 0);
			this->labelT2->Name = L"labelT2";
			this->labelT2->Size = System::Drawing::Size(27, 18);
			this->labelT2->TabIndex = 34;
			this->labelT2->Text = L"T2";
			// 
			// labelT1
			// 
			this->labelT1->AutoSize = true;
			this->labelT1->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->labelT1->Location = System::Drawing::Point(329, 41);
			this->labelT1->Margin = System::Windows::Forms::Padding(5, 0, 5, 0);
			this->labelT1->Name = L"labelT1";
			this->labelT1->Size = System::Drawing::Size(27, 18);
			this->labelT1->TabIndex = 33;
			this->labelT1->Text = L"T1";
			// 
			// pictureBoxD5
			// 
			this->pictureBoxD5->BackColor = System::Drawing::Color::RoyalBlue;
			this->pictureBoxD5->ContextMenuStrip = this->contextMenuStrip2;
			this->pictureBoxD5->Location = System::Drawing::Point(16, 25);
			this->pictureBoxD5->Margin = System::Windows::Forms::Padding(0);
			this->pictureBoxD5->Name = L"pictureBoxD5";
			this->pictureBoxD5->Size = System::Drawing::Size(530, 6);
			this->pictureBoxD5->SizeMode = System::Windows::Forms::PictureBoxSizeMode::CenterImage;
			this->pictureBoxD5->TabIndex = 32;
			this->pictureBoxD5->TabStop = false;
			// 
			// pictureBoxD6
			// 
			this->pictureBoxD6->BackColor = System::Drawing::Color::RoyalBlue;
			this->pictureBoxD6->ContextMenuStrip = this->contextMenuStrip2;
			this->pictureBoxD6->Location = System::Drawing::Point(550, 25);
			this->pictureBoxD6->Margin = System::Windows::Forms::Padding(0);
			this->pictureBoxD6->Name = L"pictureBoxD6";
			this->pictureBoxD6->Size = System::Drawing::Size(521, 6);
			this->pictureBoxD6->SizeMode = System::Windows::Forms::PictureBoxSizeMode::CenterImage;
			this->pictureBoxD6->TabIndex = 31;
			this->pictureBoxD6->TabStop = false;
			// 
			// pictureBoxT5
			// 
			this->pictureBoxT5->BackColor = System::Drawing::Color::Black;
			this->pictureBoxT5->Location = System::Drawing::Point(545, 34);
			this->pictureBoxT5->Margin = System::Windows::Forms::Padding(0);
			this->pictureBoxT5->Name = L"pictureBoxT5";
			this->pictureBoxT5->Size = System::Drawing::Size(6, 38);
			this->pictureBoxT5->SizeMode = System::Windows::Forms::PictureBoxSizeMode::CenterImage;
			this->pictureBoxT5->TabIndex = 30;
			this->pictureBoxT5->TabStop = false;
			// 
			// pictureBoxD7
			// 
			this->pictureBoxD7->BackColor = System::Drawing::Color::Green;
			this->pictureBoxD7->ContextMenuStrip = this->contextMenuStrip2;
			this->pictureBoxD7->Location = System::Drawing::Point(15, 85);
			this->pictureBoxD7->Margin = System::Windows::Forms::Padding(0);
			this->pictureBoxD7->Name = L"pictureBoxD7";
			this->pictureBoxD7->Size = System::Drawing::Size(330, 6);
			this->pictureBoxD7->SizeMode = System::Windows::Forms::PictureBoxSizeMode::CenterImage;
			this->pictureBoxD7->TabIndex = 29;
			this->pictureBoxD7->TabStop = false;
			// 
			// pictureBoxD8
			// 
			this->pictureBoxD8->BackColor = System::Drawing::Color::Green;
			this->pictureBoxD8->ContextMenuStrip = this->contextMenuStrip2;
			this->pictureBoxD8->Location = System::Drawing::Point(742, 85);
			this->pictureBoxD8->Margin = System::Windows::Forms::Padding(0);
			this->pictureBoxD8->Name = L"pictureBoxD8";
			this->pictureBoxD8->Size = System::Drawing::Size(334, 6);
			this->pictureBoxD8->SizeMode = System::Windows::Forms::PictureBoxSizeMode::CenterImage;
			this->pictureBoxD8->TabIndex = 28;
			this->pictureBoxD8->TabStop = false;
			// 
			// pictureBoxD3
			// 
			this->pictureBoxD3->BackColor = System::Drawing::Color::DarkGoldenrod;
			this->pictureBoxD3->ContextMenuStrip = this->contextMenuStrip2;
			this->pictureBoxD3->Location = System::Drawing::Point(15, 96);
			this->pictureBoxD3->Margin = System::Windows::Forms::Padding(0);
			this->pictureBoxD3->Name = L"pictureBoxD3";
			this->pictureBoxD3->Size = System::Drawing::Size(734, 6);
			this->pictureBoxD3->SizeMode = System::Windows::Forms::PictureBoxSizeMode::CenterImage;
			this->pictureBoxD3->TabIndex = 27;
			this->pictureBoxD3->TabStop = false;
			// 
			// pictureBoxD4
			// 
			this->pictureBoxD4->BackColor = System::Drawing::Color::DarkGoldenrod;
			this->pictureBoxD4->ContextMenuStrip = this->contextMenuStrip2;
			this->pictureBoxD4->Location = System::Drawing::Point(336, 108);
			this->pictureBoxD4->Margin = System::Windows::Forms::Padding(0);
			this->pictureBoxD4->Name = L"pictureBoxD4";
			this->pictureBoxD4->Size = System::Drawing::Size(734, 6);
			this->pictureBoxD4->SizeMode = System::Windows::Forms::PictureBoxSizeMode::CenterImage;
			this->pictureBoxD4->TabIndex = 26;
			this->pictureBoxD4->TabStop = false;
			// 
			// pictureBoxD1
			// 
			this->pictureBoxD1->BackColor = System::Drawing::Color::Red;
			this->pictureBoxD1->ContextMenuStrip = this->contextMenuStrip2;
			this->pictureBoxD1->Location = System::Drawing::Point(20, 4);
			this->pictureBoxD1->Margin = System::Windows::Forms::Padding(0);
			this->pictureBoxD1->Name = L"pictureBoxD1";
			this->pictureBoxD1->Size = System::Drawing::Size(991, 6);
			this->pictureBoxD1->SizeMode = System::Windows::Forms::PictureBoxSizeMode::CenterImage;
			this->pictureBoxD1->TabIndex = 25;
			this->pictureBoxD1->TabStop = false;
			// 
			// pictureBoxD2
			// 
			this->pictureBoxD2->BackColor = System::Drawing::Color::Red;
			this->pictureBoxD2->ContextMenuStrip = this->contextMenuStrip2;
			this->pictureBoxD2->Location = System::Drawing::Point(79, 15);
			this->pictureBoxD2->Margin = System::Windows::Forms::Padding(0);
			this->pictureBoxD2->Name = L"pictureBoxD2";
			this->pictureBoxD2->Size = System::Drawing::Size(991, 6);
			this->pictureBoxD2->SizeMode = System::Windows::Forms::PictureBoxSizeMode::CenterImage;
			this->pictureBoxD2->TabIndex = 24;
			this->pictureBoxD2->TabStop = false;
			// 
			// pictureBoxT2
			// 
			this->pictureBoxT2->BackColor = System::Drawing::Color::Black;
			this->pictureBoxT2->Location = System::Drawing::Point(742, 34);
			this->pictureBoxT2->Margin = System::Windows::Forms::Padding(0);
			this->pictureBoxT2->Name = L"pictureBoxT2";
			this->pictureBoxT2->Size = System::Drawing::Size(6, 38);
			this->pictureBoxT2->SizeMode = System::Windows::Forms::PictureBoxSizeMode::CenterImage;
			this->pictureBoxT2->TabIndex = 23;
			this->pictureBoxT2->TabStop = false;
			// 
			// pictureBoxR4
			// 
			this->pictureBoxR4->BackColor = System::Drawing::Color::Black;
			this->pictureBoxR4->Location = System::Drawing::Point(1065, 34);
			this->pictureBoxR4->Margin = System::Windows::Forms::Padding(0);
			this->pictureBoxR4->Name = L"pictureBoxR4";
			this->pictureBoxR4->Size = System::Drawing::Size(6, 38);
			this->pictureBoxR4->SizeMode = System::Windows::Forms::PictureBoxSizeMode::CenterImage;
			this->pictureBoxR4->TabIndex = 22;
			this->pictureBoxR4->TabStop = false;
			// 
			// pictureBoxT4
			// 
			this->pictureBoxT4->BackColor = System::Drawing::Color::Black;
			this->pictureBoxT4->Location = System::Drawing::Point(1009, 34);
			this->pictureBoxT4->Margin = System::Windows::Forms::Padding(0);
			this->pictureBoxT4->Name = L"pictureBoxT4";
			this->pictureBoxT4->Size = System::Drawing::Size(6, 38);
			this->pictureBoxT4->SizeMode = System::Windows::Forms::PictureBoxSizeMode::CenterImage;
			this->pictureBoxT4->TabIndex = 21;
			this->pictureBoxT4->TabStop = false;
			// 
			// pictureBoxR2
			// 
			this->pictureBoxR2->BackColor = System::Drawing::Color::Black;
			this->pictureBoxR2->Location = System::Drawing::Point(504, 34);
			this->pictureBoxR2->Margin = System::Windows::Forms::Padding(0);
			this->pictureBoxR2->Name = L"pictureBoxR2";
			this->pictureBoxR2->Size = System::Drawing::Size(6, 38);
			this->pictureBoxR2->SizeMode = System::Windows::Forms::PictureBoxSizeMode::CenterImage;
			this->pictureBoxR2->TabIndex = 20;
			this->pictureBoxR2->TabStop = false;
			// 
			// pictureBoxR1
			// 
			this->pictureBoxR1->BackColor = System::Drawing::Color::Black;
			this->pictureBoxR1->Location = System::Drawing::Point(444, 34);
			this->pictureBoxR1->Margin = System::Windows::Forms::Padding(0);
			this->pictureBoxR1->Name = L"pictureBoxR1";
			this->pictureBoxR1->Size = System::Drawing::Size(6, 38);
			this->pictureBoxR1->SizeMode = System::Windows::Forms::PictureBoxSizeMode::CenterImage;
			this->pictureBoxR1->TabIndex = 19;
			this->pictureBoxR1->TabStop = false;
			// 
			// pictureBoxT1
			// 
			this->pictureBoxT1->BackColor = System::Drawing::Color::Black;
			this->pictureBoxT1->Location = System::Drawing::Point(339, 34);
			this->pictureBoxT1->Margin = System::Windows::Forms::Padding(0);
			this->pictureBoxT1->Name = L"pictureBoxT1";
			this->pictureBoxT1->Size = System::Drawing::Size(6, 38);
			this->pictureBoxT1->SizeMode = System::Windows::Forms::PictureBoxSizeMode::CenterImage;
			this->pictureBoxT1->TabIndex = 18;
			this->pictureBoxT1->TabStop = false;
			// 
			// pictureBoxT3
			// 
			this->pictureBoxT3->BackColor = System::Drawing::Color::Black;
			this->pictureBoxT3->Location = System::Drawing::Point(75, 34);
			this->pictureBoxT3->Margin = System::Windows::Forms::Padding(0);
			this->pictureBoxT3->Name = L"pictureBoxT3";
			this->pictureBoxT3->Size = System::Drawing::Size(6, 38);
			this->pictureBoxT3->SizeMode = System::Windows::Forms::PictureBoxSizeMode::CenterImage;
			this->pictureBoxT3->TabIndex = 17;
			this->pictureBoxT3->TabStop = false;
			// 
			// pictureBoxR3
			// 
			this->pictureBoxR3->BackColor = System::Drawing::Color::Black;
			this->pictureBoxR3->Location = System::Drawing::Point(15, 34);
			this->pictureBoxR3->Margin = System::Windows::Forms::Padding(0);
			this->pictureBoxR3->Name = L"pictureBoxR3";
			this->pictureBoxR3->Size = System::Drawing::Size(6, 38);
			this->pictureBoxR3->SizeMode = System::Windows::Forms::PictureBoxSizeMode::CenterImage;
			this->pictureBoxR3->TabIndex = 0;
			this->pictureBoxR3->TabStop = false;
			// 
			// tableLayoutPanel5
			// 
			this->tableLayoutPanel5->ColumnCount = 9;
			this->tableLayoutPanel5->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle()));
			this->tableLayoutPanel5->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle()));
			this->tableLayoutPanel5->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle()));
			this->tableLayoutPanel5->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle()));
			this->tableLayoutPanel5->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle()));
			this->tableLayoutPanel5->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle()));
			this->tableLayoutPanel5->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle()));
			this->tableLayoutPanel5->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Absolute,
				112)));
			this->tableLayoutPanel5->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Absolute,
				45)));
			this->tableLayoutPanel5->Controls->Add(this->checkBoxT13, 5, 0);
			this->tableLayoutPanel5->Controls->Add(this->checkBoxT12, 4, 0);
			this->tableLayoutPanel5->Controls->Add(this->checkBoxT11, 3, 0);
			this->tableLayoutPanel5->Controls->Add(this->button_back_1, 0, 0);
			this->tableLayoutPanel5->Controls->Add(this->button_forward_1, 2, 0);
			this->tableLayoutPanel5->Controls->Add(this->checkBoxT14, 6, 0);
			this->tableLayoutPanel5->Controls->Add(this->label_dv1, 7, 0);
			this->tableLayoutPanel5->Controls->Add(this->button_re_im_1, 1, 0);
			this->tableLayoutPanel5->Location = System::Drawing::Point(1400, 39);
			this->tableLayoutPanel5->Margin = System::Windows::Forms::Padding(2);
			this->tableLayoutPanel5->Name = L"tableLayoutPanel5";
			this->tableLayoutPanel5->RowCount = 1;
			this->tableLayoutPanel5->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Percent, 100)));
			this->tableLayoutPanel5->Size = System::Drawing::Size(458, 26);
			this->tableLayoutPanel5->TabIndex = 101;
			// 
			// checkBoxT13
			// 
			this->checkBoxT13->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Bottom)
				| System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->checkBoxT13->AutoSize = true;
			this->checkBoxT13->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 7.8F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->checkBoxT13->Location = System::Drawing::Point(274, 0);
			this->checkBoxT13->Margin = System::Windows::Forms::Padding(0);
			this->checkBoxT13->Name = L"checkBoxT13";
			this->checkBoxT13->Size = System::Drawing::Size(49, 26);
			this->checkBoxT13->TabIndex = 95;
			this->checkBoxT13->Text = L"T3";
			this->checkBoxT13->UseVisualStyleBackColor = true;
			// 
			// checkBoxT12
			// 
			this->checkBoxT12->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Bottom)
				| System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->checkBoxT12->AutoSize = true;
			this->checkBoxT12->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 7.8F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->checkBoxT12->Location = System::Drawing::Point(225, 0);
			this->checkBoxT12->Margin = System::Windows::Forms::Padding(0);
			this->checkBoxT12->Name = L"checkBoxT12";
			this->checkBoxT12->Size = System::Drawing::Size(49, 26);
			this->checkBoxT12->TabIndex = 94;
			this->checkBoxT12->Text = L"T2";
			this->checkBoxT12->UseVisualStyleBackColor = true;
			// 
			// checkBoxT11
			// 
			this->checkBoxT11->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Bottom)
				| System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->checkBoxT11->AutoSize = true;
			this->checkBoxT11->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 7.8F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->checkBoxT11->Location = System::Drawing::Point(176, 0);
			this->checkBoxT11->Margin = System::Windows::Forms::Padding(10, 0, 0, 0);
			this->checkBoxT11->Name = L"checkBoxT11";
			this->checkBoxT11->Size = System::Drawing::Size(49, 26);
			this->checkBoxT11->TabIndex = 93;
			this->checkBoxT11->Text = L"T1";
			this->checkBoxT11->UseVisualStyleBackColor = true;
			// 
			// button_back_1
			// 
			this->button_back_1->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 10.2F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->button_back_1->Location = System::Drawing::Point(2, 0);
			this->button_back_1->Margin = System::Windows::Forms::Padding(2, 0, 0, 0);
			this->button_back_1->Name = L"button_back_1";
			this->button_back_1->Size = System::Drawing::Size(44, 26);
			this->button_back_1->TabIndex = 91;
			this->button_back_1->Text = L"<<";
			this->button_back_1->UseVisualStyleBackColor = true;
			this->button_back_1->Click += gcnew System::EventHandler(this, &MyForm::button_back_1_Click);
			// 
			// button_forward_1
			// 
			this->button_forward_1->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 10.2F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->button_forward_1->Location = System::Drawing::Point(122, 0);
			this->button_forward_1->Margin = System::Windows::Forms::Padding(2, 0, 0, 0);
			this->button_forward_1->Name = L"button_forward_1";
			this->button_forward_1->Size = System::Drawing::Size(44, 26);
			this->button_forward_1->TabIndex = 92;
			this->button_forward_1->Text = L">>";
			this->button_forward_1->UseVisualStyleBackColor = true;
			this->button_forward_1->Click += gcnew System::EventHandler(this, &MyForm::button_forward_1_Click);
			// 
			// checkBoxT14
			// 
			this->checkBoxT14->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Bottom)
				| System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->checkBoxT14->AutoSize = true;
			this->checkBoxT14->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 7.8F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->checkBoxT14->Location = System::Drawing::Point(323, 0);
			this->checkBoxT14->Margin = System::Windows::Forms::Padding(0);
			this->checkBoxT14->Name = L"checkBoxT14";
			this->checkBoxT14->Size = System::Drawing::Size(49, 26);
			this->checkBoxT14->TabIndex = 96;
			this->checkBoxT14->Text = L"T4";
			this->checkBoxT14->UseVisualStyleBackColor = true;
			// 
			// label_dv1
			// 
			this->label_dv1->AutoSize = true;
			this->label_dv1->Location = System::Drawing::Point(374, 0);
			this->label_dv1->Margin = System::Windows::Forms::Padding(2, 0, 2, 0);
			this->label_dv1->Name = L"label_dv1";
			this->label_dv1->Size = System::Drawing::Size(31, 17);
			this->label_dv1->TabIndex = 97;
			this->label_dv1->Text = L"dv1";
			// 
			// button_re_im_1
			// 
			this->button_re_im_1->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Bottom)
				| System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->button_re_im_1->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9.75F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->button_re_im_1->Location = System::Drawing::Point(48, 0);
			this->button_re_im_1->Margin = System::Windows::Forms::Padding(2, 0, 0, 0);
			this->button_re_im_1->Name = L"button_re_im_1";
			this->button_re_im_1->Size = System::Drawing::Size(72, 26);
			this->button_re_im_1->TabIndex = 89;
			this->button_re_im_1->Text = L"amp2";
			this->button_re_im_1->UseVisualStyleBackColor = true;
			this->button_re_im_1->Click += gcnew System::EventHandler(this, &MyForm::button_re_im_1_Click);
			// 
			// tableLayoutPanel4
			// 
			this->tableLayoutPanel4->ColumnCount = 10;
			this->tableLayoutPanel4->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle()));
			this->tableLayoutPanel4->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle()));
			this->tableLayoutPanel4->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle()));
			this->tableLayoutPanel4->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle()));
			this->tableLayoutPanel4->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle()));
			this->tableLayoutPanel4->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle()));
			this->tableLayoutPanel4->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle()));
			this->tableLayoutPanel4->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle()));
			this->tableLayoutPanel4->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle()));
			this->tableLayoutPanel4->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Percent,
				100)));
			this->tableLayoutPanel4->Controls->Add(this->button_direct2, 0, 0);
			this->tableLayoutPanel4->Controls->Add(this->comboBox_choise_chart2, 1, 0);
			this->tableLayoutPanel4->Controls->Add(this->button_ZX_2, 2, 0);
			this->tableLayoutPanel4->Controls->Add(this->button_400_2000_2, 3, 0);
			this->tableLayoutPanel4->Controls->Add(this->textBox_min2, 4, 0);
			this->tableLayoutPanel4->Controls->Add(this->textBox_max2, 5, 0);
			this->tableLayoutPanel4->Controls->Add(this->button2, 6, 0);
			this->tableLayoutPanel4->Controls->Add(this->textBox_cursor2, 7, 0);
			this->tableLayoutPanel4->Controls->Add(this->checkBox2, 9, 0);
			this->tableLayoutPanel4->Controls->Add(this->checkBox9, 8, 0);
			this->tableLayoutPanel4->Location = System::Drawing::Point(1, 445);
			this->tableLayoutPanel4->Margin = System::Windows::Forms::Padding(0, 2, 0, 0);
			this->tableLayoutPanel4->Name = L"tableLayoutPanel4";
			this->tableLayoutPanel4->RowCount = 1;
			this->tableLayoutPanel4->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Percent, 100)));
			this->tableLayoutPanel4->Size = System::Drawing::Size(1169, 35);
			this->tableLayoutPanel4->TabIndex = 90;
			// 
			// button_direct2
			// 
			this->button_direct2->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->button_direct2->Location = System::Drawing::Point(2, 0);
			this->button_direct2->Margin = System::Windows::Forms::Padding(2, 0, 2, 0);
			this->button_direct2->Name = L"button_direct2";
			this->button_direct2->Size = System::Drawing::Size(82, 30);
			this->button_direct2->TabIndex = 90;
			this->button_direct2->Text = L"UNDIR";
			this->button_direct2->UseVisualStyleBackColor = true;
			this->button_direct2->Click += gcnew System::EventHandler(this, &MyForm::button_direct2_Click);
			// 
			// button_ZX_2
			// 
			this->button_ZX_2->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9.75F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->button_ZX_2->Location = System::Drawing::Point(349, 0);
			this->button_ZX_2->Margin = System::Windows::Forms::Padding(2, 0, 2, 0);
			this->button_ZX_2->Name = L"button_ZX_2";
			this->button_ZX_2->Size = System::Drawing::Size(88, 30);
			this->button_ZX_2->TabIndex = 89;
			this->button_ZX_2->Text = L"R_Zx R";
			this->button_ZX_2->UseVisualStyleBackColor = true;
			this->button_ZX_2->Click += gcnew System::EventHandler(this, &MyForm::button_ZX_2_Click);
			// 
			// button_400_2000_2
			// 
			this->button_400_2000_2->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9.75F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->button_400_2000_2->Location = System::Drawing::Point(441, 0);
			this->button_400_2000_2->Margin = System::Windows::Forms::Padding(2, 0, 2, 0);
			this->button_400_2000_2->Name = L"button_400_2000_2";
			this->button_400_2000_2->Size = System::Drawing::Size(75, 30);
			this->button_400_2000_2->TabIndex = 88;
			this->button_400_2000_2->Text = L"2000";
			this->button_400_2000_2->UseVisualStyleBackColor = true;
			this->button_400_2000_2->Click += gcnew System::EventHandler(this, &MyForm::button_400_2000_2_Click);
			// 
			// checkBox9
			// 
			this->checkBox9->AutoSize = true;
			this->checkBox9->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 7.8F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->checkBox9->Location = System::Drawing::Point(991, 0);
			this->checkBox9->Margin = System::Windows::Forms::Padding(12, 0, 0, 0);
			this->checkBox9->Name = L"checkBox9";
			this->checkBox9->Size = System::Drawing::Size(68, 21);
			this->checkBox9->TabIndex = 84;
			this->checkBox9->Text = L"zoom";
			this->checkBox9->UseVisualStyleBackColor = true;
			// 
			// tableLayoutPanel10
			// 
			this->tableLayoutPanel10->CellBorderStyle = System::Windows::Forms::TableLayoutPanelCellBorderStyle::Single;
			this->tableLayoutPanel10->ColumnCount = 7;
			this->tableLayoutPanel10->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Percent,
				12.80576F)));
			this->tableLayoutPanel10->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Percent,
				87.19424F)));
			this->tableLayoutPanel10->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Absolute,
				116)));
			this->tableLayoutPanel10->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Absolute,
				114)));
			this->tableLayoutPanel10->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Absolute,
				118)));
			this->tableLayoutPanel10->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Absolute,
				102)));
			this->tableLayoutPanel10->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Absolute,
				192)));
			this->tableLayoutPanel10->Controls->Add(this->toolStrip2, 1, 0);
			this->tableLayoutPanel10->Controls->Add(this->checkBox_sonde_calk, 2, 0);
			this->tableLayoutPanel10->Controls->Add(this->menuStrip1, 0, 0);
			this->tableLayoutPanel10->Controls->Add(this->label_T, 6, 0);
			this->tableLayoutPanel10->Controls->Add(this->checkBox_add_file, 3, 0);
			this->tableLayoutPanel10->Controls->Add(this->checkBox_re_im, 5, 0);
			this->tableLayoutPanel10->Controls->Add(this->checkBox_stable_ph, 4, 0);
			this->tableLayoutPanel10->Location = System::Drawing::Point(1, 1);
			this->tableLayoutPanel10->Margin = System::Windows::Forms::Padding(0);
			this->tableLayoutPanel10->Name = L"tableLayoutPanel10";
			this->tableLayoutPanel10->RowCount = 1;
			this->tableLayoutPanel10->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Percent, 100)));
			this->tableLayoutPanel10->Size = System::Drawing::Size(1309, 35);
			this->tableLayoutPanel10->TabIndex = 98;
			// 
			// toolStrip2
			// 
			this->toolStrip2->ImageScalingSize = System::Drawing::Size(20, 20);
			this->toolStrip2->Items->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(12) {
				this->toolStripSeparator15,
					this->toolStripButton_on_off, this->toolStripSeparator39, this->buttonSURF, this->toolStripSeparator40, this->toolStripSeparator7,
					this->toolStripButton3, this->toolStripSeparator8, this->toolStripSeparator41, this->toolStripButton_metro_file, this->toolStripSeparator11,
					this->toolStripSeparator12
			});
			this->toolStrip2->Location = System::Drawing::Point(86, 1);
			this->toolStrip2->Name = L"toolStrip2";
			this->toolStrip2->Size = System::Drawing::Size(574, 30);
			this->toolStrip2->TabIndex = 100;
			this->toolStrip2->Text = L"toolStrip2";
			// 
			// toolStripSeparator15
			// 
			this->toolStripSeparator15->Name = L"toolStripSeparator15";
			this->toolStripSeparator15->Size = System::Drawing::Size(6, 30);
			// 
			// toolStripButton_on_off
			// 
			this->toolStripButton_on_off->DisplayStyle = System::Windows::Forms::ToolStripItemDisplayStyle::Text;
			this->toolStripButton_on_off->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9.75F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->toolStripButton_on_off->ImageTransparentColor = System::Drawing::Color::Magenta;
			this->toolStripButton_on_off->Name = L"toolStripButton_on_off";
			this->toolStripButton_on_off->Size = System::Drawing::Size(54, 27);
			this->toolStripButton_on_off->Text = L"ВКЛ ";
			this->toolStripButton_on_off->Click += gcnew System::EventHandler(this, &MyForm::toolStripButton_on_off_Click);
			// 
			// toolStripSeparator39
			// 
			this->toolStripSeparator39->Name = L"toolStripSeparator39";
			this->toolStripSeparator39->Size = System::Drawing::Size(6, 30);
			// 
			// buttonSURF
			// 
			this->buttonSURF->DisplayStyle = System::Windows::Forms::ToolStripItemDisplayStyle::Text;
			this->buttonSURF->Font = (gcnew System::Drawing::Font(L"Segoe UI", 10.2F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->buttonSURF->ImageTransparentColor = System::Drawing::Color::Magenta;
			this->buttonSURF->Name = L"buttonSURF";
			this->buttonSURF->Size = System::Drawing::Size(72, 27);
			this->buttonSURF->Text = L"ПОТОК";
			this->buttonSURF->Click += gcnew System::EventHandler(this, &MyForm::Surf);
			// 
			// toolStripSeparator40
			// 
			this->toolStripSeparator40->Name = L"toolStripSeparator40";
			this->toolStripSeparator40->Size = System::Drawing::Size(6, 30);
			// 
			// toolStripSeparator7
			// 
			this->toolStripSeparator7->Name = L"toolStripSeparator7";
			this->toolStripSeparator7->Size = System::Drawing::Size(6, 30);
			// 
			// toolStripButton3
			// 
			this->toolStripButton3->DisplayStyle = System::Windows::Forms::ToolStripItemDisplayStyle::Text;
			this->toolStripButton3->Font = (gcnew System::Drawing::Font(L"Segoe UI", 10.2F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->toolStripButton3->ImageTransparentColor = System::Drawing::Color::Magenta;
			this->toolStripButton3->Name = L"toolStripButton3";
			this->toolStripButton3->Size = System::Drawing::Size(109, 27);
			this->toolStripButton3->Text = L"ОБНОВИТЬ";
			this->toolStripButton3->Click += gcnew System::EventHandler(this, &MyForm::renew);
			// 
			// toolStripSeparator8
			// 
			this->toolStripSeparator8->Name = L"toolStripSeparator8";
			this->toolStripSeparator8->Size = System::Drawing::Size(6, 30);
			// 
			// toolStripSeparator41
			// 
			this->toolStripSeparator41->Name = L"toolStripSeparator41";
			this->toolStripSeparator41->Size = System::Drawing::Size(6, 30);
			// 
			// toolStripButton_metro_file
			// 
			this->toolStripButton_metro_file->DisplayStyle = System::Windows::Forms::ToolStripItemDisplayStyle::Text;
			this->toolStripButton_metro_file->Font = (gcnew System::Drawing::Font(L"Segoe UI", 10.2F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->toolStripButton_metro_file->ImageTransparentColor = System::Drawing::Color::Magenta;
			this->toolStripButton_metro_file->Name = L"toolStripButton_metro_file";
			this->toolStripButton_metro_file->Size = System::Drawing::Size(130, 27);
			this->toolStripButton_metro_file->Text = L"МЕТРОЛОГИЯ";
			this->toolStripButton_metro_file->Click += gcnew System::EventHandler(this, &MyForm::toolStripButton_metro_file_Click);
			// 
			// toolStripSeparator11
			// 
			this->toolStripSeparator11->Name = L"toolStripSeparator11";
			this->toolStripSeparator11->Size = System::Drawing::Size(6, 30);
			// 
			// toolStripSeparator12
			// 
			this->toolStripSeparator12->Name = L"toolStripSeparator12";
			this->toolStripSeparator12->Size = System::Drawing::Size(6, 30);
			// 
			// checkBox_sonde_calk
			// 
			this->checkBox_sonde_calk->AutoSize = true;
			this->checkBox_sonde_calk->Location = System::Drawing::Point(663, 3);
			this->checkBox_sonde_calk->Margin = System::Windows::Forms::Padding(2);
			this->checkBox_sonde_calk->Name = L"checkBox_sonde_calk";
			this->checkBox_sonde_calk->Size = System::Drawing::Size(102, 21);
			this->checkBox_sonde_calk->TabIndex = 87;
			this->checkBox_sonde_calk->Text = L"sonde_calk";
			this->checkBox_sonde_calk->UseVisualStyleBackColor = true;
			// 
			// menuStrip1
			// 
			this->menuStrip1->Dock = System::Windows::Forms::DockStyle::None;
			this->menuStrip1->ImageScalingSize = System::Drawing::Size(60, 30);
			this->menuStrip1->Items->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(1) { this->файлToolStripMenuItem });
			this->menuStrip1->Location = System::Drawing::Point(1, 1);
			this->menuStrip1->Name = L"menuStrip1";
			this->menuStrip1->Padding = System::Windows::Forms::Padding(5, 2, 0, 2);
			this->menuStrip1->Size = System::Drawing::Size(84, 28);
			this->menuStrip1->TabIndex = 89;
			this->menuStrip1->Text = L"menuStrip1";
			// 
			// файлToolStripMenuItem
			// 
			this->файлToolStripMenuItem->DropDownItems->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(7) {
				this->открытьToolStripMenuItem,
					this->ToolStripMenuItem_save_file, this->toolStripMenuItem_delete, this->ToolStripMenuItem_parametrs, this->toolStripSeparator1,
					this->сохрвнитьДляАлехToolStripMenuItem, this->toolStripComboBox_sonde_type
			});
			this->файлToolStripMenuItem->Margin = System::Windows::Forms::Padding(3, 0, 10, 0);
			this->файлToolStripMenuItem->Name = L"файлToolStripMenuItem";
			this->файлToolStripMenuItem->Size = System::Drawing::Size(68, 24);
			this->файлToolStripMenuItem->Text = L"МЕНЮ";
			// 
			// открытьToolStripMenuItem
			// 
			this->открытьToolStripMenuItem->DropDownItems->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(2) {
				this->ToolStripMenuItem_data_file,
					this->ToolStripMenuItem_temporary_file
			});
			this->открытьToolStripMenuItem->Name = L"открытьToolStripMenuItem";
			this->открытьToolStripMenuItem->Size = System::Drawing::Size(223, 26);
			this->открытьToolStripMenuItem->Text = L"открыть";
			// 
			// ToolStripMenuItem_data_file
			// 
			this->ToolStripMenuItem_data_file->Name = L"ToolStripMenuItem_data_file";
			this->ToolStripMenuItem_data_file->Size = System::Drawing::Size(205, 26);
			this->ToolStripMenuItem_data_file->Text = L"файл данных";
			this->ToolStripMenuItem_data_file->Click += gcnew System::EventHandler(this, &MyForm::data_Click);
			// 
			// ToolStripMenuItem_temporary_file
			// 
			this->ToolStripMenuItem_temporary_file->Name = L"ToolStripMenuItem_temporary_file";
			this->ToolStripMenuItem_temporary_file->Size = System::Drawing::Size(205, 26);
			this->ToolStripMenuItem_temporary_file->Text = L"временный файл";
			this->ToolStripMenuItem_temporary_file->Visible = false;
			this->ToolStripMenuItem_temporary_file->Click += gcnew System::EventHandler(this, &MyForm::temporary_file_Click);
			// 
			// ToolStripMenuItem_save_file
			// 
			this->ToolStripMenuItem_save_file->Name = L"ToolStripMenuItem_save_file";
			this->ToolStripMenuItem_save_file->Size = System::Drawing::Size(223, 26);
			this->ToolStripMenuItem_save_file->Text = L"Сохранить как";
			this->ToolStripMenuItem_save_file->Click += gcnew System::EventHandler(this, &MyForm::ToolStripMenuItem_save_file_Click);
			// 
			// toolStripMenuItem_delete
			// 
			this->toolStripMenuItem_delete->Name = L"toolStripMenuItem_delete";
			this->toolStripMenuItem_delete->Size = System::Drawing::Size(223, 26);
			this->toolStripMenuItem_delete->Text = L"удалить ";
			this->toolStripMenuItem_delete->Click += gcnew System::EventHandler(this, &MyForm::toolStripMenuItem_delete_Click);
			// 
			// ToolStripMenuItem_parametrs
			// 
			this->ToolStripMenuItem_parametrs->Name = L"ToolStripMenuItem_parametrs";
			this->ToolStripMenuItem_parametrs->Size = System::Drawing::Size(223, 26);
			this->ToolStripMenuItem_parametrs->Text = L"Параметры ";
			this->ToolStripMenuItem_parametrs->Click += gcnew System::EventHandler(this, &MyForm::ToolStripMenuItem_parametrs_Click);
			// 
			// toolStripSeparator1
			// 
			this->toolStripSeparator1->Name = L"toolStripSeparator1";
			this->toolStripSeparator1->Size = System::Drawing::Size(220, 6);
			// 
			// сохрвнитьДляАлехToolStripMenuItem
			// 
			this->сохрвнитьДляАлехToolStripMenuItem->Name = L"сохрвнитьДляАлехToolStripMenuItem";
			this->сохрвнитьДляАлехToolStripMenuItem->Size = System::Drawing::Size(223, 26);
			this->сохрвнитьДляАлехToolStripMenuItem->Text = L"Сохрвнить для Алех";
			// 
			// toolStripComboBox_sonde_type
			// 
			this->toolStripComboBox_sonde_type->Items->AddRange(gcnew cli::array< System::Object^  >(4) {
				L"LWD_4Tx", L"CARTOGRAPH", L"AUTONOM_5Tx",
					L"AUTONOM_5Tx_SDR"
			});
			this->toolStripComboBox_sonde_type->Name = L"toolStripComboBox_sonde_type";
			this->toolStripComboBox_sonde_type->Size = System::Drawing::Size(121, 28);
			this->toolStripComboBox_sonde_type->SelectedIndexChanged += gcnew System::EventHandler(this, &MyForm::toolStripComboBox_sonde_type_SelectedIndexChanged);
			// 
			// label_T
			// 
			this->label_T->AutoSize = true;
			this->label_T->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 11.25F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->label_T->Location = System::Drawing::Point(1117, 1);
			this->label_T->Margin = System::Windows::Forms::Padding(2, 0, 2, 0);
			this->label_T->Name = L"label_T";
			this->label_T->Size = System::Drawing::Size(43, 24);
			this->label_T->TabIndex = 102;
			this->label_T->Text = L"T C";
			// 
			// checkBox_add_file
			// 
			this->checkBox_add_file->AutoSize = true;
			this->checkBox_add_file->Location = System::Drawing::Point(783, 1);
			this->checkBox_add_file->Margin = System::Windows::Forms::Padding(5, 0, 0, 0);
			this->checkBox_add_file->Name = L"checkBox_add_file";
			this->checkBox_add_file->Size = System::Drawing::Size(91, 21);
			this->checkBox_add_file->TabIndex = 111;
			this->checkBox_add_file->Text = L"ADD FILE";
			this->checkBox_add_file->UseVisualStyleBackColor = true;
			// 
			// checkBox_re_im
			// 
			this->checkBox_re_im->AutoSize = true;
			this->checkBox_re_im->Location = System::Drawing::Point(1014, 3);
			this->checkBox_re_im->Margin = System::Windows::Forms::Padding(2);
			this->checkBox_re_im->Name = L"checkBox_re_im";
			this->checkBox_re_im->Size = System::Drawing::Size(66, 21);
			this->checkBox_re_im->TabIndex = 88;
			this->checkBox_re_im->Text = L"Re Im";
			this->checkBox_re_im->UseVisualStyleBackColor = true;
			this->checkBox_re_im->CheckedChanged += gcnew System::EventHandler(this, &MyForm::checkBox_re_im_CheckedChanged);
			// 
			// checkBox_stable_ph
			// 
			this->checkBox_stable_ph->AutoSize = true;
			this->checkBox_stable_ph->Location = System::Drawing::Point(898, 1);
			this->checkBox_stable_ph->Margin = System::Windows::Forms::Padding(5, 0, 0, 0);
			this->checkBox_stable_ph->Name = L"checkBox_stable_ph";
			this->checkBox_stable_ph->Size = System::Drawing::Size(82, 21);
			this->checkBox_stable_ph->TabIndex = 112;
			this->checkBox_stable_ph->Text = L"Stab PH";
			this->checkBox_stable_ph->UseVisualStyleBackColor = true;
			this->checkBox_stable_ph->CheckedChanged += gcnew System::EventHandler(this, &MyForm::checkBox_stable_ph_CheckedChanged);
			// 
			// progressBar1
			// 
			this->progressBar1->Location = System::Drawing::Point(1400, 3);
			this->progressBar1->Margin = System::Windows::Forms::Padding(2);
			this->progressBar1->Name = L"progressBar1";
			this->progressBar1->Size = System::Drawing::Size(116, 21);
			this->progressBar1->TabIndex = 100;
			// 
			// tableLayoutPanel6
			// 
			this->tableLayoutPanel6->ColumnCount = 9;
			this->tableLayoutPanel6->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle()));
			this->tableLayoutPanel6->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle()));
			this->tableLayoutPanel6->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle()));
			this->tableLayoutPanel6->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle()));
			this->tableLayoutPanel6->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle()));
			this->tableLayoutPanel6->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle()));
			this->tableLayoutPanel6->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle()));
			this->tableLayoutPanel6->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Absolute,
				115)));
			this->tableLayoutPanel6->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Absolute,
				42)));
			this->tableLayoutPanel6->Controls->Add(this->checkBoxT23, 5, 0);
			this->tableLayoutPanel6->Controls->Add(this->checkBoxT22, 4, 0);
			this->tableLayoutPanel6->Controls->Add(this->checkBoxT21, 3, 0);
			this->tableLayoutPanel6->Controls->Add(this->button_back_2, 0, 0);
			this->tableLayoutPanel6->Controls->Add(this->button_re_im_2, 1, 0);
			this->tableLayoutPanel6->Controls->Add(this->button_forward_2, 2, 0);
			this->tableLayoutPanel6->Controls->Add(this->checkBoxT24, 6, 0);
			this->tableLayoutPanel6->Controls->Add(this->label_dv2, 7, 0);
			this->tableLayoutPanel6->Location = System::Drawing::Point(1400, 445);
			this->tableLayoutPanel6->Margin = System::Windows::Forms::Padding(2);
			this->tableLayoutPanel6->Name = L"tableLayoutPanel6";
			this->tableLayoutPanel6->RowCount = 1;
			this->tableLayoutPanel6->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Percent, 100)));
			this->tableLayoutPanel6->Size = System::Drawing::Size(458, 26);
			this->tableLayoutPanel6->TabIndex = 102;
			// 
			// checkBoxT23
			// 
			this->checkBoxT23->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Bottom)
				| System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->checkBoxT23->AutoSize = true;
			this->checkBoxT23->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 7.8F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->checkBoxT23->Location = System::Drawing::Point(274, 0);
			this->checkBoxT23->Margin = System::Windows::Forms::Padding(0);
			this->checkBoxT23->Name = L"checkBoxT23";
			this->checkBoxT23->Size = System::Drawing::Size(49, 26);
			this->checkBoxT23->TabIndex = 95;
			this->checkBoxT23->Text = L"T3";
			this->checkBoxT23->UseVisualStyleBackColor = true;
			// 
			// checkBoxT22
			// 
			this->checkBoxT22->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Bottom)
				| System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->checkBoxT22->AutoSize = true;
			this->checkBoxT22->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 7.8F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->checkBoxT22->Location = System::Drawing::Point(225, 0);
			this->checkBoxT22->Margin = System::Windows::Forms::Padding(0);
			this->checkBoxT22->Name = L"checkBoxT22";
			this->checkBoxT22->Size = System::Drawing::Size(49, 26);
			this->checkBoxT22->TabIndex = 94;
			this->checkBoxT22->Text = L"T2";
			this->checkBoxT22->UseVisualStyleBackColor = true;
			// 
			// checkBoxT21
			// 
			this->checkBoxT21->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Bottom)
				| System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->checkBoxT21->AutoSize = true;
			this->checkBoxT21->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 7.8F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->checkBoxT21->Location = System::Drawing::Point(176, 0);
			this->checkBoxT21->Margin = System::Windows::Forms::Padding(10, 0, 0, 0);
			this->checkBoxT21->Name = L"checkBoxT21";
			this->checkBoxT21->Size = System::Drawing::Size(49, 26);
			this->checkBoxT21->TabIndex = 93;
			this->checkBoxT21->Text = L"T1";
			this->checkBoxT21->UseVisualStyleBackColor = true;
			// 
			// button_back_2
			// 
			this->button_back_2->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 10.2F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->button_back_2->Location = System::Drawing::Point(2, 0);
			this->button_back_2->Margin = System::Windows::Forms::Padding(2, 0, 0, 0);
			this->button_back_2->Name = L"button_back_2";
			this->button_back_2->Size = System::Drawing::Size(44, 26);
			this->button_back_2->TabIndex = 91;
			this->button_back_2->Text = L"<<";
			this->button_back_2->UseVisualStyleBackColor = true;
			this->button_back_2->Click += gcnew System::EventHandler(this, &MyForm::button_back_2_Click);
			// 
			// button_re_im_2
			// 
			this->button_re_im_2->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Bottom)
				| System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->button_re_im_2->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9.75F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->button_re_im_2->Location = System::Drawing::Point(48, 0);
			this->button_re_im_2->Margin = System::Windows::Forms::Padding(2, 0, 0, 0);
			this->button_re_im_2->Name = L"button_re_im_2";
			this->button_re_im_2->Size = System::Drawing::Size(72, 26);
			this->button_re_im_2->TabIndex = 89;
			this->button_re_im_2->Text = L"amp2";
			this->button_re_im_2->UseVisualStyleBackColor = true;
			this->button_re_im_2->Click += gcnew System::EventHandler(this, &MyForm::button_re_im_2_Click);
			// 
			// button_forward_2
			// 
			this->button_forward_2->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 10.2F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->button_forward_2->Location = System::Drawing::Point(122, 0);
			this->button_forward_2->Margin = System::Windows::Forms::Padding(2, 0, 0, 0);
			this->button_forward_2->Name = L"button_forward_2";
			this->button_forward_2->Size = System::Drawing::Size(44, 26);
			this->button_forward_2->TabIndex = 92;
			this->button_forward_2->Text = L">>";
			this->button_forward_2->UseVisualStyleBackColor = true;
			this->button_forward_2->Click += gcnew System::EventHandler(this, &MyForm::button_forward_2_Click);
			// 
			// checkBoxT24
			// 
			this->checkBoxT24->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Bottom)
				| System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->checkBoxT24->AutoSize = true;
			this->checkBoxT24->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 7.8F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->checkBoxT24->Location = System::Drawing::Point(323, 0);
			this->checkBoxT24->Margin = System::Windows::Forms::Padding(0);
			this->checkBoxT24->Name = L"checkBoxT24";
			this->checkBoxT24->Size = System::Drawing::Size(49, 26);
			this->checkBoxT24->TabIndex = 96;
			this->checkBoxT24->Text = L"T4";
			this->checkBoxT24->UseVisualStyleBackColor = true;
			// 
			// label_dv2
			// 
			this->label_dv2->AutoSize = true;
			this->label_dv2->Location = System::Drawing::Point(374, 0);
			this->label_dv2->Margin = System::Windows::Forms::Padding(2, 0, 2, 0);
			this->label_dv2->Name = L"label_dv2";
			this->label_dv2->Size = System::Drawing::Size(31, 17);
			this->label_dv2->TabIndex = 98;
			this->label_dv2->Text = L"dv2";
			// 
			// chart3
			// 
			this->chart3->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Bottom)
				| System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->chart3->BorderlineColor = System::Drawing::Color::Black;
			this->chart3->BorderlineDashStyle = System::Windows::Forms::DataVisualization::Charting::ChartDashStyle::Solid;
			chartArea2->CursorX->IsUserEnabled = true;
			chartArea2->CursorX->IsUserSelectionEnabled = true;
			chartArea2->CursorY->Interval = 0.0001;
			chartArea2->CursorY->IsUserEnabled = true;
			chartArea2->CursorY->IsUserSelectionEnabled = true;
			chartArea2->Name = L"ChartArea1";
			this->chart3->ChartAreas->Add(chartArea2);
			this->chart3->ContextMenuStrip = this->contextMenuStrip1;
			legend2->Name = L"Legend1";
			legend2->Position->Auto = false;
			legend2->Position->Height = 5;
			legend2->Position->Width = 50;
			legend2->Position->X = 30;
			legend2->Position->Y = 1;
			this->chart3->Legends->Add(legend2);
			this->chart3->Location = System::Drawing::Point(1400, 80);
			this->chart3->Margin = System::Windows::Forms::Padding(2);
			this->chart3->Name = L"chart3";
			series12->BorderWidth = 2;
			series12->ChartArea = L"ChartArea1";
			series12->ChartType = System::Windows::Forms::DataVisualization::Charting::SeriesChartType::Point;
			series12->Color = System::Drawing::Color::Green;
			series12->Legend = L"Legend1";
			series12->Name = L"T1";
			series13->BorderWidth = 2;
			series13->ChartArea = L"ChartArea1";
			series13->ChartType = System::Windows::Forms::DataVisualization::Charting::SeriesChartType::Point;
			series13->Color = System::Drawing::Color::Chocolate;
			series13->Legend = L"Legend1";
			series13->Name = L"T2";
			series14->BorderWidth = 2;
			series14->ChartArea = L"ChartArea1";
			series14->ChartType = System::Windows::Forms::DataVisualization::Charting::SeriesChartType::Point;
			series14->Color = System::Drawing::Color::Red;
			series14->Legend = L"Legend1";
			series14->Name = L"T3";
			series15->BorderWidth = 2;
			series15->ChartArea = L"ChartArea1";
			series15->ChartType = System::Windows::Forms::DataVisualization::Charting::SeriesChartType::Point;
			series15->Color = System::Drawing::Color::Blue;
			series15->Legend = L"Legend1";
			series15->Name = L"T4";
			series16->BorderWidth = 2;
			series16->ChartArea = L"ChartArea1";
			series16->ChartType = System::Windows::Forms::DataVisualization::Charting::SeriesChartType::Line;
			series16->Color = System::Drawing::Color::DarkOliveGreen;
			series16->IsVisibleInLegend = false;
			series16->Legend = L"Legend1";
			series16->Name = L"T1_";
			series17->BorderWidth = 2;
			series17->ChartArea = L"ChartArea1";
			series17->ChartType = System::Windows::Forms::DataVisualization::Charting::SeriesChartType::Line;
			series17->Color = System::Drawing::Color::Gold;
			series17->IsVisibleInLegend = false;
			series17->Legend = L"Legend1";
			series17->Name = L"T2_";
			series18->BorderWidth = 2;
			series18->ChartArea = L"ChartArea1";
			series18->ChartType = System::Windows::Forms::DataVisualization::Charting::SeriesChartType::Line;
			series18->Color = System::Drawing::Color::DarkRed;
			series18->IsVisibleInLegend = false;
			series18->Legend = L"Legend1";
			series18->Name = L"T3_";
			series19->BorderWidth = 2;
			series19->ChartArea = L"ChartArea1";
			series19->ChartType = System::Windows::Forms::DataVisualization::Charting::SeriesChartType::Line;
			series19->Color = System::Drawing::Color::DarkBlue;
			series19->IsVisibleInLegend = false;
			series19->Legend = L"Legend1";
			series19->Name = L"T4_";
			series20->BorderWidth = 2;
			series20->ChartArea = L"ChartArea1";
			series20->ChartType = System::Windows::Forms::DataVisualization::Charting::SeriesChartType::Line;
			series20->Color = System::Drawing::Color::OliveDrab;
			series20->Legend = L"Legend1";
			series20->Name = L"T1__";
			series21->BorderWidth = 2;
			series21->ChartArea = L"ChartArea1";
			series21->ChartType = System::Windows::Forms::DataVisualization::Charting::SeriesChartType::Line;
			series21->Color = System::Drawing::Color::Gold;
			series21->Legend = L"Legend1";
			series21->Name = L"T2__";
			series22->BorderWidth = 2;
			series22->ChartArea = L"ChartArea1";
			series22->ChartType = System::Windows::Forms::DataVisualization::Charting::SeriesChartType::Line;
			series22->Color = System::Drawing::Color::DarkRed;
			series22->Legend = L"Legend1";
			series22->Name = L"T3__";
			series23->BorderWidth = 2;
			series23->ChartArea = L"ChartArea1";
			series23->ChartType = System::Windows::Forms::DataVisualization::Charting::SeriesChartType::Line;
			series23->Color = System::Drawing::Color::MidnightBlue;
			series23->Legend = L"Legend1";
			series23->Name = L"T4__";
			this->chart3->Series->Add(series12);
			this->chart3->Series->Add(series13);
			this->chart3->Series->Add(series14);
			this->chart3->Series->Add(series15);
			this->chart3->Series->Add(series16);
			this->chart3->Series->Add(series17);
			this->chart3->Series->Add(series18);
			this->chart3->Series->Add(series19);
			this->chart3->Series->Add(series20);
			this->chart3->Series->Add(series21);
			this->chart3->Series->Add(series22);
			this->chart3->Series->Add(series23);
			this->chart3->Size = System::Drawing::Size(499, 360);
			this->chart3->TabIndex = 100;
			this->chart3->Text = L"CCCC";
			this->chart3->TextAntiAliasingQuality = System::Windows::Forms::DataVisualization::Charting::TextAntiAliasingQuality::Normal;
			// 
			// chart4
			// 
			this->chart4->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Bottom)
				| System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->chart4->BorderlineColor = System::Drawing::Color::Black;
			this->chart4->BorderlineDashStyle = System::Windows::Forms::DataVisualization::Charting::ChartDashStyle::Solid;
			chartArea3->CursorX->IsUserEnabled = true;
			chartArea3->CursorX->IsUserSelectionEnabled = true;
			chartArea3->CursorY->Interval = 0.0001;
			chartArea3->CursorY->IsUserEnabled = true;
			chartArea3->CursorY->IsUserSelectionEnabled = true;
			chartArea3->Name = L"ChartArea1";
			this->chart4->ChartAreas->Add(chartArea3);
			this->chart4->ContextMenuStrip = this->contextMenuStrip1;
			legend3->Name = L"Legend1";
			legend3->Position->Auto = false;
			legend3->Position->Height = 5;
			legend3->Position->Width = 50;
			legend3->Position->X = 30;
			legend3->Position->Y = 1;
			this->chart4->Legends->Add(legend3);
			this->chart4->Location = System::Drawing::Point(1400, 485);
			this->chart4->Margin = System::Windows::Forms::Padding(2);
			this->chart4->Name = L"chart4";
			series24->BorderWidth = 2;
			series24->ChartArea = L"ChartArea1";
			series24->ChartType = System::Windows::Forms::DataVisualization::Charting::SeriesChartType::Point;
			series24->Color = System::Drawing::Color::Green;
			series24->Legend = L"Legend1";
			series24->Name = L"T1";
			series25->BorderWidth = 2;
			series25->ChartArea = L"ChartArea1";
			series25->ChartType = System::Windows::Forms::DataVisualization::Charting::SeriesChartType::Point;
			series25->Color = System::Drawing::Color::Chocolate;
			series25->Legend = L"Legend1";
			series25->Name = L"T2";
			series26->BorderWidth = 2;
			series26->ChartArea = L"ChartArea1";
			series26->ChartType = System::Windows::Forms::DataVisualization::Charting::SeriesChartType::Point;
			series26->Color = System::Drawing::Color::Red;
			series26->Legend = L"Legend1";
			series26->Name = L"T3";
			series27->BorderWidth = 2;
			series27->ChartArea = L"ChartArea1";
			series27->ChartType = System::Windows::Forms::DataVisualization::Charting::SeriesChartType::Point;
			series27->Color = System::Drawing::Color::Blue;
			series27->Legend = L"Legend1";
			series27->Name = L"T4";
			series28->BorderWidth = 2;
			series28->ChartArea = L"ChartArea1";
			series28->ChartType = System::Windows::Forms::DataVisualization::Charting::SeriesChartType::Line;
			series28->Color = System::Drawing::Color::DarkOliveGreen;
			series28->IsVisibleInLegend = false;
			series28->Legend = L"Legend1";
			series28->Name = L"T1_";
			series29->BorderWidth = 2;
			series29->ChartArea = L"ChartArea1";
			series29->ChartType = System::Windows::Forms::DataVisualization::Charting::SeriesChartType::Line;
			series29->Color = System::Drawing::Color::Gold;
			series29->IsVisibleInLegend = false;
			series29->Legend = L"Legend1";
			series29->Name = L"T2_";
			series30->BorderWidth = 2;
			series30->ChartArea = L"ChartArea1";
			series30->ChartType = System::Windows::Forms::DataVisualization::Charting::SeriesChartType::Line;
			series30->Color = System::Drawing::Color::DarkRed;
			series30->IsVisibleInLegend = false;
			series30->Legend = L"Legend1";
			series30->Name = L"T3_";
			series31->BorderWidth = 2;
			series31->ChartArea = L"ChartArea1";
			series31->ChartType = System::Windows::Forms::DataVisualization::Charting::SeriesChartType::Line;
			series31->Color = System::Drawing::Color::DarkBlue;
			series31->IsVisibleInLegend = false;
			series31->Legend = L"Legend1";
			series31->Name = L"T4_";
			series32->BorderWidth = 2;
			series32->ChartArea = L"ChartArea1";
			series32->ChartType = System::Windows::Forms::DataVisualization::Charting::SeriesChartType::Line;
			series32->Color = System::Drawing::Color::OliveDrab;
			series32->Legend = L"Legend1";
			series32->Name = L"T1__";
			series33->ChartArea = L"ChartArea1";
			series33->ChartType = System::Windows::Forms::DataVisualization::Charting::SeriesChartType::Line;
			series33->Color = System::Drawing::Color::Yellow;
			series33->Legend = L"Legend1";
			series33->Name = L"T2__";
			series34->ChartArea = L"ChartArea1";
			series34->ChartType = System::Windows::Forms::DataVisualization::Charting::SeriesChartType::Line;
			series34->Color = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(192)), static_cast<System::Int32>(static_cast<System::Byte>(0)),
				static_cast<System::Int32>(static_cast<System::Byte>(0)));
			series34->Legend = L"Legend1";
			series34->Name = L"T3__";
			series35->ChartArea = L"ChartArea1";
			series35->ChartType = System::Windows::Forms::DataVisualization::Charting::SeriesChartType::Line;
			series35->Color = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(0)), static_cast<System::Int32>(static_cast<System::Byte>(0)),
				static_cast<System::Int32>(static_cast<System::Byte>(192)));
			series35->Legend = L"Legend1";
			series35->Name = L"T4__";
			this->chart4->Series->Add(series24);
			this->chart4->Series->Add(series25);
			this->chart4->Series->Add(series26);
			this->chart4->Series->Add(series27);
			this->chart4->Series->Add(series28);
			this->chart4->Series->Add(series29);
			this->chart4->Series->Add(series30);
			this->chart4->Series->Add(series31);
			this->chart4->Series->Add(series32);
			this->chart4->Series->Add(series33);
			this->chart4->Series->Add(series34);
			this->chart4->Series->Add(series35);
			this->chart4->Size = System::Drawing::Size(499, 360);
			this->chart4->TabIndex = 103;
			this->chart4->Text = L"CCCC";
			this->chart4->TextAntiAliasingQuality = System::Windows::Forms::DataVisualization::Charting::TextAntiAliasingQuality::Normal;
			// 
			// chart2
			// 
			this->chart2->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Bottom)
				| System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->chart2->BorderlineColor = System::Drawing::Color::Black;
			this->chart2->BorderlineDashStyle = System::Windows::Forms::DataVisualization::Charting::ChartDashStyle::Solid;
			chartArea4->CursorX->IsUserEnabled = true;
			chartArea4->CursorX->IsUserSelectionEnabled = true;
			chartArea4->CursorY->Interval = 0.0001;
			chartArea4->CursorY->IsUserEnabled = true;
			chartArea4->CursorY->IsUserSelectionEnabled = true;
			chartArea4->Name = L"ChartArea1";
			this->chart2->ChartAreas->Add(chartArea4);
			this->chart2->ContextMenuStrip = this->contextMenuStrip2;
			legend4->Name = L"Legend1";
			legend4->Position->Auto = false;
			legend4->Position->Height = 5;
			legend4->Position->Width = 25;
			legend4->Position->X = 30;
			legend4->Position->Y = 1;
			this->chart2->Legends->Add(legend4);
			this->chart2->Location = System::Drawing::Point(3, 485);
			this->chart2->Margin = System::Windows::Forms::Padding(2);
			this->chart2->Name = L"chart2";
			series36->BorderWidth = 2;
			series36->ChartArea = L"ChartArea1";
			series36->ChartType = System::Windows::Forms::DataVisualization::Charting::SeriesChartType::Line;
			series36->Color = System::Drawing::Color::Green;
			series36->Legend = L"Legend1";
			series36->Name = L"T1";
			series37->BorderWidth = 2;
			series37->ChartArea = L"ChartArea1";
			series37->ChartType = System::Windows::Forms::DataVisualization::Charting::SeriesChartType::Line;
			series37->Color = System::Drawing::Color::DarkGoldenrod;
			series37->Legend = L"Legend1";
			series37->Name = L"T2";
			series38->BorderWidth = 2;
			series38->ChartArea = L"ChartArea1";
			series38->ChartType = System::Windows::Forms::DataVisualization::Charting::SeriesChartType::Line;
			series38->Color = System::Drawing::Color::Red;
			series38->Legend = L"Legend1";
			series38->Name = L"T3";
			series39->BorderWidth = 2;
			series39->ChartArea = L"ChartArea1";
			series39->ChartType = System::Windows::Forms::DataVisualization::Charting::SeriesChartType::Line;
			series39->Color = System::Drawing::Color::RoyalBlue;
			series39->Legend = L"Legend1";
			series39->Name = L"T4";
			series40->BorderWidth = 2;
			series40->ChartArea = L"ChartArea1";
			series40->ChartType = System::Windows::Forms::DataVisualization::Charting::SeriesChartType::Line;
			series40->Color = System::Drawing::Color::Magenta;
			series40->Legend = L"Legend1";
			series40->Name = L"T5";
			series41->BorderDashStyle = System::Windows::Forms::DataVisualization::Charting::ChartDashStyle::Dash;
			series41->BorderWidth = 2;
			series41->ChartArea = L"ChartArea1";
			series41->ChartType = System::Windows::Forms::DataVisualization::Charting::SeriesChartType::Line;
			series41->Color = System::Drawing::Color::Green;
			series41->IsVisibleInLegend = false;
			series41->Legend = L"Legend1";
			series41->Name = L"T1_";
			series42->BorderDashStyle = System::Windows::Forms::DataVisualization::Charting::ChartDashStyle::Dash;
			series42->BorderWidth = 2;
			series42->ChartArea = L"ChartArea1";
			series42->ChartType = System::Windows::Forms::DataVisualization::Charting::SeriesChartType::Line;
			series42->Color = System::Drawing::Color::DarkGoldenrod;
			series42->IsVisibleInLegend = false;
			series42->Legend = L"Legend1";
			series42->Name = L"T2_";
			series43->BorderDashStyle = System::Windows::Forms::DataVisualization::Charting::ChartDashStyle::Dash;
			series43->BorderWidth = 2;
			series43->ChartArea = L"ChartArea1";
			series43->ChartType = System::Windows::Forms::DataVisualization::Charting::SeriesChartType::Line;
			series43->Color = System::Drawing::Color::Red;
			series43->IsVisibleInLegend = false;
			series43->Legend = L"Legend1";
			series43->Name = L"T3_";
			series44->BorderDashStyle = System::Windows::Forms::DataVisualization::Charting::ChartDashStyle::Dash;
			series44->BorderWidth = 2;
			series44->ChartArea = L"ChartArea1";
			series44->ChartType = System::Windows::Forms::DataVisualization::Charting::SeriesChartType::Line;
			series44->Color = System::Drawing::Color::RoyalBlue;
			series44->IsVisibleInLegend = false;
			series44->Legend = L"Legend1";
			series44->Name = L"T4_";
			series45->BorderDashStyle = System::Windows::Forms::DataVisualization::Charting::ChartDashStyle::Dash;
			series45->BorderWidth = 2;
			series45->ChartArea = L"ChartArea1";
			series45->ChartType = System::Windows::Forms::DataVisualization::Charting::SeriesChartType::Line;
			series45->Color = System::Drawing::Color::Magenta;
			series45->Legend = L"Legend1";
			series45->Name = L"T5_";
			series46->BorderDashStyle = System::Windows::Forms::DataVisualization::Charting::ChartDashStyle::Dash;
			series46->BorderWidth = 2;
			series46->ChartArea = L"ChartArea1";
			series46->ChartType = System::Windows::Forms::DataVisualization::Charting::SeriesChartType::Line;
			series46->Color = System::Drawing::Color::Black;
			series46->Legend = L"Legend1";
			series46->Name = L"zero";
			this->chart2->Series->Add(series36);
			this->chart2->Series->Add(series37);
			this->chart2->Series->Add(series38);
			this->chart2->Series->Add(series39);
			this->chart2->Series->Add(series40);
			this->chart2->Series->Add(series41);
			this->chart2->Series->Add(series42);
			this->chart2->Series->Add(series43);
			this->chart2->Series->Add(series44);
			this->chart2->Series->Add(series45);
			this->chart2->Series->Add(series46);
			this->chart2->Size = System::Drawing::Size(1392, 360);
			this->chart2->TabIndex = 104;
			this->chart2->Text = L"CCCC";
			this->chart2->TextAntiAliasingQuality = System::Windows::Forms::DataVisualization::Charting::TextAntiAliasingQuality::Normal;
			this->chart2->CursorPositionChanged += gcnew System::EventHandler<System::Windows::Forms::DataVisualization::Charting::CursorEventArgs^ >(this, &MyForm::cursor_position1);
			// 
			// tableLayoutPanel3
			// 
			this->tableLayoutPanel3->ColumnCount = 10;
			this->tableLayoutPanel3->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle()));
			this->tableLayoutPanel3->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle()));
			this->tableLayoutPanel3->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle()));
			this->tableLayoutPanel3->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle()));
			this->tableLayoutPanel3->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle()));
			this->tableLayoutPanel3->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle()));
			this->tableLayoutPanel3->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle()));
			this->tableLayoutPanel3->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle()));
			this->tableLayoutPanel3->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle()));
			this->tableLayoutPanel3->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle()));
			this->tableLayoutPanel3->Controls->Add(this->checkBox8, 8, 0);
			this->tableLayoutPanel3->Controls->Add(this->textBox_cursor1, 7, 0);
			this->tableLayoutPanel3->Controls->Add(this->button_direct1, 0, 0);
			this->tableLayoutPanel3->Controls->Add(this->button5, 6, 0);
			this->tableLayoutPanel3->Controls->Add(this->checkBox1, 9, 0);
			this->tableLayoutPanel3->Controls->Add(this->comboBox_choise_chart1, 1, 0);
			this->tableLayoutPanel3->Controls->Add(this->textBox_min1, 4, 0);
			this->tableLayoutPanel3->Controls->Add(this->button_ZX_1, 2, 0);
			this->tableLayoutPanel3->Controls->Add(this->button_400_2000_1, 3, 0);
			this->tableLayoutPanel3->Controls->Add(this->textBox_max1, 5, 0);
			this->tableLayoutPanel3->Location = System::Drawing::Point(1, 39);
			this->tableLayoutPanel3->Margin = System::Windows::Forms::Padding(0, 2, 0, 0);
			this->tableLayoutPanel3->Name = L"tableLayoutPanel3";
			this->tableLayoutPanel3->RowCount = 1;
			this->tableLayoutPanel3->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Percent, 100)));
			this->tableLayoutPanel3->Size = System::Drawing::Size(1144, 35);
			this->tableLayoutPanel3->TabIndex = 110;
			// 
			// checkBox8
			// 
			this->checkBox8->AutoSize = true;
			this->checkBox8->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 7.8F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->checkBox8->Location = System::Drawing::Point(991, 0);
			this->checkBox8->Margin = System::Windows::Forms::Padding(12, 0, 0, 0);
			this->checkBox8->Name = L"checkBox8";
			this->checkBox8->Size = System::Drawing::Size(68, 21);
			this->checkBox8->TabIndex = 84;
			this->checkBox8->Text = L"zoom";
			this->checkBox8->UseVisualStyleBackColor = true;
			// 
			// button_direct1
			// 
			this->button_direct1->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->button_direct1->Location = System::Drawing::Point(2, 0);
			this->button_direct1->Margin = System::Windows::Forms::Padding(2, 0, 2, 0);
			this->button_direct1->Name = L"button_direct1";
			this->button_direct1->Size = System::Drawing::Size(82, 30);
			this->button_direct1->TabIndex = 89;
			this->button_direct1->Text = L"UNDIR";
			this->button_direct1->UseVisualStyleBackColor = true;
			this->button_direct1->Click += gcnew System::EventHandler(this, &MyForm::button_direct1_Click);
			// 
			// button_ZX_1
			// 
			this->button_ZX_1->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9.75F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->button_ZX_1->Location = System::Drawing::Point(349, 0);
			this->button_ZX_1->Margin = System::Windows::Forms::Padding(2, 0, 2, 0);
			this->button_ZX_1->Name = L"button_ZX_1";
			this->button_ZX_1->Size = System::Drawing::Size(88, 30);
			this->button_ZX_1->TabIndex = 88;
			this->button_ZX_1->Text = L"R_Zx L";
			this->button_ZX_1->UseVisualStyleBackColor = true;
			this->button_ZX_1->Click += gcnew System::EventHandler(this, &MyForm::button_ZX_1_Click);
			// 
			// button_400_2000_1
			// 
			this->button_400_2000_1->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9.75F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->button_400_2000_1->Location = System::Drawing::Point(441, 0);
			this->button_400_2000_1->Margin = System::Windows::Forms::Padding(2, 0, 2, 0);
			this->button_400_2000_1->Name = L"button_400_2000_1";
			this->button_400_2000_1->Size = System::Drawing::Size(75, 30);
			this->button_400_2000_1->TabIndex = 87;
			this->button_400_2000_1->Text = L"2000";
			this->button_400_2000_1->UseVisualStyleBackColor = true;
			this->button_400_2000_1->Click += gcnew System::EventHandler(this, &MyForm::button_400_2000_1_Click);
			// 
			// panel1
			// 
			this->panel1->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Bottom)
				| System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->panel1->Controls->Add(this->textBox_T5);
			this->panel1->Controls->Add(this->textBox_T4);
			this->panel1->Controls->Add(this->textBox_T3);
			this->panel1->Controls->Add(this->textBox_T2);
			this->panel1->Controls->Add(this->textBox_T1);
			this->panel1->Controls->Add(this->label_ro);
			this->panel1->Controls->Add(this->trackBar1);
			this->panel1->Location = System::Drawing::Point(1, 970);
			this->panel1->Margin = System::Windows::Forms::Padding(0);
			this->panel1->Name = L"panel1";
			this->panel1->Size = System::Drawing::Size(1396, 61);
			this->panel1->TabIndex = 112;
			// 
			// textBox_T5
			// 
			this->textBox_T5->Location = System::Drawing::Point(1074, 20);
			this->textBox_T5->Name = L"textBox_T5";
			this->textBox_T5->Size = System::Drawing::Size(63, 22);
			this->textBox_T5->TabIndex = 117;
			this->textBox_T5->TextChanged += gcnew System::EventHandler(this, &MyForm::textBox_T5_TextChanged);
			// 
			// textBox_T4
			// 
			this->textBox_T4->Location = System::Drawing::Point(980, 20);
			this->textBox_T4->Name = L"textBox_T4";
			this->textBox_T4->Size = System::Drawing::Size(63, 22);
			this->textBox_T4->TabIndex = 116;
			this->textBox_T4->TextChanged += gcnew System::EventHandler(this, &MyForm::textBox_T4_TextChanged);
			// 
			// textBox_T3
			// 
			this->textBox_T3->Location = System::Drawing::Point(888, 20);
			this->textBox_T3->Name = L"textBox_T3";
			this->textBox_T3->Size = System::Drawing::Size(63, 22);
			this->textBox_T3->TabIndex = 115;
			this->textBox_T3->TextChanged += gcnew System::EventHandler(this, &MyForm::textBox_T3_TextChanged);
			// 
			// textBox_T2
			// 
			this->textBox_T2->Location = System::Drawing::Point(792, 20);
			this->textBox_T2->Name = L"textBox_T2";
			this->textBox_T2->Size = System::Drawing::Size(63, 22);
			this->textBox_T2->TabIndex = 114;
			this->textBox_T2->TextChanged += gcnew System::EventHandler(this, &MyForm::textBox_T2_TextChanged);
			// 
			// textBox_T1
			// 
			this->textBox_T1->Location = System::Drawing::Point(696, 20);
			this->textBox_T1->Name = L"textBox_T1";
			this->textBox_T1->Size = System::Drawing::Size(63, 22);
			this->textBox_T1->TabIndex = 113;
			this->textBox_T1->TextChanged += gcnew System::EventHandler(this, &MyForm::textBox_T1_TextChanged);
			// 
			// label_ro
			// 
			this->label_ro->AutoSize = true;
			this->label_ro->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 10.2F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->label_ro->Location = System::Drawing::Point(520, 20);
			this->label_ro->Name = L"label_ro";
			this->label_ro->Size = System::Drawing::Size(32, 20);
			this->label_ro->TabIndex = 112;
			this->label_ro->Text = L"Ro";
			// 
			// trackBar1
			// 
			this->trackBar1->CausesValidation = false;
			this->trackBar1->LargeChange = 1;
			this->trackBar1->Location = System::Drawing::Point(3, 3);
			this->trackBar1->Maximum = 11;
			this->trackBar1->Name = L"trackBar1";
			this->trackBar1->Size = System::Drawing::Size(486, 56);
			this->trackBar1->TabIndex = 111;
			this->trackBar1->TickStyle = System::Windows::Forms::TickStyle::None;
			this->trackBar1->Scroll += gcnew System::EventHandler(this, &MyForm::trackBar1_Scroll);
			this->trackBar1->ValueChanged += gcnew System::EventHandler(this, &MyForm::trackBar1_Scroll);
			// 
			// toolStripLabel8
			// 
			this->toolStripLabel8->Name = L"toolStripLabel8";
			this->toolStripLabel8->Size = System::Drawing::Size(23, 23);
			// 
			// toolStripSeparator31
			// 
			this->toolStripSeparator31->Name = L"toolStripSeparator31";
			this->toolStripSeparator31->Size = System::Drawing::Size(6, 30);
			// 
			// toolStripSeparator32
			// 
			this->toolStripSeparator32->Name = L"toolStripSeparator32";
			this->toolStripSeparator32->Size = System::Drawing::Size(6, 30);
			// 
			// toolStripLabel9
			// 
			this->toolStripLabel9->Name = L"toolStripLabel9";
			this->toolStripLabel9->Size = System::Drawing::Size(23, 23);
			// 
			// toolStripSeparator33
			// 
			this->toolStripSeparator33->Name = L"toolStripSeparator33";
			this->toolStripSeparator33->Size = System::Drawing::Size(6, 30);
			// 
			// toolStripTextBox_Ro_zp
			// 
			this->toolStripTextBox_Ro_zp->Name = L"toolStripTextBox_Ro_zp";
			this->toolStripTextBox_Ro_zp->Size = System::Drawing::Size(100, 23);
			// 
			// toolStripSeparator34
			// 
			this->toolStripSeparator34->Name = L"toolStripSeparator34";
			this->toolStripSeparator34->Size = System::Drawing::Size(6, 30);
			// 
			// toolStripLabel10
			// 
			this->toolStripLabel10->Name = L"toolStripLabel10";
			this->toolStripLabel10->Size = System::Drawing::Size(40, 27);
			this->toolStripLabel10->Text = L"R_zp";
			// 
			// toolStripSeparator35
			// 
			this->toolStripSeparator35->Name = L"toolStripSeparator35";
			this->toolStripSeparator35->Size = System::Drawing::Size(6, 30);
			// 
			// toolStripTextBox_R_zp
			// 
			this->toolStripTextBox_R_zp->Name = L"toolStripTextBox_R_zp";
			this->toolStripTextBox_R_zp->Size = System::Drawing::Size(60, 30);
			// 
			// toolStripSeparator36
			// 
			this->toolStripSeparator36->Name = L"toolStripSeparator36";
			this->toolStripSeparator36->Size = System::Drawing::Size(6, 30);
			// 
			// прямая_задача
			// 
			this->прямая_задача->DisplayStyle = System::Windows::Forms::ToolStripItemDisplayStyle::Text;
			this->прямая_задача->Font = (gcnew System::Drawing::Font(L"Segoe UI Semibold", 9.75F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(204)));
			this->прямая_задача->Image = (cli::safe_cast<System::Drawing::Image^>(resources->GetObject(L"прямая_задача.Image")));
			this->прямая_задача->ImageTransparentColor = System::Drawing::Color::Magenta;
			this->прямая_задача->Name = L"прямая_задача";
			this->прямая_задача->Size = System::Drawing::Size(84, 27);
			this->прямая_задача->Text = L"PH_CALK";
			// 
			// toolStripSeparator37
			// 
			this->toolStripSeparator37->Name = L"toolStripSeparator37";
			this->toolStripSeparator37->Size = System::Drawing::Size(6, 30);
			// 
			// toolStripLabel11
			// 
			this->toolStripLabel11->Name = L"toolStripLabel11";
			this->toolStripLabel11->Size = System::Drawing::Size(55, 27);
			this->toolStripLabel11->Text = L"X_AXIS";
			// 
			// toolStripSeparator38
			// 
			this->toolStripSeparator38->Name = L"toolStripSeparator38";
			this->toolStripSeparator38->Size = System::Drawing::Size(6, 30);
			// 
			// comboBox_axis
			// 
			this->comboBox_axis->Name = L"comboBox_axis";
			this->comboBox_axis->Size = System::Drawing::Size(121, 21);
			// 
			// MyForm
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(120, 120);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Dpi;
			this->AutoScroll = true;
			this->AutoSize = true;
			this->ClientSize = System::Drawing::Size(1902, 1032);
			this->Controls->Add(this->tableLayoutPanel1);
			this->Controls->Add(this->textBox5);
			this->Margin = System::Windows::Forms::Padding(2);
			this->Name = L"MyForm";
			this->StartPosition = System::Windows::Forms::FormStartPosition::Manual;
			this->Text = L"SONDE_TEST_1.0";
			this->FormClosing += gcnew System::Windows::Forms::FormClosingEventHandler(this, &MyForm::form_close);
			this->Load += gcnew System::EventHandler(this, &MyForm::MyForm_Load);
			this->Shown += gcnew System::EventHandler(this, &MyForm::MyForm_Shown);
			this->Resize += gcnew System::EventHandler(this, &MyForm::MyForm_Resize);
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->chart1))->EndInit();
			this->contextMenuStrip1->ResumeLayout(false);
			this->contextMenuStrip2->ResumeLayout(false);
			this->tableLayoutPanel1->ResumeLayout(false);
			this->panel_Sonde_Picture->ResumeLayout(false);
			this->panel_Sonde_Picture->PerformLayout();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBoxD13))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBoxD12))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBoxD10))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBoxD11))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBoxD9))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBoxD5))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBoxD6))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBoxT5))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBoxD7))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBoxD8))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBoxD3))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBoxD4))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBoxD1))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBoxD2))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBoxT2))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBoxR4))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBoxT4))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBoxR2))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBoxR1))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBoxT1))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBoxT3))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBoxR3))->EndInit();
			this->tableLayoutPanel5->ResumeLayout(false);
			this->tableLayoutPanel5->PerformLayout();
			this->tableLayoutPanel4->ResumeLayout(false);
			this->tableLayoutPanel4->PerformLayout();
			this->tableLayoutPanel10->ResumeLayout(false);
			this->tableLayoutPanel10->PerformLayout();
			this->toolStrip2->ResumeLayout(false);
			this->toolStrip2->PerformLayout();
			this->menuStrip1->ResumeLayout(false);
			this->menuStrip1->PerformLayout();
			this->tableLayoutPanel6->ResumeLayout(false);
			this->tableLayoutPanel6->PerformLayout();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->chart3))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->chart4))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->chart2))->EndInit();
			this->tableLayoutPanel3->ResumeLayout(false);
			this->tableLayoutPanel3->PerformLayout();
			this->panel1->ResumeLayout(false);
			this->panel1->PerformLayout();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->trackBar1))->EndInit();
			this->ResumeLayout(false);
			this->PerformLayout();

		}
#pragma endregion


		int form_width;
		bool surf_on_off = false;// для surf start /stop
		bool on_off_detector_surf = false;
		bool F_on_off = false;
		int zp_calc_400 = 0, zp_calc_2000 = 0;
		int *min_n = new int[2];//начальная и конечная точки на  графике
		int *max_n = new int[2];//
		int freq_n;//
		int file_frames = 0;
		int N_Tx;// количество передатчиков		  

		bool temp_flg = 0, temp_after_delete_flg = 0;
		int  N = 0;
		bool  was_save = 0;
		bool temporary_file = 0;
		int Point_X = 0; int Point_X_2 = 0;
		bool air_is_norm = false;

		const char* Data_file_name = "err";
		const char* ConfName = "Conf.txt";

		String^ extension;
		Thread^ Read_Port_Thread = gcnew Thread(gcnew ThreadStart(this, &MyForm::Read_Port));
		bool start = 0;
		bool first_kadr = true;
		int *n_raw_data_point = new int[2];
		int *cursor_x_position = new int[2];
		int vector_size_cntr = 0;
		
		bool write_to_file_all_data_flg = false;
		ALL_DATA *all_data_buff = new ALL_DATA;
		uint32_t global_signature = 0xaa;

	void do_chart(int chart_N) {
			float mV = 0.00023842;
		checkBox_sonde_calk->Text = "sonde_calk";
		toolStripMenuItemT1->Visible = false;
		toolStripMenuItemT2->Visible = false;
		toolStripMenuItemT3->Visible = false;
		toolStripMenuItemT4->Visible = false;
		toolStripMenuItemT5->Visible = false;

		float sko_summ_am_zero_zz[2] = { 0.0f, 0.0f };;
		float sko_summ_am_zero_zx = { 0.0f };
		float sko_summ2_am_zero_zz[2] = { 0.0f, 0.0f };
		float sko_summ2_am_zero_zx = { 0.0f };

		bool mess_box_flg_show = true;

		Chart^ CHART = (Chart^)this->tableLayoutPanel1->Controls["chart" + (chart_N + 1).ToString()];
		TextBox^ TEXTBOX;
		ComboBox^ COMBOBOX;
		CheckBox^ CHECKBOX_DEPTH;
		CheckBox^ CHECKBOX_TEMPERATURE;
		CheckBox^ CHECKBOX_ZOOM;
		TableLayoutPanel^  TABLELAYOUTPANEL = (TableLayoutPanel^)this->tableLayoutPanel1->Controls["tableLayoutPanel" + (chart_N + 3)];
		COMBOBOX = (ComboBox^)TABLELAYOUTPANEL->Controls["comboBox_choise_chart" + (chart_N + 1)];
		CHECKBOX_TEMPERATURE = (CheckBox^)TABLELAYOUTPANEL->Controls["checkBox" + (chart_N + 1)];
		CHECKBOX_ZOOM = (CheckBox^)TABLELAYOUTPANEL->Controls["checkBox" + (chart_N + 8)];
		TEXTBOX = (TextBox^)TABLELAYOUTPANEL->Controls["textBox_cursor" + (chart_N + 1)]; TEXTBOX->Text = "";

		CHART->ChartAreas["ChartArea1"]->AxisX->LabelStyle->Format = "0";

		if (index_dir_undir[chart_N] == 0) {//UNDIR
			for (int i = 0; i < 5; i++)
				CHART->Series[i]->LegendText = L"T" + (i + 1);

			CHART->Series[0]->Color = System::Drawing::Color::Green;
			CHART->Series[1]->Color = System::Drawing::Color::Chocolate;
			CHART->Series[2]->Color = System::Drawing::Color::Red;
			CHART->Series[3]->Color = System::Drawing::Color::Blue;
			CHART->Series[4]->Color = System::Drawing::Color::Magenta;
		}

		if (index_dir_undir[chart_N] == 1) {//DIRECT
			if (index_rzx[chart_N] == Left_) {//"R_Zx L"
				CHART->Series[0]->LegendText = L"T1";
				CHART->Series[1]->LegendText = L"T2";
				CHART->Series[2]->LegendText = L"T4";
				CHART->Series[3]->LegendText = L"T5";

				CHART->Series[0]->Color = System::Drawing::Color::Green;
				CHART->Series[1]->Color = System::Drawing::Color::Chocolate;
				CHART->Series[2]->Color = System::Drawing::Color::Red;
				CHART->Series[3]->Color = System::Drawing::Color::Blue;
				CHART->Series[4]->Color = System::Drawing::Color::Magenta;

				CHART->Series[5]->Color = System::Drawing::Color::Green;
				CHART->Series[6]->Color = System::Drawing::Color::Chocolate;
				CHART->Series[7]->Color = System::Drawing::Color::Red;
				CHART->Series[8]->Color = System::Drawing::Color::Blue;
				CHART->Series[9]->Color = System::Drawing::Color::Magenta;
			}
			else if (index_rzx[chart_N] == Right_) {//"R_Zx R"
				CHART->Series[0]->LegendText = L"T1";
				CHART->Series[1]->LegendText = L"T2";
				CHART->Series[2]->LegendText = L"T3";
				CHART->Series[3]->LegendText = L"T5";

				CHART->Series[0]->Color = System::Drawing::Color::Chocolate;
				CHART->Series[1]->Color = System::Drawing::Color::Green;
				CHART->Series[2]->Color = System::Drawing::Color::Red;
				CHART->Series[3]->Color = System::Drawing::Color::Blue;
				CHART->Series[4]->Color = System::Drawing::Color::Magenta;

				CHART->Series[5]->Color = System::Drawing::Color::Chocolate;
				CHART->Series[6]->Color = System::Drawing::Color::Green;
				CHART->Series[7]->Color = System::Drawing::Color::Red;
				CHART->Series[8]->Color = System::Drawing::Color::Blue;
				CHART->Series[9]->Color = System::Drawing::Color::Magenta;
			}
		}

		while (busy == true) { ; }

		if (max_n[chart_N] - min_n[chart_N] > 1
			//&& vector_size_cntr >= (max_n[chart_N] - min_n[chart_N])
			) {
			extern complex <float> fourier_k[3][16];

			if (surf_on_off == true)
				CHART->ChartAreas["ChartArea1"]->CursorX->IsUserSelectionEnabled = false;
			else
				CHART->ChartAreas["ChartArea1"]->CursorX->IsUserSelectionEnabled = true;

			//cout << "min_n " << min_n[chart_N] << " max_n " << max_n[chart_N] << " all_data.size " << all_data.size() << " gp_data.size " << gp_data.size() << endl;
			CHART->ChartAreas["ChartArea1"]->AxisY->IsLogarithmic = false;

			if (CHECKBOX_ZOOM->Checked == false
				&& CHART->ChartAreas["ChartArea1"]->AxisX->ScaleView->IsZoomed
				) {
				CHART->ChartAreas["ChartArea1"]->AxisX->ScaleView->ZoomReset(0);
				CHART->ChartAreas["ChartArea1"]->AxisY->ScaleView->ZoomReset(0);
				CHART->ChartAreas["ChartArea1"]->AxisY->Minimum = System::Double::NaN;
				CHART->ChartAreas["ChartArea1"]->AxisY->Maximum = System::Double::NaN;
			}
			progressBar1->Visible = true;
			progressBar1->Minimum = 0;
			progressBar1->Maximum = (max_n[chart_N] - min_n[chart_N]) / 500;
			progressBar1->Value = 0;
			progressBar1->Step = 1;
			//для второго графика выбираем тот же индекс , что и для первого

			if (sonde_type == CARTOGRAPH_ || sonde_type == LWD_4Tx_) {
				N_Tx = 4; mV = 0.00023842;
			}

			if (sonde_type == AUTONOM_5Tx_) {
				N_Tx = 5;  mV = 0.00023842;
			}

			if (sonde_type == AUTONOM_5Tx_SDR_) {
				N_Tx = 5; mV = 3.6;
			}

			for (int i = 0; i < 11; i++) {
				CHART->Series[i]->Points->SuspendUpdates();
				while (CHART->Series[i]->Points->Count > 0) {
					CHART->Series[i]->Points->RemoveAt(CHART->Series[i]->Points->Count - 1);
				}
				CHART->Series[i]->Points->ResumeUpdates();
				CHART->Series[i]->Points->Clear();
			}

			CHART->ChartAreas["ChartArea1"]->AxisX->IntervalAutoMode;
			CHART->ChartAreas["ChartArea1"]->AxisY->IntervalAutoMode;

			int p_b_s_count = 0;

			//считаем СКО для молчащих передатчиков
			int N_ = (max_n[chart_N] - min_n[chart_N]);
			if (COMBOBOX->SelectedItem == "AM Zero ZZ" || COMBOBOX->SelectedItem == "AM Zero ZX") {
				for (int n = min_n[chart_N]; n < max_n[chart_N]; n++) {
					if (sonde_type == CARTOGRAPH_) {
						sko_summ_am_zero_zx += (abs(all_data[N].data[index_freq[chart_N]].R_zx[index_rzx[chart_N]].Tx_0)*mV*1000.0f);
					}
					sko_summ_am_zero_zz[0] += gp_data[n].ZERO_AM_RX_1[index_freq[chart_N]] * mV*1000.0f;
					sko_summ_am_zero_zz[1] += gp_data[n].ZERO_AM_RX_2[index_freq[chart_N]] * mV*1000.0f;
				}
				sko_summ_am_zero_zz[0] /= N_;
				sko_summ_am_zero_zz[1] /= N_;
				if (sonde_type == CARTOGRAPH_) {
					sko_summ_am_zero_zx /= N_;
				}

			}

			//cout << "chart_N " << chart_N << " index_freq[chart_N] " << index_freq[chart_N] << endl;
			//Console::WriteLine(" COMBOBOX->SelectedItem " + COMBOBOX->SelectedItem);


			for (int n = min_n[chart_N]; n < max_n[chart_N]; n++)// количество циклов n
			{
				p_b_s_count++;
				if (p_b_s_count == 500) {
					p_b_s_count = 0;
					progressBar1->PerformStep();
				}

				float N;
				if (CHECKBOX_TEMPERATURE->Checked == true) {
					if (gp_data[n].temperature < -50 || gp_data[n].temperature > 200)
						gp_data[n].temperature = 0;
					N = (gp_data[n].temperature);
				}
				if (CHECKBOX_TEMPERATURE->Checked == false) { N = n; }

				// выводим графики
				//PH Z2 - Z1
				if (COMBOBOX->SelectedItem == "PH Z2-Z1") {
					CHART->ChartAreas["ChartArea1"]->AxisY->Title = "mDeg";
					for (int Tx = 0; Tx < N_Tx; Tx++) {
						CHART->Series[Tx]->Points->AddXY(N, gp_data[n].DELTA_PH[index_freq[chart_N]][Tx] * mG);
					}
					CHART->Series[10]->Points->AddXY(N, 0);

				}

				//AM Z2/Z1
				else if (COMBOBOX->SelectedItem == "AM Z2/Z1") {
					CHART->ChartAreas["ChartArea1"]->AxisY->Title = "";
					//CHART->ChartAreas["ChartArea1"]->AxisY->Minimum = 0;
					float L1[4] = { 358, 720, 1078, 1437 }; float L2[4] = { 598, 960, 1318, 1677 };
					CHART->Series[4]->Points->AddXY(N, pow(L1[0], 3) / pow(L2[0], 3));
					CHART->Series[5]->Points->AddXY(N, pow(L1[1], 3) / pow(L2[1], 3));
					CHART->Series[6]->Points->AddXY(N, pow(L1[2], 3) / pow(L2[2], 3));
					CHART->Series[7]->Points->AddXY(N, pow(L1[3], 3) / pow(L2[3], 3));

					CHART->Series[0]->Points->AddXY(N, gp_data[n].AM_RX_2[index_freq[chart_N]][0] / gp_data[n].AM_RX_1[index_freq[chart_N]][0]);
					CHART->Series[1]->Points->AddXY(N, gp_data[n].AM_RX_1[index_freq[chart_N]][1] / gp_data[n].AM_RX_2[index_freq[chart_N]][1]);
					CHART->Series[2]->Points->AddXY(N, gp_data[n].AM_RX_2[index_freq[chart_N]][2] / gp_data[n].AM_RX_1[index_freq[chart_N]][2]);
					CHART->Series[3]->Points->AddXY(N, gp_data[n].AM_RX_1[index_freq[chart_N]][3] / gp_data[n].AM_RX_2[index_freq[chart_N]][3]);


				}

				//AM Z1
				else if (COMBOBOX->SelectedItem == "AM Z1") {
					CHART->ChartAreas["ChartArea1"]->AxisY->Title = "mV";
					//CHART->ChartAreas["ChartArea1"]->AxisY->Minimum = 0;
					for (int Tx = 0; Tx < N_Tx; Tx++) {
						CHART->Series[Tx]->Points->AddXY(N, gp_data[n].AM_RX_1[index_freq[chart_N]][Tx] * mV);
					}
				}

				//AM Z2
				else if (COMBOBOX->SelectedItem == "AM Z2") {
					CHART->ChartAreas["ChartArea1"]->AxisY->Title = "mV";
					//CHART->ChartAreas["ChartArea1"]->AxisY->Minimum = 0;
					for (int Tx = 0; Tx < N_Tx; Tx++) {
						CHART->Series[Tx]->Points->AddXY(N, gp_data[n].AM_RX_2[index_freq[chart_N]][Tx] * mV);
					}
				}

				//AM_Zero Z1 ; Z2
				else if (COMBOBOX->SelectedItem == "AM Zero ZZ") {
					CHART->ChartAreas["ChartArea1"]->AxisY->Title = "mkV";
					//CHART->ChartAreas["ChartArea1"]->AxisY->Minimum = 0;
					float buff0_msd = gp_data[n].ZERO_AM_RX_1[index_freq[chart_N]] * mV*1000.0f;
					float buff1_msd = gp_data[n].ZERO_AM_RX_2[index_freq[chart_N]] * mV*1000.0f;
					sko_summ2_am_zero_zz[0] += pow((buff0_msd - sko_summ_am_zero_zz[0]), 2);
					sko_summ2_am_zero_zz[1] += pow((buff1_msd - sko_summ_am_zero_zz[1]), 2);
					CHART->Series[0]->Points->AddXY(N, buff0_msd);
					CHART->Series[1]->Points->AddXY(N, buff1_msd);
				}

				//Temp RZZ;
				else if (COMBOBOX->SelectedItem == "Temp RZZ") {
					CHART->ChartAreas["ChartArea1"]->AxisY->Title = "Celsium";
					if (gp_data[n].temperature < -50 || gp_data[n].temperature > 250)
						gp_data[n].temperature = 0;
					CHART->Series[3]->Points->AddXY(N, gp_data[n].temperature);
				}

				//PH_SMT_from_computer
				else if (COMBOBOX->SelectedItem == "PH_SMT_from_computer") {
					CHART->ChartAreas["ChartArea1"]->AxisY->Title = "mDeg";					
					if (metro_is_readed_from_sonde == true && metrology_GP_struct.signature == global_signature) {
						GP_DATA gp_data_buff = gp_data[n];
						uint16_t condition = get_condition(&gp_data_buff, &metrology_GP_struct, (uint16_t)index_freq[chart_N], mV);
						simmetry(&gp_data_buff, &metrology_GP_struct, (uint16_t)index_freq[chart_N], condition);
						for (int Tx = 0; Tx < N_Tx; Tx++) {
							CHART->Series[Tx]->Points->AddXY(N, gp_data_buff.phase_smt[index_freq[chart_N]][Tx] * mG);
						}
					}
					else {
						if (mess_box_flg_show == true) {
							MessageBox::Show("ПРОЧИТАЙТЕ ФАЙЛ МЕТРОЛОГИИ ИЗ ПРИБОРА !");
							mess_box_flg_show = false;
						}
					}

				}

				//УЭС_from_computer
				else if (COMBOBOX->SelectedItem == "УЭС_from_computer") {
					CHART->ChartAreas["ChartArea1"]->AxisY->Title = "Om.m";
					//CHART->ChartAreas["ChartArea1"]->AxisY->Minimum = 0;
					if (sonde_type == CARTOGRAPH_) {
						if (metro_is_readed_from_sonde == true && metrology_GP_struct.signature == global_signature) {
							GP_DATA gp_data_buff = gp_data[n];
							uint16_t condition = get_condition(&gp_data_buff, &metrology_GP_struct, (uint16_t)(index_freq[chart_N]), mV);
							simmetry(&gp_data_buff, &metrology_GP_struct, (uint16_t)index_freq[chart_N], condition);

							for (int Tx = 0; Tx < N_Tx; Tx++) {
								RO_ARG(&metrology_GP_struct, &gp_data_buff, Tx, (uint16_t)index_freq[chart_N]);
								CHART->Series[Tx]->Points->AddXY(N, gp_data_buff.rho_smt[index_freq[chart_N]][Tx]);
							}
						}
						else {
							if (mess_box_flg_show == true) {
								MessageBox::Show("ПРОЧИТАЙТЕ ФАЙЛ МЕТРОЛОГИИ ИЗ ПРИБОРА !");
								mess_box_flg_show = false;
							}
						}
					}

				}

				//PH_SMT_from_sonde
				else if (COMBOBOX->SelectedItem == "PH_SMT_from_sonde") {
					CHART->ChartAreas["ChartArea1"]->AxisY->Title = "mDeg";
					for (int Tx = 0; Tx < N_Tx; Tx++) {
						CHART->Series[Tx]->Points->AddXY(N, gp_data[n].phase_smt[index_freq[chart_N]][Tx] * mG);
					}
					if (chart_N == 0) {
						toolStripMenuItemT1->Visible = true;
						toolStripMenuItemT2->Visible = true;
						toolStripMenuItemT3->Visible = true;
						toolStripMenuItemT4->Visible = true;
						toolStripMenuItemT5->Visible = true;
					}

				}

				//УЭС_from_sonde
				else if (COMBOBOX->SelectedItem == "УЭС_from_sonde") {
					CHART->ChartAreas["ChartArea1"]->AxisY->Title = "Om.m";
					//CHART->ChartAreas["ChartArea1"]->AxisY->Minimum = 0;
					for (int Tx = 0; Tx < N_Tx; Tx++) {
						CHART->Series[Tx]->Points->AddXY(N, gp_data[n].rho_smt[index_freq[chart_N]][Tx]);
					}
				}

				//УЭС_error
				else if (COMBOBOX->SelectedItem == "Погрешность УЭС") {

					if (metro_is_readed_from_sonde == true && metrology_GP_struct.signature == global_signature) {
						// забираем симметризованные фазы из прибора в буфер
						GP_DATA gp_data_buff = gp_data[n];
						for (int Tx = 0; Tx < N_Tx; Tx++) {
							//сдвигаем фазы на УЭС, заданное ползунком
							gp_data_buff.phase_smt[index_freq[chart_N]][Tx] += ph_shift.d_fi[index_freq[chart_N]][Tx];
							//считаем УЭС с фазовым сдвигом
							RO_ARG(&metrology_GP_struct, &gp_data_buff, Tx, (uint16_t)index_freq[chart_N]);
							CHART->Series[Tx]->Points->AddXY(N, gp_data_buff.rho_smt[index_freq[chart_N]][Tx]);
						}
					}

					else {
						if (mess_box_flg_show == true) {
							MessageBox::Show("ПРОЧИТАЙТЕ ФАЙЛ МЕТРОЛОГИИ ИЗ ПРИБОРА !");
							mess_box_flg_show = false;
						}
					}


				}

				//FRAME
				else if (COMBOBOX->SelectedItem == "FRAME") {
					CHART->ChartAreas["ChartArea1"]->AxisY->Title = "points";
					//CHART->ChartAreas["ChartArea1"]->AxisY->Minimum = 0;
					CHART->Series[0]->Points->AddXY(N, gp_data[n].frame);

				}
				///////////////////////////////////////////////////////////////////////////////////////////////

				if (sonde_type == CARTOGRAPH_) {
					//L"AM_GEO" 1
					if (COMBOBOX->SelectedItem == "AM_GEO") {
						CHART->ChartAreas["ChartArea1"]->AxisY->Title = "units";
						if (checkBox_sonde_calk->Checked == false) {
							RAW_DATA raw; GEO result; ;
							//для двух приемников и одной частоты index_freq на графике chart_N
							raw = all_data[N].raw_data[index_freq[chart_N]];
							//считаем геосигналы для двух направленных приемников
							if (checkBox_stable_ph->Checked == true)
								calk_geo_sph(&raw, &result, fourier_k, index_rzx[chart_N]);
							if (checkBox_stable_ph->Checked == false)
								calk_geo_rph(&raw, &result, fourier_k, index_rzx[chart_N]);

							for (int Tx = 0; Tx < N_Tx; Tx++) {
								CHART->Series[Tx]->Points->AddXY(N, abs(result.geo[index_rzx[chart_N]][Tx]));
							}
						}
						if (checkBox_sonde_calk->Checked == true) {
							for (int Tx = 0; Tx < N_Tx; Tx++) {
								CHART->Series[Tx + 5]->Points->AddXY(N,
									abs(all_data[N].data[index_freq[chart_N]].R_zx[index_rzx[chart_N]].Geo[Tx]));
							}
						}
					}

					//L"PH_GEO" 2
					else if (COMBOBOX->SelectedItem == "PH_GEO") {
						CHART->ChartAreas["ChartArea1"]->AxisY->Title = "mDeg";
						if (checkBox_sonde_calk->Checked == false) {
							RAW_DATA raw; GEO result; ;
							//для двух приемников и одной частоты index_freq на графике chart_N
							raw = all_data[N].raw_data[index_freq[chart_N]];
							//считаем геосигналы для двух направленных приемников
							if (checkBox_stable_ph->Checked == true)
								calk_geo_sph(&raw, &result, fourier_k, index_rzx[chart_N]);
							if (checkBox_stable_ph->Checked == false)
								calk_geo_rph(&raw, &result, fourier_k, index_rzx[chart_N]);

							for (int Tx = 0; Tx < N_Tx; Tx++) {
								CHART->Series[Tx]->Points->AddXY(N, arg(result.geo[index_rzx[chart_N]][Tx]));
							}
						}
						if (checkBox_sonde_calk->Checked == true) {
							for (int Tx = 0; Tx < N_Tx; Tx++) {
								CHART->Series[Tx + 5]->Points->AddXY(N,
									arg(all_data[N].data[index_freq[chart_N]].R_zx[index_rzx[chart_N]].Geo[Tx]));
							}
						}
					}

					//L"AM_V_ZX" 3
					else if (COMBOBOX->SelectedItem == "AM_V_ZX") {
						CHART->ChartAreas["ChartArea1"]->AxisY->Title = "mV";
						if (checkBox_sonde_calk->Checked == false) {
							RAW_DATA raw; GEO result;
							//для двух приемников и одной частоты index_freq на графике chart_N
							raw = all_data[N].raw_data[index_freq[chart_N]];
							//считаем геосигналы для двух направленных приемников
							if (checkBox_stable_ph->Checked == true)
								calk_geo_sph(&raw, &result, fourier_k, index_rzx[chart_N]);
							if (checkBox_stable_ph->Checked == false)
								calk_geo_rph(&raw, &result, fourier_k, index_rzx[chart_N]);
							for (int Tx = 0; Tx < N_Tx; Tx++) {
								CHART->Series[Tx]->Points->AddXY(N, result.amp_zx[index_rzx[chart_N]][Tx]);
							}
						}
						if (checkBox_sonde_calk->Checked == true) {
							for (int Tx = 0; Tx < N_Tx; Tx++) {
								CHART->Series[Tx + 5]->Points->AddXY(N,
									all_data[N].data[index_freq[chart_N]].R_zx[index_rzx[chart_N]].amp_Vzx[Tx]);
							}
						}
					}

					//L"AM_V_ZZ" 4
					else if (COMBOBOX->SelectedItem == "AM_V_ZZ") {
						CHART->ChartAreas["ChartArea1"]->AxisY->Title = "mV";
						if (checkBox_sonde_calk->Checked == false) {
							RAW_DATA raw; GEO result; ;
							//для двух приемников и одной частоты index_freq на графике chart_N
							raw = all_data[N].raw_data[index_freq[chart_N]];
							//считаем геосигналы для двух направленных приемников
							if (checkBox_stable_ph->Checked == true)
								calk_geo_sph(&raw, &result, fourier_k, index_rzx[chart_N]);
							if (checkBox_stable_ph->Checked == false)
								calk_geo_rph(&raw, &result, fourier_k, index_rzx[chart_N]);

							for (int Tx = 0; Tx < N_Tx; Tx++) {
								CHART->Series[Tx]->Points->AddXY(N, result.amp_zz[index_rzx[chart_N]][Tx]);
							}
						}
						if (checkBox_sonde_calk->Checked == true) {
							for (int Tx = 0; Tx < N_Tx; Tx++) {
								CHART->Series[Tx + 5]->Points->AddXY(N,
									all_data[N].data[index_freq[chart_N]].R_zx[index_rzx[chart_N]].amp_Vzz[Tx]);
							}
						}
					}

					//"PH_VZX / VZZ" 5
					else if (COMBOBOX->SelectedItem == "PH_VZX / VZZ") {
						CHART->ChartAreas["ChartArea1"]->AxisY->Title = "mDeg";
						if (checkBox_sonde_calk->Checked == false) {
							RAW_DATA raw; GEO result; ;
							//для двух приемников и одной частоты index_freq на графике chart_N
							raw = all_data[N].raw_data[index_freq[chart_N]];
							//считаем геосигналы для двух направленных приемников
							if (checkBox_stable_ph->Checked == true)
								calk_geo_sph(&raw, &result, fourier_k, index_rzx[chart_N]);
							if (checkBox_stable_ph->Checked == false)
								calk_geo_rph(&raw, &result, fourier_k, index_rzx[chart_N]);

							for (int Tx = 0; Tx < N_Tx; Tx++) {
								CHART->Series[Tx]->Points->AddXY(N, result.arg_zx_zz[index_rzx[chart_N]][Tx] * mG);
							}
						}
						if (checkBox_sonde_calk->Checked == true) {
							for (int Tx = 0; Tx < N_Tx; Tx++) {
								CHART->Series[Tx + 5]->Points->AddXY(N,
									all_data[N].data[index_freq[chart_N]].R_zx[index_rzx[chart_N]].ph_Vzx_Vzz[Tx] * mG);
							}
						}
					}

					//L"betta_z" 6
					else if (COMBOBOX->SelectedItem == L"betta_z") {
						CHART->ChartAreas["ChartArea1"]->AxisY->Title = "units";
						RAW_DATA raw; GEO result; ;
						//для двух приемников и одной частоты index_freq на графике chart_N
						raw = all_data[N].raw_data[index_freq[chart_N]];
						//считаем геосигналы для двух направленных приемников
						if (checkBox_stable_ph->Checked == true)
							calk_geo_sph(&raw, &result, fourier_k, index_rzx[chart_N]);
						if (checkBox_stable_ph->Checked == false)
							calk_geo_rph(&raw, &result, fourier_k, index_rzx[chart_N]);

						for (int Tx = 0; Tx < N_Tx; Tx++) {
							CHART->Series[Tx]->Points->AddXY(N, result.betta_z[index_rzx[chart_N]][Tx]);
						}
					}

					//L"AM_V_XX" 7
					else if (COMBOBOX->SelectedItem == "AM_V_XX") {
						CHART->ChartAreas["ChartArea1"]->AxisY->Title = "mV";
						if (checkBox_sonde_calk->Checked == false) {
							RAW_DATA raw; GEO result; ;
							//для двух приемников и одной частоты index_freq на графике chart_N
							raw = all_data[N].raw_data[index_freq[chart_N]];
							//считаем геосигналы для двух направленных приемников
							if (checkBox_stable_ph->Checked == true)
								calk_geo_sph(&raw, &result, fourier_k, index_rzx[chart_N]);
							if (checkBox_stable_ph->Checked == false)
								calk_geo_rph(&raw, &result, fourier_k, index_rzx[chart_N]);

							for (int Tx = 0; Tx < N_Tx; Tx++) {
								CHART->Series[Tx]->Points->AddXY(N, result.amp_xx[index_rzx[chart_N]][Tx]);
							}
						}
						if (checkBox_sonde_calk->Checked == true) {
							for (int Tx = 0; Tx < N_Tx; Tx++) {
								CHART->Series[Tx + 5]->Points->AddXY(N, 0);
							}
						}
					}

					//L"dV" разброс по 16 точкам 8
					else if (COMBOBOX->SelectedItem == "dV") {
						CHART->ChartAreas["ChartArea1"]->AxisY->Title = "units";
						RAW_DATA raw; GEO result; ;
						//для двух приемников и одной частоты index_freq на графике chart_N
						raw = all_data[N].raw_data[index_freq[chart_N]];
						//считаем геосигналы для двух направленных приемников
						if (checkBox_stable_ph->Checked == true)
							calk_geo_sph(&raw, &result, fourier_k, index_rzx[chart_N]);
						if (checkBox_stable_ph->Checked == false)
							calk_geo_rph(&raw, &result, fourier_k, index_rzx[chart_N]);

						for (int Tx = 0; Tx < N_Tx; Tx++) {
							CHART->Series[Tx]->Points->AddXY(N, result.amp_zx[index_rzx[chart_N]][Tx] / sqrt(result.dv[index_rzx[chart_N]][Tx]));
						}
					}

					//L"SNR" разброс по 16 точкам 9
					else if (COMBOBOX->SelectedItem == "SNR") {
						CHART->ChartAreas["ChartArea1"]->AxisY->Title = "units";
						RAW_DATA raw; GEO result; ;
						//для двух приемников и одной частоты index_freq на графике chart_N
						raw = all_data[N].raw_data[index_freq[chart_N]];
						//считаем геосигналы для двух направленных приемников
						if (checkBox_stable_ph->Checked == true)
							calk_geo_sph(&raw, &result, fourier_k, index_rzx[chart_N]);
						if (checkBox_stable_ph->Checked == false)
							calk_geo_rph(&raw, &result, fourier_k, index_rzx[chart_N]);

						for (int Tx = 0; Tx < N_Tx; Tx++) {
							CHART->Series[Tx]->Points->AddXY(N, result.amp_zz[index_rzx[chart_N]][Tx] / result.msd[index_rzx[chart_N]][Tx]);
						}
					}

					//L"AM Zero ZX" 10
					else if (COMBOBOX->SelectedItem == "AM Zero ZX") {
						float buff_msd = abs(all_data[N].data[index_freq[chart_N]].R_zx[index_rzx[chart_N]].Tx_0)*mV*1000.0f;
						sko_summ2_am_zero_zx += pow((buff_msd - sko_summ_am_zero_zx), 2);
						CHART->ChartAreas["ChartArea1"]->AxisY->Title = "mkV";
						CHART->Series[0]->Points->AddXY(N, buff_msd);
						//cout << real(all_data[N].data[index_freq[chart_N]].R_zx[index_rzx[chart_N]].Tx_0) << " " 
						//     << imag(all_data[N].data[index_freq[chart_N]].R_zx[index_rzx[chart_N]].Tx_0) << endl;
					}

					//L"AM_GEO_SMT",// 11
					if (COMBOBOX->SelectedItem == "AM_GEO_SMT") {
						CHART->ChartAreas["ChartArea1"]->AxisY->Title = "dB";
						checkBox_sonde_calk->Text = "tele";


						DECOMPRESSED_1freq d_data[2] = { 0, };
						METROLOGY_CARTOGRAPH metro;
						DATA data_for_decompress[2];
						metro.reap = reap;
						metro.select_key = global_selected_key;
						bool work_400 = false; bool work_2000 = false; uint8_t write_arr_freq_idx = 0;
						if ((global_selected_key >> Work_400) & 1) work_400 = true;
						if ((global_selected_key >> Work_2000) & 1) work_2000 = true;
						if (work_400 == true && work_2000 == true) { write_arr_freq_idx = _2000_kGz; }//работают обе частоты
						else if (work_400 == true && work_2000 == false) { write_arr_freq_idx = _400_kGz; }//работает только 400
						else if (work_400 == false && work_2000 == true) { write_arr_freq_idx = _2000_kGz; }//работает только 2000

						receive_compressed_data(&d_data[0], &metro, all_data[N].data[write_arr_freq_idx].out_arr);

						//??????????????????????????????почему нули////////////////////////
						/*
						cout << "out_arr " << endl;
						for (int n = 0; n < 32; n++)
							cout << bitset<8>(all_data[N].data[write_arr_freq_idx].out_arr[n]) << " ";
						cout << endl;
						cout << endl;
						*/
						if (checkBox_sonde_calk->Checked == false) {
							for (int Tx = 0; Tx < N_Tx; Tx++) {
								if (!isinf(all_data[N].data[index_freq[chart_N]].ATT_dB_geo_signal_smt[Tx]))
									CHART->Series[Tx]->Points->AddXY(N,
										all_data[N].data[index_freq[chart_N]].ATT_dB_geo_signal_smt[Tx]);
								else
									CHART->Series[Tx]->Points->AddXY(N, 0);
							}


						}
						if (checkBox_sonde_calk->Checked == true) {
							for (int Tx = 0; Tx < N_Tx; Tx++) {
								CHART->Series[Tx + 5]->Points->AddXY(N,
									d_data[index_freq[chart_N]].GA[Tx]);
							}
						}
					}
					//L"PH_GEO_SMT",// 12
					if (COMBOBOX->SelectedItem == "PH_GEO_SMT") {
						CHART->ChartAreas["ChartArea1"]->AxisY->Title = "deg";
						if (checkBox_sonde_calk->Checked == false) {
							for (int Tx = 0; Tx < N_Tx; Tx++) {
								CHART->Series[Tx]->Points->AddXY(N,
									all_data[N].data[index_freq[chart_N]].PH_deg_geo_signal_smt[Tx]);
							}
						}
						if (checkBox_sonde_calk->Checked == true) {
							for (int Tx = 0; Tx < N_Tx; Tx++) {
								CHART->Series[Tx + 5]->Points->AddXY(N,
									all_data[N].data[index_freq[chart_N]].PH_deg_geo_signal_smt[Tx]);
							}
						}
					}

					//L"BORDER_ANGLE",// 13
					else if (COMBOBOX->SelectedItem == "BORDER_ANGLE") {
						CHART->ChartAreas["ChartArea1"]->AxisY->Title = "mDeg";
						RAW_DATA raw; GEO result; ;
						//для двух приемников и одной частоты index_freq на графике chart_N
						raw = all_data[N].raw_data[index_freq[chart_N]];
						//считаем геосигналы для двух направленных приемников
						if (checkBox_stable_ph->Checked == true)
							calk_geo_sph(&raw, &result, fourier_k, index_rzx[chart_N]);
						if (checkBox_stable_ph->Checked == false)
							calk_geo_rph(&raw, &result, fourier_k, index_rzx[chart_N]);

						for (int Tx = 0; Tx < N_Tx; Tx++) {
							CHART->Series[Tx]->Points->AddXY(N, result.border_angle[index_rzx[chart_N]][Tx] * mG);
						}
					}

					//L"INC Abs; Zen; Azm",// 14
					else if (COMBOBOX->SelectedItem == "INC Abs; Zen; Azm") {
						CHART->ChartAreas["ChartArea1"]->AxisY->Title = "Deg";
						CHART->Series[0]->Points->AddXY(N, all_data[N].data[index_freq[chart_N]].INC.Aps);
						CHART->Series[1]->Points->AddXY(N, all_data[N].data[index_freq[chart_N]].INC.Zen);
						CHART->Series[2]->Points->AddXY(N, all_data[N].data[index_freq[chart_N]].INC.Azm);
					}

					//L"ANGULAR_VELOCITY",// 15
					else if (COMBOBOX->SelectedItem == "ANGULAR_VELOCITY") {
						CHART->ChartAreas["ChartArea1"]->AxisY->Title = "min-1";
						CHART->Series[0]->Points->AddXY(N, all_data[N].data[index_freq[chart_N]].Wg);
					}
					//L"START_SECTOR",// 16
					else if (COMBOBOX->SelectedItem == "START_SECTOR") {
						CHART->ChartAreas["ChartArea1"]->AxisY->Title = "N";
						CHART->Series[0]->Points->AddXY(N, all_data[N].data[index_freq[chart_N]].start_sector);
					}
					//L"Temp RZX",// 17
					else if (COMBOBOX->SelectedItem == "Temp RZX") {
						CHART->ChartAreas["ChartArea1"]->AxisY->Title = "Celsium";
						//CHART->Series[0]->Points->AddXY(N, all_data[N].data[index_freq[chart_N]].R_zx[index_rzx[chart_N]].temperature);
						CHART->Series[0]->Points->AddXY(N, all_data[N].data[index_freq[chart_N]].depth);
					}
					//"FRAME",// 18
				}


			}

			//for sko///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			if (COMBOBOX->SelectedItem == "AM Zero ZZ") {
				sko_summ2_am_zero_zz[0] /= N_;
				sko_summ2_am_zero_zz[1] /= N_;
				sko_summ2_am_zero_zz[0] = sqrt(sko_summ2_am_zero_zz[0]);
				sko_summ2_am_zero_zz[1] = sqrt(sko_summ2_am_zero_zz[1]);
				TEXTBOX->Text = "R1=" + (sko_summ2_am_zero_zz[0]).ToString("F1") + "; R2=" + (sko_summ2_am_zero_zz[1]).ToString("F1") + " mkV";
			}

			if (COMBOBOX->SelectedItem == "AM Zero ZX") {
				sko_summ2_am_zero_zx /= N_;
				sko_summ2_am_zero_zx = sqrt(sko_summ2_am_zero_zx);
				TEXTBOX->Text = (sko_summ2_am_zero_zx).ToString("F1") + " mkV";
			}

			progressBar1->Visible = false;

		}
			
		}

	void do_raw_chart(int chart_N, int data_N) {
			extern complex <float> Fourier_Table[2][16];
			Chart^ CHART = (Chart^)this->tableLayoutPanel1->Controls["chart" + (chart_N + 3).ToString()];
			ComboBox^ COMBOBOX;
			CheckBox^ CHECKBOX_TX;
			TableLayoutPanel^  TABLELAYOUTPANEL = (TableLayoutPanel^)this->tableLayoutPanel1->Controls["tableLayoutPanel" + (chart_N + 5)];
			Button^ BUTTON = (Button^)TABLELAYOUTPANEL->Controls["button_re_im_" + (chart_N + 1)];
			Label^	LABEL = (Label^)TABLELAYOUTPANEL->Controls["label_dv" + (chart_N + 1)];
			CHART->ChartAreas["ChartArea1"]->AxisX->LabelStyle->Format = "0";
			CHART->ChartAreas["ChartArea1"]->AxisY->LabelStyle->Format = "0.00";
			TableLayoutPanel^  TABLELAYOUTPANEL1 = (TableLayoutPanel^)this->tableLayoutPanel1->Controls["tableLayoutPanel" + (chart_N + 3)];
			CheckBox^ CHECKBOX_ZOOM;
			CHECKBOX_ZOOM = (CheckBox^)TABLELAYOUTPANEL1->Controls["checkBox" + (chart_N + 8)];
			if (CHECKBOX_ZOOM->Checked == false
				&& CHART->ChartAreas["ChartArea1"]->AxisX->ScaleView->IsZoomed
				) {
				CHART->ChartAreas["ChartArea1"]->AxisX->ScaleView->ZoomReset(0);
				CHART->ChartAreas["ChartArea1"]->AxisY->ScaleView->ZoomReset(0);
			}

			CHART->ChartAreas["ChartArea1"]->CursorX->IsUserSelectionEnabled = false;

			if (index_rzx[chart_N] == Left_) {//"R_Zx L"
				CHART->Series[0]->LegendText = L"T1";
				CHART->Series[1]->LegendText = L"T2";
				CHART->Series[2]->LegendText = L"T4";
				CHART->Series[3]->LegendText = L"T5";

				CHART->Series[0]->Color = System::Drawing::Color::Green;
				CHART->Series[1]->Color = System::Drawing::Color::Chocolate;
				CHART->Series[2]->Color = System::Drawing::Color::Red;
				CHART->Series[3]->Color = System::Drawing::Color::Blue;

				CHART->Series[4]->Color = System::Drawing::Color::Green;
				CHART->Series[5]->Color = System::Drawing::Color::Chocolate;
				CHART->Series[6]->Color = System::Drawing::Color::Red;
				CHART->Series[7]->Color = System::Drawing::Color::Blue;
			}
			else if (index_rzx[chart_N] == Right_) {//"R_Zx R"
				CHART->Series[0]->LegendText = L"T1";
				CHART->Series[1]->LegendText = L"T2";
				CHART->Series[2]->LegendText = L"T3";
				CHART->Series[3]->LegendText = L"T5";

				CHART->Series[0]->Color = System::Drawing::Color::Chocolate;
				CHART->Series[1]->Color = System::Drawing::Color::Green;
				CHART->Series[2]->Color = System::Drawing::Color::Red;
				CHART->Series[3]->Color = System::Drawing::Color::Blue;

				CHART->Series[4]->Color = System::Drawing::Color::Chocolate;
				CHART->Series[5]->Color = System::Drawing::Color::Green;
				CHART->Series[6]->Color = System::Drawing::Color::Red;
				CHART->Series[7]->Color = System::Drawing::Color::Blue;
			}

			for (int i = 0; i < 12; i++) {
				CHART->Series[i]->Points->Clear();
			}

			while (busy == true) { ; }

			if ((data_N > min_n[chart_N]) && (data_N < max_n[chart_N])) {
				extern complex <float> fourier_k[3][16];
				for (int Tx = 0; Tx < N_Tx; Tx++) {
					CHECKBOX_TX = (CheckBox^)TABLELAYOUTPANEL->Controls["checkBoxT" + (chart_N + 1) + (Tx + 1)];
					if (CHECKBOX_TX->Checked == true) {
						//cout << "chart " << chart_N + 1 << " Tx " << Tx + 1 << endl;
						RAW_DATA raw; GEO result;
						//для двух приемников и одной частоты index_freq на графике chart_N
						raw = all_data[data_N].raw_data[index_freq[chart_N]];
						//считаем геосигналы для двух направленных приемников
						calk_geo_rph(&raw, &result, fourier_k, Right_);
						calk_geo_rph(&raw, &result, fourier_k, Left_);
						//выводим 
						for (int n = 0; n < 16; n++) {
							if (checkBox_stable_ph->Checked == false) {
								//квадраты амплитуд 16 ти секторов 
								if (index_amp[chart_N] == 0) {
									CHART->Series[Tx]->Points->AddXY(n, result.amp_2[index_rzx[chart_N]][Tx][n]);
									//апроксимация - сумма трех гармоник от квадрата амплитуды
									int Rzx = index_rzx[chart_N];
									float approx =
										abs(result.harm[Rzx][Tx][0]) +
										abs(result.harm[Rzx][Tx][1])*cos(PI*n / 8.0f - arg(result.harm[Rzx][Tx][1])) +
										abs(result.harm[Rzx][Tx][2])*cos(2.0f*PI*n / 8.0f - arg(result.harm[Rzx][Tx][2]));
									CHART->Series[Tx + 4]->Points->AddXY(n, approx);
								}

								//  амплитуды 16 ти секторов
								if (index_amp[chart_N] == 1) {
									CHART->Series[Tx]->Points->AddXY(n, sqrt(result.amp_2[index_rzx[chart_N]][Tx][n]));
									//апроксимация - сумма Vzx* и Vzz
									int Rzx = index_rzx[chart_N];
									float fi = result.border_angle[Rzx][Tx];
									float fi1_amp = arg(result.harm_amp[Rzx][Tx][1]);
									float fi2_amp = arg(result.harm_amp[Rzx][Tx][2]);
									//float approx = result.amp_zz[Rzx][Tx] + cos(PI*n / 8.0f - fi)*result.amp_zx[Rzx][Tx];
									float approx_amp1 = abs(result.harm_amp[Rzx][Tx][0])
										+ cos(PI*n / 8.0f - fi1_amp)*abs(result.harm_amp[Rzx][Tx][1])
										//+ cos(2.0f*PI*n / 8.0f - fi2_amp)*abs(result.harm_amp[Rzx][Tx][2])
										;
									CHART->Series[Tx + 4]->Points->AddXY(n, approx_amp1);
									//апроксимация фурье по АМПЛИТУДЕ
									float approx_amp2 = abs(result.harm_amp[Rzx][Tx][0])
										+ cos(PI*n / 8.0f - fi1_amp)*abs(result.harm_amp[Rzx][Tx][1])
										+ cos(2.0f*PI*n / 8.0f - fi2_amp)*abs(result.harm_amp[Rzx][Tx][2])
										;
									CHART->Series[Tx + 8]->Points->AddXY(n, approx_amp2);
								}
							}

							if (checkBox_stable_ph->Checked == true) {
								int Rzx = index_rzx[chart_N];
								if (checkBox_re_im->Checked == false) {//показываем амплитуду и фазу						
									if (index_amp[chart_N] == 0) {//amp
										CHART->Series[Tx + 4]->Points->AddXY(n, mV*sqrt(pow(raw.R_zx[Rzx][Tx].sector[Im][n], 2) + pow(raw.R_zx[Rzx][Tx].sector[Re][n], 2)));
									}
									if (index_amp[chart_N] == 1) {//Ph
										CHART->Series[Tx + 4]->Points->AddXY(n, mG*atan2(raw.R_zx[Rzx][Tx].sector[Im][n], raw.R_zx[Rzx][Tx].sector[Re][n]));
									}
								}
								else if (checkBox_re_im->Checked == true) {//показываем I Q	и аппроксимацию					
									if (index_amp[chart_N] == 0) {//I
										CHART->Series[Tx]->Points->AddXY(n, raw.R_zx[Rzx][Tx].sector[Re][n] * mV);
									}
									if (index_amp[chart_N] == 1) {//Q
										CHART->Series[Tx]->Points->AddXY(n, raw.R_zx[Rzx][Tx].sector[Im][n] * mV);
									}




								}
							}

						}
						//аппроксимация
					}
				}
			}




		}
	
	private: System::Void data_Click(System::Object^  sender, System::EventArgs^  e) {
		//Открываем файл и записываем данные в вектор для вывода на графики и анализа
		//Существует три расширения .bin; .biv; .DEV; 
		//для bin и biv сигнатура занимает 4 байта,  начиная с нулевого
		//для DEV сигнатура занимает 4 байта,  начиная с одиннадцатого
		//Существует 4 типа приборов LWD_4Tx 241; CARTOGRAPH 351; AUTONOM_5Tx 151; AUTONOM_5Tx_SDR; 152
		//Открываем файл и в зависимости от расширения читаем сигнатуру
		//

		DATA_FILE_PARAM param;
		ifstream fin;
		param.fin = &fin;
		bool  res;
		//file_frames = 1;

		textBox_min1->Clear();
		textBox_min2->Clear();
		textBox_max1->Clear();
		textBox_max2->Clear();

		if (surf_on_off == true) {
			this->timer_read_file->Enabled = false;
			surf_on_off = false;
			this->buttonSURF->BackColor = System::Drawing::Color::Gainsboro;//выключаем таймер
			ToolStripMenuItem_save_file->Visible = true;
		}

		openFileDialog->Filter = "bin files   (*.bin; *.ifn; *.biv; *.DEV; *.las;)|*.bin; *.ifn; *.biv; *.DEV; *.las;";
		//если открываем ранее запомненый файл
		if (was_save == true) {
			was_save = false;
			Data_file_name = Dataout_for_save.c_str();
			param.extension = extension_for_save;
			param.Data_file_name = Data_file_name;
			fin.open(Data_file_name, ios::binary);
			if (!fin) {
				res = false;
			}
			else res = true;
			fin.close();
		}

		else if (temporary_file == true) {
			temporary_file = false;
			param.extension = ".bin";
			param.Data_file_name = Data_file_name;
			fin.open(Data_file_name, ios::binary);
			if (!fin) {
				res = false;
			}
			else res = true;
			fin.close();
		}

		else {
			if (openFileDialog->ShowDialog() == System::Windows::Forms::DialogResult::OK) {
				param.Data_file_name = (char*)Marshal::StringToHGlobalAnsi(openFileDialog->FileName).ToPointer();
				Data_file_name = param.Data_file_name;
				param.extension = (char*)Marshal::StringToHGlobalAnsi(Path::GetExtension(openFileDialog->FileName)).ToPointer();
				res = true;
			}
			else res = false;
		}

		//если файл открыт
		if (res == true) {

			//если не включено добавление следующего файла в конец предыдущего
			if (checkBox_add_file->Checked == false) {
				all_data.clear();
				//_4Tx_data.clear();
				//_5Tx_data.clear();
				gp_data.clear();
			}

			Text = L"DATA " + gcnew String(Data_file_name);
			fin.open(Data_file_name, ios::binary);
			define_sonde_type(&param);
			fin.close();

			sonde_type = param.sonde_type;
			global_signature = param.signature;


			if (param.sonde_type == CARTOGRAPH_) {
				toolStripComboBox_sonde_type->SelectedItem = "CARTOGRAPH";
			}

			if (param.sonde_type == LWD_4Tx_) {
				toolStripComboBox_sonde_type->SelectedItem = "LWD_4Tx";
			}

			if (param.sonde_type == AUTONOM_5Tx_) {
				toolStripComboBox_sonde_type->SelectedItem = "AUTONOM_5Tx";
			}

			if (param.sonde_type == AUTONOM_5Tx_SDR_) {
				toolStripComboBox_sonde_type->SelectedItem = "AUTONOM_5Tx_SDR";
			}


			// чтение блоков из файла 
			fin.open(Data_file_name, ios::binary);
			for (int n = 0; n < param.file_frames; n++) {// количество циклов n, равное кол-ву кадров 

				variant<_4Tx_DATA, _5Tx_DATA, ALL_DATA > current_tool_data_buff;
				GP_DATA gp_data_buff;
				ALL_DATA all_data_buff;
				current_tool_data_buff = read_blok(&param, n);

				gp_data_buff = transform_data(current_tool_data_buff, &all_data_buff);
				gp_data.push_back(gp_data_buff);

				if (param.sonde_type == CARTOGRAPH_) {
					all_data.push_back(all_data_buff);
				}


			}

			fin.close();
			// конец чтения блоков из файла///////////////////////////////////////////////////////////////

			//заполняем для графиков/////////////////////////////////////////////
			for (int i = 0; i < 2; i++)
				min_n[i] = 0, max_n[i] = gp_data.size();
							
			cout << "all_data.size " << all_data.size() << endl;
			cout << "GP_data.size " << gp_data.size() << endl;
			cout << std::dec;

			do_chart(0);
			do_chart(1);

		}

	}

	private: System::Void renew(System::Object^  sender, System::EventArgs^  e) {
		DATA_FILE_PARAM param;
		if (busy == false) {
			if (surf_on_off == true) {
				this->timer_read_file->Enabled = false;
				surf_on_off = false;
				this->buttonSURF->BackColor = System::Drawing::Color::Gainsboro;//выключаем таймер	
			}

			if (sonde_on_off == true) {
				Data_file_name = Dataout.c_str();
				param.extension = ".bin";
			}

			if (Data_file_name == "err")MessageBox::Show("Сначала откройте файл данных !");
			else {
				ifstream fin;
				param.fin = &fin;

				fin.open(Data_file_name, ios::binary);
				define_sonde_type(&param);
				fin.close();

				sonde_type = param.sonde_type;
				global_signature = param.signature;

				if (param.sonde_type == CARTOGRAPH_) {
					toolStripComboBox_sonde_type->SelectedItem = "CARTOGRAPH";
				}

				if (param.sonde_type == LWD_4Tx_) {
					toolStripComboBox_sonde_type->SelectedItem = "LWD_4Tx";
				}

				if (param.sonde_type == AUTONOM_5Tx_) {
					toolStripComboBox_sonde_type->SelectedItem = "AUTONOM_5Tx";
				}

				if (param.sonde_type == AUTONOM_5Tx_SDR_) {
					toolStripComboBox_sonde_type->SelectedItem = "AUTONOM_5Tx_SDR";
				}

				all_data.clear();
				//_4Tx_data.clear();
				gp_data.clear();

				// чтение блоков из файла 
				fin.open(Data_file_name, ios::binary);
				for (int n = 0; n < param.file_frames; n++) {// количество циклов n, равное кол-ву кадров 
					variant<_4Tx_DATA, _5Tx_DATA, ALL_DATA > current_tool_data_buff;
					GP_DATA gp_data_buff;
					ALL_DATA all_data_buff;
					current_tool_data_buff = read_blok(&param, n);
					gp_data_buff = transform_data(current_tool_data_buff, &all_data_buff);
					gp_data.push_back(gp_data_buff);
					if (param.sonde_type == CARTOGRAPH_) {
						all_data.push_back(all_data_buff);
					}

				}
				fin.close();
				// конец чтения блоков из файла///////////////////////////////////////////////////////////////

				textBox_max1->Clear();
				textBox_max2->Clear();

				for (int i = 0; i < 2; i++)
					min_n[i] = 0, max_n[i] = gp_data.size();

				do_chart(0);
				do_chart(1);


			}
		}
	}

	private: System::Void Surf(System::Object^  sender, System::EventArgs^  e) {
		if (Data_file_name == "err")MessageBox::Show("Нехуй по кнопкам просто так пиздить !");
		else {
			this->timer_read_file->Interval = surf_interval;

			if (surf_on_off == true) {
				chart1->ChartAreas["ChartArea1"]->CursorX->IsUserSelectionEnabled = true;
				chart2->ChartAreas["ChartArea1"]->CursorX->IsUserSelectionEnabled = true;
				this->timer_read_file->Enabled = false, surf_on_off = false, this->buttonSURF->BackColor = System::Drawing::Color::Gainsboro;//выключаем таймер 
				ToolStripMenuItem_save_file->Visible = true;
			}
			else {
				chart1->ChartAreas["ChartArea1"]->CursorX->IsUserSelectionEnabled = false;
				chart2->ChartAreas["ChartArea1"]->CursorX->IsUserSelectionEnabled = false;
				this->timer_read_file->Enabled = true, surf_on_off = true, this->buttonSURF->BackColor = System::Drawing::Color::Red; //включаем таймер 	
				ToolStripMenuItem_save_file->Visible = false;
			}
		}
	}
    
			 //timer чтения из файла(surf)
	private: System::Void timer_read_file_Tick(System::Object^  sender, System::EventArgs^  e) {
		DATA_FILE_PARAM param;

		if (sonde_on_off == true && busy == false) {

			Data_file_name = Dataout.c_str();
			param.extension = ".bin";

			ifstream fin;
			param.fin = &fin;
			fin.open(Data_file_name, ios::binary);
			define_sonde_type(&param);
			fin.close();

			sonde_type = param.sonde_type;
			global_signature = param.signature;

			if (param.file_frames > 2) {
				all_data.clear();
				gp_data.clear();
				param.file_frames = param.file_frames - 1;

				if (param.file_frames < Points) {
					for (int i = 0; i < 2; i++)
						min_n[i] = 0, max_n[i] = param.file_frames;
				}

				else {
					for (int i = 0; i < 2; i++)
						min_n[i] = param.file_frames - Points, max_n[i] = param.file_frames;
				}

				textBox_min1->Clear();
				textBox_min2->Clear();
				textBox_max1->Clear();
				textBox_max2->Clear();


				// чтение блоков из файла 
				fin.open(Data_file_name, ios::binary);
				for (int n = 0; n < param.file_frames; n++) {// количество циклов n, равное кол-ву кадров 
					variant<_4Tx_DATA, _5Tx_DATA, ALL_DATA > current_tool_data_buff;
					GP_DATA gp_data_buff;
					ALL_DATA all_data_buff;
					current_tool_data_buff = read_blok(&param, n);
					gp_data_buff = transform_data(current_tool_data_buff, &all_data_buff);
					gp_data.push_back(gp_data_buff);
					if (param.sonde_type == CARTOGRAPH_) {
						all_data.push_back(all_data_buff);
					}

				}
				fin.close();
				// конец чтения блоков из файла///////////////////////////////////////////////////////////////

				do_chart(0);
				do_chart(1);

				//do_raw_chart(0, all_data.size()-1);
				//do_raw_chart(1, all_data.size()-1);

				label_T->Text = "";
				if(param.file_frames - 3 > 0)
				label_T->Text = "T " + gp_data[param.file_frames - 3].temperature.ToString("F1") + "C";
				if (gp_data[param.file_frames - 3].temperature > 125)  {
					PlaySound("C:\\EXP\\beep.Wav", NULL, SND_FILENAME);
				}
			}
		}

	}

	private: System::Void cursor_position(System::Object^  sender, System::Windows::Forms::DataVisualization::Charting::CursorEventArgs^  e) {
		//this->textBox1->Location = System::Drawing::Point(chart1->Cursor->Position.X - chart1->Location.X, chart1->Cursor->Position.Y - chart1->Location.Y);
		textBox_cursor1->Text = chart1->ChartAreas["ChartArea1"]->CursorX->Position.ToString("F2") + "; " + (chart1->ChartAreas["ChartArea1"]->CursorY->Position).ToString("F3");
		cursor_x_position[0] = chart1->ChartAreas["ChartArea1"]->CursorX->Position;
	}

	private: System::Void cursor_position1(System::Object^  sender, System::Windows::Forms::DataVisualization::Charting::CursorEventArgs^  e) {
		//this->textBox6->Location = System::Drawing::Point(chart2->Cursor->Position.X - chart2->Location.X, chart2->Cursor->Position.Y - chart1->Location.Y);
		textBox_cursor2->Text = chart2->ChartAreas["ChartArea1"]->CursorX->Position.ToString("F2") + "; " + (chart2->ChartAreas["ChartArea1"]->CursorY->Position).ToString("F3");
		cursor_x_position[1] = chart2->ChartAreas["ChartArea1"]->CursorX->Position;
	}

	private: System::Void min_n_change(System::Object^  sender, System::EventArgs^  e) {
		if (textBox_min1->Text != "") {
			try { min_n[0] = System::Int64::Parse(textBox_min1->Text); }
			catch (...) { ; }
		}
		if (textBox_min1->Text == "") min_n[0] = 0;
		if (min_n[0] >= all_data.size() - 1) min_n[0] = 1;
	}

	private: System::Void min_n2_change(System::Object^  sender, System::EventArgs^  e) {
		if (textBox_min2->Text != "") {
			try { min_n[1] = System::Int64::Parse(textBox_min2->Text); }
			catch (...) { ; }
		}
		if (textBox_min2->Text == "") min_n[1] = 0;
		if (min_n[1] >= all_data.size() - 1) min_n[1] = 1;
	}

	private: System::Void max_n_change(System::Object^  sender, System::EventArgs^  e) {
		if (textBox_max1->Text != "") {
			try { max_n[0] = System::Int64::Parse(textBox_max1->Text); }
			catch (...) { ; }
		}
		if (textBox_max1->Text == "") max_n[0] = all_data.size() - 1;
		if (max_n[0] >= all_data.size() - 1) max_n[0] = all_data.size() - 1;
	}

	private: System::Void max_n2_change(System::Object^  sender, System::EventArgs^  e) {
		if (textBox_max2->Text != "") {
			try { max_n[1] = System::Int64::Parse(textBox_max2->Text); }
			catch (...) { ; }
		}
		if (textBox_max2->Text == "") max_n[1] = all_data.size() - 1;
		if (max_n[1] >= all_data.size() - 1) max_n[1] = all_data.size() - 1;
	}
			 // update
	private: System::Void MyForm_Load(System::Object^  sender, System::EventArgs^  e) {
		checkBox_stable_ph->Checked = true;

		if (check_update == true) {

			remove("SONDE_TEST.exe");
			MoveFile("SONDE_TEST_NEW.exe", "SONDE_TEST.exe");


			CHAR path[MAX_PATH];
			GetModuleFileNameA(NULL, path, MAX_PATH);
			string path_str(path);
			std::cout << path_str << endl;
			path_str = path_str.substr(0, path_str.find_last_of("\\"));
			std::cout << path_str << endl;

			int version_now = 0, version_new = 0;
			string v;
			ifstream fin;
			ofstream fout;

			fin.open("version.txt", ios::in);
			fin >> v;
			if (v == "version") {
				fin >> version_now;
				cout << "version_now " << version_now << endl;
			}
			fin.close();

			HINTERNET hInet, hUrl;
			ULONG timeout = 2000; // миллисекунды.
			HRESULT res = -1;
			InternetSetOption(hInet, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
			hInet = InternetOpen(
				"Microsoft Internet Explorer",  // agent
				INTERNET_OPEN_TYPE_PRECONFIG,   // access
				NULL, NULL, 0);                 // defaults

			hUrl = InternetOpenUrl(hInet,
				"ftp://AMK_VORONEJ:DaJa1305@89.189.156.184/VORONEJ_FTP/UPDATE/SONDE_TEST",
				NULL,
				NULL,
				INTERNET_FLAG_PRAGMA_NOCACHE | INTERNET_FLAG_NO_CACHE_WRITE,
				NULL);
			InternetCloseHandle(hInet);

			cout << "hInet " << hInet << " hUrl " << hUrl << endl;
			if (hUrl != 0) {
				res = DeleteUrlCacheEntryA("ftp://89.189.156.184/VORONEJ_FTP/UPDATE/SONDE_TEST/version.txt");//вводить без пароля
				cout << "DeleteUrlCache " << res << endl;
				res = URLDownloadToFile(NULL, "ftp://AMK_VORONEJ:DaJa1305@89.189.156.184/VORONEJ_FTP/UPDATE/SONDE_TEST/version.txt", "version_url.txt", BINDF_GETNEWESTVERSION, NULL);

			}



			cout << "URLDownloadToFile " << res << endl;
			if (res == 0) {
				fin.open("version_url.txt", ios::in);
				fin >> v;
				if (v == "version")fin >> version_new;
				cout << "version_new " << version_new << endl;
				if (version_new - version_now > 0) {

					if (MessageBox::Show("Появилось обновление, скачать? ", "", MessageBoxButtons::YesNo) == System::Windows::Forms::DialogResult::Yes) {
						MessageBox::Show("В папке программы старые \r\n файлы будут заменены на новые");

						String ^download_path = gcnew System::String(path_str.c_str());
						Console::WriteLine("Path " + download_path);
						String ^download_SONDE_TESTexe;
						String ^download_version;

						const char* Download_SONDE_TESTexe;
						const char* Download_version;


						download_SONDE_TESTexe = download_path + "/SONDE_TEST_NEW.exe";
						download_version = download_path + "/version.txt";

						Download_SONDE_TESTexe = (char*)Marshal::StringToHGlobalAnsi(download_SONDE_TESTexe).ToPointer();
						Download_version = (char*)Marshal::StringToHGlobalAnsi(download_version).ToPointer();

						HRESULT res0 = URLDownloadToFile(NULL, "ftp://AMK_VORONEJ:DaJa1305@89.189.156.184/VORONEJ_FTP/UPDATE/SONDE_TEST/SONDE_TEST.exe", Download_SONDE_TESTexe, 0, NULL);
						HRESULT res1 = URLDownloadToFile(NULL, "ftp://AMK_VORONEJ:DaJa1305@89.189.156.184/VORONEJ_FTP/UPDATE/SONDE_TEST/version.txt", Download_version, 0, NULL);


						cout << "res " << res0 << " " << res1 << endl;

						if (res0 == 0 && res1 == 0) {
							MessageBox::Show("обновленные файлы скачаны в папку программы");
							check_update = false;
							version_now = version_new;
							time_t seconds = time(NULL);
							fout.open("version.txt", ios::out);
							fout << "version " << version_now << endl;
							fout << "time " << seconds << endl;
							fout.close();
							cout << "time_t " << seconds << endl;
							ShellExecute(0, 0, "SONDE_TEST_NEW.exe", 0, 0, SW_SHOW);
							exit(0);

						}
						else
							MessageBox::Show("скачивание не удалось попробуйте в следующий раз!");



					}
				}
				fin.close();

				remove("version_url.txt");
				time_t seconds = time(NULL);
				fout.open("version.txt", ios::out);
				fout << "version " << version_now << endl;
				fout << "time " << seconds << endl;
				fout.close();
				cout << "time_t " << seconds << endl;
			}
			else {
				time_t seconds, seconds_now = time(NULL);
				fin.open("version.txt", ios::in);
				fin >> v >> version_now >> v >> seconds;
				fin.close();
				if (seconds_now - seconds > 700000)
					MessageBox::Show("Давно не было информации об обновлениях проверьте подключение к интернету!");

				fout.open("version.txt", ios::out);
				fout << "version " << version_now << endl;
				fout << "time " << seconds_now << endl;
				fout.close();
			}

			//WinExec(((AnsiString)getenv("COMSPEC") + " /c ping -n 2 localhost > nul & Move /y \"C:\\Temp\\Program.exe\" \"C:\\Program.exe\" & Start \"\" \"C:\\program.exe\"").c_str(), SW_HIDE);


		}

	}

	private: System::Void MyForm_Shown(System::Object^  sender, System::EventArgs^  e) {
		//MessageBox::Show("Выберите файл -> тип зонда  !");	
		//data_Click(sender, e);
		//if (file1_is_open == false) {
		//	файлToolStripMenuItem->ShowDropDown();
		//	toolStripComboBox1->DroppedDown = true;
		//}
	}
			 //обрабатываем значение сохранить параметры 
	private: System::Void ToolStripMenuItem_parametrs_Click(System::Object^  sender, System::EventArgs^  e) {
		SONDEOKR30::MyForm1 Config;
		Config.ShowDialog();
	}

	private: System::Void form_close(System::Object^  sender, System::Windows::Forms::FormClosingEventArgs^  e) {
		cout << "closing " << endl;
		while (busy_read_from_sonde == true) {
			;
		}
		serialPort1->Close();
		Timer_get_data_from_sonde->Enabled = false;

		// загружаем в файл установки 
		if (index_dir_undir[up_chart] == undir)
			undir_selected_index[up_chart] = comboBox_choise_chart1->SelectedIndex;
		if (index_dir_undir[up_chart] == direct)
			direct_selected_index[up_chart] = comboBox_choise_chart1->SelectedIndex;

		if (index_dir_undir[down_chart] == undir)
			undir_selected_index[down_chart] = comboBox_choise_chart2->SelectedIndex;
		if (index_dir_undir[down_chart] == direct)
			direct_selected_index[down_chart] = comboBox_choise_chart2->SelectedIndex;


		fstream conf_out;
		conf_out.open(ConfName, ios::out);
		conf_out << surf_interval << endl;//1
		conf_out << Points << endl;//2
		conf_out << get_data_interval << endl;//3
		conf_out << check_update << endl;//4
		if (Com_Name == "")
			Com_Name = "COM1";
		conf_out << Com_Name << endl;//5
		if (local_file_to_ftp_upload == "")
			local_file_to_ftp_upload =
			"C:\\WORK\\repos\\DEBUG_VISUALIZATION\\TEST_SONDE_KARTOGRAF_2.0\\x64\\Release\\AIR_TEMP_1.0.exe";
		conf_out << local_file_to_ftp_upload << endl;//6
		conf_out << index_dir_undir[0] << endl;//7
		conf_out << index_dir_undir[1] << endl;//8
		conf_out << index_rzx[up_chart] << endl;//9
		conf_out << index_rzx[down_chart] << endl;//10
		conf_out << index_freq[0] << endl;//11
		conf_out << index_freq[1] << endl;//12
		conf_out << undir_selected_index[0] << endl;//13
		conf_out << undir_selected_index[1] << endl;//14
		conf_out << direct_selected_index[0] << endl;//15
		conf_out << direct_selected_index[1] << endl;//16
		conf_out << toolStripComboBox_sonde_type->SelectedIndex << endl;//17
		conf_out.close();



		//cout << "form_closed comboBox_choise_chart1->SelectedIndex " << comboBox_choise_chart1->SelectedIndex <<  endl;
	}
			 //обрабатываем выбор частот и фаз или амплитуд
	private: System::Void comboBox1_SelectedIndexChanged(System::Object^  sender, System::EventArgs^  e) {
		if (index_dir_undir[up_chart] == undir)
			undir_selected_index[up_chart] = comboBox_choise_chart1->SelectedIndex;
		if (index_dir_undir[up_chart] == direct)
			direct_selected_index[up_chart] = comboBox_choise_chart1->SelectedIndex;

		button_400_2000_1->Visible = true;
		Console::WriteLine(comboBox_choise_chart1->SelectedItem);
	}

	private: System::Void comboBox2_SelectedIndexChanged(System::Object^  sender, System::EventArgs^  e) {
		if (index_dir_undir[down_chart] == undir)
			undir_selected_index[down_chart] = comboBox_choise_chart2->SelectedIndex;
		if (index_dir_undir[down_chart] == direct)
			direct_selected_index[down_chart] = comboBox_choise_chart2->SelectedIndex;
		button_400_2000_2->Visible = true;
		Console::WriteLine(comboBox_choise_chart2->SelectedItem);
	}

	private: System::Void digit2(System::Object^  sender, System::Windows::Forms::KeyPressEventArgs^  e) {
		if (!Char::IsDigit(e->KeyChar) && !Char::IsControl(e->KeyChar)) e->Handled = true;
		if ((e->KeyChar) == '-') e->Handled = false; if ((e->KeyChar) == ',') e->Handled = false;
	}

	private: System::Void do_(System::Object^  sender, System::EventArgs^  e) {
		do_chart(0);
	}

	private: System::Void do_2(System::Object^  sender, System::EventArgs^  e) {
		do_chart(1);
	}

    // ON/OFF
	private: System::Void toolStripButton_on_off_Click(System::Object^  sender, System::EventArgs^  e) {
		ID id;
		variant <METROLOGY_CARTOGRAPH, METROLOGY_4Tx, METROLOGY_5Tx> current_metrology_struct;
		uint8_t D0 = 0;
		bool read_status = false;
		int stop = 0, stop_ = 0;

		if (sonde_on_off == 0) {
			serialPort1->Close();
			serialPort1->PortName = gcnew String(Com_Name.c_str());
			Sleep(100);
			serialPort1->BaudRate = baudrate;
			serialPort1->ReadTimeout = 3500;
			serialPort1->WriteTimeout = 2000;
			serialPort1->ReadBufferSize = 10000;
			Console::WriteLine(serialPort1->PortName);
			try { serialPort1->Open(); }
			catch (...) { stop = 1; }

			if (!stop) {
				serialPort1->DiscardOutBuffer();
				Sleep(100);
				//получить сигнатуру//////////////////////////////////////////////				
				send_to_tool(serialPort1, CMD_GET_SIGNATURE, &D0, 0);
				Sleep(2000);
				uint8_t CMD;  uint16_t readed_length = 0;  uint8_t DATA_IN[10] = { 0, };
				//int result_receive_from_sonde = receive_//om_tool_(serialPort1, CMD, DATA_IN, 6);
				int result_receive_from_sonde = receive_from_tool(serialPort1, CMD, DATA_IN, &readed_length);
				serialPort1->Close();

				//не запрашивается сигнатура - не картограф LWD_4Tx cо старой прошивкой
				if (result_receive_from_sonde != 0) {//можно принудительно установить LWD_4Tx
					MessageBox::Show("ID НЕ ПОЛУЧЕН, БУДЕТ УСТАНАВЛЕН ТИП LWD_4Tx!");
					toolStripComboBox_sonde_type->SelectedItem = "LWD_4Tx";
					sonde_type = LWD_4Tx_;
				}

				if (result_receive_from_sonde == 0) {
					uint32_t signature = *(uint32_t*)(DATA_IN);//убрать 2 при переходе на receive_from_tool
					cout << "signature_0 " << std::hex << signature << endl;
					id = get_sonde_id(signature);
					cout << "id.type_0 " << std::dec << id.type << endl;

					if (id.type == CARTOGRAPH_) {
						toolStripComboBox_sonde_type->SelectedItem = "CARTOGRAPH";
						sonde_type == CARTOGRAPH_;
					}
					else if (id.type == LWD_4Tx_) {
						toolStripComboBox_sonde_type->SelectedItem = "LWD_4Tx";
						sonde_type = LWD_4Tx_;
					}
					else if (id.type == AUTONOM_5Tx_) {
						toolStripComboBox_sonde_type->SelectedItem = "AUTONOM_5Tx";
						sonde_type = AUTONOM_5Tx_;
					}
					else if (id.type == AUTONOM_5Tx_SDR_) {
						toolStripComboBox_sonde_type->SelectedItem = "AUTONOM_5Tx_SDR";
						sonde_type = AUTONOM_5Tx_SDR_;
					}
				}

				///////////////////////////////////////////////////////////////////////////////
				//читаем файл метрологии
				Sleep(300);
				current_metrology_struct = read_metro_from_tool(serialPort1, sonde_type, &read_status);
				if (read_status == true) {
					//METROLOGY_GP gp_metro = transform_metro(current_metrology_struct);
					//id = get_sonde_id(gp_metro.signature);
					if (sonde_type == CARTOGRAPH_) {
						METROLOGY_CARTOGRAPH metro = get<METROLOGY_CARTOGRAPH>(current_metrology_struct); 					
						global_selected_key = metro.select_key;
					}
					MessageBox::Show("ФАЙЛ МЕТРОЛОГИИ  СЧИТАН ИЗ ПРИБОРА!");
				}
				else
					MessageBox::Show("ФАЙЛ МЕТРОЛОГИИ НЕ БЫЛ СЧИТАН !!!");

				serialPort1->Close();
				/////////////////////////////////////////////////////////////////////////////////////
				sonde_on_off = true;

				if (sonde_type == CARTOGRAPH_ ||sonde_type == AUTONOM_5Tx_SDR_)Timer_get_data_from_sonde->Interval = 2000;
				else Timer_get_data_from_sonde->Interval = 1000;

				Timer_get_data_from_sonde->Enabled = true;
				this->toolStripButton_on_off->BackColor = System::Drawing::Color::Red;
				this->toolStripButton_on_off->Text = "ВЫКЛ";
				ToolStripMenuItem_temporary_file->Visible = true;
				temp_flg = 1;
				first_kadr = true;
				write_to_file_all_data_flg = false;

			}
			else MessageBox::Show("Не удется открыть порт!");

		}


		else {
			sonde_on_off = false;
			this->toolStripButton_on_off->BackColor = System::Drawing::Color::White;
			this->toolStripButton_on_off->Text = "ВКЛ";
			Timer_get_data_from_sonde->Enabled = false;
			Sleep(100);
			ToolStripMenuItem_temporary_file->Visible = false;
			first_kadr = false;
			write_to_file_all_data_flg = false;
		}

	}

	// получить данные из прибора
	private:void timer_get_data_from_sonde(System::Object^ source, ElapsedEventArgs^ e) {
		bool console_debug = true;
		ID id;
		uint8_t D0 = 0;
		DATA data_buff;
		RAW_DATA raw_data_buff;
		DATA data_buff_0;
		RAW_DATA raw_data_buff_0;

		memset(&data_buff, 0x00, sizeof(DATA));
		memset(&raw_data_buff, 0x00, sizeof(RAW_DATA));
		memset(&data_buff_0, 0x00, sizeof(DATA));
		memset(&raw_data_buff_0, 0x00, sizeof(RAW_DATA));

		bool stop = false;

		if (sonde_on_off == true && metro_check == false) {

			busy_read_from_sonde = true;
			try {
				serialPort1->PortName = gcnew String(Com_Name.c_str());
				Sleep(100);
				serialPort1->ReadTimeout = 2000;
				serialPort1->WriteTimeout = 2000;
				if (sonde_type == CARTOGRAPH_)
					serialPort1->ReadBufferSize = 10000;
				else if (sonde_type == LWD_4Tx_ || sonde_type == AUTONOM_5Tx_ || AUTONOM_5Tx_SDR_ == 152)
					serialPort1->ReadBufferSize = 2048;
				serialPort1->BaudRate = baudrate;
				serialPort1->Open();

				cout << "sonde type " << sonde_type << " baudrate " << baudrate << endl;
			}

			catch (...) { stop = true; }

			if (!stop) {
				////////////CARTOGRAPH_//////////////////////////////////////////////////////////////////////////////////////////////////
				if (sonde_type == CARTOGRAPH_) {
					int readed_from_sonde = 1;
					send_to_tool(serialPort1, CMD_GET_DATA, &D0, 0);
					Sleep(300);
					bool struct_length = false;
					uint8_t CMD; uint16_t income_length = 0; uint8_t DATA_IN[2048];
					readed_from_sonde = receive_from_tool(serialPort1, CMD, DATA_IN, &income_length);
					if (readed_from_sonde == 0) {
						if (income_length == sizeof(DATA)) {
							struct_length = true;
							memcpy(&data_buff, DATA_IN, sizeof(DATA));
							if (console_debug == true)
								cout << "ALL DATA ONLY freq " << data_buff.dds_freq << " kHz " << endl;
						}
						else if (income_length == sizeof(DATA) + sizeof(RAW_DATA)) {
							struct_length = true;
							memcpy(&data_buff, DATA_IN, sizeof(DATA));
							memcpy(&raw_data_buff, DATA_IN + sizeof(DATA), sizeof(RAW_DATA));
							if (console_debug == true)
								cout << "ALL DATA + RAW_DATA freq " << data_buff.dds_freq << " kHz " << endl;
						}
						else {
							struct_length = false;
							if (console_debug == true)
								cout << "struct_length error " << endl;
						}
					}
					else
						cout << "readed_from_sonde " << readed_from_sonde << endl;
					/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
					if (readed_from_sonde == 0) {
						id = get_sonde_id(data_buff.signature);
						if (id.type == CARTOGRAPH_) {
							uint8_t work_mode = (global_selected_key >> 24);							
							if (work_mode == _400) {//400
								cout << "work_mode 400" << endl;
								
								if (data_buff.dds_freq == 401) {
									//пишем в первые члены структуры, вторые обнуляем
									all_data_buff->data[0] = data_buff;
									all_data_buff->raw_data[0] = raw_data_buff;
									all_data_buff->data[1] = data_buff_0;
									all_data_buff->raw_data[1] = raw_data_buff_0;
									fout->open(Dataout.c_str(), ios_base::app | ios_base::binary);
									fout->write((char*)all_data_buff, sizeof(ALL_DATA));
									fout->flush();
									fout->close();
								}
								
							}
							if (work_mode == _2000) {//2000
								cout << "work_mode 2000" << endl;
								//проверяем
								if (data_buff.dds_freq == 2000) {
									//пишем во вторые члены структуры, первые обнуляем
									all_data_buff->data[0] = data_buff_0;
									all_data_buff->raw_data[0] = raw_data_buff_0;
									all_data_buff->data[1] = data_buff;
									all_data_buff->raw_data[1] = raw_data_buff;
									fout->open(Dataout.c_str(), ios_base::app | ios_base::binary);
									fout->write((char*)all_data_buff, sizeof(ALL_DATA));
									fout->flush();
									fout->close();
								}
								
							}

							if (work_mode == BOTH) {//работают обе частоты
								cout << "work_mode BOTH" << endl;
							    // если первый кадр на частоте 400 - то пишем в первый член структуры
								if (first_kadr == true && data_buff.dds_freq == 401) {
									all_data_buff->data[0] = data_buff;
									all_data_buff->raw_data[0] = raw_data_buff;
									first_kadr = false;
									write_to_file_all_data_flg = false;
									//cout << "f_k " << first_kadr << " freq " << data_buff.dds_freq << " flg " << write_to_file_all_data_flg << endl;
								}
								//если первый кадр на частоте 2000 - то пропускаем
								else if (first_kadr == true && data_buff.dds_freq == 2000) {
									first_kadr = false;
									write_to_file_all_data_flg = false;
									//cout << "f_k " << first_kadr << " freq " << data_buff.dds_freq << " flg " << write_to_file_all_data_flg << endl;
								}
								// если не первый кадр на частоте 400 - то пишем в первый член структуры и 
								// ждем второго кадра на частоте 2000 (флаг записи false)
								else if (first_kadr == false && data_buff.dds_freq == 401) {
									all_data_buff->data[0] = data_buff;
									all_data_buff->raw_data[0] = raw_data_buff;
									first_kadr = false;
									write_to_file_all_data_flg = false;
									//cout << " Rzz1 " << data_buff.R_zz.Rzz1[1] << endl;
									//cout <<" Rzz11 " << all_data_buff.data[0].R_zz.Rzz1[1] << endl;
									//cout << "f_k " << first_kadr << " freq " << data_buff.dds_freq << " flg " << write_to_file_all_data_flg << endl;
								}
								//если не первый кадр на частоте 2000 - то пишем во второй член структуры 
								//и записываем в файл (флаг записи true)
								else if (first_kadr == false && data_buff.dds_freq == 2000) {
									all_data_buff->data[1] = data_buff;
									all_data_buff->raw_data[1] = raw_data_buff;
									first_kadr = false;
									write_to_file_all_data_flg = true;
									//cout << "f_k " << first_kadr << " freq " << data_buff.dds_freq << " flg " << write_to_file_all_data_flg << endl;
								}
								// пишем в файл объединенную структуру
								if (write_to_file_all_data_flg == true) {
									fout = new ofstream;
									busy = true;
									fout->open(Dataout.c_str(), ios_base::app | ios_base::binary);
									fout->write((char*)all_data_buff, sizeof(ALL_DATA));
									fout->flush();
									fout->close();
									busy = false;
									delete fout;
									write_to_file_all_data_flg = false;
								}

							}
						
						}

					}
					////////////////////////////////////////////////////////////////////////////////////////

					console_debug = false;
					if (struct_length == true && console_debug == true) {
						cout << "ALL_DATA_1FREQ " << endl;
						cout << " SGN " << std::dec << data_buff.signature << endl;
						//std::bitset<16> y(c);
						cout << " frame " << data_buff.frame << endl;
						cout << " dds_freq " << data_buff.dds_freq << endl;
						cout << endl;

						cout << "out_arr " << endl;
						for (int n = 0; n < 32; n++)
							cout << bitset<8>(data_buff.out_arr[n]) << " ";
						cout << endl;
						cout << endl;

						DECOMPRESSED_1freq d_data[2] = { 0, };
						METROLOGY_CARTOGRAPH metro;
						DATA data_for_decompress[2];

						metro.select_key = global_selected_key;
						metro.reap = reap;
						receive_compressed_data(&d_data[0], &metro, data_buff.out_arr);

						cout << "freq 400 dec " << d_data[_400_kGz].dds_freq << endl;
						cout << "ATT GEO 400 dec ";
						for (int Tx = 0; Tx < 4; Tx++) {
							cout << d_data[_400_kGz].GA[Tx] << " ";
						}
						cout << endl;
						cout << "PH GEO 400 dec ";
						for (int Tx = 0; Tx < 4; Tx++) {
							cout << d_data[_400_kGz].GP[Tx] << " ";
						}
						cout << endl;
						cout << "Ro  400 dec ";
						for (int Tx = 0; Tx < 4; Tx++) {
							cout << d_data[_400_kGz].Ro[Tx] << " ";
						}
						cout << endl;
						cout << endl;

						cout << "freq 2000 dec " << d_data[_2000_kGz].dds_freq << endl;
						cout << "ATT GEO 2000 dec  ";
						for (int Tx = 0; Tx < 4; Tx++) {
							cout << d_data[_2000_kGz].GA[Tx] << " ";
						}
						cout << endl;
						cout << "PH GEO 2000 dec ";
						for (int Tx = 0; Tx < 4; Tx++) {
							cout << d_data[_2000_kGz].GP[Tx] << " ";
						}
						cout << endl;
						cout << "Ro  2000 dec ";
						for (int Tx = 0; Tx < 4; Tx++) {
							cout << d_data[_2000_kGz].Ro[Tx] << " ";
						}
						cout << endl;
						cout << endl;

						/*
						cout << "phase_smt ";
						for (int i = 0; i < 4; i++) {
							cout << data_buff.phase_smt[i] << " ";
						}
						cout << endl;
						cout << endl;
						*/

						/*
						cout << "rho_smt ";
						for (int i = 0; i < 4; i++) {
							cout << data_buff.rho_smt[i] << " ";
						}
						cout << endl;

						cout << endl;
						*/
						//cout << "INC.Aps " << data_buff.INC.Aps << " INC.Azm " << data_buff.INC.Azm << " INC.Zen " << data_buff.INC.Zen << endl;
						//cout << "ApsStartAngle " << data_buff.start_sector << " Wg " << data_buff.Wg  << endl;
						//cout << endl;

						cout << endl;
					}

				}

				/////////////////////////LWD_4Tx ////////////////////////////////////////////////////////////////////////////////////////////
				else if (sonde_type == LWD_4Tx_) {
					Console::WriteLine(""); Console::WriteLine("");
					const uint8_t length = 252;
					uint8_t *DATA = new uint8_t;
					*DATA = length;
					uint8_t CMD;
					uint8_t DATA_IN[length] = { 0, };
					_4Tx_DATA *_4Tx_data = new _4Tx_DATA;
					uint32_t signature_for_pallete;
					int readed_from_sonde = 1;

					if (send_to_tool_(serialPort1, CMD_WORK, DATA, 1) == 1) {
						MessageBox::Show("Соединение с прибором прервано переподключите !");
					}
					else {
						Sleep(100);
						readed_from_sonde = receive_from_tool_(serialPort1, CMD, DATA_IN, length);
						Console::WriteLine("_4Tx_DATA received ");				
					}
					if (readed_from_sonde == 0) {
						_4Tx_data = (_4Tx_DATA*)(DATA_IN + 12);
						uint32_t buff;
						buff = _4Tx_data->signature << 12;	buff = buff >> 12;
						signature_for_pallete = buff / 1000;
						cout << "signature_for_pallete " << signature_for_pallete << endl;
						if (signature_for_pallete == LWD_4Tx_) {
							fout = new ofstream;
							fout->open(Dataout.c_str(), ios_base::app | ios_base::binary);
							fout->write((char*)_4Tx_data, sizeof(_4Tx_DATA));
							fout->flush();
							fout->close();
							delete fout;

							if (console_debug == true) {
								cout << " CMD " << std::hex << CMD << endl;
								cout << " SGN " << _4Tx_data->signature << endl;
								//std::bitset<16> y(c);
								cout << " condition " << _4Tx_data->condition << endl;
								cout << " frame " << _4Tx_data->frame << endl;
								cout << std::dec << std::setprecision(6);
								cout << "TEMP " << _4Tx_data->temperature << endl;

								cout << "Ro " << _4Tx_data->rho_smt[0][0] << " " << _4Tx_data->rho_smt[0][1] << " " << _4Tx_data->rho_smt[0][2] << " " << _4Tx_data->rho_smt[0][3]
									<< " " << _4Tx_data->rho_smt[1][0] << " " << _4Tx_data->rho_smt[1][1] << " " << _4Tx_data->rho_smt[1][2] << " " << _4Tx_data->rho_smt[1][3] << endl;
								cout << "Ro " << _4Tx_data->phase_smt[0][0] << " " << _4Tx_data->phase_smt[0][1] << " " << _4Tx_data->phase_smt[0][2] << " " << _4Tx_data->phase_smt[0][3]
									<< " " << _4Tx_data->phase_smt[1][0] << " " << _4Tx_data->phase_smt[1][1] << " " << _4Tx_data->phase_smt[1][2] << " " << _4Tx_data->phase_smt[1][3] << endl;

								for (int i = 0; i < 4; i++) {
									cout << "AM_TX_400 " << i << " RX_1 " << _4Tx_data->AM_RX_1[0][i] << " RX_2 " << _4Tx_data->AM_RX_2[0][i] << endl;
								}
								for (int i = 0; i < 4; i++) {
									cout << "AM_TX_2000 " << i << " RX_1 " << _4Tx_data->AM_RX_1[1][i] << " RX_2 " << _4Tx_data->AM_RX_2[1][i] << endl;
								}

								cout << "AM_TX_400_0 " << " RX_1 " << _4Tx_data->ZERO_AM_RX_1[0] << " RX_2 " << _4Tx_data->ZERO_AM_RX_2[0] << endl;
								cout << "AM_TX_2000_0 " << " RX_1 " << _4Tx_data->ZERO_AM_RX_1[1] << " RX_2 " << _4Tx_data->ZERO_AM_RX_2[1] << endl;

								for (int i = 0; i < 4; i++) {
									cout << "PH_TX_400 " << i << " RX_1 " << _4Tx_data->PH_RX_1[0][i] << " RX_2 " << _4Tx_data->PH_RX_2[0][i] << endl;
								}
								for (int i = 0; i < 4; i++) {
									cout << "PH_TX_2000 " << i << " RX_1 " << _4Tx_data->PH_RX_1[1][i] << " RX_2 " << _4Tx_data->PH_RX_2[1][i] << endl;
								}

								cout << "PH_TX_400_0 " << " RX_1 " << _4Tx_data->ZERO_AM_RX_1[0] << " RX_2 " << _4Tx_data->ZERO_AM_RX_2[0] << endl;
								cout << "PH_TX_400_0 " << " RX_1 " << _4Tx_data->ZERO_AM_RX_1[1] << " RX_2 " << _4Tx_data->ZERO_AM_RX_2[1] << endl;

							}

						}

					}

				}

				/////////////////////////AUTONOM_5Tx_ или AUTONOM_5Tx_SDR_////////////////////////////////////////////////////////////////////////////////////////////
				else if (sonde_type == AUTONOM_5Tx_ || sonde_type == AUTONOM_5Tx_SDR_) {
					Console::WriteLine(""); Console::WriteLine("");

					uint8_t CMD; uint8_t DATA[2] = { 0x05 ,0x00 };
					uint8_t DATA_IN[2048] = { 0, }; uint16_t readed_length = 0;
					_5Tx_DATA *_5Tx_data = new _5Tx_DATA;

					int readed_from_sonde = 1;

					send_to_tool(serialPort1, CMD_GET_DATA, &D0, 0);


					Sleep(300);
					readed_from_sonde = receive_from_tool(serialPort1, CMD, DATA_IN, &readed_length);

					if (readed_from_sonde == 0) {
						/////////////////////////AUTONOM_5Tx или AUTONOM_5Tx_SDR_////////////////////////////////////////////////////////////////////////////////////////////						
						Console::WriteLine("_5Tx_DATA received ");
						_5Tx_data = (_5Tx_DATA*)(DATA_IN);
						id = get_sonde_id(_5Tx_data->signature);
						cout << "id.type " << std::dec << id.type << endl;
						if (id.type == AUTONOM_5Tx_ || id.type == AUTONOM_5Tx_SDR_) {
							fout = new ofstream;
							fout->open(Dataout.c_str(), ios_base::app | ios_base::binary);
							fout->write((char*)_5Tx_data, sizeof(_5Tx_DATA));
							fout->flush();
							fout->close();
							delete fout;

							if (console_debug == true) {

								cout << " CMD " << std::hex << (int)CMD << endl;
								cout << " SGN " << _5Tx_data->signature << endl;
								//std::bitset<16> y(c);
								cout << " condition " << bitset<32>(_5Tx_data->condition) << endl;
								cout << " frame " << std::dec << _5Tx_data->frame << endl;
								cout << std::dec << std::setprecision(6);
								cout << "TEMP " << _5Tx_data->temperature << endl;

								cout << "Ro " << _5Tx_data->rho_smt[0][0] << " " << _5Tx_data->rho_smt[0][1] << " " << _5Tx_data->rho_smt[0][2] << " " << _5Tx_data->rho_smt[0][3] << " " << _5Tx_data->rho_smt[0][4]
									<< " " << _5Tx_data->rho_smt[1][0] << " " << _5Tx_data->rho_smt[1][1] << " " << _5Tx_data->rho_smt[1][2] << " " << _5Tx_data->rho_smt[1][3] << " " << _5Tx_data->rho_smt[1][4] << endl;
								cout << "PH_SMT " << _5Tx_data->phase_smt[0][0] * mG << " " << _5Tx_data->phase_smt[0][1] * mG << " " << _5Tx_data->phase_smt[0][2] * mG << " " << _5Tx_data->phase_smt[0][3] * mG << " " << _5Tx_data->phase_smt[1][4] * mG
									<< " " << _5Tx_data->phase_smt[1][0] * mG << " " << _5Tx_data->phase_smt[1][1] * mG << " " << _5Tx_data->phase_smt[1][2] * mG << " " << _5Tx_data->phase_smt[1][3] * mG << " " << _5Tx_data->phase_smt[1][4] * mG << endl;

								for (int i = 0; i < 5; i++) {
									cout << "AM_TX_400 " << i << " RX_1 " << _5Tx_data->AM_RX_1[0][i] << " RX_2 " << _5Tx_data->AM_RX_2[0][i] << endl;
								}
								for (int i = 0; i < 5; i++) {
									cout << "AM_TX_2000 " << i << " RX_1 " << _5Tx_data->AM_RX_1[1][i] << " RX_2 " << _5Tx_data->AM_RX_2[1][i] << endl;
								}

								cout << "AM_TX_400_0 " << " RX_1 " << _5Tx_data->ZERO_AM_RX_1[0] << " RX_2 " << _5Tx_data->ZERO_AM_RX_2[0] << endl;
								cout << "AM_TX_2000_0 " << " RX_1 " << _5Tx_data->ZERO_AM_RX_1[1] << " RX_2 " << _5Tx_data->ZERO_AM_RX_2[1] << endl;

								for (int i = 0; i < 5; i++) {
									cout << "DELTA_PH_400 " << i << " " << _5Tx_data->DELTA_PH[0][i] * mG << endl;
								}
								for (int i = 0; i < 5; i++) {
									cout << "DELTA_PH_2000 " << i << " " << _5Tx_data->DELTA_PH[1][i] * mG << endl;
								}
							}

						}
					}
					cout << "result " << readed_from_sonde << endl;

				}
			}

			else {
				stop = 0;
				cout << "struct not read !!!" << endl;
			}

			//metro_check = false;
			serialPort1->Close();
			busy_read_from_sonde = false;

		}

	}

	public:  System::Void  Read_Port() {
		Timer_get_data_from_sonde = gcnew System::Timers::Timer(2000);
		Timer_get_data_from_sonde->Elapsed += gcnew ElapsedEventHandler(this, &MyForm::timer_get_data_from_sonde);
		Timer_get_data_from_sonde->AutoReset = true;
		Timer_get_data_from_sonde->Enabled = false;
	}

	private: System::Void button_400_2000_1_Click(System::Object^  sender, System::EventArgs^  e) {
		if (index_freq[0] == 1) {
			index_freq[0] = 0;
			this->button_400_2000_1->Text = "400";
		}
		else if (index_freq[0] == 0) {
			index_freq[0] = 1;
			this->button_400_2000_1->Text = "2000";
		}

		if (gp_data.size() > 1 && surf_on_off == false) {
			do_chart(0);
		}

	}

	private: System::Void button_400_2000_2_Click(System::Object^  sender, System::EventArgs^  e) {
		if (index_freq[1] == 1) {
			index_freq[1] = 0;
			this->button_400_2000_2->Text = "400";
		}
		else if (index_freq[1] == 0) {
			index_freq[1] = 1;
			this->button_400_2000_2->Text = "2000";
		}

		if (gp_data.size() > 1 && surf_on_off == false) {
			do_chart(1);
		}

	}

	private: System::Void button_ZX_1_Click(System::Object^  sender, System::EventArgs^  e) {
		if (index_rzx[up_chart] == Right_) {
			index_rzx[up_chart] = Left_;
			this->button_ZX_1->Text = "R_Zx L";
		}
		else if (index_rzx[up_chart] == Left_) {
			index_rzx[up_chart] = Right_;
			this->button_ZX_1->Text = "R_Zx R";
		}
		if (all_data.size() > 1 && surf_on_off == false) {
			do_chart(0);
		}
	}

	private: System::Void button_ZX_2_Click(System::Object^  sender, System::EventArgs^  e) {
		if (index_rzx[down_chart] == Right_) {
			index_rzx[down_chart] = Left_;
			this->button_ZX_2->Text = "R_Zx L";
		}
		else if (index_rzx[down_chart] == Left_) {
			index_rzx[down_chart] = Right_;
			this->button_ZX_2->Text = "R_Zx R";
		}
		if (all_data.size() > 1 && surf_on_off == false) {
			do_chart(1);
		}
	}

	private: System::Void button_direct1_Click(System::Object^  sender, System::EventArgs^  e) {
		comboBox_choise_chart1->Items->Clear();
		if (index_dir_undir[up_chart] == direct) {
			index_dir_undir[up_chart] = undir;
			this->button_direct1->Text = "UNDIR";

			comboBox_choise_chart1->Items->AddRange(gcnew cli::array< System::Object^  >(12) {
				L"PH Z2-Z1",//1
					L"AM Z2/Z1",//2
					L"AM Z1",//3 
					L"AM Z2",//4
					L"AM Zero ZZ",//5	
					L"Temp RZZ",//6
					L"PH_SMT_from_computer",//7		
					L"УЭС_from_computer",//8
					L"PH_SMT_from_sonde",//9
					L"УЭС_from_sonde",//10
					L"Погрешность УЭС",//11	
					"FRAME",//12					
			});

			comboBox_choise_chart1->SelectedIndex = undir_selected_index[up_chart];

			this->button_ZX_1->Visible = false;

			for (int j = 0; j < 8; j++) {
				PictureBox^ PICTUREBOX = (PictureBox^)this->panel_Sonde_Picture->Controls["pictureBoxD" + (j + 1).ToString()];
				PICTUREBOX->Visible = false;
			}
			for (int j = 8; j < 12; j++) {
				PictureBox^ PICTUREBOX = (PictureBox^)this->panel_Sonde_Picture->Controls["pictureBoxD" + (j + 1).ToString()];
				PICTUREBOX->Visible = true;
			}

			pictureBoxR3->Visible = false; pictureBoxR4->Visible = false; pictureBoxT5->Visible = false;
			labelR3->Visible = false; labelR4->Visible = false; labelT5->Visible = false;

			N_Raw_Data_point_1->Visible = false;

		}
		else if (index_dir_undir[0] == 0) {
			index_dir_undir[0] = 1;
			this->button_direct1->Text = "DIRECT";

			comboBox_choise_chart1->Items->AddRange(gcnew cli::array< System::Object^  >(18) {
				L"AM_GEO",//1
					L"PH_GEO",//2
					L"AM_V_ZX",//3				
					L"AM_V_ZZ",//4
					L"betta_z",//5
					L"PH_VZX / VZZ",//6
					L"AM_V_XX",//7
					L"dV",//8
					L"SNR",//9
					L"AM Zero ZX",//10
					L"AM_GEO_SMT",//11
					L"PH_GEO_SMT",//12
					L"BORDER_ANGLE",//13
					L"INC Abs; Zen; Azm",//14
					L"ANGULAR_VELOCITY",//15
					L"START_SECTOR",//16
					L"Temp RZX",//17
					L"FRAME",//18
			});
			comboBox_choise_chart1->SelectedIndex = direct_selected_index[up_chart];
			this->button_ZX_1->Visible = true;

			for (int j = 0; j < 8; j++) {
				PictureBox^ PICTUREBOX = (PictureBox^)this->panel_Sonde_Picture->Controls["pictureBoxD" + (j + 1).ToString()];
				PICTUREBOX->Visible = true;
			}
			for (int j = 8; j < 12; j++) {
				PictureBox^ PICTUREBOX = (PictureBox^)this->panel_Sonde_Picture->Controls["pictureBoxD" + (j + 1).ToString()];
				PICTUREBOX->Visible = false;
			}

			pictureBoxR3->Visible = true; pictureBoxR4->Visible = true; pictureBoxT5->Visible = true;
			labelR3->Visible = true; labelR4->Visible = true; labelT5->Visible = true;

			N_Raw_Data_point_1->Visible = true;

		}



	}

	private: System::Void button_direct2_Click(System::Object^  sender, System::EventArgs^  e) {
		comboBox_choise_chart2->Items->Clear();
		if (index_dir_undir[1] == 1) {
			index_dir_undir[1] = 0;
			this->button_direct2->Text = "UNDIR";

			comboBox_choise_chart2->Items->AddRange(gcnew cli::array< System::Object^  >(12) {
				L"PH Z2-Z1",//1
					L"AM Z2/Z1",//2
					L"AM Z1",//3 
					L"AM Z2",//4
					L"AM Zero ZZ",//5	
					L"Temp RZZ",//6
					L"PH_SMT_from_computer",//7		
					L"УЭС_from_computer",//8
					L"PH_SMT_from_sonde",//9
					L"УЭС_from_sonde",//10	
					L"Погрешность УЭС",//11	
					"FRAME",//12					
			});
			comboBox_choise_chart2->SelectedIndex = undir_selected_index[down_chart];
			this->button_ZX_2->Visible = false;//


			for (int j = 0; j < 8; j++) {
				PictureBox^ PICTUREBOX = (PictureBox^)this->panel_Sonde_Picture->Controls["pictureBoxD" + (j + 1).ToString()];
				PICTUREBOX->Visible = false;
			}
			for (int j = 8; j < 12; j++) {
				PictureBox^ PICTUREBOX = (PictureBox^)this->panel_Sonde_Picture->Controls["pictureBoxD" + (j + 1).ToString()];
				PICTUREBOX->Visible = true;
			}

			pictureBoxR3->Visible = false; pictureBoxR4->Visible = false; pictureBoxT5->Visible = false;
			labelR3->Visible = false; labelR4->Visible = false; labelT5->Visible = false;

			N_Raw_Data_point_2->Visible = false;

		}
		else if (index_dir_undir[1] == 0) {
			index_dir_undir[1] = 1;
			this->button_direct2->Text = "DIRECT";

			comboBox_choise_chart2->Items->AddRange(gcnew cli::array< System::Object^  >(18) {
				L"AM_GEO",//1
					L"PH_GEO",//2
					L"AM_V_ZX",//3				
					L"AM_V_ZZ",//4
					L"betta_z",//5
					L"PH_VZX / VZZ",//6
					L"AM_V_XX",//7
					L"dV",//8
					L"SNR",//9
					L"AM Zero ZX",//10
					L"AM_GEO_SMT",//11
					L"PH_GEO_SMT",//12
					L"BORDER_ANGLE",//13
					L"INC Abs; Zen; Azm",//14
					L"ANGULAR_VELOCITY",//15
					L"START_SECTOR",//16
					L"Temp RZX",//17
					L"FRAME",//18
			});
			comboBox_choise_chart2->SelectedIndex = direct_selected_index[down_chart];
			this->button_ZX_2->Visible = true;

			for (int j = 0; j < 8; j++) {
				PictureBox^ PICTUREBOX = (PictureBox^)this->panel_Sonde_Picture->Controls["pictureBoxD" + (j + 1).ToString()];
				PICTUREBOX->Visible = true;
			}
			for (int j = 8; j < 12; j++) {
				PictureBox^ PICTUREBOX = (PictureBox^)this->panel_Sonde_Picture->Controls["pictureBoxD" + (j + 1).ToString()];
				PICTUREBOX->Visible = false;
			}

			pictureBoxR3->Visible = true; pictureBoxR4->Visible = true; pictureBoxT5->Visible = true;
			labelR3->Visible = true; labelR4->Visible = true; labelT5->Visible = true;

			N_Raw_Data_point_2->Visible = true;

		}
		//comboBox_choise_chart2->SelectedIndex = 0;


	}

	private: System::Void checkBox_re_im_CheckedChanged(System::Object^  sender, System::EventArgs^  e) {
		if (checkBox_re_im->Checked == true)
			checkBox_stable_ph->Checked = true;
		button_re_im_1->PerformClick();
		button_re_im_2->PerformClick();
	}

	private: System::Void checkBox_stable_ph_CheckedChanged(System::Object^  sender, System::EventArgs^  e) {
		button_re_im_1->PerformClick();
		button_re_im_2->PerformClick();
	}

	private: System::Void button_re_im_1_Click(System::Object^  sender, System::EventArgs^  e) {
		if (checkBox_stable_ph->Checked == false) {
			if (index_amp[0] == 1) {
				index_amp[0] = 0;
				this->button_re_im_1->Text = "amp2";
			}
			else if (index_amp[0] == 0) {
				index_amp[0] = 1;
				this->button_re_im_1->Text = "amp2";
			}
		}
		if (checkBox_stable_ph->Checked == true) {

			if (checkBox_re_im->Checked == false) {
				if (index_amp[0] == 1) {
					index_amp[0] = 0;
					this->button_re_im_1->Text = "amp";
				}
				else if (index_amp[0] == 0) {
					index_amp[0] = 1;
					this->button_re_im_1->Text = "Ph";
				}
			}
			else if (checkBox_re_im->Checked == true) {
				if (index_amp[0] == 1) {
					index_amp[0] = 0;
					this->button_re_im_1->Text = "Re";
				}
				else if (index_amp[0] == 0) {
					index_amp[0] = 1;
					this->button_re_im_1->Text = "Im";
				}
			}

		}

		do_raw_chart(0, n_raw_data_point[0]);
	}

	private: System::Void button_re_im_2_Click(System::Object^  sender, System::EventArgs^  e) {
		if (checkBox_stable_ph->Checked == false) {
			if (index_amp[1] == 1) {
				index_amp[1] = 0;
				this->button_re_im_2->Text = "amp2";
			}
			else if (index_amp[1] == 0) {
				index_amp[1] = 1;
				this->button_re_im_2->Text = "amp2";
			}
		}
		if (checkBox_stable_ph->Checked == true) {
			if (checkBox_re_im->Checked == false) {
				if (index_amp[1] == 1) {
					index_amp[1] = 0;
					this->button_re_im_2->Text = "amp";
				}
				else if (index_amp[1] == 0) {
					index_amp[1] = 1;
					this->button_re_im_2->Text = "Ph";
				}
			}
			else if (checkBox_re_im->Checked == true) {
				if (index_amp[1] == 1) {
					index_amp[1] = 0;
					this->button_re_im_2->Text = "Re";
				}
				else if (index_amp[1] == 0) {
					index_amp[1] = 1;
					this->button_re_im_2->Text = "Im";
				}
			}
		}


		do_raw_chart(1, n_raw_data_point[1]);
	}


	private: System::Void ToolStripMenuItem_save_file_Click(System::Object^  sender, System::EventArgs^  e) {
		saveFileDialog1->Filter = "bin files   (*.bin;)|*.bin; ";
		//saveFileDialog1->CheckFileExists = false;
		//saveFileDialog1->CheckPathExists = false;
		string Alexout_for_save;
		bool dataout_for_save = false; bool alexout_for_save = false;

		if (saveFileDialog1->ShowDialog() == System::Windows::Forms::DialogResult::OK) {
			Dataout_for_save = (char*)Marshal::StringToHGlobalAnsi(saveFileDialog1->FileName).ToPointer();
			extension_for_save = (char*)Marshal::StringToHGlobalAnsi(Path::GetExtension(saveFileDialog1->FileName)).ToPointer();
			dataout_for_save = true;
		}

		if (sonde_type == CARTOGRAPH_) {
			if (saveFileDialog1->ShowDialog() == System::Windows::Forms::DialogResult::OK) {
				Alexout_for_save = (char*)Marshal::StringToHGlobalAnsi(saveFileDialog1->FileName).ToPointer();
				alexout_for_save = true;
			}
		}
		

		ofstream fout, fout_alex;
		if (dataout_for_save == true)
			fout.open(Dataout_for_save.c_str(), ios_base::binary);
		if (alexout_for_save == true)
			fout_alex.open(Alexout_for_save.c_str(), ios::binary);

		ALL_DATA all_data_buff;
		for (int N = min_n[0]; N < max_n[0]; N++) {
			if (sonde_type == CARTOGRAPH_)
				all_data_buff = all_data[N];

			if (dataout_for_save == true) {
				if (sonde_type == CARTOGRAPH_)
					fout.write((char*)&all_data_buff, sizeof(all_data_buff));
				if (sonde_type == LWD_4Tx_) {
					_4Tx_DATA _4_tx_data_buff = transform_GP_to_4Tx_data(&gp_data[N]);
					fout.write((char*)&_4_tx_data_buff, sizeof(_4_tx_data_buff));
				}
				if (sonde_type == AUTONOM_5Tx_ || sonde_type == AUTONOM_5Tx_SDR_) {
					GP_DATA gp_data_buff = gp_data[N];
					fout.write((char*)&gp_data_buff, sizeof(GP_DATA));
				}
		
			}


			if (alexout_for_save == true && sonde_type == CARTOGRAPH_) {
				RAW_DATA_ALEX alex_buff;//[freq] [Rx_Left, Rx_Right], [Tx1234[4]]
				for (int freq = 0; freq < 2; freq++) {
					for (int Rx = 0; Rx < 2; Rx++) {
						for (int Tx = 0; Tx < 4; Tx++) {
							for (int n = 0; n < 16; n++) {
								alex_buff.R_zx[freq][Rx][Tx][n] = { all_data_buff.raw_data[freq].R_zx[Rx][Tx].sector[Re][n] ,
																	all_data_buff.raw_data[freq].R_zx[Rx][Tx].sector[Im][n] };
								//alex_buff.R_zx[freq][Rx][Tx][n] =(pow(all_data_buff.raw_data[freq].R_zx[Rx][Tx].sector[Re][n], 2) +
																 // pow(all_data_buff.raw_data[freq].R_zx[Rx][Tx].sector[Im][n], 2));
							}
						}
					}
				}
				alex_buff.start_sector[0] = all_data_buff.data[0].start_sector;
				alex_buff.start_sector[1] = all_data_buff.data[1].start_sector;
				fout_alex.write((char*)&alex_buff, sizeof(alex_buff));
			}

		}
		if (dataout_for_save == true) {
			fout.close();
			was_save = true;
			ToolStripMenuItem_data_file->PerformClick();
		}

		if (alexout_for_save == true)
			fout_alex.close();

	}

	private: System::Void toolStripMenuItem_delete_Click(System::Object^  sender, System::EventArgs^  e) {
		if (surf_on_off == true) {
			buttonSURF->PerformClick();
		}
		Sleep(200);
		if (remove(Dataout.c_str())) {
			printf("Error removing file");
		}
		else {
			for (int i = 0; i < 11; i++) {
				chart1->Series[i]->Points->Clear();
				chart2->Series[i]->Points->Clear();
			}
		}




	}

	private: System::Void temporary_file_Click(System::Object^  sender, System::EventArgs^  e) {
		if (sonde_on_off == true) {
			cout << "Sonde_is_ON " << endl;
			Data_file_name = Dataout.c_str();
			temporary_file = true;
			ToolStripMenuItem_data_file->PerformClick();
		}

	}

			 MyForm1^ Metro;
	private: System::Void toolStripButton_metro_file_Click(System::Object^  sender, System::EventArgs^  e) {
		if (metro_form_is_open == false) {
			metro_form_is_open = true;
			Metro = gcnew MyForm1();
			Metro->Show();

		}
		else {
			;
		}
	}

	private: System::Void button_back_1_Click(System::Object^  sender, System::EventArgs^  e) {
		if (n_raw_data_point[0] > min_n[0] - 1 && n_raw_data_point[0] < max_n[0])
			n_raw_data_point[0]--;
		do_raw_chart(0, n_raw_data_point[0]);
		chart1->ChartAreas["ChartArea1"]->CursorX->Position = n_raw_data_point[0];
		cout << "N1 " << n_raw_data_point[0] << endl;
	}

	private: System::Void button_back_2_Click(System::Object^  sender, System::EventArgs^  e) {
		if (n_raw_data_point[1] > min_n[1] - 1 && n_raw_data_point[1] < max_n[1])
			n_raw_data_point[1]--;
		do_raw_chart(1, n_raw_data_point[1]);
		chart2->ChartAreas["ChartArea1"]->CursorX->Position = n_raw_data_point[1];
		cout << "N2 " << n_raw_data_point[1] << endl;
	}

	private: System::Void button_forward_1_Click(System::Object^  sender, System::EventArgs^  e) {
		if (n_raw_data_point[0] > min_n[0] && n_raw_data_point[0] < max_n[0] - 1)
			n_raw_data_point[0]++;
		do_raw_chart(0, n_raw_data_point[0]);
		chart1->ChartAreas["ChartArea1"]->CursorX->Position = n_raw_data_point[0];
		cout << "N1 " << n_raw_data_point[0] << endl;
	}

	private: System::Void button_forward_2_Click(System::Object^  sender, System::EventArgs^  e) {
		if (n_raw_data_point[1] > min_n[1] && n_raw_data_point[1] < max_n[1] - 1)
			n_raw_data_point[1]++;
		do_raw_chart(1, n_raw_data_point[1]);
		chart2->ChartAreas["ChartArea1"]->CursorX->Position = n_raw_data_point[1];
		cout << "N2 " << n_raw_data_point[1] << endl;
	}

	private: System::Void N_Raw_Data_point_1_Click(System::Object^  sender, System::EventArgs^  e) {
		if (cursor_x_position[0] > min_n[0] && cursor_x_position[0] < max_n[0])
			n_raw_data_point[0] = cursor_x_position[0];
		do_raw_chart(0, n_raw_data_point[0]);
		chart1->ChartAreas["ChartArea1"]->CursorX->Position = n_raw_data_point[0];
	}

	private: System::Void N_Raw_Data_point_2_Click(System::Object^  sender, System::EventArgs^  e) {
		if (cursor_x_position[1] > min_n[1] && cursor_x_position[1] < max_n[1])
			n_raw_data_point[1] = cursor_x_position[1];
		do_raw_chart(1, n_raw_data_point[1]);
		chart2->ChartAreas["ChartArea1"]->CursorX->Position = n_raw_data_point[1];
	}

	private: System::Void toolStripComboBox_sonde_type_SelectedIndexChanged(System::Object^  sender, System::EventArgs^  e) {
		if (toolStripComboBox_sonde_type->SelectedItem == "AUTONOM_5Tx") {
			sonde_type = AUTONOM_5Tx_;
			Dataout = "C:\\EXP\\Dataout_AUTONOM_5Tx.bin";
			baudrate = 125000;
		}
		if (toolStripComboBox_sonde_type->SelectedItem == "AUTONOM_5Tx_SDR") {
			sonde_type = AUTONOM_5Tx_SDR_;
			Dataout = "C:\\EXP\\Dataout_AUTONOM_5Tx_SDR.bin";
			baudrate = 125000;
		}
		if (toolStripComboBox_sonde_type->SelectedItem == "LWD_4Tx") {
			sonde_type = LWD_4Tx_;
			Dataout = "C:\\EXP\\Dataout_LWD_4Tx.bin";
			baudrate = 125000;
		}

		if (toolStripComboBox_sonde_type->SelectedItem == "AUTONOM_5Tx"
			|| toolStripComboBox_sonde_type->SelectedItem == "AUTONOM_5Tx_SDR"
			|| toolStripComboBox_sonde_type->SelectedItem == "LWD_4Tx"
			) {
			tableLayoutPanel1->ColumnStyles[1]->Width = 0;
			this->Width = form_width;

			if (index_dir_undir[up_chart] == direct) {
				button_direct1->PerformClick();
			}
			if (index_dir_undir[down_chart] == direct) {
				button_direct2->PerformClick();
			}
			button_direct1->Visible = false;
			button_direct2->Visible = false;
		}
		if (toolStripComboBox_sonde_type->SelectedItem == "CARTOGRAPH") {
			sonde_type = CARTOGRAPH_;
			Dataout = "C:\\EXP\\Dataout_cartograf.bin";
			tableLayoutPanel1->ColumnStyles[1]->Width = 450;
			this->Width = form_width + 450;
			button_direct1->Visible = true;
			button_direct2->Visible = true;
			baudrate = 125000;
		}

		if (metro_form_is_open == true) {
			Metro->Close();
			metro_form_is_open = false;
			toolStripButton_metro_file->PerformClick();
			this->Activate();
		}


	}

	private: System::Void MyForm_Resize(System::Object^  sender, System::EventArgs^  e) {
		if (toolStripComboBox_sonde_type->SelectedItem == "AUTONOM_5Tx"
			|| toolStripComboBox_sonde_type->SelectedItem == "AUTONOM_5Tx_SDR"
			|| toolStripComboBox_sonde_type->SelectedItem == "LWD_4Tx"
			) {
			form_width = this->Width;

		}
		if (toolStripComboBox_sonde_type->SelectedItem == "CARTOGRAPH") {
			form_width = this->Width - 450;
		}
	}

			 void do_ph_shift(void) {
				 float Ro;
				 extern float air[2][5];

				 if (trackBar1->Value == 0) Ro = 1.0f;
				 if (trackBar1->Value == 1) Ro = 5.0f;
				 if (trackBar1->Value == 2) Ro = 10.0f;
				 if (trackBar1->Value == 3) Ro = 25.0f;
				 if (trackBar1->Value == 4) Ro = 50.0f;
				 if (trackBar1->Value == 5) Ro = 100.0f;
				 if (trackBar1->Value == 6) Ro = 250.0f;
				 if (trackBar1->Value == 7) Ro = 500.0f;
				 if (trackBar1->Value == 8) Ro = 1000.0f;
				 if (trackBar1->Value == 9) Ro = 1500.0f;
				 if (trackBar1->Value == 10) Ro = 2000.0f;
				 if (trackBar1->Value == 11) Ro = 10000.0f;

				 if (trackBar1->Value == 11)
					 label_ro->Text = "Ro = infinity";
				 else
					 label_ro->Text = "Ro = " + Ro + " Om";

				 for (int i = 0; i < 5; i++) {
					 TextBox^ TEXTBOX;
					 TEXTBOX = (TextBox^)panel1->Controls["textBox_T" + (i + 1)];
					 if (TEXTBOX->Text != "" && TEXTBOX->Text != "-") air[index_freq[0]][i] = (System::Int64::Parse(TEXTBOX->Text)) / mG;
					 else
						 air[index_freq[0]][i] = 0;
				 }

				 int N_Tx = 4;
				 if (metro_is_readed_from_sonde == true && metrology_GP_struct.signature == global_signature) {
					 for (int freq = 0; freq < 2; freq++) {
						 for (int Tx = 0; Tx < N_Tx; Tx++) {
							 ph_shift.d_fi[freq][Tx] = SIGNAL(&metrology_GP_struct, Tx, freq, Ro) - air[index_freq[0]][Tx];
						 }
					 }
				 }
			 }

	private: System::Void trackBar1_Scroll(System::Object^  sender, System::EventArgs^  e) {
		do_ph_shift();
	}
	private: System::Void textBox_T1_TextChanged(System::Object^  sender, System::EventArgs^  e) {
		do_ph_shift();
	}
	private: System::Void textBox_T2_TextChanged(System::Object^  sender, System::EventArgs^  e) {
		do_ph_shift();
	}
	private: System::Void textBox_T3_TextChanged(System::Object^  sender, System::EventArgs^  e) {
		do_ph_shift();
	}
	private: System::Void textBox_T4_TextChanged(System::Object^  sender, System::EventArgs^  e) {
		do_ph_shift();
	}
	private: System::Void textBox_T5_TextChanged(System::Object^  sender, System::EventArgs^  e) {
		do_ph_shift();
	}

	private: System::Void toolStripMenuItemT1_Click(System::Object^  sender, System::EventArgs^  e) {
		textBox_T1->Text = ((int)chart1->ChartAreas["ChartArea1"]->CursorY->Position).ToString("F0");
	}
	private: System::Void toolStripMenuItemT2_Click(System::Object^  sender, System::EventArgs^  e) {
		textBox_T2->Text = ((int)chart1->ChartAreas["ChartArea1"]->CursorY->Position).ToString("F0");
	}
	private: System::Void toolStripMenuItemT3_Click(System::Object^  sender, System::EventArgs^  e) {
		textBox_T3->Text = ((int)chart1->ChartAreas["ChartArea1"]->CursorY->Position).ToString("F0");
	}
	private: System::Void toolStripMenuItemT4_Click(System::Object^  sender, System::EventArgs^  e) {
		textBox_T4->Text = ((int)chart1->ChartAreas["ChartArea1"]->CursorY->Position).ToString("F0");
	}
	private: System::Void toolStripMenuItemT5_Click(System::Object^  sender, System::EventArgs^  e) {
		textBox_T5->Text = ((int)chart1->ChartAreas["ChartArea1"]->CursorY->Position).ToString("F0");
	}

	private: System::Void toolStripMenuItemmin1_Click(System::Object^  sender, System::EventArgs^  e) {
		textBox_min1->Text = ((uint32_t)chart1->ChartAreas["ChartArea1"]->CursorX->Position).ToString("F0");
	}
	private: System::Void toolStripMenuItemmin2_Click(System::Object^  sender, System::EventArgs^  e) {
		textBox_min2->Text = ((uint32_t)chart2->ChartAreas["ChartArea1"]->CursorX->Position).ToString("F0");
	}
	private: System::Void toolStripMenuItemmax1_Click(System::Object^  sender, System::EventArgs^  e) {
		textBox_max1->Text = ((uint32_t)chart1->ChartAreas["ChartArea1"]->CursorX->Position).ToString("F0");
	}
	private: System::Void toolStripMenuItemmax2_Click(System::Object^  sender, System::EventArgs^  e) {
		textBox_max2->Text = ((uint32_t)chart2->ChartAreas["ChartArea1"]->CursorX->Position).ToString("F0");
	}


	};

}

