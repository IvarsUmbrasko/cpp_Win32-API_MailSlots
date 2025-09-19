#undef UNICODE
#include <windows.h>
#include <stdio.h>
#include "resource.h"
#define WM_MAILSLOT_MESSAGE WM_USER

#pragma warning(disable : 4996)

bool terminated = false;
bool robotEnable = false;
HANDLE listenerT;
HANDLE mailSlot;
HANDLE botMailSlot;
HANDLE robotT;

BOOL CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_MAINDIALOG), NULL, DLGPROC(MainWndProc));
	return 0;
}

DWORD WINAPI ListenerThread(LPVOID lpParameter) {
	HWND hMain = (HWND)lpParameter;
	DWORD messageSize, bytesRead;
	char buf[512];

	mailSlot = CreateMailslot("\\\\.\\mailslot\\Shouter", 0, MAILSLOT_WAIT_FOREVER, NULL);
	botMailSlot = CreateMailslot("\\\\.\\mailslot\\bot", 0, MAILSLOT_WAIT_FOREVER, NULL);

	while (!terminated) {
		if (!robotEnable) {
			GetMailslotInfo(mailSlot, NULL, &messageSize, NULL, NULL);
			if (messageSize != MAILSLOT_NO_MESSAGE) {
				if (ReadFile(mailSlot, &buf, messageSize, &bytesRead, NULL)) {
					buf[bytesRead] = '\0';
					PostMessage(hMain, WM_MAILSLOT_MESSAGE, (WPARAM)buf, 0);
				}
			}
		}
		else {
			GetMailslotInfo(botMailSlot, NULL, &messageSize, NULL, NULL);
			if (messageSize != MAILSLOT_NO_MESSAGE) {
				if (ReadFile(botMailSlot, &buf, messageSize, &bytesRead, NULL)) {
					buf[bytesRead] = '\0';
					PostMessage(hMain, WM_MAILSLOT_MESSAGE, (WPARAM)buf, 0);
					char* userMessage = strstr(buf, "to bot: ");
					if (robotEnable && userMessage) {
						userMessage += 8;
						char response[512] = { 0 };
						if (strcmp(userMessage, "Hello") == 0 || strcmp(userMessage, "Hi") == 0) {
							strcpy(response, "Robot: Hello! How can I assist you?");
						}
						else if (strcmp(userMessage, "!help") == 0) {
							strcpy(response, "1. Hello/Hi\r\n2. What's your name?\r\n3. How are you?\r\n4. Goodbye");
						}
						else if (strcmp(userMessage, "What's your name?") == 0) {
							strcpy(response, "Robot: I am just robot i don't have name.");
						}
						else if (strcmp(userMessage, "How are you?") == 0) {
							strcpy(response, "Robot: I'm just a robot, but I'm functioning well!");
						}
						else if (strcmp(userMessage, "Goodbye") == 0) {
							strcpy(response, "Robot: Goodbye! Have a great day!");
						}
						else {
							strcpy(response, "Robot: I'm sorry, I don't understand your message.");
						}
						PostMessage(hMain, WM_MAILSLOT_MESSAGE, (WPARAM)response, 0);
					}
				}
			}
		}
		Sleep(100);
	}
	return 0;
}

void shoutMessage(HWND hWnd) {
	char compName[16];
	DWORD computerNameLen = 16;
	if (GetComputerName(compName, &computerNameLen))
	{
		char userName[256];
		GetWindowText(GetDlgItem(hWnd, IDC_MYNAME), userName, 256);

		char messageText[256];
		GetWindowText(GetDlgItem(hWnd, IDC_MYSCREAM), messageText, 256);

		char messageBuf[1024];
		if (robotEnable) {
			sprintf(messageBuf, "%s from %s to bot: %s", userName, compName, messageText);
			DWORD bytesRead;
			HANDLE slotFile = CreateFile("\\\\*\\mailslot\\bot", GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			WriteFile(slotFile, messageBuf, strlen(messageBuf), &bytesRead, NULL);
			SetDlgItemText(hWnd, IDC_MYSCREAM, "");
			CloseHandle(slotFile);
		}
		else {
			sprintf(messageBuf, "%s from %s shout: %s", userName, compName, messageText);
			DWORD bytesRead;
			HANDLE slotFile = CreateFile("\\\\*\\mailslot\\Shouter", GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			WriteFile(slotFile, messageBuf, strlen(messageBuf), &bytesRead, NULL);
			SetDlgItemText(hWnd, IDC_MYSCREAM, "");
			CloseHandle(slotFile);
		}
	}
}

BOOL CALLBACK MainWndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
	switch (Msg) {
	case WM_INITDIALOG:
		listenerT = CreateThread(NULL, 0, ListenerThread, hWnd, 0, NULL);
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_ROBOT:
			robotEnable = (IsDlgButtonChecked(hWnd, IDC_ROBOT) == BST_CHECKED);
			return TRUE;
		case IDC_SHOUT:
			shoutMessage(hWnd);
			return TRUE;
		case IDC_EXIT:
			DestroyWindow(hWnd);
			return TRUE;
		}
		return FALSE;

	case WM_MAILSLOT_MESSAGE:
	{
		char* buf = (char*)wParam;
		SendDlgItemMessage(hWnd, IDC_HUBBUB, EM_REPLACESEL, FALSE, (LPARAM)buf);
		SendDlgItemMessage(hWnd, IDC_HUBBUB, EM_REPLACESEL, FALSE, (LPARAM)"\r\n");
	}
	return TRUE;
	case WM_DESTROY:
		terminated = true;
		WaitForSingleObject(listenerT, INFINITE);
		CloseHandle(listenerT);
		PostQuitMessage(0);
		return TRUE;
	}
	return FALSE;
}