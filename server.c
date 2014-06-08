
#include <tchar.h>
#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#include <windows.h>


#include <aclapi.h>
#include <strsafe.h>

#define NCLIENTES 30
#define PIPE_NAME TEXT("\\\\.\\pipe\\comunicacao")
#define PIPE_NAME2 TEXT("\\\\.\\pipe\\difusao")

//
HANDLE hPipesDifusao[NCLIENTES]={NULL};
int total=0; //total de utilizadores ja registados

//prototipos
void Cleanup(PSID pEveryoneSID, PSID pAdminSID, PACL pACL, PSECURITY_DESCRIPTOR pSD);
void ErrorExit(LPTSTR lpszFunction);


DWORD WINAPI AtendeCliente(LPVOID param){
	BOOL ret = FALSE;
	TCHAR buf[256];
	DWORD n;
	int i;
	int resposta;
	HANDLE hPipe = (HANDLE) param;

	_tprintf(TEXT("[SERVIDOR-%d] Um cliente ligou-se...\n"),GetCurrentThreadId());
	while (1) {
		ret = ReadFile(hPipe, buf, sizeof(buf), &n, NULL);
		buf[n / sizeof(TCHAR)*2] = '\0';
		if (!ret || !n)
			break;
		_tprintf(TEXT("[SERVIDOR-%d] Recebi %d bytes: '%s'... (ReadFile)\n"), GetCurrentThreadId(), n, buf);
		resposta=_tcslen(buf)*sizeof(TCHAR);
		WriteFile(hPipe,&resposta,sizeof(int),&n,NULL);

		//divulgar informação a todos 
		for(i=0;i<total;i++)
		{
			if(hPipesDifusao[i]!=NULL)
				WriteFile(hPipesDifusao[i],buf,resposta,&n,NULL);
		}

	}
	_tprintf(TEXT("[SERVIDOR-%d] Vou desligar o pipe... (DisconnectNamedPipe/CloseHandle)\n"), GetCurrentThreadId());
	if (!DisconnectNamedPipe(hPipe)){
		_tperror(TEXT("Erro ao desligar o pipe!"));
		ExitThread(-1);
	}

	CloseHandle(hPipe);
	return 0;
}

void main(void) {
    
	TCHAR buf[256];
    HANDLE hPipe;
	HANDLE threads[NCLIENTES];
	int i;

	//variaveis usadas para segurança nos pipes
	SECURITY_ATTRIBUTES sa;
	PSECURITY_DESCRIPTOR pSD;
	PACL pAcl;
	EXPLICIT_ACCESS ea;
	PSID pEveryoneSID = NULL, pAdminSID = NULL;
	SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
	TCHAR str[256];
	//=======================================


	//UNICODE: By default, windows console does not process wide characters. 
	//Probably the simplest way to enable that functionality is to call _setmode:
#ifdef UNICODE 
	_setmode(_fileno(stdin), _O_WTEXT); 
	_setmode(_fileno(stdout), _O_WTEXT); 
#endif




	pSD = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR,SECURITY_DESCRIPTOR_MIN_LENGTH);
	if (pSD == NULL) {
		ErrorExit(TEXT("Erro LocalAlloc!!!\n"));
		return;
	}
	if (!InitializeSecurityDescriptor(pSD,SECURITY_DESCRIPTOR_REVISION)) {
		ErrorExit(TEXT("Erro IniSec!!!\n"));
		return;
	}

	// Create a well-known SID for the Everyone group.
	if(!AllocateAndInitializeSid(&SIDAuthWorld, 1, SECURITY_WORLD_RID,0, 0, 0, 0, 0, 0, 0, &pEveryoneSID)) 
	{
		_stprintf_s(str, 256,TEXT("AllocateAndInitializeSid() error %u\n"),GetLastError());
		ErrorExit(str);
		Cleanup(pEveryoneSID, pAdminSID, NULL, pSD);
	}
	else
		_tprintf(TEXT("AllocateAndInitializeSid() for the Everyone group is OK\n"));


	ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));

	ea.grfAccessPermissions = GENERIC_WRITE|GENERIC_READ;
	ea.grfAccessMode = SET_ACCESS;
	ea.grfInheritance= SUB_CONTAINERS_AND_OBJECTS_INHERIT;
	ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
	ea.Trustee.ptstrName = (LPTSTR) pEveryoneSID;

	if (SetEntriesInAcl(1,&ea,NULL,&pAcl)!=ERROR_SUCCESS) {
		ErrorExit(TEXT("Erro SetAcl!!!\n"));
		return;
	}

	if (!SetSecurityDescriptorDacl(pSD,TRUE,pAcl,FALSE)) {
		ErrorExit(TEXT("Erro IniSec!!!\n"));
		return;
	}

	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = pSD;
	sa.bInheritHandle = TRUE;

	//ciclo de criaçao de pipes e atendimento de clientes
	while (1){
		_tprintf(TEXT("[SERVIDOR] Vou passar à criação de uma cópia do pipe '%s'... (CreateNamedPipe)\n"), PIPE_NAME);
		hPipe = CreateNamedPipe(PIPE_NAME, PIPE_ACCESS_DUPLEX, PIPE_WAIT | PIPE_TYPE_MESSAGE
								| PIPE_READMODE_MESSAGE, NCLIENTES, sizeof(buf), sizeof(buf), 1000,&sa);
		if(hPipe == INVALID_HANDLE_VALUE){
			_tperror(TEXT("Limite de clientes atingido!"));
			break;
		}


	
		_tprintf(TEXT("[SERVIDOR] Esperar ligação de um cliente... (ConnectNamedPipe)\n"));
		if(!ConnectNamedPipe(hPipe, NULL)){
			_tperror(TEXT("Erro na ligação ao cliente!"));
			exit(-1);
		}

		//2ndo named pipe
		_tprintf(TEXT("[SERVIDOR] Vou passar à criação de uma cópia do pipe '%s'... (CreateNamedPipe)\n"), PIPE_NAME2);
		hPipesDifusao[total] = CreateNamedPipe(PIPE_NAME2, PIPE_ACCESS_OUTBOUND, PIPE_WAIT | PIPE_TYPE_MESSAGE
			| PIPE_READMODE_MESSAGE, NCLIENTES, sizeof(buf), sizeof(buf), 1000, &sa);
		if(hPipesDifusao[total] == INVALID_HANDLE_VALUE){
			_tperror(TEXT("Limite de clientes atingido!"));
			break;
		}
	

	
		_tprintf(TEXT("[SERVIDOR] Esperar ligação de um cliente... (ConnectNamedPipe)\n"));
		if(!ConnectNamedPipe(hPipesDifusao[total], NULL)){
			_tperror(TEXT("Erro na ligação ao cliente!"));
			exit(-1);
		}



		//Lançar thread
		threads[total++]=CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)AtendeCliente, (LPVOID)hPipe, 0, NULL);
		
		Sleep(200);
	}
	//Fora do ciclo de criação das threads
	//Esperar threads terminarem
	for (i = 0; i < total; i++)
		WaitForSingleObject(threads[total], INFINITE);
	
}

// Retrieve the system error message for the last-error code
void ErrorExit(LPTSTR lpszFunction) 
{ 

    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    DWORD dw = GetLastError(); 


    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

    // Display the error message and exit the process

    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, 
        (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR)); 
    StringCchPrintf((LPTSTR)lpDisplayBuf, 
        LocalSize(lpDisplayBuf) / sizeof(TCHAR),
        TEXT("%s failed with error %d: %s"), 
        lpszFunction, dw, lpMsgBuf); 
    MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK); 

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
    ExitProcess(dw); 
}

// Buffer clean up routine
void Cleanup(PSID pEveryoneSID, PSID pAdminSID, PACL pACL, PSECURITY_DESCRIPTOR pSD)
{
	if(pEveryoneSID)
		FreeSid(pEveryoneSID);
	if(pAdminSID)
		FreeSid(pAdminSID);
	if(pACL)
		LocalFree(pACL);
	if(pSD)
		LocalFree(pSD);
}