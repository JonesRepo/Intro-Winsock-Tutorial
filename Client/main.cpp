#include <WinSock2.h>
#include <WS2tcpip.h>

#include <stdio.h>
#include <iostream>

#define PORT "12345"

void Exit();

int main() {
	atexit(Exit);

	// Initialize Winsock
	WSADATA wsaData;
	int startupError = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (startupError != 0) {
		printf("WSAStartup() failed: %d\n", startupError);
		system("pause");
		return 1;
	}

	// Provide address information for socket creation
	addrinfo hints{}, * addressInfo;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	int getaddrError = getaddrinfo(nullptr, PORT, &hints, &addressInfo);
	if (getaddrError != 0) {
		printf("getaddrinfo() failed: %d\n", getaddrError);
		exit(1);
	}

	// Create the socket using the same information
	SOCKET clientSocket = socket(hints.ai_family, hints.ai_socktype, hints.ai_protocol);
	if (clientSocket == INVALID_SOCKET) {
		printf("socket() failed: %d\n", WSAGetLastError());
		exit(1);
	}

	// Try to connect socket to server; will fail shortly if none found
	// NOTE: connect blocks, meaning it will pause the program
	int connectError = connect(clientSocket, addressInfo->ai_addr, addressInfo->ai_addrlen);
	if (connectError == SOCKET_ERROR) {
		printf("connect() failed: %d\n", WSAGetLastError());
		closesocket(clientSocket);
		exit(1);
	}

	// Free up memory allocated to create addressInfo
	freeaddrinfo(addressInfo);

//===========================
//  GAME LOGIC
//===========================
// Max length of strings
#define MAX 100

	// Use to store messages from server
	char message[MAX]{};

	// Indicates player is connected and opponent is doing something
	printf("Opponent writing phrase...\n\n");

	// This message contains the phrase hint
	int recvError = recv(clientSocket, message, MAX, NULL);
	if (recvError == SOCKET_ERROR) {
		printf("recv() failed: %d\n", WSAGetLastError());
		closesocket(clientSocket);
		exit(1);
	}

	// Display hint to player
	printf("%s\n\n", message);

	// Use to store player guesses
	char guess[MAX]{};

	// Will set number of lives dynamically after phrase is received
	int lives = -1;

	// Enter game loop
	while (true) {
		// Wait for server to send the encoded message 
		// NOTE: recv blocks
		int recvError = recv(clientSocket, message, MAX, NULL);
		if (recvError == SOCKET_ERROR) {
			printf("recv() failed: %d\n", WSAGetLastError());
			closesocket(clientSocket);
			exit(1);
		}

		// If lives hasn't been defined, set it's initial value dynamically
		if (lives == -1) {
			lives = strnlen(message, MAX) / 3;
			if (lives < 10) lives = 10;
		}

		// If the message indicates a victor, print victor accordingly (opposite of message)
		if (strncmp(message, "They won!", MAX) == 0) {
			printf("You won!\n\n");
			break;
		}
		if (strncmp(message, "You won!", MAX) == 0) {
			printf("They won!\n\n");
			break;
		}

		// Indicate how many lives are left
		printf("Tries left: %d\n%s\n\n", lives, message);

		// Prompt player for guess
		std::cin.getline(guess, MAX);

		// Decrement lives, only if guess is valid
		if (strnlen(guess, MAX) == 1 || strnlen(guess, MAX) == strnlen(message, MAX)) {
			lives--;
		}

		// Send the guess to server
		int sendError = send(clientSocket, guess, MAX, NULL);
		if (sendError == SOCKET_ERROR) {
			printf("send() failed: %d\n", WSAGetLastError());
			closesocket(clientSocket);
			exit(1);
		}
	}

	// Clean up Winsock
	closesocket(clientSocket);
	exit(0);
}

void Exit() {
	WSACleanup();
	system("pause");
}