#include <windows.h>
#include <tchar.h>
#include "resource.h"
#include <CommCtrl.h>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <ctime> 
using namespace std;

wstring word;
HWND hProgress;
int countSecond;
bool wordFounD;
bool stopProc = true;

void CleanUpTheResultFile()
{
    ofstream resultFile("summary.txt", ios::out | ios::trunc);
    if (resultFile.is_open()) {
        resultFile.close();
        stopProc = false;
    } else {
        MessageBox(NULL, L"Error clearing result file 'summary.txt'!", L"Error", MB_OK | MB_ICONERROR);
    }
}

wstring ReplaceWordOnStars(const wstring& line, const wstring& word, int& repl)
{
    wstring rez = line;
    size_t pos = 0;
    while ((pos = rez.find(word, pos)) != wstring::npos) {
        rez.replace(pos, word.length(), word.length(), L'*');
        pos += word.length();
        repl++;
    }
    return rez;
}

void CheckIfThereIsResult(HWND hWnd, bool wordFound, const wstring& word)
{
    if (wordFound) {
        wstring message = L"Word '" + word + L"' is found";
        MessageBox(hWnd, message.c_str(), L"Information", MB_OK | MB_ICONINFORMATION);
    }
    else {
        wstring message = L"Word '" + word + L"' is not found";
        MessageBox(hWnd, message.c_str(), L"Information", MB_OK | MB_ICONINFORMATION);
    }
}

void OpenAndVerifyTheFile(const wstring& fileName, const wstring& word, wofstream& resultFile, int& repl, bool& wordFound)
{
    ifstream file(fileName, ios::binary | ios::ate);
    if (file.is_open()) {
        streampos fileSize = file.tellg();
        file.seekg(0, ios::beg);
        string line;
        while (getline(file, line)) {
            wstring wstrLine(line.begin(), line.end());
            if (wstrLine.find(word) != wstring::npos) {
                wstrLine = ReplaceWordOnStars(wstrLine, word, repl);
                wordFound = true;

                resultFile << wstrLine << endl;
            }
        }
        if (repl > 0) {
            resultFile << "_____________________________" << endl;
            resultFile << "Name: " << fileName << endl;
            resultFile << "Size: " << fileSize << " bytes" << endl;
            resultFile << "Number of replacements: " << repl << endl;
            resultFile << "_____________________________" << endl;
        }
        file.close();
    }
}

void SearchFileFindWords(const wstring& directory, const wstring& word)
{
    bool wordFound = false;
    int totalReplacements = 0; 
    wofstream resultFile("summary.txt", ios::out | ios::trunc);
    if (resultFile.is_open()) {
        WIN32_FIND_DATA findFileData;
        HANDLE hFind = FindFirstFile((directory + L"\\*.txt").c_str(), &findFileData);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                wstring fileName = findFileData.cFileName;
                int replacements = 0;
                OpenAndVerifyTheFile(fileName, word, resultFile, replacements, wordFound);
                totalReplacements += replacements;

                wordFounD = wordFound;
            } while (FindNextFile(hFind, &findFileData) != 0);
            FindClose(hFind);
        }
        resultFile.close();
    } else {
        MessageBox(NULL, L"Error opening result file 'rez.txt'!", L"Error", MB_OK | MB_ICONERROR);
    }
}

DWORD WINAPI SearchThreadProc(LPVOID lpParam)
{
    HWND hEdit1 = GetDlgItem((HWND)lpParam, IDC_EDIT1);
    if (hEdit1 != NULL) {
        int textLength = GetWindowTextLength(hEdit1);
        if (textLength > 0) {
            wchar_t* buffer = new wchar_t[textLength + 1];
            GetWindowText(hEdit1, buffer, textLength + 1);
            word = buffer;
            delete[] buffer;

            SearchFileFindWords(L".", word);
        }
        if (textLength > 0) {
            srand(time(0));
            countSecond = rand() % 2 + 2;
            SendMessage(hProgress, PBM_SETRANGE, 0, MAKELPARAM(0, countSecond));
            SetTimer((HWND)lpParam, 1, 1000, NULL);
        }
    }
    return 0;
}

void StreamCreation(HWND hWnd)
{
    HANDLE hThread = CreateThread(NULL, 0, SearchThreadProc, hWnd, 0, NULL);
    if (hThread == NULL) {
        MessageBox(NULL, L"Failed to create thread!", L"Error", MB_OK | MB_ICONERROR);
    } else {
        CloseHandle(hThread);
    }
}

int CALLBACK DlgProc(HWND hWnd, UINT mes, WPARAM wp, LPARAM lp)
{
    switch (mes)
    {
    case WM_INITDIALOG:
    {
        srand(time(0));
        hProgress = GetDlgItem(hWnd, IDC_PROGRESS1);
        SendMessage(hProgress, PBM_SETRANGE, 0, MAKELPARAM(0, 2));
        SendMessage(hProgress, PBM_SETSTEP, 1, 0);
        SendMessage(hProgress, PBM_SETPOS, 0, 0);
        SendMessage(hProgress, PBM_SETBKCOLOR, 0, LPARAM(RGB(192, 192, 192)));
        SendMessage(hProgress, PBM_SETBARCOLOR, 0, LPARAM(RGB(160, 32, 240)));
    }
    break;
    case WM_COMMAND:
    {
        switch (LOWORD(wp))
        {
        case IDC_BUTTON1: {
            StreamCreation(hWnd);
        }
        break;
        case IDC_BUTTON2: {
            CleanUpTheResultFile();
            KillTimer(hWnd, 1);
            SendMessage(hProgress, PBM_SETPOS, 0, 0);
            stopProc = true;
        }
        break;
        }
        break;
    }
    case WM_TIMER:
    {
        int nPos = SendMessage(hProgress, PBM_GETPOS, 0, 0);
        if (nPos < countSecond) {
            SendMessage(hProgress, PBM_STEPIT, 0, 0);
        } else {
            KillTimer(hWnd, 1);
            SendMessage(hProgress, PBM_SETBKCOLOR, 0, LPARAM(RGB(192, 192, 192)));
            SendMessage(hProgress, PBM_SETPOS, 0, 0);

            if (stopProc && word.size() > 0) {
                CheckIfThereIsResult(hWnd, wordFounD, word);
            }
        }
        break;
    }
    case WM_CLOSE: {
        EndDialog(hWnd, 0);
        return TRUE;
    }
    }
    return FALSE;
}

int WINAPI _tWinMain(HINSTANCE hInst, HINSTANCE hPrev, LPTSTR lpszCmdLine, int nCmdShow) {
    return DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG1), NULL, (DLGPROC)DlgProc);
}