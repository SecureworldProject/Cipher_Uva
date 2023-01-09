#define _CRT_SECURE_NO_WARNINGS
// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include <stdio.h>
#include <stdint.h>
#include "context_cipher.h"
#include <math.h>

struct Cipher* cipher_data;

//Tamaño para representar decimalmente 2^32 posiciones: 10 digitos. Key size es un int (4bytes). (2^64 = 16 trillones (18 ceros), 24 bits para concat)
#define CONCAT_TAM 14
#define NOOP ((void)0)
#define ENABLE_PRINTS 0					// Affects the PRINT() functions. If 0 does not print anything. If 1 traces are printed.
#define PRINT(...) do { if (ENABLE_PRINTS) printf(__VA_ARGS__); else NOOP;} while (0)
#define PRINT1(...) PRINT("    "); PRINT(__VA_ARGS__)
#define PRINT2(...) PRINT("        "); PRINT(__VA_ARGS__)
#define PRINT3(...) PRINT("            "); PRINT(__VA_ARGS__)
#define PRINT4(...) PRINT("                "); PRINT(__VA_ARGS__)
#define PRINT5(...) PRINT("                    "); PRINT(__VA_ARGS__)
#define PRINTX(DEPTH, ...) do { if (ENABLE_PRINTS) { for (int x=0; x<DEPTH; x++){ printf("    ");} printf(__VA_ARGS__); } else NOOP;} while (0)
#define PRINT_HEX(BUF, BUF_SIZE) print_hex(#BUF, BUF, BUF_SIZE);

DWORD print_hex(char* buf_name, void* buf, int size) {
    if (ENABLE_PRINTS) {
        //printf("First %d bytes of %s contain:\n", size, buf_name);

        //byte [size*3 + size/32 + 1] str_fmt_buf;
        char* full_str = NULL;
        char* target_str = NULL;
        //int total = 0;

        // Size of string will consist on:
        //   (size*3)			 - 3 characters for every byte (2 hex characters plus 1 space). Space changed for '\n' every 32 bytes
        //   (size/8 - size/32)	 - Every 8 bytes another space is added after the space (if it is not multiple of 32, which already has '\n' instead)
        //   (1)				 - A '\n' is added at the end
        //full_str = static_cast <char*> (calloc((size * 3) + (size / 8 - size / 32) + (1), sizeof(char)));
        if (full_str == NULL) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        target_str = full_str;

        for (int i = 0; i < size; i++) {
            if ((i + 1) % 32 == 0) {
                target_str += sprintf(target_str, "%02hhX\n", ((byte*)buf)[i]);
            }
            else if ((i + 1) % 8 == 0) {
                target_str += sprintf(target_str, "%02hhX  ", ((byte*)buf)[i]);
            }
            else {
                target_str += sprintf(target_str, "%02hhX ", ((byte*)buf)[i]);
            }
        }
        target_str += sprintf(target_str, "\n");
        printf(full_str);
        //free(full_str);
    }
    return ERROR_SUCCESS;
}

//Function prototypes
extern "C" _declspec(dllexport) int init(struct Cipher* cipher_data_param);
extern "C" _declspec(dllexport) int cipher(LPVOID out_buf, LPCVOID in_buf, DWORD size, size_t offset, struct KeyData* key);
extern "C" _declspec(dllexport) int decipher(LPVOID out_buf, LPCVOID in_buf, DWORD size, size_t offset, struct KeyData* key);

int init(struct Cipher* cipher_data_param) {
    cipher_data = cipher_data_param;
    printf("Initializing (%ws)\n", cipher_data->file_name);

    return 0;
}
int cipher(LPVOID out_buf, LPCVOID in_buf, DWORD size, size_t offset, struct KeyData* key) { //offset es la posicion en el fichero, hacerlo bien que es la posicion que tengo que cifrar
    printf("Ciphering (%ws)\n", cipher_data->file_name);
    byte result;
	int message[11] = { 0 };
	size_t key_p[100] = { 0 };
	int digitos[100] = { 0 };
	int i = 0;
	int F = 0;
	int G = 0;
	int H = 0;
	int I = 0;

	//AQUI OBTENGO UN ARRAY DE LOS DIGITOS DE FRN y KEY de 1 en 1 para tener más variación a la hora de la rotación 
	/*while (frn_p > 0) {

		digitos[i] = frn_p % 1000;
		frn_p = frn_p / 1000;
		PRINT("Digitos de FRN: %i\n", digitos[i]);
		PRINT("Contador de digitos FRN: %i\n", i + 1);

		i++;
	}*/
	for (int x = 0; x <= key->size; x++) {
		key_p [x] = key->data[x];
		while (key_p[x] > 0) {

			digitos[i] = key_p[x] % 10;
			key_p[x] = key_p[x] / 10;
			PRINT("Digitos de KEY: %i\n", digitos[i]);
			//PRINT("Contador de digitos KEY: %i\n", i + 1);

			i++;
		}
	}
	
	

	for (size_t pos = 0; pos <= size; pos++) {

		int idx_i = i; // cantidad de digitos de la key+frn
		size_t pos_p = (pos % 1000000); // mODULO 10000 PARA QUE NO EXCEDA LOS 6 DIGITOS EN EL ARRAY

		//Meto la pos en el message
		int len_message_real = 0;


		if (pos == 0) { // PARA LA POS 0:
			digitos[idx_i] = 0;

			//PRINT("Digitos de POS cuando es 0: %i\n", digitos[idx_i]);
			//PRINT("Contador de digitos POS cuando es 0: %i\n", idx_i + 1);

			idx_i++;
		}
		else { // PARA POS MAYOR QUE 0:


			while (pos_p > 0) {

				digitos[idx_i] = pos_p % 10;
				pos_p = pos_p / 10;
				//PRINT("Digitos de POS: %i\n", digitos[idx_i]);
				//PRINT("Contador de digitos POS: %i\n", idx_i + 1);
				idx_i++;
			}

		}
		int digitos_r[100] = { 0 }; // ARRAY DE DIGITOS ROTADOS 
		len_message_real = idx_i; // cantidad de digitos de la key+frn+POS (MESSAGE TOTAL)

		//ROTANDO EL MESSAGE PARA CONSTRUIR NUEVOS NUMEROS DE 3 DIGITOS CADA VEZ
		for (int j = 0; j < len_message_real; j++) {
			if (j + (pos % len_message_real) < len_message_real) {
				digitos_r[j] = (digitos[j + (pos % (len_message_real))]);

			}
			else {
				digitos_r[j] = digitos[j + (pos % (len_message_real)) - len_message_real];

			}
			digitos[j] = digitos_r[j];//Guardo las posiciones para partir de ahí en el proximo ciclo
			//PRINT("Digitos reordenados: %i , %i\n", digitos_r[j], j );

		}


		int numeroentero[11][3]; // Array de 33 digitos (3 digitos (columnas) por cada fila y 11 filas)

		int c = 0;
		int cont = 0;
		c = len_message_real / 3; //cantidad de numeros de 3 digitos que tiene el message


		//PRINT("c: %i\n", c);

		//Relleneando para hacer 11 variables de 3 digitos.

		for (int p = 0; p < 11; p++) { //relleno el primer array de 33 digitos
			int A = 0;
			for (int j = 0; j < 3; j++) {

				numeroentero[p][j] = digitos_r[cont];
				A += numeroentero[p][j] * (pow(10, j));
				cont++;
				if (cont == len_message_real)
					cont = 0; //reinicio el contador si llega al final del mensaje para repetir la secuencia de bits desde la entrada hasta que alcance los 33 digitos
				//PRINT("numeroentero: %i, %i, %i\n", numeroentero[p][j], j, p);
			}

			message[p] = A % 256; // relleno el message con 11 números de 3 digitos
			//PRINT("message[%i]==== %i\n", p, message[p]);

		}

		//PRINT("He llenado hasta %d\n", len_message_real);

		if (c > 11) { //SI TIENE MÁS DE 33 DIGITOS 
			int aux[11][3];

			for (int k = 1; k <= (c / 11); k++) {
				for (int p = 0; p < 11; p++) {
					int A = 0;
					for (int j = 0; j < 3; j++) {
						//int p = pow(10, j);

						aux[p][j] = numeroentero[p][j] ^ digitos_r[cont] % 10; // SE RELLENA UN SEGUNDO ARRAY CON LO QUE RESTE HASTA 33 DIGITOS 
						PRINT("numeroentero sin xor: %i, %i, %i\n", numeroentero[p][j], j, p);
						PRINT("digitos_r[%i]==== %i\n", cont, digitos_r[cont]);
						PRINT("aux: %i, %i, %i\n", aux[p][j], j, p);
						numeroentero[p][j] = aux[p][j]; //SE HACE XOR DE CADA POSICION DEL PRIMER ARRAY CON SU POSICIÓN CORRESPONDIENTE EN EL SEGUNDO.
						cont++;
						if (cont == len_message_real)
							cont = 0; // sE INICIALIZA EL CONTADOR SI LLEGA AL TOTAL DEL MENSAJE PARA COMPLETAR EL ARRAY CON 33 DIGITOS

						A += numeroentero[p][j] * (pow(10, j));
						//PRINT("A: %i, %i, %i\n", A, j, p);
						//PRINT("aux: %i, %i, %i\n", aux[p][j], j, p);
						//PRINT("numeroentero co xor: %i, %i, %i\n", numeroentero[p][j], j, p);
					}
					message[p] = A % 256;
					//PRINT("message cifrado[%i]==== %i\n", p, message[p]);//SE RELLENA EL message CON 11 NUMEROS de 3 digitos, modulo 256.
					PRINT("POS=== %i\n", pos);
					//PRINT("numeroentero[p]: %i\n", numeroentero[p]);

				}
			}

		}




		//confusion: 

		F = (message[0] & message[1]) | (~message[2] & message[3]);
		//PRINT("F==== %i\n", F);
		G = (F & message[4]) | (message[5] & ~message[6]);
		//PRINT("G==== %i\n", G);
		H = (G ^ message[7] ^ message[8]);
		//PRINT("H==== %i\n", H);
		I = (H ^ (message[9] | ~message[10]));
		//PRINT("I==== %i\n", I);
		int resultado = 0;
		if (I < 0) {
			resultado = (I * -1) % 256;//PARA EVITAR LOS NUMEROS NEGATIVOS

		}
		else {
			resultado = I % 256;
		}

	
		
		result = resultado & 0xFF;//¿Cambia a byte?
		//PRINT("result=%x\n", (byte)result);

		//byte_claro = byte_claro + byte_ciphered
		//ciphered[pos] = (plain[pos] + resultado) % 256;
		((byte*)out_buf)[pos] = (((byte*)in_buf)[pos] + result);
	}






    //char concat[CONCAT_TAM];
    /*byte my_key = 0;
    for (size_t i = 0; i < key->size; i++) {
        my_key += key->data[i];
    }*/
    /*for (size_t i = 0; i < size; i++) {
        snprintf(concat, CONCAT_TAM, "%d%s", i + offset, key->data);// Ahora mismo no hace padding con 0s
        printf("Concat = %s\n",concat);
        result = md5String(concat)[15];
        //printf("Result: %d",result);
        ((byte*)out_buf)[i] = (((byte*)in_buf)[i]+result);
        //free(result);
    }*/
    printf("Buffer ciphered");
    return 0;
}

/*int cipher_old(LPVOID out_buf, LPCVOID in_buf, DWORD size, size_t offset, struct KeyData* key) { //offset es la posicion en el fichero, hacerlo bien que es la posicion que tengo que cifrar
    printf("Bienvenido al cifrador\n");
    printf("Ciphering (%ws)\n", cipher_data->file_name);
    byte* result;
    char concat[CONCAT_TAM];
    //int total = 0;
    byte total;
    //DWORD pos_max = size + offset;
    
    printf("A continuacion se cifra todo el buffer\n");
    /*byte my_key = 0;
    for (size_t i = 0; i < key->size; i++) {
        my_key += key->data[i];
    }
    for (int i = 0; i < size; i++) {
        snprintf(concat, CONCAT_TAM, "%d%s", i+offset, key->data);// Ahora mismo no hace padding con 0s
        result = md5String(concat);
       /* for (int j = 0; j < 16; j++) { 
            total += result[j];
        }
        total = result[15];
        //total = (((int*)in_buf)[i] + total) % 256;
        //total = (((byte*)in_buf)[i] + total);
        //((char*)out_buf)[i] = (byte)total;
        //((byte*)out_buf)[i] = (byte)total;
        ((byte*)out_buf)[i] = (((byte*)in_buf)[i] + total);
        free(result);
    }
    printf("Cifrado terminado");
    return 0;
}*/

int decipher(LPVOID out_buf, LPCVOID in_buf, DWORD size, size_t offset, struct KeyData* key) {
    printf("Deciphering (%ws)\n", cipher_data->file_name);
	byte result;
	int message[11] = { 0 };
	size_t key_p[100] = { 0 };
	int digitos[100] = { 0 };
	int i = 0;
	int F = 0;
	int G = 0;
	int H = 0;
	int I = 0;

	//AQUI OBTENGO UN ARRAY DE LOS DIGITOS DE FRN y KEY de 1 en 1 para tener más variación a la hora de la rotación 
	/*while (frn_p > 0) {

		digitos[i] = frn_p % 1000;
		frn_p = frn_p / 1000;
		PRINT("Digitos de FRN: %i\n", digitos[i]);
		PRINT("Contador de digitos FRN: %i\n", i + 1);

		i++;
	}*/
	for (int x = 0; x <= key->size; x++) {
		key_p[x] = key->data[x];
		while (key_p[x] > 0) {

			digitos[i] = key_p[x] % 10;
			key_p[x] = key_p[x] / 10;
			//PRINT("Digitos de KEY: %i\n", digitos[i]);
			//PRINT("Contador de digitos KEY: %i\n", i + 1);

			i++;
		}
	}



	for (size_t pos = 0; pos <= size; pos++) {

		int idx_i = i; // cantidad de digitos de la key+frn
		size_t pos_p = (pos % 1000000); // mODULO 10000 PARA QUE NO EXCEDA LOS 6 DIGITOS EN EL ARRAY

		//Meto la pos en el message
		int len_message_real = 0;


		if (pos == 0) { // PARA LA POS 0:
			digitos[idx_i] = 0;

			//PRINT("Digitos de POS cuando es 0: %i\n", digitos[idx_i]);
			//PRINT("Contador de digitos POS cuando es 0: %i\n", idx_i + 1);

			idx_i++;
		}
		else { // PARA POS MAYOR QUE 0:


			while (pos_p > 0) {

				digitos[idx_i] = pos_p % 10;
				pos_p = pos_p / 10;
				//PRINT("Digitos de POS: %i\n", digitos[idx_i]);
				//PRINT("Contador de digitos POS: %i\n", idx_i + 1);
				idx_i++;
			}

		}
		int digitos_r[100] = { 0 }; // ARRAY DE DIGITOS ROTADOS 
		len_message_real = idx_i; // cantidad de digitos de la key+frn+POS (MESSAGE TOTAL)

		//ROTANDO EL MESSAGE PARA CONSTRUIR NUEVOS NUMEROS DE 3 DIGITOS CADA VEZ
		for (int j = 0; j < len_message_real; j++) {
			if (j + (pos % len_message_real) < len_message_real) {
				digitos_r[j] = (digitos[j + (pos % (len_message_real))]);

			}
			else {
				digitos_r[j] = digitos[j + (pos % (len_message_real)) - len_message_real];

			}
			digitos[j] = digitos_r[j];//Guardo las posiciones para partir de ahí en el proximo ciclo
			//PRINT("Digitos reordenados: %i , %i\n", digitos_r[j], j );

		}


		int numeroentero[11][3]; // Array de 33 digitos (3 digitos (columnas) por cada fila y 11 filas)

		int c = 0;
		int cont = 0;
		c = len_message_real / 3; //cantidad de numeros de 3 digitos que tiene el message


		//PRINT("c: %i\n", c);

		//Relleneando para hacer 11 variables de 3 digitos.

		for (int p = 0; p < 11; p++) { //relleno el primer array de 33 digitos
			int A = 0;
			for (int j = 0; j < 3; j++) {

				numeroentero[p][j] = digitos_r[cont];
				A += numeroentero[p][j] * (pow(10, j));
				cont++;
				if (cont == len_message_real)
					cont = 0; //reinicio el contador si llega al final del mensaje para repetir la secuencia de bits desde la entrada hasta que alcance los 33 digitos
				//PRINT("numeroentero: %i, %i, %i\n", numeroentero[p][j], j, p);
			}

			message[p] = A % 256; // relleno el message con 11 números de 3 digitos
			//PRINT("message[%i]==== %i\n", p, message[p]);

		}

		//PRINT("He llenado hasta %d\n", len_message_real);

		if (c > 11) { //SI TIENE MÁS DE 33 DIGITOS 
			int aux[11][3];

			for (int k = 1; k <= (c / 11); k++) {
				for (int p = 0; p < 11; p++) {
					int A = 0;
					for (int j = 0; j < 3; j++) {
						//int p = pow(10, j);

						aux[p][j] = numeroentero[p][j] ^ digitos_r[cont] % 10; // SE RELLENA UN SEGUNDO ARRAY CON LO QUE RESTE HASTA 33 DIGITOS 
						PRINT("numeroentero sin xor: %i, %i, %i\n", numeroentero[p][j], j, p);
						PRINT("digitos_r[%i]==== %i\n", cont, digitos_r[cont]);
						PRINT("aux: %i, %i, %i\n", aux[p][j], j, p);
						numeroentero[p][j] = aux[p][j]; //SE HACE XOR DE CADA POSICION DEL PRIMER ARRAY CON SU POSICIÓN CORRESPONDIENTE EN EL SEGUNDO.
						cont++;
						if (cont == len_message_real)
							cont = 0; // sE INICIALIZA EL CONTADOR SI LLEGA AL TOTAL DEL MENSAJE PARA COMPLETAR EL ARRAY CON 33 DIGITOS

						A += numeroentero[p][j] * (pow(10, j));
						//PRINT("A: %i, %i, %i\n", A, j, p);
						//PRINT("aux: %i, %i, %i\n", aux[p][j], j, p);
						//PRINT("numeroentero co xor: %i, %i, %i\n", numeroentero[p][j], j, p);
					}
					message[p] = A % 256;
					//PRINT("message cifrado[%i]==== %i\n", p, message[p]);//SE RELLENA EL message CON 11 NUMEROS de 3 digitos, modulo 256.
					PRINT("POS=== %i\n", pos);
					//PRINT("numeroentero[p]: %i\n", numeroentero[p]);

				}
			}

		}




		//confusion: 

		F = (message[0] & message[1]) | (~message[2] & message[3]);
		//PRINT("F==== %i\n", F);
		G = (F & message[4]) | (message[5] & ~message[6]);
		//PRINT("G==== %i\n", G);
		H = (G ^ message[7] ^ message[8]);
		//PRINT("H==== %i\n", H);
		I = (H ^ (message[9] | ~message[10]));
		//PRINT("I==== %i\n", I);
		int resultado = 0;
		if (I < 0) {
			resultado = (I * -1) % 256;//PARA EVITAR LOS NUMEROS NEGATIVOS

		}
		else {
			resultado = I % 256;
		}


		result = resultado & 0xFF;//¿Cambia a byte?
		//PRINT("result=%x\n", (byte)result);

		//byte_claro = byte_claro + byte_ciphered
		//ciphered[pos] = (plain[pos] + resultado) % 256;
		((byte*)out_buf)[pos] = (((byte*)in_buf)[pos] - result);
	}
        
   
    printf("Buffer deciphered");
    return 0;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

