\mainpage 3DVisualizer — Профессиональный 3D Viewer v2.1

@tableofcontents

@section intro 📌 Описание проекта

**3DVisualizer** — это высокопроизводительное приложение на C++20 и Qt6 для визуализации 3D-моделей.

Программа поддерживает:
- Загрузку моделей формата **OBJ** (до 1,000,000 вершин)
- **Реалтайм-рендеринг** через OpenGL 3.3+ с 5 режимами затенения
- **Трассировку лучей** (Ray Tracing) с экспортом в BMP/JPEG/PNG до 1920×1440
- **Аналитические примитивы**: сфера, цилиндр, конус, куб, пирамида
- **Множественные источники света** (до 5 точечных источников)
- **Наложение текстур** с UV-развёрткой
- **Запись GIF-анимаций** и скриншотов
- **Визуальные свойства материалов**: отражение, прозрачность, преломление, шероховатость, сглаживание граней

@section features ⭐ Ключевые возможности

@subsection features_render 🎨 Режимы рендеринга
| Режим | Описание |
|:---|:---|
| **Wireframe** | Каркасная модель с настраиваемыми рёбрами (сплошные/пунктирные) |
| **Flat Shading** | Плоское затенение граней (расчёт нормалей в шейдере через dFdx/dFdy) |
| **Gouraud Shading** | Мягкое затенение Гуро (интерполяция цвета по вершинам) |
| **Phong Shading** | Мягкое затенение Фонга (интерполяция нормалей, попиксельный расчёт) |
| **Texture** | Наложение текстур с мультитекстурным освещением |

@subsection features_raytrace 🔆 Трассировка лучей (Ray Tracing)
- **Физически корректное освещение** по модели Фонга с тенями
- **Отражение** (reflectivity) с учётом шероховатости поверхности
- **Преломление** (transparency + IOR) по закону Снеллиуса с коэффициентом Френеля
- **KD-дерево** для ускорения пересечений с полигональными сетками
- **Аналитические примитивы** с точным пересечением луча
- **Шахматный пол** с вкл/выкл
- Экспорт изображений до **1920×1440** в форматах BMP, JPEG, PNG

@subsection features_materials 💎 Свойства материалов
| Параметр | Диапазон | Описание |
|:---|:---|:---|
| **Base Color** | RGB | Базовый цвет поверхности |
| **Reflectivity** | 0.0 – 1.0 | Коэффициент зеркального отражения |
| **Transparency** | 0.0 – 1.0 | Прозрачность материала |
| **IOR** | 1.0 – 3.0 | Показатель преломления (1.5 = стекло) |
| **Roughness** | 0.0 – 1.0 | Шероховатость (размытие отражений) |
| **Smooth Shading** | bool | Сглаживание граней (Phong vs Flat) |

@subsection features_scene 🗄 Управление сценой
- До **6 объектов** в сцене одновременно
- **Глобальные и локальные трансформации** (перенос, вращение, масштаб)
- **Переключение активного объекта** через выпадающий список
- **Удаление объектов** из сцены
- Автоматическая **нормализация** загружаемых OBJ-моделей в единичный куб
- Автоматический расчёт **нормалей** при отсутствии в файле

@section layers 🏗 Архитектура проекта (Layered MVC)

Проект разделён на логические слои для обеспечения масштабируемости и чистоты кода:

| Слой | Описание | Основные компоненты | Паттерны |
|:---|:---|:---|:---|
| **Model** | Хранение данных, парсинг, математика | `Mesh`, `AnalyticMesh`, `ObjParser`, `Scene`, `Matrix` | Фасад, Композитор |
| **Controller** | Посредник между данными и интерфейсом | `ViewerController`, команды трансформаций | Команда, Фасад |
| **View** | Отрисовка графики (OpenGL / Ray Tracing) | `SceneRenderer`, `RayTracer`, шейдеры, камера | Стратегия, Компоновщик |
| **GUI / App** | Взаимодействие с пользователем | `QtView`, `QtGLWidget`, `GifRecorder`, `SettingsManager` | Наблюдатель, Хранитель |

@section tech_specs 🔧 Технические детали

@subsection tech_opengl 🖥 OpenGL-рендеринг
- **Версия**: OpenGL 3.3+ (Core Profile)
- **Шейдеры**: GLSL 330 — 6 программ (Edge, Point, Flat, Gouraud, Phong, Texture)
- **Буферы**: VAO/VBO/IBO для каркаса и сплошных граней
- **Освещение**: До 5 точечных источников с позицией, цветом и интенсивностью
- **Проекции**: Перспективная (Perspective) и ортографическая (Orthographic)

@subsection tech_raytracing 📐 Трассировка лучей
- **Алгоритм**: Обратная трассировка лучей (Backward Ray Tracing)
- **Ускорение**: KD-дерево (axis-aligned splitting planes, median split)
- **Глубина рекурсии**: Настраиваемая (1–10, по умолчанию 3)
- **Сэмплирование**: Rejection sampling для шероховатости
- **Оптимизации**: Early exit для теневых лучей, bias-исправление самозатенения

@subsection tech_analytic 🔷 Аналитические примитивы
| Примитив | Метод пересечения | Особенности |
|:---|:---|:---|
| **Sphere** | Решение квадратного уравнения | Точная нормаль (point - center) / radius |
| **Cylinder** | Проекция на плоскость + крышки | Боковая поверхность + 2 круговых основания |
| **Cone** | Квадратное уравнение конуса | Боковая поверхность + основание с нормалью |
| **Cube** | Slab method (AABB) | Нормаль по доминирующей координате |
| **Pyramid** | Möller–Trumbore per triangle | 6 треугольников (2 основание + 4 боковые) |

@subsection tech_parser 📄 OBJ-парсер
- **Формат**: Wavefront OBJ (v, vn, vt, f)
- **Индексы**: Поддержка положительных, отрицательных и смешанных (v/vt/vn)
- **Оптимизация**: Чтение в буфер + zero-copy парсинг через strtof/strtol
- **Нормализация**: Центрирование и масштабирование в единичный куб [-1, 1]
- **Нормали**: Автоматический расчёт через векторное произведение при отсутствии

@section patterns 🧩 Применённые паттерны проектирования

- **Strategy (Стратегия):** Инкапсуляция техник рендеринга (`IShaderTechnique` → `PhongTechnique`, `FlatTechnique`...) и стратегий проекции (`IProjectionStrategy` → `CentralProjection`, `ParallelProjection`)
- **Command (Команда):** Отделение логики трансформаций (`TranslateCommand`, `RotateCommand`, `ScaleCommand`) от элементов интерфейса
- **Facade (Фасад):** Упрощённый доступ к сложным операциям модели через `ViewerController`
- **Memento (Хранитель):** Отслеживание состояний трансформации (`TransformState`) для синхронизации со слайдерами
- **Observer (Наблюдатель):** Уведомление интерфейса о прогрессе записи GIF (`GifRecorder` → сигналы Qt)
- **Composite (Компоновщик):** Управление иерархией объектов сцены (`Scene` → `SceneObject` → `IMesh`)

@section build 🛠 Инструкции по сборке

@subsection build_req 📋 Требования
- **C++20** совместимый компилятор (GCC 10+, Clang 12+, MSVC 2019+)
- **Qt6** (Core, Widgets, OpenGLWidgets)
- **CMake** 3.16+
- **Graphviz** (для генерации диаграмм Doxygen)
- **GTest** (для сборки тестов)

@subsection build_commands 🔨 Команды сборки
@code{.bash}
# Клонирование и сборка
mkdir build && cd build
cmake .. -DRelease=ON
make -j$(nproc)

# Запуск тестов
cmake .. -DBUILD_TESTING=ON
make -j$(nproc)
ctest --output-on-failure

# Генерация документации
cmake .. -DBUILD_DVI=ON
make doxygen

# Создание дистрибутива
make dist
@endcode

@section file_structure 📁 Структура проекта

@code
src/
├── main.cpp                          # Точка входа
├── Includes/
│   ├── AnalyticPrimitive.h/.cpp      # Аналитические примитивы + трансформации
│   ├── RTTypes.h                     # Типы для Ray Tracing (Ray, AABB, RTMaterial, KDNode...)
│   ├── s21_matrix_oop.h/.cpp         # Матричная библиотека 4×4 (аффинные преобразования)
│   └── gif.h                         # Библиотека записи GIF (gif.h)
├── Model/
│   ├── TypesModel.h                  # Базовые типы (Vertex, Normal, Face, MeshData, Material)
│   ├── Model.h/.cpp                  # Mesh, SceneObject, Scene, AffineTransformer
│   ├── ObjParser.h/.cpp              # Парсер OBJ-файлов
│   ├── AnalyticMesh.h/.cpp           # Триангуляция аналитических примитивов
│   └── TestModel.cpp                 # Unit-тесты (GTest)
├── Controller/
│   ├── Controller.h/.cpp             # ViewerController + команды трансформаций
│   └── TypesView.h                   # Общие типы View-слоя
└── View/
    ├── View.h/.cpp                   # QtView — главное окно и UI
    ├── Renderer/
    │   ├── Render.h/.cpp             # SceneRenderer — OpenGL-рендерер
    │   └── Shaders.h                 # GLSL-шейдеры (6 програм)
    ├── RayTracer.h/.cpp              # RayTracer — трассировка лучей
    ├── Camera.h/.cpp                 # OrbitCamera — орбитальная камера
    ├── GifRecorder.h/.cpp            # Запись GIF-анимаций
    ├── SettingsManager.h/.cpp        # Сохранение/загрузка настроек (QSettings)
    └── TransformState.h/.cpp         # Состояние трансформаций для UI
@endcode

@section class_diagram 📊 Полная диаграмма классов

\dot
digraph ClassDiagram {
    rankdir=TB;
    compound=true;
    node [shape=record, style=filled, fontname="Verdana", fontsize=10];
    edge [fontname="Verdana", fontsize=9, arrowhead=onormal];

    // ==================== СЛОЙ: MODEL ====================
    subgraph cluster_model {
        label="MODEL LAYER"; color=forestgreen; style=bold;

        IMesh [label="{«interface»\nIMesh|+ extractMeshData() : MeshData\l+ getVertices/Normals/Edges/Faces()\l+ getAnalyticPrimitive()\l+ hasTexCoords() : bool\l}", fillcolor=palegreen];

        Mesh [label="{Mesh|- m_vertices/m_faces/m_normals/m_texCoords\l+ extractMeshData()\l+ fillWireframeData()\l+ fillSolidData()\l}", fillcolor=palegreen];

        AnalyticMesh [label="{AnalyticMesh|- m_primitive : shared_ptr\l+ triangulate()\l+ addSphere/Cylinder/Cone/Cube/Pyramid()\l}", fillcolor=palegreen];

        ObjParser [label="{ObjParser|+ load(path) : shared_ptr\l- computeNormals()\l- normalizeModel()\l- parseVertex/Normal/TexCoord/Face()\l}", fillcolor=palegreen];

        AffineTransformer [label="{AffineTransformer|+ translate/rotate/scale()\l+ createIdentityMatrix()\l+ createRotationX/Y/ZMatrix()\l}", fillcolor=palegreen];

        Scene [label="{Scene|- m_objects : vector\l- m_globalTransform : Matrix\l+ addObject() / removeObject()\l+ getObject() / getObjects()\l}", fillcolor=palegreen];

        SceneObject [label="{SceneObject|- m_mesh : shared_ptr\l- m_transform : Matrix\l- m_material : Material\l+ getMesh() / setTransform()\l+ getMaterial() / setMaterial()\l}", fillcolor=palegreen];

        IMesh -> Mesh [style=dashed];
        IMesh -> AnalyticMesh [style=dashed];
        ObjParser -> Mesh [label=" creates", style=dotted, arrowhead=vee];
        Scene -> SceneObject [arrowhead=diamond, label=" owns"];
        SceneObject -> IMesh [arrowhead=vee];
    }

    // ==================== СЛОЙ: CONTROLLER ====================
    subgraph cluster_controller {
        label="CONTROLLER LAYER"; color=blue; style=bold;

        ViewerController [label="{ViewerController (Facade)|+ loadModel(path) : bool\l+ addSphere/Cylinder/Cone/Cube/Pyramid()\l+ translate/rotate/scale()\l+ getSceneSnapshot() : SceneSnapshot\l+ getActiveMaterial() / setActiveMaterial()\l}", fillcolor=lightblue];

        ICommand [label="{«interface»\nICommand|+ execute() : void\l}", fillcolor=lightblue];

        BaseCommand [label="{BaseCommand|- m_controller : ViewerController&\l- m_global : bool\l}", fillcolor=lightcyan];

        TransformCmd [label="{TransformCommands\l(Translate/Rotate/Scale)|+ execute()\l}", fillcolor=lightcyan];

        ICommand -> BaseCommand [style=dashed];
        BaseCommand -> TransformCmd [style=dashed];
        ViewerController -> ICommand [label=" dispatches", arrowhead=vee];
    }

    // ==================== СЛОЙ: VIEW (OpenGL) ====================
    subgraph cluster_view_gl {
        label="VIEW LAYER: OpenGL"; color=purple; style=bold;

        SceneRenderer [label="{SceneRenderer (Context)|- m_edge/flat/gouraud/phong/textureTech\l- m_meshBuffers : unordered_map\l- m_floorVAO/VBO/IBO\l+ render() / resize()\l+ renderRayTraced()\l+ ensureMeshUploaded()\l}", fillcolor=plum1];

        IShaderTechnique [label="{«interface»\nIShaderTechnique|+ init(gl)\l+ bindAndSetUniforms()\l+ release()\l}", fillcolor=orange];

        BaseTech [label="{BaseTechnique|# m_program : QOpenGLShaderProgram\l+ release()\l+ setLights()\l}", fillcolor=lightyellow];

        ConcreteTech [label="{ConcreteTechniques\l(Edge/Point/Flat/Gouraud/Phong/Texture)|+ bindAndSetUniforms()\l}", fillcolor=lightyellow];

        SceneRenderer -> IShaderTechnique [arrowhead=diamond, label=" uses"];
        IShaderTechnique -> BaseTech [style=dashed];
        BaseTech -> ConcreteTech [style=dashed];
    }

    // ==================== СЛОЙ: VIEW (Ray Tracing) ====================
    subgraph cluster_view_rt {
        label="VIEW LAYER: Ray Tracing"; color=orangered; style=bold;

        RayTracer [label="{RayTracer|- m_triangles : vector\l- m_root : KDNode*\l- m_analyticPrimitives : vector\l- m_analyticMaterials : unordered_map\l+ render() : QImage\l+ setSceneData() / setCamera()\l+ traceRay() / traceRayRecursive()\l- buildKDTree() / intersectKDTree()\l- shadeSurface() / traceReflection() / traceRefraction()\l}", fillcolor=lightsalmon];

        KDNode [label="{KDNode|- bounds : AABB\l- left/right : KDNode*\l- triangles : vector\l- axis : int\l- isLeaf : bool\l}", fillcolor=lightsalmon];

        AnalyticPrimitive [label="{«interface»\nAnalyticPrimitive|+ intersect(ray, tmin, tmax, hit) : bool\l+ name() : QString\l}", fillcolor=lightsalmon];

        TransformedPrimitive [label="{TransformedPrimitive|- m_base : shared_ptr\l- m_transform : QMatrix4x4\l- m_invTransform : QMatrix4x4\l+ intersect() с трансформацией луча\l}", fillcolor=lightsalmon];

        RayTracer -> KDNode [arrowhead=diamond, label=" owns"];
        RayTracer -> AnalyticPrimitive [arrowhead=vee];
        AnalyticPrimitive -> TransformedPrimitive [style=dashed];
    }

    // ==================== СЛОЙ: VIEW (Camera & Utils) ====================
    subgraph cluster_view_utils {
        label="VIEW LAYER: Camera & Utils"; color=darkorchid; style=bold;

        ICamera [label="{«interface»\nICamera|+ getViewMatrix()\l+ getProjectionMatrix()\l+ move/rotate/zoom()\l+ getPosition/Target/Fov()\l}", fillcolor=plum1];

        OrbitCamera [label="{OrbitCamera|- m_position/target/up\l- m_yaw/pitch/distance\l- m_projectionStrategy\l+ updatePosition()\l}", fillcolor=plum1];

        IProjectionStrategy [label="{«interface»\nIProjectionStrategy|+ getMatrix() : QMatrix4x4\l}", fillcolor=plum1];

        CentralProjection [label="{CentralProjection|+ perspective()\l}", fillcolor=plum1];

        ParallelProjection [label="{ParallelProjection|+ ortho()\l}", fillcolor=plum1];

        ICamera -> OrbitCamera [style=dashed];
        IProjectionStrategy -> CentralProjection [style=dashed];
        IProjectionStrategy -> ParallelProjection [style=dashed];
        OrbitCamera -> IProjectionStrategy [arrowhead=diamond];
    }

    // ==================== GUI / APP ====================
    subgraph cluster_gui {
        label="GUI & APPLICATION"; color=grey; style=bold;

        QtGLWidget [label="{QtGLWidget|+ paintGL() / resizeGL()\l+ mousePress/MoveEvent()\l+ wheelEvent() / keyPressEvent()\l- m_controller / m_renderer / m_camera\l}", fillcolor=lightgrey];

        QtView [label="{QtView|- m_controller / m_renderer / m_camera\l- m_settings : RenderSettings\l- m_gifRecorder : GifRecorder\l- m_settingsManager : SettingsManager\l+ setupUI() / run()\l+ onLoadFileClicked() / onRayTraceClicked()\l}", fillcolor=lightgrey];

        GifRecorder [label="{GifRecorder|- m_buffer : vector\l- m_frameProvider : FrameProvider*\l+ startRecording() / stopRecording()\l+ saveToFile()\l- onTimerTick()\l}", fillcolor=lightyellow];

        SettingsManager [label="{SettingsManager|+ load() : RenderSettings\l+ save(settings)\l+ defaults() : RenderSettings\l}", fillcolor=lightyellow];

        TransformState [label="{TransformState|- m_current : Transform\l+ updateMove/Rotate/Scale()\l+ reset()\l+ toSlider() / fromSlider()\l}", fillcolor=lightyellow];

        QtView -> QtGLWidget [arrowhead=diamond];
        QtView -> GifRecorder [arrowhead=diamond];
        QtView -> SettingsManager [arrowhead=vee];
        QtView -> TransformState [arrowhead=vee];
    }

    // ==================== ВНЕШНИЕ СВЯЗИ ====================
    ViewerController -> Scene [arrowhead=diamond, label=" owns"];
    ViewerController -> ObjParser [arrowhead=vee];

    QtGLWidget -> ViewerController [arrowhead=vee];
    QtGLWidget -> SceneRenderer [arrowhead=diamond];
    QtGLWidget -> ICamera [arrowhead=diamond];

    SceneRenderer -> RayTracer [arrowhead=vee, label=" raytrace"];
    SceneRenderer -> ICamera [arrowhead=vee];

    RayTracer -> ICamera [arrowhead=vee];
    RayTracer -> AnalyticPrimitive [arrowhead=vee];

    TransformCmd -> ViewerController [arrowhead=vee, label=" targets"];
}
\enddot

@section changelog 📝 История изменений

| Версия | Дата | Изменения |
|:---|:---|:---|
| **v2.1** | 2024 | Добавлена трассировка лучей, KD-дерево, аналитические примитивы, множественные источники света, свойства материалов, GIF-запись |
| **v2.0** | 2024 | Рефакторинг на Qt6, C++20, стратегии шейдеров, текстурирование, орбитальная камера |
| **v1.0** | 2023 | Базовый wireframe-вьювер с аффинными преобразованиями |

@section authors 👥 Авторы

Проект разработан в рамках образовательной программы **School 21**.

@note
Для генерации документации требуется Doxygen с поддержкой Graphviz (HAVE_DOT = YES).
Все диаграммы генерируются автоматически из исходного кода.
