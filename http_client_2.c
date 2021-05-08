/*
 ============================================================================
 Name        : http_client_2.c
 Author      : 19122017 KIMURA Hitoshi
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

 /*
  ============================================================================
  コピペがバレないように命名規則，構造，mainなどを改造します．
  ・とりあえずifdef DEBUGブロックを全部消す
  ============================================================================
  */

  /*
   ============================================================================
   実行例は適当につけてください
   ============================================================================
   */

#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>

#define BUFSIZE 1024    /* バッファサイズ */

int http(char* host, int port, char* req, char* res);
int search(char* pattern, char* text);

int main(int argc, char* argv[]) {
	if (argc != 2 && argc != 3) {
		printf("Invalid arguments.\n");
		return (EXIT_SUCCESS);
	}
	int port;
	char hostname[256];
	char res_buf[BUFSIZE];
	char httpRequest[BUFSIZE];
	int i;

	if (argc == 2) {
		i = 3 + search("://", argv[1]);
		int j = 0;
		while (argv[1][i] && argv[1][i] != '/' && j < BUFSIZE) {
			hostname[j++] = argv[1][i++];
		}
		hostname[j] = '\0';
		port = 80;
	}
	else {
		snprintf(hostname, BUFSIZE, "%s", argv[2]);
		port = atoi(argv[3]);
	}

	snprintf(httpRequest, BUFSIZE, "GET %s HTTP/1.1\r\nHost:%s\r\n", argv[1], hostname);
	if (http(hostname, port, httpRequest, res_buf)) {
		return (EXIT_FAILURE);
	}

	char servername[BUFSIZE];
	char contentLength[BUFSIZE];
	int locate_server = search("Server:", res_buf);
	int locate_content = search("Content-Length:", res_buf);
	if (locate_server == -1) {
		printf("Can't find \"Server:\"\n");
	}
	else {
		locate_server += 8;
		for (i = 0; res_buf[locate_server + i] != '\r'; ++i) {
			servername[i] = res_buf[locate_server + i];
		}
		servername[i] = '\0';
		printf("Server: %s\n", servername);
	}
	if (locate_content == -1) {
		printf("Can't find \"Content-Length:\"\n");
	}
	else {
		locate_content += 16;
		for (i = 0; res_buf[locate_content + i] != '\r'; ++i) {
			contentLength[i] = res_buf[locate_content + i];
		}
		contentLength[i] = '\0';
		printf("Content-Length: %s\n", contentLength);
	}

	return (EXIT_SUCCESS);
}

int http(char* host, int port, char* req, char* res) {
	struct hostent* server_host;
	struct sockaddr_in server_adrs;

	int tcpsock;

	char s_buf[BUFSIZE], r_buf[BUFSIZE];
	int strsize;

	/* サーバ名をアドレス(hostent構造体)に変換する */
	if ((server_host = gethostbyname(host)) == NULL) {
		printf("Couldn't resolve hostname.\n");
		return 1;
	}
	/* サーバの情報をsockaddr_in構造体に格納する */
	memset(&server_adrs, 0, sizeof(server_adrs));
	server_adrs.sin_family = AF_INET;
	server_adrs.sin_port = htons(port);
	memcpy(&server_adrs.sin_addr, server_host->h_addr_list[0], server_host->h_length);

	/* ソケットをSTREAMモードで作成する */
	if ((tcpsock = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
		printf("Socket generation failed.\n");
		return -1;
	}

	/* ソケットにサーバの情報を対応づけてサーバに接続する */
	if (connect(tcpsock, (struct sockaddr*)&server_adrs, sizeof(server_adrs)) == -1) {
		printf("Couldn't establish connection.\n");
		return 2;
	}

	snprintf(s_buf, BUFSIZE, "%s", req);
	strsize = strlen(s_buf);
	send(tcpsock, s_buf, strsize, 0);
	send(tcpsock, "\r\n", 2, 0); /* HTTPのメソッド（コマンド）の終わりは空行 */

	/* サーバから文字列を受信する */
	if ((strsize = recv(tcpsock, r_buf, BUFSIZE - 1, 0)) == -1) {
		printf("Couldn't receive response.\n");
		return 3;
	}
	r_buf[strsize] = '\0';
	snprintf(res, BUFSIZE, "%s", r_buf);

	close(tcpsock); /* ソケットを閉じる */
	return 0;
}

int search(char* pattern, char* text) {
	int t = 0;
	int p = 0;
	int i = 0;
	int pattern_len = strlen(pattern);
	int text_len = strlen(text);

	while (t <= text_len && p <= pattern_len) {
		t = i + p;
		if (text[t] == pattern[p]) {
			++p;
			t = i + p;
			while (text[t] == pattern[p]) {
				if (p == pattern_len - 1) {
					return (t - pattern_len + 1);
				}
				++p;
				t = i + p;
			}
			p = 0;
		}
		++i;
	}
	return -1;
}
