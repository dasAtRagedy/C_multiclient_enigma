/* The Enigma machines were a family of portable cipher machines
with rotor scramblers. Good operating procedures, properly enforced,
would have made the cipher unbreakable.
However, most of the German armed and secret services and civilian agencies
that used Enigma employed poor procedures and it was these that allowed the
cipher to be broken.

The German plugboard-equipped Enigma became the Third Reich's
principal crypto-system. It was reconstructed by the Polish
General Staff's Cipher Bureau in December 1932 with the aid of
French-supplied intelligence material that had been obtained
from a German spy. Shortly before the outbreak of World War II,
the Polish Cipher Bureau initiated the French and British into its
Enigma-breaking techniques and technology at a conference held in Warsaw.

From this beginning, the British Government Code and Cypher School at
Bletchley Park built up an extensive cryptanalytic facility. Initially,
the decryption was mainly of Luftwaffe and a few Army messages,
as the German Navy employed much more secure procedures for using Enigma.

Alan Turing, a Cambridge University mathematician and logician,
provided much of the original thinking that led to the design of
the cryptanalytical Bombe machines, and the eventual breaking of naval Enigma.
However, when the German Navy introduced an Enigma version with a
fourth rotor for its U-boats, there was a prolonged period when those messages
could not be decrypted. With the capture of relevant cipher keys and the use
of much faster U.S. Navy Bombes, regular, rapid reading of
German naval messages resumed.

The rotors (alternatively wheels or drums, Walzen in German)
formed the heart of an Enigma machine. Each rotor was a disc
approximately 10 cm (3.9 in) in diameter made from hard rubber
or bakelite with brass spring-loaded pins on one face arranged
in a circle; on the other side are a corresponding number
of circular electrical contacts. The pins and contacts represent
the alphabet, typically the 26 letters A to Z.

Setting Wiring                      Notch   Window  Turnover
Base    ABCDEFGHIJKLMNOPQRSTUVWXYZ
I       EKMFLGDQVZNTOWYHXUSPAIBRCJ  Y       Q       R
II      AJDKSIRUXBLHWTMCQGZNPYFVOE  M       E       F
III     BDFHJLCPRTXVZNYEIWGAKMUSQO  D       V       W
IV      ESOVPZJAYQUIRHXLNFTGKDCMWB  R       J       K
V       VZBRGITYUPSDNHLXAWMJQOFECK  H       Z       A
VI      JPGVOUMFYQBENHZRDKASXLICTW  H/U     Z/M     A/N
VII     NZJHGRCXMYSWBOUFAIVLPEKQDT  H/U     Z/M     A/N
VIII    FKQHTLXOCBJSPDZRAMEWNIUYGV  H/U     Z/M     A/N

With the exception of the early Enigma models A and B,
the last rotor came before a reflector (German: Umkehrwalze,
meaning reversal rotor), a patented feature distinctive of the
Enigma family amongst the various rotor machines designed
in the period. The reflector connected outputs of the
last rotor in pairs, redirecting current back through the
rotors by a different route. The reflector ensured that
Enigma is self-reciprocal: conveniently, encryption was
the same as decryption. However, the reflector also gave
Enigma the property that no letter ever encrypted to itself.
This was a severe conceptual flaw and a cryptological mistake
subsequently exploited by codebreakers.

Setting     Wiring
Base        ABCDEFGHIJKLMNOPQRSTUVWXYZ
A           EJMZALYXVBWFCRQUONTSPIKHGD
B           YRUHQSLDPXNGOKMIEBFZCWVJAT
C           FVPJIAOYEDRZXWGCTKUQSBNMHL
*/
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define ROTATE 26

#include "enigma.h"

const char *alpha = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
const char *rotor_ciphers[] = {
    "EKMFLGDQVZNTOWYHXUSPAIBRCJ",
    "AJDKSIRUXBLHWTMCQGZNPYFVOE",
    "BDFHJLCPRTXVZNYEIWGAKMUSQO",
    "ESOVPZJAYQUIRHXLNFTGKDCMWB",
    "VZBRGITYUPSDNHLXAWMJQOFECK",
    "JPGVOUMFYQBENHZRDKASXLICTW",
    "NZJHGRCXMYSWBOUFAIVLPEKQDT",
    "FKQHTLXOCBJSPDZRAMEWNIUYGV"
};

const char *rotor_notches[] = {"Q", "E", "V", "J", "Z", "ZM", "ZM", "ZM"};

const char *rotor_turnovers[] = {"R", "F", "W", "K", "A", "AN", "AN", "AN"};

const char *reflectors[] = {
    "EJMZALYXVBWFCRQUONTSPIKHGD",
    "YRUHQSLDPXNGOKMIEBFZCWVJAT",
    "FVPJIAOYEDRZXWGCTKUQSBNMHL"
};

/*
 * Produce a rotor object
 * Setup the correct offset, cipher set and turn overs.
 */
 /*
struct Rotor new_rotor(struct Enigma *machine, int rotornumber, int offset) {
    struct Rotor r;
    r.offset = offset;
    r.turnnext = 0;
    r.cipher = rotor_ciphers[rotornumber - 1];
    r.turnover = rotor_turnovers[rotornumber - 1];
    r.notch = rotor_notches[rotornumber - 1];
    machine->numrotors++;

    return r;
}//*/

/*
 * Return the req_index position of a character inside a string
 * if not found then -1
 **/
int str_index(const char *str, int character) {
    char *pos;
    int req_index;
    pos = strchr(str, character);

    // pointer arithmetic
    if (pos){
        req_index = (int) (pos - str);
    } else {
        req_index = -1;
    }

    return req_index;
}

/*
 * Cycle a rotor's offset but keep it in the array.
 */
void rotor_cycle(struct Rotor *rotor) {
    rotor->offset++;
    rotor->offset = rotor->offset % ROTATE;

    // Check if the notch is active, if so trigger the turnnext
    if(str_index(rotor->turnover, alpha[rotor->offset]) >= 0) {
        rotor->turnnext = 1;
    }
}

/*
 * Pass through a rotor, right to left, cipher to alpha.
 * returns the exit req_index location.
 */
int rotor_forward(struct Rotor *rotor, int req_index) {

    // In the cipher side, out the alpha side
    req_index = (req_index + rotor->offset) % ROTATE;
    req_index = str_index(alpha, rotor->cipher[req_index]);
    req_index = (ROTATE + req_index - rotor->offset) % ROTATE;

    return req_index;
}

/*
 * Pass through a rotor, left to right, alpha to cipher.
 * returns the exit req_index location.
 */
int rotor_reverse(struct Rotor *rotor, int req_index) {

    // In the cipher side, out the alpha side
    req_index = (req_index + rotor->offset) % ROTATE;
    req_index = str_index(rotor->cipher, alpha[req_index]);
    req_index = (ROTATE + req_index - rotor->offset) % ROTATE;

    return req_index;

}

/*
 * Run the enigma machine
 * /
int main(int argc, char *argv[])
{
    struct Enigma machine = {}; // initialized to defaults
    int i, c, req_index;

    // Command line options
    int opt_debug = 0;
    int opt_r1 = 3;
    int opt_r2 = 2;
    int opt_r3 = 1;
    int opt_o1 = 0;
    int opt_o2 = 0;
    int opt_o3 = 0;

    // Command Parsing
    for (i = 1; i < argc; i++){
        if (strcmp(argv[i], "-d") == 0) opt_debug = 1;
        if (strcmp(argv[i], "-r") == 0) {
            opt_r1 = atoi(&argv[i+1][0])/100;
            opt_r2 = atoi(&argv[i+1][1])/10;
            opt_r3 = atoi(&argv[i+1][2]);
            i++;
        }
        if (strcmp(argv[i], "-o") == 0) {
            opt_o1 = atoi(&argv[i+1][0])/100;
            opt_o2 = atoi(&argv[i+1][1])/10;
            opt_o3 = atoi(&argv[i+1][2]);
            i++;
        }
    }

    if(opt_debug) {
        printf("Rotors set to : %d %d %d \n", opt_r3, opt_r2, opt_r1);
        printf("Offsets set to: %d %d %d \n", opt_o3, opt_o2, opt_o1);
    }

    // Configure an enigma machine
    machine.reflector = reflectors[1];
    machine.rotors[0] = new_rotor(&machine, 3, 0);
    machine.rotors[1] = new_rotor(&machine, 2, 0);
    machine.rotors[2] = new_rotor(&machine, 1, 0);

    while((c = getchar())!=EOF) {

        if (!isalpha(c)) {
            printf("%c", c);
            continue;
        }

        c = toupper(c);

        // Plugboard
        req_index = str_index(alpha, c);
        if(opt_debug) {
            printf("Input c ******** %c \n", c);
        }

        // Cycle first rotor before pushing through,
        rotor_cycle(&machine.rotors[0]);

        // Double step the rotor
        if(str_index(machine.rotors[1].notch,
                    alpha[machine.rotors[1].offset]) >= 0 ) {
            rotor_cycle(&machine.rotors[1]);
        }

        // Stepping the rotors
        for(int i = 0; i < machine.numrotors - 1; i++) {
            c = alpha[machine.rotors[i].offset];

            if(machine.rotors[i].turnnext) {
                machine.rotors[i].turnnext = 0;
                rotor_cycle(&machine.rotors[i+1]);
                if(opt_debug) {
                    printf("Cycling  rotor :%d \n", i+1);
                    printf("Turnover rotor :%d \n", i);
                    printf("Character  is  :%c \n", c);
                }
            }
         }

        // Pass through all the rotors forward
        for(i=0; i < machine.numrotors; i++) {
            req_index = rotor_forward(&machine.rotors[i], req_index);
        }

        // Pass through the reflector
        if(opt_debug) {
            printf("Into reflector %c\n", alpha[index]);
            printf("Out of reflector %c\n", machine.reflector[index]);
        }

        // Inbound
        c = machine.reflector[index];
        // Outbound
        req_index = str_index(alpha, c);

        if(opt_debug) {
            printf("Index out of reflector %i\n", req_index);
            printf("->Reflected character %c \n", c);
        }

        // Pass back through the rotors in reverse
        for(i = machine.numrotors - 1; i >= 0; i--) {
            req_index = rotor_reverse(&machine.rotors[i], req_index);
        }

        // Pass through Plugboard
        c = alpha[index];

        if(opt_debug) {
           printf("Plugboard req_index %d \n", req_index);
           printf("Output character ******** ");
        }
        putchar(c);

        if(opt_debug) printf("\n\n");
    }

    return 0;
}//*/