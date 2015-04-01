#ifndef SONAR_H
#define SONAR_H

// Define this to
extern void Sonar_rxhandler(int16_t distance);

void Sonar_init();
void Sonar_fire();


#endif