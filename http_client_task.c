/*
 実行例1
 $ ./http_client_task http://www.kit.ac.jp/  proxy.cis.kit.ac.jp 8080
 [Server name, Content-Length] = [Apache, 230]

 実行例2
 $ ./http_client_task http://www.chiseki.go.jp/
 [Server name, Content-Length] = [Apache, 13814]

 実行例3
 $ ./http_client_task http://www.yahoo.co.jp/
 [Server name, Content-Length] = [ATS, 1]
 [Server name, Content-Length] = [ATS, 6793]

 */

/*
 ============================================================================
 Name        : http_client_task.c
 Author      : 19122017 Hitoshi Kimura
 Version     :
 Copyright   :
 Description :
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
#define NAMSIZE 64

int exec_http(char *hostname, int hostport, char *req, char *res, int size);
int BMmatching(char *pattern, char *target, int startpos);

int main(int argc, char *argv[]) {
	if (argc < 2 || argc == 3 || argc > 4) {
		printf("E: Invalid arguments.\n");
		return (EXIT_SUCCESS);
	}
	char *targetURL;
	int port;
	char hostname[NAMSIZE];
	char res_buf[BUFSIZE];
	int i;

	if (argc == 2) {
		targetURL = argv[1];
		i = 3 + BMmatching("://", argv[1], 0);
		int j = 0;
		while (argv[1][i] && argv[1][i] != '/' && j < NAMSIZE) {
			hostname[j++] = argv[1][i++];
		}
		hostname[j] = '\0';
		port = 80;
	} else {
		targetURL = argv[1];
		snprintf(hostname, NAMSIZE, "%s", argv[2]);
		port = atoi(argv[3]);
	}
	char httpRequest[BUFSIZE];
	snprintf(httpRequest, BUFSIZE, "GET %s HTTP/1.1\r\nHost:%s\r\n\r\n", targetURL, hostname);	//HTTPリクエスト生成

#ifdef DEBUG
	fprintf(stderr, "main: host: %s\n", hostname);
	fprintf(stderr, "main: port: %d\n", port);
	fprintf(stderr, "main: targetURL: %s\n", targetURL);
#endif
	switch (exec_http(hostname, port, httpRequest, res_buf, BUFSIZE)) {
	case 0: //success
		break;
	case 1:	//err gethostbyname
		printf("E: Couldn't resolve hostname.\n");
		return (EXIT_FAILURE);
		break;
	case 2: //err connect
		printf("E: Couldn't establish connection.\n");
		return (EXIT_FAILURE);
		break;
	case 3: //struct pointer err
		printf("E: Couldn't receive response.\n");
		return (EXIT_FAILURE);
		break;
	case -1:
		printf("E: exec_http() err.\n");
		return (EXIT_FAILURE);
		break;
	default:
		break;
	}

#ifdef DEBUG
	fprintf(stderr, "main: ----HTTP Response----\n%s\n", res_buf);
#endif

//	以下、レスポンス解析部
	int server_start;
	int content_start;
	int locate_server;
	int locate_content;
	char servername[NAMSIZE];
	char content_length_str[NAMSIZE];
//	キーワードを検索
	locate_server = BMmatching("Server:", res_buf, 0);
	locate_content = BMmatching("Content-Length:", res_buf, 0);
#ifdef DEBUG
	fprintf(stderr, "main: [Server, Content-Length] keyword found at [%d, %d]\n", locate_server, locate_content);
#endif
//	サーバ名、コンテンツ長を転記
	if (locate_server == -1) {
		snprintf(servername, NAMSIZE, "Can't find \"Server:\"");
	} else {
		locate_server += 8;
		for (i = 0; res_buf[locate_server + i] != '\r'; ++i) {
			servername[i] = res_buf[locate_server + i];
		}
		servername[i] = '\0';
	}
	if (locate_content == -1) {
		snprintf(content_length_str, NAMSIZE, "Can't find \"Content-Length:\"");
	} else {
		locate_content += 16;
		for (i = 0; res_buf[locate_content + i] != '\r'; ++i) {
			content_length_str[i] = res_buf[locate_content + i];
		}
		content_length_str[i] = '\0';
	}
	printf("[Server name, Content-Length] = [%s, %s]\n", servername, content_length_str);
	server_start = locate_server + 8;
	content_start = locate_content + 16;
	while (((locate_server = BMmatching("Server:", res_buf, server_start)) != -1) &&
			((locate_content = BMmatching("Content-Length:", res_buf, content_start)) != 0)) {
#ifdef DEBUG
		fprintf(stderr, "main: [Server, Content-Length] keyword found at [%d, %d]\n", locate_server, locate_content);
#endif
		locate_server += 8;
		for (i = 0; res_buf[locate_server + i] != '\r'; ++i) {
			servername[i] = res_buf[locate_server + i];
		}
		servername[i] = '\0';
		locate_content += 16;
		for (i = 0; res_buf[locate_content + i] != '\r'; ++i) {
			content_length_str[i] = res_buf[locate_content + i];
		}
		content_length_str[i] = '\0';
		printf("[Server name, Content-Length] = [%s, %s]\n", servername, content_length_str);
		server_start = locate_server + 8;
		content_start = locate_content + 16;
	}

	return (EXIT_SUCCESS);
}

//	hostname　ホストドメイン名　hostport ホストポート番号　req httpリクエスト
int exec_http(char *hostname, int hostport, char *req, char *res, int size) {
	struct hostent *server_host;
	struct sockaddr_in server_adrs;

	int tcpsock;

	char s_buf[size], r_buf[size];
	int strsize;

	/* サーバ名をアドレス(hostent構造体)に変換する */
	if ((server_host = gethostbyname(hostname)) == NULL) {
#ifdef DEBUG
		fprintf(stderr, "exec_http: gethostbyname()\n");
#endif
		return 1;
	}
#ifdef DEBUG
	fprintf(stderr, "exec_http:	HOSTNAME:	%s\n", server_host->h_name);
	fprintf(stderr, "exec_http:	HTTP:\n%s\n", req);
#endif

	/* サーバの情報をsockaddr_in構造体に格納する */
	memset(&server_adrs, 0, sizeof(server_adrs));
	server_adrs.sin_family = AF_INET;
	server_adrs.sin_port = htons(hostport);
	memcpy(&server_adrs.sin_addr, server_host->h_addr_list[0], server_host->h_length);

	/* ソケットをSTREAMモードで作成する */
	if ((tcpsock = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
#ifdef DEBUG
		fprintf(stderr, "exec_http: socket()\n");
#endif
		return -1;
	}

	/* ソケットにサーバの情報を対応づけてサーバに接続する */
	if (connect(tcpsock, (struct sockaddr*) &server_adrs, sizeof(server_adrs)) == -1) {
#ifdef DEBUG
		fprintf(stderr, "exec_http: connect\n");
#endif
		return 2;
	}

	/*リクエスト送信*/
	snprintf(s_buf, size, "%s\r\n", req);
	strsize = strlen(s_buf);
	send(tcpsock, s_buf, strsize, 0);
	send(tcpsock, "\r\n", 2, 0); /* HTTPのメソッド（コマンド）の終わりは空行 */

	/* サーバから文字列を受信する */
	if ((strsize = recv(tcpsock, r_buf, size - 1, 0)) == -1) {
#ifdef DEBUG
		fprintf(stderr, "exec_http: recv()\n");
#endif
		return 3;
	}
	r_buf[strsize] = '\0';
	snprintf(res, size, "%s", r_buf);

	close(tcpsock); /* ソケットを閉じる */
	return 0;
}

// 返却 = 一致が見つかったとき、一致した場所の先頭, 一致しなかったら-1
int BMmatching(char *pattern, char *target, int startpos) {
//	string.hが必要
	int i;
	int d[256];
	int pattern_len = strlen(pattern);
	int target_len = strlen(target);

	//	シフト量表初期化
	for (i = 0; i < 256; i++) {
		d[i] = pattern_len;
	}
	//	シフト量表作成
	for (i = 0; i < pattern_len - 1; i++) {
		d[(int) pattern[i]] = pattern_len - i - 1;
	}

	//	テスト用
//	for (i = '!'; i < '{'; i++) {
//		printf("%c %d\n", i, d[i]);
//	}

	//	k = パターンの末尾位置	dp = 比較位置のkからの移動量
	//	t 添字の0スタートを考慮したtarget側の探索位置 p 添字の0スタートを考慮したpattern側の探索位置
	int k = startpos + pattern_len - 1;
	int dp = 0;
	int t, p;
	while (k <= target_len) {
//		tとpを設定
		t = k - 1 - dp;
		p = pattern_len - 1 - dp;
//		比較判定
		if (target[t] != pattern[p]) {
			if (target[t] < 0 || target[t] > 255) {	//TODO	target中でascii値域外を踏んだときの処理。要検討。
				k = k - dp + pattern_len;
			}
			else {
				if ((d[(int) target[t]] - dp) <= 0) {
					++k;
				}
				else {
					k = k - dp + d[(int) target[t]];
				}
			}
			dp = 0;
			continue;
		}

		if (dp == (pattern_len - 1)) {
			return t;
		}
		dp++;
	}
	return -1;
}

