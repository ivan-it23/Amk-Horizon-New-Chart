//{{NO_DEPENDENCIES}}
// Включаемый файл, созданный в Microsoft Visual C++.
// Используется Amk-Horizon-New-Chart.rc

#define IDS_APP_TITLE			103

#define IDR_MAINFRAME			128
#define IDD_AMKHORIZONNEWCHART_DIALOG	102
#define IDD_ABOUTBOX			103
#define IDM_ABOUT				104
#define IDM_EXIT				105
#define IDM_OPEN_FILE			106
// ID 107-108 зарезервированы (ранее использовались для удаленных пунктов меню "Вид")
#define IDI_AMKHORIZONNEWCHART			109
#define IDI_SMALL				110
#define IDC_AMKHORIZONNEWCHART			111
#define IDC_MYICON				2

// Элементы управления
#define IDC_DUAL_MODE_CHECK		1001
// ID 1002-1007 зарезервированы (ранее использовались для удаленных элементов)

// Скроллбары для графиков (горизонтальные и вертикальные)
#define IDC_CHART1_HSCROLL		1008
#define IDC_CHART2_HSCROLL		1009
#define IDC_CHART1_VSCROLL		1041
#define IDC_CHART2_VSCROLL		1042

// Кнопки схемы прибора
#define IDC_CHART1_SCHEMATIC_BTN	1010
#define IDC_CHART2_SCHEMATIC_BTN	1011

// Диалоговое окно схемы прибора
#define IDD_SCHEMATIC_DIALOG		1012
#define IDC_SCHEMATIC_AREA			1013
#define IDC_FREQUENCY_COMBO			1014
#define IDC_CURVE_LIST				1015
#define IDC_PRESET_ALL_AMP_R1		1016
#define IDC_PRESET_ALL_AMP_R2		1017
#define IDC_PRESET_RATIOS			1020
#define IDC_PRESET_DIFFERENCES		1021
// ID 1022 зарезервирован (ранее использовался для IDC_COPY_FROM_CHART, удалён в v2.9.6)
#define IDC_APPLY_SETTINGS			1023

// Элементы управления X Range в панели инструментов (Chart 1)
#define IDC_XRANGE_FROM_EDIT		1024
#define IDC_XRANGE_TO_EDIT			1025
#define IDC_XRANGE_APPLY_BTN		1026
#define IDC_XRANGE_RESET_BTN		1027
#define IDC_XRANGE_FROM_LABEL		1028
#define IDC_XRANGE_TO_LABEL			1029

// Элементы управления X Range для Chart 2
#define IDC_XRANGE2_FROM_EDIT		1030
#define IDC_XRANGE2_TO_EDIT			1031
#define IDC_XRANGE2_APPLY_BTN		1032
#define IDC_XRANGE2_RESET_BTN		1033
#define IDC_XRANGE2_FROM_LABEL		1034
#define IDC_XRANGE2_TO_LABEL		1035

// Кнопки сохранения диапазона
#define IDC_SAVE_RANGE1_BTN			1036
// ID 1037 зарезервирован (ранее использовался для IDC_SAVE_RANGE2_BTN, удалён в v2.7.1)

// Статусная строка
#define IDC_STATUSBAR				1038

// Пресет температуры (v2.9.7)
#define IDC_PRESET_TEMPERATURE		1039

// Пресеты УЭС и симметризованных фаз (v2.16.0)
#define IDC_PRESET_RHO_SMT			1083
#define IDC_PRESET_PHASE_SMT		1084

// Пресеты молчащих передатчиков
#define IDC_PRESET_SILENT_AMPL		1085
#define IDC_PRESET_SILENT_DPH		1086

// Выбор типа оси X (v2.10.0)
#define IDC_XAXIS_TYPE_COMBO		1040

// Кнопки сброса zoom
#define IDC_RESET_ZOOM1_BTN			1043
#define IDC_RESET_ZOOM2_BTN			1044

// Элементы отображения координат курсора (Chart 1) - новый компактный дизайн v2.15.1
// Формат: 📍 [X] : [Y1] | [Y2] (Y2 только для двойных осей)
#define IDC_CURSOR1_ICON			1045
#define IDC_CURSOR1_X_EDIT			1046
#define IDC_CURSOR1_SEPARATOR		1047
#define IDC_CURSOR1_Y1_EDIT			1048
#define IDC_CURSOR1_Y2_SEPARATOR	1049
#define IDC_CURSOR1_Y2_EDIT			1081

// Элементы отображения координат курсора (Chart 2) - новый компактный дизайн v2.15.1
#define IDC_CURSOR2_ICON			1050
#define IDC_CURSOR2_X_EDIT			1051
#define IDC_CURSOR2_SEPARATOR		1052
#define IDC_CURSOR2_Y1_EDIT			1053
#define IDC_CURSOR2_Y2_SEPARATOR	1054
#define IDC_CURSOR2_Y2_EDIT			1082

// === Меню и диалог COM-порта (v2.13.0) ===
#define IDM_DEVICE_CONNECT			107
#define IDM_DEVICE_DISCONNECT		108
#define IDM_DEVICE_START			109
#define IDM_DEVICE_STOP				110
#define IDM_DEVICE_CLEAR			111
#define IDD_SERIAL_STATUS			1055

// Элементы диалога статуса COM-порта
#define IDC_SERIAL_PORT_COMBO		1060
#define IDC_SERIAL_CONNECT_BTN		1061
#define IDC_SERIAL_STATUS_LABEL		1062
#define IDC_SERIAL_START_BTN		1063
#define IDC_SERIAL_STOP_BTN			1064
#define IDC_SERIAL_CLEAR_BTN		1065
#define IDC_SERIAL_FRAME_LABEL		1066
#define IDC_SERIAL_PACKETS_LABEL	1067
#define IDC_SERIAL_INFO_LABEL		1068

// === Элементы управления режимом Surf (v2.14.0) ===
#define IDC_CHART1_SURF_CHECK		1070
#define IDC_CHART1_SURF_EDIT		1071
#define IDC_CHART2_SURF_CHECK		1073
#define IDC_CHART2_SURF_EDIT		1074

// === Элементы записи в файл (v2.15.0) ===
#define IDC_SERIAL_RECORD_CHECK		1076
#define IDC_SERIAL_RECORD_STATUS	1077
#define IDC_SERIAL_RECORD_FRAMES	1078
#define IDC_SERIAL_BROWSE_BTN		1079
#define IDC_SERIAL_OPEN_FOLDER_BTN	1080

// === Заметки на графике (v2.16.x) ===
#define IDD_NOTE_DIALOG				1087
#define IDC_NOTE_TEXT_EDIT			1088
#define IDD_NOTE_VIEW_DIALOG		1089
#define IDC_NOTE_VIEW_TEXT_EDIT		1090
#define IDC_NOTE_DELETE_BTN			1091

// === Меню записи (v2.15.0) ===
#define IDM_DEVICE_RECORD_FOLDER	112
#define IDM_DEVICE_OPEN_FOLDER		113

#ifndef IDC_STATIC
#define IDC_STATIC				-1
#endif
// Следующие стандартные значения для новых объектов
//
#ifdef APSTUDIO_INVOKED
#ifndef APSTUDIO_READONLY_SYMBOLS

#define _APS_NO_MFC					130
#define _APS_NEXT_RESOURCE_VALUE	129
#define _APS_NEXT_COMMAND_VALUE		32771
#define _APS_NEXT_CONTROL_VALUE		1008
#define _APS_NEXT_SYMED_VALUE		112
#endif
#endif
