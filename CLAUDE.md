# Карта проекта и руководство для агента

Карта проверена по рабочему дереву 2026-11-04. Это навигатор по коду: перед изменением сверяй детали с текущими исходниками. При добавлении подсистем, переносе файлов или изменении сборки обновляй соответствующие разделы этого файла.

> **Правило синхронизации:** `AGENTS.md` и `CLAUDE.md` в корне проекта всегда должны быть побайтово одинаковыми. Любую правку вноси сразу в оба файла (или правь один и копируй его поверх другого) в рамках одного изменения. Проверка: `git diff --no-index AGENTS.md CLAUDE.md` не должен выводить различий.

## Быстрый старт

- VirtualLifeSimulator — настольная Windows-песочница искусственной жизни: клеточный мир, боты с наследуемой нейросетью, питание, размножение, мутации и эксперименты с условиями среды.
- Язык — **C++20**, сборка — **Visual Studio / MSBuild**, toolset **v143**, Windows SDK 10.0. Имя родительской папки `C# projects` не отражает язык проекта.
- Решение: `VirtualLifeSimulator1.sln`; фактический список единиц компиляции и настройки: `VirtualLifeSimulator1.vcxproj`.
- **Два активных движка в одном приложении:** Classic в `source/`, CyberBiology3 в `source/cb3/` (namespace `cb3`). Файлы в `cb3` не являются архивом или ненужными дублями.
- Общий интерфейс и графики живут в namespace `vlsui`; SDL-окно и renderer общие для обоих движков.
- Начни с `git status --short`: в рабочем дереве могут быть незакоммиченные изменения пользователя. Сохраняй их и не откатывай несвязанные файлы.
- Для правил симуляции сначала определи нужный режим. Для общей функции интерфейса проверь оба режима и общие компоненты.
- Пользовательское описание управления и экспериментов: `HOW_TO_PLAY.md`. Описание тестов: `tests/README.md`. Зависимости: `external/README.md`.

## Архитектура и поток выполнения

```text
source/Main.cpp: wWinMain
  ├─ SDL.cpp: окно, renderer, ввод
  ├─ GUI.cpp: InitImGUI, ImGui / ImPlot
  └─ глобальный Main simulation: цикл событий, шагов и отрисовки
       ├─ Classic: Main → Field::tick → Object::tick / Bot::tick
       │                                └─ BotNeuralNet → Neuron
       └─ CyberBiology3: Cb3Runtime → cb3::Main → cb3::Field → cb3::Bot

Оба режима:
  GUI.cpp своего режима → общие UILayout / UITheme / BrainWindow
  NeuralNetRenderer своего режима → общий NeuralGraphView
  Main::RecordChartSample → Field::ReadTelemetry → EcosystemChart
  Main: файловый интерфейс → ObjectSaver своего режима → SavedObjects/
```

`Main` объявлен в `Main.h`, но его реализация разделена между `Main.cpp` и `GUI.cpp`. Это справедливо и для `cb3::Main`: обработка мыши, отрисовка и многие действия находятся в `GUI.cpp`, а не только в `Main.cpp`.

Фактическая точка входа — `source/Main.cpp::wWinMain`. В `source/cb3/Main.cpp` есть `MainLoop`, но встроенный режим запускается через `Cb3Runtime`, а не через отдельный executable. Переключение режима проходит через `RequestSimulationMode`, `SwitchSimulationMode`, `CheckRuntimeRequests` и подтверждение в UI; оно создаёт новый мир.

## Карта каталогов и корневых файлов

| Путь | Назначение |
| --- | --- |
| `source/` | Classic, оболочка приложения и общие компоненты |
| `source/cb3/` | Второй движок CyberBiology3, его модели, UI, сериализация и автоадаптация |
| `tests/` | Проверки реального UI и симуляции, PowerShell-запуск |
| `imgui/` | Встроенный сторонний Dear ImGui; используемые backend: `imgui_impl_sdl` и `imgui_impl_sdlrenderer` |
| `IMplot/` | Встроенный сторонний ImPlot для графиков |
| `external/SDL2/`, `external/SDL2_image/` | Заголовки, `.lib` и DLL SDL; архитектурные подкаталоги `lib/x64` и `lib/x86` |
| `external/_downloads/` | Архивы и распакованные дистрибутивы зависимостей; настройки сборки указывают на каталоги выше |
| `SavedObjects/` | Пользовательские сохранения Classic; подкаталог `CyberBiology3/` — сохранения CB3; игнорируется Git |
| `materials/image.jpg` | Изображение, используемое отдельным примером `121main.cpp` |
| `121main.cpp` | Отдельный пример SDL-загрузки изображения с абсолютным старым путём; **не включён** в `.vcxproj`, не точка входа приложения |
| `VirtualLifeSimulator1.sln` | Решение Visual Studio |
| `VirtualLifeSimulator1.vcxproj` | Сборка, зависимости, C++20, `/utf-8`, ресурсы и копирование DLL |
| `VirtualLifeSimulator1.vcxproj.filters` | Группировка файлов в Solution Explorer; поддерживай вместе с проектом при добавлении файлов |
| `VirtualLifeSimulator.rc`, `resource.h`, `VirtualLifeSimulator.ico` | Windows-ресурсы приложения и иконка |
| `imgui.ini` | Состояние расположения окон ImGui; не меняй случайно при проверке UI |
| Корневые `SDL2.dll`, `SDL2_image.dll`, `lib*.dll`, `zlib1.dll` | Runtime-библиотеки; post-build может использовать их как запасной источник |
| `.gitignore` | Исключения для сборок, локальных настроек VS и сохранений |
| `LICENSE` | Лицензия проекта; у сторонних библиотек есть свои лицензии |
| `.vs/`, `Debug/`, `x64/`, `VirtualL.b8f19b53/` | Локальные данные VS, результаты и промежуточные файлы сборки; не исходники |

## Classic и оболочка: назначение файлов `source/`

В таблице `X.h/.cpp` означает оба файла: объявления/структуры в заголовке, реализация в `.cpp`.

| Файл | Ответственность и полезные точки поиска |
| --- | --- |
| `Main.h/.cpp` | Точка входа, глобальный `simulation`, жизненный цикл мира, seed/id/tick, TPS/FPS, клавиатура, выбор объекта, файловые операции, quicksave, переключение движков. `MakeStep`, `CatchKeyboard`, `Deselect`, `LoadFilenames`, `LoadWorldFromFile`, `RecordChartSample`, `Shutdown` |
| `GUI.h/.cpp` | Инициализация/завершение ImGui и реализация UI-методов `Main`: шапка, инструменты, инспектор, подвал, среда, файлы, опасные действия, справка, подтверждения, мозг, графики. Здесь также `Render`, `MouseClick`, `HandleFieldNavigation`, `ClearWorld` |
| `SDL.h/.cpp` | Общие `window`, `renderer`, размеры окна, `io`, `mouseState`; SDL-init, resize, чтение мыши, вывод ImGui и кадра. `CreateWindowSDL`, `CreateRenderer`, `UpdateWindowSize`, `ReadMouseState`, `SDLPresentScene` |
| `Field.h/.cpp` | Сетка объектов, создание/удаление/перемещение, вода/ил, яблоки, шаг мира, рабочие потоки, счётчики, viewport и камера, преобразование экранных координат, телеметрия. `tick`, `AddObject`, `RemoveObject`, `RemoveBot`, `ScreenCoordsToLocal`, `ZoomAtScreenPoint`, `TrackObject`, `ReadTelemetry` |
| `Object.h/.cpp` | База всех объектов: координаты, энергия, возраст, тип, защита от повторного tick в одном кадре, доступ к полю и отрисовка. Статические указатели привязываются через `SetPointers` |
| `Bot.h/.cpp` | Жизненный цикл бота: восприятие, решения мозга, движение/поворот, атака, фотосинтез, размножение, мутации, родство, энергия и искусственный отбор. `tick`, `FillBrainInput`, `think`, `Multiply`, `Attack`, `Photosynthesis`, `GiveEnergy`, `Mutagen`. Мозг — общий `GenomeRef genome` + своя `brainMemory`; `GetActiveBrain`/`GetInitialBrain` дают редактируемый `BrainView`, `GetGenome` — только чтение, `SetObservedBot` — бот для значений в окне мозга |
| `Apple.h/.cpp` | Пищевой объект «яблоко», энергия и текстура |
| `Organics.h/.cpp` | Мёртвая органика с энергией, падение и отрисовка |
| `Rock.h/.cpp` | Камни/препятствия и их отрисовка |
| `BotNeuralNet.h/.cpp` | Структура входов/выходов мозга, обработка сети, рандомизация, мутации и оптимизация в `namespace brain`. `BrainInput`, `BrainOutput`, `Process`, `GetOutput`; `BrainGenome`/`GenomeRef` — общий геном с атомарным счётчиком ссылок и copy-on-write (`MakeUnique`); `BotNeuralNet` — автономная сеть |
| `Neuron.h/.cpp` | Типы нейронов, bias, соединения и веса, операции над связями и локальные мутации |
| `NeuralNetRenderer.h/.cpp` | Адаптер Classic-мозга к общей визуализации; выбранный нейрон и его свойства, `Render`, `ResetSelection` |
| `ObjectSaver.h/.cpp` | Бинарная сериализация мира, объектов и мозга; `MyOutStream`, `MyInputStream`, `SaveWorld`, `LoadWorld`, compact/legacy-чтение, `PersistentState` |
| `Settings.h` | Compile-time параметры Classic: размеры сетки, стартовые условия, клавиши, ограничения потоков, питание/мутации, архитектура мозга, пути и magic numbers сохранений |
| `Systems.h` | Общие include системных/STL/SDL/ImGui/ImPlot-заголовков |
| `MyTypes.h/.cpp` | Classic-типы `Color`, `Point`, `Rect`, псевдонимы типов, случайные числа и часы |
| `SimulationMode.h` | Идентификаторы режимов: Classic=1, CyberBiology3=2; преобразование ID и имён |
| `Cb3Runtime.h/.cpp` | PImpl-мост из оболочки в `cb3::Main`; шаг, ввод, отрисовка, пауза и запрос смены режима. Владеет CB3-симуляцией и временем жизни её текстур |

## Общие компоненты `vlsui`

| Файл | Назначение |
| --- | --- |
| `UILayout.h` | `CalculateLayout`, `WorkspaceLayout`: прямоугольники header/tools/world/inspector/footer, компактный режим. Геометрия UI отделена от размеров мира |
| `UITheme.h` | Общая тема, цвета, шрифт Segoe UI с кириллицей, вспомогательные элементы и оформление рабочего пространства |
| `BotDiet.h` | `ClassifyFeeding`, `FeedingTypeName`: единая интерпретация питания для карточки и телеметрии |
| `NeuralGraphView.h` | Общий ImGui-canvas мозга: узлы/рёбра, выделение, масштаб, pan, clipping и фильтр связей |
| `BrainWindow.h` | Шаблон `DrawBrainWindow`: общее окно текущего/начального мозга, граф, свойства нейронов и редактирование |
| `BrainEditHistory.h` | Отдельная локальная история каждого нейрона и версии мозга: отмена/повтор типа, смещения и исходящих связей; мутация мозга записывается по изменившимся нейронам. Истории выделяются по мере правок и освобождаются при закрытии окна/смене бота; память и значения не откатываются |
| `SimulationTelemetry.h` | `TelemetryCounters`, `ScanTelemetry`, снимки/метрики, `TelemetryHistory`, укрупнение истории, контекст и события графика; общая модель данных без привязки к типу бота |
| `EcosystemChart.h/.cpp` | Общие подробные графики и preview в подвале: сбор наблюдений через `Observe`, режимы/периоды, шкалы, tooltip, события и кэш рядов |

## CyberBiology3: назначение файлов `source/cb3/`

| Файл | Ответственность |
| --- | --- |
| `Main.h/.cpp` | `cb3::Main`: состояние режима, шаги, клавиатура, файлы, глобальная мутация, камни, адаптация; внешние методы `RunFrameSimulation`, `HandleKeyboard`, `HandleMouseClick`, `HandleFieldNavigation`, `ConsumeModeSwitchRequest` |
| `GUI.h/.cpp` | Интерфейс CB3 и методы его `Main`: окна, инспектор, мышь, Render; дополнительное окно `DrawAAWindow` |
| `Field.h/.cpp` | Собственная сетка, потоки/синхронизация, объекты, среда, камера и телеметрия. `FieldDynamicParams` включает runtime-настройки вроде `PSreward` и `botMaxEnergy` |
| `Object.h/.cpp` | Базовый объект CB3; не взаимозаменяем с Classic `Object` |
| `Bot.h/.cpp` | Поведение и наследование CB3-ботов, работа со своими входами/выходами сети, питание, размножение и мутации. Мозг бота — общий `GenomeRef genome` + своя `brainMemory`; `GetActiveBrain`/`GetInitialBrain` возвращают редактируемый `BrainView` (с копированием общего генома), `GetGenome` — только чтение, `SetObservedBot` — бот, чьи значения нейронов сохраняются для окна мозга |
| `Apple.h/.cpp`, `Organics.h/.cpp`, `Rock.h/.cpp` | Реализации объектов CB3 |
| `BotNeuralNet.h/.cpp` | Собственная нейросеть CB3 с размерами слоёв из `neuronsInLayer`, другими входами/выходами и представлением памяти. Алгоритмы в `namespace brain` (`Process`, `Mutate`, `Optimize`…); `BrainGenome` (initial + active, общий для родственников) и `GenomeRef` (атомарный счётчик ссылок, `MakeUnique` — copy-on-write); `BotNeuralNet` — автономная сеть для тестов |
| `Neuron.h/.cpp` | CB3-нейроны, связи, веса и мутации |
| `NeuralNetRenderer.h/.cpp` | Адаптер CB3-мозга к общему `NeuralGraphView` |
| `ObjectSaver.h/.cpp` | Сериализация CB3; проверка формата/режима и восстановление состояния объектов и мира |
| `AutomaticAdaptation.h/.cpp` | Автоматизированные эксперименты отбора: фазы/эпохи, `ParameterSweep`, ожидание популяции, изменение параметров, история и UI |
| `ImageFactory.h/.cpp` | Создание/удаление текстур яблок, органики, камней и ботов |
| `Utils.h/.cpp` | CB3-типы `Color`, `Point`, `Rect`, случайные числа, часы и вспомогательные функции |
| `Settings.h` | Константы CB3, клавиши, размеры мира/слоёв сети, пути сохранений и magic numbers; многие параметры — макросы даже внутри namespace |
| `Systems.h` | Системные и библиотечные include CB3 |
| `SDL.h` | Объявления общих SDL-символов и aliases в `cb3`; своего `SDL.cpp` у этого режима нет |
| `Chart.h/.cpp` | Совместимость со старым именем: `cb3::Chart` — alias `vlsui::EcosystemChart`; `.cpp` лишь включает заголовок |

## Где искать нужное изменение

| Задача | Начни здесь → затем проверь |
| --- | --- |
| Старт, выход, выбор движка | `source/Main.cpp`: `wWinMain`, `SwitchSimulationMode`, `Shutdown` → `Cb3Runtime.cpp`, подтверждения в обоих `GUI.cpp` |
| Кнопки, вкладки, окна, подписи | `GUI.cpp` нужного режима: `DrawMainWindow`, `DrawSidePanelWindow`, `DrawWindows` → `UITheme.h`, `HOW_TO_PLAY.md` |
| Размеры/расположение панелей | `UILayout.h::CalculateLayout` → оба `GUI.cpp`, viewport/hit testing в обоих `Field.cpp` |
| Выбор бота, кисть, мышь | `GUI.cpp::MouseClick`, `Main::Deselect` → `Field::ScreenCoordsToLocal`, `TrackObject`, `ValidateObjectExistance` |
| Камера, zoom, scrollbars | `Field.cpp`: `PanView`, `ZoomAtScreenPoint`, `GetViewportRect`, `ClampViewOffset` → `HandleFieldNavigation`, `DrawFieldScrollbars` |
| Горячие клавиши | `Settings.h` нужного режима → `Main.cpp::CatchKeyboard`, `HOW_TO_PLAY.md` |
| Еда, энергия, движение, размножение | `Bot.cpp` нужного режима → `Field.cpp`, `Settings.h` и runtime `FieldDynamicParams` |
| Начальная популяция/стартовые параметры | `Main::SpawnInitialPopulation`, `Field::SpawnControlGroup`, конструкторы `Main`/`Field`, `Settings.h` |
| Нейросеть и мутации | `BotNeuralNet.*`, `Neuron.*`, `Bot.*` нужного режима → его `Settings.h`, сериализатор |
| Окно мозга | `BrainWindow.h`, `NeuralGraphView.h` → оба `NeuralNetRenderer.*`, `DrawBotBrainWindow` |
| Категории питания и их подписи | `BotDiet.h` → `Bot::GiveEnergy`/getters, карточки в обоих `GUI.cpp`, `ScanTelemetry` |
| Графики, метрики, история | `EcosystemChart.*`, `SimulationTelemetry.h` → оба `Main::RecordChartSample`, `Field::ReadTelemetry` |
| Сохранить/загрузить/переименовать | `Main.cpp` нужного режима: `LoadFilenames`, `BuildSavePath`, `LoadWorldFromFile`, `QuickSaveWorld` → `ObjectSaver.*`, константы `Settings.h` |
| Потоки, скорость тика | `Field.cpp` нужного режима: `StartThreads`, `WaitForThreads`, `tick_multiple_threads` → `Settings.h`, `Main::MakeStep` |
| Автоадаптация | `cb3/AutomaticAdaptation.*` → `cb3/GUI.cpp::DrawAAWindow`, `cb3/Main.cpp`, `FieldDynamicParams` |
| Ошибки линковки/зависимостей | `.vcxproj`, `external/README.md`, архитектура SDL `.lib`/DLL → перечень `ClCompile`, пути `ObjectFileName` |

## Важные связи и ограничения

- **Не смешивай модели движков.** У них разные `Bot`, `Color`, `BrainInput`, `BrainOutput`, параметры и бинарные форматы. Общие изменения оформляй в существующих компонентах `vlsui`, где это подходит.
- **Compile-time и runtime — разные уровни.** Начальные значения/макросы находятся в `Settings.h`, изменяемая среда — в `FieldDynamicParams` и полях `Field`. В Classic награда за фотосинтез — `photosynthesisReward`, в CB3 — `params.PSreward`.
- **Макросы не изолируются namespace.** Не подключай без необходимости оба `Settings.h` или оба набора моделей в одну единицу компиляции. `Cb3Runtime` скрывает реализацию CB3 через PImpl.
- **Объект может исчезнуть в ходе тика.** Не разыменовывай старый `selectedObject` после удаления. Используй существующие `TrackObject`/`ValidateObjectExistance`, `Deselect` и `ResetSelection`; при смене текущего/начального мозга обновляй указатель выбранного нейрона.
- **Соблюдай синхронизацию Field.** Снимок телеметрии читается после завершения рабочих потоков. Не обходи/изменяй клетки из UI параллельно рабочей фазе. Уничтожение поля должно корректно завершать и join-ить потоки.
- **Мир и экран — отдельные координаты.** Layout, viewport, масштаб, панорамирование и hit testing должны совпадать. Изменение ширины окна не должно менять размеры симуляции или координаты в сохранении.
- **Мозг имеет две версии:** active — используемая/изменяемая сеть, initial — наследуемая исходная. Учитывай обе при изменении мутаций, просмотра и сериализации.
- **Геном мозга общий у родственников (оба движка):** обе версии нейронов лежат в `BrainGenome`, который потомок без мутации делит с родителем; у бота свои только память нейронов, а значения нейронов — временный буфер `think()`. Перед любым изменением нейронов вызывай `genome.MakeUnique()` (или бери `BrainView` через `GetActiveBrain`/`GetInitialBrain`), иначе правка затронет родственников. После прямой правки через `BrainView` вызывай `RecheckBrain()`: флаг `activeIsOptimizedInitial` решает, можно ли потомку делить геном. Формат сохранений прежний — каждый бот пишется с развёрнутым мозгом; при загрузке мира одинаковые геномы снова объединяются.
- **Сохранения:** Classic — `SavedObjects/`, CB3 — `SavedObjects/CyberBiology3/`; quicksave называется `Quicksave` внутри соответствующего каталога. Пути относительны рабочему каталогу процесса. Заголовки/magic numbers и ID режима важнее расширения файла.
- Classic читает legacy и compact-форматы (`LoadWorldLegacy`, `LoadWorldCompact`); CB3 имеет отдельный сериализатор. При изменении сохраняемого состояния проверь `PersistentState`, порядок чтения/записи, версию/совместимость и отказ при неверном режиме. Не считай одинаковое имя метода признаком одинакового формата.
- **Телеметрия относится к сессии:** история не сохраняется в файл мира; после загрузки начинается заново. Разделяй рождения/гибель в тике и ручное добавление/удаление (`TelemetryCounters`).
- `ClassifyFeeding`: меньше 20 учтённых единиц энергии — Unknown, доля источника от 2/3 — соответствующая категория, иначе Mixed. Это наблюдаемое питание, не вид бота; яблоки и органика учитываются вместе. В CB3 счётчики источников ограничены 100, в Classic такого ограничения нет.
- История графиков укрупняет старые наблюдения с сохранением экстремумов; пустая популяция даёт NaN для неопределённых метрик. Не подменяй их нулями и не удаляй всю историю при заполнении буфера.
- В исходниках есть русские UI-строки; сохраняй корректную кодировку, учитывай `/utf-8` в сборке. Подключение шрифта — `UITheme.h::AddInterfaceFont`, сейчас это Windows Segoe UI.
- Новый `.cpp` нужно явно добавить в `.vcxproj`; для одноимённых CB3-файлов сохраняй отдельный `ObjectFileName` под `$(IntDir)cb3\`, иначе возникнут коллизии `.obj`. `.filters` отвечает только за отображение в IDE.
- Сторонние `imgui/`, `IMplot/` и бинарные зависимости меняй только по относящейся к ним задаче. Не включай пользовательские сохранения и случайные изменения `imgui.ini` в правки.

## Сборка и проверка

Команды выполняются из корня проекта в PowerShell. Нужны установленный MSVC v143 и Windows SDK; поиск VS уже реализован в smoke-скрипте.

```powershell
# Основная проверка: собирает Debug x64 и запускает Classic и CB3.
.\tests\Run-UIWorkspaceSmoke.ps1

# Если приложение уже открыто: другое имя executable для сборки.
.\tests\Run-UIWorkspaceSmoke.ps1 -ApplicationTargetName VirtualLifeSimulator1-UI

# Только после актуальной сборки Debug x64.
.\tests\Run-UIWorkspaceSmoke.ps1 -SkipBuild
```

Отдельная сборка приложения:

```powershell
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vsRoot = & $vswhere -latest -property installationPath
$msbuild = Join-Path $vsRoot 'MSBuild\Current\Bin\MSBuild.exe'
& $msbuild .\VirtualLifeSimulator1.vcxproj /p:Configuration=Debug /p:Platform=x64 /m /v:minimal /nologo
```

Доступны Debug/Release × Win32/x64; основной существующий тестовый путь — Debug x64. Типичный executable — `x64/Debug/VirtualLifeSimulator1.exe`. Запускай с рабочим каталогом корня проекта для ожидаемых путей сохранений. Post-build копирует DLL рядом с exe; соответствие x86/x64 обязательно. Подробнее о структуре зависимостей — `external/README.md`.

`tests/UIWorkspaceSmoke.cpp` — общий тестовый стенд; компилируется дважды (для CB3 с `TEST_CB3`), получает доступ к внутреннему состоянию через `InterfaceSmokeTest` и линкуется с объектными файлами приложения. `tests/Run-UIWorkspaceSmoke.ps1` собирает проект, компилирует оба теста и проверяет их exit codes.

Проверки выполняют реальный UI через SDL software renderer в памяти без desktop-окна, системного ввода и GPU. Покрывают заселение, паузу/шаг, выбор/zoom, кисть, сохранения, окна, resize 640×480…1920×1080, переполнение панелей, режимы просмотра, классификацию питания, окно мозга и телеметрию/графики. PNG и изолированные сохранения находятся в `x64/Debug/ui-smoke-classic/` и `x64/Debug/ui-smoke-cb3/`; корневые сохранения и `imgui.ini` тесты не перезаписывают.

Для правок логики/UI запускай подходящие проверки; при общей правке проверяй оба режима. При изменении только документации достаточно сверить пути, символы, команды и diff. В отчёте точно указывай, что запускалось, а что осталось непроверенным.

## Быстрый поиск и порядок работы

```powershell
git status --short
rg --files source tests
rg -n 'DrawSidePanelWindow|DrawBotBrainWindow|DrawChartWindow' source -g '*.cpp'
rg -n 'ScreenCoordsToLocal|ZoomAtScreenPoint|TrackObject' source
rg -n 'RecordChartSample|ReadTelemetry|TelemetryCounters' source
rg -n 'SaveWorld|LoadWorld|MagicNumber|PersistentState' source
git diff --check
```

Сначала найди символ в нужном движке, затем его вызовы и общие компоненты. Ищи в `source/` и `tests/`, чтобы не читать библиотечные копии и build-артефакты. Предпочитай поиск по имени функции поиску по номерам строк: крупные `GUI.cpp`, `Main.cpp` и `Field.cpp` меняются часто.

При изменении управления/видимого поведения обновляй `HOW_TO_PLAY.md`; при расширении проверок — `tests/README.md`; при изменении структуры — эту карту. Не фиксируй в инструкциях временное состояние рабочей копии, локальные ошибки сборки или неподтверждённые предположения как постоянные свойства проекта.
