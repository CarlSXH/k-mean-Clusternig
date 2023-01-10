

#define _CRTDBG_MAP_ALLOC

#include <stdlib.h>  
#include <crtdbg.h>  

#include <fstream>
#include <vector>
#include <windows.h>

#include <d2d1.h>
#include <d2d1_1.h>
#include <dwrite.h>

#pragma comment(lib, "D2D1.lib")
#pragma comment(lib, "Dwrite.lib")
#pragma comment(lib, "Dxguid.lib")
#pragma comment(lib, "Windowscodecs.lib")


#include <wincodec.h>
#include <string>
#include <vector>
#include <ctime>

#ifndef _UNICODE
typedef CHAR Char;
#define Text(t) (t)
#else
typedef WCHAR Char;
#define Text(t) L##t
#endif

#ifdef _DEBUG
#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
// Replace _NORMAL_BLOCK with _CLIENT_BLOCK if you want the
// allocations to be of _CLIENT_BLOCK type
#define new DBG_NEW
#else
#define DBG_NEW new
#endif

using namespace std;
using namespace D2D1;

// Global Variables:
HINSTANCE hInst;                                // current instance
const Char *szTitle;                  // The title bar text
const Char *szWindowClass;            // the main window class name
HWND  g_hWnd;


ID2D1Factory1         *g_pFactory = NULL;
IDWriteFactory        *g_pWriteFactory = NULL;

IDWriteTextFormat     *g_pTextFormat = NULL;


int g_nWidth = 700;
int g_nHeight = 700;


// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

HRESULT             InitD2D();
void                CleanUpD2D();



HRESULT InitD2D()
{
    HRESULT hr;
    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE::D2D1_FACTORY_TYPE_SINGLE_THREADED, &g_pFactory);
    if (FAILED(hr))
        return hr;

    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE::DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&g_pWriteFactory);
    hr = g_pWriteFactory->CreateTextFormat(L"Microsoft Sans Serif", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 20, L"", &g_pTextFormat);



    return hr;
}
void CleanUpD2D()
{
    if (g_pFactory != NULL)
    {
        g_pFactory->Release();
        g_pFactory = NULL;
    }
    if (g_pTextFormat != NULL)
    {
        g_pTextFormat->Release();
        g_pTextFormat = NULL;
    }
    if (g_pWriteFactory != NULL)
    {
        g_pWriteFactory->Release();
        g_pWriteFactory = NULL;
    }
}

void Refresh(HWND hWnd)
{
    RECT rect;
    GetClientRect(hWnd, &rect);
    InvalidateRect(hWnd, &rect, TRUE);
    UpdateWindow(hWnd);
}


struct Point
{
    float x, y;
};

struct Example
{
    Point p;
    char flag;
};

vector<Example> *g_Examples;

bool g_bHasAnswer = false;
float g_Cost = 0.0f;

int g_GroupCount = 3;
#define MAX_GROUPCOUNT 10

D2D1_COLOR_F g_Color[MAX_GROUPCOUNT] =
{
    ColorF(ColorF::Red),
    ColorF(ColorF::LightGreen),
    ColorF(ColorF::Blue),
    ColorF(ColorF::Yellow),
    ColorF(ColorF::Cyan),
    ColorF(ColorF::Magenta),
    ColorF(ColorF::Olive),
    ColorF(ColorF::Purple),
    ColorF(ColorF::Maroon),
    ColorF(ColorF::Navy),
};


struct Temp
{
    Point *centroids;


    Point *average;

    int *averageCount;
};

float GetAnswer(vector<Example> *examples, Temp temp)
{
    if (examples->size() < 3)
        return -1;


    Point *centroids = temp.centroids;

    Point *average = temp.average;

    // temp.averageCount is used twice
    // first use is to store the centroid
    // second use is the average count
    int *averageCount = temp.averageCount;

    ZeroMemory(centroids, sizeof(Point) * g_GroupCount);
    ZeroMemory(average, sizeof(Point) * g_GroupCount);
    ZeroMemory(averageCount, sizeof(int) * g_GroupCount);

    for (int i = 0; i < g_GroupCount; i++)
    {
        int index = 0;
        bool valid = true;
        do
        {
            valid = true;
            index = rand() % examples->size();
            for (int j = i - 1; j > 0; j--)
            {
                if (averageCount[j] == index)
                {
                    valid = false;
                    break;
                }
            }
        } while (!valid);
        centroids[i] = examples->at(index).p;
    }



    bool changed = false;

    do
    {
        for (int i = 0; i < g_GroupCount; i++)
        {
            average[i].x = 0;
            average[i].y = 0;
            averageCount[i] = 0;
        }

        changed = false;
        for (int i = 0; i < examples->size(); i++)
        {
            Example ex = examples->at(i);
            float min = FLT_MAX;
            int index = -1;
            for (int j = 0; j < g_GroupCount; j++)
            {
                Point c = centroids[j];
                float dis = (ex.p.x - c.x) * (ex.p.x - c.x) + (ex.p.y - c.y) * (ex.p.y - c.y);
                if (dis < min)
                {
                    min = dis;
                    index = j;
                }
            }

            if (examples->at(i).flag != index)
                changed = true;
            examples->at(i).flag = index;
        }

        for (int i = 0; i < examples->size(); i++)
        {
            Example ex = examples->at(i);
            average[ex.flag].x += ex.p.x;
            average[ex.flag].y += ex.p.y;
            averageCount[ex.flag]++;
        }

        for (int i = 0; i < g_GroupCount; i++)
        {
            centroids[i] = { average[i].x / averageCount[i] , average[i].y / averageCount[i] };
        }

    } while (changed);


    float cost = 0;
    for (int i = 0; i < examples->size(); i++)
    {
        Example ex = examples->at(i);
        float xdif = centroids[ex.flag].x - ex.p.x;
        float ydif = centroids[ex.flag].y - ex.p.y;
        cost += xdif * xdif + ydif * ydif;
    }

    return cost;
}

void Calculate()
{
    vector<Example> examples = vector<Example>(*g_Examples);

    float mincost = FLT_MAX;
    vector<Example> bestExamples;
    Temp temp;

    temp.centroids = new Point[g_GroupCount];
    temp.average = new Point[g_GroupCount];
    temp.averageCount = new int[g_GroupCount];

    for (int i = 0; i < 100; i++)
    {
        float cost = GetAnswer(&examples, temp);
        if (cost < mincost)
        {
            mincost = cost;
            bestExamples = vector<Example>(examples);
        }
    }

    delete[] temp.centroids;
    delete[] temp.average;
    delete[] temp.averageCount;

    g_bHasAnswer = true;

    g_Cost = mincost;

    *g_Examples = vector<Example>(bestExamples);
}

bool g_AutoRefresh = false;

void AutoRefreshPoints()
{
    if (g_AutoRefresh)
    {
        if (g_Examples->size() > g_GroupCount)
            Calculate();
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_ERASEBKGND:
    {
        ID2D1HwndRenderTarget *pRenderTarget = NULL;
        HRESULT hr = g_pFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(g_hWnd, SizeU(g_nWidth, g_nHeight)), &pRenderTarget);

        pRenderTarget->BeginDraw();
        pRenderTarget->Clear(ColorF(ColorF::White));

        ID2D1SolidColorBrush *pBrush;

        pRenderTarget->CreateSolidColorBrush(ColorF(ColorF::Black, 1.0f), &pBrush);


        float r = 3;
        for (int i = 0; i < g_Examples->size(); i++)
        {
            pRenderTarget->DrawEllipse(Ellipse(Point2F(g_Examples->at(i).p.x * 700, g_Examples->at(i).p.y * 700), r, r), pBrush, 2.5);
        }


        for (int i = 0; i < g_Examples->size(); i++)
        {
            Example ex = g_Examples->at(i);

            if (ex.flag <= -1)
                pBrush->SetColor(ColorF(ColorF::Black));
            else
                pBrush->SetColor(g_Color[ex.flag]);

            pRenderTarget->FillEllipse(Ellipse(Point2F(g_Examples->at(i).p.x * 700, g_Examples->at(i).p.y * 700), r, r), pBrush);
        }

        pBrush->SetColor(ColorF(ColorF::Black));

        wstring s = to_wstring(g_GroupCount);
        pRenderTarget->DrawTextA(s.c_str(), s.size(), g_pTextFormat, { 0, 0, 40, 20 }, pBrush);

        s = to_wstring(g_Examples->size());
        pRenderTarget->DrawTextA(s.c_str(), s.size(), g_pTextFormat, { 0, 20, 700, 40 }, pBrush);

        s = to_wstring(g_Cost);
        pRenderTarget->DrawTextA(s.c_str(), s.size(), g_pTextFormat, { 0, 40, 700, 60 }, pBrush);







        pBrush->Release();
        pRenderTarget->EndDraw();

        pRenderTarget->Release();

        break;
    }
    case WM_EXITSIZEMOVE:
    {
        RECT rect;
        GetClientRect(hWnd, &rect);

        g_nWidth = rect.right - rect.left;
        g_nHeight = rect.bottom - rect.top;

        break;
    }
    case WM_MOUSEMOVE:
    {
        break;
    }
    case WM_LBUTTONUP:
    {
        int xPos = (lParam & 0x0000FFFF);
        int yPos = (lParam >> 16);

        g_bHasAnswer = false;

        Example ex;
        ex.p.x = xPos / 700.0f;
        ex.p.y = yPos / 700.0f;
        ex.flag = -1;
        g_Examples->push_back(ex);

        AutoRefreshPoints();

        Refresh(hWnd);
        break;
    }
    case WM_KEYUP:
    {
        switch (wParam)
        {
        case 'S':
        {
            if (g_Examples->size() > g_GroupCount)
                Calculate();
            Refresh(hWnd);
            break;
        }
        case 'C':
        {
            g_Examples->clear();
            Refresh(hWnd);
            break;
        }
        case 'R':
        {
            g_AutoRefresh = !g_AutoRefresh;
            break;
        }
        default:
        {
            if (wParam == '0')
                g_GroupCount = 10;
            if (wParam >= '3' && wParam <= '9')
                g_GroupCount = wParam - '0';

            AutoRefreshPoints();
            Refresh(hWnd);
            break;
        }
        }

        break;
    }
    case WM_DESTROY:
    {
        PostQuitMessage(0);
        break;
    }
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPLICATION));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_APPLICATION));

    return RegisterClassEx(&wcex);
}
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance; // Store instance handle in our global variable

    g_hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, g_nWidth + 16, g_nHeight + 39, NULL, NULL, hInstance, NULL);

    if (!g_hWnd)
    {
        return FALSE;
    }

    ShowWindow(g_hWnd, nCmdShow);
    UpdateWindow(g_hWnd);

    return TRUE;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    g_Examples = new vector<Example>();

    if (FAILED(InitD2D()))
    {
        MessageBox(NULL, Text("Failed creating D2D devices!"), Text("ERROR"), 0);
        return FALSE;
    }

    // Initialize global strings
    szTitle = Text("Title");
    szWindowClass = Text("Animation2D");
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance(hInstance, nCmdShow))
    {
        MessageBox(NULL, Text("Failed creating window!"), Text("ERROR"), 0);
        return FALSE;
    }


    MSG msg;
    ZeroMemory(&msg, sizeof(MSG));

    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    CleanUpD2D();

    delete g_Examples;

    _CrtDumpMemoryLeaks();

    return (int)msg.wParam;
}