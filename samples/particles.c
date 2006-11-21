/*
  BGI library implementation for Microsoft(R) Windows(TM)
  Copyright (C) 2006  Daniil Guitelson

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <graphics.h>
#include <math.h>

#define PARTICLE_NUMBER 25
#define PARTICLE_MASS 1
#define PARTICLE_RADIUS 5
#define TIME_DELTA .01

typedef struct tagVECTOR {
  double x, y;
} VECTOR;

typedef struct tagPARTICLE {
  VECTOR position;
  VECTOR speed;
} PARTICLE;

PARTICLE particles[PARTICLE_NUMBER] ; // main array;
PARTICLE particles2[PARTICLE_NUMBER] ;// second, temprory array

void calcInteraction(PARTICLE * active, PARTICLE * passive, VECTOR * accel) {
  accel->x = -(active->position.x - passive->position.x) / 10;
  accel->y = -(active->position.y - passive->position.y) / 10;
}


void vectorInit(VECTOR * vec, double x, double y) {
  vec->x = x;
  vec->y = y;
}

void vectorAdd(VECTOR * dest, VECTOR * toAdd) {
  dest->x += toAdd->x;
  dest->y += toAdd->y;
}

void updateParticle(PARTICLE * source, PARTICLE * dest, VECTOR * accel) {
  source->speed.x += accel->x * TIME_DELTA;
  source->speed.y += accel->y * TIME_DELTA;
  *dest = *source;
  dest->position.x += source->speed.x * TIME_DELTA;
  dest->position.y += source->speed.y * TIME_DELTA;
}

void updateParticles(PARTICLE * ps, PARTICLE * tmp) {
  int i, j;
  VECTOR accel;
  VECTOR summAccel;
  memcpy(tmp, ps, sizeof(PARTICLE) * PARTICLE_NUMBER);
  for(i = 0; i != PARTICLE_NUMBER; i++) {
    vectorInit(&summAccel, 0, 0);
    for(j = 0; j != PARTICLE_NUMBER; j++) {
      calcInteraction(ps + i, ps + j, &accel);
      vectorAdd(&summAccel, &accel);
    }
    updateParticle(ps + i, tmp + i, &summAccel);
  }
  memcpy(ps, tmp, sizeof(PARTICLE) * PARTICLE_NUMBER);
}

void drawParticles(PARTICLE * ps) {
  int i;
  int w = getmaxx() / 2;
  int h = getmaxy() / 2;
  for(i = 0; i != PARTICLE_NUMBER; i++) {
    setcolor((i % getmaxcolor()) + 1);
    circle(w + ps[i].position.x, h - ps[i].position.y, PARTICLE_RADIUS);
  }
}

void initParticle(PARTICLE * p) {
  p->position.x = PARTICLE_RADIUS + (rand() % (getmaxx() / 2 - PARTICLE_RADIUS)) - getmaxx() / 2;
  p->position.y = PARTICLE_RADIUS + (rand() % (getmaxy() / 2 - PARTICLE_RADIUS)) - getmaxy() / 2;
  p->speed.x = 0;
  p->speed.y = 0;
}

void initParticles(PARTICLE * ps) {
  int i;
  srand(clock());
  for(i = 0; i != PARTICLE_NUMBER; i++) {
    initParticle(ps + i);
  }
}

void main(void) {
  int gd = DETECT, gm = 0;
  int page = 0;
  initgraph(&gd, &gm, "");
  initParticles(particles);
  while(!anykeypressed()) {
    updateParticles(particles, particles2);
    setactivepage(1 - page);
    cleardevice();
    drawParticles(particles);
    setvisualpage(1 - page);
    page = 1 - page;
    delay(10);
  }
  closegraph();
}











