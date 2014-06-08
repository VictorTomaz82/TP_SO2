#include <windows.h>
#include <tchar.h>
#include <io.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

#include "resource.h"
#include "DLLTP.h"
#define TAM 255

//definir o nome dos named pipes
#define PIPE_NAME TEXT("\\\\.\\pipe\\comunicacao")
#define PIPE_NAME2 TEXT("\\\\.\\pipe\\difusao")

//=======================================================================declaraçoes temporarias
int NAutenticar(TCHAR *login, TCHAR *pass);
int NCriaNovoUtilizador(TCHAR *login, TCHAR *pass);
int NLerListaUtilizadores(UTILIZADOR *utilizadores);
int NLerListaUtilizadoresRegistados(UTILIZADOR *utilizadores);
int NIniciarConversa(TCHAR *utilizador);
int NDesligarConversa();	
int NEnviarMensagemPrivada(TCHAR *msg);
void NEnviarMensagemPública(TCHAR *msg);	
//CHAT NLerInformacaoInicial();
//MENSAGEM NLerMensagensPublicas();
//MENSAGEM NLerMensagensPrivadas();
int NSair();
int NDesligar();
//=======================================================================declaraçoes temporarias

// Variável global hInstance usada para guardar "hInst" inicializada na função
// WinMain(). "hInstance" é necessária no bloco WinProc() para lançar a Dialog
// box em DialogBox(...) 
HINSTANCE hInstance;	
int linha = 0;

//dados para usar no pipe
DWORD n;
HANDLE hPipeComunicacao;
HANDLE hPipeDifusao;

//UNICODE: By default, windows console does not process wide characters. 
//Probably the simplest way to enable that functionality is to call _setmode:
//#ifdef UNICODE
//	_setmode(_fileno(stdin), _O_WTEXT); 
//	_setmode(_fileno(stdout), _O_WTEXT); 
//#endif

//para a janelavirtual
HBITMAP hbit;						// hanlde para um bitmap
int maxX, maxY;						// Dimensões do écran
HBRUSH hbrush;						// Cor de fundo da janela
HDC memdc;							// handler para imagem da janela em memória

typedef int (*PonteiroFuncao)(TCHAR*, TCHAR*);

// Declaração das funções de processamento de eventos das janelas
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK WndProc2(HWND, UINT, WPARAM, LPARAM);

BOOL CALLBACK DialogProc(HWND, UINT, WPARAM, LPARAM);
INT CALLBACK DialogAutenticacao(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DialogUtilizadores(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DialogGerir(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DialogMessgPublica(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DialogAcerca(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);

//Declaração de funcões auxilaires
void printChat(HWND hWnd);
DWORD WINAPI TFuncEnvioPublico( LPVOID lpParam );
DWORD WINAPI TFuncEnvioPrivado( LPVOID lpParam );

//Arrays de estrutura UTILIZADOR
UTILIZADOR users[NUMUTILIZADORES];
int total;
UTILIZADOR online[NUMUTILIZADORES];
int totalonline;
CHAT mychat;


// ============================================================================
// WinMain()
// NOTA: Ver declaração de HACCEL, LoadAccelerators() e TranslateAccelerator()
//		 Estas são as modificações necessárias para tratar as teclas de atalho
//		 para opções do menu
// ============================================================================

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow) 
{
	HWND hWnd;			// handler da janela (a gerar por CreateWindow())
	MSG lpMsg;			// Estrutura das mensagens
	WNDCLASSEX wcApp,wcApp2;	// Estrutura que define a classe da janela
	HACCEL hAccel;		// Handler da resource accelerators (teclas de atalho do menu)

	LPTSTR lpszWrite = TEXT("Mensagem enviada pelo cliente!"); //mensagem de teste

	//valores usados na definiçao da classe da janela
	wcApp.cbSize = sizeof(WNDCLASSEX);	
	wcApp.hInstance = hInst;													
	wcApp.lpszClassName = TEXT("Chat");
	wcApp.lpfnWndProc = WndProc;		
	wcApp.style = CS_HREDRAW | CS_VREDRAW ;
	wcApp.hIcon = LoadIcon(hInst, (LPCTSTR)IDI_ICON1);
	wcApp.hIconSm = LoadIcon(hInst, (LPCTSTR) IDI_ICON1);																	
	wcApp.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcApp.lpszMenuName = (LPCTSTR) IDR_MENU1;		// Menu da janela		

	wcApp.cbClsExtra = 0;				
	wcApp.cbWndExtra = 0;
	wcApp.hbrBackground = CreateSolidBrush(RGB(210,210,210));	

	//valores a usar na janela de chat privado
	wcApp2.cbSize = sizeof(WNDCLASSEX);	
	wcApp2.hInstance = hInst;													
	wcApp2.lpszClassName = TEXT("Chat Privado");
	wcApp2.lpfnWndProc = WndProc2;		
	wcApp2.style = CS_HREDRAW | CS_VREDRAW;
	wcApp2.hIcon = LoadIcon(hInst, (LPCTSTR)IDI_ICON1);
	wcApp2.hIconSm = LoadIcon(hInst, (LPCTSTR) IDI_ICON4);																	
	wcApp2.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcApp2.lpszMenuName = (LPCTSTR) NULL;		// Menu da janela		

	wcApp2.cbClsExtra = 0;				
	wcApp2.cbWndExtra = 0;
	wcApp2.hbrBackground = CreateSolidBrush(RGB(210,210,210));	

	////

	//registo da classe da janela
	if (!RegisterClassEx(&wcApp))
		return(0);
	//registo da classe da janela
	if (!RegisterClassEx(&wcApp2))
		return(0);


	// ============================================================================
	// Guardar hInst na variável global hInstance (instância do programa actual)
	// para uso na função de processamento da janela WndProc()
	// ============================================================================
	hInstance=hInst;

	//criação da janela principal
	hWnd = CreateWindow(
		TEXT("Chat"),				// Nome da janela e/ou programa
		TEXT("Chat"),	// Título da janela
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,	// Estilo da janela 
		250,			// Posição x 
		80,			// Posição y 
		800,			// Largura 
		600,			// Altura 
		(HWND) HWND_DESKTOP,	// handle da janela pai (HWND_DESKTOP para 1ª)
		(HMENU) NULL,			// handle do menu (se tiver menu)
		(HINSTANCE) hInst,			// handle da instância actual (vem de WinMain())
		(LPSTR) NULL);			// Não há parâmetros adicionais 

	// ============================================================================
	// Carregar as definições das teclas aceleradoras (atalhos de opções dos Menus)
	// LoadAccelerators(instância_programa, ID_da_resource_atalhos)
	// ============================================================================
	hAccel=LoadAccelerators(hInst, (LPCWSTR) IDR_ACCELERATOR1);


	ShowWindow(hWnd, nCmdShow);	// "hWnd"= handler da janela "nCmdShow"= modo, parâmetro de WinMain()
	UpdateWindow(hWnd);			// Refrescar a janela (gera WM_PAINT) 

	
	///////////////////////////////////LIGAÇÂO AOS PIPES/////////////////////////////////////

	//Verifica se o pipe de comunicaçao existe
	
	if (!WaitNamedPipe(PIPE_NAME, NMPWAIT_WAIT_FOREVER)) {
		MessageBox(NULL, TEXT("Nao encontrou o pipe de comunicacao!"), TEXT("ERRO!"), MB_OK);
		//exit(1);
		return 1;
    }

	//instancia o pipe
    hPipeComunicacao = CreateFile(PIPE_NAME, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hPipeComunicacao==NULL) {
		MessageBox(NULL, TEXT("Erro ao instanciar o pipe de comunicacao!"), TEXT("ERRO!"), MB_OK);
		//exit(1);
		return 1;
    }

	//é preciso pra nao dar erro :" no error"
	Sleep(200);

	//esperar pelo pipe de difusao
		if (!WaitNamedPipe(PIPE_NAME2, NMPWAIT_WAIT_FOREVER)) {
		MessageBox(NULL, TEXT("Nao encontrou o pipe de difusao!"), TEXT("ERRO!"), MB_OK);
		//exit(1);
		return 1;
    }

	//instancia o pipe de difusao
    hPipeDifusao = CreateFile(PIPE_NAME2, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hPipeDifusao==NULL) {
		MessageBox(NULL, TEXT("Erro ao instanciar o pipe de difusao!"), TEXT("ERRO!"), MB_OK);
		//exit(1);
		return 1;
    }

	//mensagem de teste	
	WriteFile(hPipeComunicacao, lpszWrite, (lstrlen(lpszWrite)+1)*sizeof(TCHAR), &n, NULL);






	// ============================================================================
	// Loop de Mensagens
	// Para usar as teclas aceleradoras do menu é necessário chamar a função
	// TranslateAccelerator(handler_janela, handler_accelerators, pont_mensagem)
	// Se esta função devolver falso, não foi premida tecla de aceleração 
	// ============================================================================

	while (GetMessage(&lpMsg,NULL,0,0)) {	
		//if(!TranslateAccelerator(hWnd, hAccel, &lpMsg)){
		TranslateMessage(&lpMsg);		// Pré-processamento da mensagem
		DispatchMessage(&lpMsg);		// Enviar a mensagem traduzida de volta ao Windows
	}
	//}									
	return((int)lpMsg.wParam);	// Status = Parâmetro "wParam" da estrutura "lpMsg"
}


// ============================================================================
// FUNÇÕES DE PROCESSAMENTO DE EVENTOS DAS JANELAS PRINCIPAIS
// ============================================================================

LRESULT CALLBACK WndProc(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam) 
{
	int resposta;					// Resposta a MessageBox	
	HDC hdc;
	HDC auxdc;
	HDC auxmemdc;					// handler para Device Context auxiliar em memória
	int i;
	HWND hWndList;
	TCHAR str[2 * TAMTEXTO];
	PAINTSTRUCT PtStc;				// Ponteiro para estrutura de WM_PAINT

	HANDLE hPipe;

	// Processamento das mensagens
	switch (messg) 
	{
	case WM_CREATE:

		// Criação da Janela: Criar uma janela virtual para memorizar oconteúdo da actual
		maxX = GetSystemMetrics(SM_CXSCREEN);		// Tamanho máximo = Écran (caso de a janela real estar maximizada)
		maxY = GetSystemMetrics(SM_CYSCREEN);
		hdc = GetDC(hWnd);
		memdc = CreateCompatibleDC(hdc);					// Criar janela virtual
		hbit = CreateCompatibleBitmap(hdc, maxX, maxY);
		SelectObject(memdc, hbit);
		hbrush = (HBRUSH)GetStockObject(WHITE_BRUSH);		// Fundo jan.virtual=branco
		SelectObject(memdc, hbrush);
		PatBlt(memdc, 0, 0, maxX, maxY, PATCOPY);
		ReleaseDC(hWnd, hdc);

	//parte de comunicaçao

	//do { 
	//	//_tprintf(TEXT("[CLIENTE] Comando: "));
	//	//_tscanf_s(TEXT("%s"), str, 256);


	//	//if (!WriteFile(hPipe, str, _tcslen(str)*sizeof(TCHAR), &n, NULL)) {
	//	if (!WriteFile(hPipe, "teste", 5*sizeof(TCHAR), &n, NULL)) {
	//		//_tperror(TEXT("[ERRO] Escrever no pipe... (WriteFile)\n"));
	//		MessageBox(hWnd, TEXT("Erro ao escrever no pipe"), TEXT("Fim"), MB_OK);
	//		exit(1);
	//	}
	//	//_tprintf(TEXT("[CLIENTE] Enviei %d bytes ao servidor... (WriteFile)\n"), n);
	//	resposta=MessageBox(hWnd, TEXT("enviei dados. sair?"), TEXT("Fim"), MB_YESNO);
	////} while (_tcscmp(str, TEXT("fim")));
	//} while (!resposta);

	//Sleep(2000);

 //   //_tprintf(TEXT("[CLIENTE] Vou desligar o pipe... (CloseHandle)\n"));
 //   CloseHandle(hPipe);

	//Sleep(2000);

	//exit(0);
	//}

		//Pede login
		resposta=DialogBox(hInstance, (LPCWSTR)IDD_DIALOG2, hWnd, (DLGPROC)DialogAutenticacao);
		if(resposta==-1){
			MessageBox(hWnd, TEXT("Login cancelado! A Sair..."), TEXT("Erro!"), MB_OK);
			PostQuitMessage(0);
		}

		//imprime o chat
		printChat(hWnd);
		LerInformacaoInicial();

		//cria listbox
		hWndList = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("listbox"), TEXT(""), 
			WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_AUTOVSCROLL, 
			630, 0, 150, 545, hWnd, (HMENU)105, NULL, NULL);

		//preenche a lista com os utilizadores online
		for (i = 0; i < totalonline; i++){
			SendDlgItemMessage(hWndList, NULL, LB_ADDSTRING, 0, (LPARAM)online[i].login);
		}
		total = NLerListaUtilizadoresRegistados(users);

	case WM_PAINT:
		//actualiza a listbox da main window
		totalonline = NLerListaUtilizadores(online);
		for (i = 0; i < totalonline; i++){
			SendDlgItemMessage(hWnd, IDC_LIST1, LB_ADDSTRING, 0, (LPARAM)online[i].login);
		}
		//"pinta" a mainwindow
		hdc = BeginPaint(hWnd, &PtStc);
		BitBlt(hdc, 0, 0, maxX, maxY, memdc, 0, 0, SRCCOPY);
		EndPaint(hWnd, &PtStc);
		break;

		//==============================================================================
		// Resposta a opções do Menu da janela
		// Clicar num menu gera a mensagem WM_COMMAND. Nesta mensagem:
		// O valor LOWORD(wParam) é o identificador das opções do menu (ver o ficheiro 
		// "resource.h" que o VC++ criou). Os símbolos usados em seguida no "switch"  
		// estão definidos em "resource.h"
		//==============================================================================
	case WM_COMMAND: 
		switch (LOWORD(wParam)) 
		{				
		case ID_LOGIN:{
			SYSTEMTIME hora;
			GetSystemTime(&hora);			
			DialogBox(hInstance, (LPCWSTR)IDD_DIALOG2, hWnd, (DLGPROC)DialogAutenticacao);
			//imprime o chat
			printChat(hWnd);
			break;
					  }
		case ID_UTILIZADORES:
			DialogBox(hInstance, (LPCWSTR)IDD_DIALOG3, hWnd, (DLGPROC)DialogUtilizadores);
			break;
		case ID_ENVIA:
			DialogBox(hInstance, (LPCWSTR)IDD_DIALOG4, hWnd, (DLGPROC)DialogMessgPublica);
			break;
		case ID_LE:
			DialogBox(hInstance, (LPCWSTR)IDD_DIALOG5, hWnd, (DLGPROC)DialogMessgPublica);
			break;
		//as proximas duas opccoes só vao estar disponiveis ao admin (vao estar escondidas aos restantes Utilizadores)
		case ID_GERIR_CRIARNOVOUTILIZADOR:
			DialogBox(hInstance, (LPCWSTR)IDD_DIALOG7, hWnd, (DLGPROC)DialogGerir);
			break;
		case ID_GERIR_DESLIGARSERVER:
			//envia mensagem ao server para ele se desligar 
			NDesligar();
			break;
		case ID_ACERCA:
			DialogBox(hInstance, (LPCWSTR)IDD_DIALOG1, hWnd, (DLGPROC)DialogAcerca);
			break;

		case WM_MOVE:						// Detectar que a janela se moveu
			InvalidateRect(hWnd, NULL, 1);	// Gerar WM_PAINT para actualizar 
			// as coordenadas visíveis na janela
			break;
		}
		break;
		//==============================================================================
		// Terminar e Processamentos default
		//==============================================================================
	case WM_CLOSE:
		resposta = MessageBox(hWnd, TEXT("Terminar o Programa?"), TEXT("Fim"), MB_YESNO | MB_ICONQUESTION);
		if (resposta == IDYES)
			PostQuitMessage(0);
		break;
		// Restantes mensagens têm processamento default
	default:
		return(DefWindowProc(hWnd,messg,wParam,lParam));
		break;
	}
	return(0);
}

LRESULT CALLBACK WndProc2(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam) 
{
	int resposta;					// Resposta a MessageBox	
	HDC hdc;
	HDC auxdc;
	static HDC janela_virtual;
	int i;
	HWND hWndList;
	TCHAR str[2 * TAMTEXTO];
	PAINTSTRUCT PtStc;				// Ponteiro para estrutura de WM_PAINT
	static int maxX,maxY;
	DWORD dwThreadId;

	// Processamento das mensagens
	switch (messg) 
	{
	case WM_CREATE:

		//cria editbox
		CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("EDIT"), TEXT(""), 
			WS_CHILD | WS_VISIBLE | ES_AUTOVSCROLL | WS_TABSTOP, 
			10, 530, 570,25, hWnd, (HMENU)IDC_PVT_EDIT,GetModuleHandle(NULL), NULL);

		CreateWindowEx(NULL, TEXT("BUTTON"), TEXT("Enviar"), 
			WS_CHILD | WS_VISIBLE | WS_TABSTOP| BS_DEFPUSHBUTTON, 
			620, 530, 130, 25, hWnd, (HMENU)IDC_PVT_SEND,GetModuleHandle(NULL), NULL);

		//imprime o chat
		//printChat(hWnd);


		//
		//hdc = GetDC(hWnd);
		////CriaBrush(hdc);
		//Rectangle(hdc, 200, 200, 400, 400);

		//ReleaseDC(hWnd, hdc);
		//

	case WM_PAINT:

		//"pinta" a mainwindow
		hdc = BeginPaint(hWnd, &PtStc);
		BitBlt(hdc, 0, 0, maxX, maxY, janela_virtual, 0, 0, SRCCOPY);
		EndPaint(hWnd, &PtStc);
		break;

		//==============================================================================
		// Resposta a opções do Menu da janela
		// Clicar num menu gera a mensagem WM_COMMAND. Nesta mensagem:
		// O valor LOWORD(wParam) é o identificador das opções do menu (ver o ficheiro 
		// "resource.h" que o VC++ criou). Os símbolos usados em seguida no "switch"  
		// estão definidos em "resource.h"
		//==============================================================================
	case WM_COMMAND: 
		switch (LOWORD(wParam)) 
		{	
		case IDC_PVT_SEND:
			{
				//cria thread de envio de mensagem privada com o conteudo da editbox
				GetDlgItemText(hWnd, IDC_PVT_EDIT, str, TAMTEXTO);
				//CreateThread(NULL,0,TFuncEnvioPrivado,str,0,&dwThreadId);
			}


		case WM_MOVE:						// Detectar que a janela se moveu
			InvalidateRect(hWnd, NULL, 1);	// Gerar WM_PAINT para actualizar as coordenadas visíveis na janela	
			break;
		}
		break;
		//==============================================================================
		// Terminar e Processamentos default
		//==============================================================================
	case WM_CLOSE:
		//desliga a conversa privada
		NDesligarConversa();

		//resposta = MessageBox(hWnd, TEXT("Terminar o Programa?"), TEXT("Fim"), MB_YESNO | MB_ICONQUESTION);
		//if (resposta == IDYES)
		//	PostQuitMessage(0);
		//break;
		// Restantes mensagens têm processamento default
	default:
		return(DefWindowProc(hWnd,messg,wParam,lParam));
		break;
	}
	return(0);
}

// ============================================================================
// FUNÇÕES DE PROCESSAMENTO DAS DIALOG BOXES
// ============================================================================
INT CALLBACK DialogAutenticacao(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{	
	TCHAR login[30], passwd[30], mensagem[100];
	int i;
	MENSAGEM ultima;
	int resposta=0;


	//====================== ciclo de interpretação de eventos

	switch (messg){

	case WM_COMMAND: //Clicou num controlo
		switch (LOWORD(wParam)){ //Que controlo?
		case IDCANCEL:

			EndDialog(hWnd, -1);			
			return -1;

		case IDOK:
			//Buscar as 2 strings
			//Comparar com admin admin : em C++ ==, em C _tcscmp(str1,str2)

			GetDlgItemText(hWnd, IDC_EDIT1, login, 30);
			GetDlgItemText(hWnd, IDC_EDIT2, passwd, 30);				
			if (!NAutenticar(login, passwd)){
				MessageBox(hWnd, TEXT("Aceite"), TEXT("Login"), MB_OK);
				EnviarMensagemPública(TEXT("Olá"));
				ultima = LerMensagensPublicas();
				EndDialog(hWnd, 0);
			}
			else
				MessageBox(hWnd, TEXT("Não aceite"), TEXT("Login"), MB_OK);	
			SetDlgItemText(hWnd,IDC_EDIT2,TEXT(""));
			return 1;
		case IDC_LIST1:
			if (HIWORD(wParam) == LBN_DBLCLK){
				//Preencher caixa de texto com o texto selecionado da listbox
				int i = SendDlgItemMessage(hWnd, IDC_LIST1, LB_GETCURSEL, 0, 0);
				TCHAR login_temp[30];
				SendDlgItemMessage(hWnd, IDC_LIST1, LB_GETTEXT, i, (LPARAM)login_temp);
				SetDlgItemText(hWnd, IDC_EDIT1, (LPCWSTR) login_temp);

			}
			return 1;
		}
	}
	return 0;

}

BOOL CALLBACK DialogUtilizadores(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam){
	int i;
	HWND hWnd2;

	switch (messg){

	case WM_COMMAND:
		if (LOWORD(wParam) == IDC_LIST1 && HIWORD(wParam) == LBN_DBLCLK){

			////

			//criação da janela de chat privado
			hWnd2 = CreateWindow(
				TEXT("Chat Privado"),				// Nome da janela e/ou programa
				TEXT("Chat Privado"),	// Título da janela
				WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,	// Estilo da janela 
				250,			// Posição x 
				80,			// Posição y 
				800,			// Largura 
				600,			// Altura 
				(HWND) hWnd,	// handle da janela pai (HWND_DESKTOP para 1ª)
				(HMENU) NULL,			// handle do menu (se tiver menu)
				(HINSTANCE) NULL,			// handle da instância actual (vem de WinMain())
				(LPSTR) NULL);			// Não há parâmetros adicionais 

			ShowWindow(hWnd2,1);			// "hWnd"= handler da janela 
			UpdateWindow(hWnd2);			// Refrescar a janela (gera WM_PAINT) 

			/*EndDialog(hWnd, 0);*/
			///

			//isto nao pode ser assim porcausa da modalidade
			//DialogBox(hInstance, (LPCWSTR)IDD_DIALOG6, hWnd, (DLGPROC)DialogMessgPrivada);
			//EndDialog(hWnd,0);
		}
		return 1;

	case WM_CLOSE:
		EndDialog(hWnd, 0);
		return 1;

	case WM_INITDIALOG:
		totalonline = NLerListaUtilizadores(online);
		for (i = 0; i < totalonline; i++){
			SendDlgItemMessage(hWnd, IDC_LIST1, LB_ADDSTRING, 0, (LPARAM)online[i].login);
		}
		return 1;
	}
	return 0;
}

BOOL CALLBACK DialogMessgPublica(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam){
	MENSAGEM ultima;
	DWORD dwThreadId;
	TCHAR str[2 * TAMTEXTO];
	switch (messg){
	case WM_CLOSE:
		EndDialog(hWnd, 0);
		return 1;

	case WM_INITDIALOG:
		ultima = LerMensagensPublicas();
		_stprintf_s(str, 2 * TAMTEXTO, TEXT("(%02d/%02d/%d-%02d:%02d:%02d) %s"), ultima.instante.dia,
			ultima.instante.mes, ultima.instante.ano, ultima.instante.hora,
			ultima.instante.minuto, ultima.instante.segundo, ultima.texto);
		SetDlgItemText(hWnd, IDC_LIDA, str);
		return 1;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK){
			GetDlgItemText(hWnd, IDC_ENVIADA, str, TAMTEXTO);

			//cria uma thread que comunica com o server
			CreateThread(NULL,0,TFuncEnvioPublico,str,0,&dwThreadId);
			//actualizar o chat
			//printChat(hWnd);
			//EnviarMensagemPública(str);
			EndDialog(hWnd, 0);
		}
		return 1;
	}
	return 0;
}

BOOL CALLBACK DialogGerir(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{	
	TCHAR login[30], passwd[30], mensagem[100];
	int i;
	MENSAGEM ultima;
	switch (messg){
	case WM_INITDIALOG:
		//limpa conteudo das editboxes
		SetDlgItemText(hWnd, IDC_EDIT1, TEXT(""));
		SetDlgItemText(hWnd, IDC_EDIT2, TEXT(""));

		//preenche a listBox com os dados dos users
		for (i = 0; i<total; i++)
			SendDlgItemMessage(hWnd, IDC_LIST1, LB_ADDSTRING, 0, (LPARAM)users[i].login);

		// Seleccionar o item 0 da List Box
		SendDlgItemMessage(hWnd, IDC_LIST1, LB_SETCURSEL, 0, 0);		
		return 1;
	case WM_COMMAND: //Clicou num controlo
		switch (LOWORD(wParam)){ //Que controlo?
		case IDCANCEL:
			EndDialog(hWnd, 0);
			return 1;

		case IDOK:

			GetDlgItemText(hWnd, IDC_EDIT1, login, 30);
			GetDlgItemText(hWnd, IDC_EDIT2, passwd, 30);
			//falta adicionar na base de dados

			return 1;
		case IDC_LIST1:
			if (HIWORD(wParam) == LBN_DBLCLK){

				//Mostrar caixa de mensagem com o detalhes do utilizador selecionado
				int i = SendDlgItemMessage(hWnd, IDC_LIST1, LB_GETCURSEL, 0, 0);
				TCHAR login_temp[30];
				SendDlgItemMessage(hWnd, IDC_LIST1, LB_GETTEXT, i, (LPARAM)login_temp);
				for (i = 0; i < total; i++)
					if (!_tcscmp(login_temp, users[i].login)){
						_stprintf_s(mensagem, 100, TEXT("login:%s\npass:%s\nestado:%s\ntipo:%s"), users[i].login,
							users[i].password, (users[i].estado ? TEXT("online") : TEXT("offline")),
							(users[i].tipo == 1 ? TEXT("normal") : TEXT("administrador")));

					}
					MessageBox(hWnd, mensagem, TEXT("Detalhes do Utilizador"), MB_OK);
			}
			return 1;
		}
	}
	return 0;

}

BOOL CALLBACK DialogAcerca(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam){

	switch (messg){

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hWnd, LOWORD(wParam));
			return 1;
		}
		break;			

	case IDOK:
		EndDialog(hWnd, 0);
		return 1;
	}
	return 0;
}

//FUNÇÕES AUXILIARES E DE LANÇAMENTO DE THREADS
void printChat(HWND hWnd){

	TCHAR str[2 * TAMTEXTO];
	HDC hdc;


	if (linha == 0){ //Se ainda não apresentou informação inicial
		mychat = LerInformacaoInicial();
		hdc = GetDC(hWnd);

		for (; _tcscmp(mychat.publicas[linha].texto, TEXT("")); linha++){
			_stprintf_s(str, 2 * TAMTEXTO, TEXT("[%02d:%02d:%02d - %02d/%02d/%d] %s"), mychat.publicas[linha].instante.hora,
				mychat.publicas[linha].instante.minuto, mychat.publicas[linha].instante.segundo, mychat.publicas[linha].instante.dia,
				mychat.publicas[linha].instante.mes, mychat.publicas[linha].instante.ano, mychat.publicas[linha].texto);
			TextOut(hdc, 0, linha * 20, str, _tcslen(str));
			TextOut(memdc, 0, linha * 20, str, _tcslen(str));
		}
		ReleaseDC(hWnd, hdc);
	}

}

DWORD WINAPI TFuncEnvioPublico( LPVOID lpParam ) 
{ 
	NEnviarMensagemPública((TCHAR*)lpParam);
}

DWORD WINAPI TFuncEnvioPrivado( LPVOID lpParam ) 
{ 
	NEnviarMensagemPrivada((TCHAR*)lpParam);
}



//=====================================================FUNÇOES DO DLL=======================================================

/* Valida o acesso a um dado login e password de um dado utilizador.
	Esta função retorna a validação (sucesso ou insucesso) e ainda se o
	utilizador em causa é administrador.*/
int NAutenticar(TCHAR *login, TCHAR *pass)
{

	return 0;
}

int NCriaNovoUtilizador(TCHAR *login, TCHAR *pass)
{
	//verifica se existe o login no registo
	//caso nao haja acrescenta a info

	return 0;
}


/* Recebe (por argumento) informação actualizada de forma autónoma
sobre os jogadores online.*/
int NLerListaUtilizadores(UTILIZADOR *utilizadores)
{

	return 0;
}

/*Opcional, só para saber que utilizadores existem e quais são os seus detalhes*/
int NLerListaUtilizadoresRegistados(UTILIZADOR *utilizadores)
{

	return 0;
}

/* Pede permissão para iniciar conversa privada com um dado
utilizador. Caso este esteja livre é aceite, caso esteja já em
conversa privada com outro utilizador o pedido é recusado.*/
int NIniciarConversa(TCHAR *utilizador)
{

	return 0;
}

/* Comunica ao servidor que deseja terminar a conversa que mantém
actualmente em privado com um outro utilizador. As novas mensagens
enviadas em privado por este ou pelo outro serão rejeitadas. Esta
função devolve um código de erro, caso este utilizador não mantinha
qualquer conversa privada.*/
int NDesligarConversa()
{

	return 0;
}

/*Enviar a mensagem ao utilizador com quem mantemos a conversa
privada. Caso o utilizador destinatário já tenha desligado (ou este)
esta conversa, a função devolve um código de erro.*/	
int NEnviarMensagemPrivada(TCHAR *msg)
{

	return 0;
}

/*Enviar a mensagem a todos os utilizadores online.*/
void NEnviarMensagemPública(TCHAR *msg)
{

}

/* Recebe toda informação histórica necessária para apresentação nas
janelas de conversa pública e privada.*/	
//CHAT NLerInformacaoInicial()
//{
//
//}
	
/*Recebe informação autonomamente que é enviada pelo servidor e
acrescenta no fundo da janela de conversa pública.*/
//MENSAGEM NLerMensagensPublicas()
//{
//
//
//}

/*Recebe informação autonomamente que é enviada pelo servidor e
acrescenta no fundo da janela de conversa privada.*/
//MENSAGEM NLerMensagensPrivadas()
//{
//
//}

/* Envia o pedido de saída (logout) do sistema ao servidor. */
int NSair()
{

	return 0;
}

/* Envia o pedido de encerramento do sistema ao servidor. */
int NDesligar()
{

	return 0;
}

//==========================================================================================================================