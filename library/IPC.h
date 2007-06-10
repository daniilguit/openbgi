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

#ifndef __IPC_H__
#define __IPC_H__

#include <windows.h>

/**
 * Wrappers around windows events API. Written just for more
 * meaning names.
 */
HANDLE IPC_createEvent(const char * name);
HANDLE IPC_openEvent(const char * name);
HANDLE IPC_createMutex(const char * name, int owned);
HANDLE IPC_openMutex(const char * name);
void IPC_lockMutex(HANDLE mutex);
void IPC_unlockMutex(HANDLE mutex);

void IPC_raiseEvent(HANDLE event);
void IPC_waitEvent(HANDLE event);

/**
 * Wrappers around windows inter process memory sharing API
 */
HANDLE IPC_createSection(const char * name, int size);
HANDLE IPC_openSection(const char * name);
void * IPC_createSharedMemory(const char * name, int size);
void * IPC_openSharedMemory(const char * name);
void IPC_closeSharedMemory(void * addr);

#endif
