#include <windows.h>
#include <commctrl.h>
#include "resource.h"

#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 16383

LRESULT CALLBACK WindowProcedure(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK DlgProcAdd(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK DlgProcScheme(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK DlgProcAbout(HWND, UINT, WPARAM, LPARAM);

char szClassName[] = "MSConfig";
char szTitleName[] = "Редактор автозагрузки";

HINSTANCE hInst;
HWND lvList;

int WINAPI WinMain(HINSTANCE hThisInstance, HINSTANCE hPrevInstance,
					LPSTR lpszArgument, int nFunsterStil)
{
	hInst = hThisInstance;

    HWND hwnd;
    MSG messages;
    WNDCLASSEX wincl;

    wincl.hInstance = hThisInstance;
    wincl.lpszClassName = szClassName;
    wincl.lpfnWndProc = WindowProcedure;
    wincl.style = CS_DBLCLKS;
    wincl.cbSize = sizeof(WNDCLASSEX);
    wincl.hIcon = LoadIcon(hThisInstance, MAKEINTRESOURCE(IDI_ICON1));
    wincl.hIconSm = LoadIcon(hThisInstance, MAKEINTRESOURCE(IDI_ICON1));
    wincl.hCursor = LoadCursor(NULL, IDC_ARROW);
	wincl.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);
    wincl.cbClsExtra = 0;
    wincl.cbWndExtra = 0;
    wincl.hbrBackground = (HBRUSH)COLOR_WINDOW;

    if(!RegisterClassEx(&wincl)) return 0;

    hwnd = CreateWindowEx(0, szClassName, szTitleName,
        WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 605, 372, HWND_DESKTOP, NULL, hThisInstance, NULL
    );

    ShowWindow(hwnd, nFunsterStil);

    while(GetMessage(&messages, NULL, 0, 0))
    {
        TranslateMessage(&messages);
        DispatchMessage(&messages);
    }

    return messages.wParam;
}


int iStart = 0;
bool bStats[512][3];	// Массив отслеживания изменений  bStats[][0] - активность; bStats[][1] - изменения; bStats[][2] - ветка реестра, HKCU - true, HKLM - false;
char szRunPath[] = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";
char szNotRunPath[] = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunNot";
bool isEdit = false;


void QueryKey(HKEY hKey, bool isOn, bool bRoot, HWND lvList) 
{ 
	DWORD dwValueCount;
	char szValueName[256];
	DWORD dwType, dwValueSize;
	DWORD dwDataSize;
	char szData[1024];
	LVITEM liTemp;

	if(RegQueryInfoKey(hKey, NULL, NULL, NULL, NULL, NULL, NULL, &dwValueCount, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
	{
		for(int i=0; i<(int)dwValueCount; i++)
		{ 
			if(iStart == 512)
			{
				MessageBox(lvList, "Привышен лимит записей в автозагрузке", szTitleName, MB_OK);
				PostQuitMessage(0);
				break;
			}

			dwValueSize = 256;

			if(RegEnumValue(hKey, i, szValueName, &dwValueSize, NULL, &dwType, NULL, NULL) == ERROR_SUCCESS)
			{
				if((dwType == REG_SZ) || (dwType == REG_EXPAND_SZ))
				{					
					dwDataSize = 1024;
					if(RegQueryValueEx(hKey, szValueName, NULL, NULL, (LPBYTE)szData, &dwDataSize) == ERROR_SUCCESS)
					{
						liTemp.mask = LVIF_TEXT;
						liTemp.iItem = iStart;
						liTemp.iSubItem = 0;
						if(isOn) liTemp.pszText = "On";
						else liTemp.pszText = "Off";
						ListView_InsertItem(lvList, &liTemp);
						liTemp.iSubItem = 1;
						liTemp.pszText = szValueName;
						ListView_SetItem(lvList, &liTemp);
						liTemp.iSubItem = 2;
						if(bRoot) liTemp.pszText = TEXT("HKCU");
						else liTemp.pszText = TEXT("HKLM");
						ListView_SetItem(lvList, &liTemp);
						liTemp.iSubItem = 3;
						liTemp.pszText = szData;
						ListView_SetItem(lvList, &liTemp);

						if(isOn) bStats[iStart][0] = true;
						else bStats[iStart][0] = false;
						bStats[iStart][1] = false;
						bStats[iStart][2] = bRoot;

						iStart++;
					}
				}
			}
		}
    }
}


LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message)
    {
    case WM_CREATE:
		{
			HWND cmdRefresh = CreateWindow("button", "Обновить", WS_CHILD|WS_VISIBLE|BS_ICON, 328, 281, 32, 32, hwnd, (HMENU)ID_FILE_REFRESH, NULL, NULL);
			SendMessage(cmdRefresh, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1))); // для иконки
			CreateWindow("button", "Применить", WS_CHILD|WS_VISIBLE, 370, 285, 100, 24, hwnd, (HMENU)ID_FILE_APPLY, NULL, NULL);
			CreateWindow("button", "Закрыть", WS_CHILD|WS_VISIBLE, 480, 285, 100, 24, hwnd, (HMENU)ID_FILE_CLOSE40002, NULL, NULL);
			CreateWindow("button", "Включить все", WS_CHILD|WS_VISIBLE, 5, 285, 110, 24, hwnd, (HMENU)ID_FILE_ALLON, NULL, NULL);
			CreateWindow("button", "Выключить все", WS_CHILD|WS_VISIBLE, 125, 285, 110, 24, hwnd, (HMENU)ID_FILE_ALLOFF, NULL, NULL);
			
			lvList = CreateWindow(WC_LISTVIEW, NULL, WS_VISIBLE|WS_TABSTOP|WS_CHILD|WS_DLGFRAME|LVS_REPORT|WS_HSCROLL,
				5, 0, 575, 272, hwnd, NULL, NULL, NULL);

			LVCOLUMN cTemp;
			cTemp.mask = LVCF_TEXT|LVCF_WIDTH;							// Тип маски
			cTemp.pszText = "Сост.";									// Текст заголовка
			cTemp.cx = 30;												// Ширина столбца
			SendMessage(lvList, LVM_INSERTCOLUMN, 0, (LPARAM)&cTemp);	// Добавить и отобразить запись	
			cTemp.pszText = "Название";
			cTemp.cx = 160;											
			SendMessage(lvList, LVM_INSERTCOLUMN, 1, (LPARAM)&cTemp);	
			cTemp.pszText = "Расположение";
			cTemp.cx = 50; 
			SendMessage(lvList, LVM_INSERTCOLUMN, 2, (LPARAM)&cTemp);
			cTemp.pszText = "Значение";
			cTemp.cx = 380; 
			SendMessage(lvList, LVM_INSERTCOLUMN, 3, (LPARAM)&cTemp);

			SendMessage(lvList, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);		// Опция ListView, для выделения всех столбцов

			SendMessage(hwnd, WM_COMMAND, ID_FILE_REFRESH, 0);

			break;
		}
	case WM_COMMAND:
        switch(LOWORD(wParam))
        {
		case ID_FILE_REFRESH:
			{
				HKEY hKey;
				iStart = 0;
				
				ListView_DeleteAllItems(lvList);

				if(RegOpenKeyEx(HKEY_CURRENT_USER, szRunPath, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
				{
					QueryKey(hKey, true, true, lvList);
					RegCloseKey(hKey);
				}

				if(RegOpenKeyEx(HKEY_CURRENT_USER, szNotRunPath, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
				{
					QueryKey(hKey, false, true, lvList);
					RegCloseKey(hKey);
				}

				if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, szRunPath, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
				{
					QueryKey(hKey, true, false, lvList);
					RegCloseKey(hKey);
				}

				if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, szNotRunPath, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
				{
					QueryKey(hKey, false, false, lvList);
					RegCloseKey(hKey);
				}

				break;
			}
		case ID_FILE_APPLY:
			{
				char szValue[256];
				HKEY hKeyRoot, hKeyFrom, hKeyTo;
				char szData[1024];
				DWORD dwDataSize;

				for(int i=0; i<=iStart; i++)
				{
					if(bStats[i][1])
					{
						ListView_GetItemText(lvList, i, 1, szValue, 256);

						if(bStats[i][2]) hKeyRoot = HKEY_CURRENT_USER;
						else hKeyRoot = HKEY_LOCAL_MACHINE;
						
						if(bStats[i][0])
						{
							RegOpenKeyEx(hKeyRoot, szRunPath, 0, KEY_ALL_ACCESS, &hKeyFrom);
							RegCreateKey(hKeyRoot, szNotRunPath, &hKeyTo);
						}
						else
						{
							RegCreateKey(hKeyRoot, szNotRunPath, &hKeyFrom);
							RegOpenKeyEx(hKeyRoot, szRunPath, 0, KEY_ALL_ACCESS, &hKeyTo);
						}

						dwDataSize = 1024;
						RegQueryValueEx(hKeyFrom, szValue, NULL, NULL, (LPBYTE)szData, &dwDataSize);
						if(RegSetValueEx(hKeyTo, szValue, NULL, REG_SZ, (LPBYTE)szData, dwDataSize) == ERROR_SUCCESS) RegDeleteValue(hKeyFrom, szValue);

						RegCloseKey(hKeyFrom);
						RegCloseKey(hKeyTo);
						
						bStats[i][0] = !bStats[i][0];
						bStats[i][1] = false;
					}
				}
				if(lParam != 34) MessageBox(hwnd, "Изменения сохранены", szTitleName, NULL);
					
				break;
			}
		case ID_FILE_ALLON:
			{
				LVITEM liTemp;
				liTemp.mask = LVIF_TEXT;
				liTemp.iSubItem = 0;
				liTemp.pszText = "On";
				for(int i=0; i<=iStart; i++)
				{
					liTemp.iItem = i;
					ListView_SetItem(lvList, &liTemp);
					if(!bStats[i][0]) bStats[i][1] = true;
					else bStats[i][1] = false;
				}
				break;
			}
		case ID_FILE_ALLOFF:
			{
				LVITEM liTemp;
				liTemp.mask = LVIF_TEXT;
				liTemp.iSubItem = 0;
				liTemp.pszText = "Off";
				for(int i=0; i<=iStart; i++)
				{
					liTemp.iItem = i;
					ListView_SetItem(lvList, &liTemp);
					if(bStats[i][0]) bStats[i][1] = true;
					else bStats[i][1] = false;
				}
				break;
			}	
		case ID_FILE_CLOSE40002:
			{
				bool bK = false;
				for(int i=0; i<=iStart; i++)
				{
					if(bStats[i][1])
					{
						bK = true;
						break;
					}
				}
				if(bK)
				{
					int iSel = MessageBox(hwnd, "Изменения не были применены. Хотите сохранить изменения перед выходом?", szTitleName, MB_YESNOCANCEL);
					if(iSel == IDYES) SendMessage(hwnd, WM_COMMAND, ID_FILE_APPLY, 34);
					else if(iSel == IDCANCEL) break;
				}
				PostQuitMessage(0);
				break;
			}
		case ID_EDIT_TOGGLE:
			{
				int iSel = ListView_GetSelectionMark(lvList);
				if(iSel != -1)
				{
					char buff[8];
					ListView_GetItemText(lvList, iSel, 0, buff, 8);
					LVITEM liTemp;
					memset(&liTemp, 0, sizeof(LVITEM));
					liTemp.mask = LVIF_TEXT;
					liTemp.iItem = iSel;
					liTemp.iSubItem = 0;
					if(strcmp(buff, "On") == 0) liTemp.pszText = "Off";
					else liTemp.pszText = "On";
					ListView_SetItem(lvList, &liTemp);

					bStats[iSel][1] = !bStats[iSel][1];
				}
				else MessageBox(hwnd, "Запись не выбрана", szTitleName, NULL);
				break;
			}
		case ID_EDIT_EDIT:
			{
				if(ListView_GetSelectionMark(lvList) == -1)
				{
					MessageBox(hwnd, "Запись не выбрана", szTitleName, NULL);
					break;
				}
				isEdit = true;
				DialogBox(hInst, MAKEINTRESOURCE(IDD_ADD), hwnd, DlgProcAdd);	
				break;
			}
		case ID_EDIT_DELETE:
			{
				int iSel = ListView_GetSelectionMark(lvList);
				if(iSel == -1)
				{
					MessageBox(hwnd, "Запись не выбрана", szTitleName, NULL);
					break;
				}
				char szValue[256];
				ListView_GetItemText(lvList, iSel, 1, szValue, 256);
				char buff[512];
				strcpy_s(buff, TEXT("Вы действительно хотите удалить запись \""));
				strcat_s(buff, szValue);
				strcat_s(buff, "\"?");
				if(MessageBox(hwnd, buff, szTitleName, MB_YESNO) == IDYES)
				{
					HKEY hKeyRoot, hKey;
					if(bStats[iSel][2]) hKeyRoot = HKEY_CURRENT_USER;
					else hKeyRoot = HKEY_LOCAL_MACHINE;
					
					if(bStats[iSel][0]) RegOpenKeyEx(hKeyRoot, szRunPath, 0, KEY_ALL_ACCESS, &hKey);
					else RegOpenKeyEx(hKeyRoot, szNotRunPath, 0, KEY_ALL_ACCESS, &hKey);

					if(RegDeleteValue(hKey, szValue) != ERROR_SUCCESS)
					{
						MessageBox(hwnd, "Не удалось удалить запись", szTitleName, NULL);
						break;
					}

					RegCloseKey(hKey);

					SendMessage(hwnd, WM_COMMAND, ID_FILE_REFRESH, 0);
				}
				break;
			}
		case ID_EDIT_ADD:
			{
				DialogBox(hInst, MAKEINTRESOURCE(IDD_ADD), hwnd, DlgProcAdd);

				break;
			}
		
		case ID_SCHEME_EDIT:
			{
				DialogBox(hInst, MAKEINTRESOURCE(IDD_SCHEME), hwnd, DlgProcScheme);

				break;
			}
		case ID_HELP_ABOUT:
			{
				DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUT), hwnd, DlgProcAbout);
				break;
			}
        }
        break;
	case WM_NOTIFY:
		{
			if((((LPNMHDR)lParam)->code == NM_RCLICK) && (((LPNMHDR)lParam)->hwndFrom == lvList))
			{
				POINT p;
				GetCursorPos(&p);
				static HMENU hPopupMenu = GetSubMenu(GetMenu(hwnd), 1);
				int iSel = -1;
				iSel = TrackPopupMenu(hPopupMenu, TPM_LEFTBUTTON|TPM_RETURNCMD, p.x, p.y, 0, lvList, NULL);
				
				SendMessage(hwnd, WM_COMMAND, iSel, 0);
			}
			break;
		}
    case WM_DESTROY:
		{
			PostQuitMessage(0);
			break;
		}
    default:
        return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0;
}


BOOL CALLBACK DlgProcAdd(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HWND editName, editPath, radioCU, radioLM, checkActive;
	static OPENFILENAME ofn;
	static int iSel;
	static bool _bCU = false, _bActive = false;
    switch(message)
    {
    case WM_INITDIALOG:
		{
			editName = GetDlgItem(hDlg, IDC_NAME);
			editPath = GetDlgItem(hDlg, IDC_PATH);
			radioCU = GetDlgItem(hDlg, IDC_CU);
			radioLM = GetDlgItem(hDlg, IDC_LM);
			checkActive = GetDlgItem(hDlg, IDC_ACTIVE);
			
			// OPENFILE STRUCTURE
			ofn.lStructSize = sizeof(OPENFILENAME);
			ofn.hwndOwner = hDlg;
			ofn.hInstance = NULL;
			ofn.lpstrFilter = "Executable Files (*.EXE)\0*.exe\0All Files (*.*)\0*.*";
			ofn.lpstrCustomFilter = NULL;
			ofn.nMaxCustFilter = 0;
			ofn.nFilterIndex = 0;
			ofn.lpstrFile = NULL;
			ofn.nMaxFile = _MAX_PATH;
			ofn.lpstrFileTitle = NULL;
			ofn.nMaxFileTitle = _MAX_FNAME+_MAX_EXT;
			ofn.lpstrInitialDir = NULL;
			ofn.lpstrTitle = NULL;
			ofn.Flags = 0;
			ofn.nFileOffset = 0;
			ofn.nFileExtension = 0;
			ofn.lpstrDefExt = NULL;
			ofn.lCustData = 0L;
			ofn.lpfnHook = NULL;
			ofn.lpTemplateName = NULL;
			// Flag for open
			ofn.Flags = OFN_HIDEREADONLY|OFN_CREATEPROMPT;
			
			if(isEdit)
			{
				SetWindowText(hDlg, TEXT("Изменить запись"));
				iSel = ListView_GetSelectionMark(lvList);
				if(bStats[iSel][0])
				{
					SendMessage(checkActive, BM_SETCHECK, 1, 0);
					_bActive = true;
				}
				if(bStats[iSel][2])
				{
					SendMessage(radioCU, BM_SETCHECK, 1, 0);
					_bCU = true;
				}
				else SendMessage(radioLM, BM_SETCHECK, 1, 0);
				char szValue[256];
				ListView_GetItemText(lvList, iSel, 1, szValue, 256);
				SetWindowText(editName, szValue);
				char szData[1024];
				ListView_GetItemText(lvList, iSel, 3, szData, 1024);
				SetWindowText(editPath, szData);
			}
			else
			{
				SendMessage(radioCU, BM_SETCHECK, 1, 0);
				SendMessage(checkActive, BM_SETCHECK, 1, 0);
			}
			break;
		}
	case WM_COMMAND:
		{
			switch(LOWORD(wParam))
            {
            case IDOK:
                {
					char szName[256], szPath[1024];
					GetWindowText(editName, szName, sizeof(char)*256);
					GetWindowText(editPath, szPath, sizeof(char)*1024);
					
					if(strlen(szName) && strlen(szPath))
					{
						DWORD bCU, bActive;
						bCU = SendMessage(radioCU, BM_GETSTATE, 0, 0);
						bActive = SendMessage(checkActive, BM_GETCHECK, 0, 0);

						HKEY hKeyRoot, hKey;

						if(bCU) hKeyRoot = HKEY_CURRENT_USER;
						else hKeyRoot = HKEY_LOCAL_MACHINE;
						
						if(bActive) RegCreateKey(hKeyRoot, szRunPath, &hKey);
						else RegCreateKey(hKeyRoot, szNotRunPath, &hKey);
						if(!isEdit)
						{
							if(RegQueryValueEx(hKey, szName, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
							{
								RegSetValueEx(hKey, szName, NULL, REG_SZ, (LPBYTE)&szPath, sizeof(char)*strlen(szPath));
								RegCloseKey(hKey);
								SendMessage(GetParent(hDlg), WM_COMMAND, ID_FILE_REFRESH, 0);
								EndDialog(hDlg, 0);
								MessageBox(GetParent(hDlg), "Запись успешно добавлена!", szTitleName, NULL);
								
								return 0;
							}
							else MessageBox(hDlg, "Запись с таким именем уже есть в данном разделе. Попробуйте изменить имя записи.", szTitleName, NULL); 
						}
						else
						{
							// Удаление старой записи
							char szValue[256];
							ListView_GetItemText(lvList, iSel, 1, szValue, 256);

							HKEY hKeyRootFrom, hKeyFrom;

							if(_bCU) hKeyRootFrom = HKEY_CURRENT_USER;
							else hKeyRootFrom = HKEY_LOCAL_MACHINE;
							
							if(_bActive) RegCreateKey(hKeyRootFrom, szRunPath, &hKeyFrom);
							else RegCreateKey(hKeyRootFrom, szNotRunPath, &hKeyFrom);

							RegDeleteValue(hKeyFrom, szValue);
							RegCloseKey(hKeyFrom);

							// Добавление новой
							RegSetValueEx(hKey, szName, NULL, REG_SZ, (LPBYTE)&szPath, sizeof(char)*strlen(szPath));
							RegCloseKey(hKey);
							SendMessage(GetParent(hDlg), WM_COMMAND, ID_FILE_REFRESH, 0);
							EndDialog(hDlg, 0);
							MessageBox(GetParent(hDlg), "Запись успешно Изменена!", szTitleName, NULL);
							
							return 0;
						}
					}
					else MessageBox(hDlg, "Необходимо заполнить поля", "szTitleName", NULL);
                    break;
                }
            case IDCANCEL:
				{
                    EndDialog(hDlg, 0);
                    return 0;
				}
			case IDC_BROWSE:
				{
					static char szFileName[_MAX_PATH];
					static char szTitleFileName[_MAX_FNAME + _MAX_EXT];
					char szBuff[_MAX_FNAME + _MAX_EXT];
					GetWindowText(editPath, szBuff, _MAX_FNAME + _MAX_EXT);
					if(szBuff[0] != '"') strcpy_s(szFileName, szBuff);

					ofn.lpstrFile = szFileName;
					ofn.lpstrFileTitle = szTitleFileName;

					if(GetOpenFileName(&ofn))
					{
						SetWindowText(editPath, szFileName);
						char czName[1024];
						GetWindowText(editName, czName, sizeof(char)*1024);
						if(!strlen(czName)) SetWindowText(editName, szTitleFileName);
					}
					break;
				}
            }
			break;
		}
		break;
    }
    return 0;
}


BOOL CALLBACK DlgProcScheme(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HWND comboList, editName;
	char szSchemePath[] = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunNotScheme";
    switch(message)
    {
    case WM_INITDIALOG:
		{
			comboList = GetDlgItem(hDlg, IDC_LIST);
			editName = GetDlgItem(hDlg, IDC_SCNAME);

			SendMessage(comboList, CB_RESETCONTENT, 0, 0);

			HKEY hKey;
			DWORD dwKeyCount;
			CHAR szKeyName[256];

			if(RegOpenKeyEx(HKEY_CURRENT_USER, szSchemePath, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
			{
				if(RegQueryInfoKey(hKey, NULL, NULL, NULL, &dwKeyCount, NULL, NULL, NULL, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
				{
					for(int i=0; i<(int)dwKeyCount; i++)
					{
						if(RegEnumKey(hKey, i, szKeyName, 256) == ERROR_SUCCESS)
						{
							SendMessage(comboList, CB_ADDSTRING, 0, (LPARAM)szKeyName);
						}
					}
				}
				RegCloseKey(hKey);
			}
						
			break;
		}
	case WM_COMMAND:
		{
			switch(LOWORD(wParam))
            {
            case IDC_APPLY:
                {
					char szName[256];
					GetWindowText(comboList, szName, 256);
					HKEY hKey, hKeyScheme;

					if(strlen(szName))
					{
						if(RegOpenKeyEx(HKEY_CURRENT_USER, szSchemePath, 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
						{
							if(RegOpenKeyEx(hKey, szName, 0, KEY_ALL_ACCESS, &hKeyScheme) == ERROR_SUCCESS)
							{
								HKEY hKeyCUR, hKeyCUN, hKeyLMR, hKeyLMN;		// CUR - Current User Run, CUN - Current User Not Run
								HKEY hKeyCURF, hKeyCUNF, hKeyLMRF, hKeyLMNF;	// CURF - Current User Run (From)
								HKEY hKeyCU, hKeyLM;

								RegCreateKey(hKeyScheme, TEXT("CUR"), &hKeyCURF);
								RegCreateKey(hKeyScheme, TEXT("CUN"), &hKeyCUNF);
								RegCreateKey(hKeyScheme, TEXT("LMR"), &hKeyLMRF);
								RegCreateKey(hKeyScheme, TEXT("LMN"), &hKeyLMNF);
								
								RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion"), 0, KEY_READ, &hKeyCU);
								RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion"), 0, KEY_READ, &hKeyLM);
								
								RegDeleteKey(hKeyCU, TEXT("Run"));
								RegDeleteKey(hKeyCU, TEXT("RunNot"));
								RegDeleteKey(hKeyLM, TEXT("Run"));
								RegDeleteKey(hKeyLM, TEXT("RunNot"));

								RegCloseKey(hKeyCU);
								RegCloseKey(hKeyLM);

								RegCreateKey(HKEY_CURRENT_USER, szRunPath, &hKeyCUR);
								RegCreateKey(HKEY_CURRENT_USER, szNotRunPath, &hKeyCUN);
								RegCreateKey(HKEY_LOCAL_MACHINE, szRunPath, &hKeyLMR);
								RegCreateKey(HKEY_LOCAL_MACHINE, szNotRunPath, &hKeyLMN);

								RegCopyTree(hKeyCURF, NULL, hKeyCUR);
								RegCopyTree(hKeyCUNF, NULL, hKeyCUN);
								RegCopyTree(hKeyLMRF, NULL, hKeyLMR);
								RegCopyTree(hKeyLMNF, NULL, hKeyLMN);

								RegCloseKey(hKeyCUR);
								RegCloseKey(hKeyCUN);
								RegCloseKey(hKeyLMR);
								RegCloseKey(hKeyLMN);
								RegCloseKey(hKeyCURF);
								RegCloseKey(hKeyCUNF);
								RegCloseKey(hKeyLMRF);
								RegCloseKey(hKeyLMNF);
								RegCloseKey(hKeyScheme);

								MessageBox(hDlg, "Схема применена", szTitleName, NULL);
							}
							RegCloseKey(hKey);
						}
					}
					else MessageBox(hDlg, "Выберите схему", szTitleName, NULL);
                    break;
                }
			case IDC_DELETE:
				{
					char szName[256];
					GetWindowText(comboList, szName, 256);
					HKEY hKey;
					if(strlen(szName))
					{
						if(RegOpenKeyEx(HKEY_CURRENT_USER, szSchemePath, 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
						{
							char buff[256];
							strcpy_s(buff, szName);
							strcat_s(buff, TEXT("\\CUR"));
							RegDeleteKey(hKey, buff);
							strcpy_s(buff, szName);
							strcat_s(buff, TEXT("\\CUN"));
							RegDeleteKey(hKey, buff);
							strcpy_s(buff, szName);
							strcat_s(buff, TEXT("\\LMR"));
							RegDeleteKey(hKey, buff);
							strcpy_s(buff, szName);
							strcat_s(buff, TEXT("\\LMN"));
							RegDeleteKey(hKey, buff);

							RegDeleteKey(hKey, szName);
							RegCloseKey(hKey);

							SendMessage(hDlg, WM_INITDIALOG, 0, 0); 
							MessageBox(hDlg, "Схема успешно удалена", szTitleName, NULL);
						}
					}
					else MessageBox(hDlg, "Выберите схему", szTitleName, NULL);
					break;
				}
			case IDC_SAVE:
				{
					char szName[256];
					GetWindowText(editName, szName, 256);
					HKEY hKey, hKeyN;
					if(strlen(szName))
					{
						if(RegCreateKey(HKEY_CURRENT_USER, szSchemePath, &hKey) == ERROR_SUCCESS)
						{
							if(RegOpenKeyEx(hKey, szName, 0, KEY_ALL_ACCESS, &hKeyN) == ERROR_SUCCESS)
							{
								MessageBox(hDlg, "Схема с таким именем уже существует", szTitleName, NULL);
								break;
							}
							else
							{
								HKEY hKeyCUR, hKeyCUN, hKeyLMR, hKeyLMN;		// CUR - Current User Run, CUN - Current User Not Run
								HKEY hKeyCURF, hKeyCUNF, hKeyLMRF, hKeyLMNF;	// CURF - Current User Run (From)
								HKEY hKeyScheme;

								RegCreateKey(hKey, szName, &hKeyScheme);

								RegCreateKey(hKeyScheme, TEXT("CUR"), &hKeyCUR);
								RegCreateKey(hKeyScheme, TEXT("CUN"), &hKeyCUN);
								RegCreateKey(hKeyScheme, TEXT("LMR"), &hKeyLMR);
								RegCreateKey(hKeyScheme, TEXT("LMN"), &hKeyLMN);
								
								RegOpenKeyEx(HKEY_CURRENT_USER, szRunPath, 0, KEY_READ, &hKeyCURF);
								RegOpenKeyEx(HKEY_CURRENT_USER, szNotRunPath, 0, KEY_READ, &hKeyCUNF);
								RegOpenKeyEx(HKEY_LOCAL_MACHINE, szRunPath, 0, KEY_READ, &hKeyLMRF);
								RegOpenKeyEx(HKEY_LOCAL_MACHINE, szNotRunPath, 0, KEY_READ, &hKeyLMNF);

								RegCopyTree(hKeyCURF, NULL, hKeyCUR);
								RegCopyTree(hKeyCUNF, NULL, hKeyCUN);
								RegCopyTree(hKeyLMRF, NULL, hKeyLMR);
								RegCopyTree(hKeyLMNF, NULL, hKeyLMN);

								RegCloseKey(hKeyCUR);
								RegCloseKey(hKeyCUN);
								RegCloseKey(hKeyLMR);
								RegCloseKey(hKeyLMN);
								RegCloseKey(hKeyCURF);
								RegCloseKey(hKeyCUNF);
								RegCloseKey(hKeyLMRF);
								RegCloseKey(hKeyLMNF);
								RegCloseKey(hKeyScheme);

								SendMessage(hDlg, WM_INITDIALOG, 0, 0);
							}
							RegCloseKey(hKey);
						}
					}
					else MessageBox(hDlg, "Введите название схемы", szTitleName, NULL);
					break;
				}
			case IDOK:
				{
                    EndDialog(hDlg, 0);
                    return 0;
				}
            case IDCANCEL:
				{
                    EndDialog(hDlg, 0);
                    return 0;
				}
            }
			break;
		}
		break;
    }
    return 0;
}

BOOL CALLBACK DlgProcAbout(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message)
    {
	case WM_COMMAND:
		{
			switch(LOWORD(wParam))
            {
			case IDOK:
				{
                    EndDialog(hDlg, 0);
                    return 0;
				}
			case IDCANCEL:
				{
                    EndDialog(hDlg, 0);
                    return 0;
				}
            }
			break;
		}
		break;
    }
    return 0;
}
