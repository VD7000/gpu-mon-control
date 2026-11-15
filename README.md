# gpu-mon-control

Фоновый Windows daemon для масштабирования движения мыши, переключения GPU-профилей и управления topology дисплеев.

Проект собирается под Windows с использованием C++14 и Win32 API.

## Режимы запуска

### Daemon-режим

Запускается без аргументов:

```cmd
gpu-mon-control.exe
```

В daemon-режиме приложение:

- создаёт именованный mutex для защиты от второго экземпляра;
- создаёт скрытое top-level окно Win32;
- устанавливает низкоуровневый mouse hook;
- создаёт иконку в System Tray;
- регистрирует глобальную горячую клавишу;
- запускает цикл обработки Windows-сообщений.

Консольное окно не создаётся.

### CLI-режим

При наличии аргументов daemon-инфраструктура не запускается. Не создаются mutex, скрытое окно, tray icon и mouse hook.

Команда выполняется синхронно, после чего процесс завершается с exit code.

## CLI-команды

Показать справку:

```cmd
gpu-mon-control.exe --help
```

Создать или перезаписать конфигурацию:

```cmd
gpu-mon-control.exe --init-config
```

Переключиться на конкретный профиль:

```cmd
gpu-mon-control.exe switch RTX3060
```

Переключиться на следующий профиль:

```cmd
gpu-mon-control.exe toggle
```

Verbose-режим:

```cmd
gpu-mon-control.exe --verbose --help
```

В CLI-режиме приложение пытается подключиться к консоли родительского процесса через `AttachConsole(ATTACH_PARENT_PROCESS)`.

## Конфигурация

Текущая конфигурация хранится в `config.yaml` рядом с исполняемым файлом.

Пример:

```yaml
general:
  toggle_hotkey: "Ctrl+Alt+G"
  base_resolution_x: 2560
  base_resolution_y: 1080

gpus:
  RTX3060:
    adapter_id: "HighPerfAdapter=10DE&2503&87F51043;"
    topology: "internal"
  GTX1060:
    adapter_id: "HighPerfAdapter=10DE&1C03&37161458;"
    topology: "external"
```

### Секция `general`

- `toggle_hotkey` — необязательная глобальная комбинация переключения профилей;
- `base_resolution_x` — базовая ширина эталонного разрешения;
- `base_resolution_y` — базовая высота эталонного разрешения.

Если базовое разрешение не указано, используются значения по умолчанию `2560x1080`.

### Секция `gpus`

Каждый профиль содержит:

- `adapter_id` — строка DirectX `HighPerfAdapter`;
- `topology` — режим дисплеев.

Поддерживаемые topology:

```text
internal
external
extend
```

При отсутствии `config.yaml` приложение автоматически создаёт шаблон.

## Переключение GPU и дисплеев

`GpuSwitcher` работает с ключом:

```text
HKCU\Software\Microsoft\DirectX\UserGpuPreferences
```

Используется значение:

```text
DirectXUserGlobalSettings
```

При команде `switch` выполняются следующие операции:

1. профиль загружается из `config.yaml`;
2. его `adapter_id` записывается в реестр через `RegSetValueExW`;
3. его topology применяется через `SetDisplayConfig`.

Соответствие topology:

| YAML | Win32 |
|---|---|
| `internal` | `SDC_TOPOLOGY_INTERNAL` |
| `external` | `SDC_TOPOLOGY_EXTERNAL` |
| `extend` | `SDC_TOPOLOGY_EXTEND` |

Команда `toggle` читает текущий `adapter_id`, находит активный профиль и выбирает следующий профиль циклически.

Уже запущенные приложения могут не сменить GPU до перезапуска.

## MouseScaler

`MouseScaler` реализован в файлах:

```text
MouseScaler.h
MouseScaler.cpp
```

Функции модуля:

- установка `WH_MOUSE_LL`;
- отслеживание `WM_MOUSEMOVE`;
- проверка видимости системного курсора;
- коррекция движения мыши;
- защита от рекурсивных событий после `SetCursorPos`;
- включение и отключение hook во время работы.

Масштаб пересчитывается:

- при запуске daemon;
- при получении `WM_DISPLAYCHANGE`;
- после изменения базового разрешения через reload конфигурации.

Скрытое окно является обычным top-level окном с `WS_OVERLAPPED`, поэтому оно получает системные broadcast-сообщения `WM_DISPLAYCHANGE`.

## System Tray

Tray icon создаётся через `Shell_NotifyIconW` с системной иконкой `IDI_APPLICATION`.

Tooltip имеет вид:

```text
gpu-mon-control | GPU: RTX3060 | Scale: Active
```

Контекстное меню содержит:

- `Toggle GPU`;
- список GPU-профилей;
- `Mouse Scaler Active`;
- `Reload config.yaml`;
- `Open config.yaml`;
- `Exit`.

Активный профиль отмечается флагом `MF_CHECKED`.

При завершении вызывается `Shell_NotifyIconW(NIM_DELETE, ...)`.

## Reload конфигурации

Пункт `Reload config.yaml` перечитывает файл без перезапуска daemon.

При успешной загрузке:

1. текущая конфигурация заменяется новой;
2. hotkey перерегистрируется;
3. базовое разрешение применяется к `MouseScaler`;
4. масштаб пересчитывается;
5. tooltip tray обновляется;
6. новые профили появляются при следующем открытии меню.

При ошибке старая конфигурация сохраняется.

Reload не переключает GPU автоматически. Для применения профиля необходимо выбрать его в tray или выполнить CLI-команду `switch`/`toggle`.

## Горячая клавиша

Поддерживаются модификаторы:

```text
Ctrl
Alt
Shift
Win
```

Примеры:

```yaml
toggle_hotkey: "Ctrl+Alt+G"
toggle_hotkey: "Shift+F8"
toggle_hotkey: "Win+F12"
```

При получении `WM_HOTKEY` выполняется тот же flow, что и у команды `toggle`.

Горячая клавиша работает только если:

- daemon запущен без аргументов;
- конфигурация успешно загружена;
- комбинация не занята другим приложением;
- daemon и пользователь находятся в одной интерактивной Windows-сессии.

Запуск от администратора обычно не требуется.

## Single Instance

Daemon использует именованный mutex:

```text
GpuMonControl_MouseScaler
```

Если другой daemon уже запущен, второй экземпляр завершается.

CLI-команды mutex не используют.

## Структура модулей

| Модуль | Назначение |
|---|---|
| `main.cpp` | `wWinMain`, маршрутизация daemon/CLI, hidden window, tray callbacks |
| `CliDispatcher` | Обработка CLI-команд |
| `ConfigManager` | Чтение и запись `config.yaml` |
| `GpuSwitcher` | Чтение и запись DirectX GPU preference в реестр |
| `DisplayController` | Применение topology через CCD API |
| `MouseScaler` | Низкоуровневый mouse hook и масштабирование |
| `TrayController` | System Tray icon и tooltip |

## Права доступа и безопасность

Приложение не требует обязательного запуска от администратора для:

- mouse hook;
- global hotkey;
- работы с `HKCU`;
- чтения конфигурации;
- tray icon.

Дополнительные права могут потребоваться политиками Windows или при работе с приложениями другого integrity level.

`config.yaml` управляет registry-параметром GPU и topology дисплеев, поэтому каталог с приложением не должен быть доступен для записи недоверенным пользователям.

## Ограничения

- используется `config.yaml`, а не `config.ini`;
- автоматического file watcher нет, reload выполняется вручную через tray;
- `adapter_id` должен соответствовать формату DirectX GPU preference;
- уже запущенные приложения могут потребовать перезапуска для выбора другого GPU;
- topology применяется только если `SetDisplayConfig` принимает выбранный режим.

## Сборка

Проект предназначен для Visual Studio с C++14 и Win32 API.

После сборки рядом с `.exe` должен находиться:

```text
config.yaml
```

Для запуска тестовых сценариев доступны batch-файлы в `x64\Debug`:

```text
test-help.bat
test-verbose-help.bat
init-config.bat
switch-rtx3060.bat
toggle-gpu.bat
start-daemon.bat
```
