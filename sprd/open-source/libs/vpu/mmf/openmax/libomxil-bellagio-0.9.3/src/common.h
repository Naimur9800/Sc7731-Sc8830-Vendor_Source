/**
  src/common.h

  OpenMAX Integration Layer Core. This library implements the OpenMAX core
  responsible for environment setup, component tunneling and communication.

  Copyright (C) 2007-2011 STMicroelectronics
  Copyright (C) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).

  This library is free software; you can redistribute it and/or modify it under
  the terms of the GNU Lesser General Public License as published by the Free
  Software Foundation; either version 2.1 of the License, or (at your option)
  any later version.

  This library is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
  FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
  details.

  You should have received a copy of the GNU Lesser General Public License
  along with this library; if not, write to the Free Software Foundation, Inc.,
  51 Franklin St, Fifth Floor, Boston, MA
  02110-1301  USA

*/

#ifndef __COMMON_H__
#define __COMMON_H__

#define MAX_LINE_LENGTH 2048

int makedir(const char *newdir);

char *componentsRegistryGetFilename(void);
char* loadersRegistryGetFilename(char* registry_name);
int exists(const char* fname);

typedef struct nameList {
	char* name;
	struct nameList *next;
} nameList;


#endif

