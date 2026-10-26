#include <windows.h>
#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cwchar>
 
 
// Результаты расчёта
struct LifeStats
{
    long long daysLived   = 0;
 
    double    sleepHours  = 0;   // часов проспал
    long long sleepDays   = 0;   // суток проспал
 
    long long blinks      = 0;   // морганий
 
    long long heartBeats  = 0;   // ударов сердца
    double    bloodLiters = 0;   // литров перекачанной крови
 
    double    waterLiters = 0;   // литров выпитой воды
 
    long long laughs      = 0;   // раз посмеялся
};
 
// Интерфейс наблюдателя 
class IModelObserver
{
public:
    virtual void OnModelChanged() = 0;
    virtual ~IModelObserver() {}
};
 

class LifeModel
{
public:
    void Attach(IModelObserver* obs) { observers.push_back(obs); }
 
    // Установить дату рождения. При успехе - пересчитать статистику
    // и уведомить всех наблюдателей. При ошибке - вернуть false и текст ошибки.
    bool SetBirthDate(int day, int month, int year, std::wstring& errorMsg)
    {
        if (!IsValidDate(day, month, year, errorMsg))
            return false;
 
        birthDay = day;
        birthMonth = month;
        birthYear = year;
        hasData = true;
 
        Calculate();
        Notify();               
        return true;
    }
 
    bool HasData()   const { return hasData; }
    int  GetDay()    const { return birthDay; }
    int  GetMonth()  const { return birthMonth; }
    int  GetYear()   const { return birthYear; }
    const LifeStats& GetStats() const { return stats; }
 
private:
    int  birthDay = 1, birthMonth = 1, birthYear = 2000;
    bool hasData = false;
    LifeStats stats;
    std::vector<IModelObserver*> observers;
 
    void Notify()
    {
        for (auto* o : observers)
            o->OnModelChanged();
    }
 
    static bool IsLeap(int y)
    {
        return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
    }
 
    static int DaysInMonth(int m, int y)
    {
        static const int dm[] = {31,28,31,30,31,30,31,31,30,31,30,31};
        if (m < 1 || m > 12) return 0;
        if (m == 2 && IsLeap(y)) return 29;
        return dm[m - 1];
    }
 
    bool IsValidDate(int d, int m, int y, std::wstring& err)
    {
        SYSTEMTIME st;
        GetLocalTime(&st);
 
        if (y < 1900 || y > (int)st.wYear)
        {
            err = L"Год должен быть в диапазоне от 1900 до текущего.";
            return false;
        }
        if (m < 1 || m > 12)
        {
            err = L"Месяц должен быть числом от 1 до 12.";
            return false;
        }
        int maxDay = DaysInMonth(m, y);
        if (d < 1 || d > maxDay)
        {
            err = L"Некорректный день для указанного месяца/года.";
            return false;
        }
        if (y == (int)st.wYear &&
            (m > (int)st.wMonth || (m == (int)st.wMonth && d > (int)st.wDay)))
        {
            err = L"Дата рождения не может быть в будущем.";
            return false;
        }
        return true;
    }
 
    static long long ToJDN(int d, int m, int y)
    {
        int a  = (14 - m) / 12;
        int yy = y + 4800 - a;
        int mm = m + 12 * a - 3;
        return d + (153 * mm + 2) / 5 + 365LL * yy + yy / 4 - yy / 100 + yy / 400 - 32045;
    }
 
    static long long DaysBetween(int d1,int m1,int y1,int d2,int m2,int y2)
    {
        long long diff = ToJDN(d2,m2,y2) - ToJDN(d1,m1,y1);
        return diff < 0 ? 0 : diff;
    }
 
    void Calculate()
    {
        SYSTEMTIME st;
        GetLocalTime(&st);
 
        long long days = DaysBetween(birthDay, birthMonth, birthYear,
                                      st.wDay, st.wMonth, st.wYear);
        stats.daysLived = days;
 
        
        const double SLEEP_HOURS_PER_DAY   = 8.0;   // средняя продолжительность сна в сутки
        const double AWAKE_HOURS_PER_DAY   = 16.0;  // часы бодрствования в сутки
        const double BLINKS_PER_MINUTE     = 15.0;  // среднее число морганий в минуту (при бодрствовании)
        const double HEARTBEATS_PER_MINUTE = 70.0;  // средний пульс покоя, уд/мин
        const double BLOOD_ML_PER_BEAT     = 70.0;  // мл крови, перекачиваемой за одно сокращение
        const double WATER_LITERS_PER_DAY  = 2.0;   // рекомендуемая норма воды в сутки
        const double LAUGHS_PER_DAY        = 18.0;  // среднее число смехов взрослого человека в сутки

 
        double totalSleepHours = days * SLEEP_HOURS_PER_DAY;
        stats.sleepHours = totalSleepHours;
        stats.sleepDays  = (long long)(totalSleepHours / 24.0);
 
        double awakeMinutes = days * AWAKE_HOURS_PER_DAY * 60.0;
        stats.blinks = (long long)(awakeMinutes * BLINKS_PER_MINUTE);
 
        double totalMinutes = days * 24.0 * 60.0;
        double beats = totalMinutes * HEARTBEATS_PER_MINUTE;
        stats.heartBeats  = (long long)beats;
        stats.bloodLiters = beats * BLOOD_ML_PER_BEAT / 1000.0;
 
        stats.waterLiters = days * WATER_LITERS_PER_DAY;
        stats.laughs      = (long long)(days * LAUGHS_PER_DAY);
    }
};
 
    
static HINSTANCE g_hInstance = nullptr;
static const wchar_t* MAIN_CLASS  = L"LifeCalcMainWnd";
static const wchar_t* INPUT_CLASS = L"LifeCalcInputWnd";
 
class InputForm; // предварительное объявление
 
// =====================================================================
//  VIEW + запуск КОНТРОЛЛЕРА (главная форма)
// =====================================================================
class MainForm : public IModelObserver
{
public:
    explicit MainForm(LifeModel& m) : model(m) {}
 
    bool Create()
    {
        hwnd = CreateWindowExW(
            0, MAIN_CLASS, L"Калькулятор Жизни",
            (WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME),
            CW_USEDEFAULT, CW_USEDEFAULT, 480, 430,
            nullptr, nullptr, g_hInstance, this);
        return hwnd != nullptr;
    }
 
    void Show(int nCmdShow)
    {
        ShowWindow(hwnd, nCmdShow);
        UpdateWindow(hwnd);
    }
 
    void OnModelChanged() override { RefreshResults(); }   // View реагирует на модель
 
    HWND       GetHwnd()  const { return hwnd; }
    LifeModel& GetModel()       { return model; }
 
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
 
private:
    static const int ID_BTN_INPUT = 101;
 
    LifeModel& model;
    HWND hwnd       = nullptr;
    HWND btnInput   = nullptr;
    HWND lblResults = nullptr;
 
    void CreateControls()
    {
        btnInput = CreateWindowW(L"BUTTON", L"Ввести данные",
            WS_CHILD | WS_VISIBLE, 20, 20, 180, 32,
            hwnd, (HMENU)(INT_PTR)ID_BTN_INPUT, g_hInstance, nullptr);
 
        lblResults = CreateWindowW(L"STATIC", L"Данные о дате рождения не введены.",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            20, 70, 430, 300, hwnd, nullptr, g_hInstance, nullptr);
    }
 
    void RefreshResults()
    {
        if (!model.HasData())
        {
            SetWindowTextW(lblResults, L"Данные о дате рождения не введены.");
            return;
        }
        const LifeStats& s = model.GetStats();
        wchar_t buf[1024];
        swprintf(buf, 1024,
            L"Дата рождения: %02d.%02d.%04d\r\n"
            L"Прожито дней: %lld\r\n\r\n"
            L"Проспал часов: %.0f  (~%lld суток)\r\n"
            L"Сделано морганий: %lld\r\n"
            L"Ударов сердца: %lld\r\n"
            L"Перекачано крови: %.1f л\r\n"
            L"Выпито воды: %.1f л\r\n"
            L"Смеялся раз: %lld",
            model.GetDay(), model.GetMonth(), model.GetYear(),
            s.daysLived,
            s.sleepHours, s.sleepDays,
            s.blinks,
            s.heartBeats,
            s.bloodLiters,
            s.waterLiters,
            s.laughs);
        SetWindowTextW(lblResults, buf);
    }
 
    friend class InputForm;
 
public:
    void HandleCommand(WPARAM wParam);
};
 
// =====================================================================
//  VIEW + КОНТРОЛЛЕР окна ввода даты (свой контрол вместо DateTimePicker)
// =====================================================================
class InputForm
{
public:
    explicit InputForm(MainForm& mf) : mainForm(mf) {}
 
    void ShowModal()
    {
        HWND owner = mainForm.GetHwnd();
        EnableWindow(owner, FALSE);
 
        hwnd = CreateWindowExW(
            WS_EX_DLGMODALFRAME, INPUT_CLASS, L"Ввод даты рождения",
            WS_POPUP | WS_CAPTION | WS_SYSMENU,
            CW_USEDEFAULT, CW_USEDEFAULT, 300, 230,
            owner, nullptr, g_hInstance, this);
 
        // Центрируем относительно главного окна
        RECT ro, ri;
        GetWindowRect(owner, &ro);
        GetWindowRect(hwnd, &ri);
        int w = ri.right - ri.left, h = ri.bottom - ri.top;
        int x = ro.left + ((ro.right - ro.left) - w) / 2;
        int y = ro.top  + ((ro.bottom - ro.top) - h) / 2;
        SetWindowPos(hwnd, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
 
        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);
 
        closed = false;
        MSG msg;
        while (!closed && GetMessageW(&msg, nullptr, 0, 0))
        {
            if (!IsDialogMessageW(hwnd, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
        }
 
        EnableWindow(owner, TRUE);
        SetForegroundWindow(owner);
    }
 
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
 
private:
    static const int ID_EDIT_DAY = 201, ID_EDIT_MONTH = 202, ID_EDIT_YEAR = 203;
    static const int ID_BTN_OK = 210, ID_BTN_CANCEL = 211;
 
    MainForm& mainForm;
    HWND hwnd = nullptr;
    HWND editDay = nullptr, editMonth = nullptr, editYear = nullptr;
    bool closed = false;
 
    void CreateControls()
    {
        CreateWindowW(L"STATIC", L"День:",   WS_CHILD|WS_VISIBLE, 20,20,50,20, hwnd,nullptr,g_hInstance,nullptr);
        editDay = CreateWindowW(L"EDIT", L"", WS_CHILD|WS_VISIBLE|WS_BORDER|ES_NUMBER|ES_AUTOHSCROLL,
                                 90,18,70,24, hwnd,(HMENU)(INT_PTR)ID_EDIT_DAY, g_hInstance,nullptr);
 
        CreateWindowW(L"STATIC", L"Месяц:",  WS_CHILD|WS_VISIBLE, 20,55,50,20, hwnd,nullptr,g_hInstance,nullptr);
        editMonth = CreateWindowW(L"EDIT", L"", WS_CHILD|WS_VISIBLE|WS_BORDER|ES_NUMBER|ES_AUTOHSCROLL,
                                   90,53,70,24, hwnd,(HMENU)(INT_PTR)ID_EDIT_MONTH, g_hInstance,nullptr);
 
        CreateWindowW(L"STATIC", L"Год:",    WS_CHILD|WS_VISIBLE, 20,90,50,20, hwnd,nullptr,g_hInstance,nullptr);
        editYear = CreateWindowW(L"EDIT", L"", WS_CHILD|WS_VISIBLE|WS_BORDER|ES_NUMBER|ES_AUTOHSCROLL,
                                  90,88,90,24, hwnd,(HMENU)(INT_PTR)ID_EDIT_YEAR, g_hInstance,nullptr);
 
        CreateWindowW(L"BUTTON", L"OK", WS_CHILD|WS_VISIBLE|BS_DEFPUSHBUTTON,
                      40,150,90,30, hwnd,(HMENU)(INT_PTR)ID_BTN_OK, g_hInstance,nullptr);
        CreateWindowW(L"BUTTON", L"Отмена", WS_CHILD|WS_VISIBLE,
                      160,150,90,30, hwnd,(HMENU)(INT_PTR)ID_BTN_CANCEL, g_hInstance,nullptr);
 
        // Восстановление последних введённых данных из модели
        LifeModel& model = mainForm.GetModel();
        wchar_t buf[16];
        if (model.HasData())
        {
            swprintf(buf, 16, L"%d", model.GetDay());   SetWindowTextW(editDay, buf);
            swprintf(buf, 16, L"%d", model.GetMonth()); SetWindowTextW(editMonth, buf);
            swprintf(buf, 16, L"%d", model.GetYear());  SetWindowTextW(editYear, buf);
        }
        SetFocus(editDay);
    }
 
    void ShowError(const wchar_t* text)
    {
        MessageBoxW(hwnd, text, L"Ошибка ввода", MB_OK | MB_ICONERROR);
    }
 
    // ==== Контроллер: реагирует на нажатие кнопок, обновляет модель ====
    void OnOk()
    {
        wchar_t bufD[16], bufM[16], bufY[16];
        GetWindowTextW(editDay,   bufD, 16);
        GetWindowTextW(editMonth, bufM, 16);
        GetWindowTextW(editYear,  bufY, 16);
 
        if (wcslen(bufD) == 0 || wcslen(bufM) == 0 || wcslen(bufY) == 0)
        {
            ShowError(L"Все поля (день, месяц, год) должны быть заполнены.");
            return;
        }
 
        wchar_t* endp = nullptr;
        long d = wcstol(bufD, &endp, 10);
        if (*endp != L'\0') { ShowError(L"Поле \"День\" должно содержать целое число."); return; }
        long m = wcstol(bufM, &endp, 10);
        if (*endp != L'\0') { ShowError(L"Поле \"Месяц\" должно содержать целое число."); return; }
        long y = wcstol(bufY, &endp, 10);
        if (*endp != L'\0') { ShowError(L"Поле \"Год\" должно содержать целое число."); return; }
 
        std::wstring err;
        if (!mainForm.GetModel().SetBirthDate((int)d, (int)m, (int)y, err))
        {
            ShowError(err.c_str());
            return; // остаёмся в окне, чтобы пользователь исправил данные
        }
 
        closed = true;
        DestroyWindow(hwnd);
    }
 
    void OnCancel()
    {
        closed = true;
        DestroyWindow(hwnd);
    }
 
public:
    void HandleCommand(WPARAM wParam)
    {
        switch (LOWORD(wParam))
        {
        case ID_BTN_OK:     OnOk();     break;
        case ID_BTN_CANCEL: OnCancel(); break;
        }
    }
    void HandleCreate() { CreateControls(); }
};
 
// =====================================================================
//  Оконные процедуры
// =====================================================================
void MainForm::HandleCommand(WPARAM wParam)
{
    if (LOWORD(wParam) == ID_BTN_INPUT)
    {
        InputForm form(*this);   // Controller создаётся по действию пользователя
        form.ShowModal();
    }
}
 
LRESULT CALLBACK MainForm::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    MainForm* self;
    if (msg == WM_NCCREATE)
    {
        auto cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        self = reinterpret_cast<MainForm*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)self);
        self->hwnd = hwnd;
    }
    else
    {
        self = reinterpret_cast<MainForm*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }
 
    switch (msg)
    {
    case WM_CREATE:
        self->CreateControls();
        return 0;
    case WM_COMMAND:
        self->HandleCommand(wParam);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}
 
LRESULT CALLBACK InputForm::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    InputForm* self;
    if (msg == WM_NCCREATE)
    {
        auto cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        self = reinterpret_cast<InputForm*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)self);
        self->hwnd = hwnd;
    }
    else
    {
        self = reinterpret_cast<InputForm*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }
 
    switch (msg)
    {
    case WM_CREATE:
        self->HandleCreate();
        return 0;
    case WM_COMMAND:
        self->HandleCommand(wParam);
        return 0;
    case WM_CLOSE:
        self->closed = true;
        DestroyWindow(hwnd);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}
 
// =====================================================================
//  Точка входа
// =====================================================================
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    g_hInstance = hInstance;
 
    WNDCLASSW wcMain = {};
    wcMain.lpfnWndProc   = MainForm::WndProc;
    wcMain.hInstance     = hInstance;
    wcMain.lpszClassName = MAIN_CLASS;
    wcMain.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wcMain.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassW(&wcMain);
 
    WNDCLASSW wcInput = {};
    wcInput.lpfnWndProc   = InputForm::WndProc;
    wcInput.hInstance     = hInstance;
    wcInput.lpszClassName = INPUT_CLASS;
    wcInput.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wcInput.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassW(&wcInput);
 
    LifeModel model;              // MODEL
    MainForm  mainForm(model);    // VIEW (+ запуск CONTROLLER)
    model.Attach(&mainForm);      // подписываем View на модель (активная модель)
 
    if (!mainForm.Create())
        return -1;
 
    mainForm.Show(nCmdShow);
 
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}