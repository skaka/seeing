# Seeing AI — Intelligent Non-Linear Video Editor (NLE)

<div dir="rtl">

**برنامج Seeing AI هو برنامج مونتاج وتعديل فيديو خطي (NLE) يعتمد بالكامل على تقنيات الذكاء الاصطناعي الثنائية (Dual-AI Architecture)، ومبني باستخدام لغة C++20 ومكتبة الرسوميات Qt6 ليعمل على مختلف أنظمة التشغيل بكفاءة عالية.**

تم تصميم البرنامج كبيئة تحرير ذكية متكاملة مستوحاة من واجهات VS Code، بحيث تدعم أتمتة عمليات المونتاج بالكامل إما محلياً (Offline) لحماية خصوصية البيانات أو سحابياً (Cloud) للأداء السريع.

</div>

---

## ── Dual-AI Architecture / هيكلية الذكاء الاصطناعي الثنائي

<div dir="rtl">

يعتمد البرنامج على نموذجين مختلفين للذكاء الاصطناعي يعملان بشكل متكامل:

### 1. ذكاء فهرسة وتحليل الوسائط (VLM Indexer AI)
مسؤول عن قراءة وتحليل الصور والفيديوهات والصوت بمجرد استيرادها إلى المشروع:
* **محلي بالكامل (Offline - Qwen2-VL)**: يقوم بتحليل الوسائط محلياً داخل حاوية Docker دون الحاجة لأي اتصال بالإنترنت أو استهلاك للبيانات، مع حظر كامل لأي تحميل تلقائي من خوادم Hugging Face لضمان الحفاظ على موارد النظام.
* **سحابي بالكامل (API-based - OpenAI / Gemini)**: يقوم باستخراج كادرات مرئية ذكية من الفيديوهات وإرسالها مشفرة عبر واجهة برمجة التطبيقات لتوصيف المحتوى بشكل فائق الجودة والسرعة.
* **تلميحات تفاعلية (Tooltips)**: يعرض البرنامج وصف التحليل الذكي للقطات كبطاقات تلميحات هوامشية تفاعلية بمجرد تمرير الفأرة فوق الوسائط في لوحة الاستكشاف (Explorer).

### 2. ذكاء إدارة عمليات التحرير والمونتاج (NLE Montage Manager)
مساعد المونتاج الذكي الذي يستقبل الأوامر الصوتية أو النصية من لوحة الدردشة:
* **مونتاج تلقائي بالكامل (Automated Montage)**: يقوم بتحليل قائمة الوسائط المستوردة ووصفها البصري، ثم يولد مخططاً زمنياً متسلسلاً ويقوم بتركيب الكليبات خلف بعضها بدقة تلقائياً عبر ميزة الأوامر المتسلسلة (`sequence` actions).
* **تنفيذ الأوامر الذكية**: يدعم قص وتعديل وتحريك وحذف الكليبات وتطبيق عمليات الـ Undo والـ Redo بشكل آلي أو يدوي.
* **خيارات متعددة**: يدعم كلاً من نماذج OpenAI، ونماذج Gemini، ونماذج Ollama المحلية (مثل Llama3)، بالإضافة لنموذج محاكاة تجريبي (Dummy AI).

</div>

---

## ── Architecture / هندسة النظام

```
┌─────────────────────────────────────────────────────────────────┐
│                        MainWindow                               │
│  ┌──────────┬──────────────────────────┬──────────────────────┐ │
│  │  Media   │      Preview Panel       │    AI Copilot        │ │
│  │  Panel   │                          │    Chat Panel        │ │
│  │          ├──────────────────────────┤                      │ │
│  │  (Left)  │    Timeline Panel        │    (Right)           │ │
│  │          │  QGraphicsScene/View     │                      │ │
│  │  Explorer│  Multitrack Timeline     │  Dual AI Assistant   │ │
│  └──────────┴──────────────────────────┴──────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘

           ┌─────────┐     ┌────────────────┐     ┌─────────┐
           │  VIEW   │◄────│    MODEL       │────►│  VIEW   │
           │ (Panels)│     │ (ProjectModel) │     │ (Panels)│
           └────┬────┘     └───────▲────────┘     └─────────┘
                │                  │
                │     ┌────────────┴───────────┐
                └────►│     CONTROLLER         │
                      │  (EditorController)    │
                      │         │              │
                      │    ┌────▼────┐         │
                      │    │AI Engine│         │
                      │    │Interface│         │
                      │    └─────────┘         │
                      └────────────────────────┘
```

---

## ── Docker Container Configuration / تشغيل البرنامج عبر الحاويات

<div dir="rtl">

يتم تشغيل البرنامج بالكامل داخل بيئة حاوية معزولة (Docker/Podman) لمنع أي تعارض في المكتبات ولتأمين تشغيل سلس لجميع معالجات بايثون ونماذج الذكاء الاصطناعي:

* يتم ربط بطاقة الرسوميات (GPU) محلياً لضمان تسريع الذكاء الاصطناعي.
* يتم مشاركة المجلدات التفضيلية ومجلد الوسائط لتسجيل المشاريع مباشرة في حاسوبك المضيف.

### خطوات التشغيل السريعة:
</div>

```bash
# 1. Build the container image
./build.sh

# 2. Run the program (starts Marlin server & logs GUI forwarding)
./run.sh
```

---

## ── Configuration & Local Weights / الإعدادات وأوزان النماذج المحلية

<div dir="rtl">

عند النقر على الاختصار `Ctrl + ,` أو من خلال قائمة `AI` -> `Configure AI Copilot`:
1. ستظهر لك قائمة الاختيارات الخاصة بكل نموذج (تحليل الوسائط، والدردشة التحريرية).
2. في الجزء السفلي من خانة اختيار الـ VLM، ستجد مؤشراً ذكياً باللون الأخضر أو الأحمر يوضح ما إذا كانت أوزان نموذج Qwen2-VL متوفرة محلياً داخل الحاوية في المسار `/app/model_weights` أم لا، مما يعطيك تحكماً كاملاً لربط النموذج بالإنترنت أو إبقائه أوفلاين.

</div>

---

## ── AI Assistant Test Commands / أوامر تجربة مساعد المونتاج

Type these inside the **AI Copilot Chat Panel** on the right:

| Category | Arabic Command | English Command | Effect / التأثير |
|:---|:---|:---|:---|
| **Auto Montage** | `اعمل مونتاج تلقائي لكل الفيديوهات والصور بترتيب زمني` | `Make a dynamic montage using all imported assets` | **يرتب جميع الوسائط خلف بعضها في التايملاين تلقائياً** |
| **Add Clip** | `أضف كليب باسم البداية من 0 إلى 5 ثوان على التراك 0` | `add clip Intro on track 0 from 0s to 5s` | يضيف كليباً بالاسم والزمن المحدد |
| **Trim Clip** | `احذف أول 3 ثوان من التايملاين` | `cut the first 3 seconds` | يقص الزمن المحدد من أول كليب |
| **Move Clip** | `انقل كليب البداية إلى الثانية 10` | `move clip Intro to 10s` | ينقل كليب محدد إلى نقطة زمنية جديدة |
| **General** | `تراجع` / `أعد تنفيذ` | `undo` / `redo` | عمليات التراجع وإعادة التنفيذ على التايملاين |

---

## ── Codebase Layout / هيكلية الملفات البرمجية

```
seeing/
├── CMakeLists.txt                  # CMake build definition (C++20, Qt6)
├── Dockerfile                      # Standardized rootless container build
├── build.sh                        # Automation compilation script
├── run.sh                          # GUI display-forwarding run script
├── src/
│   ├── main.cpp                    # Entry point, theme configuration, MVC setup
│   ├── model/
│   │   ├── project_model.h/cpp     # JSON-based NLE project state tracker
│   ├── view/
│   │   ├── main_window.h/cpp       # 4-panel split visual window layout
│   │   ├── media_panel.h/cpp       # Media asset manager and interactive explorer
│   │   ├── settings_dialog.h/cpp   # Dual-AI (VLM & LLM) engine selector dialog
│   │   ├── timeline_panel.h/cpp    # Multitrack interactive timeline visualization
│   │   └── chat_panel.h/cpp        # NLE assistant chat dialog interface
│   ├── controller/
│   │   └── editor_controller.h/cpp # Routes and triggers AI commands & JSON mutations
│   └── ai/
│       ├── http_ai_engine.h/cpp    # Cloud provider adapter (OpenAI, Gemini, Ollama)
│       ├── marlin_server.py        # Dynamic routing python gateway for local/cloud VLMs
│       └── ai_engine_factory.h/cpp # Factory creator pattern for co-pilot engines
```

## License

This project is licensed under the MIT License.
