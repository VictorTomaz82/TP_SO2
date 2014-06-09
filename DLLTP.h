#include <windows.h>
#include <tchar.h>
#define TAMLOGIN 15
#define TAMPASS 15
#define TAMTEXTO 100
#define NUMMSGSPUBLICAS 30
#define NUMMSGSPRIVADAS 30
#define NUMUTILIZADORES 30

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the DLL_IMP_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
//  functions as being imported from a DLL, whereas this DLL sees symbols

#include <windows.h>

//Definir uma constante para facilitar a leitura do protótipo da função
//Este .h deve ser incluído no projecto que o vai usar (modo implícito)

#define DllExport extern "C"

//Esta macro é definida pelo sistema caso estejamos na DLL (<DLL_IMP>_EXPORTS definida)
//ou na app (<DLL_IMP>_EXPORTS não definida) onde DLL_IMP é o nome deste projecto
#ifdef DLLTP_EXPORTS
#define  DLL_IMP_API __declspec(dllexport)
#else
#define DLL_IMP_API __declspec(dllimport)
#endif



	typedef struct{
		TCHAR login[TAMLOGIN], password[TAMPASS];
		TCHAR DataRegisto[15];
		TCHAR DataLastLogin[15];
		int tipo; //1-utilizador 2-admin
		int estado; //0-livre 1-ocupado numa conversa privada
		//Não pode ser short se não no regedit da problemas
	}UTILIZADOR;

	typedef struct{
		short int ano, mes, dia, hora, minuto, segundo;
	}DATA;

	typedef struct{
		TCHAR texto[TAMTEXTO];
		DATA instante;
	}MENSAGEM;


	typedef struct{
		MENSAGEM publicas[NUMMSGSPUBLICAS];
		MENSAGEM privadas[NUMUTILIZADORES][NUMUTILIZADORES][NUMMSGSPRIVADAS];
	}CHATGLOBAL;



	typedef struct{
		MENSAGEM publicas[NUMMSGSPUBLICAS];
		MENSAGEM privadas[NUMUTILIZADORES][NUMMSGSPUBLICAS];
	}CHAT;



	//DLL_IMP_API int Autenticar(TCHAR *login, TCHAR *pass);
	/* Valida o acesso a um dado login e password de um dado utilizador.
	Esta função retorna a validação (sucesso ou insucesso) e ainda se o
	utilizador em causa é administrador.*/
	//DLL_IMP_API int LerListaUtilizadores(UTILIZADOR *utilizadores);
	/* Recebe (por argumento) informação actualizada de forma autónoma
	sobre os jogadores online.*/
	//DLL_IMP_API int LerListaUtilizadoresRegistados(UTILIZADOR *utilizadores);
	/*Opcional, só para saber que utilizadores existem e quais são os seus detalhes*/
	//DLL_IMP_API int IniciarConversa(TCHAR *utilizador);
	/* Pede permissão para iniciar conversa privada com um dado
	utilizador. Caso este esteja livre é aceite, caso esteja já em
	conversa privada com outro utilizador o pedido é recusado.*/
	//DLL_IMP_API int DesligarConversa();
	/* Comunica ao servidor que deseja terminar a conversa que mantém
	actualmente em privado com um outro utilizador. As novas mensagens
	enviadas em privado por este ou pelo outro serão rejeitadas. Esta
	função devolve um código de erro, caso este utilizador não mantinha
	qualquer conversa privada.*/
	//DLL_IMP_API int EnviarMensagemPrivada(TCHAR *msg);
	/*Enviar a mensagem ao utilizador com quem mantemos a conversa
	privada. Caso o utilizador destinatário já tenha desligado (ou este)
	esta conversa, a função devolve um código de erro.*/
	DLL_IMP_API void EnviarMensagemPública(TCHAR *msg);
	/*Enviar a mensagem a todos os utilizadores online.*/
	DLL_IMP_API CHAT LerInformacaoInicial();
	/* Recebe toda informação histórica necessária para apresentação nas
	janelas de conversa pública e privada.*/
	DLL_IMP_API	MENSAGEM LerMensagensPublicas();
	/*Recebe informação autonomamente que é enviada pelo servidor e
	acrescenta no fundo da janela de conversa pública.*/
	DLL_IMP_API MENSAGEM LerMensagensPrivadas();
	/*Recebe informação autonomamente que é enviada pelo servidor e
	acrescenta no fundo da janela de conversa privada.*/
	//DLL_IMP_API int Sair();
	/* Envia o pedido de saída (logout) do sistema ao servidor. */
	//DLL_IMP_API int Desligar();
	/* Envia o pedido de encerramento do sistema ao servidor. */