# Поддержка `.bin`, `.biv`, `.DEV` в исторической модели `_5Tx_DATA`

## Назначение документа

Этот файл сохранён как **историко-справочный мост** между старой терминологией `_5Tx_DATA` и текущей adapter-driven архитектурой.

Он **не является главным документом** по текущей реализации. Для current-state используйте:

- `FILE_FORMATS_ANALYSIS.md`
- `FILE_VALIDATION_FOR_5TX_MODEL.md`
- `Amk-Horizon-New-Chart/ANALYSIS_GP_TOOL_DATA.md`

## 1. Что из старой модели осталось актуальным

Из legacy-контекста по-прежнему важны только три факта:

- `.biv` по контейнерной логике эквивалентен `.bin`;
- `.DEV` добавляет `11` байт заголовка перед payload;
- тип прибора определяется по `signature`.

Поддерживаемые семейства и сейчас те же:

- `151` -> `AUTONOM_5TX`
- `152` -> `AUTONOM_5TX_SDR`
- `241` -> `LWD_4TX`

## 2. Как это выглядит в текущем проекте

Сейчас проект уже не строится вокруг “одной модели `_5Tx_DATA` для всего”.

Текущая схема:

`container -> signature -> adapter -> ParsedToolFrame -> canonical GP_TOOL_DATA + ToolRecordMetadata`

То есть:

- `AUTONOM_5TX` и `AUTONOM_5TX_SDR` adapter-level близки к canonical 5Tx shape;
- `LWD_4TX` идёт через отдельный adapter path;
- после чтения все данные materialize-ятся в canonical `GP_TOOL_DATA`.

## 3. Что в имени файла теперь считать условностью

Историческое имя файла может вводить в заблуждение.

Фактически текущая поддержка:

- относится не только к `5Tx`;
- охватывает и `4Tx`, и `5Tx`;
- описывается не `_5Tx_DATA` как основной runtime-моделью, а adapter-driven parsing + canonical shape.

## 4. Практический current-state вывод

Если вам нужен ответ на вопрос “как сейчас реально читаются `.bin/.biv/.DEV`”, этот файл **не должен быть первым источником**.

Правильный порядок чтения документации:

1. `FILE_FORMATS_ANALYSIS.md`
2. `FILE_VALIDATION_FOR_5TX_MODEL.md`
3. `Amk-Horizon-New-Chart/ANALYSIS_GP_TOOL_DATA.md`
4. только потом этот файл как исторический комментарий

## 5. Зачем файл оставлен в репозитории

Он полезен, потому что:

- объясняет происхождение названий `_5Tx_DATA`, `AUTONOM_5Tx`, `AUTONOM_5Tx_SDR`;
- помогает понять, почему часть старых обсуждений ссылается именно на `_5Tx_DATA`;
- связывает старую терминологию с текущим canonical `GP_TOOL_DATA`.

## 6. Источники истины

- current-state — `FILE_FORMATS_ANALYSIS.md`, `FILE_VALIDATION_FOR_5TX_MODEL.md`, `Amk-Horizon-New-Chart/ANALYSIS_GP_TOOL_DATA.md`
- исторический контекст — этот файл

Если между ними есть расхождения, для текущего поведения приложения нужно ориентироваться на код и current-state документы.
