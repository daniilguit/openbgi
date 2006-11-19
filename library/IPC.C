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

#include "IPC.h"
#include <windows.h>
#include <assert.h>


HANDLE IPC_createEvent(const char * name)
{
  return CreateEvent(NULL, FALSE, FALSE, name);
}

HANDLE IPC_openEvent(const char * name)
{
  return OpenEvent(EVENT_ALL_ACCESS, TRUE, name);
}

HANDLE IPC_createMutex(const char * name, int owned)
{
  return CreateMutex(NULL, owned, name);
}

HANDLE IPC_openMutex(const char * name)
{
  return OpenMutex(MUTEX_ALL_ACCESS, FALSE, name);
}

void IPC_raiseEvent(HANDLE event)
{
  SetEvent(event);
}

void IPC_waitEvent(HANDLE event)
{
  WaitForSingleObject(event, INFINITE);
}

void IPC_lockMutex(HANDLE mutex)
{
  WaitForSingleObject(mutex, INFINITE);
}

void IPC_unlockMutex(HANDLE mutex)
{
  ReleaseMutex(mutex);
}

HANDLE IPC_createSection(const char * name, int size)
{ 
  return CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, size, name);
}

HANDLE IPC_openSection(const char * name)
{
  return OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, name);
}

void * IPC_createSharedMemory(const char * name, int size)
{
  HANDLE sec = IPC_createSection(name, size);
  //assert(sec != 0);
  return MapViewOfFile(sec, FILE_MAP_ALL_ACCESS, 0, 0, 0);
}

void * IPC_openSharedMemory(const char * name)
{
  HANDLE sec = IPC_openSection(name);
  //assert(sec != 0);
  return MapViewOfFile(sec, FILE_MAP_ALL_ACCESS, 0, 0, 0);
}

void IPC_closeSharedMemory(void * addr)
{
  UnmapViewOfFile(addr);
}

