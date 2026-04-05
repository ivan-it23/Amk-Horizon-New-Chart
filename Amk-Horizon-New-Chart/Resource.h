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
#define IDC_CUT_RANGE1_BTN          1109
// ID 1037 зарезервирован (ранее использовался для IDC_SAVE_RANGE2_BTN, удалён в v2.7.1)

// Статусная строка
#define IDC_STATUSBAR				1038

// Пресет температуры (v2.9.7)
#define IDC_PRESET_TEMPERATURE		1039

// Пресеты УЭС и симметризованных фаз (v2.16.0)
#define IDC_PRESET_RHO_SMT			1083
#define IDC_PRESET_PHASE_SMT		1084
#define IDC_RHO_SMT_FROM_SONDE		1102
#define IDC_RHO_SMT_FROM_COMPUTER	1103
#define IDC_PHASE_SMT_FROM_SONDE	1104
#define IDC_PHASE_SMT_FROM_COMPUTER	1105

// Пресеты молчащих передатчиков
#define IDC_PRESET_SILENT_AMPL		1085
#define IDC_PRESET_SILENT_DPH		1086

// Выбор типа оси X (v2.10.0)
#define IDC_XAXIS_TYPE_COMBO		1040

// Кнопки сброса zoom
#define IDC_RESET_ZOOM1_BTN			1043
#define IDC_RESET_ZOOM2_BTN			1044

// Кнопки осевого undo ('-') у скроллбаров
#define IDC_CHART1_HUNDO_BTN       1098
#define IDC_CHART1_VUNDO_BTN       1099
#define IDC_CHART2_HUNDO_BTN       1100
#define IDC_CHART2_VUNDO_BTN       1101

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
#define IDM_DEVICE_METROLOGY        114
#define IDM_NOTES_JOURNAL			115
#define IDM_CHART_ADD_NOTE         1112
#define IDM_CHART_SET_X_FROM       1113
#define IDM_CHART_SET_X_TO         1114
#define IDD_SERIAL_STATUS			1055

// Элементы диалога статуса COM-порта
#define IDC_SERIAL_PORT_COMBO		1060
#define IDC_SERIAL_CONNECT_BTN		1061
#define IDC_SERIAL_STATUS_LABEL		1062
#define IDC_SERIAL_START_BTN		1063
#define IDC_SERIAL_STOP_BTN			1064
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
#define IDC_SERIAL_APPEND_CHECK		1081
#define IDC_SERIAL_APPEND_SHOW_FULL_CHECK	1082
#define IDC_SERIAL_POLL_INTERVAL_EDIT	1083
#define IDC_SERIAL_POLL_INTERVAL_LABEL	1084

// === Заметки на графике (v2.16.x) ===
#define IDD_NOTE_DIALOG				1087
#define IDC_NOTE_TEXT_EDIT			1088
#define IDD_NOTE_VIEW_DIALOG		1089
#define IDC_NOTE_VIEW_TEXT_EDIT		1090
#define IDC_NOTE_DELETE_BTN			1091
#define IDD_NOTES_JOURNAL_DIALOG    1093
#define IDC_NOTES_JOURNAL_LIST      1094
#define IDC_NOTES_APPLY_BTN         1095
#define IDC_NOTES_REFRESH_BTN       1096
#define IDC_NOTES_INFO_LABEL        1097
#define IDC_CHART1_SHOW_NOTES_CHECK 1106
#define IDC_CHART2_SHOW_NOTES_CHECK 1107
#define IDC_NOTE_SAVE_BTN          1108
#define IDC_CHART1_UNDO_ZOOM_BTN   1110
#define IDC_CHART2_UNDO_ZOOM_BTN   1111

// === Переключатель цели Y-zoom для dual-axis (v2.18.0) ===
#define IDC_CHART1_YZOOM_TARGET_COMBO  1115
#define IDC_CHART2_YZOOM_TARGET_COMBO  1116
#define IDC_CHART1_YZOOM_LABEL         1117
#define IDC_CHART2_YZOOM_LABEL         1118

// Кнопка-переключатель Старт/Стоп опроса (панель Chart 1)
#define IDC_POLLING_TOGGLE_BTN         1119

// Кнопка-переключатель Подключить/Отключить прибор (панель Chart 1)
#define IDC_CONNECT_TOGGLE_BTN         1120

// Сообщение главному окну: открыть диалог «Подключение к прибору» (ПКМ по кнопке «Включить»)
#define WM_APP_OPEN_SERIAL_CONNECTION    (WM_APP + 27)

// Сообщение главному окну: открыть в проводнике папку текущего файла данных (клик по иконке в статусной строке)
#define WM_APP_OPEN_CURRENT_FILE_FOLDER  (WM_APP + 28)

// Журнал обмена с прибором (немодальная панель + развёрнутое окно)
#define WM_APP_DEVICE_LOG_REFRESH (WM_APP + 36)
// Дебаунс обновления UI лога (не пересекать с IDT_SERIAL_POLL 9001 в SerialModule.h)
#define IDT_DEVICE_LOG_REFRESH       9002
#define IDM_DEVICE_LOG               1121
#define IDC_DEVICE_LOG_EXPAND      1122
#define IDC_DEVICE_LOG_CLEAR       1123
#define IDC_DEVICE_LOG_EDIT        1124
#define IDC_DEVICE_LOG_EDIT_EXPANDED 1125
#define IDC_DEVICE_LOG_COLLAPSE    1126
#define IDC_DEVICE_LOG_RAW_CHECK     1127

// === Метрология (AUTONOM 5Tx) ===
#define IDD_METROLOGY_DIALOG        1092

#define IDC_METRO_READONLY_CHECK    1200

// L1/L2 (T1..T5)
#define IDC_METRO_L1_T1             1201
#define IDC_METRO_L1_T2             1202
#define IDC_METRO_L1_T3             1203
#define IDC_METRO_L1_T4             1204
#define IDC_METRO_L1_T5             1205
#define IDC_METRO_L2_T1             1206
#define IDC_METRO_L2_T2             1207
#define IDC_METRO_L2_T3             1208
#define IDC_METRO_L2_T4             1209
#define IDC_METRO_L2_T5             1210

// Frequencies
#define IDC_METRO_F1                1211
#define IDC_METRO_F2                1212

// AIR (10 fields) and MIN (10 fields)
#define IDC_METRO_AIR_1             1220
#define IDC_METRO_AIR_2             1221
#define IDC_METRO_AIR_3             1222
#define IDC_METRO_AIR_4             1223
#define IDC_METRO_AIR_5             1224
#define IDC_METRO_AIR_6             1225
#define IDC_METRO_AIR_7             1226
#define IDC_METRO_AIR_8             1227
#define IDC_METRO_AIR_9             1228
#define IDC_METRO_AIR_10            1229

#define IDC_METRO_MIN_1             1230
#define IDC_METRO_MIN_2             1231
#define IDC_METRO_MIN_3             1232
#define IDC_METRO_MIN_4             1233
#define IDC_METRO_MIN_5             1234
#define IDC_METRO_MIN_6             1235
#define IDC_METRO_MIN_7             1236
#define IDC_METRO_MIN_8             1237
#define IDC_METRO_MIN_9             1238
#define IDC_METRO_MIN_10            1239

// Signature/identification panel
#define IDC_METRO_TYPE_COMBO        1240
#define IDC_METRO_NTX_COMBO         1241
#define IDC_METRO_MOD_COMBO         1242
#define IDC_METRO_NUMBER_EDIT       1243
#define IDC_METRO_SERIAL_EDIT       1244
#define IDC_METRO_DIAMETER_EDIT     1245
#define IDC_METRO_SIGNATURE32_EDIT  1246
#define IDC_METRO_SIGNATUREPAL_EDIT 1247

// Rx_Position
#define IDC_METRO_RX_POSITION_COMBO 1248

// Buttons
#define IDC_METRO_BTN_OPEN_FILE     1251
#define IDC_METRO_BTN_SAVE_FILE     1252
#define IDC_METRO_BTN_READ_SONDE    1253
#define IDC_METRO_BTN_WRITE_SONDE   1254

// === Меню записи (v2.15.0) ===
#define IDM_DEVICE_RECORD_FOLDER	112
#define IDM_DEVICE_OPEN_FOLDER		113

// === v2.19.0: Параллельная запись COM + просмотр файла ===
#define IDM_RETURN_TO_COM			116
#define IDC_RETURN_TO_COM_BTN		1290
#define IDC_BG_RECORD_STATUS		1291

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
