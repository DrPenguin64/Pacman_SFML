#pragma once
#include "resource.h"
#include <windows.h>
#include <string>

struct ResizeDialog_InputData
{
    int a = 0;
    int b = 0;
    bool confirmed = false;
};

INT_PTR CALLBACK InputDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static ResizeDialog_InputData* data = nullptr;

    switch (msg)
    {
    case WM_INITDIALOG:
        data = reinterpret_cast<ResizeDialog_InputData*>(lParam);
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
        {
            char buf[32];

            GetDlgItemTextA(hDlg, IDC_EDIT_A, buf, sizeof(buf));
            data->a = atoi(buf);

            GetDlgItemTextA(hDlg, IDC_EDIT_B, buf, sizeof(buf));
            data->b = atoi(buf);

            data->confirmed = true;
            EndDialog(hDlg, IDOK);
            return TRUE;
        }

        case IDCANCEL:
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
        }
        break;
    }
    return FALSE;
}