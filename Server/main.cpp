#include <WinSock2.h>
#include <WS2tcpip.h>

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <ctype.h>

#define PORT "12345"

void Exit();

int main() {
	atexit(Exit);

	// Initialize Winsock
	WSADATA wsaData;
	int startupError = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (startupError != 0) {
		printf("WSAStartup() failed: %d\n", startupError);
		exit(1);
	}

	// Provide address information for socket creation
	addrinfo hints{}, * addressInfo;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;
	int getaddrError = getaddrinfo(nullptr, PORT, &hints, &addressInfo);
	if (getaddrError != 0) {
		printf("getaddrinfo() failed: %d\n", getaddrError);
		exit(1);
	}

	// Create the socket using the same information
	SOCKET serverSocket = socket(hints.ai_family, hints.ai_socktype, hints.ai_protocol);
	if (serverSocket == INVALID_SOCKET) {
		printf("socket() failed: %d\n", WSAGetLastError());
		exit(1);
	}

	// Bind socket to actual address returned from getaddrinfo
	int bindError = bind(serverSocket, addressInfo->ai_addr, addressInfo->ai_addrlen);
	if (bindError == SOCKET_ERROR) {
		printf("bind() failed: %d\n", WSAGetLastError());
		closesocket(serverSocket);
		exit(1);
	}

	// Free up memory allocated to create addressInfo
	freeaddrinfo(addressInfo);

	// Set socket state for listening
	int listenError = listen(serverSocket, SOMAXCONN);
	if (listenError == SOCKET_ERROR) {
		printf("listen() failed: %d\n", WSAGetLastError());
		closesocket(serverSocket);
		exit(1);
	}

	// Wait for and accept an incoming connection
	// NOTE: accept blocks, meaning it will pause the program
	SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
	if (clientSocket == INVALID_SOCKET) {
		printf("accept() failed: %d\n", WSAGetLastError());
		closesocket(serverSocket);
		exit(1);
	}

//===========================
//  GAME LOGIC
//===========================

// Max length of strings
#define MAX 100 

	// Prompt server user for a word
	printf("Phrase: ");
	char word[MAX]{};
	std::cin.getline(word, MAX);

	// Prompt server user for a hint
	printf("Hint: ");
	char category[MAX]{};
	std::cin.getline(category, MAX);

	// Send the hint to client
	int sendError = send(clientSocket, category, MAX, NULL);
	if (sendError == SOCKET_ERROR) {
		printf("send() failed: %d\n", WSAGetLastError());
		closesocket(clientSocket);
		closesocket(serverSocket);
		exit(1);
	}

	// Encode word; will be sent to client
	char message[MAX]{};
	for (int c = 0; c < MAX; c++) {
		if (word[c] == '\0')
			break;

		if (word[c] == ' ')
			message[c] = ' ';
		else if (!isalpha(word[c]))
			message[c] = word[c];
		else
			message[c] = '*';
	}

	// Use to store client guess
	char guess[MAX]{};

	// Give player lives, partially based on length of phrase
	int lives = strnlen(word, MAX) / 3;
	if (lives < 10) lives = 10;

	// Enter game loop
	while (true) {
		// Print client's guess, lives left, and encoded/decoded message
		printf("Guess: %s  Tries left: %d\n%s\n\n", guess, lives, message);

		// Send encoded/decoded message to client
		int sendError = send(clientSocket, message, MAX, NULL);
		if (sendError == SOCKET_ERROR) {
			printf("send() failed: %d\n", WSAGetLastError());
			closesocket(clientSocket);
			closesocket(serverSocket);
			exit(1);
		}

		// End game
		if (lives <= 0)
			break;

		// Wait for client to send a message that can be received
		// NOTE: recv blocks
		int recvError = recv(clientSocket, guess, MAX, NULL);
		if (recvError == SOCKET_ERROR) {
			printf("recv() failed: %d\n", WSAGetLastError());
			closesocket(clientSocket);
			closesocket(serverSocket);
			exit(1);
		}

		// If opponent only guesses a letter
		if (strnlen(guess, MAX) == 1) {
			for (int c = 0; c < MAX; c++) {
				if (word[c] == *guess || tolower(word[c]) == tolower(*guess)) {
					message[c] = word[c];
				}
			}
		}

		// Decrement lives only if guess is valid
		if (strnlen(guess, MAX) == 1 || strnlen(guess, MAX) == strnlen(word, MAX)) {
			lives--;
		}

		// If client runs out of lives, you win...
		if (lives <= 0) {
			strncpy_s(message, "You won!", MAX);
		}
		// ...Unless they guessed the word right on the last try!
		if (strncmp(guess, word, MAX) == 0) {
			strncpy_s(message, "They won!", MAX);
			lives = 0; // Set this to make the game end
		}
	}

	// Clean up Winsock
	closesocket(clientSocket);
	closesocket(serverSocket);
	exit(0);
}

void Exit() {
	WSACleanup();
	system("pause");
}