#include "Header/Main.h"

using namespace Injector;
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR pCmdLine, _In_ int nShowCmd)
{
    WNDCLASSEX WindowClass = { sizeof(WNDCLASSEX), CS_CLASSDC, UI::WndProc, 0L, 0L, GetModuleHandle(NULL), LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1)), NULL, NULL, NULL, _T("SMI_MainWindow"), NULL };
    RegisterClassEx(&WindowClass);

    UI::MainWindowHandle = CreateWindow(WindowClass.lpszClassName, L"Simple Module Injector", WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, 100, 100, 500, 188, NULL, NULL, WindowClass.hInstance, NULL);

    if (!UI::CreateDirectXDeviceAndSwapChain(UI::MainWindowHandle))
    {
        MessageBoxA(UI::MainWindowHandle, "Failed to create Direct3D device", NULL, MB_OK | MB_ICONERROR);
        UI::CleanupDirectXDeviceAndSwapChain();
        UnregisterClass(WindowClass.lpszClassName, WindowClass.hInstance);
        return EXIT_FAILURE;
    }

    //Show Window
    ShowWindow(UI::MainWindowHandle, SW_NORMAL);

    //Initialize imGui
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = NULL;
    ImGui_ImplWin32_Init(UI::MainWindowHandle);
    ImGui_ImplDX11_Init(UI::g_pd3dDevice, UI::g_pd3dDeviceContext);
    io.Fonts->AddFontFromFileTTF(strcat(getenv("SystemDrive"), "\\Windows\\Fonts\\Verdana.ttf"), 23.f);

    //Window Loop
    MSG Message = { 0 };
    while (Message.message != WM_QUIT)
    {
        InjectorFunctions::Loop();
        if (PeekMessage(&Message, NULL, 0U, 0U, PM_REMOVE)) { TranslateMessage(&Message); DispatchMessage(&Message); continue; }

        //Start ImGui Frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
       
        //Push Styles
        UI::SetImGuiStyles();

        //Show Popup Notification Loop
        if (!UI::PopupNotificationMessage.empty())
        {
            ImGui::OpenPopup("###PopupNotification");
            if (ImGui::BeginPopupModal("###PopupNotification", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar))
            {
                ImGui::TextWrapped(UI::PopupNotificationMessage.c_str());
                if (ImGui::Button("OK", ImVec2(400, 0))) 
                {
                    UI::PopupNotificationMessage.clear();
                    ImGui::CloseCurrentPopup();
                }
            }
        }

        //ImGui Window Setup
        ImGui::SetNextWindowSize(ImVec2(500, 149));
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::Begin("MainWindow", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
 
        //Module Select Section
        ImGui::Text("Module Name:");
        ImGui::SameLine();
        if (UI::SelectedModuleFile != NULL) { ImGui::TextWrapped(PathFindFileNameA(UI::SelectedModuleFile)); }
        ImGui::SameLine(ImGui::GetWindowWidth() - 160);
        if (ImGui::SmallButton("Select Module")) 
        {
            char* SelectedFile = UI::ShowSelectFileDialogAndReturnPath();
            if (SelectedFile != NULL)
            {
                if (InjectorFunctions::FileHasDOSSignature(SelectedFile))
                {
                    UI::SelectedModuleFile = SelectedFile;
                }
                else
                {
                    UI::PopupNotificationMessage = "Selected file is not an valid NT executable";
                }
            }
        }

        //Process Name Input Section
        ImGui::Text("Process Name/ID:");
        ImGui::SameLine();
        ImGui::PushItemWidth(292);
        ImGui::InputText("##ProcessNameOrIDInput", UI::TargetProcessNameOrIDBufferInput, IM_ARRAYSIZE(UI::TargetProcessNameOrIDBufferInput), ImGuiInputTextFlags_CharsNoBlank);
        ImGui::Dummy(ImVec2(0, 5));
        if (ImGui::Button("Inject module", ImVec2(470, 35)))
        {
            if (!UI::SelectedModuleFile)
            {
               UI::PopupNotificationMessage = "You must select a module to inject";
            }
            else
            {
                std::string TargetProcessNameString(UI::TargetProcessNameOrIDBufferInput);
                if (!TargetProcessNameString.empty())
                {
                    if (std::all_of(TargetProcessNameString.begin(), TargetProcessNameString.end(), isdigit))
                    {
                        InjectorFunctions::InjectModule(UI::SelectedModuleFile, L"", std::stoi(TargetProcessNameString));
                    }
                    else
                    {
                        std::wstring ProcessNameWS(TargetProcessNameString.begin(), TargetProcessNameString.end());
                        InjectorFunctions::InjectModule(UI::SelectedModuleFile, ProcessNameWS, NULL);
                    }
                }
                else
                {
                    UI::PopupNotificationMessage = "You must provide a process name/id";
                }
            }
        }
        std::string VersionFooter = "v" + SMI_BUILD;
        ImGui::Text(VersionFooter.c_str());
        ImGui::SameLine(ImGui::GetWindowWidth() - 85);
        if (ImGui::Button("About"))
        {
            ImGui::OpenPopup("About SMI###AboutPopup");
        }
        if (ImGui::BeginPopupModal("About SMI###AboutPopup", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar))
        {
            std::string AuthorText = "Author: HatchesPls\nCompiled: " + (std::string)__DATE__ + " " + (std::string)__TIME__ + "\nGithub: https://git.io/J0HCq";
            ImGui::TextWrapped(AuthorText.c_str());
            if (ImGui::Button("Close", ImVec2(500, 0)))
            {
                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::End();
        ImGui::Render();
        UI::g_pd3dDeviceContext->OMSetRenderTargets(1, &UI::g_mainRenderTargetView, NULL);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        static UINT presentFlags = 0;
        if (UI::g_pSwapChain->Present(1, presentFlags) == DXGI_STATUS_OCCLUDED)
        {
            presentFlags = DXGI_PRESENT_TEST;
            Sleep(50);
        }
        else 
        {
            presentFlags = 0;
        }
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    UI::CleanupDirectXDeviceAndSwapChain();
    DestroyWindow(UI::MainWindowHandle);
    UnregisterClass(WindowClass.lpszClassName, WindowClass.hInstance);
    return EXIT_SUCCESS;
}