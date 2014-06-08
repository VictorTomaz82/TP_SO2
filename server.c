
#include <tchar.h>
#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#include <windows.h>

#define NCLIENTES 30
#define PIPE_NAME TEXT("\\\\.\\pipe\\comunicacao")
#define PIPE_NAME2 TEXT("\\\\.\\pipe\\difusao")

//
HANDLE hPipesDifusao[NCLIENTES]={NULL};
int total=0; //total de utilizadores ja registados

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
	//UNICODE: By default, windows console does not process wide characters. 
	//Probably the simplest way to enable that functionality is to call _setmode:
#ifdef UNICODE 
	_setmode(_fileno(stdin), _O_WTEXT); 
	_setmode(_fileno(stdout), _O_WTEXT); 
#endif
	while (1){
		_tprintf(TEXT("[SERVIDOR] Vou passar à criação de uma cópia do pipe '%s'... (CreateNamedPipe)\n"), PIPE_NAME);
		hPipe = CreateNamedPipe(PIPE_NAME, PIPE_ACCESS_DUPLEX, PIPE_WAIT | PIPE_TYPE_MESSAGE
								| PIPE_READMODE_MESSAGE, NCLIENTES, sizeof(buf), sizeof(buf), 1000, NULL);
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
								| PIPE_READMODE_MESSAGE, NCLIENTES, sizeof(buf), sizeof(buf), 1000, NULL);
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
