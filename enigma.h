#ifndef ENIGMA_H
#define ENIGMA_H

#include <stdio.h>

extern const char *alpha;

extern const char *rotor_ciphers[];
extern const char *rotor_notches[];
extern const char *rotor_turnovers[];

extern const char *reflectors[];


struct Rotor {
    int             offset;
    int             turnnext;
    const char      *cipher;
    const char      *turnover;
    const char      *notch;
};

struct Enigma {
    int             numrotors;
    const char      *reflector;
    struct Rotor    rotors[8];
};

extern int str_index(const char *, int);
extern void rotor_cycle(struct Rotor *);
extern int rotor_forward(struct Rotor *, int);
extern int rotor_reverse(struct Rotor *, int);

#endif