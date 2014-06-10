
#include <tchar.h>
#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#include <windows.h>
#include <string.h>

#include "DLLTP.h"

#include <aclapi.h>
#include <strsafe.h>

#include <stdlib.h>

//#define TAMLOGIN 15
//#define TAMPASS 15
//#define TAMTEXTO 100

//estrutura onde esta a info do chat global
CHATGLOBAL serverChat;
int linhas=0;
//HANDLE para o mutex que lida com o acesso ao serverChat
HANDLE mutex;
//HANDLE para o semaforo
//HANDLE semaforo;

#define NCLIENTES 30
#define PIPE_NAME TEXT("\\\\.\\pipe\\comunicacao")
#define PIPE_NAME2 TEXT("\\\\.\\pipe\\difusao")

//
TCHAR utilizadoresOnline[NCLIENTES][TAMLOGIN];


HANDLE hPipesDifusao[NCLIENTES]={NULL};
int total=0; //total de utilizadores ja registados

//prototipos
void Cleanup(PSID pEveryoneSID, PSID pAdminSID, PACL pACL, PSECURITY_DESCRIPTOR pSD);
void ErrorExit(LPTSTR lpszFunction);
boolean AutenticaUtilizador(UTILIZADOR *user);
boolean CriaNovoUtilizador(UTILIZADOR *user);

//estrutura temporal
SYSTEMTIME st;

int SESSAO = 1;

DWORD WINAPI AtendeCliente(LPVOID param){
	BOOL ret = FALSE;
	TCHAR buf[256];
	TCHAR out[256];
	DWORD n;
	int i;
	int resposta;
	HANDLE hPipe = (HANDLE) param;

	//necessario para o tokenizer
	TCHAR separators[]   = _T("|");
	TCHAR *emissor;
	TCHAR *comando;
	TCHAR *texto;
	UTILIZADOR TEMP[] = {NULL};
	UTILIZADOR TEMP1[] = {NULL};
	//TCHAR * token;
	//int Col = 0;


	_tprintf(TEXT("[SERVIDOR-%d] Um cliente ligou-se...\n"),GetCurrentThreadId());
	while (1) {
		_tprintf(_T("Vamos ler no pipe\n"));
		ret = ReadFile(hPipe, buf, sizeof(buf), &n, NULL);
		buf[n / sizeof(TCHAR)] = '\0';
		if (!ret || !n)
			break;
		_tprintf(TEXT("[Thread %d]  enviou %d bytes: '%s'... (ReadFile)\n"), GetCurrentThreadId(), n, buf);

		//recebeu comando - mais vale aqui ler sempre o mm numero de tokens

		//formato normal
		//token = _tcstok( buf, separators);
		//while( token != NULL )
		//{

		//    _tprintf(_T("Token: %s\n"),token);
		//    token = _tcstok(NULL,separators);
		//}

		emissor = _tcstok( buf, separators);
		_tprintf(_T("Emissor: %s\n"),emissor);
		comando = _tcstok(NULL,separators);
		_tprintf(_T("Comando: %s\n"),comando);
		texto = _tcstok(NULL,separators);
		_tprintf(_T("Texto: %s\n"),texto);

		if(lstrcmpW(comando, TEXT("GLOBAL"))==0)
		{
			//se for mensagem global
			//prepara estrutura de mensagem para difundir
			GetLocalTime(&st);

			//constroi a string de output
			_stprintf_s(out,TAMTEXTO, TEXT("(%02d/%02d/%04d - %02d:%02d:%02d) [%s]: %s"), st.wDay,st.wMonth, st.wYear,st.wHour,st.wMinute, st.wSecond,emissor, texto);			
			resposta=_tcslen(out)*sizeof(TCHAR);

			//==============================adicionar a mensagem ao serverChat=======================

			//Espera pelo semaforo
			//WaitForSingleObject(semaforo,INFINITE);

			//Para obter o direito de usar estrutura serverChat global
			WaitForSingleObject(mutex,INFINITE);

			wcsncpy_s(serverChat.publicas[linhas].login,TAMLOGIN,emissor,TAMLOGIN);
			
			serverChat.publicas[linhas].instante.dia=st.wDay;
			serverChat.publicas[linhas].instante.mes=st.wMonth;
			serverChat.publicas[linhas].instante.ano=st.wYear;
			serverChat.publicas[linhas].instante.hora=st.wHour;
			serverChat.publicas[linhas].instante.minuto=st.wMinute;
			serverChat.publicas[linhas].instante.segundo=st.wSecond;

			wcsncpy_s(serverChat.publicas[linhas++].texto,TAMTEXTO,texto,TAMTEXTO);

			//Liberta mutex
			ReleaseMutex(mutex);
			//Liberta semaforo
			//ReleaseSemaphore(semaforo,1,NULL);

			//==============================fim_adicionar a mensagem ao serverChat=======================

			//debug only
			_tprintf(TEXT("Linhas de mensagens guardadas: %d\n"),linhas);

			//divulgar informação a todos 
			for(i=0;i<total;i++)
			{
				if(hPipesDifusao[i]!=NULL)
					WriteFile(hPipesDifusao[i],out,resposta,&n,NULL);
			}
		}
		else if(lstrcmpW(comando, TEXT("A"))==0)
		{
			//validar credenciais
			//emissor
			//texto
			
			
			_stprintf_s(TEMP->login,TAMLOGIN,emissor);
			_stprintf_s(TEMP->password,TAMPASS,texto);
			
			//Vamos autenticar se for com sucesso envia a confirmação ao cliente
			if(!AutenticaUtilizador(&TEMP[0])){
				_tprintf(TEXT("NOK NOK\n"));

			}else{
				_tprintf(TEXT("OK OK\n"));

				SESSAO = 1;
				
				///////////////////////////////////////
				
				wcsncpy_s(utilizadoresOnline[total],TAMLOGIN,TEMP->login,TAMLOGIN);
				_tprintf(TEXT("Ultimo user logado: %s\n"),utilizadoresOnline[total]);
				_tprintf(TEXT("Numero de utilizadores online: %d\n"),total);
				///////////////////////////////////////


			}


		}
		else if(lstrcmpW(comando, TEXT("C"))==0)
		{
			
			//validar credenciais
			//emissor
			//texto
			
			_tprintf(_T("Criar utilizador\n"));
			_stprintf_s(TEMP1->login,TAMLOGIN,emissor);
			_stprintf_s(TEMP1->password,TAMPASS,texto);
			////criar novo user
			CriaNovoUtilizador(&TEMP1[0]);
			Sleep(600);
			//if(!){
			//	_tprintf(_T("NOK NOK"));
			//}else{
			//	_tprintf(_T("OK OK"));
			//}
		}else if(lstrcmpW(comando, TEXT("EXIT"))==0) //EXIT
		{
			
			GetLocalTime(&st);

			//constroi a string de output
			_stprintf_s(out,TAMTEXTO, TEXT("(%02d/%02d/%04d - %02d:%02d:%02d) [SERVIDOR]: O Servidor foi desligado pelo Administrador."), st.wDay,st.wMonth, st.wYear,st.wHour,st.wMinute );			
			resposta=_tcslen(out)*sizeof(TCHAR);


			//Resposta vai ser a informação que o servidor vai desligar
			
			for(i=0;i<total;i++)
			{
				if(hPipesDifusao[i]!=NULL)
					WriteFile(hPipesDifusao[i],out,resposta,&n,NULL);
			}
			
			_tprintf(_T("Shutdown Servidor...\n"));
			Sleep(1500);
			exit(EXIT_SUCCESS);
		}

		else if(lstrcmpW(comando, TEXT("LISTAONLINE"))==0)
		{
			WriteFile(hPipe, buf, _tcslen(buf)*sizeof(TCHAR), &n, NULL);

		}else if(lstrcmpW(comando, TEXT("Valida"))==0)
		{
			//Sleep(600);
			if(SESSAO == 1){
				_stprintf_s(out,TAMTEXTO,TEXT("OK"));
			}else{
				_stprintf_s(out,TAMTEXTO,TEXT("NOTOK"));		
			}
			WriteFile(hPipe, out, _tcslen(out)*sizeof(TCHAR), &n, NULL);
		}

		////retorna no pipe de comunicaçao o numero de bytes recebidos
		//WriteFile(hPipe,&resposta,sizeof(int),&n,NULL);

	}
	_tprintf(TEXT("[SERVIDOR-%d] Vou desligar o pipe... (DisconnectNamedPipe/CloseHandle)\n"), GetCurrentThreadId());
	if (!DisconnectNamedPipe(hPipe)){
		_tperror(TEXT("Erro ao desligar o pipe!"));
		ExitThread(-1);
	}

	CloseHandle(hPipe);
	return 0;
}


//==========================================================
//				Cria novo utilizador
//Fomato no regedit:
//		NOME -> Indica o utilizador o valor do utilizador é a pass
//		T_NOME -> Tipo do utilizador
//		E_NOME	-> Estado do utilizador
//Retorna 1 se for bem sucedido
//ou 0 caso contrario
//==========================================================

boolean CriaNovoUtilizador(UTILIZADOR *user){
	HKEY chave;
	DWORD queAconteceu;
	DWORD versao, tamanho;
	DWORD NRUseres = 0;
	TCHAR charBuf[10] = TEXT("U");
	TCHAR DateBufE[15] = TEXT("00/00/0000");
	TCHAR buf;

	if(RegCreateKeyEx(HKEY_LOCAL_MACHINE,TEXT("Software\\SO2Chat\\Utilizadores"),0, NULL, 
		REG_OPTION_NON_VOLATILE, //Fica mesmo apos fechar o programa
		KEY_ALL_ACCESS,	//O que vai ser feito no programa
		NULL,	//Assim o Null Esta a criar permissões para todos de read
		&chave,  //Handle da chave
		&queAconteceu	//O que aconteceu [ É criada ou já foi criada(se já exisitir devolve com o valor a informar que já foi aberta)]
		) != ERROR_SUCCESS)
		_tprintf(TEXT("Erro ao criar/abrir chave\n"));
	else
		//Se a chave foi criada, inicializar os valores

		//concatena strings
		lstrcatW(charBuf,user->login);
	//lstrcatW(charBufE,user->login);

	if(queAconteceu == REG_CREATED_NEW_KEY){

		_tprintf(TEXT("Chave: HKEY_LOCAL_MACHINE\\Software\\SO2Chat\\Utilizadores criada\n"));

		GetLocalTime(&st); //obtem data e hora actual

		_stprintf(DateBufE,TEXT("%d/%d/%d"), st.wDay, st.wMonth, st.wYear); 
		lstrcatW(user->DataRegisto,DateBufE);


		RegSetValueEx(chave,charBuf,0,REG_BINARY,(LPBYTE)user,sizeof(UTILIZADOR));

		_tprintf(TEXT("Utilizador %s adicionado\n"),user->login);

		return 1; //OK
	}else if(queAconteceu == REG_OPENED_EXISTING_KEY){

		_stprintf(DateBufE,TEXT("%d/%d/%d"), st.wDay, st.wMonth, st.wYear); 
		lstrcatW(user->DataRegisto,DateBufE);

		RegSetValueEx(chave,charBuf,0,REG_BINARY,(LPBYTE)user,sizeof(UTILIZADOR));


		_tprintf(TEXT("Utilizador %s já existia\n"),user->login);

		return 0; //NotOK
	}

	
};

//================================================
//				Autentica utilizador
//	Retorna 1 se estiver OK
//	User e pass
//	Caso NotOK retorna 0
//================================================
boolean AutenticaUtilizador(UTILIZADOR *user){
	HKEY chave;
	UTILIZADOR TEMP[1] = {NULL,NULL,NULL,NULL};
	TCHAR DateBufE[15] = TEXT("00/00/0000");
	TCHAR  pass[TAMPASS];
	DWORD  TAM = sizeof(UTILIZADOR);


	TCHAR charBuf[15] = TEXT("U");

	lstrcatW(charBuf,user->login);
	//Open KEY
	if( RegOpenKeyEx(HKEY_LOCAL_MACHINE,TEXT("Software\\SO2Chat\\Utilizadores"),0,KEY_READ,&chave) == ERROR_SUCCESS){

		//Find User

		if(RegQueryValueEx(chave, charBuf, NULL, NULL, (LPBYTE)&TEMP, &TAM) == ERROR_SUCCESS){


			_tprintf(TEXT(" %s =? %s.\n"),user->password,TEMP->password);

			if(!lstrcmpW(TEMP->password,user->password)){


				GetLocalTime(&st); //obtem data e hora actual

				_stprintf(DateBufE,TEXT("%d/%d/%d"), st.wDay, st.wMonth, st.wYear); 
				lstrcatW(user->DataRegisto,DateBufE);


				_tprintf(TEXT("[OK] %s atenticado.\n"),user->login);
				return 1;
			}else{
				_tprintf(TEXT("[NOTOK] %s Password errada.\n"),user->login);
				return 0;
			}

		}else {
			_tprintf(TEXT("[NOTOK] %s não existe na BD.\n"),user->login);
			return 0;
		}

	}else{
		_tprintf(TEXT("[ERRO] Erro a abrir a Key do utilizador.\n"));
		return 0;
	}
};


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

	//Estrutura teste
	UTILIZADOR novo[3]= {
		{TEXT("Admin"),TEXT("admin"),1,1},
		{ TEXT("ZE"),TEXT("ZE"),1,1}
	};


	//UNICODE: By default, windows console does not process wide characters. 
	//Probably the simplest way to enable that functionality is to call _setmode:
#ifdef UNICODE 
	_setmode(_fileno(stdin), _O_WTEXT); 
	_setmode(_fileno(stdout), _O_WTEXT); 
#endif

	//mecanismos de sincronizaçãp
	// Cria mutex inicialmente livre
	mutex=CreateMutex(NULL,FALSE,NULL);

	// Cria semaforo com 2 unidades
	//semaforo=CreateSemaphore(NULL,1,1,NULL);


	//Teste de criar utilizador e salvar no regedit
	//Tenho de enviar os dados em separado
	for (i = 0; i < 2; i++){
		CriaNovoUtilizador(&novo[i]);
		AutenticaUtilizador(&novo[i]);
	}


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
	for (i = 0; i < total; i++){
		WaitForSingleObject(threads[total], INFINITE);
	}
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